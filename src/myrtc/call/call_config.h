/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef CALL_CALL_CONFIG_H_
#define CALL_CALL_CONFIG_H_

#include "api/fec_controller.h"
#include "api/network_state_predictor.h"
#include "api/rtc_error.h"
#include "api/task_queue_factory.h"
#include "api/bitrate_settings.h"
#include "api/network_control.h"

namespace webrtc {

class RtcEventLog;

struct CallConfig {
  explicit CallConfig(RtcEventLog* event_log);
  CallConfig(const CallConfig&);
  ~CallConfig();

  RTC_DEPRECATED static constexpr int kDefaultStartBitrateBps = 300000;

  // Bitrate config used until valid bitrate estimates are calculated. Also
  // used to cap total bitrate used. This comes from the remote connection.
  BitrateConstraints bitrate_config;

  // RtcEventLog to use for this call. Required.
  // Use webrtc::RtcEventLog::CreateNull() for a null implementation.
  RtcEventLog* event_log = nullptr;

  // FecController to use for this call.
  //FecControllerFactoryInterface* fec_controller_factory = nullptr;

  // Task Queue Factory to be used in this call. Required.
  TaskQueueFactory* task_queue_factory = nullptr;

  // NetworkStatePredictor to use for this call.
  NetworkStatePredictorFactoryInterface* network_state_predictor_factory =
      nullptr;

  // Network controller factory to use for this call.
  NetworkControllerFactoryInterface* network_controller_factory = nullptr;
};

}  // namespace webrtc

#endif  // CALL_CALL_CONFIG_H_
