#pragma once

#include <Matrix4x4.h>
#include <Transform.h>
#include <string>
#include <vector>

// ノード
struct Node {
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node> children;
};