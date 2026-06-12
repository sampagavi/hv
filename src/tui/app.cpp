#include "app.hpp"
#include "topology_panel.hpp"
#include "stream_panel.hpp"
#include "attention_panel.hpp"
#include "metrics_panel.hpp"
#include "anomaly_panel.hpp"
#include "../simulator/mock_model.hpp"
#include "../anomaly/detector.hpp"
#include "../replay/replayer.hpp"
#include "../replay/serializer.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <atomic>
#include <mutex>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace ftxui;

static RingBuffer<LayerPacket>  g_packet_buf;
static RingBuffer<AnomalyEvent, ANOMALY_BUFFER_CAPACITY> g_anomaly_buf;

// Build llama-style topology for the tree panel
static std::vector<TopologyNode> build_topology() {
    std::vector<TopologyNode> nodes;
    TopologyNode embed;
    embed.name = "embed_tokens";
    embed.type = LayerType::Embedding;
    nodes.push_back(embed);

    for (int i = 0; i < 32; ++i) {
        TopologyNode layer;
        layer.name = "layer." + std::to_string(i);
        layer.type = LayerType::Unknown;
        layer.expanded = (i < 2);

        TopologyNode attn; attn.name = "self_attn"; attn.type = LayerType::AttentionSelf;
        TopologyNode mlp;  mlp.name  = "mlp";       mlp.type  = LayerType::MLP_SwiGLU;
        TopologyNode ln;   ln.name   = "layernorm";  ln.type   = LayerType::LayerNorm;

        layer.children = {attn, mlp, ln};
        nodes.push_back(std::move(layer));
    }
    return nodes;
}

static Element help_overlay() {
    return vbox({
        text(" Keybindings ") | bold | center,
        separator(),
        text("  Tab          Cycle panel focus"),
        text("  j / k        Navigate topology up/down"),
        text("  Enter/Space  Expand/collapse topology node"),
        text("  h/j/k/l      Pan attention matrix"),
        text("  +/-          Adjust attention contrast"),
        text("  f            Fullscreen attention panel"),
        text("  r            Reset ring buffer"),
        text("  Ctrl+S       Save capture to file"),
        text("  Space        Pause/resume replay"),
        text("  ←/→          Step frame in replay"),
        text("  Q/q          Quit"),
        text("  ?            Toggle this help"),
    }) | border | center;
}

void run_app(AppConfig cfg) {
    auto screen = ScreenInteractive::Fullscreen();

    std::vector<TopologyNode> topology = build_topology();
    int  topo_sel   = 0;
    int  stream_scroll = 0;
    bool stream_frozen = false;
    int  pan_x = 0, pan_y = 0;
    float contrast = 1.5f;
    bool attn_fullscreen = false;
    bool show_help = false;
    int  focus_panel = 0; // 0-4

    std::mutex data_mutex;
    std::vector<LayerPacket>  packets_snapshot;
    std::vector<AnomalyEvent> anomalies_snapshot;
    const LayerPacket*        selected_packet = nullptr;

    AnomalyDetector detector(g_anomaly_buf);

    auto on_packet = [&](LayerPacket pkt) {
        detector.inspect(pkt);
        g_packet_buf.push(pkt);
        screen.PostEvent(Event::Custom);
    };

    std::unique_ptr<MockModel> sim;
    std::unique_ptr<Replayer>  replayer;

    if (cfg.replay_mode) {
        replayer = std::make_unique<Replayer>(cfg.replay_file, on_packet);
        if (!replayer->load()) {
            screen.Exit();
            return;
        }
        replayer->start();
    } else {
        sim = std::make_unique<MockModel>(on_packet);
        sim->start();
    }

    // Refresh snapshots when new packets arrive
    auto refresh = [&]() {
        std::lock_guard<std::mutex> lk(data_mutex);
        packets_snapshot  = g_packet_buf.peek(200);
        anomalies_snapshot = g_anomaly_buf.peek(128);

        // Find selected packet (most recent attention layer)
        selected_packet = nullptr;
        for (int i = (int)packets_snapshot.size() - 1; i >= 0; --i) {
            if (packets_snapshot[i].layer_type == LayerType::AttentionSelf
                && !packets_snapshot[i].attention_weights.empty()) {
                selected_packet = &packets_snapshot[i];
                break;
            }
        }
    };

    auto panel1 = MakeTopologyPanel(topology, topo_sel);
    auto panel2 = MakeStreamPanel(&packets_snapshot, stream_scroll, stream_frozen);
    auto panel3 = MakeAttentionPanel(&selected_packet, pan_x, pan_y, contrast, attn_fullscreen);
    auto panel4 = MakeMetricsPanel(&selected_packet);
    auto panel5 = MakeAnomalyPanel(&anomalies_snapshot);

    auto make_footer = [&]() -> Element {
        int total_pkts = static_cast<int>(packets_snapshot.size());
        int buf_pct    = std::min(100, total_pkts * 100 / 512);
        std::string mode = cfg.replay_mode ? "REPLAY" : "LIVE";
        auto left = hbox({
            text(" [TAB] switch pane") | color(Color::Cyan),
            text(" | [j/k] navigate") | color(Color::Cyan),
            text(" | [space] pin") | color(Color::Cyan),
            text(" | [r] reset") | color(Color::Cyan),
            text(" | [Ctrl+S] save") | color(Color::Cyan),
            text(" | [q] quit") | color(Color::Cyan),
        });
        auto right = hbox({
            text("Packets: " + std::to_string(total_pkts)) | bold,
            text("  |  "),
            text("Buffer: " + std::to_string(buf_pct) + "%")
                | color(buf_pct > 90 ? Color::Red : Color::Green) | bold,
            text("  |  "),
            text(mode) | bold | color(cfg.replay_mode ? Color::Yellow : Color::Green),
            text(" "),
        });
        return hbox({left | flex, right}) | bgcolor(Color::GrayDark);
    };

    auto layout = Renderer([&] {
        refresh();
        Element body;
        if (attn_fullscreen) {
            body = panel3->Render();
        } else {
            auto top = hbox({
                panel1->Render() | size(WIDTH, EQUAL, 30),
                panel2->Render() | flex,
            });
            auto mid = panel3->Render();
            auto bot = hbox({
                panel4->Render() | flex,
                panel5->Render() | flex,
            });
            body = vbox({top | flex, mid | size(HEIGHT, EQUAL, 14), bot | flex, make_footer()});
        }
        if (show_help) {
            body = dbox({body, help_overlay()});
        }
        return body;
    });

    auto root = CatchEvent(layout, [&](Event ev) -> bool {
        if (ev == Event::Character('q') || ev == Event::Character('Q')) {
            screen.Exit();
            return true;
        }
        if (ev == Event::Character('?')) { show_help = !show_help; return true; }
        if (ev == Event::Tab) { focus_panel = (focus_panel + 1) % 5; return true; }
        if (ev == Event::Character('r')) {
            g_packet_buf.clear();
            g_anomaly_buf.clear();
            packets_snapshot.clear();
            anomalies_snapshot.clear();
            return true;
        }
        if (ev == Event::Character('f')) { attn_fullscreen = !attn_fullscreen; return true; }
        if (ev == Event::Character('+')) { contrast = std::min(5.0f, contrast + 0.2f); return true; }
        if (ev == Event::Character('-')) { contrast = std::max(0.2f, contrast - 0.2f); return true; }

        // Topology navigation (panel 0)
        if (ev == Event::Character('j')) { ++topo_sel; return true; }
        if (ev == Event::Character('k')) { if (topo_sel > 0) --topo_sel; return true; }

        // Attention panning
        if (ev == Event::ArrowRight || ev == Event::Character('l')) { ++pan_x; return true; }
        if (ev == Event::ArrowLeft  || ev == Event::Character('h')) { if (pan_x > 0) --pan_x; return true; }
        if (ev == Event::ArrowDown  || ev == Event::Character('j') && focus_panel == 2) { ++pan_y; return true; }
        if (ev == Event::ArrowUp    || ev == Event::Character('k') && focus_panel == 2) { if (pan_y > 0) --pan_y; return true; }

        // Stream scroll freeze
        if (ev == Event::ArrowUp && focus_panel == 1) {
            stream_frozen = true; ++stream_scroll; return true;
        }
        if (ev == Event::ArrowDown && focus_panel == 1) {
            if (stream_scroll > 0) --stream_scroll;
            else stream_frozen = false;
            return true;
        }

        // Ctrl+S — save capture
        if (ev == Event::Special({19})) { // Ctrl+S = ASCII 19
            auto snp = g_packet_buf.peek(512);
            serialize_packets("capture.llmt", snp);
            return true;
        }

        // Replay controls
        if (replayer) {
            if (ev == Event::Character(' ')) { replayer->pause_toggle(); return true; }
            if (ev == Event::ArrowRight)     { replayer->step_forward(); return true; }
            if (ev == Event::ArrowLeft)      { replayer->step_backward(); return true; }
        }

        if (ev == Event::Custom) return false; // let renderer re-run
        return false;
    });

    screen.Loop(root);
}

