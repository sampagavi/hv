#include "anomaly_panel.hpp"
#include <ftxui/dom/elements.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace ftxui;

static std::string fmt_ts(int64_t us) {
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

struct CatStyle { std::string icon; Color color; };
static CatStyle cat_style(const std::string& cat) {
    if (cat == "SLOW")         return {"✖", Color::Red};
    if (cat == "OUTLIER")      return {"▲", Color::Yellow};
    if (cat == "OOM")          return {"💀", Color::Red};
    if (cat == "SPARSE")       return {"⚠", Color::Yellow};
    if (cat == "CPU_FALLBACK") return {"ℹ", Color::Cyan};
    return {"●", Color::Default};
}

ftxui::Component MakeAnomalyPanel(const std::vector<AnomalyEvent>* events) {
    return Renderer([events] {
        Elements rows;
        rows.push_back(text(" Anomaly Ledger") | bold | color(Color::Red));
        rows.push_back(separator());

        if (events->empty()) {
            rows.push_back(text("  No anomalies detected.") | dim | color(Color::Green));
        } else {
            int show  = std::min(20, (int)events->size());
            int start = (int)events->size() - show;
            for (int i = start; i < (int)events->size(); ++i) {
                const auto& e = (*events)[i];
                auto [icon, col] = cat_style(e.category);
                rows.push_back(hbox({
                    text(" " + icon + " ") | color(col) | bold,
                    text("[" + e.category + "]") | color(col) | bold | size(WIDTH, EQUAL, 14),
                    text(" " + fmt_ts(e.timestamp_us) + " ") | dim | size(WIDTH, EQUAL, 14),
                    text(e.layer_name + " ") | size(WIDTH, EQUAL, 28),
                    text(e.detail) | dim,
                }));
            }
        }

        return window(text(" Anomalies "), vbox(std::move(rows))) | flex;
    });
}
