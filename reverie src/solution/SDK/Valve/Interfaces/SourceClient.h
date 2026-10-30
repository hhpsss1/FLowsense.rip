#pragma once

struct CSource2Client {
	CEconItemSystem* GetEconItemSystem() { return  Memory->Vtable.Execute<CEconItemSystem*>(this, 129); }
};