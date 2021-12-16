//
// Copyright (c) 2021- anjisuan783
//
// SPDX-License-Identifier: MIT
//

#ifndef __WA_SDP_PROCESSOR_H__
#define __WA_SDP_PROCESSOR_H__

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <random>

#include "libsdptransform/include/json.hpp"

using JSON_TYPE = nlohmann::json;

namespace wa {

struct FormatPreference;

struct media_setting {
  bool is_audio{false};
  int32_t format;
  std::vector<uint32_t> ssrcs;
  std::string mid;
  int32_t mid_ext{0};   //urn:ietf:params:rtp-hdrext:sdes:mid 
  bool rtcp_rsize{false};
  int red{-1};
  int ulpfec{-1};
  bool flexfec{false};
  int transportcc{-1};
};

struct SessionInfo {
  int decode(const JSON_TYPE& session);
  void encode(JSON_TYPE& session);
  std::string ice_ufrag_;
  std::string ice_pwd_;
  std::string ice_options_;
  std::string fingerprint_algo_;
  std::string fingerprint_;
  std::string setup_;
};

class MediaDesc {
 public:
  //UDP RFC 8445
  //TCP RFC 6544
  struct Candidate {
    void encode(JSON_TYPE& session);
    void decode(const JSON_TYPE& session);

    std::string foundation_;
    int32_t component_{0};
    std::string transport_type_;
    int32_t priority_{0};
    std::string ip_;
    int32_t port_{0};
    std::string type_;
  };

  struct rtpmap {
    rtpmap() = default;
    rtpmap(const rtpmap&) = default;
    rtpmap(rtpmap&&);
    rtpmap& operator=(const rtpmap&) = default;
  
    void encode_rtp(JSON_TYPE& session);
    void encode_fb(JSON_TYPE& session);
    void encode_fmtp(JSON_TYPE& session);
    std::string decode(const JSON_TYPE& session);
    
    int32_t payload_type_;
    std::string encoding_name_;
    int32_t clock_rate_;
    std::string encoding_param_;
    std::vector<std::string> rtcp_fb_;  //rtcp-fb
    std::string fmtp_;

    std::vector<rtpmap> related_;
  };

  struct SSRCGroup {
    void encode(JSON_TYPE& session);
    void decode(const JSON_TYPE& session);
    // e.g FIX, FEC, SIM.
    std::string semantic_;
    // SSRCs of this type. 
    std::vector<uint32_t> ssrcs_;
  };

  struct SSRCInfo {
    void encode(JSON_TYPE& session);
    uint32_t ssrc_;
    std::string cname_;
    std::string msid_;
    std::string msid_tracker_;
    std::string mslabel_;
    std::string label_;
  };

  struct RidInfo {
    std::string id_;
    std::string direction_;
    std::string params_;
  };

 public:
  //parse m line
  void parse(const JSON_TYPE& session);
  
  void encode(JSON_TYPE&);
  
  bool is_audio() const { return type_ == "audio"; }
  
  bool is_video() const { return type_ == "video"; }
  
  media_setting get_media_settings();
  
  int32_t filterAudioPayload(const FormatPreference& option);
  
  int32_t filterVideoPayload(const FormatPreference& option);
  
  std::string setSsrcs(const std::vector<uint32_t>& ssrcs, 
                       const std::string& inmsid);

  bool filterByPayload(int32_t payload, bool, bool, bool);
  
  void filterExtmap();
 private:
  void parse_candidates(const JSON_TYPE& media);

  void parse_rtcp_fb(const JSON_TYPE& media);
  
  void parse_fmtp(const JSON_TYPE& media);

  void parse_ssrc_info(const JSON_TYPE& media);

  SSRCInfo& fetch_or_create_ssrc_info(uint32_t ssrc);
  
  rtpmap* find_rtpmap_with_payload_type(int payload_type);

  void parse_ssrc_group(const JSON_TYPE& media);

 public:
  std::string type_;
  
  int32_t port_{0};
  
  int32_t numPorts_{0};

  std::string protocols_;

  std::string payloads_;

  std::vector<Candidate> candidates_;
  
  SessionInfo session_info_;

  std::string mid_;

  std::map<int, std::string> extmaps_;

  std::string direction_;  // "recvonly" "sendonly" "sendrecv"
  
  std::string msid_;

  std::string rtcp_mux_;

  std::vector<rtpmap> rtp_maps_;

  std::vector<SSRCInfo>  ssrc_infos_;
  
  std::string rtcp_rsize_;  // for video

  std::vector<SSRCGroup> ssrc_groups_; 

  //rids for simulcast not support
  std::vector<RidInfo> rids_;

  bool disable_audio_gcc_{false};
};

class WaSdpInfo {
 public:
  WaSdpInfo();
  
  WaSdpInfo(const std::string& sdp);

  int init(const std::string& sdp);

  bool empty() { return media_descs_.empty(); }

  void media() {}

  void rids() {}
  
  std::string mediaType(const std::string& mid);

  std::string mediaDirection(const std::string& mid);

  int32_t filterAudioPayload(const std::string& mid, 
                             const FormatPreference& type);

  int32_t filterVideoPayload(const std::string& mid, 
                             const FormatPreference& type);

  bool filterByPayload(const std::string& mid, 
                       int32_t payload,
                       bool disable_red = false,
                       bool disable_rtx = false,
                       bool disable_ulpfec = false);

  void filterExtmap();

  int32_t getMediaPort(const std::string& mid);

  bool setMediaPort(const std::string& mid, int32_t port);
  
  std::string singleMediaSdp(const std::string& mid);

  void setCredentials(const WaSdpInfo&);
  
  void SetMsid(const std::string&);

  void setCandidates(const WaSdpInfo&);
  
  media_setting get_media_settings(const std::string& mid);

  void mergeMedia() {}
  
  void compareMedia() {}

  void getLegacySimulcast() {}

  void setSsrcs() {}

  WaSdpInfo* answer();
  
  std::string toString(const std::string& strMid = "");
 public:
  // version "v="
  int version_{0};

  // origin "o="
  std::string username_;
  uint64_t session_id_;
  uint32_t session_version_;
  std::string nettype_;
  int32_t addrtype_;
  std::string unicast_address_;

  // session_name  "s="
  std::string session_name_;

  // timing "t="
  int64_t start_time_;
  int64_t end_time_;

  bool session_in_medias_{false};
  SessionInfo session_info_;

  std::vector<std::string> groups_;
  std::string group_policy_;

  std::string msid_semantic_;
  std::vector<std::string> msids_;

  // m-line, media sessions  "m="
  std::vector<MediaDesc> media_descs_;

  bool ice_lite_{false};
  bool enable_extmapAllowMixed_{false};
  std::string extmapAllowMixed_;
};

} //namespace wa

#endif //!__WA_SDP_PROCESSOR_H__

