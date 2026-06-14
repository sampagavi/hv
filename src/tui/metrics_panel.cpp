#include "metrics_panel.hpp"
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <iomanip>

using namespace ftxui;

static std::string fmt(float v, int prec = 5) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << v;
    return ss.str();
}

static Element sparsity_bar(float rate) {
    int filled = static_cast<int>(rate * 20);
    Elements cells;
    for (int i = 0; i < 20; ++i) {
        float t = i / 20.0f;
        Color c = (t < 0.5f) ? Color::Green : (t < 0.75f ? Color::Yellow : Color::Red);
        cells.push_back(text(i < filled ? "█" : "░") | color(c));
    }
    std::string pct = " " + fmt(rate * 100.0f, 2) + "%";
    cells.push_back(text(pct));
    return hbox(std::move(cells));
}

static Element kv(const std::string& key, const std::string& val) {
    return hbox({
        text("  " + key) | size(WIDTH, EQUAL, 12),
        text(": "),
        text(val) | bold,
    });
}
static Element kv(const std::string& key, const std::string& val, Color vc) {
    return hbox({
        text("  " + key) | size(WIDTH, EQUAL, 12),
        text(": "),
        text(val) | color(vc) | bold,
    });
}

ftxui::Component MakeMetricsPanel(const LayerPacket** selected_packet) {
    return Renderer([selected_packet] {
        Elements rows;
        rows.push_back(text(" Runtime Metrics") | bold | color(Color::Cyan));
        rows.push_back(separator());

        const LayerPacket* pkt = *selected_packet;
        if (!pkt) {
            rows.push_back(text("  No layer selected.") | dim);
            return window(text(" Metrics "), vbox(std::move(rows))) | flex;
        }

        std::string dev = pkt->device == DeviceType::CUDA
            ? "CUDA:" + std::to_string(pkt->gpu_index) : "CPU";
        std::string shape = "[" + std::to_string(pkt->shape.batch) + ", " +
                            std::to_string(pkt->shape.seq_len) + ", " +
                            std::to_string(pkt->shape.hidden_dim) + "]";

        rows.push_back(kv("Layer", pkt->layer_name));
        rows.push_back(kv("Shape", shape));
        rows.push_back(kv("Dtype", pkt->dtype));
        if (pkt->device == DeviceType::CPU)
            rows.push_back(kv("Device", dev, Color::Yellow));
        else
            rows.push_back(kv("Device", dev));
        rows.push_back(separator());
        rows.push_back(kv("Mean", fmt(pkt->mean_activation)));
        if (pkt->max_activation > 8.0f)
            rows.push_back(kv("Max", fmt(pkt->max_activation), Color::Red));
        else
            rows.push_back(kv("Max", fmt(pkt->max_activation)));
        rows.push_back(kv("Min",  fmt(pkt->min_activation)));
        rows.push_back(kv("Stddev", fmt(pkt->stddev_activation)));
        rows.push_back(separator());
        rows.push_back(hbox({text("  Sparsity   : "), sparsity_bar(pkt->sparsity_rate)}));

        std::ostringstream lat;
        lat << std::fixed << std::setprecision(3) << pkt->latency_ms << " ms";
        Color lc = pkt->latency_ms > 10.0f ? Color::Red : (pkt->latency_ms > 4.0f ? Color::Yellow : Color::Green);
        rows.push_back(kv("Latency", lat.str(), lc));

        return window(text(" Metrics "), vbox(std::move(rows))) | flex;
    });
}
