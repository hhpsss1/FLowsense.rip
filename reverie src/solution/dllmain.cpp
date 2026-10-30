#include <shared.h>

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(module);

		if (HANDLE thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Entry_t::Fill), module, 0, nullptr))
			CloseHandle(thread);
	} else if (reason == DLL_PROCESS_DETACH) {
		
	}

	return TRUE;
}