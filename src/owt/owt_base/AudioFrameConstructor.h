// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioFrameConstructor_h
#define AudioFrameConstructor_h

#include "owt_base/MediaFramePipeline.h"
#include "owt_base/MediaDefinitionExtra.h"
#include "erizo/MediaDefinitions.h"
#include "common/logger.h"

namespace owt_base {

/**
 * A class to process the incoming streams by leveraging video coding module from
 * webrtc engine, which will framize the frames.
 */
class AudioFrameConstructor final : public erizo::MediaSink,
                                    public erizo::FeedbackSource,
                                    public FrameSource {
    DECLARE_LOGGER();

public:
    AudioFrameConstructor();
    virtual ~AudioFrameConstructor();

    void bindTransport(erizo::MediaSource* source, erizo::FeedbackSink* fbSink);
    void unbindTransport();
    void enable(bool enabled) { m_enabled = enabled; }

    // Implements the FrameSource interfaces.
    void onFeedback(const FeedbackMsg& msg);

private:
    bool m_enabled;
    erizo::MediaSource* m_transport;

    // Implement erizo::MediaSink
    int deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet) override;
    int deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet) override;
    int deliverEvent_(erizo::MediaEventPtr event) override;
    void close();
};

}
#endif /* AudioFrameConstructor_h */

