#pragma once
#include <atomic>
#include <cstdint>

struct ID3D11ShaderResourceView;

struct CMisc {
	void BuyBot();
	void OnPresent();
	void Watermark();
	void Indicators();
	void CrosshairIndicators();
	inline static bool buy_pending{ true };
	inline static std::atomic<uint64_t> hitmarker_time{ 0 };

	struct Panorama_t {
		struct IconData_t {
			ID3D11ShaderResourceView* texture_view;
			uint32_t width;
			uint32_t height;

			IconData_t() : texture_view(nullptr), width(0), height(0) {}
			IconData_t(ID3D11ShaderResourceView* view, uint32_t w, uint32_t h) : texture_view(view), width(w), height(h) {}
		};

		struct PanoramaImageData_t {
			CUtlBuffer buffer;

			PanoramaImageData_t(std::vector<uint8_t>& buffer) : buffer(buffer.data(), static_cast<int>(buffer.size()), 0) {}

			bool LoadSvg(const char* svg_data, size_t data_size, uint32_t* out_width, uint32_t* out_heigth) const {
				static auto svg_parser = Memory->Modules.Panorama.Find(xs("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 4C 8B AD")).As<bool(__fastcall*)(const char*, unsigned int, const CUtlBuffer*, uint32_t*, uint32_t*, float, void*)>();
				if (!svg_parser)
					return false;

				return svg_parser(svg_data, data_size, &buffer, out_width, out_heigth, -1, 0);
			}
		};

		IconData_t GetPanoramaTexture(const std::string& path);
		IconData_t GetWeaponIcon(uint16_t weapon_index);
		IconData_t GetWeaponIconByName(const std::string& weapon_name);
		IconData_t GetGrenadeIcon(int index);

		bool LoadVtex(const std::string& path, IconData_t* output);

		std::unordered_map<uint32_t, IconData_t> icons;
	} Panorama;

	struct QuickSwitch_t {
		void OnCreateMove();
		void Reset();

		bool enabled = true;
	private:
		uint32_t last_handle = 0;
		bool armed = false;
		bool pending = false;
		int pending_ticks = 0;
	} QuickSwitch;
};

inline CMisc* Misc = new CMisc();