#include <shared.h>
#include "../../Gui/Blur/Blur.h"
#include "../../Utils/ThreadPool/ThreadPool.h"
#include "../../Gui/Customflow/CustomflowBridge.h"

static void(__fastcall* setChannelVerbosity)(int, int) = nullptr;
static int shootingChannel = -1;
static int shootingVerbosity = 0;
static thread_local bool networkAnimationUpdate = false;

void __fastcall CreateMove(CCSGOInput* input, int slot, bool active) {
	static const auto original = GetOriginal(&CreateMove);
	original(input, slot, active);

	if (!ctx.Fill()) {
		Misc->buy_pending = true;
		Movement->m_bKeepDucked = false;
		Movement->LegMovement(nullptr);
		LagCompensation->Reset();
		Ragebot->Reset();
		return;
	}

	Events->FlushPendingImpacts();
	Misc->BuyBot();
	Ragebot->DoublePrecision();
	Ragebot->PrepareCommand();

	Movement->m_bKeepDucked = Config::Get<bool>(Vars::Movement::JumpBug) && ctx.pawn->Alive() && (ctx.cmd->button_state.held & IN_DUCK) && (Movement->m_bKeepDucked || ((ctx.cmd->button_state.held & IN_JUMP) && (ctx.pawn->m_fFlags() & FL_DUCKING)));
	Movement->angle = input->current_move_data.viewangles;

	AntiAim->OnCreateMove();

	Movement->Rotation();

	if (!EnginePrediction->Start(ctx.cmd))
		return;

	Movement->JumpBug(ctx.cmd);
	Movement->BunnyHop(ctx.cmd);
	Movement->AirStuck(ctx.cmd);
	Movement->FastLadder(ctx.cmd);
	Ragebot->Run(input);
	Movement->PeekAssist(ctx.cmd);

	Movement->LegMovement(ctx.cmd);
	EnginePrediction->End();

	GrenadeTrajectory->OnCreateMove();

	Misc->QuickSwitch.OnCreateMove();

	Events->OnCreateMove();
}

void __fastcall FrameStage(void* client, int stage) {
	static const auto original = GetOriginal(&FrameStage);
	bool was_updating = networkAnimationUpdate;
	networkAnimationUpdate = stage == 7;

	if (networkAnimationUpdate)
		LagCompensation->OnNetUpdate();

	original(client, stage);

	networkAnimationUpdate = was_updating;

	if (stage == 7) {
		Particles->Weather();
		Skins->OnFrameStage();
	}
}

void __fastcall UpdateHitboxHistory(C_CSPlayerPawn* player, int tick) {
	static const auto original = GetOriginal(&UpdateHitboxHistory);
	original(player, tick);

	if (networkAnimationUpdate)
		LagCompensation->OnAnimationUpdate(player, tick);
}

void* __fastcall AddEntity(void* system, CEntityInstance* instance, CBaseHandle handle) {
	static const auto original = GetOriginal(&AddEntity);
	EntityList->OnAdd(instance, handle);
	return original(system, instance, handle);
}

void* __fastcall RemoveEntity(void* system, CEntityInstance* instance, CBaseHandle handle) {
	static const auto original = GetOriginal(&RemoveEntity);
	EntityList->OnRemove(instance, handle);
	return original(system, instance, handle);
}

HRESULT __stdcall Present(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags) {
	static const auto original = GetOriginal(&Present);

	if (!Target)
		Valve.CreateRenderTarget();

	if (!DeviceCtx || !Draw->initialized)
		return original(swap_chain, sync_interval, flags);

	if (Target && DeviceCtx)
		DeviceCtx->OMSetRenderTargets(1, &Target, nullptr);

	Draw->BeginFrame();
	{
		// Customflow menu replaces Gui->Render() entirely.
		CustomflowBridge::Frame();

		Visuals->Player.OnPresent();
		GrenadeTrajectory->OnPresent();
		GrenadeWarnings->OnPresent();
		World->ScopeOverlay.OnPresent();
		AntiAim->OnPresent();
		Movement->OnPresent();
		Misc->OnPresent();
		Ragebot->OnPresent();
		World->OnPresent();
		World->PenetrationCrosshair.OnPresent();
	}
	Draw->EndFrame();

	ImDrawData* draw_data = ImGui::GetDrawData();

	if (draw_data && draw_data->Valid && draw_data->CmdListsCount > 0 && draw_data->TotalVtxCount > 0)
		Draw->SubmitDrawData(draw_data);

	Draw->ReleaseFrameTextures();

	return original(swap_chain, sync_interval, flags);
}

void* __fastcall UpdateGlobalVars(void* a1, CGlobalVariables* new_globals) {
	static auto original = GetOriginal(&UpdateGlobalVars);

	GlobalVars = new_globals;

	return original(a1, new_globals);
}

void __fastcall RecordLanding(CModernJump* jump, CMoveData* data, float fall_speed) {
	static auto original = GetOriginal(&RecordLanding);

	EnginePrediction->CaptureLanding(jump, data);
	original(jump, data, fall_speed);
}

__int64 __fastcall SetGroundEntity(CCSPlayer_MovementServices* movement, CMoveData* data, CGameTrace* trace) {
	static auto original = GetOriginal(&SetGroundEntity);

	const auto result = original(movement, data, trace);
	EnginePrediction->CaptureGround(movement, data, trace);
	return result;
}

__int64 __fastcall ReflectPawnState(CCS2PawnGraphController* graph, C_CSPlayerPawn* pawn) {
	static auto original = GetOriginal(&ReflectPawnState);

	const auto result = original(graph, pawn);
	if (pawn && pawn == ctx.pawn && pawn->Alive() && Movement->m_bKeepDucked)
		graph->m_flCrouchAmount().Set(1.f);

	return result;
}

HRESULT __fastcall ResizeBuffers(IDXGISwapChain* swap_chain, uint32_t buffer_count, uint32_t width, uint32_t height, DXGI_FORMAT new_format, uint32_t flags) {
	static auto original = GetOriginal(&ResizeBuffers);

	Valve.DestroyRenderTarget();

	HRESULT hr = original(swap_chain, buffer_count, width, height, new_format, flags);

	if (SUCCEEDED(hr))
		Valve.CreateRenderTarget();

	return hr;
}

void* __fastcall IsRelativeMouseMode(void* input_system, bool active) {
	static auto original = GetOriginal(&IsRelativeMouseMode);

	if (!Hooks->RelativeMouseMode)
		Hooks->RelativeMouseMode = original;

	Var->winapi.active_mouse_input = active;

	return original(input_system, Config::Get<bool>(Vars::Antiaim::MouseOverride) && !Draw->opened ? false : (Draw->opened ? false : active));
}

bool __fastcall MouseInputEnabled(void* a1) {
	static auto og = GetOriginal(&MouseInputEnabled);

	if (Config::Get<bool>(Vars::Antiaim::MouseOverride) && !Draw->opened)
		return false;

	return Draw->opened ? false : og(a1);
}

void* __fastcall RenderView(void* thisptr) {
	static auto original = GetOriginal(&RenderView);
	void* result = original(thisptr);

	static Matrix4x4_t* view_matrix_ptr = nullptr;
	if (!view_matrix_ptr) {
		auto addr = Memory->Modules.Client.Find(xs("48 8D 0D ? ? ? ? 48 C1 E0 06"));
		if (addr.Get() != 0)
			view_matrix_ptr = addr.Rel(3, 7).As<Matrix4x4_t*>();
	}

	if (view_matrix_ptr)
		ctx.viewmatrix = *view_matrix_ptr;

	if (thisptr) {
		uintptr_t view_info = reinterpret_cast<uintptr_t>(thisptr) + 0x10;
		ctx.camera_origin = *reinterpret_cast<Vector*>(view_info + 0x0);
		ctx.camera_angles = *reinterpret_cast<Vector*>(view_info + 0xC);
	}

	return result;
}

void __fastcall EmitLandSound(C_CSPlayerPawn* pawn, float fall_velocity) {
	static const auto original = GetOriginal(&EmitLandSound);

	if (pawn == ctx.pawn && Config::Get<bool>(Vars::Movement::JumpBug) && (ctx.cmd->button_state.held & IN_JUMP))
		return;

	original(pawn, fall_velocity);
}

void __fastcall SetupView(void* client, CViewSetup* setup) {
	static const auto original = GetOriginal(&SetupView);
	original(client, setup);

	World->OnSetupView(setup);
}

void __fastcall UpdateFOVSensitivity(C_CSPlayerPawn* pawn) {
	static const auto original = GetOriginal(&UpdateFOVSensitivity);
	original(pawn);

	World->OnUpdateFOVSensitivity(pawn);
}

void __fastcall DrawScopeOverlay(void* a1, uintptr_t a2) {
	static const auto original = GetOriginal(&DrawScopeOverlay);
	original(a1, a2);

	if (Config::Get<int>(Vars::World::RemoveScope) >= 1)
		*reinterpret_cast<bool*>(a2 + 4) = false;
}

float __fastcall SetupFov(void* a1) {
	static const auto original = GetOriginal(&SetupFov);

	return ctx.pawn && ctx.pawn->Alive() && ctx.pawn->m_bIsScoped() && Config::Get<bool>(Vars::World::ShowViewmodelInScope) ? 90.f : original(a1);
}

bool __fastcall IsHudModelArmsRendering(void* a1) {
	static auto original = GetOriginal(&IsHudModelArmsRendering);

	if (Config::Get<bool>(Vars::World::Thirdperson))
		return true;

	return original(a1);
}

void __fastcall UpdateHudModelArms(void* player_pawn, Vector* position, Vector* angles, char reset_animation) {
	static auto original = GetOriginal(&UpdateHudModelArms);

	if (player_pawn == ctx.pawn && angles && Config::Get<bool>(Vars::World::Thirdperson)) {
		Vector hud_angles = *angles;
		hud_angles.x = -89.f;
		hud_angles.y += 90.f;
		return original(player_pawn, position, &hud_angles, reset_animation);
	}

	return original(player_pawn, position, angles, reset_animation);
}

void __fastcall SendMove(CCSGOInput* input, int slot) {
	static auto original = GetOriginal(&SendMove);

	Vector original_view_angles = input->current_move_data.viewangles;
	original(input, slot);

	input->current_move_data.viewangles = original_view_angles;
}

float __fastcall AdjustCommandTime(void* client, int command, double time, float interval) {
	static const auto original = GetOriginal(&AdjustCommandTime);
	float adjustment = original(client, command, time, interval);
	return Ragebot->AdjustCommandTime(client, command, time, interval, adjustment);
}

std::int64_t __fastcall CullSceneObjects(void* scene_system, void* draw_list, CSceneVisibilityList* list, std::uint32_t begin, std::uint32_t end, void* output, void* pending, void* stats) {
	static const auto original = GetOriginal(&CullSceneObjects);
	return ColoredModels->OnCullSceneObjects(scene_system, draw_list, list, begin, end, output, pending, stats, original);
}

std::uint8_t __fastcall TestScenePvs(const void* clusters, const std::uint32_t* bitmap) {
	static const auto original = GetOriginal(&TestScenePvs);
	return ColoredModels->OnTestScenePvs(clusters, bitmap, _ReturnAddress(), original);
}

void __fastcall DrawOccludedDx11(void* context, std::uint32_t topology, std::uint32_t start_index, std::uint32_t index_count, std::uint32_t instance_count, std::uint32_t unused, std::int32_t base_vertex, std::uint32_t start_instance) {
	static const auto original = GetOriginal(&DrawOccludedDx11);
	ColoredModels->OnDrawIndexed(context, topology, start_index, index_count, instance_count, unused, base_vertex, start_instance, original);
}

void __fastcall DrawOccludedSoftware(void* context, std::uint32_t topology, std::uint32_t start_index, std::uint32_t index_count, std::uint32_t instance_count, std::uint32_t unused, std::int32_t base_vertex, std::uint32_t start_instance) {
	static const auto original = GetOriginal(&DrawOccludedSoftware);
	ColoredModels->OnDrawIndexed(context, topology, start_index, index_count, instance_count, unused, base_vertex, start_instance, original);
}

std::uintptr_t __fastcall DrawScenePrimitive(void* draw_data, void* context, CMeshPrimitive* primitive, void* material, void* shader_state, void* binding_cache, char depth_pass, void* stats, CSceneLayer* layer) {
	static const auto original = GetOriginal(&DrawScenePrimitive);
	return ColoredModels->OnDrawScenePrimitive(draw_data, context, primitive, material, shader_state, binding_cache, depth_pass, stats, layer, original);
}

void __fastcall SortPrimitives(void* context, CSceneLayer* layer, CMeshPrimitive* entries, std::uint32_t count) {
	static const auto original = GetOriginal(&SortPrimitives);
	original(context, layer, entries, count);

	ColoredModels->OnSortPrimitives(layer, entries, count);
}

void __fastcall RenderLayerDrawList(void* scene_system, void* draw_list, CSceneLayer* layer, void* batch_list, int start_flags, CMeshPrimitivePageList* pages) {
	static const auto original = GetOriginal(&RenderLayerDrawList);
	ColoredModels->OnRenderLayerDrawList(layer, pages);
	original(scene_system, draw_list, layer, batch_list, start_flags, pages);
}

void* __fastcall GeneratePrimitives(void* desc, CSceneAnimtableObject* object, void* a3, CMeshPrimitiveOutputBuffer* render_buf) {
	static const auto original = GetOriginal(&GeneratePrimitives);
	return ColoredModels->OnGeneratePrimitives(desc, object, a3, render_buf, original);
}

void* __fastcall LevelInit(void* a1, const char* a2) {
	static const auto original = GetOriginal(&LevelInit);

	if (!GlobalVars)
		GlobalVars = *Memory->Modules.Client.Find(xs("48 8B 1D ? ? ? ? FF 15 ? ? ? ? F3 44 0F 10 43 ? 8B C8 8B 43")).Rel(3, 7).As<CGlobalVariables**>();

	if (!Physics)
		Physics = *Memory->Modules.Client.Find(xs("48 8B 0D ? ? ? ? 48 8D 45 ? C6 44 24 ? ? 4C 8D 4C 24")).Rel(3, 7).As<CPhysics**>();

	if (!ParticleManager)
		ParticleManager = *Memory->Modules.Client.Find(xs("48 8B 0D ? ? ? ? 41 B8 ? ? ? ? F3 0F 11 74 24 ? 48 C7 44 24 ? ? ? ? ?")).Rel(3, 7).As<CParticleManager**>();

	World->SkyboxChanger.need_update = true;

	light_backup_count = 0;
	memset(light_backups, 0, sizeof light_backups);

	skybox_backup_count = 0;
	memset(skybox_backups, 0, sizeof skybox_backups);

	GrenadeTrajectory->Clear();
	GrenadeWarnings->Clear();

	return original(a1, a2);
}

void __fastcall LevelShutdown(void* a1) {
	static const auto original = GetOriginal(&LevelShutdown);

	World->SkyboxChanger.OnLevelShutdown();

	ColoredModels->Clear();
	EntityList->Clear();

	GlobalVars = nullptr;
	Physics = nullptr;
	ParticleManager = nullptr;

	light_backup_count = 0;
	memset(light_backups, 0, sizeof light_backups);

	skybox_backup_count = 0;
	memset(skybox_backups, 0, sizeof skybox_backups);

	GrenadeTrajectory->Clear();
	GrenadeWarnings->Clear();
	Particles->Clear();

	original(a1);

	Valve.DestroyRenderTarget();
}

void* __fastcall SetupSceneView(void* a1, CSceneView* view, CAggregateObject* data) {
	static const auto original = GetOriginal(&SetupSceneView);
	void* result = original(a1, view, data);

	if (!Config::Get<bool>(Vars::World::Enable) || !Config::Get<bool>(Vars::World::ModulateWorld))
		return result;

	if (!data || !data->scene_object_data || data->scene_object_data->index == -1 || data->scene_object_data->count == 0)
		return result;

	if (!SceneSystem || !SceneSystem->data || !SceneSystem->data->light)
		return result;

	Color_t color = Config::Get<Color_t>(Vars::World::WorldColor);
	const uintptr_t light_base = reinterpret_cast<uintptr_t>(SceneSystem->data->light);

	for (int i = 0; i < data->scene_object_data->count; i++) {
		int idx = (data->scene_object_data->index + i) << 5;
		*reinterpret_cast<Color_t*>(light_base + idx) = color;
	}

	return result;
}

void* __fastcall SetupLight(void* pointer, CLightScene* object, void* unk) {
	static const auto original = GetOriginal(&SetupLight);

	if (!object)
		return original(pointer, object, unk);

	static bool was_enabled = false;
	const bool enabled = Config::Get<bool>(Vars::World::Enable) && Config::Get<bool>(Vars::World::ModulateLight);
	const uintptr_t current_addr = reinterpret_cast<uintptr_t>(object);

	if (!enabled && was_enabled) {
		for (int i = 0; i < light_backup_count; i++) {
			if (!light_backups[i].valid)
				continue;

			CLightScene* light = reinterpret_cast<CLightScene*>(light_backups[i].address);
			if (light && light == object) {
				light->color.Value.x = light_backups[i].r;
				light->color.Value.y = light_backups[i].g;
				light->color.Value.z = light_backups[i].b;
				light->color.Value.w = light_backups[i].a;
			}
		}

		light_backup_count = 0;
		memset(light_backups, 0, sizeof(light_backups));
	}

	was_enabled = enabled;

	if (enabled) {
		int backup_index = -1;
		for (int i = 0; i < light_backup_count; i++) {
			if (light_backups[i].valid && light_backups[i].address == current_addr) {
				backup_index = i;
				break;
			}
		}

		if (backup_index == -1 && light_backup_count < 256) {
			backup_index = light_backup_count++;
			light_backups[backup_index].address = current_addr;
			light_backups[backup_index].valid = true;
			light_backups[backup_index].r = object->color.Value.x;
			light_backups[backup_index].g = object->color.Value.y;
			light_backups[backup_index].b = object->color.Value.z;
			light_backups[backup_index].a = object->color.Value.w;
		}

		if (backup_index != -1 && light_backups[backup_index].valid) {
			Color_t color = Config::Get<Color_t>(Vars::World::LightColor);

			object->color.Value.x = light_backups[backup_index].r * color.Rf();
			object->color.Value.y = light_backups[backup_index].g * color.Gf();
			object->color.Value.z = light_backups[backup_index].b * color.Bf();
			object->color.Value.w = light_backups[backup_index].a * color.Af();
		}
	}

	return original(pointer, object, unk);
}

void __fastcall BaseDrawArray(void* ptr, void* context, CMeshPrimitive* render_list, int nurenderables_to_draw, CSceneView* view, CSceneLayer* layer, void* a6) {
	static const auto original = GetOriginal(&BaseDrawArray);

	if (!Config::Get<bool>(Vars::World::Enable)) {
		original(ptr, context, render_list, nurenderables_to_draw, view, layer, a6);
		return;
	}

	const bool modulate_sun = Config::Get<bool>(Vars::World::ModulateSun);
	const bool modulate_clouds = Config::Get<bool>(Vars::World::ModulateClouds);
	const bool modulate_world = Config::Get<bool>(Vars::World::ModulateWorld);

	if (!modulate_sun && !modulate_clouds && !modulate_world) {
		original(ptr, context, render_list, nurenderables_to_draw, view, layer, a6);
		return;
	}

	const Color_t sun_color = Config::Get<Color_t>(Vars::World::SunColor);
	const Color_t clouds_color = Config::Get<Color_t>(Vars::World::CloudsColor);
	const Color_t world_color = Config::Get<Color_t>(Vars::World::WorldColor);

	for (int i = 0; i < nurenderables_to_draw; i++) {
		if (!render_list[i].material)
			continue;

		const std::string_view name = render_list[i].material->GetName();

		if (name.find(xs("sprays")) != std::string_view::npos || name.find(xs("stickers")) != std::string_view::npos || name.find(xs("patches")) != std::string_view::npos || name.find(xs("glass")) != std::string_view::npos || name.find(xs("blood")) != std::string_view::npos || name.find(xs("materials/models/weapons")) != std::string_view::npos || name.find(xs("weapons/models")) != std::string_view::npos || name.find(xs("characters/models")) != std::string_view::npos || name.find(xs("compmat_")) != std::string_view::npos || name.find(xs("_ignorez")) != std::string_view::npos)
			continue;

		if (modulate_sun && name.find(xs("sun")) != std::string_view::npos)
			render_list[i].color = sun_color;
		else if (modulate_clouds && name.find(xs("cloud")) != std::string_view::npos)
			render_list[i].color = clouds_color;
		else if (modulate_world)
			render_list[i].color = world_color;
	}

	original(ptr, context, render_list, nurenderables_to_draw, view, layer, a6);
}

void __fastcall SetupTonemap(void* a1, float a2, void* a3) {
	static const auto original = GetOriginal(&SetupTonemap);
	original(a1, a2, a3);

	float brightness = 1.f;
	if (Config::Get<bool>(Vars::World::Enable)) {
		const float exposure = Config::Get<float>(Vars::World::Exposure);
		if (exposure != 0.f)
			brightness = exposure / 100.f;
	}

	Memory->Vtable.Execute<void>(a1, 5, brightness);
}

void* __fastcall SetupSkyParams(uintptr_t a1, void* a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6, void* a7) {
	static const auto original = GetOriginal(&SetupSkyParams);

	if (!Config::Get<bool>(Vars::World::Enable))
		return original(a1, a2, a3, a4, a5, a6, a7);

	const std::vector<int>& removals = Config::Get<std::vector<int>>(Vars::World::Removals);

	cvar(r_draw3dskybox);
	cvar(r_drawskybox);

	bool backup_3d = false;
	bool restore_3d = false;
	if (r_draw3dskybox && removals[0]) {
		backup_3d = r_draw3dskybox->value.i1;
		r_draw3dskybox->value.i1 = false;
		restore_3d = true;
	}

	bool backup_2d = false;
	bool restore_2d = false;
	if (r_drawskybox && removals[1]) {
		backup_2d = r_drawskybox->value.i1;
		r_drawskybox->value.i1 = false;
		restore_2d = true;
	}

	void* result = original(a1, a2, a3, a4, a5, a6, a7);

	if (restore_3d)
		r_draw3dskybox->value.i1 = backup_3d;

	if (restore_2d)
		r_drawskybox->value.i1 = backup_2d;

	return result;
}

void* __fastcall UpdateShaderParam(void* a1, MM::Murmur2Hash_t hash, float* value) {
	static auto original = GetOriginal(&UpdateShaderParam);

	World->Wetness.OnUpdateShaderParam(hash, value);
	World->Bloom.OnUpdateShaderParam(hash, value);
	World->Gamma.OnUpdateShaderParam(hash, value);
	World->Dof.OnUpdateShaderParam(hash, value);

	return original(a1, hash, value);
}

void* __fastcall SetPostprocessVec(void* a1, MM::Murmur2Hash_t hash, float* value) {
	static const auto original = GetOriginal(&SetPostprocessVec);

	constexpr MM::Murmur2Hash_t dof_hash = 0x2ACAB07C;

	if (Config::Get<bool>(Vars::World::Enable) && Config::Get<bool>(Vars::World::DepthOfField)) {
		if (hash != dof_hash) {
			alignas(16) float dof_buffer[4] = { 0.f };

			World->Dof.OnUpdateShaderParam(dof_hash, dof_buffer);

			original(a1, dof_hash, dof_buffer);
		} else {
			World->Dof.OnUpdateShaderParam(hash, value);
		}
	}

	return original(a1, hash, value);
}

void* __fastcall UpdateShaderBool(__m128i* a1, MM::Murmur2Hash_t hash, bool value) {
	static const auto original = GetOriginal(&UpdateShaderBool);

	if (!Config::Get<bool>(Vars::World::Enable))
		return original(a1, hash, value);

	static const MM::Murmur2Hash_t direct_lighting = MM::HashA1Const<true>("directLighting", MM::Murmur2Seed);
	const std::vector<int>& removals = Config::Get<std::vector<int>>(Vars::World::Removals);

	if (hash == direct_lighting && removals.size() > 2 && removals[2])
		value = false;

	if (hash == 0xC6767B23) {
		if (Config::Get<bool>(Vars::World::Fullbright)) {
			value = false;

			void* vector_container = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(a1) - 272);

			const Color_t color = Config::Get<Color_t>(Vars::World::FullbrightColor);

			alignas(16) float color_vec[4] = {
				static_cast<float>(color.r) / 255.f,
				static_cast<float>(color.g) / 255.f,
				static_cast<float>(color.b) / 255.f,
				static_cast<float>(color.a) / 255.f
			};

			static const auto fn_set_vec = GetOriginal(&SetPostprocessVec);
			if (fn_set_vec)
				fn_set_vec(vector_container, 0xFF90C40E, color_vec);
		}
	}

	return original(a1, hash, value);
}

void __fastcall DrawSkyboxArray(void* thisptr, uintptr_t a2, uintptr_t mesh_array, int mesh_count, int a5, uintptr_t a6, uintptr_t a7) {
	static const auto original = GetOriginal(&DrawSkyboxArray);

	if (!mesh_array || mesh_count <= 0 || mesh_array < 0x10000) {
		original(thisptr, a2, mesh_array, mesh_count, a5, a6, a7);
		return;
	}

	const uintptr_t element_ptr = mesh_array + (static_cast<std::size_t>(mesh_count) * 0x70) - 0x58;
	if (element_ptr < 0x10000) {
		original(thisptr, a2, mesh_array, mesh_count, a5, a6, a7);
		return;
	}

	const auto skybox_object = *reinterpret_cast<CSkyboxObjectDesc**>(element_ptr);
	if (!skybox_object || reinterpret_cast<uintptr_t>(skybox_object) < 0x10000) {
		original(thisptr, a2, mesh_array, mesh_count, a5, a6, a7);
		return;
	}

	const uintptr_t current_addr = reinterpret_cast<uintptr_t>(skybox_object);

	int backup_index = -1;
	for (int i = 0; i < skybox_backup_count; i++) {
		if (skybox_backups[i].valid && skybox_backups[i].address == current_addr) {
			backup_index = i;
			break;
		}
	}

	if (backup_index == -1 && skybox_backup_count < 256) {
		backup_index = skybox_backup_count++;
		skybox_backups[backup_index].address = current_addr;
		skybox_backups[backup_index].r = skybox_object->color.r;
		skybox_backups[backup_index].g = skybox_object->color.g;
		skybox_backups[backup_index].b = skybox_object->color.b;
		skybox_backups[backup_index].a = skybox_object->color.a;
		skybox_backups[backup_index].material = nullptr;
		skybox_backups[backup_index].valid = true;

		if (skybox_object->material && reinterpret_cast<uintptr_t>(skybox_object->material) > 0x10000) {
			CMaterial2* mat = reinterpret_cast<CMaterial2*>(*skybox_object->material);
			if (mat && reinterpret_cast<uintptr_t>(mat) > 0x10000 && mat != World->SkyboxChanger.custom_material)
				skybox_backups[backup_index].material = mat;
		}
	} else if (backup_index != -1 && !skybox_backups[backup_index].material) {
		if (skybox_object->material && reinterpret_cast<uintptr_t>(skybox_object->material) > 0x10000) {
			CMaterial2* mat = reinterpret_cast<CMaterial2*>(*skybox_object->material);
			if (mat && reinterpret_cast<uintptr_t>(mat) > 0x10000 && mat != World->SkyboxChanger.custom_material)
				skybox_backups[backup_index].material = mat;
		}
	}

	const bool world_enabled = Config::Get<bool>(Vars::World::Enable);
	const bool modulate_enabled = world_enabled && Config::Get<bool>(Vars::World::ModulateSkybox);
	const bool changer_enabled = world_enabled && Config::Get<bool>(Vars::World::SkyboxChanger) && Config::Get<int>(Vars::World::SkyboxChangerType) != 0;

	if (modulate_enabled) {
		const auto color = Config::Get<Color_t>(Vars::World::SkyboxColor);
		skybox_object->color.r = color.Rf();
		skybox_object->color.g = color.Gf();
		skybox_object->color.b = color.Bf();
		skybox_object->color.a = color.Af();
	} else if (backup_index != -1 && skybox_backups[backup_index].valid) {
		skybox_object->color.r = skybox_backups[backup_index].r;
		skybox_object->color.g = skybox_backups[backup_index].g;
		skybox_object->color.b = skybox_backups[backup_index].b;
		skybox_object->color.a = skybox_backups[backup_index].a;
	}

	if (skybox_object->material && reinterpret_cast<uintptr_t>(skybox_object->material) > 0x10000) {
		if (changer_enabled && !World->SkyboxChanger.need_restore && World->SkyboxChanger.custom_material && reinterpret_cast<uintptr_t>(World->SkyboxChanger.custom_material) > 0x10000) {
			*skybox_object->material = reinterpret_cast<void*>(World->SkyboxChanger.custom_material);
		} else if (backup_index != -1 && skybox_backups[backup_index].valid && skybox_backups[backup_index].material) {
			*skybox_object->material = reinterpret_cast<void*>(skybox_backups[backup_index].material);
		}
	}

	if (World->SkyboxChanger.need_restore)
		World->SkyboxChanger.need_restore = false;

	original(thisptr, a2, mesh_array, mesh_count, a5, a6, a7);
}

void CalculateHudModelBob(void* pawn) { return; }

void* __fastcall DrawSmoke(void* a1, void* a2, int a3, int a4, void* a5, void* a6) {
	static auto original = GetOriginal(&DrawSmoke);

	if (Config::Get<std::vector<int>>(Vars::World::RemovalsPerspective)[remove_smoke])
		return nullptr;

	return original(a1, a2, a3, a4, a5, a6);
}

void* __fastcall DrawFlash(__int64 a1, int a2, __int64* a3, __int64 a4, float a5[4]) {
	static auto original = GetOriginal(&DrawFlash);

	if (Config::Get<std::vector<int>>(Vars::World::RemovalsPerspective)[remove_flash])
		return nullptr;

	return original(a1, a2, a3, a4, a5);
}

void* __fastcall DrawTorso(void* a1, void* a2, void* a3, void* a4, void* a5) {
	static auto original = GetOriginal(&DrawTorso);

	if (Config::Get<std::vector<int>>(Vars::World::RemovalsPerspective)[remove_torso])
		return nullptr;

	return original(a1, a2, a3, a4, a5);
}

void __fastcall DrawTeamIntro(void* a1, void* a2, bool* a3) {
	static auto original = GetOriginal(&DrawTeamIntro);

	if (Config::Get<std::vector<int>>(Vars::World::RemovalsPerspective)[remove_team_intro])
		return;

	return original(a1, a2, a3);
}

bool __fastcall DrawOverheadInfo(C_CSPlayerPawn* pawn) {
	static auto original = GetOriginal(&DrawOverheadInfo);

	if (!ctx.pawn || !pawn)
		return original(pawn);

	if (!ctx.pawn->Alive() && pawn->Enemy(ctx.pawn))
		return false;

	if (pawn == ctx.pawn && Config::Get<std::vector<int>>(Vars::World::Overhead)[overhead_remove_local])
		return false;

	if (!pawn->Enemy(ctx.pawn) && pawn != ctx.pawn && Config::Get<std::vector<int>>(Vars::World::Overhead)[overhead_remove_team])
		return false;

	return true;
}

void* __fastcall ViewPunch(CCSPlayer_CameraServices* a1, float* a2, float* a3, float* a4) {
	static auto original = GetOriginal(&ViewPunch);

	if (Config::Get<int>(Vars::World::VisualRecoil) == 1) {
		a1->m_vecCsViewPunchAngle() = Vector();
		a1->m_nCsViewPunchAngleTick() = -1;
		a1->m_flCsViewPunchAngleTickRatio() = 0.0f;
	}

	return original(a1, a2, a3, a4);
}

bool __fastcall DrawCrosshair(void* a1) {
	static auto original = GetOriginal(&DrawCrosshair);

	if (Config::Get<int>(Vars::World::Crosshair) == 1)
		return false;
	else if (Config::Get<int>(Vars::World::Crosshair) == 2) {
		if (ctx.pawn && !ctx.pawn->m_bIsScoped())
			return true;
	}

	return original(a1);
}

void* __fastcall RenderDecals(__int64 a1, __int64** a2, bool a3, bool a4) {
	static auto original = GetOriginal(&RenderDecals);

	if (Config::Get<std::vector<int>>(Vars::World::RemovalsPerspective)[remove_decals])
		return nullptr;

	return original(a1, a2, a3, a4);
}

uintptr_t __fastcall ServiceRead(uintptr_t a1) {
	static auto original = GetOriginal(&ServiceRead);

	const auto flags_len = *reinterpret_cast<uint32_t*>(a1 - 212);
	const auto len = flags_len & 0x3fffffff;

	std::string filename;
	if (len > 0 && len < 512) {
		char buf[512]{};
		if (flags_len & 0x40000000) {
			std::memcpy(buf, reinterpret_cast<void*>(a1 - 208), std::min(len, 512u - 1));
		} else {
			const auto str_ptr = *reinterpret_cast<uintptr_t*>(a1 - 208);
			if (str_ptr)
				std::memcpy(buf, reinterpret_cast<void*>(str_ptr), std::min(len, 512u - 1));
		}
		filename = buf;
	}

	std::replace(filename.begin(), filename.end(), '\\', '/');

	const unsigned char* target_data = nullptr;
	size_t target_size = 0;

	if (filename.find(xs("sounds/physics/")) != std::string::npos) {
		if (filename.find(xs("reverie.vsnd")) != std::string::npos) {
			target_data = vsnd::reverie;
			target_size = vsnd::reverie_size;
		} else if (filename.find(xs("rust_headshot.vsnd")) != std::string::npos) {
			target_data = vsnd::rust_headshot;
			target_size = vsnd::rust_headshot_size;
		} else if (filename.find(xs("sparkle.vsnd")) != std::string::npos) {
			target_data = vsnd::sparkle;
			target_size = vsnd::sparkle_size;
		} else if (filename.find(xs("primordial.vsnd")) != std::string::npos) {
			target_data = vsnd::primordial;
			target_size = vsnd::primordial_size;
		} else if (filename.find(xs("cod.vsnd")) != std::string::npos) {
			target_data = vsnd::cod;
			target_size = vsnd::cod_size;
		} else if (filename.find(xs("1_pig.vsnd")) != std::string::npos) {
			target_data = vsnd::pig;
			target_size = vsnd::pig_size;
		}
	} else if (filename.find(xs("particles/econ")) != std::string::npos) {
		if (filename.find(xs("soul.vpcf")) != std::string::npos) {
			target_data = vpcf::soul_visible;
			target_size = vpcf::soul_visible_size;
		} else if (filename.find(xs("soul_z.vpcf")) != std::string::npos) {
			target_data = vpcf::soul_invisible;
			target_size = vpcf::soul_invisible_size;
		} else if (filename.find(xs("tracer.vpcf")) != std::string::npos) {
			target_data = vpcf::tracer;
			target_size = vpcf::tracer_size;
		} else if (filename.find(xs("stars.vpcf")) != std::string::npos) {
			target_data = vpcf::stars;
			target_size = vpcf::stars_size;
		} else if (filename.find(xs("snow.vpcf")) != std::string::npos) {
			target_data = vpcf::snow;
			target_size = vpcf::snow_size;
		} else if (filename.find(xs("shot_sparks.vpcf")) != std::string::npos) {
			target_data = vpcf::shot_sparks;
			target_size = vpcf::shot_sparks_size;
		} else if (filename.find(xs("ash.vpcf")) != std::string::npos) {
			target_data = vpcf::ash;
			target_size = vpcf::ash_size;
		} else if (filename.find(xs("meteor.vpcf")) != std::string::npos) {
			target_data = vpcf::meteor;
			target_size = vpcf::meteor_size;
		}
	}

	if (target_data && target_size) {
		const auto async_fs = *reinterpret_cast<uintptr_t*>(a1 + 24);

		uintptr_t buffer = Memory->Vtable.Execute<uintptr_t>(reinterpret_cast<void*>(async_fs), 22, target_size, filename.c_str());
		if (buffer) {
			std::memcpy(reinterpret_cast<void*>(buffer), target_data, target_size);

			*reinterpret_cast<uintptr_t*>(a1 + 56) = buffer;
			*reinterpret_cast<uintptr_t*>(a1 + 64) = target_size;
			*reinterpret_cast<uintptr_t*>(a1 + 72) = target_size;

			using close_t = void(__fastcall*)(uintptr_t, int);
			static const close_t close = Memory->Modules.Filesystem.Find(xs("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC ? 48 8D 05")).As<close_t>();
			close(a1 - 224, 0);

			return 0;
		}
	}

	return original(a1);
}

void* __fastcall SetupViewmodelPosition(float* pos, float* angles, float* fov) {
	static const auto original = GetOriginal(&SetupViewmodelPosition);

	static struct {
		float x{ 0.f }, y{ 0.f }, z{ 0.f }, fov{ 68.f };
		std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();
	} animation_state;

	auto interpolate = [](float current, float target, float factor) {
		const float difference = target - current;
		if (std::abs(difference) < 0.001f)
			return target;

		return current + difference * factor;
	};

	using fn_t = void* (*)(float*, float*, float*);
	auto result = reinterpret_cast<fn_t>(original)(pos, angles, fov);

	if (Config::Get<bool>(Vars::World::Thirdperson)) {
		angles[0] += 300.f;
		return result;
	}

	const float delta_time = std::chrono::duration<float>(std::chrono::steady_clock::now() - animation_state.last_update).count();
	animation_state.last_update = std::chrono::steady_clock::now();

	const float smooth_factor = 12.0f * std::min(delta_time, 0.033f);

	if (Config::Get<bool>(Vars::World::ViewmodelChanger)) {
		animation_state.x = interpolate(animation_state.x, Config::Get<float>(Vars::World::ViewmodelChangerX), smooth_factor);
		animation_state.y = interpolate(animation_state.y, Config::Get<float>(Vars::World::ViewmodelChangerY), smooth_factor);
		animation_state.z = interpolate(animation_state.z, Config::Get<float>(Vars::World::ViewmodelChangerZ), smooth_factor);
		animation_state.fov = interpolate(animation_state.fov, static_cast<float>(Config::Get<int>(Vars::World::ViewmodelChangerFov)), smooth_factor);

		angles[0] += animation_state.x;
		angles[1] += animation_state.y;
		angles[2] += animation_state.z;
		*fov = animation_state.fov;
	}

	return result;
}

void* __fastcall SetupViewmodelRotation(void* a1, Vector* pos, Vector* rot) {
	static const auto original = GetOriginal(&SetupViewmodelRotation);

	static struct {
		float pitch{ 0.f }, yaw{ 0.f }, roll{ 0.f };
		std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();
	} animation_state;

	auto interpolate = [](float current, float target, float factor) {
		const float difference = target - current;
		if (std::abs(difference) < 0.001f)
			return target;

		return current + difference * factor;
	};

	const float delta_time = std::chrono::duration<float>(std::chrono::steady_clock::now() - animation_state.last_update).count();
	animation_state.last_update = std::chrono::steady_clock::now();

	const float smooth_factor = 12.0f * std::min(delta_time, 0.033f);

	if (Config::Get<bool>(Vars::World::ViewmodelChangerAngles)) {
		animation_state.pitch = interpolate(animation_state.pitch, Config::Get<float>(Vars::World::ViewmodelChangerPitch), smooth_factor);
		animation_state.yaw = interpolate(animation_state.yaw, Config::Get<float>(Vars::World::ViewmodelChangerYaw), smooth_factor);
		animation_state.roll = interpolate(animation_state.roll, Config::Get<float>(Vars::World::ViewmodelChangerRoll), smooth_factor);

		rot->x += animation_state.pitch;
		rot->y += animation_state.yaw;
		rot->z += animation_state.roll;

		while (rot->y > 180.f) rot->y -= 360.f;
		while (rot->y < -180.f) rot->y += 360.f;
	}

	if (Config::Get<bool>(Vars::World::ViewmodelChangerStatic))
		return nullptr;

	return original(a1, pos, rot);
}

bool Hooks_t::Fill() {
	dh_initialize();

	auto tier0 = reinterpret_cast<HMODULE>(Memory->Modules.Tier0.Handle());
	auto find_channel = reinterpret_cast<int(__fastcall*)(const char*)>(GetProcAddress(tier0, xs("LoggingSystem_FindChannel")));
	auto get_verbosity = reinterpret_cast<int(__fastcall*)(int)>(GetProcAddress(tier0, xs("LoggingSystem_GetChannelVerbosity")));
	setChannelVerbosity = reinterpret_cast<decltype(setChannelVerbosity)>(GetProcAddress(tier0, xs("LoggingSystem_SetChannelVerbosity")));

	if (find_channel && get_verbosity && setChannelVerbosity) {
		shootingChannel = find_channel(xs("Shooting"));
		if (shootingChannel >= 0) {
			shootingVerbosity = get_verbosity(shootingChannel);
			setChannelVerbosity(shootingChannel, 0);
		}
	}

	ColoredModels->Initialize();

	Hook(Memory->Vtable.Get<void*>(CSGOInput, 5), &CreateMove);
	Hook(Memory->Vtable.Get<void*>(Source2Client, 36), &FrameStage);
	Hook(Memory->Vtable.Get<void*>(CSGOInput, 23), &MouseInputEnabled);
	Hook(Memory->Vtable.Get<void*>(Inputs, 76), &IsRelativeMouseMode);
	Hook(Memory->Vtable.Get<void*>(SwapChain->dxgi_swapchain, 8), &Present);
	Hook(Memory->Vtable.Get<void*>(SwapChain->dxgi_swapchain, 13), &ResizeBuffers);
	Hook(Memory->Vtable.Get<void*>(ClientMode, 24), &LevelInit);
	Hook(Memory->Vtable.Get<void*>(ClientMode, 25), &LevelShutdown);

	Hook(Memory->Vtable.Get<void*>(SceneSystem->GetObject(xs("AnimatableSceneObjectDesc")), 4), &GeneratePrimitives);
	Hook(Memory->Vtable.Get<void*>(SceneSystem->GetObject(xs("AggregateSceneObjectDesc")), 7), &SetupSceneView);
	Hook(Memory->Vtable.Get<void*>(SceneSystem->GetObject(xs("BaseSceneObjectDesc")), 1), &BaseDrawArray);

	Hook(Memory->Modules.Client.Find(xs("48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 55 41 56 41 57 48 8D AC 24 90 FA FF FF 48 81 EC 70 06")).As<void*>(), &UpdateHitboxHistory);
	Hook(Memory->Modules.Client.Find(xs("48 8B 0D ? ? ? ? 4C 8D 05 ? ? ? ? 48 85 D2")).As<void*>(), &UpdateGlobalVars);
	Hook(Memory->Modules.Client.Find(xs("48 89 74 24 ? 57 48 83 EC ? 41 B9 ? ? ? ? 41 8B C0 41 23 C1 48 8B F2 41 83 F8 ? 48 8B F9 44 0F 45 C8 41 81 F9 ? ? ? ? 73 ? FF 81")).As<void*>(), &AddEntity);
	Hook(Memory->Modules.Client.Find(xs("48 89 74 24 ? 57 48 83 EC ? 41 B9 ? ? ? ? 41 8B C0 41 23 C1 48 8B F2 41 83 F8 ? 48 8B F9 44 0F 45 C8 41 81 F9 ? ? ? ? 73 ? FF 89")).As<void*>(), &RemoveEntity);
	Hook(Memory->Modules.Client.Find(xs("48 89 5C 24 18 48 89 6C 24 20 57 48 83 EC 50 48 89 74 24 60 49 8B F8 4C 89 74 24 68 48 8B EA")).As<void*>(), &SetGroundEntity);
	Hook(Memory->Modules.Client.Find(xs("48 89 5C 24 10 55 56 57 48 81 EC 80 00 00 00 F3 0F 10 82 D0 00 00 00 48 8B F2 F3 0F 5C 82 2C 01 00 00")).As<void*>(), &RecordLanding);
	Hook(Memory->Modules.Client.Find(xs("48 89 5C 24 18 56 57 41 56 48 83 EC 60 48 8D 05 ? ? ? ? 48 C7 44 24 28 7A 00 00 00")).As<void*>(), &ReflectPawnState);
	Hook(Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 4C 8B F1 48 8D 94 24")).As<void*>(), &RenderView);
	Hook(Memory->Modules.Client.Find(xs("40 55 53 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B D9 0F 29 B4 24")).As<void*>(), &EmitLandSound);
	Hook(Memory->Modules.Client.Find(xs("4C 8B DC 49 89 4B ? 53 55")).As<void*>(), &SetupView);
	Hook(Memory->Modules.Client.Find(xs("48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B B9 ? ? ? ? 48 8B F1")).As<void*>(), &UpdateFOVSensitivity);
	Hook(Memory->Modules.Client.Find(xs("48 8B C4 53 57 48 83 EC ? 48 8B FA")).As<void*>(), &DrawScopeOverlay);
	Hook(Memory->Modules.Client.Find(xs("40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 85 C0 74 ? 48 8B C8 48 83 C4")).As<void*>(), &SetupFov);
	Hook(Memory->Modules.Client.Find(xs("40 53 48 83 EC ? 8B 91 ? ? ? ? 83 FA ? 74")).As<void*>(), &IsHudModelArmsRendering);
	Hook(Memory->Modules.Client.Find(xs("40 53 44 8B 91 ? ? ? ? 4D 8B D8")).As<void*>(), &UpdateHudModelArms);
	Hook(Memory->Modules.Client.Find(xs("40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 33 C0 C6 83 ? ? ? ? ? 48 C7 83")).As<void*>(), &SendMove);
	Hook(Memory->Modules.Client.Find(xs("48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 66 0F 6E CA 49 8B F0 66 0F 70 C9 00 8B EA 48 8B F9 45 33 C9 48 8B C1 66 66 0F 1F 84 00 ? ? ? ? 66 0F 6F C1 4C 8D 15 ? ? ? ? ? ? ? ? 0F 50 C8 85 C9 75 ? 41 FF C1 48 83 C0 10 41 83 F9 02 72 ? 48 8B 87 ? ? ? ? 48 85 C0 74 ? 90 48 8D 48 ? 33 D2 66 66 0F 1F 84 00 ? ? ? ? 66 0F 6F C1 ? ? ? ? 44 0F 50 C0 45 85 C0 75 ? FF C2 48 83 C1 10 83 FA 01")).As<void*>(), &UpdateShaderParam);
	Hook(Memory->Modules.Client.Find(xs("4C 89 44 24 ? 55 53 56 57 41 55 41 56 48 8D AC 24")).As<void*>(), &SetupSkyParams);
	Hook(Memory->Modules.Client.Find(xs("48 8B C4 55 56 41 54 41 55 41 57")).As<void*>(), &CalculateHudModelBob);
	Hook(Memory->Modules.Client.Find(xs("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B 9C 24 ? ? ? ? 4D 8B F8")).As<void*>(), &DrawSmoke);
	Hook(Memory->Modules.Client.Find(xs("85 D2 0F 88 ? ? ? ? 48 89 4C 24 ? 55 56")).As<void*>(), &DrawFlash);
	Hook(Memory->Modules.Client.Find(xs("4C 8B DC 55 53 56 57 41 57")).As<void*>(), &DrawTorso);
	Hook(Memory->Modules.Client.Find(xs("44 88 4C 24 ? 55 53 41 54")).As<void*>(), &RenderDecals);
	Hook(Memory->Modules.Client.Find(xs("40 53 48 83 EC ? 48 8B D9 83 FA ? 75 ? 48 8B 0D ? ? ? ? 48 8D 54 24 ? 48 8B 01 FF 90 ? ? ? ? 8B 10")).As<void*>(), &DrawOverheadInfo);
	Hook(Memory->Modules.Client.Find(xs("48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 85 C0 0F 84")).As<void*>(), &DrawCrosshair);
	Hook(Memory->Modules.Client.Find(xs("40 55 56 57 41 56 41 57 48 8D 6C 24")).As<void*>(), &ViewPunch);
	Hook(Memory->Modules.Client.Find(xs("48 83 EC ? 45 0F B6 08")).As<void*>(), &DrawTeamIntro);
	Hook(Memory->Modules.Client.Find(xs("40 55 53 56 41 56 41 57 48 8B EC")).As<void*>(), &SetupViewmodelPosition);
	Hook(Memory->Modules.Client.Find(xs("48 8B C4 55 56 57 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B F9")).As<void*>(), &SetupViewmodelRotation);

	Hook(Memory->Modules.Scenesystem.Find(xs("45 85 C9 0F 84 ? ? ? ? 55")).As<void*>(), &SortPrimitives);
	Hook(Memory->Modules.Scenesystem.Find(xs("4C 89 4C 24 ? 4C 89 44 24 ? 48 89 54 24 ? 48 89 4C 24 ? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 41 F6 40 48 01")).As<void*>(), &RenderLayerDrawList);
	Hook(Memory->Modules.Scenesystem.Find(xs("4C 89 4C 24 20 4C 89 44 24 18 48 89 54 24 10 48 89 4C 24 08 55 53 56 57 41 55 41 56")).As<void*>(), &DrawScenePrimitive);
	Hook(Memory->Modules.Scenesystem.Find(xs("48 8B C4 4C 89 40 18 55 53 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 80 B9 ? ? ? ? 00")).As<void*>(), &CullSceneObjects);
	Hook(Memory->Modules.Scenesystem.Find(xs("4C 8B DA 4C 8B C9 48 85 C9 74 ? 0F 1F 44 00 00 45 0F B7 51 08 45 33 C0")).As<void*>(), &TestScenePvs);
	Hook(Memory->Modules.Scenesystem.Find(xs("45 85 C9 0F 8E ? ? ? ? 4C 8B DC")).As<void*>(), &DrawSkyboxArray);
	Hook(Memory->Modules.Scenesystem.Find(xs("48 89 54 24 ? 55 57 41 56 48 83 EC")).As<void*>(), &SetupLight);
	Hook(Memory->Modules.Scenesystem.Find(xs("40 53 48 81 EC ? ? ? ? F2 0F 10 51")).As<void*>(), &SetupTonemap);

	Hook(Memory->Modules.Rendersystem.Find(xs("8B 44 24 78 44 8B CD 48 8B 8B A0 0A 00 00 8B D7 89 44 24 28 8B 44 24 70 89 44 24 20 4C 8B 11 41 FF 92 A0 00 00 00")).Sub(0x109).As<void*>(), &DrawOccludedDx11);
	Hook(Memory->Modules.Rendersystem.Find(xs("C7 01 1F 80 18 00 89 71 04 89 69 08 44 89 71 0C 89 41 10 8B 44 24 78 89 41 14 48 83 83 20 04 00 00 18")).Sub(0x10A).As<void*>(), &DrawOccludedSoftware);

	Hook(Memory->Modules.Engine2.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 66 0F 6E CA 41 8B F8")).As<void*>(), &UpdateShaderBool);
	Hook(Memory->Modules.Engine2.Find(xs("48 8B C4 F3 0F 11 58 20 89 50 10 55 41 55 48 8D A8 A8 FE FF FF 48 81 EC 48 02 00 00")).As<void*>(), &AdjustCommandTime);
	Hook(Memory->Modules.Engine2.Find(xs("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 66 0F 6E CA 49 8B F8 66 0F 70 C9 ? 8B F2 48 8B D9 45 33 C9 48 8B C1 66 66 0F 1F 84 00 ? ? ? ? 66 0F 6F C1 4C 8D 15 ? ? ? ? 66 0F 76 00 0F 50 C8 85 C9 75 ? 41 FF C1 48 83 C0 ? 41 83 F9 ? 72 ? 48 8B 83")).As<void*>(), &SetPostprocessVec);

	Hook(Memory->Modules.Filesystem.Find(xs("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 48 8D 05")).As<void*>(), &ServiceRead);

	dh_enable_hook(DH_ALL_HOOKS);
	EntityList->ForceUpdate();
	return true;
}

void Hooks_t::Clear() {
	dh_disable_hook(DH_ALL_HOOKS);
	ThreadPool->Clear();
	Movement->m_bKeepDucked = false;
	EntityList->Clear();
	LagCompensation->Reset();
	Ragebot->Reset();
	dh_uninitialize();

	if (shootingChannel >= 0 && setChannelVerbosity)
		setChannelVerbosity(shootingChannel, shootingVerbosity);

	shootingChannel = -1;
	setChannelVerbosity = nullptr;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

long Hooks_t::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	Input->OnWndProc(msg, wparam, lparam);

	if (Input->IsKeyReleased(VK_INSERT) || Input->IsKeyReleased(VK_DELETE)) {
		CustomflowBridge::ToggleVisible();

		static auto rel = GetOriginal(&IsRelativeMouseMode);
		rel(Inputs, CustomflowBridge::Get().Visible() ? false : Var->winapi.active_mouse_input);
	}

	const bool menu_open = CustomflowBridge::Get().Visible();

	if (menu_open) {
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

		if ((wparam != 'R' && wparam != '1' && wparam != '2' && wparam != '3' && wparam != '4' && wparam != '5' && wparam != 'W' && wparam != 'A' && wparam != 'S' && wparam != 'D' && wparam != VK_SHIFT && wparam != VK_CONTROL && wparam != VK_TAB && wparam != VK_SPACE) || ImGui::GetIO().WantTextInput)
			return true;
	}

	if (menu_open && (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MOUSEMOVE))
		return false;

	return static_cast<long>(CallWindowProcW(Input->old_wnd_proc, hwnd, msg, wparam, lparam));
}
