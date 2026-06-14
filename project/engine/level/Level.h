#pragma once

#include <vector>
#include <memory>
#include <string>
#include <Vector3.h>
#include "LevelData.h"
#include "Object3d.h"

class Camera;

class Level {
public:
	Level() = default;
	~Level() = default;

	// 初期化（静的オブジェクトの生成）
	void Initialize(LevelData* levelData, Camera* camera);

	// 更新
	void Update();

	// 描画
	void Draw();

	// カメラパラメータの適用
	void ApplyCameraParameters(Camera* camera) const;

	// ライトパラメータの適用
	void ApplyLightParameters() const;

	// 指定したエンティティタイプのスポーナー位置を取得
	Vector3 GetSpawnerPosition(const std::string& entityType, const Vector3& defaultPos = { 0.0f, 0.0f, 0.0f }) const;

	// 指定したエンティティタイプのスポーナー回転を取得
	Vector3 GetSpawnerRotation(const std::string& entityType, const Vector3& defaultRot = { 0.0f, 0.0f, 0.0f }) const;

	// 指定したエンティティタイプのスポーナーデータを取得
	const LevelData::SpawnerData* GetSpawner(const std::string& entityType) const;


private:
	// 静的配置モデル
	std::vector<std::unique_ptr<Object3d>> objects_;

	// スポーナーデータ
	std::vector<LevelData::SpawnerData> spawners_;

	// カメラデータ
	std::vector<LevelData::CameraData> cameras_;

	// ライトデータ
	std::vector<LevelData::LightData> lights_;
};
