#include "serializer.hpp"
#include <fstream>
#include <cstring>

static constexpr uint32_t MAGIC   = 0x4C4C4D54;
static constexpr uint32_t VERSION = 1;

template<typename T>
static void write_pod(std::ofstream& f, const T& v) {
    f.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

template<typename T>
static T read_pod(std::ifstream& f) {
    T v{};
    f.read(reinterpret_cast<char*>(&v), sizeof(T));
    return v;
}

static void write_str(std::ofstream& f, const std::string& s) {
    uint32_t len = static_cast<uint32_t>(s.size());
    write_pod(f, len);
    f.write(s.data(), len);
}

static std::string read_str(std::ifstream& f) {
    uint32_t len = read_pod<uint32_t>(f);
    std::string s(len, '\0');
    f.read(s.data(), len);
    return s;
}

bool serialize_packets(const std::string& path, const std::vector<LayerPacket>& packets) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    write_pod(f, MAGIC);
    write_pod(f, VERSION);
    write_pod(f, static_cast<uint64_t>(packets.size()));

    for (const auto& p : packets) {
        write_pod(f, p.id);
        write_pod(f, p.timestamp_us);
        write_pod(f, static_cast<int32_t>(p.layer_type));
        write_pod(f, static_cast<int32_t>(p.device));
        write_pod(f, p.gpu_index);
        write_pod(f, p.shape.batch);
        write_pod(f, p.shape.seq_len);
        write_pod(f, p.shape.hidden_dim);
        write_pod(f, p.latency_ms);
        write_pod(f, p.sparsity_rate);
        write_pod(f, p.mean_activation);
        write_pod(f, p.max_activation);
        write_pod(f, p.min_activation);
        write_pod(f, p.stddev_activation);
        write_str(f, p.layer_name);
        write_str(f, p.dtype);
        write_pod(f, static_cast<uint32_t>(p.attention_weights.size()));
        for (float w : p.attention_weights) write_pod(f, w);
        write_pod(f, static_cast<uint32_t>(p.tokens.size()));
        for (const auto& t : p.tokens) write_str(f, t);
    }
    return true;
}

std::vector<LayerPacket> deserialize_packets(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};

    uint32_t magic = read_pod<uint32_t>(f);
    if (magic != MAGIC) return {};
    read_pod<uint32_t>(f); // version
    uint64_t count = read_pod<uint64_t>(f);

    std::vector<LayerPacket> packets;
    packets.reserve(count);
    for (uint64_t i = 0; i < count; ++i) {
        LayerPacket p;
        p.id              = read_pod<uint64_t>(f);
        p.timestamp_us    = read_pod<int64_t>(f);
        p.layer_type      = static_cast<LayerType>(read_pod<int32_t>(f));
        p.device          = static_cast<DeviceType>(read_pod<int32_t>(f));
        p.gpu_index       = read_pod<int>(f);
        p.shape.batch     = read_pod<int>(f);
        p.shape.seq_len   = read_pod<int>(f);
        p.shape.hidden_dim= read_pod<int>(f);
        p.latency_ms      = read_pod<float>(f);
        p.sparsity_rate   = read_pod<float>(f);
        p.mean_activation  = read_pod<float>(f);
        p.max_activation   = read_pod<float>(f);
        p.min_activation   = read_pod<float>(f);
        p.stddev_activation= read_pod<float>(f);
        p.layer_name       = read_str(f);
        p.dtype           = read_str(f);
        uint32_t aw_count = read_pod<uint32_t>(f);
        p.attention_weights.resize(aw_count);
        for (auto& w : p.attention_weights) w = read_pod<float>(f);
        uint32_t tok_count = read_pod<uint32_t>(f);
        p.tokens.resize(tok_count);
        for (auto& t : p.tokens) t = read_str(f);
        packets.push_back(std::move(p));
    }
    return packets;
}
