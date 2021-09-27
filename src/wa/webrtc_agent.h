#ifndef __WA_WEBRTC_AGENT_H__
#define __WA_WEBRTC_AGENT_H__

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

#include "h/rtc_stack_api.h"

#include "utils/IOWorker.h"
#include "utils/Worker.h"

#include "./wa_log.h"

namespace wa {

class WrtcAgentPc;

class WebrtcAgent final : public rtc_api {
  DECLARE_LOGGER();
public:
  WebrtcAgent();
  
  ~WebrtcAgent();

  int initiate(uint32_t num_workers, 
      const std::vector<std::string>& ip_addresses, const std::string& service_addr);

  int publish(TOption&, const std::string& offer) override;

  int unpublish(const std::string& connectId) override;

  void subscribe(std::string& connectId, const std::string& offer) override;
  
  void unsubscribe(const std::string& connectId) { }

  void linkup(int connectId, WebrtcAgentSink*) override;

  void cutoff(std::string& connectId) { }

  void mediaOnOff() { }

  const std::vector<std::string>& getAddresses(){
    return network_addresses_;
  }

private:
  using connection_id = std::string;
  using track_id = std::string;

  std::mutex pcLock_;
  std::map<connection_id, std::shared_ptr<WrtcAgentPc>> peerConnections_;
  //std::map<track_id, TTrackInfo> mediaTracks_;

  static std::shared_ptr<ThreadPool>  workers_;
  static std::shared_ptr<IOThreadPool> io_workers_;
  static bool global_init_;

  std::vector<std::string> network_addresses_;
  std::string stun_address_;
};

} //!wa
#endif //__WA_WEBRTC_AGENT_H__

