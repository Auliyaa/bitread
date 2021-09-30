#include <iostream>

auto map_range = [](double x, double in_min, double in_max, double out_min, double out_max) -> double
{
  return (x-in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
};

int main()
{
  auto test = [](double x, double in_min, double in_max, double out_min, double out_max)
  {
    std::cout << x << " [" << in_min << "," << in_max << "]"
              << " -> [" << out_min << "," << out_max << "]="
              << map_range(x,in_min,in_max,out_min,out_max) << std::endl;
  };

  test(5,0,10,20,40);
  test(18,0,100,0,10);

  return 0;
}
