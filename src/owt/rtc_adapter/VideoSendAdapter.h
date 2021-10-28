// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_VIDEO_SEND_ADAPTER_
#define RTC_ADAPTER_VIDEO_SEND_ADAPTER_

#include <memory>

#include "api/field_trial_based_config.h"
#include "rtp_rtcp/rtp_rtcp.h"
#include "rtp_rtcp/rtp_rtcp_defines.h"
#include "rtp_rtcp/rtp_sender_video.h"
#include "rtc_base/random.h"
#include "rtc_base/rate_limiter.h"
#include "rtc_base/time_utils.h"

#include "rtc_adapter/RtcAdapter.h"
#include "rtc_adapter/AdapterInternalDefinitions.h"

#include "owt_base/MediaFramePipeline.h"
#include "owt_base/SsrcGenerator.h"
#include "rtc_base/location.h"
#include "utility/process_thread.h"


namespace rtc_adapter {

class VideoSendAdapterImpl : public VideoSendAdapter,
                             public webrtc::Transport,
                             public webrtc::RtcpIntraFrameObserver {
 public:
  VideoSendAdapterImpl(CallOwner* owner, const RtcAdapter::Config& config);
  ~VideoSendAdapterImpl();

  // Implement VideoSendAdapter
  void onFrame(const owt_base::Frame&) override;
  int onRtcpData(char* data, int len) override;
  void reset() override;

  uint32_t ssrc() { return m_ssrc; }

  // Implement webrtc::Transport
  bool SendRtp(const uint8_t* packet,
               size_t length,
               const webrtc::PacketOptions& options) override;
  bool SendRtcp(const uint8_t* packet, size_t length) override;

  // Implements webrtc::RtcpIntraFrameObserver.
  void OnReceivedIntraFrameRequest(uint32_t ssrc);
  void OnReceivedSLI(uint32_t ssrc, uint8_t picture_id) {}
  void OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id) {}
  void OnLocalSsrcChanged(uint32_t old_ssrc, uint32_t new_ssrc) {}

 private:
  bool init();

  bool m_enableDump{false};
  RtcAdapter::Config m_config;

  bool m_keyFrameArrived{false};
  std::unique_ptr<webrtc::RateLimiter> m_retransmissionRateLimiter;
  std::unique_ptr<webrtc::RtpRtcp> m_rtpRtcp;

  owt_base::FrameFormat m_frameFormat;
  uint16_t m_frameWidth{0};
  uint16_t m_frameHeight{0};
  webrtc::Random m_random;
  uint32_t m_ssrc{0};
  owt_base::SsrcGenerator* const m_ssrcGenerator;

  webrtc::Clock* m_clock{nullptr};
  int64_t m_timeStampOffset{0};

  std::unique_ptr<webrtc::RtcEventLog> m_eventLog;
  std::unique_ptr<webrtc::RTPSenderVideo> m_senderVideo;
  std::unique_ptr<webrtc::PlayoutDelayOracle> m_playoutDelayOracle;
  std::unique_ptr<webrtc::FieldTrialBasedConfig> m_fieldTrialConfig;

  // Listeners
  AdapterFeedbackListener* m_feedbackListener;
  AdapterDataListener* m_dataListener;
  AdapterStatsListener* m_statsListener;

  //std::shared_ptr<owt_base::WebRTCTaskRunner> m_taskRunner;
  std::unique_ptr<webrtc::ProcessThread> m_taskRunner;
};

} //namespace owt

#endif /* RTC_ADAPTER_VIDEO_SEND_ADAPTER_ */
