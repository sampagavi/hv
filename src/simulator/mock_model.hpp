#pragma once
#include <atomic>
#include <thread>
#include <functional>
#include "../../include/types.hpp"

using PacketCallback = std::function<void(LayerPacket)>;

class MockModel {
public:
    explicit MockModel(PacketCallback cb) : callback_(std::move(cb)) {}
    ~MockModel() { stop(); }

    void start();
    void stop();

private:
    void run();

    PacketCallback  callback_;
    std::thread     thread_;
    std::atomic<bool> running_{false};
};
