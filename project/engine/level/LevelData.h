#pragma once

#include <string>
#include <vector>

#include <Vector3.h>

struct LevelData {
	struct ObjectData {
		std::string filename;
		Vector3 translation;
		Vector3 rotation;
		Vector3 scaling;
	};

	struct CameraData {
		Vector3 translation;
		Vector3 rotation;
	};

	struct LightData {
		Vector3 translation;
		Vector3 rotation;
	};

	std::vector<ObjectData> objects;
	std::vector<CameraData> cameras;
	std::vector<LightData> lights;
};