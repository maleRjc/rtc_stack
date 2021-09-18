#include "webrtc_agent_pc.h"

#include "media_config.h"
#include "h/rtc_return_value.h"
#include "sdp_processor.h"

namespace wa
{
DEFINE_LOGGER(WrtcAgentPc, "WrtcAgentPc");

/////////////////////////////
//WebrtcTrack
WrtcAgentPc::WebrtcTrack::WebrtcTrack(const std::string& mid, WrtcAgentPc* pc, 
                                      bool isPublish, const media_setting& setting,
                                      erizo::MediaStream* ms)
  : pc_(pc), mid_(mid) {
  if (isPublish) {
    if (setting.is_audio) {
      audioFormat_ = setting.format;
      audioFrameConstructor_.reset(new owt_base::AudioFrameConstructor());
      audioFrameConstructor_->bindTransport(dynamic_cast<erizo::MediaSource*>(ms),
                                           dynamic_cast<erizo::FeedbackSink*>(ms));
      WLOG_DEBUG("set a ssrc %u", setting.ssrcs[0]);
      pc_->setAudioSsrc(mid_, setting.ssrcs[0]);

    } else {
      videoFormat_ = setting.format;
      
      owt_base::VideoFrameConstructor::config config;
      config.ssrc = setting.ssrcs[0];
      config.rtx_ssrc = setting.ssrcs[1];
      config.rtcp_rsize = setting.rtcp_rsize;
      config.rtp_payload_type = setting.format;
      config.ulpfec_payload = setting.ulpfec?setting.ulpfec:-1;
      config.flex_fec = setting.flexfec;
      config.transportcc = setting.transportcc;
      config.red_payload = setting.red?setting.red:-1;
      
      videoFrameConstructor_.reset(
          new owt_base::VideoFrameConstructor(pc, config, pc->worker_->getTaskQueue()));
      videoFrameConstructor_->bindTransport(dynamic_cast<erizo::MediaSource*>(ms),
                                           dynamic_cast<erizo::FeedbackSink*>(ms));
      pc_->setVideoSsrcList(mid_, setting.ssrcs);
    }
    
  } else {
    if (setting.is_audio) {
      owt_base::AudioFramePacketizer::Config config;
      config.mid = setting.mid;
      config.midExtId = setting.mid_ext;
      audioFramePacketizer_.reset(
          new owt_base::AudioFramePacketizer(config, pc->worker_->getTaskQueue()));
      audioFramePacketizer_->bindTransport(dynamic_cast<erizo::MediaSink*>(ms));
      audioFormat_ = setting.format;
      
    } else {
      owt_base::VideoFramePacketizer::Config config;
      config.Red = setting.red, 
      config.Ulpfec = setting.ulpfec, 
      config.transportccExt = setting.transportcc?true:false,
      config.selfRequestKeyframe = true,
      config.mid = setting.mid,
      config.midExtId = setting.mid_ext;
      videoFramePacketizer_.reset(
          new owt_base::VideoFramePacketizer(config, pc->worker_->getTaskQueue()));
      videoFramePacketizer_->bindTransport(dynamic_cast<erizo::MediaSink*>(ms));
      videoFormat_ = setting.format;
    }
  }
}

uint32_t WrtcAgentPc::WebrtcTrack::ssrc(bool isAudio) {
  if (isAudio && audioFramePacketizer_) {
    return audioFramePacketizer_->getSsrc();
  } else if (!isAudio && videoFramePacketizer_) {
    return videoFramePacketizer_->getSsrc();
  }
  return 0;
}

void WrtcAgentPc::WebrtcTrack::addDestination(bool isAudio, owt_base::FrameDestination* dest) {
  OLOG_TRACE_THIS((isAudio?"a":"v") << ", dest:" << dest);
  if (isAudio && audioFrameConstructor_) {
    audioFrameConstructor_->addAudioDestination(dest);
  } else if (!isAudio && videoFrameConstructor_) {
    videoFrameConstructor_->addVideoDestination(dest);
  }
}

void WrtcAgentPc::WebrtcTrack::removeDestination(bool isAudio, owt_base::FrameDestination* dest) {
  OLOG_TRACE_THIS((isAudio?"a":"v") << ", dest:" << dest);
  if (isAudio && audioFrameConstructor_) {
    audioFrameConstructor_->removeAudioDestination(dest);
  } else if (!isAudio && videoFrameConstructor_) {
    videoFrameConstructor_->removeVideoDestination(dest);
  } 
}

owt_base::FrameDestination* WrtcAgentPc::WebrtcTrack::receiver(bool isAudio) 
{
  owt_base::FrameDestination* dest = nullptr;
  if (isAudio) {
    dest = audioFramePacketizer_.get();
  } else {
    dest = videoFramePacketizer_.get();
  }
  return dest;
}

srs_error_t WrtcAgentPc::WebrtcTrack::trackControl(ETrackCtrl track, bool isIn, bool isOn) {
  bool trackUpdate = false;
  if (track == e_av || track == e_audio) {
    if (isIn && audioFrameConstructor_) {
      audioFrameConstructor_->enable(isOn);
      trackUpdate = true;
    }
    if (!isIn && audioFramePacketizer_) {
      audioFramePacketizer_->enable(isOn);
      trackUpdate = true;
    }
  }
  
  if (track == e_av || track == e_video) {
    if (isIn && videoFrameConstructor_) {
      videoFrameConstructor_->enable(isOn);
      trackUpdate = true;
    }
    if (!isIn && videoFramePacketizer_) {
      videoFramePacketizer_->enable(isOn);
      trackUpdate = true;
    }
  }
  srs_error_t result = srs_success;
  
  if (!trackUpdate) {
    result = srs_error_wrap(result, "No track found");
  }

  return result;
}

void WrtcAgentPc::WebrtcTrack::requestKeyFrame() { 
  if (videoFrameConstructor_) {
    videoFrameConstructor_->RequestKeyFrame();
  }
}

/////////////////////////////
//WrtcAgentPc

WrtcAgentPc::WrtcAgentPc(const TOption& config, WebrtcAgent& mgr)
  : config_(config), 
    id_(config.connectId_), 
    mgr_(mgr),
    sink_(std::move(config_.call_back_)) {
  OLOG_TRACE_THIS("");
}

WrtcAgentPc::~WrtcAgentPc() {
  this->close();
  if(remote_sdp_)
    delete remote_sdp_;
  if(local_sdp_)
    delete local_sdp_;

  OLOG_TRACE_THIS("");
}

int WrtcAgentPc::init(std::shared_ptr<Worker>& worker, 
                      std::shared_ptr<IOWorker>& ioworker, 
                      const std::vector<std::string>& ipAddresses,
                      const std::string& stun_addr) {
  worker_ = worker;
  ioworker_ = ioworker;

  asyncTask([ipAddresses, stun_addr](std::shared_ptr<WrtcAgentPc> pc){
    pc->init_i(ipAddresses, stun_addr);
  });
  return wa_ok;
}

void WrtcAgentPc::init_i(const std::vector<std::string>& ipAddresses, 
                         const std::string& stun_addr) {
  erizo::IceConfig ice_config;
  ice_config.ip_addresses = ipAddresses;

  size_t pos1 = stun_addr.find("://");
  size_t pos2 = stun_addr.rfind(":");
  assert(pos1 != stun_addr.npos && pos2 != stun_addr.npos);

  pos1 += 3;
  ice_config.stun_server = stun_addr.substr(pos1, pos2-pos1);

  std::istringstream iss(stun_addr.substr(pos2+1));
  iss >> ice_config.stun_port;
  
  std::vector<erizo::RtpMap> rtp_mappings{rtpH264, rtpRed, rtpRtx, rtpUlpfec, rtpOpus};
  
  std::vector<erizo::ExtMap> ext_mappings;
  for(unsigned int i = 0; i < EXT_MAP_SIZE; ++i){
    ext_mappings.push_back({i, extMappings[i]});
  }
  
  connection_ = std::make_shared<erizo::WebRtcConnection>(
    worker_.get(), ioworker_.get(), id_, ice_config, rtp_mappings, ext_mappings, this);
  connection_->init();
}

void WrtcAgentPc::close() {
  if(connection_)
    connection_->close();

  sink_ = nullptr;
}

srs_error_t WrtcAgentPc::addTrackOperation(const std::string& mid, 
                                           EMediaType type, 
                                           const std::string& direction, 
                                           const FormatPreference& prefer) {
  srs_error_t ret = srs_success;
  auto found = operation_map_.find(mid);
  if(found != operation_map_.end()){
    return srs_error_new(
      wa_e_found, "%s has mapped operation %s", mid.c_str(), found->second.operation_id_.c_str());
  }

  operation op;
  op.type_ = type;
  op.sdp_direction_ = direction;
  op.format_preference_ = prefer;
  op.enabled_ = true;
  
  operation_map_.insert(std::make_pair(mid, op));
  return ret;
}

void WrtcAgentPc::signalling(const std::string& signal, const std::string& content) {
  std::weak_ptr<WrtcAgentPc> weak_ptr = weak_from_this();
  asyncTask([weak_ptr, signal, content](std::shared_ptr<WrtcAgentPc>){
    auto this_ptr = weak_ptr.lock();
    if (!this_ptr){
      return;
    }
    srs_error_t result = srs_success;  
    if (signal == "offer") {
      result = this_ptr->processOffer(content);
    } else if (signal == "candidate") {
      result = this_ptr->addRemoteCandidate(content);
    } else if (signal == "removed-candidates") {
      result = this_ptr->removeRemoteCandidates(content);
    }
    if (result != srs_success) {
      WLOG_ERROR("process %s error, code:%d, desc:%s", 
          signal.c_str(), srs_error_code(result), srs_error_desc(result).c_str());
      delete result;
    }
  });
}

void WrtcAgentPc::notifyEvent(erizo::WebRTCEvent newStatus, 
                              const std::string& message, 
                              const std::string& stream_id) {
  WLOG_INFO("message: WebRtcConnection status update, id:%s, status:%d", id_.c_str(), newStatus);
  connection_state_ = newStatus;
  switch(newStatus) {
    case erizo::CONN_GATHERED:
      processSendAnswer(stream_id, message);
      break;

    case erizo::CONN_CANDIDATE:
      //std::string mess = mess.replace(this.options.privateRegexp, this.options.publicIP);
      callBack(E_CANDIDATE, message);
      WLOG_INFO("message: candidate, id::%s, c:%s", id_.c_str(), message.c_str());
      break;

    case erizo::CONN_FAILED:
      WLOG_INFO("message: failed the ICE process, code:%s", id_.c_str());
      callBack(E_FAILED, message);
      break;

    case erizo::CONN_READY:
      WLOG_INFO("message: connection ready, id:%s", id_.c_str());
      if (!ready_) {
        ready_ = true;
        callBack(E_READY, "");
      }
      break;
    case erizo::CONN_INITIAL:
    case erizo::CONN_STARTED:
    case erizo::CONN_SDP_PROCESSED:
    case erizo::CONN_FINISHED:
      break;
  }
}

void WrtcAgentPc::processSendAnswer(const std::string& streamId, const std::string& sdpMsg) {
  WLOG_INFO("message: processSendAnswer streamId:%s", streamId.c_str());
  LOG_ASSERT(sdpMsg.length());
  
  if(!sdpMsg.empty()) {
    // First answer from native
    try{
      WaSdpInfo tempSdp(sdpMsg);

      if (!tempSdp.mids().empty()) {
        local_sdp_->session_name_ = "wa/0.1(ly)";
        local_sdp_->setMsidSemantic(tempSdp);
        local_sdp_->setCredentials(tempSdp);
        local_sdp_->setCandidates(tempSdp); 
        local_sdp_->ice_lite_ = true;
      } else {
        WLOG_ERROR("No mid in answer pcID:%s, streamId:%s, sdp:%s", 
                   id_.c_str(), streamId.c_str(), sdpMsg.c_str());
      }
    }
    catch(std::exception& ex){
      std::cout << "exception catched :" << ex.what() << std::endl;
      return;
    }
  }
  std::string answerSdp = local_sdp_->toString();

  //WLOG_DEBUG("message: processSendAnswer streamId:%s, internalsdp:%s, answersdp:%s", 
  //           streamId.c_str(), sdpMsg.c_str(), answerSdp.c_str());
  callBack(E_ANSWER, answerSdp);
}

using namespace erizo;

srs_error_t WrtcAgentPc::processOffer(const std::string& sdp) {
  srs_error_t result = srs_success;
  if (!remote_sdp_) {
    // First offer
    try{
      remote_sdp_ = new WaSdpInfo(sdp);
    }
    catch(std::exception& ex){
      delete remote_sdp_;
      remote_sdp_ = nullptr;
      return srs_error_new(wa::wa_e_parse_offer_failed, "parse remote sdp failed");
    }
 
    // Check mid
    for (auto& mid : remote_sdp_->media_descs_) {

      for (auto& i : config_.tracks_) {
        if (i.type_ == media_audio && mid.type_ == "audio") {
          addTrackOperation(mid.mid_, media_audio, i.direction_, i.preference_);
          break;
        } else if (i.type_ == media_video && mid.type_ == "video") {
          addTrackOperation(mid.mid_, media_video, i.direction_, i.preference_);
          break;
        }
      }
    
      if((result = processOfferMedia(mid)) != srs_success){
        return srs_error_wrap(result, "processOfferMedia failed");
      }
    }

    try{
      local_sdp_ = remote_sdp_->answer();
    }
    catch(std::exception& ex){
      delete local_sdp_;
      local_sdp_ = nullptr;
      return srs_error_new(wa::wa_e_parse_offer_failed, "parse locad sdp from remote failed");
    }
    
    for(auto& x : operation_map_){
      local_sdp_->filterByPayload(x.first, x.second.final_format_);
    }

    local_sdp_->filterExtmap();
    
    // Setup transport
    //let opId = null;
    for (auto& mid : remote_sdp_->media_descs_) {
      if (mid.port_ != 0) {
        if((result = setupTransport(mid)) != srs_success){
          return srs_error_wrap(result, "setupTransport failed");
        }
      }
    }

  } else {
    // TODO: Later offer not implement
  }
  
  return result;
}

srs_error_t WrtcAgentPc::addRemoteCandidate(const std::string& candidates) {
  srs_error_t result = srs_success;
  return result;
}

srs_error_t WrtcAgentPc::removeRemoteCandidates(const std::string& candidates) {
  srs_error_t result = srs_success;
  return result;
}

srs_error_t WrtcAgentPc::processOfferMedia(MediaDesc& media) {
  OLOG_TRACE_THIS("t:" << media.type_ << ", mid:" << media.mid_);
  // Check Media MID with saved operation
  auto found = operation_map_.find(media.mid_);
  if (found == operation_map_.end()) {
    media.port_ = 0;
    return srs_error_new(wa_e_found, "%s has mapped operation %s", 
                         media.mid_.c_str(), found->second.operation_id_.c_str());
  }

  const std::string& mid = media.mid_;
  operation& op = found->second;
  
  if (op.sdp_direction_ != media.direction_) {
    return srs_error_new(wa_failed, 
        "mid[%s] in offer has conflict direction with opid[%s], opd[%s] != md[%s]", 
        mid.c_str(), op.operation_id_.c_str(), op.sdp_direction_.c_str(), media.direction_.c_str());
  }

  std::string media_type{"unknow"};
  if(op.type_ == EMediaType::media_audio){
    media_type = "audio";
  }
  else if(op.type_ == EMediaType::media_video){
   media_type = "video";
  }
  
  if (media_type != media.type_) {
    return srs_error_new(wa_failed, "%s in offer has conflict media type with %s", 
                          mid.c_str(), op.operation_id_.c_str());
  }
  
  if (op.enabled_ && (media.port_ == 0)) {
    WLOG_WARNING("%s in offer has conflict port with operation %s disabled", 
                 mid.c_str(), op.operation_id_.c_str());
    op.enabled_ = false;
  }

  // Determine media format in offer
  if ("audio" == media.type_) {
    op.final_format_ = 
      remote_sdp_->filterAudioPayload(mid, op.format_preference_);
  }
  else if(remote_sdp_->mediaType(mid) == "video") {
    op.final_format_ = 
      remote_sdp_->filterVideoPayload(mid, op.format_preference_);
  }

  return srs_success;
}

srs_error_t WrtcAgentPc::setupTransport(MediaDesc& media) {
  OLOG_TRACE_THIS("t:" << media.type_ << ", mid:" << media.mid_);
  srs_error_t result = srs_success;
  
  auto op_found = operation_map_.find(media.mid_);
  operation& opSettings = op_found->second;

  auto& rids = media.rids_;
  std::string direction = (opSettings.sdp_direction_ == "sendonly") ? "in" : "out";
  //const simSsrcs = remoteSdp.getLegacySimulcast(mid);
  media_setting trackSetting = media.get_media_settings();
  
  if (opSettings.final_format_) {
    trackSetting.format = opSettings.final_format_;
  }

  if(rids.empty()) {
    // No simulcast    
    auto track_found = track_map_.find(media.mid_);
    if(track_found == track_map_.end()){
      // Set ssrc in local sdp for out direction
      if (direction == "in") {

        WebrtcTrack* ret = addTrack(media.mid_, trackSetting, (direction=="in"?true:false));
     
        uint32_t ssrc = ret->ssrc(trackSetting.is_audio);
        if(ssrc){
          ELOG_INFO("Add ssrc %u to %s in SDP for %s", ssrc, media.mid_.c_str(), id_.c_str());
          
          const std::string& opId = opSettings.operation_id_;
          auto msid_found = msid_map_.find(opId);
          if (msid_found != msid_map_.end()){
            media.setSsrcs(std::vector<uint32_t>(ssrc), msid_found->second);
          }else{
            std::string msid = media.setSsrcs(std::vector<uint32_t>(ssrc), "");
            msid_map_.insert(std::make_pair(opId, msid));
          }
        }

        ret->addDestination(trackSetting.is_audio, this);
      }
      connection_->setRemoteSdp(remote_sdp_->singleMediaSdp(media.mid_), media.mid_);
    } else {
      result = srs_error_new(wa_e_found, "Conflict trackId %s for %s", 
                             media.mid_.c_str(), id_.c_str());
    }
  }else {
#if 0
    // Simulcast
    rids.forEach((rid, index) => {
      const trackId = composeId(mid, rid);        
      if (simSsrcs) {
        // Assign ssrcs for legacy simulcast
        trackSettings[mediaType].ssrc = simSsrcs[index];
      }

      if (!trackMap.has(trackId)) {
        trackMap.set(trackId, new WrtcStream(trackId, wrtc, direction, trackSettings));
        wrtc.setRemoteSdp(remoteSdp.singleMediaSdp(mid).toString(), trackId);
        // Notify new track
        on_track({
          type: 'track-added',
          track: trackMap.get(trackId),
          operationId: opSettings.operationId,
          mid: mid,
          rid: rid
        });
      } else {
        log.warn(`Conflict trackId ${trackId} for ${wrtcId}`);
      }
    });
#endif
  }

  return result;
}

WrtcAgentPc::WebrtcTrack* WrtcAgentPc::addTrack(
    const std::string& mid, const media_setting& trackSetting, bool isPublish) {
  ELOG_TRACE("message: addTrack %s, connectionId:%s mediaStreamId:%s", 
      (isPublish?"p": "s"), id_.c_str(), mid.c_str());

  WebrtcTrack* result = nullptr;

  auto found = track_map_.find(mid);
  if (track_map_.end() != found) {
    result = found->second.get();
  } else {
    auto ms = std::make_shared<MediaStream>(
        worker_.get(), connection_, mid, mid, isPublish);
    
    connection_->addMediaStream(ms);

    std::unique_ptr<WebrtcTrack> newTrack(
          new WebrtcTrack(mid, this, isPublish, trackSetting, ms.get()));

    result = newTrack.get();
    track_map_.insert(std::make_pair(mid, std::move(newTrack)));
  }
  return result;
}

srs_error_t WrtcAgentPc::removeTrack(const std::string& mid) {
  ELOG_TRACE("message: removeTrack, connectionId:%s mediaStreamId:%s", id_.c_str(), mid.c_str());

  srs_error_t result = nullptr;
  
  auto found = track_map_.find(mid);

  if (track_map_.end() == found) {
    return srs_error_wrap(result, "not found mediaStreamId:%s", mid.c_str());
  }
  
  connection_->removeMediaStream(mid);
  found->second->close();
  track_map_.erase(found);

  return result;
}

void WrtcAgentPc::setAudioSsrc(const std::string& mid, uint32_t ssrc) {
  connection_->getLocalSdpInfo()->audio_ssrc_map[mid] = ssrc;
}

void WrtcAgentPc::setVideoSsrcList(const std::string& mid, 
                                   std::vector<uint32_t> ssrc_list) {
  connection_->getLocalSdpInfo()->video_ssrc_map[mid] = ssrc_list;
}

void WrtcAgentPc::onFrame(const owt_base::Frame& f) {
  callBack(E_DATA, f);
}

void WrtcAgentPc::onVideoInfo(const std::string& videoInfoJSON) {
  OLOG_INFO_THIS(videoInfoJSON);
}

void WrtcAgentPc::callBack(E_SINKID id, const std::string& message) {
  if (!sink_) {
    return;
  }
  
  sink_->callBack([id, message](std::shared_ptr<WebrtcAgentSink> sink) {
    switch(id) {
      case E_CANDIDATE :
        sink->onCandidate(message);
        break;
      case E_FAILED :
        sink->onFailed(message);
        break;
      case E_READY :
        sink->onReady();
        break;
      case E_ANSWER :
        sink->onAnswer(message);
        break;
      case E_DATA:
        ;
    }
  });  
}

void WrtcAgentPc::callBack(E_SINKID, const owt_base::Frame& message) {
  if (!sink_) {
    return;
  }

  if (!sink_->task_queue_) {
    sink_->onFrame(message);
  } else {
    sink_->callBack([message](std::shared_ptr<WebrtcAgentSink> sink){
      sink->onFrame(message);
    });
  }
}

void WrtcAgentPc::asyncTask(std::function<void(std::shared_ptr<WrtcAgentPc>)> f) {
  std::weak_ptr<WrtcAgentPc> weak_this = weak_from_this();
  worker_->task([weak_this, f] {
    if (auto this_ptr = weak_this.lock()) {
      f(this_ptr);
    }
  });
}

} //namespace wa

