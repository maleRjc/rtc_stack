/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_RTP_RTCP_SOURCE_RTCP_SENDER_H_
#define MODULES_RTP_RTCP_SOURCE_RTCP_SENDER_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "optional"
#include "api/transport.h"
#include "api/video_bitrate_allocation.h"
#include "remote_bitrate_estimator/bwe_defines.h"
#include "remote_bitrate_estimator/remote_bitrate_estimator.h"
#include "rtp_rtcp/receive_statistics.h"
#include "rtp_rtcp/rtp_rtcp.h"
#include "rtp_rtcp/rtp_rtcp_defines.h"
#include "rtp_rtcp/rtcp_nack_stats.h"
#include "rtp_rtcp/rtcp_packet.h"
#include "rtp_rtcp/dlrr.h"
#include "rtp_rtcp/report_block.h"
#include "rtp_rtcp/tmmb_item.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/random.h"
#include "rtc_base/thread_annotations.h"

namespace webrtc {

class ModuleRtpRtcpImpl;
class RtcEventLog;

class RTCPSender {
 public:
  struct FeedbackState {
    FeedbackState();
    FeedbackState(const FeedbackState&);
    FeedbackState(FeedbackState&&);

    ~FeedbackState();

    uint32_t packets_sent;
    size_t media_bytes_sent;
    uint32_t send_bitrate;

    uint32_t last_rr_ntp_secs;
    uint32_t last_rr_ntp_frac;
    uint32_t remote_sr;

    std::vector<rtcp::ReceiveTimeInfo> last_xr_rtis;

    // Used when generating TMMBR.
    ModuleRtpRtcpImpl* module;
  };

  explicit RTCPSender(const RtpRtcp::Configuration& config);
  virtual ~RTCPSender();

  RtcpMode Status() const;
  void SetRTCPStatus(RtcpMode method);

  bool Sending() const;
  int32_t SetSendingStatus(const FeedbackState& feedback_state,
                           bool enabled);  // combine the functions

  int32_t SetNackStatus(bool enable);

  void SetTimestampOffset(uint32_t timestamp_offset);

  // TODO(bugs.webrtc.org/6458): Remove default parameter value when all the
  // depending projects are updated to correctly set payload type.
  void SetLastRtpTime(uint32_t rtp_timestamp,
                      int64_t capture_time_ms,
                      int8_t payload_type = -1);

  void SetRtpClockRate(int8_t payload_type, int rtp_clock_rate_hz);

  uint32_t SSRC() const { return ssrc_; }

  void SetRemoteSSRC(uint32_t ssrc);

  int32_t SetCNAME(const char* cName);

  int32_t AddMixedCNAME(uint32_t SSRC, const char* c_name);

  int32_t RemoveMixedCNAME(uint32_t SSRC);

  bool TimeToSendRTCPReport(bool sendKeyframeBeforeRTP = false) const;

  int32_t SendRTCP(const FeedbackState& feedback_state,
                   RTCPPacketType packetType,
                   int32_t nackSize = 0,
                   const uint16_t* nackList = 0);

  int32_t SendCompoundRTCP(const FeedbackState& feedback_state,
                           const std::set<RTCPPacketType>& packetTypes,
                           int32_t nackSize = 0,
                           const uint16_t* nackList = 0);

  int32_t SendLossNotification(const FeedbackState& feedback_state,
                               uint16_t last_decoded_seq_num,
                               uint16_t last_received_seq_num,
                               bool decodability_flag,
                               bool buffering_allowed);

  void SetRemb(int64_t bitrate_bps, std::vector<uint32_t> ssrcs);

  void UnsetRemb();

  bool TMMBR() const;

  void SetTMMBRStatus(bool enable);

  void SetMaxRtpPacketSize(size_t max_packet_size);

  void SetTmmbn(std::vector<rtcp::TmmbItem> bounding_set);

  int32_t SetApplicationSpecificData(uint8_t subType,
                                     uint32_t name,
                                     const uint8_t* data,
                                     uint16_t length);

  void SendRtcpXrReceiverReferenceTime(bool enable);

  bool RtcpXrReceiverReferenceTime() const;

  void SetCsrcs(const std::vector<uint32_t>& csrcs);

  void SetTargetBitrate(unsigned int target_bitrate);
  void SetVideoBitrateAllocation(const VideoBitrateAllocation& bitrate);
  void SendCombinedRtcpPacket(
      std::vector<std::unique_ptr<rtcp::RtcpPacket>> rtcp_packets);

 private:
  class RtcpContext;

  // Determine which RTCP messages should be sent and setup flags.
  void PrepareReport(const FeedbackState& feedback_state);

  std::vector<rtcp::ReportBlock> CreateReportBlocks(
      const FeedbackState& feedback_state);

  std::unique_ptr<rtcp::RtcpPacket> BuildSR(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildRR(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildSDES(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildPLI(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildREMB(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildTMMBR(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildTMMBN(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildAPP(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildLossNotification(
      const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildExtendedReports(
      const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildBYE(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildFIR(const RtcpContext& context);
  std::unique_ptr<rtcp::RtcpPacket> BuildNACK(const RtcpContext& context);

 private:
  const bool audio_;
  const uint32_t ssrc_;
  Clock* const clock_;
  Random random_;
  RtcpMode method_;

  RtcEventLog* const event_log_;
  Transport* const transport_;

  const int report_interval_ms_;

  bool sending_;

  int64_t next_time_to_send_rtcp_;

  uint32_t timestamp_offset_;
  uint32_t last_rtp_timestamp_;
  int64_t last_frame_capture_time_ms_;
  // SSRC that we receive on our RTP channel
  uint32_t remote_ssrc_;
  std::string cname_;

  ReceiveStatisticsProvider* receive_statistics_;
  std::map<uint32_t, std::string> csrc_cnames_;

  // send CSRCs
  std::vector<uint32_t> csrcs_;

  // Full intra request
  uint8_t sequence_number_fir_ ;

  // Loss Notification
  struct LossNotificationState {
    uint16_t last_decoded_seq_num;
    uint16_t last_received_seq_num;
    bool decodability_flag;
  };
  LossNotificationState loss_notification_state_;

  // REMB
  int64_t remb_bitrate_;
  std::vector<uint32_t> remb_ssrcs_;

  std::vector<rtcp::TmmbItem> tmmbn_to_send_;
  uint32_t tmmbr_send_bps_;
  uint32_t packet_oh_send_;
  size_t max_packet_size_;

  // APP
  uint8_t app_sub_type_;
  uint32_t app_name_;
  std::unique_ptr<uint8_t[]> app_data_
     ;
  uint16_t app_length_;

  // True if sending of XR Receiver reference time report is enabled.
  bool xr_send_receiver_reference_time_enabled_;

  RtcpPacketTypeCounterObserver* const packet_type_counter_observer_;
  RtcpPacketTypeCounter packet_type_counter_;

  RtcpNackStats nack_stats_;

  VideoBitrateAllocation video_bitrate_allocation_;
  bool send_video_bitrate_allocation_;

  std::map<int8_t, int> rtp_clock_rates_khz_;
  int8_t last_payload_type_;

  std::optional<VideoBitrateAllocation> CheckAndUpdateLayerStructure(
      const VideoBitrateAllocation& bitrate) const;

  void SetFlag(uint32_t type, bool is_volatile);
  void SetFlags(const std::set<RTCPPacketType>& types, bool is_volatile);
  bool IsFlagPresent(uint32_t type) const;
  bool ConsumeFlag(uint32_t type, bool forced = false);
  bool AllVolatileFlagsConsumed() const;
  struct ReportFlag {
    ReportFlag(uint32_t type, bool is_volatile)
        : type(type), is_volatile(is_volatile) {}
    bool operator<(const ReportFlag& flag) const { return type < flag.type; }
    bool operator==(const ReportFlag& flag) const { return type == flag.type; }
    const uint32_t type;
    const bool is_volatile;
  };

  std::set<ReportFlag> report_flags_;

  typedef std::unique_ptr<rtcp::RtcpPacket> (RTCPSender::*BuilderFunc)(
      const RtcpContext&);
  // Map from RTCPPacketType to builder.
  std::map<uint32_t, BuilderFunc> builders_;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(RTCPSender);
};
}  // namespace webrtc

#endif  // MODULES_RTP_RTCP_SOURCE_RTCP_SENDER_H_
