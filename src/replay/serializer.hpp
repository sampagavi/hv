#pragma once
/*
 * Binary format (little-endian):
 *   [4 bytes] magic: 0x4C4C4D54 "LLMT"
 *   [4 bytes] version: 1
 *   [8 bytes] packet_count: uint64_t
 *   For each packet:
 *     [8]  id
 *     [8]  timestamp_us
 *     [4]  layer_type (int32)
 *     [4]  device (int32)
 *     [4]  gpu_index
 *     [4]  shape.batch
 *     [4]  shape.seq_len
 *     [4]  shape.hidden_dim
 *     [4]  latency_ms (float)
 *     [4]  sparsity_rate (float)
 *     [4]  mean_activation (float)
 *     [4]  max_activation (float)
 *     [4]  layer_name length (uint32)
 *     [N]  layer_name bytes
 *     [4]  dtype length (uint32)
 *     [N]  dtype bytes
 *     [4]  attention_weights count (uint32)
 *     [4*N] attention_weights floats
 *     [4]  tokens count (uint32)
 *     For each token: [4] length, [N] bytes
 */
#include <string>
#include <vector>
#include "../../include/types.hpp"
#include "../buffer/ring_buffer.hpp"

bool serialize_packets(const std::string& path,
                       const std::vector<LayerPacket>& packets);

std::vector<LayerPacket> deserialize_packets(const std::string& path);
