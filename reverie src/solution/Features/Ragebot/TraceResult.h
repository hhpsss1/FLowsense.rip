#pragma once

#include <cstdint>

struct CHitbox;

namespace RageTrace {

struct Impact {
	const CHitbox* hitbox = nullptr;
	float damage = 0.f;
	bool penetrated = false;
};

template <typename TraceData, typename NativeDecoder>
Impact ReadTargetImpact(const TraceData& data, uint32_t target_identity, NativeDecoder&& decode) {
	bool penetrated = false;
	for (int i = 0; i < data.trace_info.size; ++i) {
		const auto& info = data.trace_info[i];
		if (info.damage <= 0.f)
			return {};
		if (info.IsPenetrationSegment()) {
			penetrated = true;
			continue;
		}

		const uint32_t index = info.handle >> 16;
		if (data.filled_segment_count <= 0 || index >= static_cast<uint32_t>(data.filled_segment_count))
			return {};
		const auto& segment = data.trace_segments[index];
		const CHitbox* hitbox;
		if (segment.type == 1 && target_identity < UINT32_MAX - 1) {
			if (segment.entity_handle != target_identity)
				continue;
			hitbox = reinterpret_cast<const CHitbox*>(segment.body);
		} else {
			hitbox = decode(info);
		}
		if (hitbox)
			return {hitbox, info.damage, penetrated};
	}
	return {};
}

}
