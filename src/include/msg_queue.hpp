#ifndef MSG_QUEUE_HPP__
#define MSG_QUEUE_HPP__

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>

template<class MSG_TYPE>
class MsgQueue {
public:
  void add_msg_to_queue(std::shared_ptr<MSG_TYPE> msg)
  {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      queue_.push(msg);
      if (queue_.size() > 1000) {
        std::cout << "Warning: The size of queue > 1000 and sender isn't ready !!!" << std::endl;
      }
    }

    cond_.notify_one();
  }

  std::shared_ptr<MSG_TYPE> get_msg_from_queue()
  {
    if (!is_empty()) {
      return get_msg();
    }

    wait_queue();

    if (exit_) {
      return std::shared_ptr<MSG_TYPE>();
    } else {
      return get_msg();
    }
  }

  void wakeup_for_exit()
  {
    exit_ = true;
    cond_.notify_one();
  }

  void clean_queue()
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    queue_ = std::queue<std::shared_ptr<MSG_TYPE>>();
  }

  bool is_empty()
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return queue_.empty();
  }

private:
  std::mutex queue_mutex_;
  std::queue<std::shared_ptr<MSG_TYPE>> queue_;
  std::mutex cond_mutex_;
  std::condition_variable cond_;
  std::atomic_bool exit_{false};

  void wait_queue() {
    std::unique_lock<std::mutex> lock(cond_mutex_);
    cond_.wait(lock, [this]{
      return (!is_empty() || exit_);
    });
  }

  std::shared_ptr<MSG_TYPE> get_msg() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    std::shared_ptr<MSG_TYPE> msg = queue_.front();
    queue_.pop();
    return msg;
  }
};
#endif
