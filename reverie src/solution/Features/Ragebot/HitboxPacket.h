#pragma once

#include "Ragebot.h"
#include <emmintrin.h>

namespace RageHitboxPacket {
inline __m128d Broadcast(double value) { return _mm_set1_pd(value); }
inline __m128d Negative(__m128d value) { return _mm_xor_pd(value, _mm_set1_pd(-0.)); }
inline __m128d Select(__m128d mask, __m128d yes, __m128d no) {
	return _mm_or_pd(_mm_and_pd(mask, yes), _mm_andnot_pd(mask, no));
}
inline __m128d Mask(unsigned bits) { return _mm_castsi128_pd(_mm_set_epi64x(-(int64_t)((bits >> 1) & 1), -(int64_t)(bits & 1))); }
inline __m128d Both(__m128d first, __m128d second) { return _mm_and_pd(first, second); }

struct Rays {
	Vector directions[2];
	__m128d x, y, z, length_squared;
	Rays(const Vector& first, const Vector& second) : directions{first, second} {
		x = _mm_set_pd(second.x, first.x);
		y = _mm_set_pd(second.y, first.y);
		z = _mm_set_pd(second.z, first.z);
		length_squared = _mm_add_pd(_mm_add_pd(_mm_mul_pd(x, x), _mm_mul_pd(y, y)), _mm_mul_pd(z, z));
	}
	__m128d Dot(double dx, double dy, double dz) const {
		return _mm_add_pd(_mm_add_pd(_mm_mul_pd(Broadcast(dx), x), _mm_mul_pd(Broadcast(dy), y)), _mm_mul_pd(Broadcast(dz), z));
	}
};

inline unsigned Capsule(const RageHitbox_t::RayData_t& shape, const Rays& rays, float (&fractions)[2], unsigned enabled) {
	const auto zero = _mm_setzero_pd(), one = Broadcast(1.), epsilon = Broadcast(1e-8);
	auto active = Both(Mask(enabled), _mm_cmpge_pd(rays.length_squared, epsilon));
	if (!_mm_movemask_pd(active))
		return 0;
	const auto axis_direction = rays.Dot(shape.axis.x, shape.axis.y, shape.axis.z);
	const auto origin_direction = rays.Dot(shape.offsets[0][0], shape.offsets[0][1], shape.offsets[0][2]);
	const auto axis_origin = Broadcast(shape.axis_origin), axis_length = Broadcast(shape.axis_length_squared);
	auto best = one;
	auto hit = zero;
	if (shape.axis_length_squared > 1e-8) {
		const auto inverse = Broadcast(shape.inverse_axis_length_squared);
		const auto quadratic = _mm_sub_pd(rays.length_squared, _mm_mul_pd(_mm_mul_pd(axis_direction, axis_direction), inverse));
		const auto half = _mm_sub_pd(origin_direction, _mm_mul_pd(_mm_mul_pd(axis_direction, axis_origin), inverse));
		const auto discriminant = _mm_sub_pd(_mm_mul_pd(half, half), _mm_mul_pd(quadratic, Broadcast(shape.cylinder_constant)));
		auto candidates = Both(active, Both(_mm_cmpgt_pd(quadratic, epsilon), _mm_cmpge_pd(discriminant, zero)));
		if (_mm_movemask_pd(candidates)) {
			const auto root = _mm_sqrt_pd(Select(_mm_cmplt_pd(discriminant, zero), zero, discriminant));
			for (int side = 0; side < 2; ++side) {
				const auto current = _mm_div_pd(_mm_add_pd(Negative(half), side ? root : Negative(root)), quadratic);
				const auto position = _mm_add_pd(axis_origin, _mm_mul_pd(axis_direction, current));
				auto accepted = Both(candidates, Both(Both(_mm_cmpge_pd(current, zero), _mm_cmplt_pd(current, best)), Both(_mm_cmpge_pd(position, zero), _mm_cmple_pd(position, axis_length))));
				best = Select(accepted, current, best);
				hit = _mm_or_pd(hit, accepted);
				candidates = _mm_andnot_pd(accepted, candidates);
				if (!side)
					active = _mm_andnot_pd(accepted, active);
				if (!_mm_movemask_pd(candidates))
					break;
			}
		}
	}
	for (int end = 0; end < (shape.axis_length_squared == 0. ? 1 : 2) && _mm_movemask_pd(active); ++end) {
		const auto half = end ? rays.Dot(shape.offsets[end][0], shape.offsets[end][1], shape.offsets[end][2]) : origin_direction;
		const auto discriminant = _mm_sub_pd(_mm_mul_pd(half, half), _mm_mul_pd(rays.length_squared, Broadcast(shape.cap_constants[end])));
		auto candidates = Both(active, _mm_cmpge_pd(discriminant, zero));
		if (!_mm_movemask_pd(candidates))
			continue;
		const auto root = _mm_sqrt_pd(Select(_mm_cmplt_pd(discriminant, zero), zero, discriminant));
		for (int side = 0; side < 2; ++side) {
			const auto current = _mm_div_pd(_mm_add_pd(Negative(half), side ? root : Negative(root)), rays.length_squared);
			auto accepted = Both(candidates, Both(_mm_cmpge_pd(current, zero), _mm_cmplt_pd(current, best)));
			if (shape.axis_length_squared > 1e-8) {
				const auto position = _mm_add_pd(axis_origin, _mm_mul_pd(axis_direction, current));
				accepted = Both(accepted, end ? _mm_cmpge_pd(position, axis_length) : _mm_cmple_pd(position, zero));
			}
			best = Select(accepted, current, best);
			hit = _mm_or_pd(hit, accepted);
			candidates = _mm_andnot_pd(accepted, candidates);
			if (!_mm_movemask_pd(candidates))
				break;
		}
	}
	unsigned bits = _mm_movemask_pd(hit);
	float converted[4];
	_mm_storeu_ps(converted, _mm_cvtpd_ps(best));
	for (int lane = 0; lane < 2; ++lane)
		if (bits & (1u << lane))
			fractions[lane] = converted[lane];
	return bits;
}

inline unsigned Intersect(const RageHitbox_t& shape, const RageHitbox_t::RayData_t& prepared, const Vector& eye, const Rays& rays, float (&fractions)[2], unsigned enabled = 3) {
	if (!shape.valid)
		return 0;
	const auto zero = _mm_setzero_pd();
	const auto projection = rays.Dot(prepared.relative_center.x, prepared.relative_center.y, prepared.relative_center.z);
	const auto distance = Broadcast(prepared.center_distance_squared), bound = Broadcast(shape.bound_radius_squared);
	const auto before = _mm_cmple_pd(projection, zero);
	const auto beyond = _mm_cmpge_pd(projection, rays.length_squared);
	const auto start_outside = Both(before, _mm_cmpgt_pd(distance, bound));
	const auto end_distance = _mm_add_pd(_mm_sub_pd(distance, _mm_mul_pd(Broadcast(2.), projection)), rays.length_squared);
	const auto end_outside = Both(_mm_andnot_pd(before, beyond), _mm_cmpgt_pd(end_distance, bound));
	const auto middle_distance = _mm_sub_pd(_mm_mul_pd(distance, rays.length_squared), _mm_mul_pd(projection, projection));
	const auto middle_outside = _mm_andnot_pd(_mm_or_pd(before, beyond), _mm_cmpgt_pd(middle_distance, _mm_mul_pd(bound, rays.length_squared)));
	const auto rejected = _mm_or_pd(_mm_or_pd(start_outside, end_outside), _mm_or_pd(middle_outside, _mm_cmplt_pd(rays.length_squared, Broadcast(1e-8))));
	enabled &= ~static_cast<unsigned>(_mm_movemask_pd(rejected));
	if (!enabled)
		return 0;
	if (shape.radius > 0.f) {
		if (enabled != 3) {
			int lane = enabled == 1 ? 0 : 1;
			RageHitbox_t::Ray_t ray(rays.directions[lane]);
			return RageHitbox_t::IntersectCapsule(eye, rays.directions[lane], shape.first, shape.last, shape.radius, fractions[lane], &prepared, &ray) ? enabled : 0;
		}
		return Capsule(prepared, rays, fractions, enabled);
	}
	unsigned hit = 0;
	for (int lane = 0; lane < 2; ++lane) {
		if ((enabled & (1u << lane)) && RageHitbox_t::IntersectBox(prepared.box_prepared ? prepared.box_origin : Math::TransformPoint(eye - shape.position, shape.inverse), Math::TransformPoint(rays.directions[lane], shape.inverse), shape.mins, shape.maxs, fractions[lane]))
			hit |= 1u << lane;
	}
	return hit;
}

inline float Hitchance(std::span<const RageHitbox_t> shapes, std::span<const RageHitbox_t::RayData_t> prepared,
	std::span<const size_t> desired, std::span<const size_t> others, const std::array<std::array<float, 2>, CSpreadCache::sample_count>& offsets,
	const Vector& eye, const Vector& forward, const Vector& right, const Vector& up, float required, float range) {
	if (desired.empty() || !std::isfinite(range) || range <= 0.f)
		return 0.f;
	const bool fixed = std::all_of(offsets.begin() + 1, offsets.end(), [&](const auto& offset) { return offset == offsets.front(); });
	int hits = 0;
	for (size_t sample = 0; sample < offsets.size(); sample += 2) {
		Vector deltas[2];
		for (int lane = 0; lane < 2; ++lane) {
			const auto& spread = offsets[sample + lane];
			Vector direction = forward + right * spread[0] + up * spread[1];
			deltas[lane] = direction * (range / direction.Length());
		}
		Rays rays(deltas[0], deltas[1]);
		float nearest[2] = {1.f, 1.f};
		size_t selected[2] = {shapes.size(), shapes.size()};
		for (size_t index : desired) {
			float fractions[2] = {1.f, 1.f};
			unsigned hit = Intersect(shapes[index], prepared[index], eye, rays, fractions);
			for (int lane = 0; lane < 2; ++lane) {
				if ((hit & (1u << lane)) && fractions[lane] < nearest[lane]) {
					nearest[lane] = fractions[lane];
					selected[lane] = index;
				}
			}
		}
		unsigned hit = static_cast<unsigned>(selected[0] != shapes.size()) | (static_cast<unsigned>(selected[1] != shapes.size()) << 1);
		for (size_t index : others) {
			if (!hit)
				break;
			float fractions[2] = {1.f, 1.f};
			unsigned blocked = Intersect(shapes[index], prepared[index], eye, rays, fractions, hit);
			for (int lane = 0; lane < 2; ++lane)
				if ((blocked & (1u << lane)) && (fractions[lane] < nearest[lane] || (fractions[lane] == nearest[lane] && index < selected[lane])))
					hit &= ~(1u << lane);
		}
		if (fixed)
			return (hit & 1) ? 1.f : 0.f;
		for (int lane = 0; lane < 2; ++lane) {
			hits += (hit >> lane) & 1;
			float lower = static_cast<float>(hits) / offsets.size();
			float upper = static_cast<float>(hits + offsets.size() - sample - lane - 1) / offsets.size();
			if (lower >= required) return lower;
			if (upper < required) return upper;
		}
	}
	return static_cast<float>(hits) / static_cast<float>(offsets.size());
}
}
