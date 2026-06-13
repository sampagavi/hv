#include "stream_panel.hpp"
#include <ftxui/dom/elements.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace ftxui;

static std::string format_timestamp(int64_t us) {
    int64_t total_ms = us / 1000;
    int64_t ms       = total_ms % 1000;
    time_t  secs     = static_cast<time_t>(total_ms / 1000);
    struct tm tm_info;
    localtime_r(&secs, &tm_info);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &tm_info);
    std::ostringstream out;
    out << buf << "." << std::setw(3) << std::setfill('0') << ms;
    return out.str();
}

static std::string device_str(const LayerPacket& p) {
    if (p.device == DeviceType::CPU) return "CPU   ";
    return "CUDA:" + std::to_string(p.gpu_index);
}

static std::string layer_type_str(LayerType t) {
    switch (t) {
        case LayerType::Embedding:    return "Embed";
        case LayerType::AttentionSelf:return "Attn ";
        case LayerType::MLP_SwiGLU:  return "MLP  ";
        case LayerType::LayerNorm:    return "LNorm";
        default:                      return "?????";
    }
}

static Color latency_color(float ms) {
    if (ms < 2.0f) return Color::Green;
    if (ms < 6.0f) return Color::Yellow;
    return Color::Red;
}

ftxui::Component MakeStreamPanel(
    const std::vector<LayerPacket>* packets,
    int& scroll_offset,
    bool& frozen)
{
    return Renderer([packets, &scroll_offset, &frozen] {
        Elements header = {
            text(" ID    ") | bold | color(Color::Cyan),
            text("| Timestamp     ") | bold | color(Color::Cyan),
            text("| Type  ") | bold | color(Color::Cyan),
            text("| Device ") | bold | color(Color::Cyan),
            text("| Latency") | bold | color(Color::Cyan),
        };

        Elements rows;
        rows.push_back(hbox(std::move(header)));
        rows.push_back(separator());

        int visible = 20;
        int total   = static_cast<int>(packets->size());
        int start   = frozen
            ? std::max(0, total - visible - scroll_offset)
            : std::max(0, total - visible);

        for (int i = start; i < std::min(start + visible, total); ++i) {
            const auto& p = (*packets)[i];
            std::ostringstream lat;
            lat << std::fixed << std::setprecision(3) << p.latency_ms << " ms";
            bool is_cpu = (p.device == DeviceType::CPU);
            auto row = hbox({
                text(" " + std::to_string(p.id)) | size(WIDTH, EQUAL, 7),
                text("| " + format_timestamp(p.timestamp_us) + " ") | size(WIDTH, EQUAL, 16),
                text("| " + layer_type_str(p.layer_type) + " ") | size(WIDTH, EQUAL, 8),
                (is_cpu
                    ? text("| " + device_str(p) + " ") | size(WIDTH, EQUAL, 9) | color(Color::Yellow)
                    : text("| " + device_str(p) + " ") | size(WIDTH, EQUAL, 9)),
                text("| ") | size(WIDTH, EQUAL, 2),
                text(lat.str()) | color(latency_color(p.latency_ms)) | bold,
            });
            rows.push_back(std::move(row));
        }

        if (frozen)
            rows.push_back(text(" [FROZEN — navigate ↑/↓]") | color(Color::Yellow));

        return window(text(" Live Stream "), vbox(std::move(rows))) | flex;
    });
}
