#include "replayer.hpp"
#include "serializer.hpp"
#include <chrono>
#include <thread>

Replayer::Replayer(const std::string& path, PacketCallback cb)
    : path_(path), callback_(std::move(cb)) {}

bool Replayer::load() {
    packets_ = deserialize_packets(path_);
    return !packets_.empty();
}

void Replayer::start() {
    running_ = true;
    thread_ = std::thread(&Replayer::run, this);
}

void Replayer::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void Replayer::step_forward() {
    if (cursor_ + 1 < packets_.size()) {
        callback_(packets_[cursor_++]);
    }
}

void Replayer::step_backward() {
    if (cursor_ > 0) --cursor_;
}

void Replayer::run() {
    while (running_ && cursor_ < packets_.size()) {
        if (paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        size_t cur = cursor_.load();
        callback_(packets_[cur]);
        size_t next = cur + 1;
        cursor_ = next;

        if (next < packets_.size()) {
            int64_t delay_us = packets_[next].timestamp_us - packets_[cur].timestamp_us;
            if (delay_us > 0 && delay_us < 5'000'000) {
                std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
            }
        }
    }
    running_ = false;
}

