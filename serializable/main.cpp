#include "settings.h"
#include <iostream>

class my_settings: public settings
{
  using settings::settings;

  SETTING_ENTRY(int, foo_bar , "foo.bar" , 42);
  SETTING_ENTRY(int, foo_bar2, "foo.bar2", 42);
  SETTING_ENTRY(int, foo_bar3, "foo.bar3", 42);
};

int main()
{
  my_settings s;
  std::cout << s.to_json().dump(3) << std::endl;

  return 0;
}
