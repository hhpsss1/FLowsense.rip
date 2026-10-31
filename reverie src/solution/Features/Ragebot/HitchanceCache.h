#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

class CHitchanceCache {
public:
	struct Key {
		uint64_t record = 0;
		uint64_t spread = 0;
		std::array<uint32_t, 8> inputs{};
		int group = -1;
		bool operator==(const Key&) const = default;

		static Key Make(uint64_t record, uint64_t spread, const std::array<float, 3>& eye,
			const std::array<float, 3>& angle, float required, int group, float range) {
			Key key;
			key.record = record;
			key.spread = spread;
			key.group = group;
			for (size_t i = 0; i < 3; ++i) {
				key.inputs[i] = std::bit_cast<uint32_t>(eye[i]);
				key.inputs[i + 3] = std::bit_cast<uint32_t>(angle[i]);
			}
			key.inputs[6] = std::bit_cast<uint32_t>(required);
			key.inputs[7] = std::bit_cast<uint32_t>(range);
			return key;
		}
	};

	bool Find(const Key& key, float& result) const {
		if (!populated)
			return false;
		const auto hash = Hash(key);
		for (const auto& entry : buckets[hash & (buckets.size() - 1)].entries) {
			if (entry.valid && entry.hash == hash && entry.key == key) {
				result = entry.result;
				return true;
			}
		}
		return false;
	}

	void Store(const Key& key, float result) {
		populated = true;
		const auto hash = Hash(key);
		auto& bucket = buckets[hash & (buckets.size() - 1)];
		for (auto& entry : bucket.entries) {
			if (entry.valid && entry.hash == hash && entry.key == key) {
				entry.result = result;
				return;
			}
		}
		auto& entry = bucket.entries[bucket.next++ & (bucket.entries.size() - 1)];
		entry = {key, hash, result, true};
	}

	void Clear() {
		if (populated)
			buckets = {};
		populated = false;
	}

private:
	struct Entry {
		Key key;
		uint64_t hash = 0;
		float result = 0.f;
		bool valid = false;
	};
	struct Bucket {
		std::array<Entry, 4> entries;
		uint64_t next = 0;
	};
	std::array<Bucket, 32> buckets{};
	bool populated = false;

	static uint64_t Hash(const Key& key) {
		uint64_t hash = key.record * 0x9E3779B185EBCA87ULL + key.spread;
		for (auto input : key.inputs)
			hash = (hash ^ input) * 0x100000001B3ULL;
		hash ^= static_cast<uint32_t>(key.group);
		hash ^= hash >> 33;
		hash *= 0xFF51AFD7ED558CCDULL;
		return hash ^ (hash >> 33);
	}
};
