#include <cpu.h>
#include <file.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <atomic>
#include <sstream>

using namespace os::topo;

std::vector<::size_t> cpu::_topo_all_cores{};
std::vector<::size_t> cpu::_topo_non_siblings_cores{};
std::map<::size_t, ::size_t> cpu::_topo_numa{};
std::map<::size_t, ::size_t> cpu::_topo_sibling{};
std::map<::size_t, ::size_t> cpu::_topo_base_core{};

void cpu::read_topology()
{
  static std::atomic_flag topo_read = ATOMIC_FLAG_INIT;
  if (topo_read.test_and_set()) return;

  // start by parsing /proc/cpuinfo and associate each core to a numa node.
  boost::regex processor_rx{"processor\\s+:\\s+([0-9]+)"};
  boost::regex physical_rx{"physical id\\s+:\\s+([0-9]+)"};

  auto cpuinfo_lines = file::read_lines("/proc/cpuinfo");
  std::int32_t cpu_id{-1};
  for (const auto& cpuinfo_line : cpuinfo_lines)
  {
    boost::smatch m;
    if (boost::regex_search(cpuinfo_line, m, processor_rx))
    {
      cpu_id = std::stoi(m[1].str());
      _topo_all_cores.emplace_back(cpu_id);
    }
    if (boost::regex_search(cpuinfo_line, m, physical_rx))
    {
      if (cpu_id != -1)
      {
        _topo_numa[cpu_id] = std::stoi(m[1]);
        cpu_id = -1;
      }
    }
  }

  // order cpu ids by their numa node first, then by their cpu ids.
  std::sort(_topo_all_cores.begin(), _topo_all_cores.end(), [&](const ::size_t& a, const ::size_t& b) -> bool
  {
    // compare numa node first
    if (_topo_numa[a] < _topo_numa[b]) return true;
    if (_topo_numa[a] > _topo_numa[b]) return false;
    // then core id if numa is the same
    return a < b;
  });

  // once all cpus have been associated to their numa, parse /sys/devices/system/cpu/cpu*/topology/thread_siblings_list
  //  in order to associate each core to its hyper-threaded sibling
  // note that hyper-threaded cores will not appear in the listing so that we can remove them from the cpu_ids list later on and only treat each core once.
  for (const auto& cpu_id : _topo_all_cores)
  {
    std::stringstream path;
    path << "/sys/devices/system/cpu/cpu" << cpu_id << "/topology/thread_siblings_list";
    auto lines = file::read_lines(path.str());
    if (lines.empty())
    {
      throw std::runtime_error{"Parsing error: " + path.str()};
    }

    std::vector<std::string> tkns;
    boost::split(tkns, lines[0], boost::is_any_of(","), boost::token_compress_on);
    if (tkns.empty())
    {
      throw std::runtime_error{"Parsing error: " + path.str()};
    }

    if (tkns.size() == 1)
    {
      _topo_sibling[cpu_id] = -1;
    }
    else if (tkns.size() == 2)
    {
      if (std::stoul(tkns[0]) == cpu_id)
      {
        _topo_sibling[cpu_id] = std::stoi(tkns[1]);
        _topo_base_core[std::stoi(tkns[1])] = cpu_id;
      }
    }
    else
    {
      throw std::runtime_error{"Parsing error: " + path.str()};
    }
  }

  // generate the list of base, non-sibling cores
  for (const auto& core_id : _topo_all_cores)
  {
    if (_topo_sibling.find(core_id) != _topo_sibling.end())
    {
      _topo_non_siblings_cores.emplace_back(core_id);
    }
  }
}

cpu::cpu(size_t id)
  : _id{id}
{
  read_topology();
}

cpu::stats_t cpu::stats() const
{
  std::string cpu_name = "cpu";
  if (_id != std::numeric_limits<::size_t>::max())
  {
    cpu_name += std::to_string(_id);
  }

  auto lines = file::read_lines("/proc/stat");
  for (const auto& line : lines)
  {
    // split each line into an array
    std::vector<std::string> tkns;
    boost::split(tkns, line, boost::is_any_of(" "), boost::token_compress_on);

    if (tkns[0] == cpu_name)
    {
      // cpu found: parse line to fetch cpu stats values
      stats_t r;
      r.user    = std::stoul(tkns[1]);
      r.nice    = std::stoul(tkns[2]);
      r.system  = std::stoul(tkns[3]);
      r.idle    = std::stoul(tkns[4]);
      r.iowait  = std::stoul(tkns[5]);
      r.irq     = std::stoul(tkns[6]);
      r.softirq = std::stoul(tkns[7]);
      return r;
    }
  }

  throw std::runtime_error("cpu id not found");
}

size_t cpu::nproc(bool with_siblings)
{
  return (with_siblings ? _topo_all_cores : _topo_non_siblings_cores).size();
}

cpu cpu::sibling() const
{
  auto lu = _topo_sibling.find(_id);
  return (lu != _topo_sibling.end() ? lu->second : _topo_base_core.find(_id)->second);
}

bool cpu::is_ht_sibling() const
{
  return _topo_sibling.find(_id) == _topo_sibling.end();
}

size_t cpu::stats_t::total() const
{
  return user + nice + system + idle + iowait + irq + softirq;
}

double cpu::stats_t::usage(const cpu::stats_t& before) const
{
  if (total() == before.total())
  {
    return 0;
  }

  return 1. - double(idle-before.idle)/double(total()-before.total());
}
