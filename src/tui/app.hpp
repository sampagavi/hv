#pragma once
#include <memory>
#include <string>
#include "../buffer/ring_buffer.hpp"
#include "../../include/types.hpp"

struct AppConfig {
    bool        replay_mode{false};
    std::string replay_file;
};

void run_app(AppConfig cfg);
