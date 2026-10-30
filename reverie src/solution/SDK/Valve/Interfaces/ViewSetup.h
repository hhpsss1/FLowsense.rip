#pragma once

struct CViewSetup {
	char pad0[0x450];
public:
	float ortho_left;
	float ortho_top;
	float ortho_right;
	float ortho_bottom;
private:
	char pa2[0x38];
public:
	float world_fov;
	float viewmodel_fov;
	Vector vec_origin;
private:
	char pad3[0xC];
public:
	Vector vec_angles;

	FindOffset(int, view_width, 0x440);
	FindOffset(int, view_height, 0x448);
	FindOffset(float, view_aspect_ratio, 0x4D4);
	FindOffset(uint8_t, view_aspect_flag, 0x551);
};