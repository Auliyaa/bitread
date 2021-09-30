#pragma once

#include <stdexcept>
#include <vector>
#include <string>

namespace os::topo
{

struct file
{
  /// @brief read a file's content into memory and return it split line by line
  static std::vector<std::string> read_lines(const std::string& p);
};

} // os::topo
