#pragma once

namespace MM {
	using Murmur2Hash_t = std::uint32_t;
    inline constexpr std::uint32_t Murmur2Seed = 0x31415926u;

#define _to_lower_u(c) (std::uint32_t)(((c >= 'A') && (c <= 'Z')) ? c + 32 : c)

    template <bool LowerCase = true, std::size_t N>
    [[nodiscard]] consteval Murmur2Hash_t HashA1Const(const char(&text)[N], Murmur2Hash_t seed) {
        const std::uint32_t m = 0x5bd1e995u;
        const int r = 24;

        MM::Murmur2Hash_t h = seed ^ (N - 1);

        std::size_t i = 0;
        while (i + 4 <= N - 1) {
            MM::Murmur2Hash_t k;
            if constexpr (LowerCase) {
                k = _to_lower_u(text[i]) | (_to_lower_u(text[i + 1]) << 8) | (_to_lower_u(text[i + 2]) << 16) | (_to_lower_u(text[i + 3]) << 24);
            } else {
                k = (MM::Murmur2Hash_t)text[i] | ((MM::Murmur2Hash_t)text[i + 1] << 8) | ((MM::Murmur2Hash_t)text[i + 2] << 16) | ((MM::Murmur2Hash_t)text[i + 3] << 24);
            }

            k *= m;
            k ^= k >> r;
            k *= m;

            h *= m;
            h ^= k;

            i += 4;
        }

        if constexpr (LowerCase) {
            switch ((N - 1) - i) {
            case 3: h ^= _to_lower_u(text[i + 2]) << 16; [[fallthrough]];
            case 2: h ^= _to_lower_u(text[i + 1]) << 8; [[fallthrough]];
            case 1: h ^= _to_lower_u(text[i]); h *= m;
            }
        } else {
            switch ((N - 1) - i) {
            case 3: h ^= (std::uint32_t)text[i + 2] << 16; [[fallthrough]];
            case 2: h ^= (std::uint32_t)text[i + 1] << 8; [[fallthrough]];
            case 1: h ^= (std::uint32_t)text[i]; h *= m;
            }
        }

        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return h;
    }

    template <bool LowerCase = true>
    MM::Murmur2Hash_t HashA1Runtime(const char *text, std::size_t len, MM::Murmur2Hash_t seed) {
        const std::uint32_t m = 0x5bd1e995u;
        const int r = 24;

        MM::Murmur2Hash_t h = seed ^ len;

        while (len >= 4) {
            MM::Murmur2Hash_t k;
            if constexpr (LowerCase) {
                k = _to_lower_u(text[0]) | (_to_lower_u(text[1]) << 8) | (_to_lower_u(text[2]) << 16) | (_to_lower_u(text[3]) << 24);
            } else {
                k = (MM::Murmur2Hash_t)text[0] | ((MM::Murmur2Hash_t)text[1] << 8) | ((MM::Murmur2Hash_t)text[2] << 16) | ((MM::Murmur2Hash_t)text[3] << 24);
            }

            k *= m;
            k ^= k >> r;
            k *= m;

            h *= m;
            h ^= k;

            text += 4;
            len -= 4;
        }

        if constexpr (LowerCase) {
            switch (len) {
            case 3: h ^= _to_lower_u(text[2]) << 16; [[fallthrough]];
            case 2: h ^= _to_lower_u(text[1]) << 8; [[fallthrough]];
            case 1: h ^= _to_lower_u(text[0]);
                h *= m;
            }
        } else {
            switch (len) {
            case 3: h ^= (MM::Murmur2Hash_t)text[2] << 16; [[fallthrough]];
            case 2: h ^= (MM::Murmur2Hash_t)text[1] << 8; [[fallthrough]];
            case 1: h ^= (MM::Murmur2Hash_t)text[0];
                h *= m;
            }
        }

        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return h;
    }
}