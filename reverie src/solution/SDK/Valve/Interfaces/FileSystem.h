#pragma once

struct CFileSystem {
	void Close(void* file_handle) { Memory->Vtable.Execute<void>(this, 14, file_handle); }
	uint32_t GetSize(void* file_handle) { return Memory->Vtable.Execute<uint32_t>(this, 18, file_handle); }
	bool Exists(const char* path) { return Memory->Vtable.Execute<bool>(this, 21, path, xs("game")); }
	void* Open(const char* path, const char* mode) { return Memory->Vtable.Execute<void*>(this, 75, path, mode, 0, xs("game")); }
	void Read(uint8_t* buffer, uint32_t file_size, void* file_handle) { Memory->Vtable.Execute<void>(this, 76, buffer, file_size, file_size, file_handle); }

	std::vector<uint8_t> ReadRaw(const char* path) {
		void* file_handle = this->Open(path, xs("rb"));
		if (!file_handle)
			return {};

		uint32_t file_size = this->GetSize(file_handle);
		if (file_size == 0)
			return {};

		std::vector<uint8_t> buffer(file_size);

		this->Read(buffer.data(), file_size, file_handle);
		this->Close(file_handle);

		return buffer;
	}
};