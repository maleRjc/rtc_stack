// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "owt_base/VideoFramePacketizer.h"
#include "owt_base/MediaUtilities.h"
#include "common/rtputils.h"
#include "myrtc/api/task_queue_base.h"
#include "rtc_adapter/thread/StaticTaskQueueFactory.h"

using namespace rtc_adapter;

namespace owt_base {

// To make it consistent with the webrtc library, 
// we allow packets to be transmitted
// in up to 2 times max video bitrate if the bandwidth estimate allows it.
static const int TRANSMISSION_MAXBITRATE_MULTIPLIER = 2;

DEFINE_LOGGER(VideoFramePacketizer, "owt.VideoFramePacketizer");

VideoFramePacketizer::VideoFramePacketizer(VideoFramePacketizer::Config& config)
    : m_rtcAdapter{std::move(config.factory->CreateRtcAdapter())} {
  auto factory = rtc_adapter::createDummyTaskQueueFactory(config.task_queue);
  auto task_queue = factory->CreateTaskQueue(
      "deliver_frame", webrtc::TaskQueueFactory::Priority::NORMAL);
  task_queue_ = std::move(
      std::make_unique<rtc::TaskQueue>(std::move(task_queue)));
  
  init(config);
}

VideoFramePacketizer::~VideoFramePacketizer() {
  close();
  if (m_videoSend) {
    m_rtcAdapter->destoryVideoSender(m_videoSend);
    m_videoSend = nullptr;
  }
}

bool VideoFramePacketizer::init(VideoFramePacketizer::Config& config) {
  if (m_videoSend) {
    return false;
  }
  
  // Create Send Video Stream
  rtc_adapter::RtcAdapter::Config sendConfig;

  sendConfig.transport_cc = config.transportccExt;
  sendConfig.red_payload = config.Red;
  sendConfig.ulpfec_payload = config.Ulpfec;
  if (!config.mid.empty()) {
    strncpy(sendConfig.mid, config.mid.c_str(), sizeof(sendConfig.mid) - 1);
    sendConfig.mid_ext = config.midExtId;
  }
  sendConfig.feedback_listener = this;
  sendConfig.rtp_listener = this;
  sendConfig.stats_listener = this;
  m_videoSend = m_rtcAdapter->createVideoSender(sendConfig);
  m_ssrc = m_videoSend->ssrc();
  return true;
}

void VideoFramePacketizer::bindTransport(erizo::MediaSink* sink) {
  video_sink_ = sink;
  video_sink_->setVideoSinkSSRC(m_videoSend->ssrc());
  erizo::FeedbackSource* fbSource = video_sink_->getFeedbackSource();
  if (fbSource)
      fbSource->setFeedbackSink(this);
}

void VideoFramePacketizer::unbindTransport() {
  if (video_sink_) {
    video_sink_ = nullptr;
  }
}

void VideoFramePacketizer::enable(bool enabled) {
  m_enabled = enabled;
  if (m_enabled) {
    m_sendFrameCount = 0;
    if (m_videoSend) {
      m_videoSend->reset();
    }
  }
}

void VideoFramePacketizer::onFeedback(const FeedbackMsg& msg) {
  deliverFeedbackMsg(msg);
}

void VideoFramePacketizer::onAdapterData(char* data, int len) {
  if (!video_sink_) {
    return;
  }

  video_sink_->deliverVideoData(std::make_shared<erizo::DataPacket>(
      0, data, len, erizo::VIDEO_PACKET));
}

void VideoFramePacketizer::onFrame(const Frame& frame) {
  if (frame.length <= 0) {
    return ;
  }
  
  task_queue_->PostTask([this, weak_ptr = weak_from_this(), frame] () {
    if (auto shared_this = weak_ptr.lock()) {
      if (!m_enabled) {
        return;
      }
      
      if (m_selfRequestKeyframe) {
        //FIXME: This is a workround for peer client not send key-frame-request
        if (m_sendFrameCount < 151) {
          if ((m_sendFrameCount == 10)
            || (m_sendFrameCount == 30)
            || (m_sendFrameCount == 60)
            || (m_sendFrameCount == 150)) {
            // ELOG_DEBUG("Self generated key-frame-request.");
            FeedbackMsg feedback = 
                {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME };
            deliverFeedbackMsg(feedback);
          }
          m_sendFrameCount += 1;
        }
      }

      if (m_videoSend) {
        m_videoSend->onFrame(frame);
      }
    }
  });
}

void VideoFramePacketizer::onVideoSourceChanged() {
  if (m_videoSend) {
    m_videoSend->reset();
  }
}

int VideoFramePacketizer::sendFirPacket() {
  FeedbackMsg feedback = {.type = VIDEO_FEEDBACK, .cmd = REQUEST_KEY_FRAME };
  deliverFeedbackMsg(feedback);
  return 0;
}

void VideoFramePacketizer::close() {
  unbindTransport();
}

int VideoFramePacketizer::deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet) {
  if (m_videoSend) {
    m_videoSend->onRtcpData(data_packet->data, data_packet->length);
    return data_packet->length;
  }
  return 0;
}

int VideoFramePacketizer::sendPLI() {
  return 0;
}

} //namespace owt

