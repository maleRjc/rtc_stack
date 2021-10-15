// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "owt_base/MediaFramePipeline.h"

namespace owt_base {

FrameSource::~FrameSource() {
  for (auto it = m_audio_dests.begin(); it != m_audio_dests.end(); ++it) {
    (*it)->unsetAudioSource();
  }

  m_audio_dests.clear();

  for (auto it = m_video_dests.begin(); it != m_video_dests.end(); ++it) {
    (*it)->unsetVideoSource();
  }

  m_video_dests.clear();
}

void FrameSource::addAudioDestination(FrameDestination* dest) {
  m_audio_dests.push_back(dest);
  dest->setAudioSource(this);
}

void FrameSource::addVideoDestination(FrameDestination* dest) {
  m_video_dests.push_back(dest);
  dest->setVideoSource(this);
}

void FrameSource::addDataDestination(FrameDestination* dest) {
  m_data_dests.push_back(dest);
  dest->setDataSource(this);
}

void FrameSource::removeAudioDestination(FrameDestination* dest) {
  m_audio_dests.remove(dest);
  dest->unsetAudioSource();
}

void FrameSource::removeVideoDestination(FrameDestination* dest) {
  m_video_dests.remove(dest);
  dest->unsetVideoSource();
}

void FrameSource::removeDataDestination(FrameDestination* dest) {
  m_data_dests.remove(dest);
  dest->unsetDataSource();
}

void FrameSource::deliverFrame(const Frame& frame) {
  
  if (isAudioFrame(frame)) {
    for (auto it = m_audio_dests.begin(); it != m_audio_dests.end(); ++it) {
      (*it)->onFrame(frame);
    }
  } else if (isVideoFrame(frame)) {
    for (auto it = m_video_dests.begin(); it != m_video_dests.end(); ++it) {
      (*it)->onFrame(frame);
    }
  } else if (isDataFrame(frame)){
    for (auto it = m_data_dests.begin(); it != m_data_dests.end(); ++it) {
      (*it)->onFrame(frame);
    }
  } else {
    //TODO: log error here.
  }
}

void FrameSource::deliverMetaData(const MetaData& metadata) {
  for (auto it = m_audio_dests.begin(); it != m_audio_dests.end(); ++it) {
    (*it)->onMetaData(metadata);
  }

  for (auto it = m_video_dests.begin(); it != m_video_dests.end(); ++it) {
    (*it)->onMetaData(metadata);
  }
}

/*============================================================================*/
void FrameDestination::setAudioSource(FrameSource* src) {
  m_audio_src = src;
}

void FrameDestination::setVideoSource(FrameSource* src) {
  m_video_src = src;
  onVideoSourceChanged();
}

void FrameDestination::setDataSource(FrameSource* src) {
  m_data_src = src;
}

void FrameDestination::unsetAudioSource() {
  m_audio_src = nullptr;
}

void FrameDestination::unsetVideoSource() {
  m_video_src = nullptr;
}

void FrameDestination::unsetDataSource() {
  m_data_src = nullptr;
}

void FrameDestination::deliverFeedbackMsg(const FeedbackMsg& msg) {
  if (msg.type == AUDIO_FEEDBACK) {
    if (m_audio_src) {
      m_audio_src->onFeedback(msg);
    }
  } else if (msg.type == VIDEO_FEEDBACK) {
    if (m_video_src) {
      m_video_src->onFeedback(msg);
    }
  } else {
    //TODO: log error here.
  }
}

}

