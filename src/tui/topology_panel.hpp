#pragma once
#include <string>
#include <vector>
#include <ftxui/component/component.hpp>
#include "../../include/types.hpp"

struct TopologyNode {
    std::string name;
    LayerType   type;
    bool        expanded{true};
    bool        active{false};
    std::vector<TopologyNode> children;
};

ftxui::Component MakeTopologyPanel(
    std::vector<TopologyNode>& nodes,
    int& selected_index);
