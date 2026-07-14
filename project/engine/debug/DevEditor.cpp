#include "DevEditor.h"

#ifdef USE_IMGUI
#include "DirectXCommon.h"
#include "ShaderResourceViewManager.h"
#include "Input.h"
#include "WorldTransform.h"
#include "SceneManager.h"
#include "IScene.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::unique_ptr<DevEditor> DevEditor::instance = nullptr;

DevEditor* DevEditor::GetInstance() {
	if (instance == nullptr) {
		// unique_ptrの作成にprivateコンストラクタを呼ぶためnewを使用
		instance = std::unique_ptr<DevEditor>(new DevEditor());
	}
	return instance.get();
}

void DevEditor::Finalize() {
	instance.reset();
}

void DevEditor::Initialize() {
	Log("[システム] 開発エディタが初期化されました。");
	Log("[システム] ゲーム中に F1 キーを押すことで、エディタモードとゲームモードを切り替えられます。");
	Log("[システム] レンダラー: DirectX12 (ImGuiドッキングシミュレーション)");

	// resourcesディレクトリ内をスキャンしてログ出力
	if (fs::exists("resources")) {
		Log("[システム] 'resources' フォルダをスキャン中...");
		for (const auto& entry : fs::recursive_directory_iterator("resources")) {
			if (entry.is_regular_file()) {
				std::string path = entry.path().string();
				std::replace(path.begin(), path.end(), '\\', '/');
				Log("[アセット] 検出: " + path);
			}
		}
	} else {
		Log("[警告] 'resources' フォルダが見つかりません。");
	}
}

void DevEditor::Update() {
	auto input = Input::GetInstance();
	// F1キーでエディタモードのトグル
	if (input->TriggerKey(DIK_F1)) {
		isEditorMode_ = !isEditorMode_;
		Log(isEditorMode_ ? "[システム] エディタモード有効" : "[システム] ゲームモード有効");
	}
}

void DevEditor::Draw() {
	if (!isEditorMode_) {
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	float W = io.DisplaySize.x;
	float H = io.DisplaySize.y;

	// レイアウト用の各ウインドウのサイズ計算
	float menuBarHeight = 25.0f;
	float hierarchyWidth = W * 0.20f;
	if (hierarchyWidth < 250.0f) hierarchyWidth = 250.0f;

	float inspectorWidth = W * 0.22f;
	if (inspectorWidth < 300.0f) inspectorWidth = 300.0f;

	float bottomHeight = 220.0f;

	float centerWidth = W - hierarchyWidth - inspectorWidth;
	float centerHeight = H - menuBarHeight - bottomHeight;

	// 1. トップメニューバー
	DrawMenuBar();

	// 2. ヒエラルキーウインドウ
	ImGui::SetNextWindowPos(ImVec2(0.0f, menuBarHeight));
	ImGui::SetNextWindowSize(ImVec2(hierarchyWidth, H - menuBarHeight - bottomHeight));
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	if (ImGui::Begin("HierarchyPanel", nullptr, windowFlags)) {
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ヒエラルキー");
		ImGui::Separator();
		DrawHierarchyWindow();
	}
	ImGui::End();

	// 3. インスペクターウインドウ
	ImGui::SetNextWindowPos(ImVec2(W - inspectorWidth, menuBarHeight));
	ImGui::SetNextWindowSize(ImVec2(inspectorWidth, H - menuBarHeight));
	if (ImGui::Begin("InspectorPanel", nullptr, windowFlags)) {
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "インスペクター");
		ImGui::Separator();
		DrawInspectorWindow();
	}
	ImGui::End();

	// 4. ゲームビューウインドウ (レンダーテクスチャをアスペクト比維持で描画)
	ImGui::SetNextWindowPos(ImVec2(hierarchyWidth, menuBarHeight));
	ImGui::SetNextWindowSize(ImVec2(centerWidth, centerHeight));
	ImGuiWindowFlags gameViewFlags = windowFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin("GameViewPanel", nullptr, gameViewFlags)) {
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ゲームビュー (レンダーターゲット)");
		ImGui::Separator();
		DrawGameViewWindow();
	}
	ImGui::End();

	// 5 & 6. 下部パネル（タブでConsoleとProjectを切り替え）
	ImGui::SetNextWindowPos(ImVec2(0.0f, H - bottomHeight));
	ImGui::SetNextWindowSize(ImVec2(W - inspectorWidth, bottomHeight));
	if (ImGui::Begin("BottomPanel", nullptr, windowFlags)) {
		if (ImGui::BeginTabBar("BottomTabBar")) {
			if (ImGui::BeginTabItem("コンソール")) {
				DrawConsoleWindow();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("プロジェクトアセット")) {
				DrawProjectWindow();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();

	ImGui::PopStyleVar(2);
}

void DevEditor::Log(const std::string& message) {
	logs_.push_back(message);
	if (logs_.size() > 500) {
		logs_.erase(logs_.begin());
	}
}

bool DevEditor::HierarchyNode(const char* label, void* id) {
	ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if (selectedId_ == id) {
		nodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	ImGui::TreeNodeEx(id, nodeFlags, "%s", label);
	if (ImGui::IsItemClicked()) {
		selectedId_ = id;
		selectedName_ = label;
		inspectorDrawer_ = nullptr; // コールバックはシーン側で再設定される
		return true;
	}
	return selectedId_ == id;
}

void DevEditor::SetInspectorDrawer(std::function<void()> drawer) {
	inspectorDrawer_ = drawer;
}

void DevEditor::DrawTransformEdit(WorldTransform* transform) {
	if (transform == nullptr) return;

	if (ImGui::CollapsingHeader("トランスフォーム", ImGuiTreeNodeFlags_DefaultOpen)) {
		bool changed = false;
		if (ImGui::DragFloat3("座標", &transform->translation.x, 0.1f)) {
			changed = true;
		}

		// ラジアンから度数法へ変換して表示・編集
		Vector3 degRot = {
			transform->rotation.x * 57.2957795f,
			transform->rotation.y * 57.2957795f,
			transform->rotation.z * 57.2957795f
		};
		if (ImGui::DragFloat3("回転", &degRot.x, 1.0f, -360.0f, 360.0f)) {
			transform->rotation = {
				degRot.x * 0.0174532925f,
				degRot.y * 0.0174532925f,
				degRot.z * 0.0174532925f
			};
			changed = true;
		}

		if (ImGui::DragFloat3("スケール", &transform->scale.x, 0.1f, 0.01f, 100.0f)) {
			changed = true;
		}

		if (changed) {
			transform->UpdateMatrix();
		}
	}
}

void DevEditor::DrawHierarchy(const char* title, std::function<void()> contentDrawer) {
	// シーン側からヒエラルキーの中身を受け取る
	hierarchyContentDrawer_ = contentDrawer;
}

void DevEditor::DrawMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		ImGui::Text("Unity風エディタ");
		ImGui::Separator();

		// シーン遷移メニュー
		if (ImGui::BeginMenu("シーン")) {
			if (ImGui::MenuItem("タイトルシーン (TITLE)")) {
				SceneManager::GetInstance()->ChangeScene("TITLE");
				Log("[システム] シーンが TITLE に変更されました");
			}
			if (ImGui::MenuItem("ゲームプレイシーン (GAMEPLAY)")) {
				SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
				Log("[システム] シーンが GAMEPLAY に変更されました");
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();

		// 再生・一時停止・コマ送りボタン
		ImGuiIO& io = ImGui::GetIO();
		float W = io.DisplaySize.x;
		ImGui::SameLine(W * 0.5f - 110.0f);

		if (isPaused_) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			if (ImGui::Button("▶ 再生", ImVec2(60, 0))) {
				isPaused_ = false;
				Log("[システム] ゲーム再生中");
			}
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.2f, 1.0f));
			if (ImGui::Button(" 実行中 ", ImVec2(70, 0))) {
				isPaused_ = true;
				Log("[システム] ゲーム一時停止");
			}
			ImGui::PopStyleColor();
		}

		ImGui::SameLine();
		if (isPaused_) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.4f, 0.8f, 1.0f));
			if (ImGui::Button("⏸ 一時停止", ImVec2(70, 0))) {
				// すでにPaused
			}
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			if (ImGui::Button("⏸ 一時停止", ImVec2(70, 0))) {
				isPaused_ = true;
				Log("[システム] ゲーム一時停止");
			}
			ImGui::PopStyleColor();
		}

		ImGui::SameLine();
		if (ImGui::Button("⏭ コマ送り", ImVec2(75, 0))) {
			isPaused_ = true;
			isStepRequested_ = true;
			Log("[システム] コマ送り（1フレーム）が要求されました");
		}

		// 右側に解像度や切り替えキーのヒント
		ImGui::SameLine(W - 240.0f);
		ImGui::Text("F1: ビュー切り替え | FPS: %.1f", ImGui::GetIO().Framerate);

		ImGui::EndMainMenuBar();
	}
}

void DevEditor::DrawHierarchyWindow() {
	if (hierarchyContentDrawer_) {
		hierarchyContentDrawer_();
	} else {
		ImGui::Text("シーンのヒエラルキー情報が登録されていません。");
	}
}

void DevEditor::DrawInspectorWindow() {
	if (selectedId_ != nullptr) {
		ImGui::Text("名前: %s", selectedName_.c_str());
		ImGui::Text("アドレス: 0x%p", selectedId_);
		ImGui::Separator();
		if (inspectorDrawer_) {
			inspectorDrawer_();
		} else {
			ImGui::Text("編集可能なフィールドが登録されていません。");
		}
	} else {
		ImGui::Text("ヒエラルキーからノードを選択してください。");
	}
}

void DevEditor::DrawGameViewWindow() {
	uint32_t rtexSrvIndex = DirectXCommon::GetInstance()->GetRenderTextureSrvIndex();
	D3D12_GPU_DESCRIPTOR_HANDLE rtexSrvHandle = ShaderResourceViewManager::GetInstance()->GetGPUDescriptorHandle(rtexSrvIndex);

	ImVec2 availSize = ImGui::GetContentRegionAvail();
	// レンダーテクスチャのアスペクト比 16:9 (1280x720) を維持してスケーリング
	float aspect = 1280.0f / 720.0f;
	float w = availSize.x;
	float h = w / aspect;
	if (h > availSize.y) {
		h = availSize.y;
		w = h * aspect;
	}

	// 画像表示を中央揃え
	ImGui::SetCursorPosX((availSize.x - w) * 0.5f);
	ImGui::SetCursorPosY((availSize.y - h) * 0.5f + ImGui::GetCursorPosY());

	ImGui::Image((ImTextureID)rtexSrvHandle.ptr, ImVec2(w, h));

	// エディタ用のオーバーレイ描画を呼び出す (ImGui::Imageの直後に行うことで、GetItemRectの取得が可能)
	if (SceneManager::GetInstance()->GetCurrentScene()) {
		SceneManager::GetInstance()->GetCurrentScene()->DrawEditorOverlay();
	}
}

void DevEditor::DrawConsoleWindow() {
	if (ImGui::Button("ログ消去")) {
		ClearLogs();
	}
	ImGui::SameLine();
	ImGui::Checkbox("自動スクロール", &autoScroll_);
	ImGui::SameLine();
	ImGui::InputText("フィルター", searchFilter_, IM_ARRAYSIZE(searchFilter_));

	ImGui::Separator();

	ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(logs_.size()));
	while (clipper.Step()) {
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
			const std::string& item = logs_[i];
			if (searchFilter_[0] != '\0' && item.find(searchFilter_) == std::string::npos) {
				continue;
			}
			ImGui::TextUnformatted(item.c_str());
		}
	}
	clipper.End();

	if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();
}

void DevEditor::DrawProjectWindow() {
	ImGui::Text("'resources/' 内のアセットリソース (.obj, .png など):");
	ImGui::Separator();
	ImGui::BeginChild("ProjectFilesRegion");

	if (fs::exists("resources")) {
		for (const auto& entry : fs::recursive_directory_iterator("resources")) {
			if (entry.is_regular_file()) {
				std::string filename = entry.path().filename().string();
				std::string path = entry.path().string();
				std::replace(path.begin(), path.end(), '\\', '/');

				std::string ext = entry.path().extension().string();
				std::string icon = "📄 ";
				if (ext == ".png" || ext == ".jpg" || ext == ".dds") icon = "🖼️ ";
				else if (ext == ".wav" || ext == ".mp3") icon = "🎵 ";
				else if (ext == ".hlsl" || ext == ".hlsli") icon = "⚙️ ";
				else if (ext == ".obj" || ext == ".gltf") icon = "📦 ";

				ImGui::Text("%s %s (%s)", icon.c_str(), filename.c_str(), path.c_str());
			}
		}
	} else {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "resources フォルダが見つかりません。");
	}

	ImGui::EndChild();
}

void DevEditor::ClearSelection() {
	selectedId_ = nullptr;
	selectedName_ = "";
	inspectorDrawer_ = nullptr;
}

#endif
