#include "webrtc_agent.h"

#include "h/rtc_return_value.h"
#include "webrtc_agent_pc.h"

#include "erizo/global_init.h"

using namespace erizo;

namespace wa {

DEFINE_LOGGER(WebrtcAgent, "wa.agent");

std::shared_ptr<ThreadPool> WebrtcAgent::workers_;
std::shared_ptr<IOThreadPool> WebrtcAgent::io_workers_;

bool WebrtcAgent::global_init_ = false;

WebrtcAgent::WebrtcAgent() = default;

WebrtcAgent::~WebrtcAgent() = default;

int WebrtcAgent::initiate(uint32_t num_workers, 
  const std::vector<std::string>& ip_addresses, const std::string& service_addr)
{
  if(ip_addresses.empty()){
    return wa_e_invalid_param;
  }

  if(!global_init_){
    erizo::erizo_global_init();
    workers_ = std::make_shared<ThreadPool>(num_workers);
    workers_->start();
    io_workers_ = std::make_shared<IOThreadPool>(num_workers);
    io_workers_->start();
    global_init_ = true;
  }

  if(0 == num_workers){
    num_workers = 1;
  }

  ELOG_INFO("WebrtcAgent initiate %d workers, ip:%s", num_workers, ip_addresses[0].c_str());
  if(!network_addresses_.empty()){
    return wa_e_already_initialized;
  }

  network_addresses_ = ip_addresses;
  stun_address_ = service_addr;

  return wa_ok;
}

int WebrtcAgent::publish(TOption& options, const std::string& offer)
{
  {
    std::lock_guard<std::mutex> guard(pcLock_);
    auto found = peerConnections_.find(options.connectId_);
    if(found != peerConnections_.end()){
      return wa_e_found;
    }
  }

  for(auto& i : options.tracks_) {
    i.direction_ = "sendonly";
  }
  
  std::shared_ptr<WrtcAgentPc> pc = std::make_shared<WrtcAgentPc>(options, *this);
  std::shared_ptr<Worker> worker = workers_->getLessUsedWorker();
  std::shared_ptr<IOWorker> ioworker = io_workers_->getLessUsedIOWorker();
  pc->init(worker, ioworker, network_addresses_, stun_address_);
  
  int code = wa_ok;

  pc->signalling("offer", offer);
  
  std::lock_guard<std::mutex> guard(pcLock_);
  peerConnections_.insert(std::make_pair(options.connectId_, std::move(pc)));
  
  return code;
}

int WebrtcAgent::unpublish(const std::string& connectId)
{
  std::shared_ptr<WrtcAgentPc> pc;
  {
    std::lock_guard<std::mutex> guard(pcLock_);

    auto found = peerConnections_.find(connectId);
    if(found == peerConnections_.end()){
      return wa_e_not_found;
    }

    pc = found->second;
    peerConnections_.erase(found);
  }

  pc->close();
  return wa_ok;
}

std::unique_ptr<rtc_api> AgentFactory::create_agent()
{
  return std::make_unique<WebrtcAgent>();
}

} //namespace wa

