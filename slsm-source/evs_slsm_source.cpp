#include <evs-sxe-ingest-engine/source/evs_slsm_source.h>

#include <ServerX/Time/TimePoint.h>

#include <sys/syscall.h>
#include <sys/capability.h>
#include <sys/resource.h>

using namespace SXE_NAMESPACE::ingest_engine;

struct slsm_cached_sample_t
{
  eef::SampleContainerConstPtr sample;
  size_t phase_idx;

  slsm_cached_sample_t() = default;
  slsm_cached_sample_t(eef::SampleContainerConstPtr sample, size_t phase_idx)
    : sample{sample}, phase_idx{phase_idx}
  {}

  virtual ~slsm_cached_sample_t() = default;
};

evs_slsm_source::evs_slsm_source(const std::string& id)
  : media_source{id},
    _running{false}
{
}

void evs_slsm_source::add_phase(std::shared_ptr<eef_media_source> src)
{
  _phase_srcs.emplace_back(src);
}

std::shared_ptr<eef_media_source> evs_slsm_source::master_source() const
{
  return _phase_srcs[0];
}

void evs_slsm_source::init(const media_info& defined_input)
{
  // check parameters
  if (_phase_srcs.size() <= 1)
  {
    throw std::runtime_error{"Number of phases must be greater than 1"};
  }

  // init sub-sources
  for (auto src : _phase_srcs)
  {
    src->init(defined_input);
  }

  // fetch master info & check its validity
  auto master_defined_output = master_source()->defined_eef_output();
  if (master_defined_output->GetValue("")->GetVideoInfoList().empty())
  {
    throw std::runtime_error{"No video track available from the master phase."};
  }
  if (master_defined_output->GetValue("")->GetVideoInfoList().size() > 1)
  {
    throw std::runtime_error{"Only 1 video track is supported."};
  }

  // keep additional info in cache
  auto master_video_info = master_defined_output->GetValue("")->GetVideoInfoList()[0];

  _has_audio = !(master_defined_output->GetValue("")->GetAudioInfoList().empty());

  _is_interlaced =
      master_video_info->GetParameters()->GetScanMode() == eef::ScanMode_interlace &&
      master_video_info->GetParameters()->GetFrameLayout() == eef::FrameLayout_SeparatedFields;

  _phase_sample_rate = eef::FindClockInfoWithclockId(master_video_info->GetClockInfos(), "")->GetClockRate();
  _slsm_sample_rate = _phase_sample_rate;
  _slsm_sample_rate.SetFactorNumerator(_slsm_sample_rate.GetFactorNumerator() * phase_count());

  // manually duplicate & split audio & video info in order to duplicate clock infos
  // (this is required so that output clock info pointer address differ from the master source pointers)

  // video
  {
    // patch the default clock to use the SLSM rate (phase_count() * master rate)
    std::vector<eef::ClockInfoPtr> output_clock_infos;
    for (auto master_clock_info : master_defined_output->GetValue("")->GetVideoInfoList()[0]->GetClockInfos())
    {
      // duplicate all clocks to get stable pointers across all phases
      eef::ClockInfoPtr output_clock_info = nullptr;
      switch(master_clock_info->GetClockType())
      {
      case eef::ClockType_Relative: output_clock_info = eef::CreateClockInfoRelative(master_clock_info->GetClockId(), master_clock_info->GetClockRate()); break;
      case eef::ClockType_TimeCode: output_clock_info = eef::CreateClockInfoTimecode(master_clock_info->GetClockId(), master_clock_info->GetClockRate(), master_clock_info->GetTimeCodeInfo()); break;
      case eef::ClockType_UnixEpoch: output_clock_info = eef::CreateClockInfoUnixEpoch(master_clock_info->GetClockId()); break;
      default: throw std::runtime_error{"Encountered unknown clock type in input: " + std::to_string(static_cast<int>(master_clock_info->GetClockType()))};
      }

      // patch default clock rate
      if (output_clock_info->GetClockId().compare("") == 0)
      {
        output_clock_info = eef::CreateClockInfoRelative("", _slsm_sample_rate);
      }

      // store the duplicate
      output_clock_infos.emplace_back(output_clock_info);
      _output_video_clock_info[output_clock_info->GetClockId()] = output_clock_info;
    }

    auto out_video_info = eef::CreateVideoInfo(eef::CreateVideoParameters(master_video_info->GetParameters()),
                                               eef::CreateChannelInfo(master_video_info->GetChannelInfo()),
                                               output_clock_infos,
                                               master_video_info->GetSampleFlags());

    // patch output video info
    auto slsm_frame_rate = _slsm_sample_rate;
    if (_is_interlaced)
    {
      slsm_frame_rate.SetFactorNumerator(slsm_frame_rate.GetFactorNumerator() / 2);
    }
    std::const_pointer_cast<eef::IVideoParameters>(out_video_info->GetParameters())->SetVideoFrameRate(slsm_frame_rate);

    _defined_output.video.infos = eef::CreateKeyToInfoContainerMap(eef::CreateInfoContainer(out_video_info));
    _defined_output.video.pin = add_output(VIDEO_PIN_ID);
  }

  // audio
  if (_has_audio)
  {
    // extract the untouched master audio info as sample rate do not change for audio
    _defined_output.audio.infos = eef::CreateKeyToInfoContainerMap(eef::CreateInfoContainer(master_defined_output->GetValue("")->GetAudioInfoList()));
    _defined_output.audio.pin = add_output(AUDIO_PIN_ID);
  }

  // check for high priority mode capability
  if (_high_priority && !has_cap_nice())
  {
    throw std::runtime_error{"the CAP_SYS_NICE capability is not available for the current pid: " + std::to_string(getpid())};
  }
}

media_info evs_slsm_source::defined_input(const std::string&) const
{
  return media_info::null;
}

media_info evs_slsm_source::defined_output(const std::string& pin_id) const
{
  if (pin_id.compare(VIDEO_PIN_ID) == 0)
  {
    return _defined_output.video.infos;
  }
  if (pin_id.compare(AUDIO_PIN_ID) == 0)
  {
    return _defined_output.audio.infos;
  }
  return media_info::null;
}

void evs_slsm_source::start()
{
  bool expected{false};
  if (!_running.compare_exchange_strong(expected, true))
  {
    return;
  }

  // configure feeding
  _feed_buf.reset(phase_count());

  // each feeder is responsible for fetching raw samples directly from the source and push them inside a ring buffer (_feed_buf)
  auto feed_fn = [this](std::shared_ptr<eef_media_source> src, size_t p)
  {
    uint64_t stream_pos{0};

    // handle high-priority mode
    if (_high_priority && has_cap_nice())
    {
      const pid_t tid = ::syscall(SYS_gettid);
      int ret = setpriority(PRIO_PROCESS, tid, -20);
      if (ret != 0)
      {
        std::cerr << "failed to set source thread priority: error #" + std::to_string(ret) << std::endl;
      }
    }

    while(this->_running)
    {
      try
      {
        // may throw: see catch blocks below
        auto s = src->get_eef_sample();
        // the source may randomly send samples with a null timestamp.
        //  in such a case, ignore it to reduce the load since the source will not be able to process it.
        if (get_time_info(s, SYSTEM_TIME_CLOCK).GetTimeNumber() == 0)
        {
          send_event<media_error>(media_error::buffer_error,
                                  source_type(),
                                  "Received a null timestamp from the source");
        }
        else
        {
          this->_feed_buf.push(s,stream_pos % SLSM_RBUF_SIZE, p);
          ++stream_pos;
        }
      }
      __CATCH_ERROR_BLOCK__(media_error::buffer_error, "Failed to fetch sample from the source", {}, false);
    }
  };

  // instantiate feeder threads
  for (size_t phase_idx{0}; phase_idx < _phase_srcs.size(); ++phase_idx)
  {
    _feed_threads.emplace_back(std::make_shared<std::thread>(std::bind(feed_fn, _phase_srcs[phase_idx], phase_idx)));
  }

  // configure consumer
  // the consumer wait timeout is equal to half the duration of a full phase
  _consumer_wait_timeo = std::chrono::milliseconds{static_cast<long>(std::ceil(1000. * (static_cast<double>(_phase_sample_rate.GetFactorDenominator()) / static_cast<double>(_phase_sample_rate.GetFactorNumerator() * 2.))))};

  auto consumer_fn = [this]()
  {
    // handle high-priority mode
    if (_high_priority && has_cap_nice())
    {
      const pid_t tid = ::syscall(SYS_gettid);
      int ret = setpriority(PRIO_PROCESS, tid, -20);
      if (ret != 0)
      {
        std::cerr << "failed to set source thread priority: error #" + std::to_string(ret) << std::endl;
      }
    }

    // interlaced: The output group is twice as large in order to encompass 2 fields.
    // we do not want to forward only 1 field.
    slsm_group_t buf{phase_count() * (_is_interlaced ? 2 : 1)};
    // where samples are put and stored before being put in sequence inside the buffer.
    std::deque<slsm_cached_sample_t> oflow;
    // the overflow tolerance is the time (in ns) beyond which we consider an incomplete slsm sequence can never be completed and needs to be dropped.
    // see below when this variable is used for more details about the logic.
    int64_t oflow_tolerance =
        static_cast<int64_t>(std::ceil(4.*
                               static_cast<double>(1000000000. * _phase_sample_rate.GetFactorDenominator()) /
                               static_cast<double>((_phase_sample_rate.GetFactorNumerator()))));
    // interlaced: cache where bottom fields timepoints are mapped to their top-field timepoint. Used to group both the top and bottom fields inside the same group
    std::map<int64_t, int64_t> timings_cache;
    while(_running)
    {
      // Wait for a data notification indicating that we have at least phase_count() samples available from the feeders
      {
        std::unique_lock<std::mutex> _lk_{_feed_buf.cv_mtx};
        if (_feed_buf.cv.wait_for(_lk_, _consumer_wait_timeo) == std::cv_status::timeout)
        {
          // no data after a predefined timeout => loop & check for the _running flag once more
          continue;
        }
      }

      timings_cache.clear();

      // swap feed buffers and copy the back buffer inside our local bbuf
      auto bbuf = _feed_buf.swap();

      // sort input samples by timestamp
      for (decltype(_feed_buf)::block_t& block : bbuf)
      {
        for (size_t phase_idx=0; phase_idx < phase_count(); ++phase_idx)
        {
          auto sample = block[phase_idx];
          if (sample == nullptr)
          {
            // slot non-filled by our feeders => ignore it
            continue;
          }

          // interlaced: bottom field timepoints are reset to the timepoint of their top field.
          // this process eases-up grouping top & bottom fields together and ensure they are considered as part of the same phase block.
          if (_is_interlaced && sample->GetVideoSampleList()[0]->GetSampleParameters()->GetPictureStructure() == eef::PictureStructure_bottom)
          {
            // do not use sample number here as they may differ from phase to phase
            auto sample_tp = get_time_info(sample, SYSTEM_TIME_CLOCK).GetTimeNumber();
            if (timings_cache.find(sample_tp) == timings_cache.end())
            {
              // cache miss: convert the timepoint in a sample number since epoch, remove 1 (this gives us the top-field sample number)
              // then go back to a timepoint.
              // this method ensures that we stay properly aligned for non-integer sample rates.
              ServerX::Time::TimePoint tp; tp.FromNanoseconds(sample_tp);
              auto top_sn = tp.SampleNumber(ServerX::Time::Ratio{_phase_sample_rate.GetFactorNumerator(), _phase_sample_rate.GetFactorDenominator()}) - 1;
              tp.FromRatio(top_sn, _phase_sample_rate.GetFactorDenominator(), _phase_sample_rate.GetFactorNumerator());
              timings_cache[sample_tp] = tp.ToNanoseconds();
            }
            const_cast<eef::TimeLimits&>(get_time_info(sample, SYSTEM_TIME_CLOCK)).SetTimeNumber(timings_cache[sample_tp]);

            // append the bottom field after all top fields
            oflow.emplace_back(slsm_cached_sample_t{sample, phase_idx+phase_count()});
          }
          else
          {
            // top-field / frame => simply append it for reordering
            oflow.emplace_back(slsm_cached_sample_t{sample, phase_idx});
          }
        }
      }

      // sort overflow by sample timepoint
      std::sort(oflow.begin(), oflow.end(), [](const slsm_cached_sample_t& a, const slsm_cached_sample_t& b)
      {
        int64_t tp_diff = get_time_info(a.sample, SYSTEM_TIME_CLOCK).GetTimeNumber() - get_time_info(b.sample, SYSTEM_TIME_CLOCK).GetTimeNumber();
        if (tp_diff == 0) return a.phase_idx < b.phase_idx;
        return (tp_diff < 0);
      });

      // keep track of the maximum timepoint in the current vector. will be used to check the timeshift between
      //  the last received sample and the current slsm group being built.
      auto oflow_last_tp = get_time_info(oflow[oflow.size()-1].sample, SYSTEM_TIME_CLOCK).GetTimeNumber();

      // empty the sorted overflow into our group and send the group each time it is completed.
      while (!oflow.empty())
      {
        // pop the oldest sample
        auto sample_in = oflow.front();

        // fetch sample's timepoint
        auto tp_in = get_time_info(sample_in.sample, SYSTEM_TIME_CLOCK).GetTimeNumber();

        if (tp_in < buf.tp())
        {
          // drop sample for any future loop
          oflow.pop_front();

          // overlow sample is located before the current group: should not append (critical time skew in the underlying source)
          std::stringstream _err_;
          _err_ << "Sample @" << tp_in << " is located before current buffer @" << buf.tp() << ": time skew in source timings detected";
          send_event<media_error>(media_error::buffer_error, source_type(), _err_.str());
          continue;
        }
        else if (tp_in == buf.tp())
        {
          // sample belongs to the current group: store it
          buf.push(sample_in.sample, sample_in.phase_idx);
          // remove sample from queue
          oflow.pop_front();
        }
        else
        {
          // sample is located after the current group
          // this either means that the current group is empty (not an error) or that a frame is missing from the current sequence.
          if (buf.tp() != 0)
          {
            // in some cases, the next group starts arriving before the current group is properly finished.
            // this should not happen and means that something is basically wrong with the source which starts sending samples before the current
            //  sequence is properly finished.
            // however, in such a case: we check the time difference between the last sample in the overflow and the current sequence.
            // if the difference is roughly greater than the duration of 2 sequence: we can safely consider that the missing frame will never
            //  come in. if the time difference is less than 2 sequences, wait for new data to arrive by breaking the current loop and leaving the
            //  current sequence and overflow buffer as is.
            if (oflow_last_tp - buf.tp() <= oflow_tolerance)
            {
              // less than 2 phases in the oflow buffer, leave the current group alone
              //  for now and wait for the next feed loop to see if it finally comes in.
              break;
            }

            // the current group is not empty: missing sample in the current slsm group error
            send_event<media_error>(media_error::buffer_error, source_type(), "Dropping incomplete sequence @" + std::to_string(buf.tp()));

            // discard the previous group as we do not expect to receive the missing sample since newer ones are already coming in.
            buf.clear();
          }

          // append the sample
          buf.push(sample_in.sample, sample_in.phase_idx);

          // remove sample from queue
          oflow.pop_front();
        }

        if (buf.is_complete())
        {
          // the current group has been completed and is ready to be sent.
          // forward all samples and clear the group before continuing to empty the overflow.
          this->patch_and_forward(buf);

          buf.clear();
        }
      }
    }
  };

  _consumer_thread = std::make_shared<std::thread>(consumer_fn);
}

void evs_slsm_source::stop()
{
  bool expected{true};
  if (!_running.compare_exchange_strong(expected, false))
  {
    return;
  }

  for (auto th : _feed_threads)
  {
    th->join();
  }

  _feed_threads.clear();

  _consumer_thread->join();
}

bool evs_slsm_source::is_running() const
{
  return _running;
}

bool evs_slsm_source::push_next_sample(std::string& err)
{
  throw std::runtime_error{"Operation not supported"};
}

void evs_slsm_source::patch_and_forward(const slsm_group_t& g)
{
  // video
  {
    // duplicate video samples to use patched video info:
    //  - use output infos (re-create the sample)
    //  - patch the default clock (use the slsm rate)
    //  - patch the field indicator (for interlaced streams)
    media_sample_ptr out = std::make_shared<media_sample>();

    // convert the timepoint of the first sample (phase 0) into a sample number (using the slsm rate)
    //  we'll then use this sample number to both patch the default clock values and the timepoint of subsequent samples instead of doing a full conversion each time
    ServerX::Time::TimePoint phase_tp; phase_tp.FromNanoseconds(get_time_info(g[0], SYSTEM_TIME_CLOCK).GetTimeNumber());
    auto phase_sn = phase_tp.SampleNumber(ServerX::Time::Ratio{_slsm_sample_rate.GetFactorNumerator(), _slsm_sample_rate.GetFactorDenominator()});

    for (size_t phase_idx{0}; phase_idx < g.phase_count(); ++ phase_idx)
    {
      auto s_in = g[phase_idx]->GetVideoSampleList()[0];
      std::vector<eef::TimeLimits> tls_out;

      // timelimits clocks have to be in the exact same order as what's defined in the defined_output. hence our sub-optimal way of iteration
      for (auto output_clock_info : _defined_output.video.infos->GetVideoInfoList()[0]->GetClockInfos())
      {
        const auto& tl_in = get_time_info(s_in, output_clock_info->GetClockId());

        // copy the input timelimits using the duplicate output clock infos
        eef::TimeLimits tl_out = eef::TimeLimits{tl_in.GetEditRate(), tl_in.GetTimeNumber(), tl_in.GetDurationNumber(), _output_video_clock_info[tl_in.GetClockInfo()->GetClockId()]};

        if (tl_out.GetClockInfo()->GetClockId().compare("") == 0)
        {
          // default clock: reorder the phase inside the current group (+1 per phase & field)
          tl_out.SetEditRate(_output_video_clock_info[tl_in.GetClockInfo()->GetClockId()]->GetClockRate());
          tl_out.SetTimeNumber(static_cast<int64_t>(phase_sn + phase_idx));
          tl_out.SetDurationNumber(1);
        }
        else if (tl_in.GetClockInfo()->GetClockId().compare(SYSTEM_TIME_CLOCK) == 0)
        {
          // system-time: convert the transformed sample number into ns using the slsm rate
          ServerX::Time::TimePoint tp_in;
          tp_in.FromRatio(phase_sn + phase_idx, _slsm_sample_rate.GetFactorDenominator(), _slsm_sample_rate.GetFactorNumerator());
          ServerX::Time::TimePoint tp_out;
          tp_out.FromRatio(phase_sn + phase_idx + 1, _slsm_sample_rate.GetFactorDenominator(), _slsm_sample_rate.GetFactorNumerator());

          tl_out.SetTimeNumber(tp_in.ToNanoseconds());
          tl_out.SetDurationNumber(tp_out.ToNanoseconds() - tp_in.ToNanoseconds());
        }
        tls_out.emplace_back(tl_out);
      }

      auto video_sample_out = eef::CreateVideoSample(s_in->GetSampleParameters(),
                                                     _defined_output.video.infos->GetVideoInfoList()[0],
                                                     s_in->GetData(),
                                                     tls_out,
                                                     s_in->GetFlag());
      // interlaced streams: patch the top/bottom field flag
      if (_is_interlaced)
      {
        std::const_pointer_cast<eef::IVideoSampleParameters>(video_sample_out->GetSampleParameters())->SetPictureStructure(phase_idx % 2 == 0 ? eef::PictureStructure_top : eef::PictureStructure_bottom);
      }

      out->samples.emplace_back(eef::CreateSampleContainer(eef::VideoSampleList{video_sample_out},_defined_output.video.infos));
    }

    if (!_defined_output.video.pin->push(out))
    {
      send_event<media_error>(media_error::push_failed, source_type(), "Failed to push video sequence @" + std::to_string(g.tp()));
    }
  }

  // audio
  if (_has_audio && !g[0]->GetAudioSampleList().empty())
  {
    // send the audio sample for phase 0, ignore other phases
    media_sample_ptr out = std::make_shared<media_sample>();
    out->samples.emplace_back(eef::CreateSampleContainer(g[0]->GetAudioSampleList(),_defined_output.audio.infos));
    if (!_defined_output.audio.pin->push(out))
    {
      send_event<media_error>(media_error::push_failed, source_type(), "Failed to push audio sequence @" + std::to_string(g.tp()));
    }
}
}

void evs_slsm_source::flush_consumer()
{
}
