#pragma once

#include <string>
#include <memory>
#include <json.hpp>

#include "LevelData.h"

class LevelLoader {
public:
	LevelLoader() = default;
	~LevelLoader() = default;

	std::unique_ptr<LevelData> Load(const std::string& filename);
	bool Save(const std::string& filename, const LevelData* levelData);

private:
	const std::string kDefaultBaseDirectory = "resources/levels/";
	const std::string kExtension = ".json";
private:
	void ParseObject(const nlohmann::json& objectJson, LevelData* levelData);
};

