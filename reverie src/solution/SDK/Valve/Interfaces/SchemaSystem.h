#pragma once

struct CSchemaClassField {
	const char* name;
	void* type;
	uint32_t offset;
	uint32_t metadata_size;
	void* metadata;
};

struct CSchemaClassInfo {
	void* pointer;
	const char* name;
	const char* module_name;
private:
	char pad0[0x8];
public:
	uint32_t size;
	uint16_t num_fields;
private:
	char pad1[0x2];
public:
	uint16_t static_size;
	uint16_t metadata_size;
private:
	char pad2[0x4];
public:
	CSchemaClassField* fields;
};

struct CSchemaDeclaredClass {
	void* pointer;
	const char* name;
	const char* module_name;
	const char* unk;
	CSchemaClassInfo* class_info;
};

struct CSchemaDeclaredClassEntry {
	uint64_t hash[2];
	CSchemaDeclaredClass* declared_class;
};

struct CSchemaTypeScope {
	void* pointer;
	char name[256];
private:
	char pad[0x368];
public:
	int16_t num_declared_classes;
	int16_t unk_count;
	int32_t unk_flags;
	CSchemaDeclaredClassEntry* declared_classes;
};

struct CSchemaSystem {
	CSchemaTypeScope* FindTypeScopeForModule(const char* module) { return Memory->Vtable.Execute<CSchemaTypeScope*>(this, 13, module, nullptr); }
};