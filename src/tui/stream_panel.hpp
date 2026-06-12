#pragma once
#include <vector>
#include <ftxui/component/component.hpp>
#include "../../include/types.hpp"

ftxui::Component MakeStreamPanel(
    const std::vector<LayerPacket>* packets,
    int& scroll_offset,
    bool& frozen);
