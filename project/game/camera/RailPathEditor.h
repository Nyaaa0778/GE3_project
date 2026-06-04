#pragma once
#include <memory>
#include <vector>
#include <string>
#include "RailPath.h"
#include "Box.h"

class Camera;
class RailCameraController;

class RailPathEditor {
public:
    RailPathEditor() = default;
    ~RailPathEditor() = default;

    void Initialize(Camera* camera);
    void Update();
    void Draw();
    void DrawImGuiInline();

    // --- ゲッター ---
    RailPath* GetRailPath() { return &railPath_; }
    const RailPath* GetRailPath() const { return &railPath_; }
    bool IsScrollActive()  const { return isScrollActive_; }
    float GetCameraSpeed()  const { return cameraSpeed_; }
    void SetRailCameraController(RailCameraController* controller) { railController_ = controller; }

private:

    // ImGui セクション分割
    void DrawImGuiPathControls();
    void DrawImGuiPointList();
    void DrawImGuiPointEditor();
    void DrawImGuiFileIO();
    void DrawImGuiPopups();
    void DrawImGuiMinimap();

    // 3D ギズモ操作
    void UpdateDrag();
    void UpdateGizmos();

    // --- ヘルパー ---
    // 制御点のホバー判定（戻り値: インデックス、なければ -1）
    int  FindHoveredPoint(float mouseX, float mouseY) const;

    // -------------------------------------------------------
    RailPath railPath_;
    Camera* camera_ = nullptr;

    RailCameraController* railController_ = nullptr;
    float minimapZoom_ = 3.0f;
    Vector3 minimapOffset_ = {0.0f, 0.0f, 0.0f};

    // 可視化用 Box プール
    std::vector<std::unique_ptr<Box>> lineSegments_;
    std::vector<std::unique_ptr<Box>> controlPointGizmos_;

    // エディタ設定
    std::string saveFileName_ = "railPath.json";
    bool        showGizmo_ = true;
    int         selectedPointIndex_ = -1;

    // カメラ走行設定
    bool  isScrollActive_ = false;
    float cameraSpeed_ = 0.002f;

    // ドラッグ操作
    bool isDragging_ = false;
    int  draggedPointIndex_ = -1;

    // ポップアップ制御
    enum class PopupType { None, SaveOK, SaveFail, LoadOK, LoadFail };
    PopupType pendingPopup_ = PopupType::None;
};