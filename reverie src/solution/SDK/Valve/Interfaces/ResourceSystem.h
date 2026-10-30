#pragma once

struct CResourceSystem {
	CResourceBinding* BlockingLoadResourceByName(CBufferString* path, const char* name) { return Memory->Vtable.Execute<CResourceBinding*>(this, 40, path, name); }
};