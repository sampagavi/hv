#include "interceptor.hpp"
#include <chrono>

static int64_t now_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        high_resolution_clock::now().time_since_epoch()).count();
}

int64_t LayerInterceptor::enter(const std::string& layer_name, LayerType type,
                                DeviceType device, int gpu_index,
                                TensorShape shape, const std::string& dtype) {
    // Store entry context keyed by start timestamp (serves as handle).
    // In a multi-threaded model, a thread-local map would be appropriate.
    int64_t ts = now_us();
    pending_.emplace(ts, PendingEntry{next_id_++, layer_name, type, device,
                                     gpu_index, shape, dtype, ts});
    return ts;
}

void LayerInterceptor::exit(int64_t handle, float mean_act, float max_act,
                            float sparsity, std::vector<float> attn_weights,
                            std::vector<std::string> tokens) {
    int64_t end_ts = now_us();
    auto it = pending_.find(handle);
    if (it == pending_.end()) return;
    auto& e = it->second;

    LayerPacket pkt;
    pkt.id               = e.id;
    pkt.timestamp_us     = e.start_us;
    pkt.layer_name       = e.layer_name;
    pkt.layer_type       = e.type;
    pkt.device           = e.device;
    pkt.gpu_index        = e.gpu_index;
    pkt.shape            = e.shape;
    pkt.dtype            = e.dtype;
    pkt.latency_ms       = static_cast<float>(end_ts - e.start_us) / 1000.0f;
    pkt.sparsity_rate    = sparsity;
    pkt.mean_activation  = mean_act;
    pkt.max_activation   = max_act;
    pkt.attention_weights = std::move(attn_weights);
    pkt.tokens           = std::move(tokens);

    pending_.erase(it);
    callback_(std::move(pkt));
}

