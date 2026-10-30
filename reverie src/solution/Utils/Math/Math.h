#pragma once

#include <cmath>

struct Vector4D {
	Vector4D() = default;
	Vector4D(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	float x{ 0.f }, y{ 0.f }, z{ 0.f }, w{ 0.f };
};

struct Vector {
	Vector() = default;
	Vector(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector(float x, float y) : x(x), y(y), z(0.f) {}

	Vector operator-(const Vector& v) const { return Vector(this->x - v.x, this->y - v.y, this->z - v.z); }
	Vector operator+(const Vector& v) const { return Vector(this->x + v.x, this->y + v.y, this->z + v.z); }
	Vector operator*(const Vector& v) const { return Vector(this->x * v.x, this->y * v.y, this->z * v.z); }
	Vector operator/(float fl) const { return Vector(this->x / fl, this->y / fl, this->z / fl); }
	Vector operator*(float fl) const { return Vector(this->x * fl, this->y * fl, this->z * fl); }
	Vector operator-=(const Vector& v) { return Vector(this->x -= v.x, this->y -= v.y, this->z -= v.z); }
	Vector operator+=(const Vector& v) { return Vector(this->x += v.x, this->y += v.y, this->z += v.z); }
	Vector operator/=(float fl) { return Vector(this->x /= fl, this->y /= fl, this->z /= fl); }
	bool operator!=(const Vector& src) const { return (src.x != this->x) || (src.y != this->y) || (src.z != this->z); }
	float& operator[](int i) { return reinterpret_cast<float*>(this)[i]; }
	const float& operator[](int i) const { return reinterpret_cast<const float*>(this)[i]; }
	Vector& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
	Vector operator-() { return Vector(-this->x, -this->y, -this->z); }

	float LengthSqr() const { return this->x * this->x + this->y * this->y + this->z * this->z; }
	float Length() const { return std::sqrtf(this->LengthSqr()); }
	float Length2d() const { return std::sqrtf(this->x * this->x + this->y * this->y); }

	float DistTo(Vector& other) { return (*this - other).Length(); }

	Vector Lerp(const Vector& other, float ct) const { return Vector(this->x + ct * (other.x - this->x), this->y + ct * (other.y - this->y), this->z + ct * (other.z - this->z)); }

	Vector Cross(const Vector& other) const { return { this->y * other.z - this->z * other.y, this->z * other.x - this->x * other.z, this->x * other.y - this->y * other.x }; }
	Vector CrossProduct(const Vector& other) const { return this->Cross(other); }

	bool Zero() { return this->x == 0.f && this->y == 0.f && this->z == 0.f; }
	bool Valid() { return std::isfinite(this->x) && std::isfinite(this->y) && std::isfinite(this->z); }

	float Normalize();
	float NormalizeLength();
	Vector& NormalizeVector();
	Vector Normalized();

	float Dot(const Vector&, bool = false);

	Vector Clamp();
	Vector Rotate(const Vector4D&) const;

	float x{ 0.f }, y{ 0.f }, z{ 0.f };
};

struct Vector2D {
	Vector2D() = default;
	Vector2D(float x, float y) { this->x = x; this->y = y; }

	Vector2D operator+(const Vector2D& v) const { return Vector2D(this->x + v.x, this->y + v.y); }
	Vector2D operator-(const Vector2D& v) const { return Vector2D(this->x - v.x, this->y - v.y); }
	Vector2D operator*(float fl) const { return Vector2D(this->x * fl, this->y * fl); }
	Vector2D operator/(float fl) const { return Vector2D(this->x / fl, this->y / fl); }
	Vector2D& operator+=(const Vector2D& v) { this->x += v.x; this->y += v.y; return *this; }
	Vector2D& operator-=(const Vector2D& v) { this->x -= v.x; this->y -= v.y; return *this; }
	Vector2D& operator*=(float fl) { this->x *= fl; this->y *= fl; return *this; }
	Vector2D& operator/=(float fl) { this->x /= fl; this->y /= fl; return *this; }
	bool operator==(const Vector2D& v) const { return this->x == v.x && this->y == v.y; }
	bool operator!=(const Vector2D& v) const { return !(*this == v); }

	float LengthSqr() { return this->x * this->x + this->y * this->y; }
	float Length() { return std::sqrtf(this->LengthSqr()); }

	float DistToSqr(const Vector2D& v) const { return (*this - v).LengthSqr(); }
	float DistTo(const Vector2D& v) const { return (*this - v).Length(); }

	float Dot(const Vector2D& v) const { return this->x * v.x + this->y * v.y; }

	bool Zero() const { return this->x == 0.f && this->y == 0.f; }
	bool Valid() const { return std::isfinite(this->x) && std::isfinite(this->y); }

	void Normalize();
	Vector2D Normalized();

	float x{ 0.f }, y{ 0.f };
};

struct Matrix3x4_t;

struct Matrix2x4_t {
	Matrix2x4_t() = default;

	float array[2][4] = {};

	Matrix3x4_t ToMatrix3x4() const;
	Vector GetOrigin(int index);
	void SetOrigin(Vector vec);
};

struct Matrix3x4_t {
	Matrix3x4_t() = default;

	const float* operator[](int index) const noexcept { return this->array[index]; }
	float* operator[](int index) noexcept { return this->array[index]; }

	float array[3][4] = {};
};

struct Matrix4x4_t {
	Matrix4x4_t() = default;

	float array[16];
};

namespace Math {
	inline constexpr float _pi = 3.14159265358979323846f;
	inline constexpr float _pi2 = _pi * 2.f;
	inline constexpr float rads = 180.f / _pi;
	inline constexpr float degs = _pi / 180.f;

	inline constexpr float Rad2Deg(float r) { return r * rads; }
	inline constexpr float Deg2Rad(float d) { return d * degs; }

	inline Vector TransformPoint(const Vector& in, const Matrix3x4_t& mat) { return Vector(in.x * mat[0][0] + in.y * mat[0][1] + in.z * mat[0][2] + mat[0][3], in.x * mat[1][0] + in.y * mat[1][1] + in.z * mat[1][2] + mat[1][3], in.x * mat[2][0] + in.y * mat[2][1] + in.z * mat[2][2] + mat[2][3]); }

	void Angle2Vectors(const Vector& angles, Vector* forward, Vector* right = nullptr, Vector* up = nullptr);

	float NormalizeYaw(float);
}
