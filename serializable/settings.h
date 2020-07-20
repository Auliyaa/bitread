#pragma once

#include "json.h"

struct settings_value
{
  sxe::nlohmann::json value;
  sxe::nlohmann::json default_value;

  settings_value() = default;
  settings_value(const settings_value&) = default;
  settings_value(settings_value&&) = default;
  settings_value(const sxe::nlohmann::json& default_value) : value(default_value), default_value(default_value) {}

  settings_value& operator=(const settings_value&) = default;

  virtual ~settings_value() = default;
};

class settings
{
public:
  settings(const std::string& default_conf_path=std::string{""}, const std::string& conf_path=std::string{""});
  settings(const settings&) = default;
  settings(settings&&) = default;

  settings& operator=(const settings&) = default;

  virtual ~settings() = default;

  sxe::nlohmann::json to_json() const;
  sxe::nlohmann::json to_defaults_json() const;

  std::vector<std::string> keys() const;
  bool has_key(const std::string& key) const;
  bool is_default(const std::string& key) const;

  template<typename T>
  T get_default(const std::string& key) const
  {
    const auto l = _data.find(key);
    if (l == _data.end())
    {
      return T();
    }
    return l->second.default_value;
  }

  template<typename T>
  T get(const std::string& key) const
  {
    const auto l = _data.find(key);
    if (l == _data.end())
    {
      return T();
    }
    return l->second.value.get<T>();
  }

  inline operator sxe::nlohmann::json() const { return to_json(); }
  inline operator std::string() const { return to_json().dump(); }

protected:
  std::map<std::string, settings_value> _data;
  static void* __reg_value_(const std::string& key, const sxe::nlohmann::json& d, std::map<std::string, settings_value>& store);
};

#define SETTING_ENTRY(type, name, key, default_value)\
public:\
  inline type get_##name##_default() const { return default_value; }\
  inline type get_##name() const { return get<type>(key); }\
  inline void set_##name(const type& name##_value) { _data[key].value = name##_value; }\
  inline std::string key_##name() const { return key; }\
private:\
  void* __##name##__handle__{__reg_value_(key, default_value, _data)};

