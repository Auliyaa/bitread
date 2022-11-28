#include <gtest/gtest.h>

#include <timecode.h>
#include <map>
#include <vector>

#include "test_Timecode_data.cxx"

class TimecodeTest : public ::testing::TestWithParam<timecode_t::rate_t>
{
};

static const std::vector<timecode_t::rate_t> rates = {
  timecode_t::RATE_PAL,
  timecode_t::RATE_PAL_HS,
  timecode_t::RATE_WEB,
  timecode_t::RATE_WEB_HS,
  timecode_t::RATE_FILM,
  timecode_t::RATE_NTSC,
  timecode_t::RATE_NTSC_HS
};

const auto print_test_name = [](const testing::TestParamInfo<TimecodeTest::ParamType>& rate)
{
  return std::to_string(rate.param.fps) + "_" + std::string{rate.param.drop ? "drop" : "nodrop"};
};

TEST_P(TimecodeTest, loop)
{
  auto& ref = TC_DATA[GetParam()];
  uint64_t fn=1700;
  for (const auto& s : ref)
  {
    Timecode tc{GetParam(),fn};
    EXPECT_STREQ(tc.str().c_str(),s.c_str());
    EXPECT_EQ(tc.framecount(),fn);
    ++fn;
  }
}

const uint64_t TEST_COUNT=100000;

TEST_P(TimecodeTest, increment_decrement)
{
  for (uint64_t fn=0; fn < TEST_COUNT; ++fn)
  {
    Timecode tc{GetParam(), fn};
    ++tc;
    EXPECT_EQ(tc.framecount(),fn+1);
    --tc;
    EXPECT_EQ(tc.framecount(),fn);
    tc+=10;
    EXPECT_EQ(tc.framecount(),fn+10);
    tc-=10;
    EXPECT_EQ(tc.framecount(),fn);
    auto tc2 = tc+10;
    EXPECT_EQ((tc2-tc).framecount(), 10);
  }
}

TEST_P(TimecodeTest, st12)
{
  for (uint64_t fn=0; fn < TEST_COUNT; ++fn)
  {
    Timecode tc{GetParam(), fn};
    uint32_t st12 = tc.st12();
    Timecode tc2{GetParam(), 0};
    tc2.set_st12(st12);
    EXPECT_EQ(tc.str(),tc2.str());
  }
}

TEST_P(TimecodeTest, framecount)
{
  Timecode tc{GetParam()};
  tc.set_framecount(0);
  EXPECT_EQ(tc.framecount(),0);
  EXPECT_STREQ(tc.str().c_str(),GetParam().drop ? "00:00:00;00" : "00:00:00:00");
  tc+=1;
  EXPECT_EQ(tc.framecount(),1);
  EXPECT_STREQ(tc.str().c_str(),GetParam().drop ? "00:00:00;01" : "00:00:00:01");
  tc-=1;
  EXPECT_EQ(tc.framecount(),0);
  EXPECT_STREQ(tc.str().c_str(),GetParam().drop ? "00:00:00;00" : "00:00:00:00");

  for (uint64_t fc=0; fc < TEST_COUNT; ++fc)
  {
    EXPECT_EQ(tc.framecount(),fc);
    tc+=1;
  }
}

TEST_P(TimecodeTest, overflow)
{
  /// when the frame count exceeds a given value: the timecode  was overflowing. Leading to invalid values being stored.
  Timecode tc(GetParam(),18446744071797000860ULL);
  EXPECT_LT(tc.ff(), 25);
  EXPECT_NE(tc.dd(), 0);
  std::cout << tc.dd() << "d " << tc.str() << std::endl;
}

TEST_P(TimecodeTest, drop_10m)
{
  if (DROP_10M_DATA.count(GetParam())==0) return;

  for(size_t ii=0; ii<4; ++ii)
  {
    drop_10m_test_data_t data = DROP_10M_DATA[GetParam()][ii];
    Timecode tc(GetParam());
    tc.set_framecount(data.frame_count);
    EXPECT_EQ(tc.framecount(), data.frame_count);
    EXPECT_EQ(tc.str(), data.str);
  }
}

INSTANTIATE_TEST_CASE_P(Timecode, TimecodeTest, ::testing::ValuesIn(rates), print_test_name);
