#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include "../../include/types.hpp"

using PacketCallback = std::function<void(LayerPacket)>;

class Replayer {
public:
    Replayer(const std::string& path, PacketCallback cb);
    ~Replayer() { stop(); }

    bool load();
    void start();
    void stop();

    void pause_toggle()  { paused_ = !paused_; }
    void step_forward();
    void step_backward();
    bool is_paused()  const { return paused_; }
    size_t current()  const { return cursor_; }
    size_t total()    const { return packets_.size(); }

private:
    void run();

    std::string      path_;
    PacketCallback   callback_;
    std::vector<LayerPacket> packets_;
    std::atomic<size_t>  cursor_{0};
    std::atomic<bool>    paused_{false};
    std::atomic<bool>    running_{false};
    std::thread          thread_;
};
