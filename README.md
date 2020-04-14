# bitread
A tiny C++ bit reader

Example test & usage:

```cpp
TEST(bitread, constructor_1)
{
  // 00110011 11001100 10101010 01010101
  bitread<uint32_t> br(0b00110011110011001010101001010101);

  EXPECT_EQ(0b01010101, br.get<uint8_t>(0,8));
  EXPECT_EQ(0b10101010, br.get<uint8_t>(8,8));
  EXPECT_EQ(0b11001100, br.get<uint8_t>(16,8));
  EXPECT_EQ(0b00110011, br.get<uint8_t>(24,8));
  EXPECT_EQ(0b01010101, br.get<uint16_t>(0,8));
  EXPECT_EQ(0b10101010, br.get<uint16_t>(8,8));
  EXPECT_EQ(0b11001100, br.get<uint16_t>(16,8));
  EXPECT_EQ(0b00110011, br.get<uint16_t>(24,8));
  EXPECT_EQ(0b01010101, br.get<uint32_t>(0,8));
  EXPECT_EQ(0b10101010, br.get<uint32_t>(8,8));
  EXPECT_EQ(0b11001100, br.get<uint32_t>(16,8));
  EXPECT_EQ(0b00110011, br.get<uint32_t>(24,8));
  EXPECT_EQ(0b01010101, br.get<uint64_t>(0,8));
  EXPECT_EQ(0b10101010, br.get<uint64_t>(8,8));
  EXPECT_EQ(0b11001100, br.get<uint64_t>(16,8));
  EXPECT_EQ(0b00110011, br.get<uint64_t>(24,8));
  EXPECT_EQ(0b0011    , br.get<uint8_t>(24,4));
  EXPECT_EQ(0b010     , br.get<uint8_t>(5,3));
}

TEST(bitread, constructor_2)
{
  // 00110011 11001100 10101010 01010101
  uint8_t payload[4];
  payload[0] = 0b00110011;
  payload[1] = 0b11001100;
  payload[2] = 0b10101010;
  payload[3] = 0b01010101;
  bitread<uint32_t> br(payload, 4);

  EXPECT_EQ(0b01010101, br.get<uint8_t>(0,8));
  EXPECT_EQ(0b10101010, br.get<uint8_t>(8,8));
  EXPECT_EQ(0b11001100, br.get<uint8_t>(16,8));
  EXPECT_EQ(0b00110011, br.get<uint8_t>(24,8));
  EXPECT_EQ(0b01010101, br.get<uint16_t>(0,8));
  EXPECT_EQ(0b10101010, br.get<uint16_t>(8,8));
  EXPECT_EQ(0b11001100, br.get<uint16_t>(16,8));
  EXPECT_EQ(0b00110011, br.get<uint16_t>(24,8));
  EXPECT_EQ(0b01010101, br.get<uint32_t>(0,8));
  EXPECT_EQ(0b10101010, br.get<uint32_t>(8,8));
  EXPECT_EQ(0b11001100, br.get<uint32_t>(16,8));
  EXPECT_EQ(0b00110011, br.get<uint32_t>(24,8));
  EXPECT_EQ(0b01010101, br.get<uint64_t>(0,8));
  EXPECT_EQ(0b10101010, br.get<uint64_t>(8,8));
  EXPECT_EQ(0b11001100, br.get<uint64_t>(16,8));
  EXPECT_EQ(0b00110011, br.get<uint64_t>(24,8));
  EXPECT_EQ(0b0011    , br.get<uint8_t>(24,4));
  EXPECT_EQ(0b010     , br.get<uint8_t>(5,3));
}
```
