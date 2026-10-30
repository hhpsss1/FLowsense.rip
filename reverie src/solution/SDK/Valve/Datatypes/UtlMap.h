#pragma once

template <typename k, typename v>

struct CUtlMap {
	struct Node_t {
		int left;
		int right;
		int parent;
		int tag;
		k key;
		v value;
	};

	auto Begin() const { return data; }
	auto End() const { return data + size; }
	auto begin() const { return this->Begin(); }
	auto end() const { return this->End(); }

	int size;
	int unknown;
	Node_t* data;
	int root;
};