#include <type_traits>
#include <cstdint>
#include <cstdlib>

/**
 * Given a buffer stored onto n bits, this class allows reading a value starting at an arbitraty index (in bits) with an arbitrary size (in bits)
 * Based over: https://stackoverflow.com/questions/11815894/how-to-read-write-arbitrary-bits-in-c-c
 */
template<typename buf_t>
struct bitread {
  /// Data buffer (must be integral, data size depends on the template parameter)
  buf_t buf;

  /// Creates a reader from an arbitrary value/
  /// e.g:
  /// bitread<uint16_t> b(0b1111000000001111);
  /// b.get<uint8_t>(2,4); // -> 0b0011
  bitread(buf_t b)
    : buf(b)
  {
    static_assert(std::is_integral<decltype(buf)>::value, "integral required");
  }

  /// Parses a char buffer and append each char in a single integer value.
  /// e.g:
  /// uint8_t buf[2]; buf[0] = 0b11110000; buf[1] = 0b00001111;
  /// bitread<uint16_t> b(buf, 2); // b.buf == 0b1111000000001111
  bitread(const uint8_t* p, size_t sz)
  {
    static_assert(std::is_integral<decltype(buf)>::value, "integral required");

    buf = 0;
    for(size_t ii=0; ii < sz; ++ii)
    {
      buf |= static_cast<decltype(buf)>(p[ii]) << ((sizeof(decltype(buf)) - (1+ii)) * 8);
    }
  }

  bitread() = delete;
  bitread(const bitread&) = default;
  bitread(bitread&&) = default;
  virtual ~bitread() = default;

  /// Fetch a value starting at index idx (starting from the most LSB) and reads up to sz bits
  /// e.g:
  /// uint8_t buf[2]; buf[0] = 0b11110000; buf[1] = 0b00001111;
  /// bitread<uint16_t> b(buf, 2); // b.buf == 0b1111000000001111
  /// b.get<uint8_t>(7,7); // -> 0b1100000
  template<typename T> inline T get(decltype(buf) idx, decltype(buf) sz) const
  {
    return static_cast<T>((((buf) & (((static_cast<decltype(buf)>(1) << (sz)) - static_cast<decltype(buf)>(1)) << (idx))) >> (idx)));
  }
};
