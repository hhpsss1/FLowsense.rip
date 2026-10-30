#pragma once

struct CGrenadeWarnings {
	void OnPresent();
    void Clear();
private:
	struct BadgeState_t {
        float scale{};
        float current_progress{};
        float target_progress{};
        ImVec2 anchor{};
        ImVec2 target_anchor{};
        float arrow_angle{ 1.5707963f };
        float target_arrow_angle{ 1.5707963f };
        bool is_oof{};
        int icon_index{ 0 };
        char main_text[16]{};
        char sub_text[16]{};
        bool has_sub{};
        bool touched{};
        bool wants{};
	};

    float GetMaxDistance(CGrenadeTrajectory::EGrenadeType type) const;
    bool ProjectAnchor(Vector& world, C_CSPlayerPawn* local_pawn, ImVec2& anchor, float& arrow_angle, bool& is_oof) const;
    bool TraceClear(Vector& from, Vector& to, C_CSPlayerPawn* skip, C_BaseEntity* accept) const;
    float ComputeHeDamage(Vector& pos, C_CSPlayerPawn* target) const;
    float ComputeBestEnemyDamage(Vector& pos, C_CSPlayerPawn* local_pawn) const;
    void DrawBadge(ImDrawList* draw_list, BadgeState_t& state);

    std::unordered_map<void*, BadgeState_t> states{};
    std::unordered_map<void*, float> inferno_spawn_times{};
};

inline CGrenadeWarnings* GrenadeWarnings = new CGrenadeWarnings();