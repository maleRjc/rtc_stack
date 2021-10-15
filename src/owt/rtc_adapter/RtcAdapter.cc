// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "RtcAdapter.h"

#include <memory>
#include <mutex>

#include "rtc_base/clock.h"
#include "rtc_adapter/thread/ProcessThreadMock.h"
#include "rtc_adapter/thread/StaticTaskQueueFactory.h"
#include "rtc_adapter/AdapterInternalDefinitions.h"
#include "rtc_adapter/AudioSendAdapter.h"
#include "rtc_adapter/VideoSendAdapter.h"
#include "rtc_adapter/VideoReceiveAdapter.h"

namespace rtc_adapter {

/////////////////////////
//RtcAdapterImpl
class RtcAdapterImpl : public RtcAdapter,
                       public CallOwner {
public:
  RtcAdapterImpl(webrtc::TaskQueueBase*);
  virtual ~RtcAdapterImpl() { }

  // Implement RtcAdapter
  VideoReceiveAdapter* createVideoReceiver(const Config&) override;
  void destoryVideoReceiver(VideoReceiveAdapter*) override;
  
  VideoSendAdapter* createVideoSender(const Config&) override;
  void destoryVideoSender(VideoSendAdapter*) override;
  
  AudioReceiveAdapter* createAudioReceiver(const Config&) override;
  void destoryAudioReceiver(AudioReceiveAdapter*) override;
  
  AudioSendAdapter* createAudioSender(const Config&) override;
  void destoryAudioSender(AudioSendAdapter*) override;

  // Implement CallOwner
  std::shared_ptr<webrtc::Call> call() override { return m_call; }
  std::shared_ptr<rtc::TaskQueue> taskQueue() override { return m_taskQueue; }
  std::shared_ptr<webrtc::RtcEventLog> eventLog() override { return m_eventLog; }

private:
  void initCall();

  std::unique_ptr<webrtc::TaskQueueFactory> m_taskQueueFactory;
  std::shared_ptr<rtc::TaskQueue> m_taskQueue;
  std::shared_ptr<webrtc::RtcEventLog> m_eventLog;
  std::shared_ptr<webrtc::Call> m_call;
};

RtcAdapterImpl::RtcAdapterImpl(webrtc::TaskQueueBase* p)
  : m_taskQueueFactory(createDummyTaskQueueFactory(p)),
    m_taskQueue(std::make_shared<rtc::TaskQueue>(m_taskQueueFactory->CreateTaskQueue(
                "CallTaskQueue",
                webrtc::TaskQueueFactory::Priority::NORMAL))),
    m_eventLog(std::make_shared<webrtc::RtcEventLogNull>()) {
}

void RtcAdapterImpl::initCall() {
  // Initialize call
  if (!m_call) {
    webrtc::Call::Config call_config(m_eventLog.get());
    call_config.task_queue_factory = m_taskQueueFactory.get();

    std::unique_ptr<webrtc::ProcessThread> moduleThread =
      std::make_unique<ProcessThreadMock>(m_taskQueue.get());
      
    m_call.reset(
      webrtc::Call::Create(call_config, 
                           webrtc::Clock::GetRealTimeClock(), 
                           std::move(moduleThread)));
  }
}

VideoReceiveAdapter* RtcAdapterImpl::createVideoReceiver(const Config& config) {
  initCall();
  return new VideoReceiveAdapterImpl(this, config);
}

void RtcAdapterImpl::destoryVideoReceiver(VideoReceiveAdapter* video_recv_adapter) {
  VideoReceiveAdapterImpl* impl = static_cast<VideoReceiveAdapterImpl*>(video_recv_adapter);
  delete impl;
}

VideoSendAdapter* RtcAdapterImpl::createVideoSender(const Config& config) {
  return new VideoSendAdapterImpl(this, config);
}

void RtcAdapterImpl::destoryVideoSender(VideoSendAdapter* video_send_adapter) {
  VideoSendAdapterImpl* impl = static_cast<VideoSendAdapterImpl*>(video_send_adapter);
  delete impl;
}

AudioReceiveAdapter* RtcAdapterImpl::createAudioReceiver(const Config& config) {
  return nullptr;
}

void RtcAdapterImpl::destoryAudioReceiver(AudioReceiveAdapter* audio_recv_adapter) {}

AudioSendAdapter* RtcAdapterImpl::createAudioSender(const Config& config) {
  return new AudioSendAdapterImpl(this, config);
}

void RtcAdapterImpl::destoryAudioSender(AudioSendAdapter* audio_send_adapter) {
  AudioSendAdapterImpl* impl = static_cast<AudioSendAdapterImpl*>(audio_send_adapter);
  delete impl;
}

/////////////////////////
//RtcAdapterFactory
std::shared_ptr<RtcAdapter>
RtcAdapterFactory::CreateRtcAdapter(webrtc::TaskQueueBase* p) {
  return std::dynamic_pointer_cast<RtcAdapter>(
             std::make_shared<RtcAdapterImpl>(p));
}

} // namespace rtc_adapter

