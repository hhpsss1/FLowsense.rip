#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

struct CSceneWorld {
	uint8_t pad[0x98];
	const char* scene_name;
};

struct CAggregateSceneObjectData2 {
	char pad0[0x4];
	int count;
	char pad1[0x30];
	int index;
};

struct CAggregateSceneObjectData {
	char pad0[0x38];
	Color_t color;
	char pad1[0x9];
};

struct CAggregateSceneObject {
	char pad0[0x120];
	int count;
	char pad1[0x4];
	CAggregateSceneObjectData* data;
};

struct CAggregateObject {
	CAggregateSceneObject* scene_object;
	CAggregateSceneObjectData2* scene_object_data;
};

struct CLightScene {
	char pad0[0xB8];
	CSceneWorld* scene_world;
	char pad1[0x24];
	ImColor color;
};

struct CRenderViewport {
	const char* image_path{ nullptr };
	int width{ -1 };
	int height{ -1 };
	int unk1{ -1 };
	int unk2{ -1 };
	float scale{ 1.333f };
	char pad0[0x30]{};
	int unk3{ 1 };
	char pad1[0x18]{};
	int unk4{ 1 };
	char pad2[0x2C]{};
};

struct CTextureDX11 {
	char pad0[8];
	int32_t unk;
	char pad1[4];
	ID3D11ShaderResourceView* srv0;
	ID3D11ShaderResourceView* srv1;
};

struct CTextureDX11Handle {
	CTextureDX11* texture;
	void* scratch_renderer;
	char pad[24];
	void* material_list;
};

struct CSceneLayer {
	void* pointer;
	char pad0[8];
	CRenderViewport* viewport;
	int32_t layer_type;
	int32_t shader_mode;
	int32_t shading_mode;
	uint32_t object_flags_required_mask;
	uint32_t object_flags_excluded_mask;
	uint32_t layer_flags;
	Vector4D clear_color;
	int32_t clear_flags;
	int32_t layer_index;
	int32_t rendering_target_binding_handle;
	char pad1[160];
	float width;
	float height;
	char pad2[760];
	void* vertex_buffer1;
	void* vertex_buffer2;
	void* vertex_buffer3;
	char pad3[136];
	char layer_name[64];
	char pad4[760];
	CTextureDX11Handle* texture_handle1;
	char pad5[56];
	CTextureDX11Handle* texture_handle2;
	char pad6[5760];

	std::uint32_t GetMaterialMode() const {
		std::uint32_t mode;
		std::memcpy(&mode, reinterpret_cast<const std::byte*>(this) + 0x30, sizeof(mode));
		return mode;
	}

	const void* GetMaterialModeData() const { return reinterpret_cast<const std::byte*>(this) + 0x30; }

	std::uint64_t GetNativeFlags() const {
		std::uint64_t flags;
		std::memcpy(&flags, reinterpret_cast<const std::byte*>(this) + 0x48, sizeof(flags));
		return flags;
	}

	const char* GetNativeName() const { return reinterpret_cast<const char*>(this) + 0x4B8; }
};

struct CSceneObject {
	void* pointer;
	char pad0[0x8];
	void* mesh_instance;
	void* desc;
	void* ref_data;
	void* entity_ptr;
	Matrix3x4_t transform;
	char pad1[0x18];
	uint32_t flags;
	char pad2[0x14];
	void* ptr1;
	char pad3[0x8];
	void* ptr2;
	uint64_t renderable_flags;
	void* ptr3;
	void* scene_world;
	CBaseHandle owner;
	char pad4[0xC];

	void DisablePVS() { this->flags &= ~(1 << 3); }
	const void* GetPvsClusters() const { return this->ptr2; }
};

static_assert(offsetof(CSceneObject, flags) == 0x78);
static_assert(offsetof(CSceneObject, ptr2) == 0xA0);
static_assert(offsetof(CSceneObject, owner) == 0xC0);

struct CSceneVisibilityRecord {
	Vector4D bounds_min;
	Vector4D bounds_max;
	std::uint64_t flags;
	CSceneObject* object;
};

struct CSceneVisibilityList {
	std::uint32_t count;
	char pad[4];
	CSceneVisibilityRecord* records;
	const void* pvs;

	const std::uint32_t* GetPvsBitmap() const {
		return this->pvs ? reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(this->pvs) + 8) : nullptr;
	}
};

static_assert(sizeof(CSceneVisibilityRecord) == 0x30);
static_assert(offsetof(CSceneVisibilityRecord, object) == 0x28);
static_assert(sizeof(CSceneVisibilityList) == 0x18);
static_assert(offsetof(CSceneVisibilityList, records) == 8);
static_assert(offsetof(CSceneVisibilityList, pvs) == 0x10);

struct CUnkData {
	FindOffset(const char*, panel_name, 0x98);
};

struct CSceneAnimtableObject : CSceneObject {
	CUtlVector<Matrix3x4_t> world_space_render_bones;
	CUtlVector<float> flex_controller_weights;
	int vertex_bone_influence_count;
	char pad0[0xC];
	void* skeleton_instance;
	char pad1[0x10];
	void* ptr3;
	char pad2[0x150];

	FindOffset(int, GetBoneCount, 0xD0);
	FindOffset(Matrix3x4_t*, GetRenderBones, 0xD8);
	FindOffset(uint32_t, Flags, 0x78);
	FindOffset(char&, LodFlag, 0x8E);
	FindOffset(CUnkData*, UnkData, 0xB8);
};

struct CSceneAnimtableObjectDesc {
	CSceneAnimtableObject* Create() { return Memory->Vtable.Execute<CSceneAnimtableObject*>(this, 21); }
};

struct CMeshPrimitive {
	void* data;
	char pad0[0x10];
	CSceneAnimtableObject* scene_animtable_object;
	CMaterial2* material;
	CMaterial2* material2;
	char pad1[0x10];
	void* some_data;
	char pad2[0x8];
	Color_t color;
	char pad3[0x1C];
};

static_assert(sizeof(CMeshPrimitive) == 0x70);
static_assert(offsetof(CMeshPrimitive, material) == 0x20);
static_assert(offsetof(CMeshPrimitive, material2) == 0x28);
static_assert(offsetof(CMeshPrimitive, color) == 0x50);

struct CMeshPrimitivePage {
	int count;
	char pad[4];
	CMeshPrimitive entries[256];
};

struct CMeshPrimitivePageList {
	int count;
	char pad[4];
	CMeshPrimitivePage** pages;
};

static_assert(offsetof(CMeshPrimitivePage, entries) == 0x8);
static_assert(offsetof(CMeshPrimitivePageList, pages) == 0x8);

struct CMeshPrimitiveOutputVector {
	int count;
	char pad[4];
	CMeshPrimitive* data;
	int capacity;
	std::uint32_t allocation_flags;
};

static_assert(sizeof(CMeshPrimitiveOutputVector) == 0x18);
static_assert(offsetof(CMeshPrimitiveOutputVector, data) == 0x8);
static_assert(offsetof(CMeshPrimitiveOutputVector, capacity) == 0x10);
static_assert(offsetof(CMeshPrimitiveOutputVector, allocation_flags) == 0x14);

struct CMeshPrimitiveOutputBuffer {
	CMeshPrimitive* mesh_primitive_array;
	int capacity;
	int count;
	CMeshPrimitiveOutputVector overflow;
	CMeshPrimitive overflow_inline_storage[20];
	CMeshPrimitiveOutputVector* duplicate_dispatch;
	int duplicate_dispatch_start_count;

	struct Cursor {
		const CMeshPrimitiveOutputBuffer* owner{};
		CMeshPrimitive* primary_data{};
		int primary_capacity{};
		int primary_count{};
		int overflow_count{};
		CMeshPrimitiveOutputVector* duplicate_vector{};
		int duplicate_count{};
		bool valid{};
	};

	CMeshPrimitive* GetPrimitive(int index) const {
		return this->mesh_primitive_array && index >= 0 && index < this->count && this->count <= this->capacity
			? &this->mesh_primitive_array[index] : nullptr;
	}

	Cursor Snapshot() const {
		Cursor cursor;
		cursor.owner = this;
		cursor.primary_data = this->mesh_primitive_array;
		cursor.primary_capacity = this->capacity;
		cursor.primary_count = this->count;
		cursor.overflow_count = this->overflow.count;
		cursor.duplicate_vector = this->duplicate_dispatch;
		cursor.duplicate_count = this->duplicate_dispatch ? this->duplicate_dispatch->count : 0;
		cursor.valid = ValidRange(this->mesh_primitive_array, this->count, this->capacity)
			&& ValidRange(this->overflow.data, this->overflow.count, this->overflow.capacity)
			&& (!this->duplicate_dispatch || ValidRange(this->duplicate_dispatch->data, this->duplicate_dispatch->count, this->duplicate_dispatch->capacity));
		return cursor;
	}

	template <typename Visitor>
	bool VisitSince(const Cursor& cursor, Visitor&& visitor) const {
		if (!cursor.valid || cursor.owner != this || cursor.primary_data != this->mesh_primitive_array
			|| cursor.primary_capacity != this->capacity || cursor.duplicate_vector != this->duplicate_dispatch
			|| cursor.primary_count < 0 || cursor.primary_count > this->count
			|| cursor.overflow_count < 0 || cursor.overflow_count > this->overflow.count
			|| !ValidRange(this->mesh_primitive_array, this->count, this->capacity)
			|| !ValidRange(this->overflow.data, this->overflow.count, this->overflow.capacity))
			return false;

		const int duplicate_count = this->duplicate_dispatch ? this->duplicate_dispatch->count : 0;
		if (cursor.duplicate_count < 0 || cursor.duplicate_count > duplicate_count
			|| (this->duplicate_dispatch && !ValidRange(this->duplicate_dispatch->data, duplicate_count, this->duplicate_dispatch->capacity)))
			return false;

		for (int i = cursor.primary_count; i < this->count; ++i)
			visitor(&this->mesh_primitive_array[i]);
		for (int i = cursor.overflow_count; i < this->overflow.count; ++i)
			visitor(&this->overflow.data[i]);
		for (int i = cursor.duplicate_count; i < duplicate_count; ++i)
			visitor(&this->duplicate_dispatch->data[i]);
		return true;
	}

private:
	static bool ValidRange(const CMeshPrimitive* data, int count, int capacity) {
		return count >= 0 && capacity >= 0 && count <= capacity && (capacity == 0 || data);
	}
};

static_assert(offsetof(CMeshPrimitiveOutputBuffer, capacity) == 0x8);
static_assert(offsetof(CMeshPrimitiveOutputBuffer, count) == 0xC);
static_assert(offsetof(CMeshPrimitiveOutputBuffer, overflow) == 0x10);
static_assert(offsetof(CMeshPrimitiveOutputBuffer, overflow_inline_storage) == 0x28);
static_assert(offsetof(CMeshPrimitiveOutputBuffer, duplicate_dispatch) == 0x8E8);
static_assert(offsetof(CMeshPrimitiveOutputBuffer, duplicate_dispatch_start_count) == 0x8F0);
static_assert(sizeof(CMeshPrimitiveOutputBuffer) == 0x8F8);

struct CLightData {
	char pad[0x18];
	void* light;
};

struct CSkyboxObjectDesc {
	char pad000[0xD0];
	void** material;
	char pad001[0x10];
	Color4_f color;
};

struct CSceneView {
	uint8_t pad[0xCC8];
	CSceneWorld* scene_world;
};

struct CSceneSystem {
	char pad[0x1F08];
	CLightData* data;

	void* GetObject(const char* name) { return Memory->Vtable.Execute<void*>(this, 17, name); }
	void DeleteObject(void* object) { Memory->Vtable.Execute<void>(this, 94, object); }
};
