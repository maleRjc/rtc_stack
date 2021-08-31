/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_VIDEO_VIDEO_CODEC_TYPE_H_
#define API_VIDEO_VIDEO_CODEC_TYPE_H_

namespace webrtc {

// Video codec types
enum VideoCodecType {
  // There are various memset(..., 0, ...) calls in the code that rely on
  // kVideoCodecGeneric being zero.
  kVideoCodecGeneric = 0,
  kVideoCodecVP8,
  kVideoCodecVP9,
  kVideoCodecH264,
#ifdef OWT_ENABLE_H265
  kVideoCodecH265,
#endif
  kVideoCodecMultiplex,
};

}  // namespace webrtc

#endif  // API_VIDEO_VIDEO_CODEC_TYPE_H_