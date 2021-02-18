#pragma once

#include <evs-sxe-core/Namespace.h>
#include <stdexcept>
#include <vector>
#include <string>

BEGIN_SXE_MODULE(core)

struct file
{
  /// @brief read a file's content into memory and return it split line by line
  static std::vector<std::string> read_lines(const std::string& p) throw(std::runtime_error);
};

END_SXE_MODULE(core)
