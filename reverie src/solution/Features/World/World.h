#pragma once

struct CWorld {
	void OnSetupView(CViewSetup* setup);
	Vector camera_origin{};

	void OnUpdateFOVSensitivity(C_CSPlayerPawn* pawn);

	struct ImpactRequest_t {
		Vector position;
		Color_t color;
		float size;
		double spawn_time;
		float duration;
	};

	std::vector<ImpactRequest_t> impact_requests;
	std::mutex impact_mutex;

	void AddBoxOverlay(const Vector& pos, float size, const Color_t& color, float duration);
	void OnPresent();

	float thirdperson_distance{ 0.0f };
	bool fov_sensitivity_cached_scoped{ false };
	float fov_sensitivity_cached_fov{ 0.0f };
	float fov_sensitivity_cached_value{ -1.0f };

	struct ScopeOverlay_t {
		void OnPresent();
	} ScopeOverlay;

	struct Wetness_t {
		void OnUpdateShaderParam(MM::Murmur2Hash_t, float*);
	} Wetness;

	struct Bloom_t {
		void OnUpdateShaderParam(MM::Murmur2Hash_t, float*);
	} Bloom;

	struct Gamma_t {
		void OnUpdateShaderParam(MM::Murmur2Hash_t, float*);
	} Gamma;

	struct Dof_t {
		void OnUpdateShaderParam(MM::Murmur2Hash_t, float*);
	} Dof;

	struct PenetrationCrosshair_t {
		void OnPresent();
	} PenetrationCrosshair;

	struct SkyboxChanger_t {
		void OnFrameStage();
		void OnLevelShutdown();

		bool need_update{ false };
		CMaterial2* default_material{ nullptr };
		CMaterial2* hook_default_material{ nullptr };
		CStrongHandle<CMaterial2> custom_material_handle{};
		CMaterial2* custom_material{ nullptr };
		bool has_custom_skybox{ false };
		bool need_restore{ false };
		int last_selected{ -1 };
		std::vector<std::string> custom_skyboxes;
	} SkyboxChanger;
};

inline CWorld* World = new CWorld();