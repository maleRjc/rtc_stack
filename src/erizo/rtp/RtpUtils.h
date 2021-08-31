// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.


#ifndef ERIZO_SRC_ERIZO_RTP_RTPUTILS_H_
#define ERIZO_SRC_ERIZO_RTP_RTPUTILS_H_

#include <stdint.h>
#include <memory>
#include <functional>

#include "erizo/rtp/RtpHeaders.h"
#include "erizo/MediaDefinitions.h"

namespace erizo {

class RtpUtils {
 public:
  static bool sequenceNumberLessThan(uint16_t first, uint16_t second);

  static bool numberLessThan(uint16_t first, uint16_t last, int bits);

  static void forEachRtcpBlock(std::shared_ptr<DataPacket> packet, std::function<void(RtcpHeader*)> f);

  static void updateREMB(RtcpHeader *chead, uint bitrate);

  static bool isPLI(std::shared_ptr<DataPacket> packet);

  static bool isFIR(std::shared_ptr<DataPacket> packet);

  static void forEachNack(RtcpHeader *chead, std::function<void(uint16_t, uint16_t, RtcpHeader*)> f);

  static std::shared_ptr<DataPacket> createPLI(uint32_t source_ssrc, uint32_t sink_ssrc);

  static std::shared_ptr<DataPacket> createFIR(uint32_t source_ssrc, uint32_t sink_ssrc, uint8_t seq_number);
  static std::shared_ptr<DataPacket> createREMB(uint32_t ssrc, std::vector<uint32_t> ssrc_list, uint32_t bitrate);

  static int getPaddingLength(std::shared_ptr<DataPacket> packet);

  static std::shared_ptr<DataPacket> makePaddingPacket(std::shared_ptr<DataPacket> packet, uint8_t padding_size);
};

}  // namespace erizo

#endif  // ERIZO_SRC_ERIZO_RTP_RTPUTILS_H_
