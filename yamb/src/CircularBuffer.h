//
// Created by panda on 8/28/19.
//
#ifndef CSHARPWRAPPER_CIRCULAR_BUFFER_H
#define CSHARPWRAPPER_CIRCULAR_BUFFER_H

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <iostream>

template<class T, size_t N>
class CircularBuffer {
public:
  explicit CircularBuffer(size_t
                          size = N) :
      max_size_(size) {}

  void put(T item) {
    std::lock_guard<std::shared_timed_mutex> lock(mutex_);
    *(buf_ + head_) = item;

    if (full_) {
      tail_ = (tail_ + 1) % max_size_;
    }

    head_ = (head_ + 1) % max_size_;

    full_ = head_ == tail_;
  }

  bool isExists(T item) {
    std::shared_lock<std::shared_timed_mutex> read_lock(mutex_);
    const auto sizebuff = size();
    if (sizebuff == 0)
      return false;
    for (int i = sizebuff; --i;) {
      if ((*(buf_ + i) - item) == 0)
        return true;
    }
    return false;
  }

  T get() {
    std::lock_guard<std::shared_timed_mutex> lock(mutex_);

    if (empty()) {
      return T();
    }

    //Read data and advance the tail (we now have a free space)
    auto val = *(buf_ + tail_);
    full_ = false;
    tail_ = (tail_ + 1) % max_size_;

    return val;
  }

  void reset() {
    std::lock_guard<std::shared_timed_mutex> lock(mutex_);
    head_ = tail_;
    full_ = false;
  }

  bool empty() const {
    //if head and tail are equal, we are empty
    return (!full_ && (head_ == tail_));
  }

  bool full() const {
    //If tail is ahead the head by 1, we are full
    return full_;
  }

  size_t capacity() const {
    return max_size_;
  }

  size_t size() const {
    size_t size = max_size_;

    if (full_)
      return size;

    if (head_ >= tail_) {
      size = head_ - tail_;
    } else {
      size = max_size_ + head_ - tail_;
    }

    return size;
  }

private:
  std::shared_timed_mutex mutex_;
  T buf_[N];
  size_t head_ = 0;
  size_t tail_ = 0;
  const size_t max_size_;
  bool full_ = false;
};

#endif //CSHARPWRAPPER_CIRCULAR_BUFFER_H
