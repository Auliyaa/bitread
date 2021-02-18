#pragma once

#include <evs-sxe-core/Namespace.h>

#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <vector>
#include <map>

BEGIN_SXE_MODULE(core)

/// @brief maps cpuinfo from /proc/cpuinfo & /proc/stat
class cpu
{
  // topology
  static std::vector<::size_t> _topo_all_cores;          // all cores, ordered by numa, then by id
  static std::vector<::size_t> _topo_non_siblings_cores; // all cores (without threaded siblings), ordered by numa, then by id
  static std::map<::size_t, ::size_t> _topo_numa;      // core-id -> numa-id
  static std::map<::size_t, ::size_t> _topo_sibling;   // core-id -> threaded sibling core-id
  static std::map<::size_t, ::size_t> _topo_base_core; // threaded sibling -> core id

  // core id
  ::size_t _id;

  // parse system core topology
  static void read_topology();
public:
  /// @brief use this id to fetch global cpu information & usage
  static constexpr const size_t global_cpu_id{std::numeric_limits<::size_t>::max()};

  /// @brief cpu stats gives the time (in jiffies) spent by each core in specific spaces & is used to compute cpu usage in %
  struct stats_t
  {
    ::size_t user;
    ::size_t nice;
    ::size_t system;
    ::size_t idle;
    ::size_t iowait;
    ::size_t irq;
    ::size_t softirq;

    stats_t()=default;
    stats_t(const stats_t&)=default;
    stats_t(stats_t&&)=default;
    virtual ~stats_t()=default;
    stats_t& operator=(const stats_t&)=default;
    stats_t& operator=(stats_t&&)=default;

    /// @brief sums all cpu times
    ::size_t total() const;

    /// @brief given a previous usage snapshot, computes the core usage ratio. returns a value between 0 & 1
    double usage(const stats_t& before) const;
  };

  cpu(::size_t id);
  cpu(const cpu&)=default;
  cpu(cpu&&)=default;
  virtual ~cpu()=default;
  cpu& operator=(const cpu&)=default;
  cpu& operator=(cpu&&)=default;

  /// @return the core id
  inline size_t id() const { return _id; }

  /// @return the core stats snapshot. throws if the core id is invalid.
  SXE_NAMESPACE::core::cpu::stats_t stats() const throw(std::runtime_error);

  /// @return the number of cpu cores reported by /proc/stat.
  static size_t nproc(bool with_siblings=true);

  /// @return either the threaded sibling (if the current instance is a non-sibling core), or the base core.
  cpu sibling() const;

  /// @return true if this cpu core is the threaded sibling of a physical compute unit
  bool is_ht_sibling() const;
};

END_SXE_MODULE(core)
