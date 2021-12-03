#ifndef __WEBRTC_AGENT_PC_H__
#define __WEBRTC_AGENT_PC_H__

#include <memory>

#include "erizo/WebRtcConnection.h"
#include "erizo/MediaStream.h"
#include "owt/owt_base/AudioFramePacketizer.h"
#include "owt/owt_base/AudioFrameConstructor.h"
#include "owt/owt_base/VideoFramePacketizer.h"
#include "owt/owt_base/VideoFrameConstructor.h"

#include "utils/IOWorker.h"
#include "utils/Worker.h"
#include "erizo/MediaStream.h"
#include "srs_kernel_error.h"

#include "h/rtc_stack_api.h"
#include "erizo/WebRtcConnection.h"
#include "owt/owt_base/MediaFramePipeline.h"
#include "owt/rtc_adapter/RtcAdapter.h"

namespace wa {

class WebrtcAgentSink;
class WebrtcAgent;
struct media_setting;
class MediaDesc;
class WaSdpInfo;

class WrtcAgentPc final : public erizo::WebRtcConnectionEventListener,
                          public owt_base::FrameDestination,
                          public owt_base::VideoInfoListener,
                          public std::enable_shared_from_this<WrtcAgentPc> {
 /*
 * WebrtcTrack represents a stream object
 * of WrtcAgentPc. It has media source
 * functions (addDestination) and media sink
 * functions (receiver) which will be used
 * in connection link-up. Each rtp-stream-id
 * in simulcast refers to one WebrtcTrack.
 */
  class WebrtcTrack {
  /*
   * audio: { format, ssrc, mid, midExtId }
   * video: { format, ssrc, mid, midExtId, transportcc, red, ulpfec }
   */
    
   public:
    enum ETrackCtrl {
      e_audio,
      e_video,
      e_av
    };
  
    WebrtcTrack(const std::string& mid,
                WrtcAgentPc*,
                bool isPublish,
                const media_setting&,
                erizo::MediaStream* mediaStream);
    ~WebrtcTrack();
    
    void close() {}
    void onMediaUpdate() {}
    
    void addDestination(bool isAudio, 
        std::shared_ptr<owt_base::FrameDestination> dest);
    void removeDestination(bool isAudio, owt_base::FrameDestination* dest);
    std::shared_ptr<owt_base::FrameDestination> receiver(bool isAudio);
    
    uint32_t ssrc(bool isAudio);
    
    int32_t format(bool isAudio) { return isAudio?audioFormat_:videoFormat_; }
    srs_error_t trackControl(ETrackCtrl, bool isIn, bool isOn);
    void requestKeyFrame();
    inline bool isAudio() {
      return name_ == "audio";
    }

    inline const std::string& getName() {
      return name_;
    }
  private:
    WrtcAgentPc* pc_{nullptr};
    std::string mid_;
  
    std::shared_ptr<owt_base::AudioFramePacketizer> audioFramePacketizer_;
    std::shared_ptr<owt_base::AudioFrameConstructor> audioFrameConstructor_;
    std::shared_ptr<owt_base::VideoFramePacketizer> videoFramePacketizer_;
    std::shared_ptr<owt_base::VideoFrameConstructor> videoFrameConstructor_;
  
    int32_t audioFormat_{0};
    int32_t videoFormat_{0};
    std::string name_;
  };

public:
  // Libnice collects candidates on |ipAddresses| only.
  WrtcAgentPc(const TOption&, WebrtcAgent&);
  ~WrtcAgentPc();

  int init(std::shared_ptr<Worker>& worker, 
           std::shared_ptr<IOWorker>& ioworker, 
           const std::vector<std::string>& ipAddresses,
           const std::string& stun_addr);

  void close();

  srs_error_t addTrackOperation(const std::string& mid, 
                                EMediaType type, 
                                const std::string& direction, 
                                const FormatPreference& prefer);

  void signalling(const std::string& signal, const std::string& content);

  void notifyEvent(erizo::WebRTCEvent newEvent, 
                   const std::string& message, 
                   const std::string &stream_id = "") override;

  void Subscribe(std::shared_ptr<WrtcAgentPc> subscriber);
  void unSubscribe(std::shared_ptr<WrtcAgentPc> subscriber);

  void setAudioSsrc(const std::string& mid, uint32_t ssrc);
  
  void setVideoSsrcList(const std::string& mid, 
                        std::vector<uint32_t> ssrc_list);

  //FrameDestination
  void onFrame(const owt_base::Frame&) override;

  const std::string& id() {
    return id_;
  }
  
 private:
  void init_i(const std::vector<std::string>& ipAddresses, 
              const std::string& stun_addr);
  void close_i();
  void subscribe_i(std::shared_ptr<WrtcAgentPc> subscriber, bool isSub);
  srs_error_t processOffer(const std::string& sdp);

  // call by WebrtcConnection
  void processSendAnswer(const std::string& streamId, 
                         const std::string& sdpMsg);

  srs_error_t addRemoteCandidate(const std::string& candidates);
   
  srs_error_t removeRemoteCandidates(const std::string& candidates);

  srs_error_t processOfferMedia(MediaDesc& media);

  srs_error_t setupTransport(MediaDesc& media);

  void onVideoInfo(const std::string& videoInfoJSON) override;

  WebrtcTrack* addTrack(const std::string& mid, 
                        const media_setting&, 
                        bool isPublish);

  srs_error_t removeTrack(const std::string& mid);

  WebrtcTrack* getTrack(const std::string& name);

  inline auto& getTracks() {
    return track_map_;
  }
  
  void asyncTask(std::function<void(std::shared_ptr<WrtcAgentPc>)> f) ;

  enum E_SINKID {
    E_CANDIDATE,
    E_FAILED,
    E_READY,
    E_ANSWER,
    E_DATA
  };

  void callBack(E_SINKID id, const std::string& message);
  void callBack(E_SINKID id, const owt_base::Frame& message);
private:
  TOption config_;
  std::string id_;
  WebrtcAgent& mgr_;

  std::shared_ptr<WebrtcAgentSink> sink_;

  std::shared_ptr<Worker> worker_;
  std::shared_ptr<IOWorker> ioworker_;
  
  WaSdpInfo* remote_sdp_{nullptr};
  WaSdpInfo* local_sdp_{nullptr};

  struct operation {
    std::string operation_id_;
    EMediaType type_{media_unknow};
    std::string sdp_direction_;
    FormatPreference format_preference_{p_unknow, ""};
    std::vector<uint32_t> rids_;
    bool enabled_{false};
    uint32_t final_format_{0};
  };

  /* mid => { 
   *  operationId, 
   *  type, 
   *  sdpDirection, 
   *  formatPreference, 
   *  rids, 
   *  enabled, 
   *  finalFormat }
   */
  std::map<std::string, operation> operation_map_;

  // composedId(mid) => WebrtcTrack
  std::map<std::string, std::unique_ptr<WebrtcTrack>> track_map_;

  // mid => msid
  std::map<std::string, std::string> msid_map_;
  
  std::shared_ptr<erizo::WebRtcConnection> connection_;

  erizo::WebRTCEvent connection_state_;

  bool ready_{false};
  
  std::unique_ptr<rtc_adapter::RtcAdapterFactory> adapter_factory_;
};

} //namespace wa

#endif //!__WEBRTC_AGENT_PC_H__

