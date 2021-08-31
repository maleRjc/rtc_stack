#include "media_config.h"

namespace wa
{

const std::string extMappings[EXT_MAP_SIZE] = {
  "urn:ietf:params:rtp-hdrext:ssrc-audio-level",
  "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01",
  "urn:ietf:params:rtp-hdrext:sdes:mid",
  "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id",
  "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id",
  "urn:ietf:params:rtp-hdrext:toffset",
  "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time"
  // 'urn:3gpp:video-orientation',
  // 'http://www.webrtc.org/experiments/rtp-hdrext/playout-delay',
};

const std::map<std::string, int> extMappings2Id = {
  {"urn:ietf:params:rtp-hdrext:ssrc-audio-level", EExtmap::AudioLevel},
  {"http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01", EExtmap::TransportCC},
  {"urn:ietf:params:rtp-hdrext:sdes:mid", EExtmap::SdesMid},
  {"urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id", EExtmap::SdesRtpStreamId},
  {"urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id", EExtmap::SdesRepairedRtpStreamId}, 
  {"urn:ietf:params:rtp-hdrext:toffset", EExtmap::Toffset},
  {"http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time", EExtmap::AbsSendTime}
  // 'urn:3gpp:video-orientation',
  // 'http://www.webrtc.org/experiments/rtp-hdrext/playout-delay',
};


erizo::RtpMap rtpH264{
  127,
  "H264",
  90000,
  erizo::MediaType::VIDEO_TYPE,
  1,
  {"ccm fir", "nack", "transport-cc", "goog-remb"},
  {}
};

erizo::RtpMap rtpRed{
  116,
  "red",
  90000,
  erizo::MediaType::VIDEO_TYPE,
  1,
  {},
  {}
};

erizo::RtpMap rtpRtx{
  96,
  "rtx",
  90000,
  erizo::MediaType::VIDEO_TYPE,
  1,
  {},
  {}
};

erizo::RtpMap rtpUlpfec{
  117,
  "ulpfec",
  90000,
  erizo::MediaType::VIDEO_TYPE,
  1,
  {},
  {}
};

erizo::RtpMap rtpOpus{
  120,
  "opus",
  48000,
  erizo::MediaType::AUDIO_TYPE,
  2,
  {"transport-cc"},
  {}
};

} //namespace wa

