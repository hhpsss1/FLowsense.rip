#pragma once

namespace Fnv1a {
	inline constexpr uint32_t Hash32(const char* const str, const uint32_t value = 0x811c9dc5) noexcept {
		return (str[0] == '\0') ? value : Hash32(&str[1], (value ^ uint32_t((uint8_t)str[0])) * 0x1000193);
	}

	inline constexpr uint64_t Hash64(const char* const str, const uint64_t value = 0xcbf29ce484222325) noexcept {
		return (str[0] == '\0') ? value : Hash64(&str[1], (value ^ uint64_t((uint8_t)str[0])) * 0x100000001b3);
	}
}

#define FnvHash(str) Fnv1a::Hash64(str)