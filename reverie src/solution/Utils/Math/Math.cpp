#include <shared.h>

float Vector::Normalize() {
	const float length = this->Length();
	const float radius = 1.0f / (length + FLT_EPSILON);

	this->x *= radius;
	this->y *= radius;
	this->z *= radius;

	return length;
}

float Vector::NormalizeLength() {
	float length = this->Length();

	if (length != 0.0f) {
		this->x /= length + std::numeric_limits<float>::epsilon();
		this->y /= length + std::numeric_limits<float>::epsilon();
		this->z /= length + std::numeric_limits<float>::epsilon();
	}

	return length;
}

Vector& Vector::NormalizeVector() {
	const float length = this->Length();
	if (length > 0.0f) [[likely]] {
		const float inv_len = 1.0f / length;
		*this *= inv_len;
	}

	return *this;
}

Vector Vector::Normalized() {
	auto vec = *this;
	vec.Normalize();
	return vec;
}

float Vector::Dot(const Vector& other, bool additional) {
	if (additional)
		return this->x * other.y + this->y * other.x + this->z * other.z;

	return this->x * other.x + this->y * other.y + this->z * other.z;
}

Vector Vector::Clamp() {
	if (this->x < -89.f)
		this->x = -89.f;

	if (this->x > 89.f)
		this->x = 89.f;

	while (this->y < -180.f)
		this->y += 360.f;

	while (this->y > 180.f)
		this->y -= 360.f;

	this->z = 0.f;

	return *this;
}

Vector Vector::Rotate(const Vector4D& orientation) const {
	Vector rotation_vec(orientation.x, orientation.y, orientation.z);
	Vector ct = rotation_vec.Cross(*this) * 2.0;

	return (*this) + (ct * orientation.w) + rotation_vec.Cross(ct);
}

void Vector2D::Normalize() {
	if (this->Length() > 0.f) [[likely]] {
		this->x /= this->Length();
		this->y /= this->Length();
	}
}

Vector2D Vector2D::Normalized() {
	if (this->Length() > 0.f) [[likely]]
		return *this / this->Length();

	return *this;
}

Matrix3x4_t Matrix2x4_t::ToMatrix3x4() const {
	const float scale = this->array[0][3];
	const float x = this->array[1][0];
	const float y = this->array[1][1];
	const float z = this->array[1][2];
	const float w = this->array[1][3];

	Matrix3x4_t matrix{};
	matrix[0][0] = scale * (1.f - 2.f * y * y - 2.f * z * z);
	matrix[1][0] = scale * (2.f * x * y + 2.f * w * z);
	matrix[2][0] = scale * (2.f * x * z - 2.f * w * y);
	matrix[0][1] = scale * (2.f * x * y - 2.f * w * z);
	matrix[1][1] = scale * (1.f - 2.f * x * x - 2.f * z * z);
	matrix[2][1] = scale * (2.f * y * z + 2.f * w * x);
	matrix[0][2] = scale * (2.f * x * z + 2.f * w * y);
	matrix[1][2] = scale * (2.f * y * z - 2.f * w * x);
	matrix[2][2] = scale * (1.f - 2.f * x * x - 2.f * y * y);
	matrix[0][3] = this->array[0][0];
	matrix[1][3] = this->array[0][1];
	matrix[2][3] = this->array[0][2];

	return matrix;
}

void Matrix2x4_t::SetOrigin(Vector vec) {
	this->array[0][0] = vec.x;
	this->array[0][1] = vec.y;
	this->array[0][2] = vec.z;
}

Vector Matrix2x4_t::GetOrigin(int index) {
	return Vector(this[index].array[0][0], this[index].array[0][1], this[index].array[0][2]);
}

void Math::Angle2Vectors(const Vector& angles, Vector* forward, Vector* right, Vector* up) {
	float sp, sy, sr, cp, cy, cr;

	sy = sin(Deg2Rad(angles.y));
	cy = cos(Deg2Rad(angles.y));
	sp = sin(Deg2Rad(angles.x));
	cp = cos(Deg2Rad(angles.x));
	sr = sin(Deg2Rad(angles.z));
	cr = cos(Deg2Rad(angles.z));

	forward->x = cp * cy;
	forward->y = cp * sy;
	forward->z = -sp;

	if (right) {
		right->x = -1 * sr * sp * cy + -1 * cr * -sy;
		right->y = -1 * sr * sp * sy + -1 * cr * cy;
		right->z = -1 * sr * cp;
	}

	if (up) {
		up->x = cr * sp * cy + -sr * -sy;
		up->y = cr * sp * sy + -sr * cy;
		up->z = cr * cp;
	}
}

float Math::NormalizeYaw(float yaw) {
	while (yaw > 180.0f)
		yaw -= 360.0f;

	while (yaw < -180.0f)
		yaw += 360.0f;

	return yaw;
}
