#pragma once

#include <cpu.h>
#include <vector>
#include <map>
#include <list>
#include <cstdlib>

namespace os::topo
{

/// @brief wraps information about a specific process from /proc/<pid>
class pid
{
  // the pid itself. must be valid.
  ::pid_t _id;

  // returns the path the stat file in the /proc filesystem.
  std::string stat_path() const;
  // convenience method to read & split the stat file.
  static std::vector<std::string> read_stat(const std::string& p);
public:
  /// @brief wraps the stats (/proc/<pid>/stat) for a pid and is used to compute cpu usage for a given process.
  struct stats_t
  {
    /// @brief time spent (in jiffies) in userspace
    ::size_t utime;
    /// @brief time spent (in jiffies) in kernelspace
    ::size_t stime;
    /// @brief global cpu time use as reference to compute cpu usage
    ::size_t cputime;

    stats_t()=default;
    stats_t(const stats_t&)=default;
    stats_t(stats_t&&)=default;
    virtual ~stats_t()=default;
    stats_t& operator=(const stats_t&)=default;
    stats_t& operator=(stats_t&&)=default;

    /// @brief sums utime & stime & gives the total non-idle time (in jiffies) spent by this process/thread
    ::size_t total() const;
    /// @brief given a previous stats snapshot, computes the cpu usage ratio. returns a value between 0 & 1
    double usage(const stats_t& before) const;
  };

  /// @brief wraps information about a task in a pid (aka. a thread).
  class task
  {
    // parent pid
    pid_t _id;
    // task pid (or tid)
    pid_t _tid;
    // path to the stat file for this task (/proc/<pid>/task/<tid>/stat)
    std::string stat_path() const;

  public:
    task(::pid_t pid, ::pid_t tid);
    task(const task&)=default;
    task(task&&)=default;
    virtual ~task()=default;
    task& operator=(const task&)=default;
    task& operator=(task&&)=default;

    /// @return the parent process id
    inline ::pid_t id() const { return _id; }
    /// @return this task id (tid)
    inline ::pid_t tid() const { return _tid; }
    /// @return the stats snapshot for this task. throws if either the pid or the tid is invalid.
    os::topo::pid::stats_t stats() const;
  };

  pid(::pid_t id);
  pid(const pid&)=default;
  pid(pid&&)=default;
  virtual ~pid()=default;
  pid& operator=(const pid&)=default;
  pid& operator=(pid&&)=default;

  /// @return the process id.
  inline pid_t id() const { return _id; }
  /// @return the tcomm column of the stat file, which corresponds to the filename for this process.
  std::string tcomm() const;
  /// @return the stats snapshot for the whole process.
  os::topo::pid::stats_t stats() const;
  /// @return all tasks associated to this process.
  std::vector<os::topo::pid::task> tasks() const;
  /// @return all stats for all tasks associated to this pid
  std::map<pid_t, stats_t> tasks_stats() const;
  /// @return the cpu core affinity list of this pid
  std::list<os::topo::cpu> affinity() const;
  /// @brief assigns a cpuset affinity for the current pid
  void set_affinity(const std::list<os::topo::cpu>& cpus);
};

} // os::topo
