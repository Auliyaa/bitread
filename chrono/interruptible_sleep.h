/// a interruptible sleep function based on a bool-castable switch
template<typename R1, typename P1,
         typename SW,
         typename R2=int64_t, typename P2=std::ratio<1,1000>>
bool sleep_for_inter(const std::chrono::duration<R1,P1>& duration,
                     SW& sw,
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
    std::this_thread::sleep_for(std::min(check_frequency, duration - now - start));
  }
  return sw;
}
