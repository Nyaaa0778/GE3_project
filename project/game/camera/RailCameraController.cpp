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
	if (controlPoints_.size() < 4) {
		// 初期軌道がない場合はデフォルトのZ軸パスを作成
		controlPoints_ = {
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 50.0f },
			{ 0.0f, 0.0f, 100.0f },
			{ 0.0f, 0.0f, 150.0f }
		};
	}

	splineTime_ = 0.0f;
	isPlaying_ = true;
	isLoop_ = false;
	speed_ = 0.005f;

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
	if (isPlaying_ && n >= 4) {
		splineTime_ += speed_;
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
			worldTransform_.rotation = { pitch, yaw, 0.0f };
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

	// ------------------------------------
	// 3. エディタ操作入力処理
	// ------------------------------------
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 mousePos = io.MousePos;
	bool leftMouseDown = io.MouseDown[0];
	bool rightMouseClicked = io.MouseClicked[1];
	bool rightMouseReleased = io.MouseReleased[1];
	float clientWidth = static_cast<float>(DirectXCommon::GetInstance()->GetClientWidth());
	float clientHeight = static_cast<float>(DirectXCommon::GetInstance()->GetClientHeight());

	if (!io.WantCaptureMouse) {
		// 左クリックによる制御点選択＆ドラッグ移動
		if (leftMouseDown) {
			if (draggedPointIndex_ == -1) {
				// マウス座標に近いノードを選択
				float minDistance = 15.0f; // 許容範囲（ピクセル）
				int closestIndex = -1;
				for (size_t i = 0; i < controlPoints_.size(); ++i) {
					Vector2 screenPos;
					if (WorldToScreen(controlPoints_[i], screenPos)) {
						float dx = mousePos.x - screenPos.x;
						float dy = mousePos.y - screenPos.y;
						float dist = sqrtf(dx * dx + dy * dy);
						if (dist < minDistance) {
							minDistance = dist;
							closestIndex = static_cast<int>(i);
						}
					}
				}
				if (closestIndex != -1) {
					draggedPointIndex_ = closestIndex;
					selectedPointIndex_ = closestIndex;
				}
			} else {
				// 選択した制御点をドラッグ移動
				Vector3 camPos = camera_->GetTranslate();
				Vector3 pointPos = controlPoints_[draggedPointIndex_];
				float D = Length(pointPos - camPos);
				float factor = D * 2.0f * tanf(0.45f * 0.5f) / clientHeight;

				const Matrix4x4& camWorld = camera_->GetWorldMatrix();
				Vector3 right = { camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2] };
				Vector3 up = { camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2] };
				right = Normalize(right);
				up = Normalize(up);

				ImVec2 mouseDelta = io.MouseDelta;
				Vector3 move = right * (mouseDelta.x * factor) + up * (-mouseDelta.y * factor);
				controlPoints_[draggedPointIndex_] += move;
			}
		} else {
			draggedPointIndex_ = -1;
		}

		// 右クリックによる制御点追加（ドラッグ時は無視）
		if (rightMouseClicked) {
			isRightMouseDown_ = true;
			rightMouseClickPos_ = { mousePos.x, mousePos.y };
		}

		if (rightMouseReleased && isRightMouseDown_) {
			isRightMouseDown_ = false;
			float dx = mousePos.x - rightMouseClickPos_.x;
			float dy = mousePos.y - rightMouseClickPos_.y;
			float dragDist = sqrtf(dx * dx + dy * dy);

			// クリックだけであれば新規制御点追加を実行
			if (dragDist < 5.0f) {
				float ndcX = (2.0f * mousePos.x) / clientWidth - 1.0f;
				float ndcY = 1.0f - (2.0f * mousePos.y) / clientHeight;

				Matrix4x4 invVP = MakeInverseMatrix(camera_->GetViewProjectionMatrix());
				Vector3 nearPt = MathUtility::Transform({ ndcX, ndcY, 0.0f }, invVP);
				Vector3 farPt = MathUtility::Transform({ ndcX, ndcY, 1.0f }, invVP);
				Vector3 rayDir = Normalize(farPt - nearPt);
				Vector3 rayOrigin = camera_->GetTranslate();

				bool inserted = false;
				if (controlPoints_.size() >= 2) {
					// 既存スプライン軌道の近くをクリックした場合はそのセグメントに挿入
					float minRayDist = 5.0f; // ワールド空間の許容距離
					size_t bestSegment = 0;
					float bestT = 0.0f;

					const int numSamples = 200;
					float totalTime = static_cast<float>(controlPoints_.size() - 1);
					for (int s = 0; s <= numSamples; ++s) {
						float sampleTime = (static_cast<float>(s) / numSamples) * totalTime;
						Vector3 pt = EvaluateSpline(controlPoints_, sampleTime);

						Vector3 toPt = pt - rayOrigin;
						float proj = Dot(toPt, rayDir);
						if (proj > 0.0f) {
							Vector3 closestOnRay = rayOrigin + rayDir * proj;
							float d = Length(pt - closestOnRay);
							if (d < minRayDist) {
								minRayDist = d;
								bestSegment = static_cast<size_t>(sampleTime);
								bestT = sampleTime - static_cast<float>(bestSegment);
								inserted = true;
							}
						}
					}

					if (inserted) {
						Vector3 insertPos = EvaluateSpline(controlPoints_, static_cast<float>(bestSegment) + bestT);
						controlPoints_.insert(controlPoints_.begin() + bestSegment + 1, insertPos);
						selectedPointIndex_ = static_cast<int>(bestSegment + 1);
					}
				}

				if (!inserted) {
					// スプラインから遠い場合は末尾に追加
					float dist = 30.0f;
					if (!controlPoints_.empty()) {
						dist = Length(controlPoints_.back() - rayOrigin);
					}
					Vector3 newPt = rayOrigin + rayDir * dist;
					controlPoints_.push_back(newPt);
					selectedPointIndex_ = static_cast<int>(controlPoints_.size() - 1);
				}
			}
		}
	}

	// ------------------------------------
	// 4. ImGui コントロール表示
	// ------------------------------------
	ImGui::Begin("Spline Editor");

	ImGui::SeparatorText("Playback Controls");
	ImGui::Checkbox("Play Path", &isPlaying_);
	ImGui::Checkbox("Loop Path", &isLoop_);
	ImGui::SliderFloat("Speed", &speed_, 0.0001f, 0.02f, "%.5f");
	
	float maxT = (std::max)(0.0f, static_cast<float>(controlPoints_.size()) - 1.0f);
	if (ImGui::SliderFloat("Position Time", &splineTime_, 0.0f, maxT, "%.3f")) {
		if (!isPlaying_ && controlPoints_.size() >= 4) {
			Vector3 position = EvaluateSpline(controlPoints_, splineTime_);
			worldTransform_.translation = position;

			Vector3 tangent = EvaluateSplineTangent(controlPoints_, splineTime_);
			if (Length(tangent) > 0.0001f) {
				tangent = Normalize(tangent);
				float yaw = atan2f(tangent.x, tangent.z);
				float pitch = atan2f(-tangent.y, sqrtf(tangent.x * tangent.x + tangent.z * tangent.z));
				worldTransform_.rotation = { pitch, yaw, 0.0f };
			}
			worldTransform_.UpdateMatrix();
			if (activeController) {
				camera_->matWorld = worldTransform_.matWorld;
				camera_->matView = MakeInverseMatrix(worldTransform_.matWorld);
				camera_->UpdateViewProjection();
			}
		}
	}

	if (ImGui::Button("Reset to Start")) {
		splineTime_ = 0.0f;
		if (controlPoints_.size() >= 4) {
			worldTransform_.translation = controlPoints_[0];
			worldTransform_.UpdateMatrix();
			if (activeController) {
				camera_->matWorld = worldTransform_.matWorld;
				camera_->matView = MakeInverseMatrix(worldTransform_.matWorld);
				camera_->UpdateViewProjection();
			}
		}
	}

	ImGui::SeparatorText("Editor File Controls");
	if (ImGui::Button("Save Rail to JSON")) {
		SaveToJson();
	}
	ImGui::SameLine();
	ImGui::Text("File: %s.json", levelFilename_.c_str());

	ImGui::SeparatorText("Control Points");
	if (ImGui::Button("Add Point (Z-Offset)")) {
		Vector3 newPt = { 0.0f, 0.0f, 0.0f };
		if (!controlPoints_.empty()) {
			newPt = controlPoints_.back() + Vector3{ 0.0f, 0.0f, 20.0f };
		}
		controlPoints_.push_back(newPt);
		selectedPointIndex_ = static_cast<int>(controlPoints_.size() - 1);
	}

	ImGui::BeginChild("PointsList", ImVec2(0, 150), true);
	for (size_t i = 0; i < controlPoints_.size(); ++i) {
		std::string ptLabel = "Point " + std::to_string(i);
		bool isSelected = (static_cast<int>(i) == selectedPointIndex_);
		
		if (ImGui::Selectable(ptLabel.c_str(), isSelected)) {
			selectedPointIndex_ = static_cast<int>(i);
		}
		
		if (isSelected) {
			ImGui::Indent();
			ImGui::DragFloat3("Coord", &controlPoints_[i].x, 0.1f);
			
			if (ImGui::Button("Delete")) {
				controlPoints_.erase(controlPoints_.begin() + i);
				selectedPointIndex_ = -1;
				i--;
			} else {
				ImGui::SameLine();
				if (i > 0 && ImGui::Button("Up")) {
					std::swap(controlPoints_[i], controlPoints_[i - 1]);
					selectedPointIndex_ = static_cast<int>(i - 1);
				}
				ImGui::SameLine();
				if (i < controlPoints_.size() - 1 && ImGui::Button("Down")) {
					std::swap(controlPoints_[i], controlPoints_[i + 1]);
					selectedPointIndex_ = static_cast<int>(i + 1);
				}
			}
			ImGui::Unindent();
		}
	}
	ImGui::EndChild();

	ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Controls:\n- Left Drag node in viewport to Move\n- Right Click viewport to Insert/Add node");

	ImGui::End();
#endif
}

void RailCameraController::DrawDebugSpline() {
#ifdef USE_IMGUI
	if (controlPoints_.size() < 2) return;

	ImDrawList* drawList = ImGui::GetForegroundDrawList();

	// 1. スプライン軌跡の描画
	const int segments = 150;
	float totalTime = static_cast<float>(controlPoints_.size() - 1);
	Vector2 prevScreen = { 0.0f, 0.0f };
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
					3.0f
				);
			}
			prevScreen = screenPt;
			hasPrev = true;
		} else {
			hasPrev = false;
		}
	}

	// 2. 制御点（丸ノード）の描画
	for (size_t i = 0; i < controlPoints_.size(); ++i) {
		Vector2 screenPos;
		if (WorldToScreen(controlPoints_[i], screenPos)) {
			bool isSelected = (static_cast<int>(i) == selectedPointIndex_);
			bool isDragged = (static_cast<int>(i) == draggedPointIndex_);

			ImU32 color = IM_COL32(255, 165, 0, 255); // オレンジ
			float radius = 7.0f;

			if (isDragged) {
				color = IM_COL32(255, 0, 0, 255); // 赤
				radius = 11.0f;
			} else if (isSelected) {
				color = IM_COL32(255, 255, 0, 255); // 黄色
				radius = 9.0f;
			}

			drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), radius, color);
			drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), radius + 1.5f, IM_COL32(0, 0, 0, 255), 0, 1.5f);

			std::string label = "P" + std::to_string(i);
			drawList->AddText(ImVec2(screenPos.x + 10.0f, screenPos.y - 10.0f), IM_COL32(255, 255, 255, 255), label.c_str());
		}
	}
#endif
}

void RailCameraController::SaveToJson() {
	if (levelFilename_.empty()) return;

	std::string fullpath = "resources/levels/" + levelFilename_ + ".json";

	nlohmann::json deserialized;
	std::ifstream inFile(fullpath);
	if (inFile.is_open()) {
		try {
			inFile >> deserialized;
		} catch (const std::exception&) {
			deserialized["name"] = "scene";
			deserialized["objects"] = nlohmann::json::array();
		}
		inFile.close();
	} else {
		deserialized["name"] = "scene";
		deserialized["objects"] = nlohmann::json::array();
	}

	nlohmann::json splineArray = nlohmann::json::array();
	for (const auto& pt : controlPoints_) {
		splineArray.push_back({ pt.x, pt.y, pt.z });
	}
	deserialized["rail_spline"] = splineArray;

	std::ofstream outFile(fullpath);
	if (outFile.is_open()) {
		outFile << deserialized.dump(4);
		outFile.close();
	}
}

Vector3 RailCameraController::EvaluateSpline(const std::vector<Vector3>& points, float time) const {
	size_t n = points.size();
	if (n < 4) return {0.0f, 0.0f, 0.0f};

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
	if (n < 4) return {0.0f, 0.0f, 1.0f};

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
	if (n < 4) return {0.0f, 0.0f, 0.0f};

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
	if (n < 4) return {0.0f, 0.0f, 1.0f};

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

	Vector3 projected = MathUtility::Transform(worldPos, viewProj);
	outScreen.x = (projected.x + 1.0f) * 0.5f * clientWidth;
	outScreen.y = (1.0f - projected.y) * 0.5f * clientHeight;
	return true;
}