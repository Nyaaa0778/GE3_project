#pragma once

#ifdef USE_IMGUI
#include "../externals/imgui/imgui.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

struct WorldTransform;
class Camera;

class DevEditor {
public:
	static DevEditor* GetInstance();
	static void Finalize();

	void Initialize();
	void Update();
	void Draw();

	// エディタモードの取得・設定
	bool IsEditorMode() const { return isEditorMode_; }
	void SetEditorMode(bool active) { isEditorMode_ = active; }

	// 一時停止の取得・設定
	bool IsPaused() const { return isPaused_; }
	void SetPaused(bool paused) { isPaused_ = paused; }

	// コマ送りの要求取得・解除
	bool IsStepRequested() const { return isStepRequested_; }
	void ClearStepRequest() { isStepRequested_ = false; }

	// ログシステム
	void Log(const std::string& message);
	void ClearLogs() { logs_.clear(); }

	// ヒエラルキー・インスペクター用ヘルパー
	bool HierarchyNode(const char* label, void* id);
	void SetInspectorDrawer(std::function<void()> drawer);
	void DrawTransformEdit(WorldTransform* transform);
	void ClearSelection();

	// シーン側がヒエラルキーを登録するための窓口
	void DrawHierarchy(const char* title, std::function<void()> contentDrawer);

private:
	DevEditor() = default;
	~DevEditor() = default;

	// unique_ptrからの削除を許可
	friend std::default_delete<DevEditor>;

	DevEditor(const DevEditor&) = delete;
	DevEditor& operator=(const DevEditor&) = delete;

	void DrawMenuBar();
	void DrawHierarchyWindow();
	void DrawInspectorWindow();
	void DrawGameViewWindow();
	void DrawConsoleWindow();
	void DrawProjectWindow();

private:
	static std::unique_ptr<DevEditor> instance;

	bool isEditorMode_ = true; // デバッグ時はデフォルトで開発画面
	bool isPaused_ = false;
	bool isStepRequested_ = false;

	void* selectedId_ = nullptr;
	std::string selectedName_ = "";
	std::function<void()> inspectorDrawer_ = nullptr;

	std::vector<std::string> logs_;
	bool autoScroll_ = true;
	char searchFilter_[128] = "";

	std::function<void()> hierarchyContentDrawer_ = nullptr;
};

#else

// リリースビルド（USE_IMGUI未定義）時は無害なダミークラスにする
class DevEditor {
public:
	static DevEditor* GetInstance() {
		static DevEditor dummy;
		return &dummy;
	}
	static void Finalize() {}

	void Initialize() {}
	void Update() {}
	void Draw() {}

	bool IsEditorMode() const { return false; }
	void SetEditorMode(bool) {}

	bool IsPaused() const { return false; }
	void SetPaused(bool) {}

	bool IsStepRequested() const { return false; }
	void ClearStepRequest() {}

	void Log(const std::string&) {}
	void ClearSelection() {}
};

#endif
