#pragma once

struct Quaternion {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 1.0f;

	Quaternion() = default;
	Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};