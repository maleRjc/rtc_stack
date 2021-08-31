#ifndef __WS_RTC_STACK_API_H__
#define __WS_RTC_STACK_API_H__

#include <memory>
#include <functional>

#include "rtc_media_frame.h"

namespace wa
{

enum EFormatPreference
{
  p_unknow = 0,
  p_h264 = 1,
  p_opus = 2,
};

enum EMediaType
{
  media_unknow,
  media_audio,
  media_video
};

struct FormatPreference
{
  EFormatPreference format_;
  std::string profile_;  //for video, h264::42e01f 42001f; vp9: 0 2
};

struct TTrackInfo
{
  std::string mid_;
  EMediaType type_{media_unknow};
  FormatPreference preference_;
  std::string direction_;
};

class ITaskQueue 
{
 public:
  virtual ~ITaskQueue() = default;

  typedef std::function<void()> Task;

  virtual void post(Task) = 0;
};

class WebrtcAgentSink 
    : public std::enable_shared_from_this<WebrtcAgentSink>
{
 public:
  virtual ~WebrtcAgentSink() = default;

  virtual void onFailed(const std::string&) = 0;
  virtual void onCandidate(const std::string&) = 0;
  virtual void onReady() = 0;
  virtual void onAnswer(const std::string&) = 0;
  virtual void onFrame(const owt_base::Frame&) = 0;
  virtual void onStat() = 0;

  void callBack(std::function<void(std::shared_ptr<WebrtcAgentSink>)> f) {
    if (!task_queue_) {
      f(shared_from_this());
    } else {
      std::weak_ptr<WebrtcAgentSink> weak_this = shared_from_this();
      task_queue_->post([weak_this, f] {
      if (auto this_ptr = weak_this.lock()) {
          f(this_ptr);
        }
      });
    }
  }

  ITaskQueue* task_queue_{nullptr};
};

struct TOption
{ 
  std::string connectId_;
  std::vector<TTrackInfo> tracks_; 
  std::shared_ptr<WebrtcAgentSink> call_back_;
};

class rtc_api
{
 public:
  ~rtc_api() { }

  /**
   * network_addresses{ip:port}
   * service_addr{udp://ip:port} "udp://192.168.1.156:9000"
   */
  virtual int initiate(uint32_t num_workers, 
      const std::vector<std::string>& network_addresses,
      const std::string& service_addr) = 0;

  virtual int publish(TOption&, const std::string& offer) = 0;

  virtual int unpublish(const std::string& connectId) = 0;

  virtual void subscribe(std::string& connectId, const std::string& offer) = 0;

  virtual void unsubscribe(const std::string& connectId) = 0;

  virtual void linkup(int connectId, WebrtcAgentSink*) = 0;

  virtual void cutoff(std::string& connectId) = 0;

  virtual void mediaOnOff() = 0;
};

class AgentFactory
{
 public:
 AgentFactory() = default;
 ~AgentFactory() = default;
 
  std::unique_ptr<rtc_api> create_agent();
};

}

#endif
