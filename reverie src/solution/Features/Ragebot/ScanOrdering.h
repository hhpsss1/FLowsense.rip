#pragma once
#include <span>
#include <utility>

namespace RageScan {
	template <typename T, size_t Extent, typename Compare>
	void StableInsertionSort(std::span<T, Extent> values, Compare less) {
		for (size_t index = 1; index < values.size(); ++index) {
			T value = std::move(values[index]);
			size_t destination = index;
			while (destination > 0 && less(value, values[destination - 1])) {
				values[destination] = std::move(values[destination - 1]);
				--destination;
			}
			values[destination] = std::move(value);
		}
	}
}
