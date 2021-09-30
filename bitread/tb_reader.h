#include <cstdint>
#include <cstdlib>

/// Parses 10-bits words from a source 8-bits buffer.
struct tb_reader
{
  const uint8_t* buf;
  size_t buf_off;

  /// Use the provided 8-bits buffer as a source and starts at the specified offset, in bits
  inline tb_reader(const uint8_t* buf, size_t offset=0): buf(buf), buf_off(offset) {}
  tb_reader(const tb_reader&) = default;
  tb_reader(tb_reader&&) = default;
  ~tb_reader() = default;

  /// Fetches the nth 10-bit word from the buffer, starting from the leftmost bit
  inline uint16_t operator[](size_t i)
  {
    // locate first bit to copy
    size_t buf_pos_bit  = buf_off + i * 10; // absolute pos of the first bit in the buffer
    size_t buf_pos_byte = buf_pos_bit / 8;  // pos of the enclosing byte in the buffer

    // see res/tb_reader.drawio for a visual representation of all the cases.
    // each case is split to allow for both clarity and later optimization of individual cases.

    switch(buf_pos_bit % 8) // position of the first bit to include in the first buffer byte
    {
    case 0: return 0b0000001111111111 & ((static_cast<uint16_t>(buf[buf_pos_byte]) << 2) | (static_cast<uint16_t>(buf[buf_pos_byte+1]) >> 6));
    case 1: return 0b0000001111111111 & ((static_cast<uint16_t>(buf[buf_pos_byte]) << 3) | (static_cast<uint16_t>(buf[buf_pos_byte+1]) >> 5));
    case 2: return 0b0000001111111111 & ((static_cast<uint16_t>(buf[buf_pos_byte]) << 4) | (static_cast<uint16_t>(buf[buf_pos_byte+1]) >> 4));
    case 3: return 0b0000001111111111 & ((static_cast<uint16_t>(buf[buf_pos_byte]) << 5) | (static_cast<uint16_t>(buf[buf_pos_byte+1]) >> 3));
    case 4: return 0b0000001111111111 & ((static_cast<uint16_t>(buf[buf_pos_byte]) << 6) | (static_cast<uint16_t>(buf[buf_pos_byte+1]) >> 2));
    case 5: return 0b0000001111111111 & ((static_cast<uint16_t>(buf[buf_pos_byte]) << 7) | (static_cast<uint16_t>(buf[buf_pos_byte+1]) >> 1));
    case 6: return 0b0000001111111111 & ((static_cast<uint16_t>(buf[buf_pos_byte]) << 8) | static_cast<uint16_t>(buf[buf_pos_byte+1])       );
    case 7: return 0b0000001111111111 & ((static_cast<uint16_t>(buf[buf_pos_byte]) << 9) | (static_cast<uint16_t>(buf[buf_pos_byte+1]) << 1) | (static_cast<uint16_t>(buf[buf_pos_byte+2]) >> 7));
    }

    return 0;
  }
};
