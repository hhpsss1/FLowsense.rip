#include <shared.h>
#include "../Utils/ThreadPool/ThreadPool.h"

void Valve_t::Fill() {
	SchemaSystem = Memory->Modules.Schema.FindInterface<CSchemaSystem*>(xs("SchemaSystem_001"));
	Prediction = Memory->Modules.Client.FindInterface<CPrediction*>(xs("Source2ClientPrediction001"));
	Inputs = Memory->Modules.Input.FindInterface<void*>(xs("InputSystemVersion001"));
	Cvar = Memory->Modules.Tier0.FindInterface<CCvar*>(xs("VEngineCvar007"));
	MaterialSystem = Memory->Modules.Materialsystem.FindInterface<CMaterialSystem*>(xs("VMaterialSystem2_001"));
	SceneSystem = Memory->Modules.Scenesystem.FindInterface<CSceneSystem*>(xs("SceneSystem_002"));
	Source2Client = Memory->Modules.Client.FindInterface<CSource2Client*>(xs("Source2Client002"));
	EngineClient = Memory->Modules.Engine2.FindInterface<CEngineToClient*>(xs("Source2EngineToClient001"));
	ParticleSystem = Memory->Modules.Particles.FindInterface<CParticleSystem*>(xs("ParticleSystemMgr003"));
	ResourceSystem = Memory->Modules.Resourcesystem.FindInterface<CResourceSystem*>(xs("ResourceSystem013"));
	FileSystem = Memory->Modules.Filesystem.FindInterface<CFileSystem*>(xs("VFileSystem017"));
	Localize = Memory->Modules.Localize.FindInterface<CLocalize*>(xs("Localize_001"));
	EnginePanorama = Memory->Modules.Panorama.FindInterface<CEnginePanorama*>(xs("PanoramaUIEngine001"));

	EntitySystem = *Memory->Modules.Client.Find(xs("48 8B 0D ? ? ? ? 4C 8D 05 ? ? ? ? 48 8D 54 24 ? E8")).Rel(3, 7).As<CEntitySystem**>();
	GlobalVars = *Memory->Modules.Client.Find(xs("48 8B 1D ? ? ? ? FF 15 ? ? ? ? F3 44 0F 10 43 ? 8B C8 8B 43")).Rel(3, 7).As<CGlobalVariables**>();
	CSGOInput = *Memory->Modules.Client.Find(xs("48 8B 0D ? ? ? ? 4C 8D 47 ? 8B 10")).Abs(3, 0).As<CCSGOInput**>();
	Physics = *Memory->Modules.Client.Find(xs("48 8B 0D ? ? ? ? 48 8D 45 ? C6 44 24 ? ? 4C 8D 4C 24")).Rel(3, 7).As<CPhysics**>();
	ClientMode = Memory->Modules.Client.Find(xs("48 8D 0D ? ? ? ? 48 69 C0 ? ? ? ? 48 03 C1 C3 CC CC")).Rel(3, 7).As<void*>();
	GameEventManager = *Memory->Modules.Client.Find(xs("48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 50 ? 4C 8B 05")).Rel(3, 7).As<CGameEventManager**>();
	ParticleManager = *Memory->Modules.Client.Find(xs("48 8B 0D ? ? ? ? 41 B8 ? ? ? ? F3 0F 11 74 24 ? 48 C7 44 24 ? ? ? ? ?")).Rel(3, 7).As<CParticleManager**>();

	if (uint8_t* ptr = Memory->Modules.Rendersystem.Find(xs("48 89 2D ? ? ? ? 66 0F 7F 05 ? ? ? ? FF 15")).Rel(3, 7).Deref().As<uint8_t*>()) {
		if (uint8_t* next_ptr = *reinterpret_cast<uint8_t**>(ptr)) {
			SwapChain = reinterpret_cast<CSwapChain*>(next_ptr);

			if (SwapChain && SwapChain->dxgi_swapchain) {
				if (SUCCEEDED(SwapChain->dxgi_swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&Device))))
					Device->GetImmediateContext(&DeviceCtx);
			}
		}
	}
}

void Valve_t::Clear() {
	this->DestroyRenderTarget();

	if (DeviceCtx) {
		DeviceCtx->Release();
		DeviceCtx = nullptr;
	}

	if (Device) {
		Device->Release();
		Device = nullptr;
	}

	SwapChain = nullptr;
	SchemaSystem = nullptr;
	EntitySystem = nullptr;
	GlobalVars = nullptr;
	Prediction = nullptr;
	Inputs = nullptr;
	Cvar = nullptr;
	Physics = nullptr;
	Source2Client = nullptr;
}

bool Ctx_t::Fill() {
	this->controller = EntitySystem->GetLocalController().As<CCSPlayerController*>();
	if (!this->controller)
		return false;

	this->pawn = EntitySystem->GetLocalPawn().As<C_CSPlayerPawn*>();
	if (!this->pawn)
		return false;

	this->weapon = this->pawn->GetActiveWeapon();
	this->weapon_data = this->weapon ? this->weapon->GetWeaponData() : nullptr;

	this->cmd_manager = this->controller->GetCommandManager();
	if (!this->cmd_manager)
		return false;

	this->cmd = this->cmd_manager->GetUserCommand();
	if (!this->cmd)
		return false;

	this->base_cmd = this->cmd->pb.mutable_base();
	if (!this->base_cmd)
		return false;

	GameRules = EntityList->game_rules_proxy && EntityList->game_rules_proxy->GetGameRules() ? EntityList->game_rules_proxy->GetGameRules() : nullptr;

	this->base_cmd->clear_subtick_moves();

	return true;
}

void Ctx_t::Clear() {
	this->base_cmd = nullptr;
	this->cmd = nullptr;
	this->cmd_manager = nullptr;
	this->weapon_data = nullptr;
	this->weapon = nullptr;
	this->controller = nullptr;
	this->pawn = nullptr;
}

void Entry_t::Fill(HMODULE m) {
	auto current_process = GetCurrentProcess();
	auto priority_class = GetPriorityClass(current_process);

	if (priority_class != HIGH_PRIORITY_CLASS && priority_class != REALTIME_PRIORITY_CLASS)
		SetPriorityClass(current_process, HIGH_PRIORITY_CLASS);

	while (!GetModuleHandleA(xs("navsystem.dll")))
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

	Logger->Attach(xs("Reverie Reborn (skinvvvalker rework)"));

	Memory->Modules.Fill();
	Valve.Fill();
	ThreadPool->Init();
	Input->Init();
	Skins->Init();
	Draw->Capture();
	Hooks->Fill();
	Config::Init();
	Listener->Init({ xs("bullet_impact"), xs("player_hurt"), xs("weapon_fire"), xs("player_death"), xs("round_start") });

	while (!GetAsyncKeyState(VK_END))
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

	Entry.Clear(m);
}

void Entry_t::Clear(HMODULE m) {
	Hooks->Clear();
	Input->Restore();

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	Draw->Clear();
	Valve.DestroyRenderTarget();
	ctx.Clear();
	Valve.Clear();
	Memory->Modules.Clear();
	Logger->Detach();
	Skins->Clear();
	Listener->Reset();

	FreeLibraryAndExitThread(m, 0);
}

void Valve_t::CreateRenderTarget() {
	this->DestroyRenderTarget();

	if (!SwapChain || !SwapChain->dxgi_swapchain || !Device)
		return;

	ID3D11Texture2D* back_buffer = nullptr;
	HRESULT hr = SwapChain->dxgi_swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	if (FAILED(hr) || !back_buffer)
		return;

	DXGI_SWAP_CHAIN_DESC sd;
	if (SUCCEEDED(SwapChain->dxgi_swapchain->GetDesc(&sd))) {
		D3D11_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = (sd.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM : sd.BufferDesc.Format;
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		hr = Device->CreateRenderTargetView(back_buffer, &desc, &Target);
		if (FAILED(hr)) {
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
			Device->CreateRenderTargetView(back_buffer, &desc, &Target);
		}
	}

	back_buffer->Release();
}

void Valve_t::DestroyRenderTarget() {
	if (Target) {
		Target->Release();
		Target = nullptr;
	}
}
