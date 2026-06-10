#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include "../../include/types.hpp"

// Non-invasive hook pattern: wrap any forward-pass call with RAII scope guards.
// In a real deployment, the shared library (LD_PRELOAD / DYLD_INSERT_LIBRARIES)
// would replace torch op symbols; here we expose the same interface used by the mock.

using PacketCallback = std::function<void(LayerPacket)>;

class LayerInterceptor {
public:
    explicit LayerInterceptor(PacketCallback cb) : callback_(std::move(cb)) {}

    // Call before entering a layer's forward pass.
    // Returns an opaque handle (start timestamp).
    int64_t enter(const std::string& layer_name, LayerType type,
                  DeviceType device, int gpu_index, TensorShape shape,
                  const std::string& dtype);

    // Call after the forward pass completes.
    // Computes latency, assembles the packet, fires callback.
    void exit(int64_t handle, float mean_act, float max_act,
              float sparsity, std::vector<float> attn_weights,
              std::vector<std::string> tokens);

private:
    struct PendingEntry {
        uint64_t    id;
        std::string layer_name;
        LayerType   type;
        DeviceType  device;
        int         gpu_index;
        TensorShape shape;
        std::string dtype;
        int64_t     start_us;
    };

    PacketCallback callback_;
    uint64_t       next_id_{0};
    std::unordered_map<int64_t, PendingEntry> pending_;
};

// RAII guard — wraps a single layer forward pass.
class ScopeHook {
public:
    ScopeHook(LayerInterceptor& interceptor,
              const std::string& name, LayerType type,
              DeviceType device, int gpu_index,
              TensorShape shape, const std::string& dtype)
        : interceptor_(interceptor)
    {
        handle_ = interceptor_.enter(name, type, device, gpu_index, shape, dtype);
    }

    void finish(float mean, float max_act, float sparsity,
                std::vector<float> attn = {}, std::vector<std::string> toks = {}) {
        interceptor_.exit(handle_, mean, max_act, sparsity,
                          std::move(attn), std::move(toks));
        fired_ = true;
    }

    ~ScopeHook() {
        if (!fired_) interceptor_.exit(handle_, 0, 0, 0, {}, {});
    }

private:
    LayerInterceptor& interceptor_;
    int64_t           handle_;
    bool              fired_{false};
};
