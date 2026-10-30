#pragma once

struct Schema_t {
	uint32_t GetOffset(const char* module_name, const char* class_name, const char* field_name);
	bool FindDeclaredClass(CSchemaTypeScope* type_scope, const char* class_name, CSchemaDeclaredClass** out_class);
private:
	std::unordered_map<std::string, uint32_t> cache;
};

inline Schema_t* Schema = new Schema_t();

#define FindOffset(TYPE, NAME, OFFSET)                                                                 \
	[[nodiscard]] __inline std::add_lvalue_reference_t<TYPE> NAME()                                          \
	{                                                                                                         \
		static const std::uint32_t uOffset = OFFSET;                                                          \
		return *reinterpret_cast<std::add_pointer_t<TYPE>>(reinterpret_cast<std::uint8_t*>(this) + (uOffset)); \
	}

#define FindPOffset(TYPE, NAME, OFFSET)                                                               \
	[[nodiscard]] __inline std::add_pointer_t<TYPE> NAME()                                                  \
	{                                                                                                        \
		const static std::uint32_t uOffset = OFFSET;                                                         \
		return reinterpret_cast<std::add_pointer_t<TYPE>>(reinterpret_cast<std::uint8_t*>(this) + (uOffset)); \
	}

#define FindField(type, name, className, fieldName) \
type& name() { \
    static uint32_t offset = Schema->GetOffset(xs("client.dll"), className, fieldName); \
    return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset); \
}

#define FindFieldEx(type, name, className, fieldName, extraOffset) \
type& name() { \
    static uint32_t offset = Schema->GetOffset(xs("client.dll"), className, fieldName); \
    return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset + extraOffset); \
}

#define FindFieldPtr(type, name, className, fieldName)                                                      \
	[[nodiscard]] __inline type* name()                                                                      \
	{                                                                                                        \
		static const uint32_t offset = Schema->GetOffset(xs("client.dll"), className, fieldName);             \
		return reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset);                          \
	}

#define FindFieldArray(name, type, className, keyName)																	\
	type* name() {																									\
		static const short offset = Schema->GetOffset(xs("client.dll"), className, keyName);												\
																														\
		return reinterpret_cast<type*>(reinterpret_cast<unsigned __int64>(this) + offset);								\
	}
