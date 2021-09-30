#include <cgroup.h>

#include <algorithm>
#include <sstream>
#include <fstream>

using namespace os::topo;

// filesystem reading helper
// convert to number
template<class Number,
         class = typename std::enable_if<std::is_integral<Number>::value>::type>
static bool fs_convert(std::string line, Number &t)
{
  t = static_cast<Number>(std::stoi(line));
  return true;
}
// convert list of interval and value (1,3-6,9 => [1,3,4,5,6,9]
bool fs_convert(std::string line, std::vector<::size_t> &t)
{
  size_t initial_size = t.size();
  // split on ','
  std::stringstream ss(line);
  std::string item;
  while (std::getline(ss, item, ','))
  {
    size_t interval_index = item.find('-');
    // max is excluded
    size_t min = 0, max = 0;
    if (interval_index == std::string::npos)
    {
      min = std::stoul(item);
      max = min+1;
    }
    else
    {
      min = std::stoul(item.substr(0,interval_index));
      max = std::stoul(item.substr(interval_index+1))+1;
    }
    for (auto i = min; i < max; ++i)
    {
      t.push_back(i);
    }
  }
  return t.size() != initial_size;
}

template<typename T>
bool fs_read(std::string filename, T &t)
{
  bool res = false;
  std::string line;
  std::ifstream f(filename);
  if (std::getline(f, line))
  {
    res = fs_convert(line, t);
  }
  return res;
}

cgroup::cgroup(const std::string& name)
{
  // open the system description files for cp infos
  std::string cgroup_cpu_path = std::string("/sys/fs/cgroup/cpuset") + "/" + name + "/";
  // read infos from filesystem
  fs_read(cgroup_cpu_path + "cpuset.cpus", _cpu_ids);
  fs_read(cgroup_cpu_path + "cpuset.cpu_exclusive", _cpu_exclusive);
  fs_read(cgroup_cpu_path + "cpuset.mems", _cpu_mems);
  fs_read(cgroup_cpu_path + "cpuset.mem_exclusive", _cpu_mems_exclusive);

  // sort result
  std::sort(_cpu_ids.begin(), _cpu_ids.end());
  std::sort(_cpu_mems.begin(), _cpu_mems.end());
}

std::list<cpu> cgroup::cpuset() const
{
  std::list<cpu> r;
  for (auto id : _cpu_ids)
  {
    r.emplace_back(cpu(id));
  }
  return r;
}

bool cgroup::exclusive() const
{
  return _cpu_exclusive;
}

std::list<::size_t> cgroup::cpumems() const
{
  std::list<::size_t> r;
  for (auto id : _cpu_mems)
  {
    r.emplace_back(id);
  }
  return r;
}

bool cgroup::cpumems_exclusive() const
{
  return _cpu_mems_exclusive;
}
