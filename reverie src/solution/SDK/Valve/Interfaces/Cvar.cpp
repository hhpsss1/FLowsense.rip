#include <shared.h>

bool CConVar::GetBool() const { return value.i1; }
int CConVar::GetInt() const { return value.i32; }
float CConVar::GetFloat() const { return value.fl; }
const char* CConVar::GetString() const { return value.sz; }

bool CConVar::SetBool(bool value) { return this->value.i1 = value; }
int CConVar::SetInt(int value) { return this->value.i32 = value; }
float CConVar::SetFloat(float value) { return this->value.fl = value; }
const char* CConVar::SetString(const char* value) { return this->value.sz = value; }

CConVar* CCvar::FindByName(const char* name) const { return name ? this->FindByHash(FnvHash(name)) : nullptr; }

CConVar* CCvar::FindByHash(uint64_t hash) const {
	if (!this->entries)
		return nullptr;

	for (uint16_t i = 0; i != 0xFFFF; i = this->entries[i].next) {
		auto convar = this->entries[i].convar;
		if (convar && convar->name && FnvHash(convar->name) == hash)
			return convar;
	}
	return nullptr;
}
