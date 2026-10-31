#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

struct alignas(8) CUniformRandomStreamState {
	using Constructor_t = CUniformRandomStreamState*(__fastcall*)(CUniformRandomStreamState*, int);
	using RandomFloat_t = float(__fastcall*)(CUniformRandomStreamState*, float, float);

	int32_t seed = 0;
	int32_t shuffle_value = 0;
	std::array<int32_t, 32> shuffle = {};
	uint32_t mutex = 0;
};

static_assert(sizeof(CUniformRandomStreamState) == 0x90);

class CSpreadCache {
  public:
	static constexpr size_t sample_count = 256;

  private:
	struct Sample_t {
		float a, bcos, bsin, c, dcos, dsin;
	};

	std::array<Sample_t, sample_count> samples = {};
	bool initialized = false;
	float last_inaccuracy = -1.f;
	float last_spread = -1.f;
	bool last_revolver = false;
	uint64_t revision = 0;

  public:
	std::array<std::array<float, 2>, sample_count> offsets = {};
	std::array<double, sample_count> SortedRadiusSquared = {};
	float maximum_radius = 0.f;
	uint64_t Revision() const { return revision; }

	template <typename Seed, typename Random>
	bool Update(float inaccuracy, float spread, bool revolver, Seed random_seed, Random random_float) {
		if (!std::isfinite(inaccuracy) || !std::isfinite(spread) || inaccuracy < 0.f || spread < 0.f)
			return false;

		if (!initialized) {
			constexpr float circle = 6.2831853071795864769f;
			for (size_t seed = 0; seed < samples.size(); ++seed) {
				CUniformRandomStreamState stream;
				random_seed(&stream, static_cast<int>(seed + 1));
				float a = random_float(&stream, 0.f, 1.f);
				float b = random_float(&stream, 0.f, circle);
				float c = random_float(&stream, 0.f, 1.f);
				float d = random_float(&stream, 0.f, circle);
				samples[seed] = {a, std::cos(b), std::sin(b), c, std::cos(d), std::sin(d)};
			}
			initialized = true;
		}

		if (last_inaccuracy == inaccuracy && last_spread == spread && last_revolver == revolver)
			return true;

		maximum_radius = 0.f;
		for (size_t seed = 0; seed < samples.size(); ++seed) {
			const auto& sample = samples[seed];
			float a = revolver ? 1.f - sample.a * sample.a : sample.a;
			float c = revolver ? 1.f - sample.c * sample.c : sample.c;
			float first = a * inaccuracy;
			float second = c * spread;
			offsets[seed] = {sample.bcos * first + sample.dcos * second, sample.bsin * first + sample.dsin * second};
			SortedRadiusSquared[seed] = static_cast<double>(offsets[seed][0]) * offsets[seed][0] +
				static_cast<double>(offsets[seed][1]) * offsets[seed][1];
			maximum_radius =
				std::max(maximum_radius, offsets[seed][0] * offsets[seed][0] + offsets[seed][1] * offsets[seed][1]);
		}
		maximum_radius = std::sqrt(maximum_radius);
		std::sort(SortedRadiusSquared.begin(), SortedRadiusSquared.end());
		last_inaccuracy = inaccuracy;
		last_spread = spread;
		last_revolver = revolver;
		++revision;
		return true;
	}
};
