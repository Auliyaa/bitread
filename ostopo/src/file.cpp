#include <evs-sxe-core/system/file.h>
#include <boost/algorithm/string.hpp>

#include <fstream>

using namespace SXE_NAMESPACE::core;

std::vector<std::string> file::read_lines(const std::string& path) throw(std::runtime_error)
{
  std::ifstream fd;
  fd.open(path, std::ios::in);
  if (!fd.is_open())
  {
    throw std::runtime_error{"Failed to open " + path + " for reading."};
  }
  std::string buf((std::istreambuf_iterator<char>(fd)), std::istreambuf_iterator<char>());
  fd.close();

  std::vector<std::string> res;
  return boost::split(res, buf, boost::is_any_of("\n"), boost::token_compress_on);
}
