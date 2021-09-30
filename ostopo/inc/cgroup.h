#pragma once

#include <string>
#include <list>

#include <cpu.h>

namespace os::topo
{

/**
 * @brief Wraps information about a registered cgroup
 * @see man cgconfig.conf
 * @see http://man7.org/linux/man-pages/man7/cpuset.7.html
 */
class cgroup
{
  std::string            _name;                       // unique name of the cgroup

  std::vector<::size_t>  _cpu_ids;                    // list of restricted cores
  bool                   _cpu_exclusive = false;      // true if the group has an exclusive use of these cpus
  std::vector<::size_t>  _cpu_mems;                   // list of restricted memory buses (socket id)
  bool                   _cpu_mems_exclusive = false; // true is the memory bus has an exclusive use of these memory buses

public:
  cgroup(const std::string& name);
  cgroup(const cgroup&)=default;
  cgroup(cgroup&&)=default;
  cgroup& operator=(const cgroup&)=default;
  cgroup& operator=(cgroup&&)=default;
  virtual ~cgroup()=default;

  /// @return the list of cpus assigned to this group
  std::list<os::topo::cpu> cpuset() const;
  /// @return true if this cgroup  is cpu exclusive
  bool exclusive() const;
  /// @return the list of cpumems assigned to this group
  std::list<::size_t> cpumems() const;
  /// @return true if cpumems are marked as exclusive
  bool cpumems_exclusive() const;
};

} // os::topo
