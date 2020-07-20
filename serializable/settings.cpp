#include "settings.h"

#include <fstream>
#include <boost/algorithm/string.hpp>

settings::settings(const std::string& default_conf_path, const std::string& conf_path)
{
  static auto read_json = [](const std::string& path) -> sxe::nlohmann::json
  {
    std::ifstream contents(path);
    sxe::nlohmann::json result;
    contents >> result;
    return result;
  };

  if (!default_conf_path.empty())
  {
    auto json = read_json(default_conf_path).flatten();
    for (const auto& kv : json.get<sxe::nlohmann::json::object_t>())
    {
      auto k = kv.first;
      boost::replace_all(k, "/", ".");
      _data[k].value = kv.second;
    }
  }

  if (!conf_path.empty())
  {
    try
    {
      auto json = read_json(conf_path).flatten();
      for (const auto& kv : json.get<sxe::nlohmann::json::object_t>())
      {
        auto k = kv.first;
        boost::replace_all(k, "/", ".");
        _data[k].value = kv.second;
      }
    }
    catch(const std::exception& x)
    {}
  }
}

sxe::nlohmann::json settings::to_json() const
{
  sxe::nlohmann::json j;
  for (const auto& kv : _data)
  {
    std::vector<std::string> tkns;
    boost::split(tkns, kv.first, boost::is_any_of("."), boost::token_compress_on);
    auto* p = &j;
    for (size_t ii{0}; ii < tkns.size()-1; ++ii)
    {
      auto& tkn = tkns[ii];
      if (tkn.empty()) continue;

      (*p)[tkn] = sxe::nlohmann::json{};
      p = &((*p)[tkn]);
    }

    (*p)[tkns[tkns.size()-1]] = kv.second.value;
  }
  return j;
}

sxe::nlohmann::json settings::to_defaults_json() const
{
  sxe::nlohmann::json j;
  for (const auto& kv : _data)
  {
    std::vector<std::string> tkns;
    boost::split(tkns, kv.first, boost::is_any_of("."), boost::token_compress_on);
    auto* p = &j;
    for (size_t ii{0}; ii < tkns.size()-1; ++ii)
    {
      auto& tkn = tkns[ii];
      if (tkn.empty()) continue;

      (*p)[tkn] = sxe::nlohmann::json{};
      p = &((*p)[tkn]);
    }

    (*p)[tkns[tkns.size()-1]] = kv.second.default_value;
  }
  return j;
}

std::vector<std::string> settings::keys() const
{
  std::vector<std::string> result;
  for (const auto& kv : _data)
  {
    result.emplace_back(kv.first);
  }
  return result;
}

bool settings::has_key(const std::string& key) const
{
  return _data.find(key) != _data.end();
}

bool settings::is_default(const std::string& key) const
{
  auto l = _data.find(key);
  return l != _data.end() ? (l->second.value == l->second.default_value) : true;
}

void* settings::__reg_value_(const std::string& key,
                               const sxe::nlohmann::json& d,
                               std::map<std::string, settings_value>& store)
{
  store[key] = settings_value(d);
  return nullptr;
}
