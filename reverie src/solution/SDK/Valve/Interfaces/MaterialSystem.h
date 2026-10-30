#pragma once

struct CMaterialParameter {
	Vector4D vec;
	void* texture_value;
	char pad[0x10];
	const char* parameter_name;
	const char* value_text;
	int64_t value;
};

struct CMaterial2 {
	virtual const char* GetName() = 0;
	virtual const char* GetNameWithMod() = 0;
	virtual void Func2() = 0;
	virtual void Func3() = 0;
	virtual bool IsLoaded() = 0;

	CMaterialParameter* FindParameter(const char* name) {
		using fn_find_parameter = CMaterialParameter*(__fastcall*)(CMaterial2*, const char*);
		static const auto find_parameter = Memory->Modules.Materialsystem.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 59 ? 48 8B F2 48 63 79 ? 48 C1 E7 ? 48 03 FB 48 3B DF")).As<fn_find_parameter>();
		return find_parameter(this, name);
	}

	void* vmt;
	char pad0[0x10];
	int parameter_count;
	char pad1[0x04];
	CMaterialParameter* parameters;
	char pad2[0x3F0];
	int num_renderables_to_draw;
	char pad3[0x04];
	void* mesh_draw_primitives;
};

struct CMaterialSystem {
	void CreateMaterial(CStrongHandle<CMaterial2>* out_mat, const char* material_name, CKeyValues3* key_values) { return Memory->Vtable.Execute<void>(this, 29, out_mat, material_name, key_values, 0, 0, 0, 0, 0, 1); }
};