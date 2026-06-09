#pragma once
#include <array>
#include <mutex>
#include <optional>
#include <vector>
#include <condition_variable>

// Fixed-capacity ring buffer. Oldest entry overwritten when full.
// RING_BUFFER_CAPACITY controls size at compile time.
static constexpr size_t RING_BUFFER_CAPACITY = 512;

template <typename T, size_t Capacity = RING_BUFFER_CAPACITY>
class RingBuffer {
public:
    RingBuffer() : head_(0), tail_(0), size_(0) {}

    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        buf_[tail_] = std::move(item);
        tail_ = (tail_ + 1) % Capacity;
        if (size_ < Capacity) {
            ++size_;
        } else {
            head_ = (head_ + 1) % Capacity; // overwrite oldest
        }
        cv_.notify_one();
    }

    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ == 0) return std::nullopt;
        T item = buf_[head_];
        head_ = (head_ + 1) % Capacity;
        --size_;
        return item;
    }

    // Returns up to n most recent items (newest last)
    std::vector<T> peek(size_t n) const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = std::min(n, size_);
        std::vector<T> result;
        result.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            size_t idx = (head_ + (size_ - count + i)) % Capacity;
            result.push_back(buf_[idx]);
        }
        return result;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = tail_ = size_ = 0;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::array<T, Capacity> buf_;
    size_t head_, tail_, size_;
};
