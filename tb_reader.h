/// Parses 10-bits words from a source 8-bits buffer.
struct tb_reader
{
  const uint8_t* buf;
  size_t buf_off;

  /// Use the provided 8-bits buffer as a source and starts at the specified offset, in bits
  tb_reader(const uint8_t* buf, size_t offset=0)
    : buf(buf), buf_off(offset)
  {
  }

  tb_reader(const tb_reader&) = default;
  tb_reader(tb_reader&&) = default;
  ~tb_reader() = default;

  /// Fetches the nth 10-bit word from the buffer, starting from the leftmost bit
  uint16_t operator[](size_t i)
  {
    uint16_t r = 0b0000000000000000;

    // locate first bit to copy
    size_t buf_pos_bit  = buf_off + i * 10; // absolute pos of the first bit in the buffer
    size_t buf_pos_byte = buf_pos_bit / 8;  // pos of the enclosing byte in the buffer
    size_t byte_pos_bit = buf_pos_bit % 8;  // position of the first bit to include in the first buffer byte

    // copy bits from the first buffer byte
    r |= static_cast<uint16_t>(buf[buf_pos_byte]) << (2 + byte_pos_bit);

    size_t remaining_bits = 10 - (8-byte_pos_bit);
    while (remaining_bits > 0)
    {
      ++buf_pos_byte;
      if (remaining_bits <= 8)
      {
        r |= (static_cast<uint16_t>(buf[buf_pos_byte]) >> (8-remaining_bits));
        break;
      }
      else
      {
        r |= (static_cast<uint16_t>(buf[buf_pos_byte]) << (remaining_bits-8));
        remaining_bits -= 8;
      }
    }

    return r & 0b0000001111111111;
  }
};
