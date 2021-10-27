#ifndef __WA_MEDIA_CONFIG_H__
#define __WA_MEDIA_CONFIG_H__

// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include "erizo/SdpInfo.h"

namespace wa {

enum EExtmap {
  AudioLevel,
  TransportCC,
  SdesMid,
/*  SdesRtpStreamId,
  SdesRepairedRtpStreamId,
  Toffset,
*/  
  AbsSendTime,
/*
  videoOrientation,
  PlayoutDelay
*/
};

#define EXT_MAP_SIZE  4

extern const std::string extMappings[EXT_MAP_SIZE];

extern erizo::RtpMap rtpH264;

extern erizo::RtpMap rtpRed;

extern erizo::RtpMap rtpRtx;

extern erizo::RtpMap rtpUlpfec;

extern erizo::RtpMap rtpOpus;

} //namespace wa

#endif //!__WA_MEDIA_CONFIG_H__

