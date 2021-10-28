// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "owt_base/VideoFrameConstructor.h"

#include <future>
#include <random>
#include "common/rtputils.h"
#include "myrtc/api/task_queue_base.h"

using namespace rtc_adapter;

namespace owt_base {

DEFINE_LOGGER(VideoFrameConstructor, "owt.VideoFrameConstructor");

VideoFrameConstructor::VideoFrameConstructor(
    VideoInfoListener* vil, const config&config, wa::Worker* worker)
  : config_(config),
    m_videoInfoListener(vil),
    m_rtcAdapter(std::move(
        RtcAdapterFactory::CreateRtcAdapter(worker->getTaskQueue()))),
    worker_{worker} {
  OLOG_TRACE_THIS("");
  m_feedbackTimer = SharedJobTimer::GetSharedFrequencyTimer(1);
  m_feedbackTimer->addListener(this);
}

VideoFrameConstructor::~VideoFrameConstructor() {
  OLOG_TRACE_THIS("");

  m_feedbackTimer->removeListener(this);
  unbindTransport();
  if (m_videoReceive) {
    m_rtcAdapter->destoryVideoReceiver(m_videoReceive);
    m_rtcAdapter.reset();
    m_videoReceive = nullptr;
  }
}

void VideoFrameConstructor::maybeCreateReceiveVideo(uint32_t ssrc) {
  if (m_videoReceive) {
    return;
  }
  m_ssrc = config_.ssrc;
  
  OLOG_INFO_THIS("create CreateReceiveVideo, ssrc:" << m_ssrc << 
                 ", rtx:" << config_.rtx_ssrc << 
                 ", inpt:" << ssrc);

  // Create Receive Video Stream
  rtc_adapter::RtcAdapter::Config recvConfig;

  recvConfig.ssrc = config_.ssrc;
  recvConfig.rtx_ssrc = config_.rtx_ssrc;
  recvConfig.rtcp_rsize = config_.rtcp_rsize;
  recvConfig.rtp_payload_type = config_.rtp_payload_type;
  recvConfig.transport_cc = config_.transportcc;
  recvConfig.red_payload = config_.red_payload;
  recvConfig.ulpfec_payload = config_.ulpfec_payload;
  recvConfig.flex_fec = config_.flex_fec;
  recvConfig.rtp_listener = this;
  recvConfig.stats_listener = this;
  recvConfig.frame_listener = this;

  m_videoReceive = m_rtcAdapter->createVideoReceiver(recvConfig);
}

void VideoFrameConstructor::bindTransport(
    erizo::MediaSource* source, erizo::FeedbackSink* fbSink) {
  m_transport = source;
  m_transport->setVideoSink(this);
  m_transport->setEventSink(this);
  setFeedbackSink(fbSink);
}

void VideoFrameConstructor::unbindTransport() {
  if (m_transport) {
    setFeedbackSink(nullptr);
    m_transport = nullptr;
  }
}

void VideoFrameConstructor::enable(bool enabled) {
  m_enabled = enabled;
  RequestKeyFrame();
}

int32_t VideoFrameConstructor::RequestKeyFrame() {
  if (!m_enabled) {
    return 0;
  }
  if (m_videoReceive) {
    m_videoReceive->requestKeyFrame();
  }
  return 0;
}

bool VideoFrameConstructor::setBitrate(uint32_t kbps) {
  // At present we do not react on this request
  return true;
}

void VideoFrameConstructor::onAdapterFrame(const Frame& frame) {
  if (m_enabled) {
    deliverFrame(frame);
  }
}

void VideoFrameConstructor::onAdapterStats(const AdapterStats& stats) {
  if (m_videoInfoListener) {
    std::ostringstream json_str;
    json_str.str("");
    json_str << "{\"video\": {\"parameters\": {\"resolution\": {"
             << "\"width\":" << stats.width << ", "
             << "\"height\":" << stats.height
             << "}}}}";
    m_videoInfoListener->onVideoInfo(json_str.str().c_str());
  }
}

void VideoFrameConstructor::onAdapterData(char* data, int len) {
  // Data come from video receive stream is RTCP
  if (fb_sink_) {
    fb_sink_->deliverFeedback(
      std::make_shared<erizo::DataPacket>(0, data, len, erizo::VIDEO_PACKET));
  }
}

int VideoFrameConstructor::deliverVideoData_(
    std::shared_ptr<erizo::DataPacket> video_packet) {
  RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(video_packet->data);
  uint8_t packetType = chead->getPacketType();

  assert(packetType != RTCP_Receiver_PT && 
         packetType != RTCP_PS_Feedback_PT && 
         packetType != RTCP_RTP_Feedback_PT);
  if (packetType == RTCP_Sender_PT)
    return 0;

  if (packetType >= RTCP_MIN_PT && packetType <= RTCP_MAX_PT) {
    return 0;
  }

  RTPHeader* head = reinterpret_cast<RTPHeader*>(video_packet->data);
  if (!m_ssrc && head->getSSRC()) {
    maybeCreateReceiveVideo(head->getSSRC());
  }
  if (m_videoReceive) {
    m_videoReceive->onRtpData(video_packet->data, video_packet->length);
  }

  return video_packet->length;
}

int VideoFrameConstructor::deliverAudioData_(
    std::shared_ptr<erizo::DataPacket> audio_packet) {
  assert(false);
  return 0;
}

void VideoFrameConstructor::onTimeout() {
  if (m_pendingKeyFrameRequests > 1) {
      RequestKeyFrame();
  }
  m_pendingKeyFrameRequests = 0;
}

void VideoFrameConstructor::onFeedback(const FeedbackMsg& msg) {
  if (msg.type != owt_base::VIDEO_FEEDBACK) {
    return;
  }
  auto share_this = std::dynamic_pointer_cast<VideoFrameConstructor>(shared_from_this());
  std::weak_ptr<VideoFrameConstructor> weak_this = share_this;
  
  worker_->task([msg, weak_this, this]() {
    if (auto share_this = weak_this.lock()) {
      if (msg.cmd == REQUEST_KEY_FRAME) {
        if (!m_pendingKeyFrameRequests) {
            RequestKeyFrame();
        }
        ++m_pendingKeyFrameRequests;
      } else if (msg.cmd == SET_BITRATE) {
        this->setBitrate(msg.data.kbps);
      }      
    }
  });
}

void VideoFrameConstructor::close() {
  unbindTransport();
}

}

