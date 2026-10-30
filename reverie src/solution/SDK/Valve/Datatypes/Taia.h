#pragma once

struct Taia_t {
	int tick;
	float fraction;

	Taia_t() = default;
	Taia_t(int tick, const float fraction) : tick(tick), fraction(fraction) {}

	Taia_t Shifted(float ticks) const {
		Taia_t offset(0, ticks);
		offset.Normalize();
		return *this + offset;
	}

	void Init(int t, float f) {
		this->tick = t;
		this->fraction = f;
	}

	static int Compare(const Taia_t* a, const Taia_t* b) {
		if (a->tick == b->tick) {
			if (a->fraction == b->fraction)
				return 0;

			return (a->fraction > b->fraction) ? 1 : -1;
		}

		return (a->tick > b->tick) ? 1 : -1;
	}

	static void Minus(const Taia_t* a, Taia_t* out, const Taia_t* b) {
		out->tick = a->tick - b->tick;
		out->fraction = a->fraction - b->fraction;

		if (out->fraction < 0.0f) {
			out->fraction += 1.0f;
			if (out->fraction < 1.f)
				out->tick--;
			else
				out->fraction = 0.f;
		}
	}

	Taia_t GetFractionalTime(float f) const {
		Taia_t result;
		float whole = 0.f;
		float fractional = std::modf(f, &whole);

		auto tick = static_cast<int>(whole);

		if (fractional < 0.f) {
			fractional += 1.f;

			if (fractional < 1.f)
				tick--;
			else
				fractional = 0.f;
		}

		Taia_t offset(tick, fractional);
		offset.Normalize();

		Taia_t::Minus(this, &result, &offset);

		return result;
	}

	Taia_t Normalized() {
		Normalize();
		return *this;
	}

	bool operator>(const Taia_t& o) const {
		if (this->tick == o.tick)
			return this->fraction > o.fraction;

		return this->tick > o.tick;
	}

	bool operator<=(const Taia_t& o) const { return !(*this > o); }
	bool operator>=(const Taia_t& o) const { return !(*this < o); }

	void Normalize() {
		float whole = 0.f;
		fraction = std::modf(fraction, &whole);
		tick += static_cast<int>(whole);
		if (fraction < 0.f) {
			fraction += 1.f;
			--tick;
		}
		if (fraction >= 1.f) {
			fraction = 0.f;
			++tick;
		}
	}

	Taia_t operator-(const Taia_t& other) const {
		Taia_t result;
		Minus(this, &result, &other);
		return result;
	}

	Taia_t operator+(const Taia_t& other) const {
		const auto frac_temp = this->fraction + other.fraction;
		auto frac = frac_temp;
		if (frac_temp >= 1.f)
			frac -= 1.f;

		auto t = this->tick + other.tick + 1;
		if (frac_temp < 1.f)
			t = this->tick + other.tick;

		return { t, frac };
	}

	bool operator<(const Taia_t& other) const {
		return tick < other.tick || (tick == other.tick && fraction < other.fraction);
	}
};
