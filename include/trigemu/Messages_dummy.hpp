#ifndef TRIGEMU_INCLUDE_TRIGEMU_MESSAGES_DUMMY_HPP_
#define TRIGEMU_INCLUDE_TRIGEMU_MESSAGES_DUMMY_HPP_

#include <cstdint>

namespace df {
  using timestamp_t = uint64_t;
  using timestamp_diff_t = int64_t;
  
  class TriggerInhibit
  {
  public:
    bool busy() { return busy_; }

  private:
    bool busy_;
  };
  
  class TimeSync
  {
  public:
    timestamp_t timestamp() { return timestamp_; }
    uint64_t system_time() { return system_time_; }
  private:
    timestamp_t timestamp_;
    uint64_t system_time_;
  };
  class TriggerDecision {};
}


#endif // include guard
