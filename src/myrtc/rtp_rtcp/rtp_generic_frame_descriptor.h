/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef MODULES_RTP_RTCP_SOURCE_RTP_GENERIC_FRAME_DESCRIPTOR_H_
#define MODULES_RTP_RTCP_SOURCE_RTP_GENERIC_FRAME_DESCRIPTOR_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "optional"
#include "rtc_base/array_view.h"

namespace webrtc {

class RtpGenericFrameDescriptorExtension;

// Data to put on the wire for FrameDescriptor rtp header extension.
class RtpGenericFrameDescriptor {
 public:
  static constexpr int kMaxNumFrameDependencies = 8;
  static constexpr int kMaxTemporalLayers = 8;
  static constexpr int kMaxSpatialLayers = 8;

  RtpGenericFrameDescriptor();
  RtpGenericFrameDescriptor(const RtpGenericFrameDescriptor&);
  ~RtpGenericFrameDescriptor();

  bool FirstPacketInSubFrame() const { return beginning_of_subframe_; }
  void SetFirstPacketInSubFrame(bool first) { beginning_of_subframe_ = first; }
  bool LastPacketInSubFrame() const { return end_of_subframe_; }
  void SetLastPacketInSubFrame(bool last) { end_of_subframe_ = last; }

  // Denotes whether the frame is discardable. That is, whether skipping it
  // would have no effect on the decodability of subsequent frames.
  // An std::optional is used because version 0 of the extension did not
  // support this flag. (The optional aspect is relevant only when parsing.)
  // TODO(bugs.webrtc.org/10243): Make this into a plain bool when v00 of
  // the extension is deprecated.
  std::optional<bool> Discardable() const { return discardable_; }
  void SetDiscardable(bool discardable) { discardable_ = discardable; }

  // Properties below undefined if !FirstPacketInSubFrame()
  // Valid range for temporal layer: [0, 7]
  int TemporalLayer() const;
  void SetTemporalLayer(int temporal_layer);

  // Frame might by used, possible indirectly, for spatial layer sid iff
  // (bitmask & (1 << sid)) != 0
  int SpatialLayer() const;
  uint8_t SpatialLayersBitmask() const;
  void SetSpatialLayersBitmask(uint8_t spatial_layers);

  int Width() const { return width_; }
  int Height() const { return height_; }
  void SetResolution(int width, int height);

  uint16_t FrameId() const;
  void SetFrameId(uint16_t frame_id);

  rtc::ArrayView<const uint16_t> FrameDependenciesDiffs() const;
  void ClearFrameDependencies() { num_frame_deps_ = 0; }
  // Returns false on failure, i.e. number of dependencies is too large.
  bool AddFrameDependencyDiff(uint16_t fdiff);

  void SetByteRepresentation(rtc::ArrayView<const uint8_t> representation);
  rtc::ArrayView<const uint8_t> GetByteRepresentation();

 private:
  bool beginning_of_subframe_ = false;
  bool end_of_subframe_ = false;

  std::optional<bool> discardable_;

  uint16_t frame_id_ = 0;
  uint8_t spatial_layers_ = 1;
  uint8_t temporal_layer_ = 0;
  size_t num_frame_deps_ = 0;
  uint16_t frame_deps_id_diffs_[kMaxNumFrameDependencies];
  int width_ = 0;
  int height_ = 0;

  std::vector<uint8_t> byte_representation_;
};

}  // namespace webrtc

#endif  // MODULES_RTP_RTCP_SOURCE_RTP_GENERIC_FRAME_DESCRIPTOR_H_
