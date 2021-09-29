#include "image_utils.h"

// ===========================================
// see https://wiki.multimedia.cx/index.php/V210
// see https://wiki.multimedia.cx/index.php/YCbCr_4:2:2
// see https://samples.mplayerhq.hu/V-codecs/R210/bmd_pixel_formats.pdf
// ===========================================

// ===========================================
// V210
// ===========================================
std::array<uint16_t, 3> v210::read_rb(const uint8_t* const data,
                                      size_t x,
                                      size_t y,
                                      size_t rowsize)
{
  std::array<uint16_t, 3> r;

  // V210 is a repeating pattern of 128-bits (16 bytes)
  // Each of this pattern is composed of 4 32-bits blocks and represent a total of 6 pixels
  const uint32_t* v210_ptr = reinterpret_cast<const uint32_t*>(data + (y*rowsize) + ((x/6)*16));

  // // https://developer.apple.com/library/archive/technotes/tn2162/_index.html#//apple_ref/doc/uid/DTS40013070-CH1-TNTAG8-V210__4_2_2_COMPRESSION_TYPE
  // resulting pattern is built as follows:
  // bits    | component
  // --------------------
  // 0-9     | U0+1         >> 0
  // 10-19   | Y0      wd0  >> 10
  // 20-29   | V0+1         >> 20
  //         |
  // 32-41   | Y1           >> 0
  // 42-51   | U2+3    wd1  >> 10
  // 52-61   | Y2           >> 20
  //         |
  // 64-73   | V2+3         >> 0
  // 74-83   | Y3      wd2  >> 10
  // 84-93   | U4+5         >> 20
  //         |
  // 96-105  | Y4           >> 0
  // 106-115 | V4+5    wd3  >> 10
  // 116-125 | Y5           >> 20

  static auto w10 = [](uint32_t w, size_t s) -> uint16_t
  {
    return static_cast<uint16_t>((w >> s) & 0b1111111111);
  };

  const size_t mod=x%6;
  switch(mod)
  {
  case 0: // Y0 U0+1 V0+1
    r[0] = w10(v210_ptr[0],10);
    r[1] = w10(v210_ptr[0],0);
    r[2] = w10(v210_ptr[0],20);
    break;
  case 1: // Y1 U0+1 V0+1
    r[0] = w10(v210_ptr[1],0);
    r[1] = w10(v210_ptr[0],0);
    r[2] = w10(v210_ptr[0],20);
    break;
  case 2: // Y2 U2+3 V2+3
    r[0] = w10(v210_ptr[1],20);
    r[1] = w10(v210_ptr[1],10);
    r[2] = w10(v210_ptr[2],0);
    break;
  case 3: // Y3 U2+3 V2+3
    r[0] = w10(v210_ptr[2],10);
    r[1] = w10(v210_ptr[1],10);
    r[2] = w10(v210_ptr[2],0);
    break;
  case 4: // Y4 U4+5 V4+5
    r[0] = w10(v210_ptr[3],0);
    r[1] = w10(v210_ptr[2],20);
    r[2] = w10(v210_ptr[3],10);
    break;
  case 5: // Y5 U4+5 V4+5
    r[0] = w10(v210_ptr[3],20);
    r[1] = w10(v210_ptr[2],20);
    r[2] = w10(v210_ptr[3],10);
    break;
  }

  return r;
}

v210::group_t v210::read_group(const uint8_t* const data, size_t x, size_t y, size_t rowsize)
{
  // see read()
  const uint32_t* v210_ptr = reinterpret_cast<const uint32_t*>(data + (y*rowsize) + ((x/6)*16));
  static auto w10 = [](uint32_t w, size_t s) -> uint16_t
  {
    return static_cast<uint16_t>((w >> s) & 0b1111111111);
  };
  return v210::group_t{
    w10(v210_ptr[0],10), w10(v210_ptr[1],0), w10(v210_ptr[1],20), w10(v210_ptr[2],10), w10(v210_ptr[3],0), w10(v210_ptr[3],20),
    w10(v210_ptr[0],0), w10(v210_ptr[1],10), w10(v210_ptr[2],20),
    w10(v210_ptr[0],20), w10(v210_ptr[2],0), w10(v210_ptr[3],10)
  };
}

size_t v210::row_size(size_t w)
{
  return ((w+47)/48)*128;
}

size_t v210::frame_size(size_t w,
                        size_t h)
{
  return row_size(w)*h;
}

// ===========================================
// yuv422p10le
// planar YUV 4:2:2 with 16-bits per component (only 10 LSB are significant)
// ===========================================
size_t yuv422p10le::frame_size(size_t w,
                               size_t h)
{
  return w*4*h;
}

std::array<size_t,3> yuv422p10le::plane_offsets(size_t w,
                                                size_t h)
{
  return {0,static_cast<size_t>(w*h*2), static_cast<size_t>(w*h*3)};
}

std::array<uint16_t,3> yuv422p10le::read(const uint8_t* const data,
                                         size_t x,
                                         size_t y,
                                         size_t w,
                                         const std::array<size_t,3>& planes_off)
{
  const uint16_t* y_plane=reinterpret_cast<const uint16_t*>(data+planes_off[0]);
  const uint16_t* u_plane=reinterpret_cast<const uint16_t*>(data+planes_off[1]);
  const uint16_t* v_plane=reinterpret_cast<const uint16_t*>(data+planes_off[2]);

  const size_t xy=y*w+x;

  return {y_plane[xy],u_plane[(xy/2)],v_plane[(xy/2)]};
}

void yuv422p10le::write(uint8_t* data,
                        const std::array<uint16_t,3>& yuv,
                        size_t x,
                        size_t y,
                        size_t w,
                        const std::array<size_t,3>& planes_off)
{
  uint16_t* y_plane=reinterpret_cast<uint16_t*>(data+planes_off[0]);
  uint16_t* u_plane=reinterpret_cast<uint16_t*>(data+planes_off[1]);
  uint16_t* v_plane=reinterpret_cast<uint16_t*>(data+planes_off[2]);

  const size_t xy=y*w+x;

  y_plane[xy]     = yuv[0];
  u_plane[(xy/2)] = yuv[1];
  v_plane[(xy/2)] = yuv[2];
}

void yuv422p10le::write2(uint8_t* data,
                         uint16_t y0,
                         uint16_t y1,
                         uint16_t u0_1,
                         uint16_t v0_1,
                         size_t x,
                         size_t y,
                         size_t w,
                         const std::array<size_t,3>& planes_off)
{
  uint16_t* y_plane=reinterpret_cast<uint16_t*>(data+planes_off[0]);
  uint16_t* u_plane=reinterpret_cast<uint16_t*>(data+planes_off[1]);
  uint16_t* v_plane=reinterpret_cast<uint16_t*>(data+planes_off[2]);

  const size_t xy=y*w+x;

  y_plane[xy]     = y0;
  y_plane[xy+1]   = y1;
  u_plane[(xy/2)] = u0_1;
  v_plane[(xy/2)] = v0_1;
}

// ===========================================
// yuv444p
// ===========================================
size_t yuv444p::frame_size(size_t w,
                           size_t h)
{
  return w*h*3;
}

std::array<size_t, 3> yuv444p::plane_offsets(size_t w,
                                             size_t h)
{
  return {0,w*h,w*h*2};
}

std::array<uint8_t, 3> yuv444p::read(const uint8_t* const data,
                                     size_t x,
                                     size_t y,
                                     size_t w,
                                     const std::array<size_t,3>& planes_off)
{
  size_t xy=y*w+x;
  return {
    data[planes_off[0]+xy],
    data[planes_off[1]+xy],
    data[planes_off[2]+xy]
  };
}

void yuv444p::write(uint8_t* data,
                    const std::array<uint8_t,3>& yuv,
                    size_t x,
                    size_t y,
                    size_t w,
                    const std::array<size_t,3>& planes_off)
{
  size_t xy=y*w+x;
  data[planes_off[0]+xy] = static_cast<uint8_t>(yuv[0]);
  data[planes_off[1]+xy] = static_cast<uint8_t>(yuv[1]);
  data[planes_off[2]+xy] = static_cast<uint8_t>(yuv[2]);
}

void yuv444p::write2(uint8_t* data,
                     uint8_t y0,
                     uint8_t y1,
                     uint8_t u0_1,
                     uint8_t v0_1,
                     size_t x,
                     size_t y,
                     size_t w,
                     const std::array<size_t,3>& planes_off)
{
  size_t xy=y*w+x;
  data[planes_off[0]+xy]   = y0;
  data[planes_off[0]+xy+1] = y1;
  data[planes_off[1]+xy]   = u0_1;
  data[planes_off[2]+xy]   = v0_1;
}
