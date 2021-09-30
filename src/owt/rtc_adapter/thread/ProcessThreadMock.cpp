#include "rtc_adapter/thread/ProcessThreadMock.h"

#include <string>
#include "rtc_base/checks.h"
#include "rtc_base/time_utils.h"

namespace rtc_adapter {

namespace {

// We use this constant internally to signal that a module has requested
// a callback right away.  When this is set, no call to TimeUntilNextProcess
// should be made, but Process() should be called directly.
const int64_t kCallProcessImmediately = -1;

int64_t GetNextCallbackTime(webrtc::Module* module, int64_t time_now) {
  int64_t interval = module->TimeUntilNextProcess();
  if (interval < 0) {
    // Falling behind, we should call the callback now.
    return time_now;
  }
  return time_now + interval;
}
}  // namespace


ProcessThreadMock::ProcessThreadMock(rtc::TaskQueue* task_queue)
  : impl_{task_queue} {
}

// Implements ProcessThread
void ProcessThreadMock::WakeUp(webrtc::Module* module) {
  RTC_DCHECK(thread_checker_.IsCurrent());

  for (ModuleCallback& m : modules_) {
    if (m.module == module)
      m.next_callback = kCallProcessImmediately;
  }

  impl_->PostDelayedTask([this]() {
      this->Process();
  }, 1);
}

// Implements ProcessThread
void ProcessThreadMock::PostTask(std::unique_ptr<webrtc::QueuedTask> task) {
  RTC_DCHECK(thread_checker_.IsCurrent());
  impl_->PostTask(std::move(task));
}

// Implements ProcessThread
void ProcessThreadMock::RegisterModule(webrtc::Module* module, const rtc::Location& from) {
  RTC_DCHECK(module) << from.ToString();
  RTC_DCHECK(thread_checker_.IsCurrent());

#if RTC_DCHECK_IS_ON
  {
    // Catch programmer error.
    for (const ModuleCallback& mc : modules_) {
      RTC_DCHECK(mc.module != module)
          << "Already registered here: " << mc.location.ToString() << "\n"
          << "Now attempting from here: " << from.ToString();
    }
  }
#endif

  // Now that we know the module isn't in the list, we'll call out to notify
  // the module that it's attached to the worker thread.  We don't hold
  // the lock while we make this call.
  module->ProcessThreadAttached(this);

  modules_.push_back(ModuleCallback(module, from));

  impl_->PostDelayedTask([this]() {
    this->Process();
  }, 1);
}

// Implements ProcessThread
void ProcessThreadMock::DeRegisterModule(webrtc::Module* module) {
  RTC_DCHECK(module);
  RTC_DCHECK(thread_checker_.IsCurrent());

  modules_.remove_if(
      [&module](const ModuleCallback& m) { return m.module == module; });

  // Notify the module that it's been detached.
  module->ProcessThreadAttached(nullptr);
}

void ProcessThreadMock::Process() {
  int64_t now = rtc::TimeMillis();
  int64_t next_checkpoint = now + (1000 * 60);

  for (ModuleCallback& m : modules_) {
    // TODO(tommi): Would be good to measure the time TimeUntilNextProcess
    // takes and dcheck if it takes too long (e.g. >=10ms).  Ideally this
    // operation should not require taking a lock, so querying all modules
    // should run in a matter of nanoseconds.
    if (m.next_callback == 0)
      m.next_callback = GetNextCallbackTime(m.module, now);

    if (m.next_callback <= now ||
        m.next_callback == kCallProcessImmediately) {
      {
        //TRACE_EVENT2("webrtc", "ModuleProcess", "function",
        //             m.location.function_name(), "file",
       //             m.location.file_and_line());
        m.module->Process();
      }
      // Use a new 'now' reference to calculate when the next callback
      // should occur.  We'll continue to use 'now' above for the baseline
      // of calculating how long we should wait, to reduce variance.
      int64_t new_now = rtc::TimeMillis();
      m.next_callback = GetNextCallbackTime(m.module, new_now);
    }

    if (m.next_callback < next_checkpoint)
      next_checkpoint = m.next_callback;
  }

  int64_t time_to_wait = next_checkpoint - rtc::TimeMillis();
  if (time_to_wait > 0) {
    impl_->PostDelayedTask([this]() {
      this->Process();
    }, time_to_wait);
  }
}

} // namespace rtc_adapter

