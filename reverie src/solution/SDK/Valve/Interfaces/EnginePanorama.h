#pragma once

struct CTextureDX11;
struct CRenderViewport;

struct CSource2UITexture {
	struct Data_t {
		CTextureDX11* dx11texture;
	};

	char pad[0x28];
	Data_t* data;
	int width;
	int height;
};

struct CImageData {
	char pad[0x30];
	CSource2UITexture* texture;
};

struct CImageProxySource {
	char pad[0x10];
	CImageData* data;

	CSource2UITexture* GetTextureID() { return Memory->Vtable.Execute<CSource2UITexture*>(this, 4); }
	void AddRef() { Memory->Vtable.Execute<void>(this, 10); }
	void Release() { Memory->Vtable.Execute<void>(this, 11); }

	ID3D11ShaderResourceView* GetNativeTexture();
	ImVec2 GetImageSize();
};

struct CImageResourceManager {
	CImageProxySource* LoadImageFromUrl(void* panel, void* default_resource_url, const char* url, uint32_t format, CRenderViewport* image_format_out);
	CImageProxySource* LoadImageInternal(const char* image_path, uint32_t format);
};

class CUIPanel;

struct CPanel2DStyle { char pad[0x8]; };
struct CPanel2D : CPanel2DStyle { CUIPanel* panel; };
struct CTopLevelWindowSource2 {};
struct CUIPanelStyle { char pad[0x8]; };

struct CPanelStyle : CUIPanelStyle {
	CUIPanel* owner;
	char pad[0x28];
	CUtlVector<void> properties;
	char pad2[0x48];
	CPanelStyle* self;
	void* callback;
};

struct CPanelHandle {
	int index = -1;
	uint32_t serial_number;

	bool Valid() const { return this->index != -1; }

	bool operator==(const CPanelHandle&) const = default;
};

struct CUIPanel {
	char pad00[0x8];
	CPanel2D* client_panel;
	const char* id;
	CUIPanel* parent;
	CTopLevelWindowSource2* parent_window;
	CUtlVector<CUIPanel*> childrens;
	char pad1[0x28];
	CPanelStyle style;
	char pad2[0x4];
	uint8_t flags;
	char pad3[0x28];
	CUtlVector<short> classes;
	char pad4[0x168];
	CPanelHandle handle;

	CUIPanel* GetRoot() { return Memory->Vtable.Execute<CUIPanel*>(this, 229); }
};

struct CPanelData {
	char pad0[8];
	uint32_t flags;
	uint32_t visible;
	CUIPanel* panel;
	uint32_t panelID;
	char pad1[4];
};

struct CUIEngine {
	char pad[0x1B8 + 0x8];
	double real_time;
	char pad1[0x30];
	void* style_factory_wrapper;
	char pad2[0x20];
	CUtlUnknown<CUIPanel> panels;
	char pad3[0x30];
	void* input_engine;
	void* layout_manager;
	char pad4[0x260];
	void* localization;
	void* sound_system;
	char pad5[0xA8];
	void* ui_settings;
	char pad6[0x38];
	void* global_context;
	char pad7[0x18];
	void* isolate;
	char pad8[0x278];
	void* filesystem;
	char pad9[0x60];
	void* stack[63];
	int32_t stack_depth;
	char pad10[0xDC];

	CImageResourceManager* GetResourceManager() { return Memory->Vtable.Execute<CImageResourceManager*>(this, 24); };
};

struct CEnginePanorama {
	CUIEngine* GetUIEngine() { return Memory->Vtable.Execute<CUIEngine*>(this, 13); }
};