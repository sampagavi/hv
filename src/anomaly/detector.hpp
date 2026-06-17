#pragma once
#include "../../include/types.hpp"
#include "../buffer/ring_buffer.hpp"

static constexpr size_t ANOMALY_BUFFER_CAPACITY = 128;

class AnomalyDetector {
public:
    explicit AnomalyDetector(RingBuffer<AnomalyEvent, ANOMALY_BUFFER_CAPACITY>& log)
        : log_(log) {}

    void inspect(const LayerPacket& pkt);

private:
    RingBuffer<AnomalyEvent, ANOMALY_BUFFER_CAPACITY>& log_;

    void emit(int64_t ts, const std::string& severity,
              const std::string& category, const std::string& layer,
              const std::string& detail, uint64_t packet_id);
};
