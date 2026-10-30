#pragma once

struct CNetworkGameClient {
	int GetServerTick() {
		return Memory->Vtable.Execute<int>(this, 8);
	}
};

struct CNetworkClientService {
	CNetworkGameClient* GetClient() {
		return Memory->Vtable.Execute<CNetworkGameClient*>(this, 23);
	}
};
