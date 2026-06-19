#include "Level.h"
#include "Camera.h"
#include "LightManager.h"
#include <algorithm>

void Level::Initialize(LevelData* levelData, Camera* camera) {
	if (!levelData) {
		return;
	}

	// コピー可能なデータを保存
	spawners_ = levelData->spawners;
	cameras_ = levelData->cameras;
	lights_ = levelData->lights;

	// 静的配置モデルの生成
	for (const auto& objectData : levelData->objects) {
		if (objectData.filename.empty()) {
			continue;
		}

		auto newObj = std::make_unique<Object3d>();
		newObj->Initialize(objectData.filename);
		newObj->SetPosition(objectData.translation);
		newObj->SetRotation(objectData.rotation);
		newObj->SetScale(objectData.scaling);
		newObj->SetCamera(camera);

		objects_.push_back(std::move(newObj));
	}
}

void Level::Update() {
	for (auto& obj : objects_) {
		obj->Update();
	}
}

void Level::Draw() {
	for (auto& obj : objects_) {
		obj->Draw();
	}
}

void Level::ApplyCameraParameters(Camera* camera) const {
	if (!camera) {
		return;
	}

	if (!cameras_.empty()) {
		camera->SetTranslate(cameras_[0].translation);
		camera->SetRotate(cameras_[0].rotation);
	} else {
		// デフォルト値
		camera->SetRotate({ 0.3f, 0.0f, 0.0f });
		camera->SetTranslate({ 0.0f, 6.0f, -20.0f });
	}
}

void Level::ApplyLightParameters() const {
	if (!lights_.empty()) {
		const auto& lightData = lights_[0];

		// 点光源の設定 (0番目)
		LightManager::GetInstance()->SetPointLightPosition(0, lightData.translation);
		LightManager::GetInstance()->SetPointLightIntensity(0, 1.0f);
		LightManager::GetInstance()->SetPointLightDistance(0, 50.0f);
		LightManager::GetInstance()->SetPointLightEnabled(0, true);
	}
}

Vector3 Level::GetSpawnerPosition(const std::string& entityType, const Vector3& defaultPos) const {
	const auto* spawner = GetSpawner(entityType);
	if (spawner->entityType.empty()) {
		return defaultPos;
	}
	return spawner->translation;
}

Vector3 Level::GetSpawnerRotation(const std::string& entityType, const Vector3& defaultRot) const {
	const auto* spawner = GetSpawner(entityType);
	if (spawner->entityType.empty()) {
		return defaultRot;
	}
	return spawner->rotation;
}

const LevelData::SpawnerData* Level::GetSpawner(const std::string& entityType) const {
	static const LevelData::SpawnerData defaultSpawner = {
		"",
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 1.0f, 1.0f, 1.0f }
	};
	for (const auto& spawner : spawners_) {
		if (spawner.entityType.find(entityType) != std::string::npos) {
			return &spawner;
		}
	}
	return &defaultSpawner;
}

std::vector<LevelData::SpawnerData> Level::GetSpawners(const std::string& entityType) const {
	std::vector<LevelData::SpawnerData> result;
	for (const auto& spawner : spawners_) {
		if (spawner.entityType.find(entityType) != std::string::npos) {
			result.push_back(spawner);
		}
	}
	return result;
}

