#pragma once

#include <cstdint>
#include <string>
#include <utility>

/**
 * @brief This class represents a media timecode (e.g: 00:00:00:00) with a defined frame rate and a drop frame flag.
 * Based on the work at https://github.com/X3TechnologyGroup/VideoFrame/blob/master/VideoFrame.js#L41
 */
class timecode_t
{
public:
  /// A timecode rate embbeds the FPS and drop flag values
  struct rate_t
  {
    inline rate_t(uint16_t f, bool d)
      : fps{f}, drop{d} {}
    rate_t(const rate_t&)=default;
    rate_t(rate_t&&)=default;
    virtual ~rate_t()=default;
    rate_t& operator=(const rate_t&)=default;
    rate_t& operator=(rate_t&&)=default;
    bool operator<(const rate_t&) const;
    bool operator==(const rate_t&) const;

    ::uint16_t fps;
    bool drop;
  };

  /// 24 FPS NDF
  static const rate_t RATE_FILM;
  /// 25 FPS NDF
  static const rate_t RATE_PAL;
  /// 50 FPS NDF
  static const rate_t RATE_PAL_HS;
  /// 30 FPS DF
  static const rate_t RATE_NTSC;
  /// 60 FPS DF
  static const rate_t RATE_NTSC_HS;
  /// 30 FPS NDF
  static const rate_t RATE_WEB;
  /// 60 FPS NDF
  static const rate_t RATE_WEB_HS;

  timecode_t(const rate_t& framerate=RATE_PAL, uint64_t frames=0);
  timecode_t(const timecode_t&)=default;
  timecode_t(timecode_t&&)=default;
  virtual ~timecode_t()=default;
  timecode_t& operator=(const timecode_t&)=default;
  timecode_t& operator=(timecode_t&&)=default;

  /// Instantiates a timecode from a string.
  static timecode_t from_string(const std::string&);

  /// @returns a recommended frame-rate based on a video edit-rate.
  /// If the specified edit-rate is not recognized, this function returns undefined.
  static rate_t recommended_framerate(const std::pair<uint16_t,uint16_t>& editrate);

  /// Throws an error if this object is invalid.
  void verify() const;
  /// A valid timecode has no error set inside the error member
  bool is_valid() const;
  /// Contains an error message if some data in this Timecode does not match its settings.
  std::string error() const;

  /// Sets the frame count
  inline timecode_t& set_ff(uint16_t v) { _d.ff = v; return *this; }
  /// Gets the frame count
  inline uint16_t ff() const { return _d.ff; }

  /// Sets the second count
  inline timecode_t& set_ss(uint16_t v) { _d.ss = v; return *this; }
  /// Gets the second count
  inline uint16_t second() const { return _d.ss; }

  /// Sets the minute count
  inline timecode_t& set_mm(uint16_t v) { _d.mm = v; return *this; }
  /// Gets the minute count
  inline uint16_t mm() const { return _d.mm; }

  /// Sets the hour count
  inline timecode_t& set_hh(uint16_t v) { _d.hh = v; return *this; }
  /// Gets the hour count
  inline uint16_t hh() const { return _d.hh; }

  /// Sets the day count
  inline timecode_t& set_dd(uint16_t v) { _d.dd = v; return *this; }
  /// Gets the day count
  inline uint64_t dd() const { return _d.dd; }

  /// Sets the timecode rate (fps & drop flag)
  void set_framerate(const rate_t& v);
  /// Gets the timecode rate (fps & drop flag)
  rate_t framerate() const;

  /// sets tc components from a SMPTE ST-12 32-bits word
  timecode_t& set_st12(uint32_t);

  /// @returns the SMPTE ST-12 32-bits word representing the current timecode
  uint32_t st12() const;

  /**
   * @brief Sets the index of the last frame stored inside this Timecode.
   * Setting this member will update day, hours, minutes, seconds and frame members.
   * <b>Note:</b> A timecode of 00:00:00:00 has a frame number of 0.
   */
  timecode_t& set_framecount(uint64_t v);
  /**
   * @brief Returns the index of the last frame stored inside this Timecode.
   * <b>Note:</b> A timecode of 00:00:00:00 has a frame number of 0.
   */
  uint64_t framecount() const;

  /// Comparison operator
  inline bool operator<(const timecode_t& o) const { return framecount() < o.framecount(); }
  /// Comparison operator
  inline bool operator>(const timecode_t& o) const { return framecount() > o.framecount(); }
  /// Comparison operator
  inline bool operator<=(const timecode_t& o) const { return framecount() <= o.framecount(); }
  /// Comparison operator
  inline bool operator>=(const timecode_t& o) const { return framecount() >= o.framecount(); }
  /// Comparison operator
  inline bool operator==(const timecode_t& o) const { return framecount() == o.framecount(); }

  /// Increment operator
  timecode_t operator+(int64_t fc) const;
  timecode_t& operator++(); // prefix
  timecode_t operator++(int) const; // postfix
  timecode_t& operator+=(int64_t);
  /// Decrement operator
  timecode_t operator-(int64_t fc) const;
  timecode_t& operator--(); // prefix
  timecode_t operator--(int) const; // postfix
  timecode_t& operator-=(int64_t);
  /// Returns the duration between 2 timecodes
  timecode_t operator-(const Timecode&) const;

  /// @brief Gets this timecode as a string (e.g: 00:00:00:00 for NDF, 00:00:00;00 for DF).
  std::string str() const;
  /// @brief Sets this timecode as a string (e.g: 00:00:00:00 for NDF, 00:00:00;00 for DF).
  timecode_t& set_str(const std::string&);

  /// Conversion operator
  inline operator std::string() const { return str(); }

private:
  // timecode data (day hour minutes seconds frames)
  struct
  {
    uint16_t ff;
    uint16_t ss;
    uint16_t mm;
    uint16_t hh;
    uint64_t dd;
  } _d = {0,0,0,0,0};

  // tc-rate
  rate_t _framerate = {0,false};

  // The goal of those members is to avoid un-necessary computations when asking for the frame count.
  struct
  {
     uint64_t second;
     uint64_t minute;
     uint64_t hour;
     uint64_t day;
     double minute_dropped;
     double ten_minute;
     uint64_t minute_real;
  } _frames_per;

  /// Updates values in the _frame_per struct
  void update_frames_per();
};

std::ostream& operator<<(std::ostream& out, const timecode_t::rate_t& v);
