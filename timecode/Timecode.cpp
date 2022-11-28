#include <timecode.h>

#include <cmath>
#include <stdexcept>
#include <sstream>
#include <regex>

using int128_t = __int128;
using uint128_t = unsigned __int128;

const timecode_t::rate_t timecode_t::RATE_FILM{24,false};
const timecode_t::rate_t timecode_t::RATE_PAL{25,false};
const timecode_t::rate_t timecode_t::RATE_PAL_HS{50,false};
const timecode_t::rate_t timecode_t::RATE_NTSC{30,true};
const timecode_t::rate_t timecode_t::RATE_NTSC_HS{60,true};
const timecode_t::rate_t timecode_t::RATE_WEB{30,false};
const timecode_t::rate_t timecode_t::RATE_WEB_HS{60,false};

// cf https://ffmpeg.org/doxygen/trunk/timecode_8c_source.html
bool timecode_t::rate_t::operator<(const timecode_t::rate_t& o) const
{
  return fps < o.fps || drop < o.drop;
}

bool timecode_t::rate_t::operator==(const timecode_t::rate_t& o) const
{
  return fps == o.fps && drop == o.drop;
}

std::ostream& operator<<(std::ostream& os, const timecode_t::rate_t& dt)
{
  os << dt.fps << (dt.drop ? "DF" : "NDF");
  return os;
}

timecode_t::Timecode(const timecode_t::rate_t& framerate, uint64_t frames)
{
  set_framerate(framerate);
  set_framecount(frames);
}

Timecode timecode_t::from_string(const std::string& s)
{
  Timecode tc;
  tc.set_str(s);
  return tc;
}

timecode_t::rate_t timecode_t::recommended_framerate(const std::pair<uint16_t, uint16_t>& editrate)
{
  if (editrate.first == 24000 && editrate.second == 1001) return RATE_FILM;
  if (editrate.first == 25 && editrate.second == 1)       return RATE_PAL;
  if (editrate.first == 50 && editrate.second == 1)       return RATE_PAL_HS;
  if (editrate.first == 30000 && editrate.second == 1001) return RATE_NTSC;
  if (editrate.first == 60000 && editrate.second == 1001) return RATE_NTSC_HS;
  if (editrate.first == 30 && editrate.second == 1)       return RATE_WEB;
  if (editrate.first == 60 && editrate.second == 1)       return RATE_WEB_HS;

  return rate_t{std::numeric_limits<uint16_t>::max(),false};
}

void timecode_t::verify() const
{
  if (!is_valid())
  {
    throw std::runtime_error{error()};
  }
}

bool timecode_t::is_valid() const
{
  return error().empty();
}

std::string timecode_t::error() const
{
  std::stringstream ss;
  if (_d.ff >= _framerate.fps)
  {
    ss << "Invalid frame number: " << _d.ff;
  }
  if (_d.ss >= 60)
  {
    ss << "Invalid second number: " << _d.ss;
  }
  if (_d.mm >= 60)
  {
    ss << "Invalid minute number: " << _d.mm;
  }
  if (_d.hh >= 24)
  {
    ss << "Invalid hour number: " << _d.hh;
  }
  return ss.str();
}

void timecode_t::set_framerate(const rate_t& v)
{
  _framerate = v;
  verify();
  update_frames_per();
}

timecode_t::rate_t timecode_t::framerate() const
{
  return _framerate;
}

Timecode& timecode_t::set_st12(uint32_t v)
{
  static auto bcd2uint = [](uint8_t bcd) -> unsigned
  {
    unsigned low  = bcd & 0xf;
    unsigned high = bcd >> 4;
    if (low > 9 || high > 9) return 0;
    return low + 10*high;
  };

  auto ff = bcd2uint(v>>24 & 0x3f);  // 6-bit frames
  if (_framerate.fps > 30)
  {
      ff <<= 1;
      if (_framerate.fps == 50) ff += !!(v & 1 << 7);
      else ff += !!(v & 1 << 23);
  }

  auto ss = bcd2uint(v>>16 & 0x7f); // 7-bit seconds
  auto mm = bcd2uint(v>>8 & 0x7f); // 7-bit minutes
  auto hh = bcd2uint(v & 0x3f); // 6-bit hours

  _d.dd = 0;
  _d.hh = hh;
  _d.mm = mm;
  _d.ss = ss;
  _d.ff = ff;

  return *this;
}

uint32_t timecode_t::st12() const
{
  uint32_t r=0;
  auto ff = _d.ff;
  // For SMPTE 12-M timecodes, frame count is a special case if > 30 FPS.
  // See SMPTE ST 12-1:2014 Sec 12.1 for more info.
  if (_framerate.fps > 30)
  {
    if (_d.ff%2 == 1)
    {
      if (_framerate.fps == 50) r |= (1 << 7);
      else r |= (1 << 23);
    }
    ff /= 2;
  }

  r |= _framerate.drop << 30;
  r |= (ff / 10) << 28;
  r |= (ff % 10) << 24;
  r |= (_d.ss / 10) << 20;
  r |= (_d.ss % 10) << 16;
  r |= (_d.mm / 10) << 12;
  r |= (_d.mm % 10) << 8;
  r |= (_d.hh / 10) << 4;
  r |= (_d.hh % 10);

  return r;
}

timecode_t& timecode_t::set_framecount(uint64_t v)
{
  uint64_t frames = v;

  if (_framerate.drop)
  {
    // Count the number of 10 minutes time-spans comprised in this timecode.
    const uint64_t ten_minutes_group_cnt = static_cast<uint64_t>(static_cast<long double>(frames) / static_cast<long double>(_frames_per.ten_minute));
    // Count remaining frames.
    const uint64_t remaining_frames = frames % static_cast<int64_t>(_frames_per.ten_minute);
    // Count remaining minutes.
    // Note: We remove _droppedFramesPerMinute from remainingFrames.
    //       This is due to the fact that every 10 minute, one minute is longer than the others by _droppedFramesPerMinute frames.
    const int64_t remaining_minutes = static_cast<int64_t>(static_cast<long double>(remaining_frames - _frames_per.minute_dropped) / static_cast<long double>(_frames_per.minute_real));
    // Restore dropped frames.
    const uint128_t dropped_frames = static_cast<uint128_t>(_frames_per.minute_dropped) * static_cast<uint128_t>(9 * ten_minutes_group_cnt + remaining_minutes);
    frames += dropped_frames;
  }

  // Update number of days.
  _d.dd = static_cast<decltype(_d.dd)>(static_cast<long double>(frames) / static_cast<long double>(_frames_per.day));
  frames -= _d.dd * _frames_per.day;

  // Update number of hours.
  _d.hh = static_cast<decltype(_d.hh)>(static_cast<long double>(frames) / static_cast<long double>(_frames_per.hour));
  frames -= _d.hh * _frames_per.hour;

  // Update number of minutes.
  _d.mm = static_cast<decltype(_d.mm)>(static_cast<long double>(frames) / _frames_per.minute);
  frames -= _d.mm * _frames_per.minute;

  // Update number of seconds.
  _d.ss = static_cast<decltype(_d.ss)>(static_cast<long double>(frames) / _frames_per.second);
  frames -= _d.ss * _frames_per.second;

  // Update number of frames.
  _d.ff = static_cast<decltype(_d.ff)>(frames);

  return *this;
}

uint64_t timecode_t::framecount() const
{
  uint64_t result = 0;

  // May be updated depending on the drop-frame flag.
  auto frame = _d.ff;

  if (_framerate.drop)
  {
    if (_d.ss == 0 &&
        frame < _frames_per.minute_dropped &&
        (_d.mm % 10) != 0)
    {
      // Except every ten minutes:
      //  . 30DF: Frames 00, 01 and 02 are mapped to the same actual video frame.
      //  . 60DF: Frames 00, 01, 02, 03 and 04 are mapped to the same actual video frame.
      frame = _frames_per.minute_dropped;
    }

    // Compute the number of minutes in this timecode.
    auto minutes_count = (_d.dd * 1440) + (_d.hh * 60) + _d.mm;
    if (minutes_count > 0)
    {
      // Compensate for each dropped frames per minute (except for the ones kept every ten minute).
      result -= _frames_per.minute_dropped * (minutes_count - static_cast<decltype(result)>(static_cast<double>(minutes_count) / 10.));
    }
  }

  // Add timecode withtout taking account of the drop value (which was compensated just before).
  // Factorization from:
  // result += frame + (sec * rate) + (min * 60 * rate) + (hour * 60 * 60 * rate) + (day * 60 * 60 * 24 * rate)
  // Note: The current result is a frame index (0-based) and not a frame count (1-based) => Append 1 to the result;
  result += (frame + _framerate.fps * (_d.ss + 60 * (_d.mm + 60 * (_d.hh + 24 * _d.dd)))) + 1;

  return result-1;
}

timecode_t timecode_t::operator+(int64_t fc) const
{
  return Timecode{_framerate,framecount()+fc};
}

timecode_t& timecode_t::operator++()
{
  set_framecount(framecount()+1);
  return *this;
}

timecode_t timecode_t::operator++(int) const
{
  return Timecode(_framerate,framecount()+1);
}

timecode_t& timecode_t::operator+=(int64_t fc)
{
  set_framecount(framecount() + fc);
  return *this;
}

timecode_t timecode_t::operator-(int64_t fc) const
{
  return Timecode{_framerate,framecount()-fc};
}

timecode_t& timecode_t::operator--()
{
  set_framecount(framecount()-1);
  return *this;
}

timecode_t timecode_t::operator--(int) const
{
  return Timecode(_framerate,framecount()-1);
}

timecode_t& timecode_t::operator-=(int64_t v)
{
  set_framecount(framecount()-v);
  return *this;
}

timecode_t timecode_t::operator-(const Timecode& o) const
{
  return Timecode{_framerate, framecount()-o.framecount()};
}

std::string timecode_t::str() const
{
  // FIXME: use stl
  auto pad = [](uint64_t n, size_t w, const std::string& c) -> std::string
  {
    auto result = std::to_string(n);
    while (result.size() < w) result = c + result;
    return result;
  };

  return pad(_d.hh  ,2,"0") + ':' +
         pad(_d.mm,2,"0") + ':' +
         pad(_d.ss,2,"0") + (_framerate.drop ? ';' : ':') +
      pad(_d.ff ,2,"0");
}

timecode_t& timecode_t::set_str(const std::string& s)
{
  std::regex rx{"^([0-9]{2}):([0-9]{2}):([0-9]{2})(?::|;)([0-9]{2})"};
  std::smatch m;
  if (std::regex_search(s, m, rx))
  {
    _d.dd = 0;
    _d.hh = std::stoi(m[1]);
    _d.mm = std::stoi(m[2]);
    _d.ss = std::stoi(m[3]);
    _d.ff = std::stoi(m[4]);
    verify();
  }
  else
  {
    throw std::runtime_error{"invalid format"};
  }

  return *this;
}

void timecode_t::update_frames_per()
{
  _frames_per.second = _framerate.fps;
  _frames_per.minute = _framerate.fps*60;
  _frames_per.hour = _framerate.fps*3600; //60*60
  _frames_per.day = _framerate.fps*86400; //60*60*24

  if (_framerate.drop)
  {
    // e.g: In drop-frame mode, there is 30,000*1001 frames per minute instead of 30.
    _frames_per.minute_real = static_cast<decltype(_frames_per.minute_real)>(
          (60.*static_cast<double>(_framerate.fps)*1000.) /
          1001.
        );
    // Every ten minutes, the count comes round => Compute the number of lost frames.
    _frames_per.minute_dropped = 60.0 * static_cast<double>(_framerate.fps) - static_cast<double>(_frames_per.minute_real);
    _frames_per.ten_minute = 10.0 * static_cast<double>(_frames_per.minute_real) + _frames_per.minute_dropped;
  }
  else
  {
    // There is no dropped frames for a 10 minutes span in this case.
    _frames_per.minute_dropped = 0;
    _frames_per.ten_minute = 10.0 * static_cast<double>(_frames_per.minute);
  }
}
