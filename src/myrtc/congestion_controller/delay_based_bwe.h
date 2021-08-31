/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_CONGESTION_CONTROLLER_GOOG_CC_DELAY_BASED_BWE_H_
#define MODULES_CONGESTION_CONTROLLER_GOOG_CC_DELAY_BASED_BWE_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "optional"
#include "api/network_state_predictor.h"
#include "api/network_types.h"
#include "api/webrtc_key_value_config.h"
#include "congestion_controller/delay_increase_detector_interface.h"
#include "congestion_controller/probe_bitrate_estimator.h"
#include "remote_bitrate_estimator/aimd_rate_control.h"
#include "remote_bitrate_estimator/bwe_defines.h"
#include "remote_bitrate_estimator/inter_arrival.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/race_checker.h"

namespace webrtc {
class RtcEventLog;

class DelayBasedBwe {
 public:
  struct Result {
    Result();
    Result(bool probe, DataRate target_bitrate);
    ~Result();
    bool updated;
    bool probe;
    DataRate target_bitrate = DataRate::Zero();
    bool recovered_from_overuse;
    bool backoff_in_alr;
  };

  explicit DelayBasedBwe(const WebRtcKeyValueConfig* key_value_config,
                         RtcEventLog* event_log,
                         NetworkStatePredictor* network_state_predictor);
  virtual ~DelayBasedBwe();

  Result IncomingPacketFeedbackVector(
      const TransportPacketsFeedback& msg,
      std::optional<DataRate> acked_bitrate,
      std::optional<DataRate> probe_bitrate,
      std::optional<NetworkStateEstimate> network_estimate,
      bool in_alr);
  void OnRttUpdate(TimeDelta avg_rtt);
  bool LatestEstimate(std::vector<uint32_t>* ssrcs, DataRate* bitrate) const;
  void SetStartBitrate(DataRate start_bitrate);
  void SetMinBitrate(DataRate min_bitrate);
  TimeDelta GetExpectedBwePeriod() const;
  void SetAlrLimitedBackoffExperiment(bool enabled);

  DataRate TriggerOveruse(Timestamp at_time,
                          std::optional<DataRate> link_capacity);

 private:
  friend class GoogCcStatePrinter;
  void IncomingPacketFeedback(const PacketResult& packet_feedback,
                              Timestamp at_time);
  Result MaybeUpdateEstimate(
      std::optional<DataRate> acked_bitrate,
      std::optional<DataRate> probe_bitrate,
      std::optional<NetworkStateEstimate> state_estimate,
      bool recovered_from_overuse,
      bool in_alr,
      Timestamp at_time);
  // Updates the current remote rate estimate and returns true if a valid
  // estimate exists.
  bool UpdateEstimate(Timestamp now,
                      std::optional<DataRate> acked_bitrate,
                      DataRate* target_bitrate);

  rtc::RaceChecker network_race_;
  RtcEventLog* const event_log_;
  const WebRtcKeyValueConfig* const key_value_config_;
  NetworkStatePredictor* network_state_predictor_;
  std::unique_ptr<InterArrival> inter_arrival_;
  std::unique_ptr<DelayIncreaseDetectorInterface> delay_detector_;
  Timestamp last_seen_packet_;
  bool uma_recorded_;
  AimdRateControl rate_control_;
  DataRate prev_bitrate_;
  bool has_once_detected_overuse_;
  BandwidthUsage prev_state_;
  bool alr_limited_backoff_enabled_;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(DelayBasedBwe);
};

}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_GOOG_CC_DELAY_BASED_BWE_H_
