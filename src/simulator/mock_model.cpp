#include "mock_model.hpp"
#include <chrono>
#include <cmath>
#include <random>
#include <thread>

static int64_t now_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        high_resolution_clock::now().time_since_epoch()).count();
}

static const std::vector<std::string> TOKENS = {
    "<s>", "The", "quick", "brown", "fox", "jumps", "over",
    "the", "lazy", "dog", ".", "</s>", "Hello", "world", "!"
};

void MockModel::start() {
    running_ = true;
    thread_ = std::thread(&MockModel::run, this);
}

void MockModel::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void MockModel::run() {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> latency_dist(0.5f, 15.0f);
    std::uniform_real_distribution<float> sparsity_dist(0.30f, 0.95f);
    std::uniform_real_distribution<float> mean_dist(-0.1f, 0.1f);
    std::uniform_real_distribution<float> max_dist(2.0f, 8.0f);
    std::uniform_real_distribution<float> min_dist(-8.0f, -2.0f);
    std::uniform_real_distribution<float> stddev_dist(0.5f, 2.0f);
    std::uniform_int_distribution<int>    anomaly_roll(0, 20);
    std::uniform_int_distribution<int>    seq_dist(8, 15);
    uint64_t packet_id = 0;

    // Build topology: embed + 32 * (attn + mlp + layernorm)
    struct LayerDef { std::string name; LayerType type; };
    std::vector<LayerDef> topology;
    topology.push_back({"embed_tokens", LayerType::Embedding});
    for (int i = 0; i < 32; ++i) {
        topology.push_back({"layer." + std::to_string(i) + ".self_attn", LayerType::AttentionSelf});
        topology.push_back({"layer." + std::to_string(i) + ".mlp",       LayerType::MLP_SwiGLU});
        topology.push_back({"layer." + std::to_string(i) + ".layernorm", LayerType::LayerNorm});
    }

    while (running_) {
        for (const auto& def : topology) {
            if (!running_) break;

            int seq_len = seq_dist(rng);
            float latency = latency_dist(rng);
            float sparsity = sparsity_dist(rng);
            float mean_act   = mean_dist(rng);
            float max_act    = max_dist(rng);
            float min_act    = min_dist(rng);
            float stddev_act = stddev_dist(rng);
            DeviceType dev = DeviceType::CUDA;
            int gpu_idx = 0;

            // Occasional OOM fallback
            bool oom = (anomaly_roll(rng) == 0);
            if (oom) {
                dev = DeviceType::CPU;
                gpu_idx = -2; // sentinel for OOM
            }

            if (anomaly_roll(rng) == 1) { max_act = 12.0f + latency_dist(rng); min_act = -max_act; }

            LayerPacket pkt;
            pkt.id             = packet_id++;
            pkt.timestamp_us   = now_us();
            pkt.layer_name     = def.name;
            pkt.layer_type     = def.type;
            pkt.device         = dev;
            pkt.gpu_index      = gpu_idx;
            pkt.shape          = {1, seq_len, 4096};
            pkt.dtype          = "bfloat16";
            pkt.latency_ms       = latency;
            pkt.sparsity_rate    = sparsity;
            pkt.mean_activation  = mean_act;
            pkt.max_activation   = max_act;
            pkt.min_activation   = min_act;
            pkt.stddev_activation = stddev_act;

            // Attention matrix for attention layers
            if (def.type == LayerType::AttentionSelf) {
                pkt.attention_weights.resize(seq_len * seq_len);
                std::uniform_real_distribution<float> w(0.0f, 1.0f);
                float row_sum = 0;
                for (int r = 0; r < seq_len; ++r) {
                    row_sum = 0;
                    for (int c = 0; c < seq_len; ++c) {
                        float v = (c <= r) ? w(rng) : 0.0f; // causal mask
                        pkt.attention_weights[r * seq_len + c] = v;
                        row_sum += v;
                    }
                    if (row_sum > 0)
                        for (int c = 0; c <= r; ++c)
                            pkt.attention_weights[r * seq_len + c] /= row_sum;
                }
                pkt.tokens.assign(TOKENS.begin(), TOKENS.begin() + seq_len);
            }

            callback_(std::move(pkt));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
