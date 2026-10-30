#include <shared.h>

thread_local const CColoredModels::PlayerPvsScope_t* CColoredModels::player_pvs_scope{ nullptr };
thread_local CColoredModels::DrawMarker_t CColoredModels::draw_marker{};

void CColoredModels::Initialize() {
	this->player_pvs_desc = SceneSystem->GetObject(xs("AnimatableSceneObjectDesc"));
	this->player_pvs_callsite = Memory->Modules.Scenesystem.Find(xs("48 8B 8E A0 00 00 00 48 85 C9 74 ? 0F B6 46 78 C0 E8 03 A8 01 74 ? 49 8B D0 E8 ? ? ? ? 4C 8B 4C 24 ? 84 C0")).Add(31).As<const void*>();

	this->depth_factory = Memory->Modules.Rendersystem.Find(xs("48 89 5C 24 08 57 48 83 EC 20 48 8B DA 48 8B F9 48 85 D2 74 06 C7 02 00 00 00 00 48 8D 15 ? ? ? ? FF 15 ? ? ? ? 85 C0 75 12 48 8B 05 ? ? ? ?")).As<DepthFactory_t>();
	this->create_depth_state = Memory->Modules.Rendersystem.Find(xs("4C 8B DC 48 83 EC 48 49 8D 43 08 49 89 53 10 49 89 43 E0 4D 8D 4B D8 49 8D 43 10 49 C7 43 08 00 00 00 00 49 89 43 E8 4D 8D 43 18 49 8D 43 08 49 89 53 D8 48 81 C1 60 01 00 00 49 89 43 18 E8")).As<DepthCreate_t>();
	this->set_depth_state = Memory->Modules.Rendersystem.Find(xs("48 8B 81 58 0A 00 00 48 85 C0 74 0A 44 8B 81 60")).As<DepthSetter_t>();
	this->set_depth_override = Memory->Modules.Rendersystem.Find(xs("48 89 91 58 0A 00 00 44 89 81 60 0A 00 00 C3 CC")).As<DepthSetter_t>();
	this->draw_dx = Memory->Modules.Rendersystem.Find(xs("8B 44 24 78 44 8B CD 48 8B 8B A0 0A 00 00 8B D7 89 44 24 28 8B 44 24 70 89 44 24 20 4C 8B 11 41 FF 92 A0 00 00 00")).Sub(0x109).As<void*>();
	this->draw_software = Memory->Modules.Rendersystem.Find(xs("C7 01 1F 80 18 00 89 71 04 89 69 08 44 89 71 0C 89 41 10 8B 44 24 78 89 41 14 48 83 83 20 04 00 00 18")).Sub(0x10A).As<void*>();
}

CColoredModels::PlayerPvsScope_t::PlayerPvsScope_t(const void* callsite, const uint32_t* bitmap) {
	this->previous = player_pvs_scope;
	this->callsite = callsite;
	this->bitmap = bitmap;
	player_pvs_scope = this;
}

CColoredModels::PlayerPvsScope_t::~PlayerPvsScope_t() {
	player_pvs_scope = this->previous;
}

bool CColoredModels::PlayerPvsScope_t::Add(const void* clusters) {
	if (!clusters || !this->callsite || !this->bitmap)
		return false;

	for (size_t i = 0; i < this->count; i++) {
		if (this->allowed_clusters[i] == clusters)
			return true;
	}

	if (this->count == this->allowed_clusters.size())
		return false;

	this->allowed_clusters[this->count++] = clusters;
	return true;
}

void CColoredModels::PlayerPvsScope_t::Collect(const CSceneVisibilityList* list, uint32_t begin, uint32_t end, const void* desc, C_CSPlayerPawn* local, uint8_t mask) {
	if (!this->callsite || !this->bitmap || !list || !list->records || begin >= end || end > list->count || end - begin > this->allowed_clusters.size())
		return;

	if (!desc || !local || !(mask & 3))
		return;

	for (uint32_t i = begin; i < end; i++) {
		CSceneObject* object = list->records[i].object;
		if (!object || !(object->flags & 8) || object->desc != desc)
			continue;

		const void* clusters = object->GetPvsClusters();
		if (clusters && ColoredModels->IsOccludedPlayer(static_cast<CSceneAnimtableObject*>(object), local, mask))
			this->Add(clusters);
	}
}

uint8_t CColoredModels::OnTestScenePvs(const void* clusters, const uint32_t* bitmap, const void* callsite, ScenePvsTest_t original) {
	const PlayerPvsScope_t* scope = player_pvs_scope;
	if (scope && clusters && bitmap && callsite && scope->bitmap == bitmap && scope->callsite == callsite) {
		for (size_t i = 0; i < scope->count; i++) {
			if (scope->allowed_clusters[i] == clusters)
				return 1;
		}
	}

	return original(clusters, bitmap);
}

CColoredModels::ScopedDrawMarker_t::ScopedDrawMarker_t(void* context, bool occluded) {
	this->previous = draw_marker;
	draw_marker = { context, occluded, this->previous.in_override };
}

CColoredModels::ScopedDrawMarker_t::~ScopedDrawMarker_t() {
	draw_marker = this->previous;
}

bool CColoredModels::BuildOccludedDescriptor(const D3D11_DEPTH_STENCIL_DESC& source, uint64_t& packed) {
	if (!source.DepthEnable || (source.DepthWriteMask != D3D11_DEPTH_WRITE_MASK_ZERO && source.DepthWriteMask != D3D11_DEPTH_WRITE_MASK_ALL))
		return false;

	D3D11_COMPARISON_FUNC inverse;
	switch (source.DepthFunc) {
	case D3D11_COMPARISON_LESS:
	case D3D11_COMPARISON_LESS_EQUAL:
		inverse = D3D11_COMPARISON_GREATER;
		break;
	case D3D11_COMPARISON_GREATER:
	case D3D11_COMPARISON_GREATER_EQUAL:
		inverse = D3D11_COMPARISON_LESS;
		break;
	default:
		return false;
	}

	const auto valid_face = [](const D3D11_DEPTH_STENCILOP_DESC& face) {
		const auto valid_op = [](D3D11_STENCIL_OP value) {
			return value >= D3D11_STENCIL_OP_KEEP && value <= D3D11_STENCIL_OP_DECR;
		};

		return face.StencilFunc >= D3D11_COMPARISON_NEVER && face.StencilFunc <= D3D11_COMPARISON_ALWAYS && valid_op(face.StencilFailOp) && valid_op(face.StencilDepthFailOp) && valid_op(face.StencilPassOp);
	};

	if (!valid_face(source.FrontFace) || !valid_face(source.BackFace))
		return false;

	const auto value = [](uint64_t input) {
		return static_cast<uint64_t>(input) - 1;
	};

	packed = 1ull | (value(inverse) << 2)
		| (value(source.FrontFace.StencilFunc) << 16)
		| (value(source.BackFace.StencilFunc) << 20)
		| (static_cast<uint64_t>(source.StencilEnable != FALSE) << 24)
		| (value(source.FrontFace.StencilFailOp) << 25)
		| (value(source.FrontFace.StencilDepthFailOp) << 28)
		| (value(source.FrontFace.StencilPassOp) << 32)
		| (value(source.BackFace.StencilFailOp) << 35)
		| (value(source.BackFace.StencilDepthFailOp) << 38)
		| (value(source.BackFace.StencilPassOp) << 41)
		| (static_cast<uint64_t>(source.StencilReadMask) << 48);

	return true;
}

CColoredModels::ScopedDepthRestore_t::~ScopedDepthRestore_t() {
	this->models.set_depth_override(this->context, nullptr, 0);
	this->models.set_depth_state(this->context, this->saved.actual, this->saved.actual_ref);
	this->models.set_depth_override(this->context, this->saved.override_state, this->saved.override_ref);
	draw_marker = this->previous;
}

bool CColoredModels::OnDrawIndexed(void* context, uint32_t topology, uint32_t start_index, uint32_t index_count, uint32_t instance_count, uint32_t unused, int32_t base_vertex, uint32_t start_instance, DrawIndexed_t original) {
	if (!draw_marker.occluded || draw_marker.context != context || draw_marker.in_override) {
		original(context, topology, start_index, index_count, instance_count, unused, base_vertex, start_instance);
		return true;
	}

	const DepthSnapshot_t saved = this->CaptureDepth(context);
	D3D11_DEPTH_STENCIL_DESC descriptor{};
	uint64_t packed = 0;
	if (!saved.valid || !saved.actual || !this->ReadDepthDescriptor(saved.actual, descriptor) || !BuildOccludedDescriptor(descriptor, packed))
		return false;

	ID3D11DepthStencilState* inverse = this->CreateDepthState(packed);
	if (!inverse)
		return false;

	ScopedDepthRestore_t restore{ context, *this, saved, draw_marker };
	draw_marker.in_override = true;
	this->set_depth_override(context, inverse, saved.actual_ref);
	this->set_depth_state(context, inverse, saved.actual_ref);
	original(context, topology, start_index, index_count, instance_count, unused, base_vertex, start_instance);
	return true;
}

bool CColoredModels::SupportsDepth(void* context) const {
	if (!context || !this->set_depth_state || !this->set_depth_override || Memory->Vtable.Get<DepthSetter_t>(context, 43) != this->set_depth_state || Memory->Vtable.Get<DepthSetter_t>(context, 44) != this->set_depth_override)
		return false;

	void* draw = Memory->Vtable.Get<void*>(context, 94);
	return draw == this->draw_dx || draw == this->draw_software;
}

CColoredModels::DepthSnapshot_t CColoredModels::CaptureDepth(void* context) const {
	if (!this->SupportsDepth(context))
		return {};

	DepthSnapshot_t saved{};
	const auto* bytes = static_cast<const uint8_t*>(context);
	memcpy(&saved.actual, bytes + 0xA20, sizeof(saved.actual));
	memcpy(&saved.actual_ref, bytes + 0xA28, sizeof(saved.actual_ref));
	memcpy(&saved.override_state, bytes + 0xA58, sizeof(saved.override_state));
	memcpy(&saved.override_ref, bytes + 0xA60, sizeof(saved.override_ref));
	saved.valid = true;
	return saved;
}

bool CColoredModels::ReadDepthDescriptor(ID3D11DepthStencilState* state, D3D11_DEPTH_STENCIL_DESC& descriptor) const {
	if (!state)
		return false;

	state->GetDesc(&descriptor);
	return true;
}

ID3D11DepthStencilState* CColoredModels::CreateDepthState(uint64_t packed) const {
	void* device = this->depth_factory ? this->depth_factory(xs("RenderDevice003"), nullptr) : nullptr;
	if (!device || Memory->Vtable.Get<DepthCreate_t>(device, 31) != this->create_depth_state)
		return nullptr;

	return this->create_depth_state(device, &packed);
}

int64_t CColoredModels::OnCullSceneObjects(void* scene_system, void* draw_list, CSceneVisibilityList* list, uint32_t begin, uint32_t end, void* output, void* pending, void* stats, CullSceneObjects_t original) {
	PlayerPvsScope_t scope(this->player_pvs_callsite, list ? list->GetPvsBitmap() : nullptr);
	const auto mask = this->GetOccludedPlayerMask();
	if (mask && EntitySystem && EngineClient && EngineClient->InGame()) {
		auto* local = EntitySystem->GetLocalPawn().As<C_CSPlayerPawn*>();
		if (local)
			scope.Collect(list, begin, end, this->player_pvs_desc, local, mask);
	}

	return original(scene_system, draw_list, list, begin, end, output, pending, stats);
}

uintptr_t CColoredModels::OnDrawScenePrimitive(void* draw_data, void* context, CMeshPrimitive* primitive, void* material, void* shader_state, void* binding_cache, char depth_pass, void* stats, CSceneLayer* layer, DrawScenePrimitive_t original) {
	const bool hidden = !depth_pass && primitive && this->IsOccludedColorPass(primitive->material, layer);
	if (hidden && !this->SupportsDepth(context))
		return 0;

	ScopedDrawMarker_t marker(context, hidden);
	return original(draw_data, context, primitive, material, shader_state, binding_cache, depth_pass, stats, layer);
}

bool CColoredModels::OrderObjectPasses(const std::vector<CMeshPrimitive*>& entries) {
	if (entries.size() <= 1)
		return false;

	std::shared_lock<std::shared_mutex> lock(this->materials_mutex);
	if (this->material_roles.empty())
		return false;

	std::unordered_map<CSceneAnimtableObject*, std::vector<PassSlot_t>> groups;

	for (CMeshPrimitive* primitive : entries) {
		if (!primitive)
			continue;

		const int role = this->GetPassRole(primitive->material);
		if (role < 0 || role > 2 || !primitive->scene_animtable_object)
			continue;

		groups.try_emplace(primitive->scene_animtable_object);
	}

	if (groups.empty())
		return false;

	for (size_t i = 0; i < entries.size(); i++) {
		CMeshPrimitive* primitive = entries[i];
		if (!primitive)
			continue;

		auto group = groups.find(primitive->scene_animtable_object);
		if (group == groups.end())
			continue;

		const int role = this->GetPassRole(primitive->material);
		group->second.push_back({ i, role >= 0 && role <= 2 ? role : 1 });
	}

	bool changed = false;
	for (auto& entry : groups) {
		auto& group = entry.second;
		if (group.size() <= 1)
			continue;

		auto rank_less = [](const PassSlot_t& left, const PassSlot_t& right) {
			return left.rank < right.rank;
		};

		if (std::is_sorted(group.begin(), group.end(), rank_less))
			continue;

		auto order = group;
		std::stable_sort(order.begin(), order.end(), rank_less);

		std::vector<CMeshPrimitive> ordered;
		ordered.reserve(order.size());
		for (const auto& slot : order)
			ordered.push_back(*entries[slot.index]);

		for (size_t i = 0; i < group.size(); i++)
			*entries[group[i].index] = ordered[i];

		changed = true;
	}

	return changed;
}

int CColoredModels::GetPassRole(CMaterial2* material) const {
	auto it = this->material_roles.find(material);
	return it != this->material_roles.end() ? static_cast<int>(it->second) : -1;
}

CStrongHandle<CMaterial2> CColoredModels::CreateMaterial(const char* material_name, const char* vmat) {
	alignas(16) std::byte storage[0x100 + sizeof(CKeyValues3)]{};
	auto* key_values = reinterpret_cast<CKeyValues3*>(storage + 0x100);
	const CKv3ID kv3_id{ material_name, 0x469806E97412167CULL, 0xE73790B53EE6F2AFULL };

	if (!MaterialSystem || !KV3::InitLoader() || !KV3::load_key_value(key_values, nullptr, vmat, &kv3_id, nullptr, 0))
		return {};

	CStrongHandle<CMaterial2> custom_material{};
	MaterialSystem->CreateMaterial(&custom_material, material_name, key_values);

	return custom_material;
}

const char* CColoredModels::GetTextureResourcePath(CMaterialParameter* parameter) {
	if (!parameter)
		return nullptr;

	auto* binding = reinterpret_cast<CStrongHandle<CResourceBinding>*>(parameter->texture_value);
	if (!binding || !binding->name)
		return nullptr;

	return binding->name->resource_name_symbol;
}

CMaterial2* CColoredModels::GetTransparentMaterial(CMaterial2* original) {
	if (!original)
		return nullptr;

	std::lock_guard<std::mutex> lock(this->transparency_mutex);

	auto it = this->transparent_materials.find(original);
	if (it != this->transparent_materials.end())
		return it->second;

	const char* color_path = CColoredModels::GetTextureResourcePath(original->FindParameter(xs("g_tColor")));
	const char* normal_path = CColoredModels::GetTextureResourcePath(original->FindParameter(xs("g_tNormal")));
	const char* ao_path = CColoredModels::GetTextureResourcePath(original->FindParameter(xs("g_tAmbientOcclusion")));

	static const char* default_color = xs("materials/default/default_color_tga_72dcfbfd.vtex");
	static const char* default_normal = xs("materials/default/default_normal_tga_7652cb.vtex");
	static const char* default_ao = xs("materials/default/default_mask_tga_fde710a5.vtex");

	char vmat[2048];
	snprintf(vmat, sizeof(vmat), R"(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} --> {
shader = "csgo_complex.vfx"
F_TRANSLUCENT = 1
F_PAINT_VERTEX_COLORS = 1
F_RENDER_BACKFACES = 0
g_vColorTint = [1.000000, 1.000000, 1.000000, 1.000000]
g_vReflectanceRange = [0.200000, 0.600000, 0.000000, 0.000000]
g_vGlossinessRange = [0.100000, 0.500000, 0.000000, 0.000000]
g_vNormalTexCoordScale = [1.000000, 1.000000, 0.000000, 0.000000]
g_vTexCoordOffset = [0.000000, 0.000000, 0.000000, 0.000000]
g_vTexCoordScale = [1.000000, 1.000000, 0.000000, 0.000000]
g_tColor = resource:"%s"
g_tNormal = resource:"%s"
g_tAmbientOcclusion = resource:"%s"

})",
		color_path ? color_path : default_color,
		normal_path ? normal_path : default_normal,
		ao_path ? ao_path : default_ao);

	char name[64];
	snprintf(name, sizeof(name), xs("sl_scoped_proxy_%llu.vmat"), static_cast<unsigned long long>(this->transparent_material_count++));

	auto handle = CColoredModels::CreateMaterial(name, vmat);
	CMaterial2* proxy = handle;

	if (proxy)
		this->transparent_materials[original] = proxy;

	return proxy;
}

CMaterial2* CColoredModels::GetCustomMaterial(int material_id, bool ignore_depth, bool additive, float fresnel_exponent, float fresnel_falloff, PassRole_t role) {
	std::lock_guard<std::shared_mutex> lock(this->materials_mutex);

	material_id = std::clamp(material_id, 0, 3);

	const bool uses_fresnel = material_id == 2 || material_id == 3;
	fresnel_exponent = uses_fresnel && std::isfinite(fresnel_exponent) ? std::round(std::clamp(fresnel_exponent, 0.f, 5.f) * 100.f) / 100.f : 0.f;
	fresnel_falloff = uses_fresnel && std::isfinite(fresnel_falloff) ? std::round(std::clamp(fresnel_falloff, 0.f, 20.f) * 100.f) / 100.f : 0.f;
	additive = additive || material_id == 2;

	char key[128];
	snprintf(key, sizeof(key), xs("dyn_mat_%d_%d_%d_%d_%.2f_%.2f"), material_id, ignore_depth ? 1 : 0, additive ? 1 : 0, static_cast<int>(role), fresnel_exponent, fresnel_falloff);

	auto it = this->material_cache.find(key);
	if (it != this->material_cache.end())
		return it->second;

	char vmat[2048]{};
	const int z_disable = ignore_depth ? 1 : 0;
	const int additive_flag = additive ? 1 : 0;
	switch (material_id) {
	case 0:
		snprintf(vmat, sizeof(vmat), shaded_vmat_template, z_disable, additive_flag);
		break;
	case 1:
		snprintf(vmat, sizeof(vmat), flat_vmat_template, z_disable, additive ? 4 : 1);
		break;
	case 2:
		snprintf(vmat, sizeof(vmat), glow_vmat_template, fresnel_exponent, fresnel_falloff, z_disable);
		break;
	case 3:
		snprintf(vmat, sizeof(vmat), glossy_vmat_template, additive_flag, z_disable, fresnel_exponent, fresnel_falloff);
		break;
	}

	char material_name[192];
	snprintf(material_name, sizeof(material_name), xs("%s_%llu.vmat"), key, static_cast<unsigned long long>(this->material_generation));

	const auto material = this->CreateMaterial(material_name, vmat);
	if (material) {
		this->material_cache.emplace(key, material);
		this->material_roles.emplace(material, role);
	}

	return material;
}

CSceneLayer* CColoredModels::GetSceneLayer(void* context) {
	if (!context)
		return nullptr;

	return *reinterpret_cast<CSceneLayer**>(static_cast<std::byte*>(context) + 0x10);
}

CMaterial2* CColoredModels::GetLayerMaterial(CMaterial2* material, CSceneLayer* layer) {
	if (!material || !layer)
		return material;

	return Memory->Vtable.Execute<CMaterial2*>(material, 6, layer->GetMaterialModeData());
}

void CColoredModels::DrawConfiguredLayer(void* desc, CSceneAnimtableObject* object, void* context, CMeshPrimitiveOutputBuffer* buffer, CMaterial2* material, const Color_t& color) {
	if (!material || color.A() == 0)
		return;

	CSceneLayer* scene_layer = this->GetSceneLayer(context);
	if (scene_layer && (scene_layer->GetNativeFlags() & (1ull << 24)))
		return;

	const auto before = buffer->Snapshot();
	CMaterial2* secondary = this->GetLayerMaterial(material, scene_layer);
	GetOriginal(&GeneratePrimitives)(desc, object, context, buffer);

	buffer->VisitSince(before, [&](CMeshPrimitive* primitive) {
		primitive->material = material;
		primitive->material2 = secondary;
		primitive->color = color;
	});
}

void CColoredModels::DrawOriginal(void* desc, CSceneAnimtableObject* object, void* context, CMeshPrimitiveOutputBuffer* buffer, int alpha) {
	if (alpha == 0)
		return;

	CSceneLayer* scene_layer = this->GetSceneLayer(context);
	if (alpha < 255 && scene_layer && (scene_layer->GetNativeFlags() & (1ull << 24)))
		return;

	const auto before = buffer->Snapshot();
	GetOriginal(&GeneratePrimitives)(desc, object, context, buffer);

	if (alpha == 255)
		return;

	buffer->VisitSince(before, [&](CMeshPrimitive* primitive) {
		if (!primitive->material)
			return;

		CMaterial2* proxy = this->GetTransparentMaterial(primitive->material);
		if (!proxy)
			return;

		primitive->material = proxy;
		primitive->material2 = this->GetLayerMaterial(proxy, scene_layer);
		primitive->color.SetA(static_cast<uint8_t>(primitive->color.A() * alpha / 255));

		std::lock_guard<std::shared_mutex> lock(this->materials_mutex);
		this->material_roles.insert_or_assign(proxy, PassRole_t::Original);
	});
}

void CColoredModels::SelectScopeBlend(RenderPlan_t& plan) {
	plan.scope_blend = ctx.pawn && Config::Get<bool>(Vars::ColoredModels::ScopeBlend) && (ctx.pawn->m_bIsScoped() || (Config::Get<bool>(Vars::ColoredModels::ScopeBlendKeepOnResume) && ctx.pawn->m_bResumeZoom()));
	if (plan.scope_blend)
		plan.base_alpha = std::clamp(Config::Get<int>(Vars::ColoredModels::ScopeBlendValue), 0, 255);
}

void CColoredModels::AddLayer(RenderPlan_t& plan, uint32_t enable, uint32_t color_id, uint32_t overlay_id, uint32_t material_id, uint32_t exponent_id, uint32_t falloff_id, PassRole_t role, bool ignore_depth) {
	if (!Config::Get<bool>(enable))
		return;

	plan.requested_layers++;

	const bool overlay = Config::Get<bool>(overlay_id);
	Color_t color = Config::Get<Color_t>(color_id);
	color.SetA(static_cast<uint8_t>(color.A() * plan.base_alpha / 255));

	if (color.A() == 0) {
		plan.preserve_original |= overlay;
		return;
	}

	CMaterial2* material = this->GetCustomMaterial(Config::Get<int>(material_id), ignore_depth, overlay, Config::Get<float>(exponent_id), Config::Get<float>(falloff_id), role);
	if (!material) {
		plan.missing_layers++;
		plan.missing_visible |= role == PassRole_t::Visible;
		return;
	}

	plan.preserve_original |= overlay;
	plan.materials[plan.layer_count] = material;
	plan.colors[plan.layer_count++] = color;
}

void* CColoredModels::OnGeneratePrimitives(void* desc, CSceneAnimtableObject* object, void* context, CMeshPrimitiveOutputBuffer* buffer, GeneratePrimitives_t original) {
	if (!object || !buffer || !buffer->Snapshot().valid)
		return original(desc, object, context, buffer);

	auto owner = object->owner;
	if (!owner.Valid())
		return original(desc, object, context, buffer);

	C_BaseEntity* entity = owner.get<C_BaseEntity>();
	if (!entity)
		return original(desc, object, context, buffer);

	const auto* binding = entity->GetClassBindingBase();
	if (!binding || !binding->name)
		return original(desc, object, context, buffer);

	const auto class_hash = FnvHash(binding->name);
	RenderPlan_t plan{};

	if (class_hash == FnvHash("C_CS2HudModelArms")) {
		this->AddLayer(plan, Vars::ColoredModels::Arms::Enable, Vars::ColoredModels::Arms::Color, Vars::ColoredModels::Arms::Overlay, Vars::ColoredModels::Arms::Material, Vars::ColoredModels::Arms::Exponent, Vars::ColoredModels::Arms::Falloff, PassRole_t::Visible, true);
	} else if (class_hash == FnvHash("C_CS2HudModelWeapon")) {
		this->AddLayer(plan, Vars::ColoredModels::Weapon::Enable, Vars::ColoredModels::Weapon::Color, Vars::ColoredModels::Weapon::Overlay, Vars::ColoredModels::Weapon::Material, Vars::ColoredModels::Weapon::Exponent, Vars::ColoredModels::Weapon::Falloff, PassRole_t::Visible, true);
	} else if (entity->IsWeapon()) {
		auto handle = entity->m_hOwnerEntity();
		if (!ctx.pawn || !handle.Valid() || handle.get<C_BaseEntity>() != ctx.pawn)
			return original(desc, object, context, buffer);

		this->SelectScopeBlend(plan);
		this->AddLayer(plan, Vars::ColoredModels::Attachments::Enable, Vars::ColoredModels::Attachments::Color, Vars::ColoredModels::Attachments::Overlay, Vars::ColoredModels::Attachments::Material, Vars::ColoredModels::Attachments::Exponent, Vars::ColoredModels::Attachments::Falloff, PassRole_t::Visible, false);
	} else if (class_hash == FnvHash("C_CSPlayerPawn")) {
		C_CSPlayerPawn* pawn = static_cast<C_CSPlayerPawn*>(entity);
		if (!pawn->Alive() || !ctx.pawn)
			return original(desc, object, context, buffer);

		if (pawn == ctx.pawn) {
			this->SelectScopeBlend(plan);
			this->AddLayer(plan, Vars::ColoredModels::Local::Enable, Vars::ColoredModels::Local::Color, Vars::ColoredModels::Local::Overlay, Vars::ColoredModels::Local::Material, Vars::ColoredModels::Local::Exponent, Vars::ColoredModels::Local::Falloff, PassRole_t::Visible, true);
		} else if (pawn->Enemy(ctx.pawn)) {
			plan.preserve_original = !Config::Get<bool>(Vars::ColoredModels::EnemyVisible::Enable);
			this->AddLayer(plan, Vars::ColoredModels::EnemyOccluded::Enable, Vars::ColoredModels::EnemyOccluded::Color, Vars::ColoredModels::EnemyOccluded::Overlay, Vars::ColoredModels::EnemyOccluded::Material, Vars::ColoredModels::EnemyOccluded::Exponent, Vars::ColoredModels::EnemyOccluded::Falloff, PassRole_t::Occluded, false);
			this->AddLayer(plan, Vars::ColoredModels::EnemyVisible::Enable, Vars::ColoredModels::EnemyVisible::Color, Vars::ColoredModels::EnemyVisible::Overlay, Vars::ColoredModels::EnemyVisible::Material, Vars::ColoredModels::EnemyVisible::Exponent, Vars::ColoredModels::EnemyVisible::Falloff, PassRole_t::Visible, false);
		} else {
			plan.preserve_original = !Config::Get<bool>(Vars::ColoredModels::FriendlyVisible::Enable);
			this->AddLayer(plan, Vars::ColoredModels::FriendlyOccluded::Enable, Vars::ColoredModels::FriendlyOccluded::Color, Vars::ColoredModels::FriendlyOccluded::Overlay, Vars::ColoredModels::FriendlyOccluded::Material, Vars::ColoredModels::FriendlyOccluded::Exponent, Vars::ColoredModels::FriendlyOccluded::Falloff, PassRole_t::Occluded, false);
			this->AddLayer(plan, Vars::ColoredModels::FriendlyVisible::Enable, Vars::ColoredModels::FriendlyVisible::Color, Vars::ColoredModels::FriendlyVisible::Overlay, Vars::ColoredModels::FriendlyVisible::Material, Vars::ColoredModels::FriendlyVisible::Exponent, Vars::ColoredModels::FriendlyVisible::Falloff, PassRole_t::Visible, false);
		}
	} else {
		return original(desc, object, context, buffer);
	}

	if (plan.requested_layers == 0 && !plan.scope_blend)
		return original(desc, object, context, buffer);

	if (plan.requested_layers > 0 && plan.missing_layers == plan.requested_layers && !plan.scope_blend)
		return original(desc, object, context, buffer);

	plan.preserve_original |= plan.missing_visible || (plan.requested_layers == 0 && plan.scope_blend) || (plan.missing_layers == plan.requested_layers && plan.missing_layers != 0);

	if (plan.preserve_original)
		this->DrawOriginal(desc, object, context, buffer, plan.base_alpha);

	for (size_t i = 0; i < plan.layer_count; i++)
		this->DrawConfiguredLayer(desc, object, context, buffer, plan.materials[i], plan.colors[i]);

	return nullptr;
}

bool CColoredModels::ShouldOrderColorLayer(const CSceneLayer* layer) {
	if (!layer)
		return false;

	const auto flags = layer->GetNativeFlags();
	return !(flags & (1ull << 24)) && (!(flags & 2) || (flags & 1));
}

bool CColoredModels::IsOccludedColorPass(CMaterial2* material, const CSceneLayer* layer) {
	if (!material || !layer || (layer->GetNativeFlags() & (1ull << 24)))
		return false;

	std::shared_lock<std::shared_mutex> lock(this->materials_mutex);
	auto it = this->material_roles.find(material);
	return it != this->material_roles.end() && it->second == PassRole_t::Occluded;
}

uint8_t CColoredModels::GetOccludedPlayerMask() const {
	uint8_t mask = 0;
	if (Config::Get<bool>(Vars::ColoredModels::EnemyOccluded::Enable) && Config::Get<Color_t>(Vars::ColoredModels::EnemyOccluded::Color).A() != 0)
		mask |= 1;
	if (Config::Get<bool>(Vars::ColoredModels::FriendlyOccluded::Enable) && Config::Get<Color_t>(Vars::ColoredModels::FriendlyOccluded::Color).A() != 0)
		mask |= 2;

	return mask;
}

bool CColoredModels::IsOccludedPlayer(CSceneAnimtableObject* object, C_CSPlayerPawn* local, uint8_t mask) const {
	if (!object || !local || !(mask & 3))
		return false;

	auto owner = object->owner;
	if (!owner.Valid())
		return false;

	C_BaseEntity* entity = owner.get<C_BaseEntity>();
	if (!entity || entity == local)
		return false;

	const auto* binding = entity->GetClassBindingBase();
	if (!binding || !binding->name || FnvHash(binding->name) != FnvHash("C_CSPlayerPawn"))
		return false;

	C_CSPlayerPawn* pawn = static_cast<C_CSPlayerPawn*>(entity);
	return pawn->Alive() && (mask & (pawn->Enemy(local) ? 1 : 2)) != 0;
}

void CColoredModels::OnSortPrimitives(CSceneLayer* layer, CMeshPrimitive* entries, uint32_t count) {
	if (!this->ShouldOrderColorLayer(layer) || !entries || count < 2 || count > static_cast<uint32_t>(INT_MAX))
		return;

	{
		std::shared_lock<std::shared_mutex> lock(this->materials_mutex);
		if (this->material_roles.empty())
			return;

		bool has_managed_material = false;
		for (uint32_t i = 0; i < count; i++) {
			const int role = this->GetPassRole(entries[i].material);
			if (role >= 0 && role <= 2 && entries[i].scene_animtable_object) {
				has_managed_material = true;
				break;
			}
		}

		if (!has_managed_material)
			return;
	}

	std::vector<CMeshPrimitive*> primitives;
	primitives.reserve(count);
	for (uint32_t i = 0; i < count; i++)
		primitives.push_back(&entries[i]);

	this->OrderObjectPasses(primitives);
}

void CColoredModels::OnRenderLayerDrawList(CSceneLayer* layer, CMeshPrimitivePageList* lists) {
	if (!this->ShouldOrderColorLayer(layer) || !lists || lists->count <= 1 || !lists->pages)
		return;

	int count = 0;
	{
		std::shared_lock<std::shared_mutex> lock(this->materials_mutex);
		if (this->material_roles.empty())
			return;

		bool has_managed_material = false;
		for (int i = 0; i < lists->count; i++) {
			CMeshPrimitivePage* page = lists->pages[i];
			if (!page || page->count < 0 || page->count > 256 || count > INT_MAX - page->count)
				return;

			count += page->count;
			if (has_managed_material)
				continue;

			for (int j = 0; j < page->count; j++) {
				const int role = this->GetPassRole(page->entries[j].material);
				if (role >= 0 && role <= 2 && page->entries[j].scene_animtable_object) {
					has_managed_material = true;
					break;
				}
			}
		}

		if (!has_managed_material)
			return;
	}

	if (count < 2)
		return;

	std::vector<CMeshPrimitive*> entries;
	entries.reserve(count);
	for (int i = 0; i < lists->count; i++) {
		CMeshPrimitivePage* page = lists->pages[i];
		for (int j = 0; j < page->count; j++)
			entries.push_back(&page->entries[j]);
	}

	this->OrderObjectPasses(entries);
}

void CColoredModels::Clear() {
	{
		std::lock_guard<std::mutex> lock(this->transparency_mutex);
		this->transparent_materials.clear();
	}

	std::lock_guard<std::shared_mutex> lock(this->materials_mutex);
	this->material_roles.clear();
	this->material_cache.clear();
	++this->material_generation;
}
