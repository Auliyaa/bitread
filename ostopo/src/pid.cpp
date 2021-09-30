#include <pid.h>
#include <file.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

using namespace os::topo;

// pid::stats_t
size_t pid::stats_t::total() const
{
  return utime + stime;
}

double pid::stats_t::usage(const pid::stats_t& before) const
{
  if (cputime == before.cputime)
  {
    return 0;
  }
  // see https://stackoverflow.com/questions/1420426/how-to-calculate-the-cpu-usage-of-a-process-by-pid-in-linux-from-c/1424556
  return double(total() - before.total()) / double(cputime - before.cputime);
}

// pid::task
std::string pid::task::stat_path() const
{
  return "/proc/" + std::to_string(_id) + "/task/" + std::to_string(_tid) + "/stat";
}

pid::task::task(pid_t pid, pid_t tid)
  : _id(pid), _tid(tid)
{
}

os::topo::pid::stats_t pid::task::stats() const
{
  auto tkns = pid::read_stat(stat_path());
  stats_t r;
  r.utime = std::stoul(tkns[13]);
  r.stime = std::stoul(tkns[14]);
  r.cputime = cpu(cpu::global_cpu_id).stats().total();
  return r;
}

// pid
std::string pid::stat_path() const
{
  return "/proc/" + std::to_string(_id) + "/stat";
}

std::vector<std::string> pid::read_stat(const std::string& p)
{
  auto lines = file::read_lines(p);
  std::vector<std::string> tkns;
  boost::split(tkns, lines[0], boost::is_any_of(" "), boost::token_compress_on);
  return tkns;
}

pid::pid(pid_t id)
  : _id(id)
{
}

std::string pid::tcomm() const
{
  return read_stat(stat_path())[1];
}

os::topo::pid::stats_t pid::stats() const
{
  auto tkns = read_stat(stat_path());
  stats_t r;
  r.utime = std::stoul(tkns[13]);
  r.stime = std::stoul(tkns[14]);
  // cputime is the sum of all cpu time assigned to this pid
  r.cputime = 0;
  for (const auto& cpu : affinity())
  {
    r.cputime += cpu.stats().total();
  }
  return r;
}

std::vector<os::topo::pid::task> pid::tasks() const
{
  std::vector<os::topo::pid::task> r;

  // tids are located under the /task subfolder
  boost::filesystem::path p("/proc/" + std::to_string(_id) + "/task/");
  for (const auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(p), {}))
  {
    pid_t tid = std::stoul(entry.path().filename().string());
    r.emplace_back(pid::task{_id, tid});
  }

  return r;
}

std::map<pid_t, os::topo::pid::stats_t> pid::tasks_stats() const
{
  std::map<pid_t, os::topo::pid::stats_t> r;
  for (const auto t : tasks())
  {
    r[t.tid()] = t.stats();
  }
  return r;
}

std::list<cpu> pid::affinity() const
{
  std::list<cpu> r;
  cpu_set_t c;
  CPU_ZERO(&c);
  sched_getaffinity(_id, sizeof(cpu_set_t), &c);
  for (size_t ii=0; ii<cpu::nproc(); ++ii)
  {
    if (CPU_ISSET(ii, &c))
    {
      r.push_back(cpu(ii));
    }
  }
  return r;
}

void pid::set_affinity(const std::list<cpu>& cpus)
{
  // prepare cpuset
  cpu_set_t mask;
  CPU_ZERO(&mask);
  std::for_each(cpus.begin(), cpus.end(), [&mask](const cpu& c) {CPU_SET(c.id(), &mask);});

  auto tids = tasks();
  int res=0;
  std::for_each(tids.begin(), tids.end(), [&mask,&res](const task &t)
  {
    int r = sched_setaffinity(t.id(), sizeof(cpu_set_t), &mask);
    if (r != 0) res = errno;
  });

  if (res != 0)
  {
    throw std::runtime_error("Failed to set affinity on process #" + std::to_string(id()) + ": " + strerror(errno));
  }
}
