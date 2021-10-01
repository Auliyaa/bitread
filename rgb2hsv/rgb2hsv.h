#pragma once

#include <cmath>

struct rgb_t
{
  rgb_t() {}
  rgb_t(unsigned char r, unsigned char g, unsigned char b)
    : r{static_cast<double>(r)/255.},
      g{static_cast<double>(g)/255.},
      b{static_cast<double>(b)/255.}
  {}

  double r = 0;       // a fraction between 0 and 1
  double g = 0;       // a fraction between 0 and 1
  double b = 0;       // a fraction between 0 and 1
};

struct hsv_t {
  double h = 0;       // angle in degrees
  double s = 0;       // a fraction between 0 and 1
  double v = 0;       // a fraction between 0 and 1
};

hsv_t rgb2hsv(const rgb_t& in)
{
  hsv_t out;
  double min, max, delta;

  min = in.r < in.g ? in.r : in.g;
  min = min  < in.b ? min  : in.b;

  max = in.r > in.g ? in.r : in.g;
  max = max  > in.b ? max  : in.b;

  out.v = max;
  delta = max - min;
  if (delta < 0.00001)
  {
    out.s = 0;
    out.h = 0;
    return out;
  }
  if(max > 0.0)
  {
    out.s = (delta / max);
  }
  else
  {
    out.s = 0.0;
    out.h = NAN;
    return out;
  }
  if(in.r >= max)
  {
    out.h = (in.g - in.b) / delta;
  }
  else if (in.g >= max)
  {
    out.h = 2.0 + (in.b - in.r) / delta;
  }
  else
  {
    out.h = 4.0 + (in.r - in.g) / delta;
  }

  out.h *= 60.0;

  if (out.h < 0.0)
  {
    out.h += 360.0;
  }

  return out;
}

rgb_t hsv2rgb(const hsv_t& in)
{
  double hh, p, q, t, ff;
  long i;
  rgb_t out;

  if(in.s <= 0.0)
  {
    out.r = in.v;
    out.g = in.v;
    out.b = in.v;
    return out;
  }
  hh = in.h;
  if (hh >= 360.0)
  {
    hh = 0.0;
  }
  hh /= 60.0;
  i = static_cast<long>(hh);
  ff = hh - i;
  p = in.v * (1.0 - in.s);
  q = in.v * (1.0 - (in.s * ff));
  t = in.v * (1.0 - (in.s * (1.0 - ff)));

  switch(i)
  {
  case 0:
    out.r = in.v;
    out.g = t;
    out.b = p;
    break;
  case 1:
    out.r = q;
    out.g = in.v;
    out.b = p;
    break;
  case 2:
    out.r = p;
    out.g = in.v;
    out.b = t;
    break;
  case 3:
    out.r = p;
    out.g = q;
    out.b = in.v;
    break;
  case 4:
    out.r = t;
    out.g = p;
    out.b = in.v;
    break;
  case 5:
  default:
    out.r = in.v;
    out.g = p;
    out.b = q;
    break;
  }
  return out;
}

hsv_t rgb2hsv(unsigned char r, unsigned char g, unsigned char b)
{
  return rgb2hsv(rgb_t{r,g,b});
}
