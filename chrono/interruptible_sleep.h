template<typename R1, typename P1,
         typename R2=int64_t, typename P2=std::ratio<1,1000>>
bool _sleep_for(const std::chrono::duration<R1,P1>& duration,
                bool& sw,
                const std::chrono::duration<R2,P2>& check_frequency = std::chrono::milliseconds(500))
{
  auto start = std::chrono::steady_clock::now();
  while(sw)
  {
    auto now = std::chrono::steady_clock::now();
    if (now-start >= duration)
    {
      return sw;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  return sw;
}
