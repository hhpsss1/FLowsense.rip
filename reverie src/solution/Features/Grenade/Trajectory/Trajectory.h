#pragma once

struct CGrenadeTrajectory {
    enum class EGrenadeType : uint8_t {
        unknown = 0,
        he,
        flash,
        smoke,
        molotov,
        incendiary,
        decoy
    };

    struct TrajectoryInfo_t {
        std::vector<Vector> points{};
        std::vector<Vector> bounces{};
        std::vector<size_t> bounce_indices;
        Vector end_pos{};
        float duration{};
        int end_tick{ -1 };
        bool valid{};
    };

    struct FlightGrenade_t {
        C_BaseEntity* entity{};
        EGrenadeType type{ EGrenadeType::unknown };
        uint32_t thrower_handle{};
        TrajectoryInfo_t info{};
        float throw_time{};
        float last_seen{};
        float detonate_time{};
        bool corrected{};
        bool detonated{};
    };

    struct DamageInfo_t {
        int damage{};
        bool is_lethal{};
        Vector position{};
    };

    void OnPresent();
    void OnCreateMove();

    void Clear();

    void UpdateInFlight(C_CSPlayerPawn* pawn);

    const std::vector<FlightGrenade_t>& GetInFlight() const { return this->in_flight; }
private:
    static constexpr float k_gravity_scale{ 0.4f };
    static constexpr float k_elasticity{ 0.45f };
    static constexpr float k_hull_size{ 2.0f };
    static constexpr float k_forward_offset{ 22.0f };
    static constexpr float k_pull_back{ 6.0f };
    static constexpr float k_velocity_inherit{ 1.25f };
    static constexpr float k_stop_speed_sq{ 400.0f };
    static constexpr float k_steep_bounce_speed_sq{ 96000.0f };
    static constexpr float k_steep_bounce_normal_z{ 0.7f };
    static constexpr int k_max_ticks{ 4096 };
    static constexpr int k_ticks_per_point{ 2 };
    static constexpr int k_max_bounces{ 20 };
    static constexpr float k_throw_cooldown{ 0.75f };
    static constexpr float k_missing_grace{ 0.1f };
    static constexpr float k_fade_duration{ 0.75f };

    void SetupThrow(C_CSPlayerPawn* local_pawn, C_BaseCSGrenade* weapon, Vector& origin, Vector& velocity);
    void Simulate(Vector start, Vector velocity, C_CSPlayerPawn* thrower_pawn, TrajectoryInfo_t& out);
    void StepSimulation(Vector& pos, Vector& vel, C_CSPlayerPawn* thrower_pawn, int sim_tick, bool& hit, bool& detonated, Vector& impact_pos);
    void ResolveCollision(CGameTrace& trace, Vector& pos, Vector& vel, C_CSPlayerPawn* thrower_pawn, int sim_tick, bool& detonated);
    void SetSimulationParams(EGrenadeType type);
    bool ShouldDetonate(Vector vel, int tick) const;

    static Vector ClipVelocity(Vector velocity, Vector normal, float overbounce);

    void UpdateWeaponProperties(C_BaseCSGrenade* weapon, CCSWeaponBaseVData* vdata);
    EGrenadeType GetTypeFromName(const char* name);

    void ComputeDamageAtEndpoint(TrajectoryInfo_t& traj, std::vector<DamageInfo_t>& damages, C_CSPlayerPawn* local_pawn, int* out_local_damage = nullptr);
    void RenderDamagePrediction(ImDrawList* draw_list, TrajectoryInfo_t& traj, C_CSPlayerPawn* local_pawn);
    void RenderPreview(ImDrawList* draw_list, C_CSPlayerPawn* local_pawn);
    void RenderTrajectory(ImDrawList* draw_list, TrajectoryInfo_t& traj, float alpha, bool is_thrown, C_CSPlayerPawn* local_pawn);

    EGrenadeType current_type{ EGrenadeType::unknown };
    float throw_velocity{ 750.0f };
    float detonate_time{ 1.5f };
    float velocity_threshold{ 0.1f };
    float sv_gravity{ 800.0f };
    float molotov_max_slope_z{ 0.7f };
    float molotov_detonate_time{ 2.f };

    std::vector<FlightGrenade_t> in_flight{};
    float last_throw_time{};
    bool should_preview{};
};

inline CGrenadeTrajectory* GrenadeTrajectory = new CGrenadeTrajectory();