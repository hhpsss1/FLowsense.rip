#pragma once

struct CParticles {
	struct PointParticle_t {
		uint32_t effect_index;
		float current_time;
		float particle_time_before_delete;
	};

	std::deque<PointParticle_t> stack_beams;
	std::vector<uint32_t> weather_index;

	void Add(const Vector&, const Vector&, Color_t, float, float);
	void Clear();

	void Soul(C_CSPlayerPawn*);
	void Weather();
	void ShotSparks(const Vector&);
};

inline CParticles* Particles = new CParticles();