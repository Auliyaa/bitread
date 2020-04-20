template<typename F>
/**
 * @brief Shares a single instance of a given object (as a shared ptr) amongst multiple section of the runtime.
 * Each instance has a common use_count and is properly disconnected when nobody uses it anymore.
 */
struct static_shared_ptr
{
  template<typename ...Args>
  /// Fetches the global instance.
  static inline std::shared_ptr<F> get(Args... a)
  {
    return getp([](Args... a) { return std::make_shared<F>(a...); }, a...);
  }

  template<class P, typename ...Args>
  /// Fetches the global instance using an external provider to construct the shared_ptr to F.
  static inline std::shared_ptr<F> getp(P p, Args... a)
  {
    auto r = _inst.lock();

    if (r == nullptr)
    {
      _inst = r = p(a...);
    }

    return r;
  }

private:
  static std::weak_ptr<F> _inst;
};

template<typename F> std::weak_ptr<F> static_shared_ptr<F>::_inst;
