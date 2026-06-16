#include "topology_panel.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

static std::string layer_type_icon(LayerType t) {
    switch (t) {
        case LayerType::Embedding:    return "[E]";
        case LayerType::AttentionSelf:return "[A]";
        case LayerType::MLP_SwiGLU:  return "[M]";
        case LayerType::LayerNorm:    return "[N]";
        case LayerType::Unknown:      return "[T]";
        default:                      return "[?]";
    }
}

ftxui::Component MakeTopologyPanel(
    std::vector<TopologyNode>& nodes,
    int& selected_index)
{
    return Renderer([&nodes, &selected_index] {
        Elements rows;
        rows.push_back(text(" Model Topology") | bold | color(Color::Cyan));
        rows.push_back(separator());

        int idx = 0;
        for (auto& node : nodes) {
            bool sel = (idx == selected_index);
            std::string prefix = node.active ? "● " : "  ";
            auto label = text(prefix + layer_type_icon(node.type) + " " + node.name);
            if (sel) label = label | inverted;
            rows.push_back(label);
            ++idx;

            if (node.expanded) {
                for (auto& child : node.children) {
                    bool csel = (idx == selected_index);
                    std::string cpfx = child.active ? "  ● " : "    ";
                    auto clabel = text(cpfx + layer_type_icon(child.type) + " " + child.name);
                    if (csel) clabel = clabel | inverted;
                    rows.push_back(clabel);
                    ++idx;
                }
            }
        }

        return window(text(" Topology "), vbox(std::move(rows))) | flex;
    });
}
