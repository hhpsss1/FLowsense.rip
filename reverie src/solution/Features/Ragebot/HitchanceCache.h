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
		uint32_t flags = 0; // bit0 = penetrated, bit1 = multipoint

		bool operator==(const Key&) const = default;

		static Key Make(uint64_t record, uint64_t spread,
			const std::array<float, 3>& eye,
			const std::array<float, 3>& angle,
			float required, int group, float range,
			uint32_t flags = 0) {
			Key key;
			key.record = record;
			key.spread = spread;
			key.group = group;
			key.flags = flags;
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

		const uint64_t hash = Hash(key);
		const auto& bucket = buckets[hash & (buckets.size() - 1)];
		for (const auto& entry : bucket.entries) {
			if (entry.valid && entry.hash == hash && entry.key == key) {
				result = entry.result;
				return true;
			}
		}
		return false;
	}

	void Store(const Key& key, float result) {
		populated = true;

		const uint64_t hash = Hash(key);
		auto& bucket = buckets[hash & (buckets.size() - 1)];

		// 1) Обновляем существующую запись.
		for (auto& entry : bucket.entries) {
			if (entry.valid && entry.hash == hash && entry.key == key) {
				entry.result = result;
				entry.age = ++global_tick; // [FIX] обновляем возраст при обновлении
				return;
			}
		}

		// 2) Ищем свободный слот.
		Entry* victim = nullptr;
		for (auto& entry : bucket.entries) {
			if (!entry.valid) {
				victim = &entry;
				break;
			}
		}

		if (!victim) {
			// Все слоты заняты — вытесняем самую старую (LRU-подобно).
			victim = &bucket.entries[0];
			for (auto& entry : bucket.entries) {
				if (entry.age < victim->age)
					victim = &entry;
			}
		}

		victim->key = key;
		victim->hash = hash;
		victim->result = result;
		victim->valid = true;
		victim->age = ++global_tick;
	}

	void Clear() {
		if (populated)
			buckets = {};
		populated = false;
		// [FIX] global_tick не сбрасываем — монотонность возраста сохраняется,
		// не важно, что после Clear() первые age большие: записи всё равно пусты.
	}

private:
	struct Entry {
		Key key;
		uint64_t hash = 0;
		float result = 0.f;
		bool valid = false;
		uint64_t age = 0;
	};
	struct Bucket {
		std::array<Entry, 4> entries;
	};

	std::array<Bucket, 32> buckets{};
	bool populated = false;
	uint64_t global_tick = 0;

	static uint64_t Hash(const Key& key) {
		uint64_t hash = 0x9E3779B97F4A7C15ULL;

		auto mix = [&](uint64_t value) {
			hash ^= value + 0x9E3779B97F4A7C15ULL + (hash << 6) + (hash >> 2);
			hash *= 0xFF51AFD7ED558CCDULL;
			hash ^= hash >> 33;
			};

		mix(key.record);
		mix(key.spread);
		mix(static_cast<uint64_t>(key.group));  // [FIX] без сужения до uint32_t
		mix(static_cast<uint64_t>(key.flags));  // [FIX] flags в хэш
		for (auto input : key.inputs)
			mix(input);

		hash ^= hash >> 33;
		hash *= 0xC4CEB9FE1A85EC53ULL;
		hash ^= hash >> 33;
		return hash;
	}
};