#include "detector.hpp"

void AnomalyDetector::inspect(const LayerPacket& pkt) {
    if (pkt.max_activation > 8.0f)
        emit(pkt.timestamp_us, "warning", "OUTLIER", pkt.layer_name,
             "max=" + std::to_string(pkt.max_activation).substr(0, 7), pkt.id);

    if (pkt.sparsity_rate > 0.90f)
        emit(pkt.timestamp_us, "warning", "SPARSE", pkt.layer_name,
             std::to_string(static_cast<int>(pkt.sparsity_rate * 100)) + "%", pkt.id);

    if (pkt.latency_ms > 10.0f)
        emit(pkt.timestamp_us, "error", "SLOW", pkt.layer_name,
             std::to_string(pkt.latency_ms).substr(0, 7) + " ms", pkt.id);

    if (pkt.device == DeviceType::CPU && pkt.gpu_index == -2)
        emit(pkt.timestamp_us, "error", "CPU_FALLBACK", pkt.layer_name,
             "executed on CPU", pkt.id);
}

void AnomalyDetector::emit(int64_t ts, const std::string& severity,
                           const std::string& category, const std::string& layer,
                           const std::string& detail, uint64_t packet_id) {
    AnomalyEvent e;
    e.timestamp_us = ts;
    e.severity     = severity;
    e.category     = category;
    e.layer_name   = layer;
    e.detail       = detail;
    e.packet_id    = packet_id;
    log_.push(e);
}
