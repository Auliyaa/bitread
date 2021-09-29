#pragma once

#include <array>

// V210 format helpers
struct v210
{
  // returns {Y,U,V} value for given x,y coordinate
  inline static std::array<uint16_t,3> read(const uint8_t* const data, size_t x, size_t y, size_t w)
  {
    return read_rb(data,x,y,row_size(w));
  }
  static std::array<uint16_t,3> read_rb(const uint8_t* const data, size_t x, size_t y, size_t rowsize);

  struct group_t
  {
    uint16_t y0,y1,y2,y3,y4,y5;
    uint16_t u0_1,u2_3,u4_5;
    uint16_t v0_1,v2_3,v4_5;
  };

  // returns a group of 6 components starting at x,y
  static group_t read_group(const uint8_t* const data, size_t x, size_t y, size_t rowsize);

  // size of a row (in bytes)
  static size_t row_size(size_t w);

  // size of a frame (in bytes)
  static size_t frame_size(size_t w, size_t h);
};

// yuv422p10le helpers
// planar Y:U:V 4:2:2 16 bits per component. 10 low bits are significant
struct yuv422p10le
{
  // size of a frame (in bytes)
  static size_t frame_size(size_t w, size_t h);

  // plane offsets (in bytes)
  static std::array<size_t,3> plane_offsets(size_t w, size_t h);

  // returns {Y,U,V} value for given x,y coordinate
  inline static std::array<uint16_t,3> read(const uint8_t* const data, size_t x, size_t y, size_t w, size_t h)
  {
    return read(data,x,y,w,plane_offsets(w,h));
  }
  static std::array<uint16_t,3> read(const uint8_t* const data, size_t x, size_t y, size_t w, const std::array<size_t,3>& planes_off);

  // sets {Y,U,V} value for given x,y coordinate
  inline static void write(uint8_t* data, const std::array<uint16_t,3>& yuv, size_t x, size_t y, size_t w, size_t h)
  {
    write(data,yuv,x,y,w,plane_offsets(w,h));
  }
  static void write(uint8_t* data, const std::array<uint16_t,3>& yuv, size_t x, size_t y, size_t w, const std::array<size_t,3>& planes_off);

  // sets {Y0,U0+1,V0+1} value for given x,y coordinates
  static void write2(uint8_t* data,
                     uint16_t y0,
                     uint16_t y1,
                     uint16_t u0_1,
                     uint16_t v0_1,
                     size_t x,
                     size_t y,
                     size_t w,
                     const std::array<size_t,3>& planes_off);
};

// yuv444 helpers
// planar Y:U:V 4:4:4 8 bits per component.
struct yuv444p
{
  // size of a frame (in bytes)
  static size_t frame_size(size_t w, size_t h);

  // plane offsets (in bytes)
  static std::array<size_t, 3> plane_offsets(size_t w, size_t h);

  // returns {Y,U,V} value for given x,y coordinate
  inline static std::array<uint8_t,3> read(const uint8_t* const data, size_t x, size_t y, size_t w, size_t h)
  {
    return read(data,x,y,w,plane_offsets(w,h));
  }
  static std::array<uint8_t,3> read(const uint8_t* const data, size_t x, size_t y, size_t w, const std::array<size_t,3>& planes_off);

  // sets {Y,U,V} value for given x,y coordinate
  inline static void write(uint8_t* data, const std::array<uint8_t,3>& yuv, size_t x, size_t y, size_t w, size_t h)
  {
    write(data,yuv,x,y,w,plane_offsets(w,h));
  }
  static void write(uint8_t* data, const std::array<uint8_t,3>& yuv, size_t x, size_t y, size_t w, const std::array<size_t,3>& planes_off);
  inline static void write(uint8_t* data, const std::array<uint16_t,3>& yuv, size_t x, size_t y, size_t w, const std::array<size_t,3>& planes_off)
  {
    write(data,std::array<uint8_t,3>{ static_cast<uint8_t>(yuv[0]),
                                      static_cast<uint8_t>(yuv[1]),
                                      static_cast<uint8_t>(yuv[1]) }, x, y, w, planes_off);
  }

  // sets {Y0,U0+1,V0+1} value for given x,y coordinates
  static void write2(uint8_t* data,
                     uint8_t y0,
                     uint8_t y1,
                     uint8_t u0_1,
                     uint8_t v0_1,
                     size_t x,
                     size_t y,
                     size_t w,
                     const std::array<size_t,3>& planes_off);
};
