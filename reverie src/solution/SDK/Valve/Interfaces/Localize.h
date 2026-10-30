#pragma once

struct CLocalize {
	const char* FindSafe(const char* t) { return Memory->Vtable.Execute<const char*>(this, 17, t); }
};