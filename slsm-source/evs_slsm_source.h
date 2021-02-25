#pragma once

#include <evs-sxe-ingest-engine/engine.h>

#include <evs-sxe-ingest-engine/source/slsm/slsm_rbuf.h>
#include <evs-sxe-ingest-engine/source/slsm/slsm_group.h>

BEGIN_SXE_MEDIA_ENGINE_MODULE


/**
 * @brief Encapsulates several sources and reorder their output streams in a SLSM sequence.
 *
 * A SLSM sequence is made by taking the output stream of several video streams (phases) and reorder them to form a single output stream.
 * When the input is interlaced: the SLSM reordering sends the top field of each phase (in order) followed by the bottom field of each phase (in order).
 * For instance, when given the following input stream:
 * +-------------------+  +-------------------+  +-------------------+
 * |                   |  |                   |  |                   |
 * |                   |  |                   |  |                   |
 * |                   |  |                   |  |                   |
 * |     F0 - top      |  |       F1 - top    |  |      F2 - top     |
 * |                   |  |                   |  |                   |
 * |                   |  |                   |  |                   |
 * +-------------------+  +-------------------+  +-------------------+
 * |                   |  |                   |  |                   |
 * |                   |  |                   |  |                   |
 * |                   |  |                   |  |                   |
 * |     F0 - bottom   |  |       F1 - bottom |  |     F2 - bottom   |
 * |                   |  |                   |  |                   |
 * |                   |  |                   |  |                   |
 * +-------------------+  +-------------------+  |-------------------|
 *
 * The SLSM ordering will output the following samples:
 *
 * +-------------------+ +-------------------+ +-------------------+
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * |     F0 - top      | |      F2 - top     | |       F1 - bottom |
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * +-------------------+ +-------------------+ +-------------------+
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * |       F1 - top    | |     F0 - bottom   | |     F2 - bottom   |
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * +-------------------+ +-------------------+ |-------------------|
 *
 * In order to respect top/bottom alternace in the output stream F1 - top will be considered as a bottom field and F1-bottom as a top field:
 *
 * +-------------------+ +-------------------+ +-------------------+
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * |     F0 - top      | |      F2 - top     | |       F1 - bottom |
 * |                   | |                   | |                   |
 * |                   | |                   | | patched to top    |
 * +-------------------+ +-------------------+ +-------------------+
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * |                   | |                   | |                   |
 * |       F1 - top    | |     F0 - bottom   | |     F2 - bottom   |
 * |                   | |                   | |                   |
 * |  patched to bottom| |                   | |                   |
 * +-------------------+ +-------------------+ |-------------------|
 *
 * The audio will only be taken (if available) from the first phase. Any audio on other phases will be ignored.
 *
 * Since frame-rates in SLSM get really high (8x720p50 becomes a 720p400), each phase is handled in a separate thread where samples are taken
 * and put in a ring buffer.
 *
 * This ring buffer is consumed by a single thread that performs the reordering and sends complete SLSM sequences to the output of this source:
 * Here is an example for a 3x SLSM configuration:
 *
 *                   +--------------+ +--------------+ +--------------+
 *                   |              | |              | |              |
 *                   |              | |              | |              |
 *                   |              | |              | |              |
 *                   |   Phase #0   | |   Phase #1   | |   Phase #2   |
 *                   |              | |              | |              |
 * feeder threads    |              | |              | |              |
 *                   |  GetSample() | |  GetSample() | |  GetSample() |
 *                   |              | |              | |              |
 *                   |              | |              | |              |
 *                   |              | |              | |              |
 *                   |              | |              | |              |
 *                   |              | |              | |              |
 *                   +--------------+ +--------------+ +--------------+
 *
 *                   +-----------+-----------+------------------------+----------+--------------------+---------+-----------
 *                   |           |           |           ||           |          |          ||        |         |         ||
 *                   |  P0 / F0  |  P1 / F0  |  P2 / F0  ||  P0 / F1  |   NULL   |  P2 / F1 ||  NULL  |  P1 / F2| P2 / F2 ||
 * ring buffer       |           |           |           ||           |          |          ||        |         |         ||
 *                   |           |           |           ||           |          |          ||        |         |         ||
 *                   |           |           |           ||           |          |          ||        |         |         ||
 *                   |           |           |           ||           |          |          ||        |         |         ||
 *                   +-----------+-----------+------------------------+----------+--------------------+---------+-----------
 *                                slot 0                            slot 1                            slot 2
 *
 *
 *                   +-----------------------------------------------------------------------------------------------------+
 *                   |                                                                                                     |
 *                   |    fetch data fromring buffer                                                                       |
 * consumer thread   |    check for incomplete sequences                                                                   |
 *                   |    reordering                                                                                       |
 *                   |                                                                                                     |
 *                   +----------------------------------------------+------------------------------------------------------+
 *                                                                  |
 *                                                                  |
 *                                                                  |
 *                                                                  |
 *                                                                  |
 *                                                                  v
 *
 * See the slsm_rbuf class for details about the ring buffer implementation.
 */
class evs_slsm_source: public media_source
{
public:
  // keep space for a latency of 6 progressive frames (3 interlaced)
  static constexpr const size_t SLSM_RBUF_SIZE{6};

  evs_slsm_source(const std::string& id);
  virtual ~evs_slsm_source() override = default;

  /**
   * @brief Adds an uninitialized source as the next phase for this SLSM setup.
   * The phase source will be initialized by the slsm source when required.
   * Note: The source must however be configured before being pushed inside the slsm source.
   * @param src
   */
  void add_phase(std::shared_ptr<eef_media_source> src);

  /**
   * @return The number of phases for this SLSM source.
   */
  inline size_t phase_count() const { return _phase_srcs.size(); }

  /**
   * @return the master phase from which SLSM infos are decided
   */
  std::shared_ptr<eef_media_source> master_source() const;

  virtual void init(const media_info& defined_input=media_info::null) override;
  virtual media_info defined_input(const std::string& pin_id) const override;
  virtual media_info defined_output(const std::string& pin_id) const override;

  virtual void start() override;
  virtual void stop() override;
  virtual bool is_running() const override;

  virtual bool push_next_sample(std::string& err) override;

protected:
  void patch_and_forward(const slsm_group_t& g);
  virtual void flush_consumer() override;

private:
  // faster, unsafe version of GetTimeInfo from the eef framework
  template<typename T>
  static const eef::TimeLimits& get_time_info(const T& sample, const std::string& id)
  {
    const auto& tls = sample->GetAllTimeInfo();
    auto lookup = std::find_if(tls.begin(), tls.end(), [&](const eef::TimeLimits& e) -> bool { return (e.GetClockInfo()->GetClockId().compare(id) == 0); });
    return const_cast<eef::TimeLimits&>(tls[static_cast<size_t>(std::distance(tls.begin(), lookup))]);
  }

  // Sub-sources & media info
  std::vector<std::shared_ptr<eef_media_source>> _phase_srcs;

  media_io_t _defined_output;

  // Extracted stream info
  bool _has_audio; // true if at least 1 audio info is available from the master source
  bool _is_interlaced; // true if each phase is an interlaced stream.
  eef::EditRate _phase_sample_rate; // the sample rate for a single phase. i.e: 50:1 for a 4xSLSM p50
  eef::EditRate _slsm_sample_rate; // the sample rate for all slsm phases. i.e: 200:1 for a 4xSLSM p50
  std::map<std::string, eef::ClockInfoPtr> _output_video_clock_info; // maps clock id to each duplicated clock info for the output video stream

  // Master stop-start switch
  std::atomic_bool _running;

  // Feed threads
  std::vector<std::shared_ptr<std::thread>> _feed_threads;
  slsm_rbuf_t<SLSM_RBUF_SIZE> _feed_buf;

  // Consumer thread
  std::shared_ptr<std::thread> _consumer_thread;
  std::chrono::milliseconds _consumer_wait_timeo; // timeout (in ms) for the consumer thread when waiting on the rbuf's condition variable
};

END_SXE_MEDIA_ENGINE_MODULE
