#include "DevelopEditor.h"
#include "WorldTransform.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "ShaderResourceViewManager.h"
#include "Input.h"
#include "Logger.h"
#include "../externals/imgui/imgui.h"
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <format>
#include <Windows.h>

std::unique_ptr<DevelopEditor> DevelopEditor::instance_ = nullptr;

#ifdef USE_IMGUI
struct AssetInfo {
	std::string name;
	std::string relativePath;
	std::string fullPath;
	std::string sizeStr;
};

static std::unordered_map<std::string, std::vector<AssetInfo>> sAssets;
static bool sAssetsLoaded = false;
static std::string sSelectedAssetPath = "";
static std::string sSelectedAssetSize = "";

static void LoadAssets() {
	if (sAssetsLoaded) return;
	sAssets.clear();

	std::vector<std::string> categories = { "fonts", "levels", "models", "shaders", "sounds", "sprites" };
	for (const auto& cat : categories) {
		std::string path = "resources/" + cat;
		if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
			for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
				if (entry.is_regular_file()) {
					AssetInfo info;
					info.name = entry.path().filename().string();
					info.relativePath = std::filesystem::relative(entry.path(), "resources").string();
					info.fullPath = entry.path().string();

					auto size = entry.file_size();
					if (size < 1024) {
						info.sizeStr = std::format("{} B", size);
					}
					else if (size < 1024 * 1024) {
						info.sizeStr = std::format("{:.1f} KB", size / 1024.0f);
					}
					else {
						info.sizeStr = std::format("{:.1f} MB", size / (1024.0f * 1024.0f));
					}
					sAssets[cat].push_back(info);
				}
			}
		}
	}
	sAssetsLoaded = true;
}

static int sSelectedEntityIndex = -1;

static void DrawInspectorWorldTransform(WorldTransform& transform) {
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		bool changed = false;
		changed |= ImGui::DragFloat3("Position", &transform.translation.x, 0.1f);

		Vector3 rotDegrees = {
			transform.rotation.x * 57.2957795f,
			transform.rotation.y * 57.2957795f,
			transform.rotation.z * 57.2957795f
		};
		if (ImGui::DragFloat3("Rotation", &rotDegrees.x, 1.0f)) {
			transform.rotation.x = rotDegrees.x * 0.0174532925f;
			transform.rotation.y = rotDegrees.y * 0.0174532925f;
			transform.rotation.z = rotDegrees.z * 0.0174532925f;
			changed = true;
		}
		changed |= ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f);

		if (changed) {
			transform.UpdateMatrix();
		}
	}
}
#endif

DevelopEditor* DevelopEditor::GetInstance() {
	if (instance_ == nullptr) {
		instance_.reset(new DevelopEditor());
	}
	return instance_.get();
}

void DevelopEditor::Finalize() {
	instance_.reset();
}

void DevelopEditor::Initialize() {
#ifdef USE_IMGUI
	LoadAssets();
#endif
}

void DevelopEditor::RegisterEntity(const std::string& name, WorldTransform* transform, std::function<void()> onInspect) {
	EditorEntity entity;
	entity.name = name;
	entity.transform = transform;
	entity.onInspect = onInspect;
	entities_.push_back(entity);
}

void DevelopEditor::RegisterCamera(const std::string& name, Camera* camera, std::function<void()> onInspect) {
	EditorEntity entity;
	entity.name = name;
	entity.camera = camera;
	entity.onInspect = onInspect;
	entities_.push_back(entity);
}

void DevelopEditor::ClearEntities() {
	entities_.clear();
}

void DevelopEditor::Update() {
#ifdef USE_IMGUI
	// F1キーでエディタモードのトグル
	if (Input::GetInstance()->TriggerKey(DIK_F1)) {
		isEditorMode_ = !isEditorMode_;
	}

	// エディタモードでない場合は、画面左上に簡易的な切り替えオーバーレイを表示
	if (!isEditorMode_) {
		ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.35f);
		if (ImGui::Begin("GameOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
			ImGui::Text("Game Mode (F1: Toggle Editor)");
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			if (ImGui::Button("Switch to Editor")) {
				isEditorMode_ = true;
			}
		}
		ImGui::End();
		return;
	}

	// 1. トップメニューバーの描画
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit", "Alt+F4")) {
				PostQuitMessage(0);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window")) {
			ImGui::MenuItem("Editor Mode", "F1", &isEditorMode_);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// 2. Hierarchy Window (Pos: 0, 20 / Size: 250, 400)
	ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_Always);
	ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (size_t i = 0; i < entities_.size(); ++i) {
			bool isSelected = (sSelectedEntityIndex == static_cast<int>(i));
			if (ImGui::Selectable(entities_[i].name.c_str(), isSelected)) {
				sSelectedEntityIndex = static_cast<int>(i);
				sSelectedAssetPath = ""; // アセット選択を解除
			}
		}
	}
	ImGui::End();

	// 3. Game View Window (Pos: 250, 20 / Size: 730, 400)
	ImGui::SetNextWindowPos(ImVec2(250, 20), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(730, 400), ImGuiCond_Always);
	ImGui::Begin("Game View", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

	// ツールバー (Play, Pause, Restart)
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
	if (isPaused_) {
		if (ImGui::Button("Play")) {
			isPaused_ = false;
		}
	}
	else {
		if (ImGui::Button("Pause")) {
			isPaused_ = true;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Restart")) {
		isPaused_ = false;
		// 再起動のシグナル（ splinetime を 0 に戻すなど ）は、各シーンが IsPaused の変化などをトリガーにして行うか、
		// あるいはシーン側で管理する変数にシグナルを送る
	}
	ImGui::SameLine();
	ImGui::Text(isPaused_ ? "Status: PAUSED" : "Status: PLAYING");
	ImGui::SameLine();
	ImGui::Text(" | FPS: %.1f", ImGui::GetIO().Framerate);

	ImGui::Separator();

	// RenderTextureを描画
	uint32_t rtexSrvIndex = DirectXCommon::GetInstance()->GetRenderTextureSrvIndex();
	D3D12_GPU_DESCRIPTOR_HANDLE rtexSrvHandle = ShaderResourceViewManager::GetInstance()->GetGPUDescriptorHandle(rtexSrvIndex);

	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	float aspect = 16.0f / 9.0f;
	float imgWidth = contentSize.x;
	float imgHeight = imgWidth / aspect;
	if (imgHeight > contentSize.y) {
		imgHeight = contentSize.y;
		imgWidth = imgHeight * aspect;
	}

	// アスペクト比を維持し中央寄せ
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (contentSize.x - imgWidth) * 0.5f);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (contentSize.y - imgHeight) * 0.5f);

	ImGui::Image((ImTextureID)rtexSrvHandle.ptr, ImVec2(imgWidth, imgHeight));

	ImGui::End();

	// 4. Inspector Window (Pos: 980, 20 / Size: 300, 700)
	ImGui::SetNextWindowPos(ImVec2(980, 20), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(300, 700), ImGuiCond_Always);
	ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	if (sSelectedEntityIndex == -1 && sSelectedAssetPath.empty()) {
		ImGui::Text("Hierarchy または Project から\nオブジェクトを選択してください。");
	}
	else if (!sSelectedAssetPath.empty()) {
		ImGui::Text("Name: %s", sSelectedAssetPath.substr(sSelectedAssetPath.find_last_of("\\/") + 1).c_str());
		ImGui::Separator();
		ImGui::Text("Type: Asset File");
		ImGui::Text("Relative Path: %s", sSelectedAssetPath.c_str());
		ImGui::Text("Size: %s", sSelectedAssetSize.c_str());
	}
	else {
		// 選択されたエンティティの詳細表示
		EditorEntity& entity = entities_[sSelectedEntityIndex];
		ImGui::Text("Name: %s", entity.name.c_str());
		ImGui::Separator();

		if (entity.transform) {
			DrawInspectorWorldTransform(*entity.transform);
		}
		else if (entity.camera) {
			Camera* camera = entity.camera;
			Vector3 pos = camera->GetTranslate();
			Vector3 rot = camera->GetRotate();
			bool changed = false;

			if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
				camera->SetTranslate(pos);
				changed = true;
			}

			Vector3 rotDegrees = {
				rot.x * 57.2957795f,
				rot.y * 57.2957795f,
				rot.z * 57.2957795f
			};
			if (ImGui::DragFloat3("Rotation", &rotDegrees.x, 1.0f)) {
				rot.x = rotDegrees.x * 0.0174532925f;
				rot.y = rotDegrees.y * 0.0174532925f;
				rot.z = rotDegrees.z * 0.0174532925f;
				camera->SetRotate(rot);
				changed = true;
			}

			if (changed) {
				camera->CalculateMatrix();
				camera->UpdateViewProjection();
			}
		}

		// カスタムのインスペクター描画コールバックがあれば呼ぶ
		if (entity.onInspect) {
			ImGui::Separator();
			entity.onInspect();
		}
	}
	ImGui::End();

	// 5. Console Window (Pos: 0, 420 / Size: 550, 300)
	ImGui::SetNextWindowPos(ImVec2(0, 420), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(550, 300), ImGuiCond_Always);
	ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	if (ImGui::Button("Clear")) {
		Logger::ClearLogs();
	}
	ImGui::SameLine();
	static bool autoScroll = true;
	ImGui::Checkbox("Auto-scroll", &autoScroll);
	ImGui::SameLine();
	static char searchFilter[128] = "";
	ImGui::InputText("Filter", searchFilter, IM_ARRAYSIZE(searchFilter));

	ImGui::Separator();

	const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);

	const auto& logs = Logger::GetLogs();
	for (const auto& logMsg : logs) {
		if (searchFilter[0] != '\0' && logMsg.find(searchFilter) == std::string::npos) {
			continue;
		}

		ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
		if (logMsg.find("Error") != std::string::npos || logMsg.find("Failed") != std::string::npos || logMsg.find("error") != std::string::npos) {
			color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
		}
		else if (logMsg.find("Warning") != std::string::npos || logMsg.find("warning") != std::string::npos) {
			color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
		}
		else if (logMsg.find("Initialized") != std::string::npos || logMsg.find("Succeeded") != std::string::npos) {
			color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
		}

		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(logMsg.c_str());
		ImGui::PopStyleColor();
	}

	if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();
	ImGui::End();

	// 6. Project Window (Pos: 550, 420 / Size: 430, 300)
	ImGui::SetNextWindowPos(ImVec2(550, 420), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(430, 300), ImGuiCond_Always);
	ImGui::Begin("Project", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	LoadAssets();

	if (ImGui::Button("Refresh")) {
		sAssetsLoaded = false;
		LoadAssets();
	}
	ImGui::SameLine();
	ImGui::Text("Assets List");

	ImGui::Separator();

	ImGui::BeginChild("AssetTree");
	for (auto& pair : sAssets) {
		const std::string& category = pair.first;
		const auto& fileList = pair.second;

		std::string headerLabel = std::format("{} ({} files)", category, fileList.size());
		if (ImGui::TreeNode(headerLabel.c_str())) {
			for (const auto& fileInfo : fileList) {
				ImGui::Bullet();
				bool isSelected = (sSelectedAssetPath == fileInfo.relativePath);
				if (ImGui::Selectable(fileInfo.name.c_str(), &isSelected)) {
					sSelectedEntityIndex = -1; // オブジェクト選択を解除
					sSelectedAssetPath = fileInfo.relativePath;
					sSelectedAssetSize = fileInfo.sizeStr;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Path: %s\nSize: %s", fileInfo.fullPath.c_str(), fileInfo.sizeStr.c_str());
				}
			}
			ImGui::TreePop();
		}
	}
	ImGui::EndChild();
	ImGui::End();
#endif
}
