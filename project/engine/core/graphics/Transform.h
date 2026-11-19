#pragma once

#include <Quaternion.h>
#include <Vector3.h>

struct EulerTransform {
	Vector3 scale;
	Vector3 rotation;
	Vector3 translation;
};

struct QuaternionTransform {
	Vector3 scale;
	Quaternion rotation;
	Vector3 translation;
};