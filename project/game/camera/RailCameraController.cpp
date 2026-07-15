#include "RailCameraController.h"

#include <MyEngine.h>
#include <MathUtility.h>
#include <cassert>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <json.hpp>
#include <imgui.h>

#include "Camera.h"
#include "DirectXCommon.h"
#include "Input.h"

using namespace MathUtility;

RailCameraController::RailCameraController() = default;
RailCameraController::~RailCameraController() = default;

void RailCameraController::Initialize(Camera* camera, const std::vector<Vector3>& initialPoints, const std::string& levelFilename) {
	assert(camera);
	camera_ = camera;
	levelFilename_ = levelFilename;

	// コントロールポイントの初期化
	controlPoints_ = initialPoints;
	if (controlPoints_.empty()) {
		// 初期軌道がない場合はデフォルトのZ軸パスを作成
		controlPoints_ = {
			{0.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 50.0f},
			{0.0f, 0.0f, 100.0f},
			{0.0f, 0.0f, 150.0f}
		};
	}

	splineTime_ = 0.0f;
	isPlaying_ = true;
	isLoop_ = false;
	speed_ = 0.08f;

	worldTransform_.Initialize();

	// 初期位置をスプライン開始点に合わせる
	if (!controlPoints_.empty()) {
		worldTransform_.translation = controlPoints_[0];
	} else {
		worldTransform_.translation = camera_->GetTranslate();
	}
	worldTransform_.rotation = camera_->GetRotate();
}

void RailCameraController::Update(bool activeController) {
	// ------------------------------------
	// 1. スプライン再生処理
	// ------------------------------------
	size_t n = controlPoints_.size();
	if (isPlaying_ && n >= 2) {
		// 接線の長さを基に進むべき媒介変数のステップ幅(dt)を決定する (物理速度を一定に保つための円弧長パラメータ化の近似)
		Vector3 tangentForSpeed = EvaluateSplineTangent(controlPoints_, splineTime_);
		float tangentLength = Length(tangentForSpeed);
		float timeStep = speed_;
		if (tangentLength > 0.0001f) {
			timeStep = speed_ / tangentLength;
			// 急激なワープを防ぐため、1フレームあたりの最大ステップを0.1にクランプ
			if (timeStep > 0.1f) {
				timeStep = 0.1f;
			}
		}
		splineTime_ += timeStep;

		float maxTime = static_cast<float>(n - 1);
		if (splineTime_ >= maxTime) {
			if (isLoop_) {
				splineTime_ = 0.0f;
			} else {
				splineTime_ = maxTime;
				isPlaying_ = false;
			}
		}

		Vector3 position = EvaluateSpline(controlPoints_, splineTime_);
		worldTransform_.translation = position;

		Vector3 tangent = EvaluateSplineTangent(controlPoints_, splineTime_);
		if (Length(tangent) > 0.0001f) {
			tangent = Normalize(tangent);
			float yaw = atan2f(tangent.x, tangent.z);
			float pitch = atan2f(-tangent.y, sqrtf(tangent.x * tangent.x + tangent.z * tangent.z));
			worldTransform_.rotation = {pitch, yaw, 0.0f};
		}
	}

	// ------------------------------------
	// 2. 行列計算とカメラ同期
	// ------------------------------------
	worldTransform_.UpdateMatrix();

	if (activeController) {
		camera_->matWorld = worldTransform_.matWorld;
		camera_->matView = MakeInverseMatrix(worldTransform_.matWorld);
		camera_->UpdateViewProjection();
	}
}

void RailCameraController::UpdateImGui() {
#ifdef USE_IMGUI

	// ------------------------------------
	// 4. ImGui コントロール表示
	// ------------------------------------
	ImGui::Begin("デバッグウィンドウ");

	ImGui::SeparatorText("Playback Controls");
	ImGui::Checkbox("Play Path", &isPlaying_);
	ImGui::Checkbox("Loop Path", &isLoop_);
	ImGui::SliderFloat("Speed", &speed_, 0.001f, 0.5f, "%.4f");

	float maxT = (std::max) (0.0f, static_cast<float>(controlPoints_.size()) - 1.0f);
	if (ImGui::SliderFloat("Position Time", &splineTime_, 0.0f, maxT, "%.3f")) {
		if (!isPlaying_ && controlPoints_.size() >= 2) {
			Vector3 position = EvaluateSpline(controlPoints_, splineTime_);
			worldTransform_.translation = position;

			Vector3 tangent = EvaluateSplineTangent(controlPoints_, splineTime_);
			if (Length(tangent) > 0.0001f) {
				tangent = Normalize(tangent);
				float yaw = atan2f(tangent.x, tangent.z);
				float pitch = atan2f(-tangent.y, sqrtf(tangent.x * tangent.x + tangent.z * tangent.z));
				worldTransform_.rotation = {pitch, yaw, 0.0f};
			}
			worldTransform_.UpdateMatrix();
			if (activeController_) {
				camera_->matWorld = worldTransform_.matWorld;
				camera_->matView = MakeInverseMatrix(worldTransform_.matWorld);
				camera_->UpdateViewProjection();
			}
		}
	}

	if (ImGui::Button("Reset to Start")) {
		splineTime_ = 0.0f;
		if (controlPoints_.size() >= 2) {
			worldTransform_.translation = controlPoints_[0];
			worldTransform_.UpdateMatrix();
			if (activeController_) {
				camera_->matWorld = worldTransform_.matWorld;
				camera_->matView = MakeInverseMatrix(worldTransform_.matWorld);
				camera_->UpdateViewProjection();
			}
		}
	}

	ImGui::End();
#endif
}

void RailCameraController::DrawDebugSpline() {
#ifdef USE_IMGUI
	if (controlPoints_.empty()) return;

	ImDrawList* drawList = ImGui::GetForegroundDrawList();

	// 1. スプライン軌跡の描画
	if (controlPoints_.size() >= 2) {
		const int segments = 150;
		float totalTime = static_cast<float>(controlPoints_.size() - 1);
		Vector2 prevScreen = {0.0f, 0.0f};
		bool hasPrev = false;

		for (int i = 0; i <= segments; ++i) {
			float t = (static_cast<float>(i) / static_cast<float>(segments)) * totalTime;
			Vector3 worldPt = EvaluateSpline(controlPoints_, t);
			Vector2 screenPt;
			if (WorldToScreen(worldPt, screenPt)) {
				if (hasPrev) {
					drawList->AddLine(
						ImVec2(prevScreen.x, prevScreen.y),
						ImVec2(screenPt.x, screenPt.y),
						IM_COL32(0, 255, 255, 255), // 水色
						2.0f
					);
				}
				prevScreen = screenPt;
				hasPrev = true;
			} else {
				hasPrev = false;
			}
		}
	} // if (controlPoints_.size() >= 2)

	// 2. 制御点（丸ノード）の描画
	for (size_t i = 0; i < controlPoints_.size(); ++i) {
		Vector2 screenPos;
		if (WorldToScreen(controlPoints_[i], screenPos)) {
			bool isSelected = (static_cast<int>(i) == selectedPointIndex_);
			bool isDragged = (static_cast<int>(i) == draggedPointIndex_);

			ImU32 color = IM_COL32(255, 165, 0, 255); // オレンジ
			float radius = 4.0f;

			if (isDragged) {
				color = IM_COL32(255, 0, 0, 255); // 赤
				radius = 8.0f;
			} else if (isSelected) {
				color = IM_COL32(255, 255, 0, 255); // 黄色
				radius = 6.0f;
			}

			drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), radius, color);
			drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), radius + 1.0f, IM_COL32(0, 0, 0, 255), 0, 1.0f);

			std::string label = "P" + std::to_string(i);
			drawList->AddText(ImVec2(screenPos.x + 8.0f, screenPos.y - 8.0f), IM_COL32(255, 255, 255, 255), label.c_str());
		}
	}
#endif
}
Vector3 RailCameraController::EvaluateSpline(const std::vector<Vector3>& points, float time) const {
	size_t n = points.size();
	if (n == 0) return {0.0f, 0.0f, 0.0f};
	if (n == 1) return points[0];

	size_t segmentIndex = static_cast<size_t>(time);
	float t = 0.0f;
	if (segmentIndex >= n - 1) {
		segmentIndex = n - 2;
		t = 1.0f;
	} else {
		t = time - static_cast<float>(segmentIndex);
	}

	return CatmullRomSpline(points, segmentIndex, t);
}

Vector3 RailCameraController::EvaluateSplineTangent(const std::vector<Vector3>& points, float time) const {
	size_t n = points.size();
	if (n == 0) return {0.0f, 0.0f, 1.0f};
	if (n == 1) return {0.0f, 0.0f, 1.0f};

	size_t segmentIndex = static_cast<size_t>(time);
	float t = 0.0f;
	if (segmentIndex >= n - 1) {
		segmentIndex = n - 2;
		t = 1.0f;
	} else {
		t = time - static_cast<float>(segmentIndex);
	}

	return CatmullRomTangent(points, segmentIndex, t);
}

Vector3 RailCameraController::CatmullRomSpline(const std::vector<Vector3>& points, size_t index, float t) const {
	size_t n = points.size();
	if (n == 0) return {0.0f, 0.0f, 0.0f};
	if (n == 1) return points[0];

	size_t i0 = (index == 0) ? 0 : index - 1;
	size_t i1 = index;
	size_t i2 = std::min(n - 1, index + 1);
	size_t i3 = std::min(n - 1, index + 2);

	const Vector3& p0 = points[i0];
	const Vector3& p1 = points[i1];
	const Vector3& p2 = points[i2];
	const Vector3& p3 = points[i3];

	float t2 = t * t;
	float t3 = t2 * t;

	Vector3 a = p1 * 2.0f;
	Vector3 b = (p2 - p0) * t;
	Vector3 c = (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2;
	Vector3 d = (p0 * -1.0f + p1 * 3.0f - p2 * 3.0f + p3) * t3;

	return (a + b + c + d) * 0.5f;
}

Vector3 RailCameraController::CatmullRomTangent(const std::vector<Vector3>& points, size_t index, float t) const {
	size_t n = points.size();
	if (n == 0) return {0.0f, 0.0f, 1.0f};
	if (n == 1) return {0.0f, 0.0f, 1.0f};

	size_t i0 = (index == 0) ? 0 : index - 1;
	size_t i1 = index;
	size_t i2 = std::min(n - 1, index + 1);
	size_t i3 = std::min(n - 1, index + 2);

	const Vector3& p0 = points[i0];
	const Vector3& p1 = points[i1];
	const Vector3& p2 = points[i2];
	const Vector3& p3 = points[i3];

	float t2 = t * t;

	Vector3 a = p2 - p0;
	Vector3 b = (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * (2.0f * t);
	Vector3 c = (p0 * -1.0f + p1 * 3.0f - p2 * 3.0f + p3) * (3.0f * t2);

	return (a + b + c) * 0.5f;
}

bool RailCameraController::WorldToScreen(const Vector3& worldPos, Vector2& outScreen) const {
	Matrix4x4 viewProj = camera_->GetViewProjectionMatrix();

	float clientWidth = static_cast<float>(DirectXCommon::GetInstance()->GetClientWidth());
	float clientHeight = static_cast<float>(DirectXCommon::GetInstance()->GetClientHeight());

	float w = worldPos.x * viewProj.m[0][3] + worldPos.y * viewProj.m[1][3] + worldPos.z * viewProj.m[2][3] + viewProj.m[3][3];
	if (w <= 0.0f) {
		return false;
	}

	Vector3 projected = TransformCoord(worldPos, viewProj);
	outScreen.x = (projected.x + 1.0f) * 0.5f * clientWidth;
	outScreen.y = (1.0f - projected.y) * 0.5f * clientHeight;
	return true;
}
