#include <shared.h>

void Memory_t::Modules_t::Fill() {
	this->Client = Module_t(xs("client.dll"));
	this->Server = Module_t(xs("server.dll"));
	this->Engine2 = Module_t(xs("engine2.dll"));
	this->Schema = Module_t(xs("schemasystem.dll"));
	this->Input = Module_t(xs("inputsystem.dll"));
	this->Tier0 = Module_t(xs("tier0.dll"));
	this->Navsystem = Module_t(xs("navsystem.dll"));
	this->Rendersystem = Module_t(xs("rendersystemdx11.dll"));
	this->Localize = Module_t(xs("localize.dll"));
	this->Filesystem = Module_t(xs("filesystem_stdio.dll"));
	this->Meshsystem = Module_t(xs("meshsystem.dll"));
	this->Particles = Module_t(xs("particles.dll"));
	this->Panorama = Module_t(xs("panorama.dll"));
	this->Scenesystem = Module_t(xs("scenesystem.dll"));
	this->Materialsystem = Module_t(xs("materialsystem2.dll"));
	this->Resourcesystem = Module_t(xs("resourcesystem.dll"));
	this->Animationsystem = Module_t(xs("animationsystem.dll"));
	this->Soundsystem = Module_t(xs("soundsystem.dll"));
	this->Worldrenderer = Module_t(xs("worldrenderer.dll"));
}

void Memory_t::Modules_t::Clear() {
	this->Client = nullptr;
	this->Server = nullptr;
	this->Engine2 = nullptr;
	this->Schema = nullptr;
	this->Input = nullptr;
	this->Tier0 = nullptr;
	this->Navsystem = nullptr;
	this->Rendersystem = nullptr;
	this->Localize = nullptr;
	this->Filesystem = nullptr;
	this->Meshsystem = nullptr;
	this->Particles = nullptr;
	this->Panorama = nullptr;
	this->Scenesystem = nullptr;
	this->Materialsystem = nullptr;
	this->Resourcesystem = nullptr;
	this->Animationsystem = nullptr;
	this->Soundsystem = nullptr;
	this->Worldrenderer = nullptr;
}

std::vector<int> Module_t::ToBytes(const std::string_view& pattern) {
	std::vector<int> bytes{};

	char* start = const_cast<char*>(pattern.data());
	char* end = const_cast<char*>(pattern.data()) + pattern.size();

	for (char* current = start; current < end; ++current) {
		if (*current == '?') {
			++current;

			if (*current == '?')
				++current;

			bytes.push_back(-1);
		} else {
			bytes.push_back(strtoul(current, &current, 16));
		}
	}

	return bytes;
}

Address_t Module_t::Find(std::string_view pattern) {
	PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(this->pointer);
	PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t*>(this->pointer) + dos_header->e_lfanew);

	auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
	auto pattern_bytes = this->ToBytes(pattern);
	auto scan_bytes = reinterpret_cast<uint8_t*>(this->pointer);

	auto pattern_size = pattern_bytes.size();
	auto pattern_data = pattern_bytes.data();

	for (int i = 0; i < size_of_image - pattern_size; i++) {
		bool found = true;

		for (int j = 0; j < pattern_size; ++j) {
			if (pattern_data[j] == -1)
				continue;

			if (scan_bytes[i + j] != pattern_data[j]) {
				found = false;
				break;
			}
		}

		if (found)
			return &scan_bytes[i];
	}

	return nullptr;
}