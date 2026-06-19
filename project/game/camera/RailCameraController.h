#pragma once

#include <memory>
#include <vector>
#include <string>

#include <Vector3.h>
#include <Vector2.h>

#include "WorldTransform.h"

class Camera;

class RailCameraController {
public:
	RailCameraController();
	~RailCameraController();
	void Initialize(Camera* camera, const std::vector<Vector3>& initialPoints, const std::string& levelFilename);

	void Update(bool activeController = true);

	void DrawDebugSpline();
	void SaveToJson();

public:
	const WorldTransform* GetWorldTransform() const { return &worldTransform_; }
	WorldTransform* GetWorldTransform() { return &worldTransform_; }

	const std::vector<Vector3>& GetControlPoints() const { return controlPoints_; }
	void SetControlPoints(const std::vector<Vector3>& points) { controlPoints_ = points; }

private:
	// スプライン計算用のヘルパー
	Vector3 EvaluateSpline(const std::vector<Vector3>& points, float time) const;
	Vector3 EvaluateSplineTangent(const std::vector<Vector3>& points, float time) const;
	Vector3 CatmullRomSpline(const std::vector<Vector3>& points, size_t index, float t) const;
	Vector3 CatmullRomTangent(const std::vector<Vector3>& points, size_t index, float t) const;

	// スクリーン投影ヘルパー
	bool WorldToScreen(const Vector3& worldPos, Vector2& outScreen) const;

private:
	// ワールド変換データ
	WorldTransform worldTransform_;

	// カメラ	
	Camera* camera_ = nullptr;

	// スプラインデータ
	std::vector<Vector3> controlPoints_;
	float splineTime_ = 0.0f;
	bool isPlaying_ = true;
	float speed_ = 0.005f;
	bool isLoop_ = true;

	std::string levelFilename_;

	// エディタ用状態
	int selectedPointIndex_ = -1;
	int draggedPointIndex_ = -1;
	bool isRightMouseDown_ = false;
	Vector2 rightMouseClickPos_;
};

