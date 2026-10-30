#pragma once

struct CColoredModels {
	using ScenePvsTest_t = uint8_t(__fastcall*)(const void*, const uint32_t*);
	using DrawIndexed_t = void(__fastcall*)(void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, int32_t, uint32_t);

	struct PlayerPvsScope_t {
		PlayerPvsScope_t(const void* callsite, const uint32_t* bitmap);
		~PlayerPvsScope_t();

		PlayerPvsScope_t(const PlayerPvsScope_t&) = delete;
		PlayerPvsScope_t& operator=(const PlayerPvsScope_t&) = delete;

		bool Add(const void* clusters);
		void Collect(const CSceneVisibilityList* list, uint32_t begin, uint32_t end, const void* desc, C_CSPlayerPawn* local, uint8_t mask);

		const PlayerPvsScope_t* previous{ nullptr };
		const void* callsite{ nullptr };
		const uint32_t* bitmap{ nullptr };
		std::array<const void*, 96> allowed_clusters{};
		size_t count{ 0 };
	};

	static thread_local const PlayerPvsScope_t* player_pvs_scope;

	uint8_t OnTestScenePvs(const void* clusters, const uint32_t* bitmap, const void* callsite, ScenePvsTest_t native_test);

	const void* player_pvs_desc{ nullptr };
	const void* player_pvs_callsite{ nullptr };

	struct DrawMarker_t {
		void* context{ nullptr };
		bool occluded{ false };
		bool in_override{ false };
	};

	static thread_local DrawMarker_t draw_marker;

	struct ScopedDrawMarker_t {
		ScopedDrawMarker_t(void* context, bool occluded);
		~ScopedDrawMarker_t();

		ScopedDrawMarker_t(const ScopedDrawMarker_t&) = delete;
		ScopedDrawMarker_t& operator=(const ScopedDrawMarker_t&) = delete;

		DrawMarker_t previous{};
	};

	struct DepthSnapshot_t {
		ID3D11DepthStencilState* actual{ nullptr };
		uint32_t actual_ref{ 0 };
		ID3D11DepthStencilState* override_state{ nullptr };
		uint32_t override_ref{ 0 };
		bool valid{ false };
	};

	using DepthFactory_t = void*(__fastcall*)(const char*, int*);
	using DepthSetter_t = void(__fastcall*)(void*, ID3D11DepthStencilState*, uint32_t);
	using DepthCreate_t = ID3D11DepthStencilState*(__fastcall*)(void*, const uint64_t*);

	bool SupportsDepth(void* context) const;
	DepthSnapshot_t CaptureDepth(void* context) const;
	bool ReadDepthDescriptor(ID3D11DepthStencilState* state, D3D11_DEPTH_STENCIL_DESC& descriptor) const;
	ID3D11DepthStencilState* CreateDepthState(uint64_t packed) const;

	DepthFactory_t depth_factory{ nullptr };
	DepthSetter_t set_depth_state{ nullptr };
	DepthSetter_t set_depth_override{ nullptr };
	DepthCreate_t create_depth_state{ nullptr };
	void* draw_dx{ nullptr };
	void* draw_software{ nullptr };

	struct ScopedDepthRestore_t {
		~ScopedDepthRestore_t();

		void* context;
		CColoredModels& models;
		DepthSnapshot_t saved;
		DrawMarker_t previous;
	};

	static bool BuildOccludedDescriptor(const D3D11_DEPTH_STENCIL_DESC& source, uint64_t& packed);

	using GeneratePrimitives_t = void*(__fastcall*)(void*, CSceneAnimtableObject*, void*, CMeshPrimitiveOutputBuffer*);
	using CullSceneObjects_t = int64_t(__fastcall*)(void*, void*, CSceneVisibilityList*, uint32_t, uint32_t, void*, void*, void*);
	using DrawScenePrimitive_t = uintptr_t(__fastcall*)(void*, void*, CMeshPrimitive*, void*, void*, void*, char, void*, CSceneLayer*);

	void Initialize();
	void* OnGeneratePrimitives(void* desc, CSceneAnimtableObject* object, void* context, CMeshPrimitiveOutputBuffer* buffer, GeneratePrimitives_t original);
	int64_t OnCullSceneObjects(void* scene_system, void* draw_list, CSceneVisibilityList* list, uint32_t begin, uint32_t end, void* output, void* pending, void* stats, CullSceneObjects_t original);
	uintptr_t OnDrawScenePrimitive(void* draw_data, void* context, CMeshPrimitive* primitive, void* material, void* shader_state, void* binding_cache, char depth_pass, void* stats, CSceneLayer* layer, DrawScenePrimitive_t original);
	bool OnDrawIndexed(void* context, uint32_t topology, uint32_t start_index, uint32_t index_count, uint32_t instance_count, uint32_t unused, int32_t base_vertex, uint32_t start_instance, DrawIndexed_t original);

	void OnSortPrimitives(CSceneLayer* layer, CMeshPrimitive* entries, uint32_t count);
	void OnRenderLayerDrawList(CSceneLayer* layer, CMeshPrimitivePageList* lists);

	bool IsOccludedColorPass(CMaterial2* material, const CSceneLayer* layer);
	uint8_t GetOccludedPlayerMask() const;
	bool IsOccludedPlayer(CSceneAnimtableObject* object, C_CSPlayerPawn* local, uint8_t mask) const;
	void Clear();

	enum class PassRole_t {
		Occluded = 0,
		Original = 1,
		Visible = 2
	};

	struct RenderPlan_t {
		std::array<CMaterial2*, 2> materials{};
		std::array<Color_t, 2> colors{};
		size_t layer_count{ 0 };
		int requested_layers{ 0 };
		int missing_layers{ 0 };
		int base_alpha{ 255 };
		bool preserve_original{ false };
		bool missing_visible{ false };
		bool scope_blend{ false };
	};

	struct PassSlot_t {
		size_t index;
		int rank;
	};

	CMaterial2* GetTransparentMaterial(CMaterial2* original);
	std::mutex transparency_mutex;
	std::unordered_map<CMaterial2*, CMaterial2*> transparent_materials;
	uint64_t transparent_material_count{ 0 };

	std::unordered_map<std::string, CStrongHandle<CMaterial2>> material_cache;
	std::unordered_map<CMaterial2*, PassRole_t> material_roles;
	std::shared_mutex materials_mutex;
	uint64_t material_generation{ 0 };

	bool OrderObjectPasses(const std::vector<CMeshPrimitive*>& entries);
	int GetPassRole(CMaterial2* material) const;
	void SelectScopeBlend(RenderPlan_t& plan);
	void AddLayer(RenderPlan_t& plan, uint32_t enable, uint32_t color_id, uint32_t overlay_id, uint32_t material_id, uint32_t exponent_id, uint32_t falloff_id, PassRole_t role, bool ignore_depth);
	void DrawConfiguredLayer(void* desc, CSceneAnimtableObject* object, void* context, CMeshPrimitiveOutputBuffer* buffer, CMaterial2* material, const Color_t& color);
	void DrawOriginal(void* desc, CSceneAnimtableObject* object, void* context, CMeshPrimitiveOutputBuffer* buffer, int alpha);
	CMaterial2* GetCustomMaterial(int material_id, bool ignore_depth, bool additive, float fresnel_exponent, float fresnel_falloff, PassRole_t role);

	static CStrongHandle<CMaterial2> CreateMaterial(const char* material_name, const char* vmat);
	static CSceneLayer* GetSceneLayer(void* context);
	static bool ShouldOrderColorLayer(const CSceneLayer* layer);
	static CMaterial2* GetLayerMaterial(CMaterial2* material, CSceneLayer* layer);
	static const char* GetTextureResourcePath(CMaterialParameter* parameter);
};

inline CColoredModels* ColoredModels = new CColoredModels();

static const char shaded_vmat_template[] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
    shader = "csgo_complex.vfx"
    F_TRANSLUCENT = 1
    F_DISABLE_Z_BUFFERING = %d
    F_PAINT_VERTEX_COLORS = 1
    F_RENDER_BACKFACES = 0
    F_ADDITIVE_BLEND = %d

    g_vColorTint = [1.000000, 1.000000, 1.000000, 1.000000]
    g_vReflectanceRange = [0.200000, 0.600000, 0.000000, 0.000000]
    g_vGlossinessRange = [0.100000, 0.500000, 0.000000, 0.000000]

    g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
    g_tNormal = resource:"materials/default/default_normal_tga_7be61377.vtex"
    g_tAmbientOcclusion = resource:"materials/default/default_ao_tga_559f1ac6.vtex"
})";

static const char flat_vmat_template[] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
    shader = "csgo_unlitgeneric.vfx"
    F_RENDER_BACKFACES = 0
    F_VERTEX_COLOR = 0
    F_DISABLE_Z_PREPASS = 1
    F_DISABLE_Z_BUFFERING = %d
    F_BLEND_MODE = %d
    g_flModelTintAmount = 1
    g_vColorTint = [1.000000, 1.000000, 1.000000, 1.000000]
    g_bFogEnabled = 0
    g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
})";

static const char glossy_vmat_template[] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
    shader = "csgo_effects.vfx"
    F_ADDITIVE_BLEND = %d
    F_DISABLE_Z_BUFFERING = %d

    g_flOpacityScale = 1.0
    g_flFresnelExponent = %f
    g_flFresnelFalloff = %f
    g_flFresnelMax = 1.0
    g_flFresnelMin = 0.0
    g_flColorBoost = 3.0
    g_flToolsVisCubemapReflectionRoughness = 0.1
    g_flBeginMixingRoughness = 0.1

    g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
    g_tMask1 = resource:"materials/default/default_mask_tga_fde710a5.vtex"
    g_tMask2 = resource:"materials/default/default_mask_tga_fde710a5.vtex"
    g_tMask3 = resource:"materials/default/default_mask_tga_fde710a5.vtex"

    g_vColorTint = [1.000000, 1.000000, 1.000000, 1.0]
})";

static const char glow_vmat_template[] = R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
    shader = "csgo_effects.vfx"
    g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
    g_tMask1 = resource:"materials/default/default_mask_tga_344101f8.vtex"
    g_tMask2 = resource:"materials/default/default_mask_tga_344101f8.vtex"
    g_tMask3 = resource:"materials/default/default_mask_tga_344101f8.vtex"
    g_flColorBoost = 20
    g_flOpacityScale = 0.7
    g_flFresnelExponent = %f
    g_flFresnelFalloff = %f
    g_flFresnelMax = 0
    g_flFresnelMin = 1
    F_ADDITIVE_BLEND = 1
    F_DISABLE_Z_BUFFERING = %d
    F_RENDER_BACKFACES = 0
    g_vColorTint = [1.00000, 1.00000, 1.00000, 1.00000]
})";
