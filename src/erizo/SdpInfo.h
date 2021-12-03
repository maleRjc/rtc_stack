// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

#ifndef ERIZO_SRC_ERIZO_SDPINFO_H_
#define ERIZO_SRC_ERIZO_SDPINFO_H_

#include <stdint.h>

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "erizo/logger.h"

namespace erizo {
/**
 * ICE candidate types
 */
enum HostType {
    HOST, SRFLX, PRFLX, RELAY
};
/**
 * Channel types
 */
enum MediaType {
    VIDEO_TYPE, AUDIO_TYPE, OTHER
};
/**
 * Stream directions
 */
enum StreamDirection {
  SENDRECV, SENDONLY, RECVONLY
};
/**
 * Simulcast rid direction
 */
enum RidDirection {
  SEND, RECV
};
std::ostream& operator<<(std::ostream&, RidDirection);
RidDirection reverse(RidDirection);
/**
 * RTP Profile
 */
enum Profile {
  AVPF, SAVPF
};
enum DtlsRole {
  ACTPASS, ACTIVE, PASSIVE
};
/**
 * SRTP info.
 */
struct CryptoInfo {
  /**
   * tag number
   */
  int tag{0};
  /**
   * The cipher suite. Only AES_CM_128_HMAC_SHA1_80 is supported as of now.
   */
  std::string cipherSuite;
  /**
   * The key
   */
  std::string keyParams;
  /**
   * The MediaType
   */
  MediaType mediaType;
};
/**
 * Contains the information of an ICE Candidate
 */
struct CandidateInfo {
  bool isBundle;
  int tag{0};
  unsigned int priority;
  unsigned int componentId;
  std::string foundation;
  std::string hostAddress;
  std::string rAddress;
  int hostPort;
  int rPort;
  std::string netProtocol;
  HostType hostType;
  std::string transProtocol;
  std::string username;
  std::string password;
  MediaType mediaType;
  std::string sdp;
  int generation{-1};
};

/**
 * A bundle tag
 */
class BundleTag {
 public:
    BundleTag(std::string theId, MediaType theType) : id(theId), mediaType(theType) {
    }
    std::string id;
    MediaType mediaType;
};

/**
 * A PT to Codec map
 */
struct RtpMap {
  unsigned int payload_type;
  std::string encoding_name;
  unsigned int clock_rate;
  MediaType media_type;
  unsigned int channels;
  std::vector<std::string> feedback_types;
  std::map<std::string, std::string> format_parameters;
};
/**
 * A RTP extmap description
 */
class ExtMap {
 public:
    ExtMap(unsigned int theValue, std::string theUri)
      : value(theValue), uri(theUri) {
    }
    unsigned int value;
    std::string uri;
    StreamDirection direction;
    std::string parameters;
    MediaType mediaType;
};

/**
 * Simulcast rid structure
 */
struct Rid {
  std::string id;
  RidDirection direction;
};

bool operator==(const Rid&, const Rid&);

/**
 * Contains the information of a single SDP.
 * Used to parse and generate SDPs
 */
class SdpInfo {
  DECLARE_LOGGER();

 public:
  /**
   * Constructor
   */
  explicit SdpInfo(const std::vector<RtpMap>& rtp_mappings);
  ~SdpInfo() = default;
  /**
   * Inits the object with a given SDP.
   * @param sdp An string with the SDP.
   * @return true if success
   */
  bool initWithSdp(const std::string& sdp, const std::string& media);
  /**
   * Adds a new candidate.
   * @param info The CandidateInfo containing the new candidate
   */
  std::string addCandidate(const CandidateInfo& info);
  /**
   * Adds SRTP info.
   * @param info The CryptoInfo containing the information.
   */
  void addCrypto(const CryptoInfo& info);
  /**
   * Gets the candidates.
   * @return A vector containing the current candidates.
   */
  std::vector<CandidateInfo>& getCandidateInfos();
  /**
   * Gets the SRTP information.
   * @return A vector containing the CryptoInfo objects with the SRTP information.
   */
  std::vector<CryptoInfo>& getCryptoInfos();
  /**
  * Gets the payloadType information
  * @return A vector containing the PT-codec information
  */
  std::vector<RtpMap>& getPayloadInfos();
  std::vector<ExtMap> getExtensionMap(MediaType media);
  /**
   * Gets the actual SDP.
   * @return The SDP in string format.
   */
  std::string getSdp();
  /**
   * @brief map external payload type to an internal id
   * @param externalPT The audio payload type as coming from this source
   * @return The internal payload id
   */
  unsigned int getAudioInternalPT(unsigned int externalPT);
  /**
   * @brief map external payload type to an internal id
   * @param externalPT The video payload type as coming from this source
   * @return The internal payload id
   */
  unsigned int getVideoInternalPT(unsigned int externalPT);
  /**
   * @brief map internal payload id to an external payload type
   * @param internalPT The payload type id used internally
   * @return The external payload type as provided to this source
   */
  unsigned int getAudioExternalPT(unsigned int internalPT);
  /**
   * @brief map internal payload it to an external payload type
   * @param internalPT The payload id as used internally
   * @return The external video payload type
   */
  unsigned int getVideoExternalPT(unsigned int internalPT);

  RtpMap* getCodecByExternalPayloadType(const unsigned int payload_type);

  void setCredentials(const std::string& username, const std::string& password, MediaType media);

  std::string getUsername(MediaType media) const;

  std::string getPassword(MediaType media) const;

  RtpMap* getCodecByName(const std::string codecName, const unsigned int clockRate);

  bool supportCodecByName(const std::string codecName, const unsigned int clockRate);

  bool supportPayloadType(const unsigned int payloadType);

  void createOfferSdp(bool videoEnabled, bool audioEnabled, bool bundle);
  /**
   * @brief copies relevant information from the offer sdp for which this will be an answer sdp
   * @param offerSdp The offer SDP as received via signaling and parsed
   */
  void setOfferSdp(std::shared_ptr<SdpInfo> offerSdp);

  void updateSupportedExtensionMap(const std::vector<ExtMap> &ext_map);
  bool isValidExtension(const std::string& uri);

  /**
  * @return A vector containing the simulcast RID informations
  */
  const std::vector<Rid>& rids() const { return rids_; }

  /**
   * The audio and video SSRCs for this particular SDP.
   */
  std::map<std::string, unsigned int> audio_ssrc_map;
  std::map<std::string, std::vector<uint32_t>> video_ssrc_map;
  /**
  * Is it Bundle
  */
  bool isBundle;
  /**
  * Has audio
  */
  bool hasAudio;
  /**
  * Has video
  */
  bool hasVideo;
  /**
  * Is there rtcp muxing
  */
  bool isRtcpMux;

  StreamDirection videoDirection, audioDirection;
  /**
  * RTP Profile type
  */
  Profile profile;
  /**
  * Is there DTLS fingerprint
  */
  bool isFingerprint;
  /**
  * DTLS Fingerprint
  */
  std::string fingerprint;
  /**
  * DTLS Role
  */
  DtlsRole dtlsRole;
  /**
  * Mapping from internal PT (key) to external PT (value)
  */
  std::map<unsigned int, unsigned int> inOutPTMap;
  /**
  * Mapping from external PT (key) to intermal PT (value)
  */
  std::map<unsigned int, unsigned int> outInPTMap;
  /**
   * The negotiated payload list
   */
  std::vector<RtpMap> payloadVector;

  std::vector<BundleTag> bundleTags;
  std::vector<ExtMap> extMapVector;

  /*
   * MLines for video and audio
   */
  int videoSdpMLine;
  int audioSdpMLine;
  int videoCodecs, audioCodecs;
  unsigned int videoBandwidth;
  std::vector<CandidateInfo> candidateVector_;
  std::vector<CryptoInfo> cryptoVector_;
  std::vector<RtpMap> internalPayloadVector_;
  std::string iceVideoUsername_, iceAudioUsername_;
  std::string iceVideoPassword_, iceAudioPassword_;
  std::map<unsigned int, RtpMap> payload_parsed_map_;
  std::vector<ExtMap> supported_ext_map_;
  std::vector<Rid> rids_;
  std::string google_conference_flag_set;
  std::map<unsigned int, int> parsed_order_map;

 private:
  bool processSdp(const std::string& sdp, const std::string& media);
  bool postProcessInfo();
  bool processCandidate(const std::vector<std::string>& pieces, MediaType mediaType, std::string sdp);
  std::string stringifyCandidate(const CandidateInfo & candidate);
  void gen_random(char* s, int len);
  void maybeAddSsrcToList(uint32_t ssrc);
};
}  // namespace erizo
#endif  // ERIZO_SRC_ERIZO_SDPINFO_H_
