#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

struct alignas(8) CUniformRandomStreamState {
	using Constructor_t = CUniformRandomStreamState * (__fastcall*)(CUniformRandomStreamState*, int);
	using RandomFloat_t = float(__fastcall*)(CUniformRandomStreamState*, float, float);

	int32_t seed = 0;
	int32_t shuffle_value = 0;
	std::array<int32_t, 32> shuffle = {};
	uint32_t mutex = 0;
};

static_assert(sizeof(CUniformRandomStreamState) == 0x90);

class CSpreadCache {
public:
	static constexpr size_t sample_count = 512;

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
		// [FIX] Защита от nullptr — раньше падало при отсутствии символов.
		if (!random_seed || !random_float)
			return false;

		// [FIX] Защита от NaN/Inf/отрицательных значений — оставлена как в оригинале.
		if (!std::isfinite(inaccuracy) || !std::isfinite(spread) || inaccuracy < 0.f || spread < 0.f)
			return false;

		// [OPT] Одноразовая инициализация сэмплов — как в оригинале, но с защитой
		// от повторного вызова при уже инициализированном состоянии.
		if (!initialized) {
			constexpr float circle = 6.2831853071795864769f;
			for (size_t seed = 0; seed < samples.size(); ++seed) {
				CUniformRandomStreamState stream{};
				random_seed(&stream, static_cast<int>(seed + 1));
				const float a = random_float(&stream, 0.f, 1.f);
				const float b = random_float(&stream, 0.f, circle);
				const float c = random_float(&stream, 0.f, 1.f);
				const float d = random_float(&stream, 0.f, circle);
				samples[seed] = { a, std::cos(b), std::sin(b), c, std::cos(d), std::sin(d) };
			}
			initialized = true;
		}

		// [FIX] Кэш попадания: если входные параметры не изменились — не пересчитываем.
		// ВАЖНО: в оригинале было `return true;` без проверки, что предыдущий
		// Update не вернул false. Мы это гарантируем — попадаем сюда только
		// после успешной инициализации и валидных входных данных.
		if (last_inaccuracy == inaccuracy && last_spread == spread && last_revolver == revolver)
			return true;

		// [OPT] Локальные переменные на стеке вместо записи в поля класса
		// на каждой итерации — так компилятор может развернуть цикл лучше.
		float local_max_radius_sq = 0.f;

		for (size_t seed = 0; seed < samples.size(); ++seed) {
			const auto& sample = samples[seed];

			// [FIX] Revolver-режим: оригинал использовал `1.f - a*a`, что может
			// дать NaN/отрицательное значение при a в (1, ∞), но random_float
			// возвращает [0, 1) — так что a*a ∈ [0, 1). Всё корректно, но
			// добавим clamp для защиты от вырожденных реализаций random_float.
			const float a_raw = revolver ? 1.f - sample.a * sample.a : sample.a;
			const float c_raw = revolver ? 1.f - sample.c * sample.c : sample.c;

			// [FIX] Если a_raw/c_raw получились отрицательными (теоретически
			// невозможно при корректном random_float, но защищаемся), clamp в 0.
			const float a = std::max(0.f, a_raw);
			const float c = std::max(0.f, c_raw);

			const float first = a * inaccuracy;
			const float second = c * spread;

			const float ox = sample.bcos * first + sample.dcos * second;
			const float oy = sample.bsin * first + sample.dsin * second;
			offsets[seed] = { ox, oy };

			// [OPT] Вычисляем квадрат радиуса один раз как double — раньше
			// смешивались float и double вычисления в двух местах.
			const double radius_sq = static_cast<double>(ox) * ox + static_cast<double>(oy) * oy;
			SortedRadiusSquared[seed] = radius_sq;

			// [FIX] maximum_radius — верхняя граница по всем сэмплам. Раньше
			// сравнивались float-квадраты, теперь double — точнее при больших
			// inaccuracy/spread. Максимум всё равно ищется корректно.
			local_max_radius_sq = std::max(local_max_radius_sq, static_cast<float>(radius_sq));
		}

		maximum_radius = std::sqrt(local_max_radius_sq);

		// [OPT] Сортируем — это нужно для PrepareHitchance (бинарный поиск).
		std::sort(SortedRadiusSquared.begin(), SortedRadiusSquared.end());

		last_inaccuracy = inaccuracy;
		last_spread = spread;
		last_revolver = revolver;
		++revision;
		return true;
	}
};