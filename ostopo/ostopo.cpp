#include <cpu.h>

#include <iostream>
#include <thread>

int main()
{
  os::topo::cpu c{0};
  auto stats_0 = c.stats();
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto stats_1 = c.stats();
    auto usage = stats_1.usage(stats_0);
    std::cout << "CPU #0: " << usage << std::endl;
    stats_0 = stats_1;
  }
}
