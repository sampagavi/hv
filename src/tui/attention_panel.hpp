#pragma once
#include <ftxui/component/component.hpp>
#include "../../include/types.hpp"

ftxui::Component MakeAttentionPanel(
    const LayerPacket** selected_packet,
    int& pan_x,
    int& pan_y,
    float& contrast,
    bool& fullscreen);
