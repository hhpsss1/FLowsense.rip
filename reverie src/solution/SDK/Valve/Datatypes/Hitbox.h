#pragma once
#include "../../../Utils/Math/Math.h"
#include <cstddef>
#include <cstdint>

struct CHitbox;

struct CHitboxDescriptor {
	CHitbox* hitbox;
	int mesh_index;
	int padding;
};

struct alignas(16) CHitboxTransformList {
	int count = 0;
	char pad0004[12] = {};
	const Matrix2x4_t* transforms = nullptr;
	int capacity = 0;
	uint32_t flags = 0xC0000000;
};

static_assert(sizeof(CHitboxDescriptor) == 0x10);
static_assert(offsetof(CHitboxTransformList, transforms) == 0x10);
static_assert(sizeof(CHitboxTransformList) == 0x20);
