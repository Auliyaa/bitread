#include "settings.h"
#include <iostream>
#include <vector>

class my_settings: public settings
{
  using settings::settings;

  SETTING_ENTRY_NODEF(std::vector<std::string>, foo_bar , "foo.bar");
  SETTING_ENTRY(int, foo_bar2, "foo.bar2", 42);
  SETTING_ENTRY(int, foo_bar3, "foo.bar3", 42);
};

int main()
{
  my_settings s;
  std::cout << s.to_json().dump(3) << std::endl;

  return 0;
}
