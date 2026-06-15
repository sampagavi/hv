#include "attention_panel.hpp"
#include <ftxui/dom/elements.hpp>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace ftxui;

static const char* weight_to_block(float w) {
    if (w < 0.1f) return " ";
    if (w < 0.3f) return ".";
    if (w < 0.5f) return "░";
    if (w < 0.7f) return "▒";
    if (w < 0.9f) return "▓";
    return "█";
}

static Color weight_color(float w) {
    if (w < 0.1f) return Color::GrayDark;
    if (w < 0.3f) return Color::Blue;
    if (w < 0.5f) return Color::Cyan;
    if (w < 0.7f) return Color::Green;
    if (w < 0.9f) return Color::Yellow;
    return Color::Red;
}

ftxui::Component MakeAttentionPanel(
    const LayerPacket** selected_packet,
    int& pan_x,
    int& pan_y,
    float& contrast,
    bool& fullscreen)
{
    return Renderer([selected_packet, &pan_x, &pan_y, &contrast, &fullscreen] {
        const LayerPacket* pkt = *selected_packet;

        std::ostringstream title_ss;
        title_ss << " Attention Matrix (head 0)"
                 << "  [+/-: contrast | arrows/hjkl: pan | f: fullscreen]";
        if (pkt && !pkt->tokens.empty()) {
            title_ss << "  Tokens: " << pkt->shape.seq_len << " × " << pkt->shape.seq_len;
            title_ss << "  Window: [" << pan_x << "-" << (pan_x+7) << "] × ["
                     << pan_y << "-" << (pan_y+7) << "]";
        }

        Elements rows;
        rows.push_back(text(title_ss.str()) | bold | color(Color::Cyan));
        rows.push_back(separator());

        if (!pkt || pkt->attention_weights.empty() || pkt->tokens.empty()) {
            rows.push_back(text("  Select an attention layer to visualize.") | dim);
            return window(text(" Attention "), vbox(std::move(rows)));
        }

        int seq = pkt->shape.seq_len;
        pan_x = std::min(pan_x, std::max(0, seq - 1));
        pan_y = std::min(pan_y, std::max(0, seq - 1));
        int display_w = std::min(16, seq - pan_x);
        int display_h = std::min(8,  seq - pan_y);

        // Legend box
        auto legend = vbox({
            text("Weight Legend") | bold | color(Color::Cyan),
            separator(),
            hbox({text("█") | color(Color::Red),     text(" >= 0.9")}),
            hbox({text("▓") | color(Color::Yellow),  text(" >= 0.7")}),
            hbox({text("▒") | color(Color::Green),   text(" >= 0.5")}),
            hbox({text("░") | color(Color::Cyan),    text(" >= 0.3")}),
            hbox({text(".") | color(Color::Blue),    text(" >= 0.1")}),
            hbox({text(" ") | color(Color::GrayDark),text("  < 0.1")}),
        }) | border | size(WIDTH, EQUAL, 18);

        // Column headers
        Elements header_cells;
        header_cells.push_back(text("     "));
        for (int c = pan_x; c < pan_x + display_w; ++c) {
            std::string tok = c < (int)pkt->tokens.size() ? pkt->tokens[c] : "?";
            if (tok.size() > 4) tok = tok.substr(0, 4);
            while (tok.size() < 4) tok += ' ';
            header_cells.push_back(text(tok + " ") | color(Color::GreenLight));
        }

        Elements matrix_rows;
        matrix_rows.push_back(hbox(std::move(header_cells)));

        for (int r = pan_y; r < pan_y + display_h; ++r) {
            Elements row_cells;
            std::string row_label = r < (int)pkt->tokens.size() ? pkt->tokens[r] : "?";
            if (row_label.size() > 4) row_label = row_label.substr(0, 4);
            while (row_label.size() < 4) row_label += ' ';
            row_cells.push_back(text(row_label + " ") | color(Color::GreenLight));
            for (int c = pan_x; c < pan_x + display_w; ++c) {
                float w = pkt->attention_weights[r * seq + c];
                float adj = std::min(1.0f, w * contrast);
                row_cells.push_back(text(std::string(weight_to_block(adj)) + " ") | color(weight_color(adj)));
            }
            matrix_rows.push_back(hbox(std::move(row_cells)));
        }

        rows.push_back(hbox({
            vbox(std::move(matrix_rows)) | flex,
            legend,
        }));

        return window(text(" Attention "), vbox(std::move(rows)));
    });
}
