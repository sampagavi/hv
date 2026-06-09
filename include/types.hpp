#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class LayerType { Embedding, AttentionSelf, MLP_SwiGLU, LayerNorm, Unknown };
enum class DeviceType { CPU, CUDA };

struct TensorShape {
    int batch, seq_len, hidden_dim;
};

struct LayerPacket {
    uint64_t    id;
    int64_t     timestamp_us;
    std::string layer_name;
    LayerType   layer_type;
    DeviceType  device;
    int         gpu_index;
    TensorShape shape;
    std::string dtype;
    float       latency_ms;
    float       sparsity_rate;
    float       mean_activation;
    float       max_activation;
    float       min_activation;
    float       stddev_activation;
    std::vector<float>       attention_weights;
    std::vector<std::string> tokens;
};

struct AnomalyEvent {
    int64_t     timestamp_us;
    std::string severity;   // "error" | "warning" | "info"
    std::string category;   // "SLOW" | "OUTLIER" | "OOM" | "SPARSE" | "CPU_FALLBACK"
    std::string layer_name;
    std::string detail;
    uint64_t    packet_id;
};
