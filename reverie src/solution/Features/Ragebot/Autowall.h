#pragma once

#include "../../Utils/Math/Math.h"

struct CTraceFilter;

struct C_CSPlayerPawn;
struct CCSWeaponBaseVData;
struct CTraceData;
struct CHitbox;

class CAutoWall {
  public:
	class Batch_t {
	  public:
		Batch_t();
		~Batch_t();
		Batch_t(const Batch_t&) = delete;
		Batch_t& operator=(const Batch_t&) = delete;
	};

	struct Result_t {
		const CHitbox* hitbox_data = nullptr;
		int hitgroup = -1;
		float damage = 0.f;
		bool penetrated = false;
	};

	struct DamageScale_t {
		float base_damage = 0.f;
		float head = 1.f;
		float body = 1.f;
		float headshot_multiplier = 1.f;
		float friendly = 1.f;
		float armor_ratio = 1.f;
		int armor = 0;
		bool helmet = false;
		bool head_only = false;
		bool immune = false;
		bool friendly_clamp = false;

		float Scale(float damage, int hitgroup) const {
			if (head_only && hitgroup != 1)
				return 0.f;
			switch (hitgroup) {
			case 1: damage = damage * headshot_multiplier * head; break;
			case 3: damage = damage * 1.25f * body; break;
			case 6:
			case 7: damage = damage * 0.75f * body; break;
			case 2:
			case 4:
			case 5:
			case 8: damage *= body; break;
			}
			if (friendly_clamp && damage / friendly >= 1.f)
				return 1.f;
			damage *= friendly;
			bool armored = hitgroup == 1 ? helmet : hitgroup == 0 || hitgroup == 2 || hitgroup == 3 || hitgroup == 4 || hitgroup == 5 || hitgroup == 8;
			if (!armored || armor <= 0)
				return damage;
			float health_damage = damage * armor_ratio;
			if ((damage - health_damage) * 0.5f > armor)
				health_damage = damage - armor / 0.5f;
			return health_damage;
		}
	};

	bool Run(const Vector& start, const Vector& end, C_CSPlayerPawn* target, Result_t& result, bool penetrate = true,
			 float maximum_range = 0.f, const CTraceFilter* prepared_filter = nullptr, const DamageScale_t* prepared_damage = nullptr);
	bool CanHit(const Vector& start, const Vector& direction, float& damage);
	DamageScale_t PrepareDamage(C_CSPlayerPawn* target);
	float DamageUpperBound(const DamageScale_t& scale, int hitgroup);

  private:
	bool Trace(CTraceData& data, const Vector& start, const Vector& end, C_CSPlayerPawn* target, int penetrations,
			   float maximum_range = 0.f, const CTraceFilter* prepared_filter = nullptr);
};

extern CAutoWall* AutoWall;
