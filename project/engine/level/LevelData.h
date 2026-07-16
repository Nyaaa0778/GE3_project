#pragma once

#include <string>
#include <vector>

#include <Vector3.h>
#include <Vector2.h>
#include <Vector4.h>

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

	struct SpawnerData {
		std::string entityType; 
		Vector3 translation;
		Vector3 rotation;
		Vector3 scaling = { 1.0f, 1.0f, 1.0f };
	};

	struct SpriteData {
		std::string filename;
		Vector2 translation;
		float rotation = 0.0f;
		Vector2 scaling = { 1.0f, 1.0f };
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	std::vector<ObjectData> objects;
	std::vector<SpawnerData> spawners;
	std::vector<CameraData> cameras;
	std::vector<LightData> lights;
	std::vector<SpriteData> sprites;
	std::vector<Vector3> railSpline;
};