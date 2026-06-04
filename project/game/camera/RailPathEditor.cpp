#define NOMINMAX
#include <MyEngine.h>

#include <MathUtility.h>

using namespace MathUtility;

#include "Box.h"
#include "PrimitiveRenderer.h"
#include "RailPathEditor.h"
#include "RailCameraController.h"

#include <imgui.h>
#include <cmath>

// -------------------------------------------------------
//  定数
// -------------------------------------------------------

namespace {
    constexpr float kScreenW = 1280.0f;
    constexpr float kScreenH = 720.0f;
    constexpr float kHoverRadiusPx = 20.0f;   // ホバー判定半径（ピクセル）
    constexpr float kLookAheadDelta = 0.001f;   // 進行方向サンプリング間隔
    constexpr int   kSegmentsPerSpan = 20;       // 1 区間あたりの軌道分割数

    // ワールド座標 → スクリーン座標へ投影
    // 戻り値: false = カメラ背面
    bool ProjectToScreen(const Vector3& world, const Matrix4x4& vp, float& sx, float& sy) {
        const float x = world.x * vp.m[0][0] + world.y * vp.m[1][0] + world.z * vp.m[2][0] + vp.m[3][0];
        const float y = world.x * vp.m[0][1] + world.y * vp.m[1][1] + world.z * vp.m[2][1] + vp.m[3][1];
        const float z = world.x * vp.m[0][2] + world.y * vp.m[1][2] + world.z * vp.m[2][2] + vp.m[3][2];
        const float w = world.x * vp.m[0][3] + world.y * vp.m[1][3] + world.z * vp.m[2][3] + vp.m[3][3];
        if (w <= 0.001f) return false;
        sx = (x / w + 1.0f) * 0.5f * kScreenW;
        sy = (-y / w + 1.0f) * 0.5f * kScreenH;
        return true;
    }
} // namespace

// -------------------------------------------------------
//  初期化 / 終了
// -------------------------------------------------------

void RailPathEditor::Initialize(Camera* camera) {
    camera_ = camera;

    if (railPath_.GetPointCount() == 0) {
        railPath_.AddPoint({0.0f, 0.0f, 0.0f});
        railPath_.AddPoint({10.0f, 2.0f, 30.0f});
        railPath_.AddPoint({-10.0f, 5.0f, 60.0f});
        railPath_.AddPoint({0.0f, 0.0f, 90.0f});
    }
}

// -------------------------------------------------------
//  メインループ
// -------------------------------------------------------

void RailPathEditor::Update() {
    if (showGizmo_ && camera_) {
        UpdateDrag();
        UpdateGizmos();
    }
}

void RailPathEditor::Draw() {
    if (!showGizmo_) return;

    for (auto& g : controlPointGizmos_) { if (g) g->Draw(); }
    for (auto& s : lineSegments_) { if (s) s->Draw(); }
}

// -------------------------------------------------------
//  ImGui
// -------------------------------------------------------

void RailPathEditor::DrawImGuiInline() {
    if (ImGui::CollapsingHeader("🛤️ Path Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawImGuiPathControls();
    }

    ImGui::Spacing();

    // 縦スクロールを削減するため、制御点リストとポイント編集画面を左右2列（Columns）に配置します
    if (ImGui::CollapsingHeader("📍 Control Points Editor", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(2, "ControlPointsLayout", false); // 境界線なし
        
        // 左列: ポイントリスト
        DrawImGuiPointList();
        
        ImGui::NextColumn();
        
        // 右列: ポイント座標編集
        DrawImGuiPointEditor();
        
        ImGui::Columns(1); // カラム終了
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("🗺️ Minimap (Top-Down)", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawImGuiMinimap();
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("💾 Save / Load File", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawImGuiFileIO();
    }

    DrawImGuiPopups();
}

// [1] パス全体の設定
void RailPathEditor::DrawImGuiPathControls() {
    ImGui::Checkbox("Show Gizmo", &showGizmo_);
    ImGui::SameLine(160);
    ImGui::Checkbox("Auto Scroll", &isScrollActive_);

    ImGui::SetNextItemWidth(180);
    ImGui::DragFloat("Scroll Speed", &cameraSpeed_, 0.0001f, 0.0001f, 0.05f, "%.4f");

    ImGui::Text("Control Points: %d", static_cast<int>(railPath_.GetPointCount()));
}

// [2] 制御点リスト
void RailPathEditor::DrawImGuiPointList() {
    if (ImGui::Button("+ Add Point", ImVec2(100, 24))) {
        Vector3 pos = railPath_.GetPointCount() > 0
            ? railPath_.GetPoints().back() + Vector3{0.0f, 0.0f, 10.0f}
        : Vector3{0.0f, 0.0f, 0.0f};
        railPath_.AddPoint(pos);
        selectedPointIndex_ = static_cast<int>(railPath_.GetPointCount()) - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear All", ImVec2(100, 24))) {
        railPath_.Clear();
        selectedPointIndex_ = -1;
    }

    ImGui::BeginChild("##PointsList", ImVec2(0, 150), true);
    const auto& pts = railPath_.GetPoints();
    for (size_t i = 0; i < pts.size(); ++i) {
        const bool selected = (selectedPointIndex_ == static_cast<int>(i));
        char label[64];
        snprintf(label, sizeof(label), "Point %2zu : (%.1f, %.1f, %.1f)", i, pts[i].x, pts[i].y, pts[i].z);

        // 選択行をハイライト
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.7f, 1.0f, 0.4f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.7f, 1.0f, 0.6f));
        }
        if (ImGui::Selectable(label, selected)) {
            selectedPointIndex_ = static_cast<int>(i);
        }
        if (selected) ImGui::PopStyleColor(2);
    }
    ImGui::EndChild();
}

// [3] 選択中の制御点の編集
void RailPathEditor::DrawImGuiPointEditor() {
    const int count = static_cast<int>(railPath_.GetPointCount());
    if (selectedPointIndex_ < 0 || selectedPointIndex_ >= count) return;

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Selected Point: #%d", selectedPointIndex_);

    Vector3 pos = railPath_.GetPoints()[selectedPointIndex_];
    ImGui::Text("Coordinate (X, Y, Z)");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::DragFloat3("##pos", &pos.x, 0.1f, 0.0f, 0.0f, "X: %.1f  Y: %.1f  Z: %.1f")) {
        railPath_.SetPoint(selectedPointIndex_, pos);
    }

    ImGui::Spacing();
    if (ImGui::Button("Insert After", ImVec2(100, 24))) {
        railPath_.InsertPoint(selectedPointIndex_ + 1, pos + Vector3{0.0f, 0.0f, 5.0f});
        selectedPointIndex_++;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete", ImVec2(100, 24))) {
        railPath_.DeletePoint(selectedPointIndex_);
        selectedPointIndex_ = std::min(selectedPointIndex_, static_cast<int>(railPath_.GetPointCount()) - 1);
    }
}

// [4] ファイル I/O
void RailPathEditor::DrawImGuiFileIO() {
    char buf[256];
    strncpy_s(buf, saveFileName_.c_str(), sizeof(buf));
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##filename", buf, sizeof(buf))) {
        saveFileName_ = buf;
    }
    ImGui::TextDisabled("resources/paths/%s", saveFileName_.c_str());

    const std::string fullPath = "resources/paths/" + saveFileName_;

    if (ImGui::Button("Save File", ImVec2(100, 24))) {
        pendingPopup_ = railPath_.SaveToJson(fullPath) ? PopupType::SaveOK : PopupType::SaveFail;
        if (pendingPopup_ == PopupType::SaveOK)   ImGui::OpenPopup("##SaveOK");
        if (pendingPopup_ == PopupType::SaveFail)  ImGui::OpenPopup("##SaveFail");
    }
    ImGui::SameLine();
    if (ImGui::Button("Load File", ImVec2(100, 24))) {
        if (railPath_.LoadFromJson(fullPath)) {
            selectedPointIndex_ = -1;
            ImGui::OpenPopup("##LoadOK");
        } else {
            ImGui::OpenPopup("##LoadFail");
        }
    }
}

// [5] モーダルポップアップ
void RailPathEditor::DrawImGuiPopups() {
    const std::string fp = "resources/paths/" + saveFileName_;

    auto modal = [](const char* id, const char* msg) {
        if (ImGui::BeginPopupModal(id, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(msg);
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(100, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        };

    char ok[256], fail[256];
    snprintf(ok, sizeof(ok), "Saved to %s", fp.c_str());
    snprintf(fail, sizeof(fail), "Failed to save to %s", fp.c_str());
    modal("##SaveOK", ok);
    modal("##SaveFail", fail);

    snprintf(ok, sizeof(ok), "Loaded from %s", fp.c_str());
    snprintf(fail, sizeof(fail), "Failed to load from %s", fp.c_str());
    modal("##LoadOK", ok);
    modal("##LoadFail", fail);
}

void RailPathEditor::DrawImGuiMinimap() {
    // ズーム調整
    ImGui::DragFloat("Zoom", &minimapZoom_, 0.1f, 0.1f, 50.0f);
    ImGui::TextDisabled("Drag canvas below to pan (Top-Down)");

    // 描画領域(キャンバス)の確保 (幅はウィンドウ一杯、高さは200px)
    ImVec2 canvasSize(ImGui::GetContentRegionAvail().x, 200.0f);
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    // 背景を暗いグレーで塗りつぶし、白い枠線を描画
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(30, 30, 30, 255));
    drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(200, 200, 200, 255));

    // ドラッグによるパン操作（マウス操作）
    ImGui::InvisibleButton("minimap_canvas", canvasSize);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        minimapOffset_.x += delta.x / minimapZoom_;
        minimapOffset_.z -= delta.y / minimapZoom_; // Z奥(+)を画面上(-)にするため符号反転
    }

    // キャンバス外にはみ出して描画されないようにクリップを設定
    drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

    // 【座標変換】3D(X, Z)空間を -> 2Dキャンバス空間に変換するラムダ関数
    auto WorldToCanvas = [&](const Vector3& worldPos) -> ImVec2 {
        ImVec2 center(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
        float cx = center.x + (worldPos.x + minimapOffset_.x) * minimapZoom_;
        float cy = center.y - (worldPos.z + minimapOffset_.z) * minimapZoom_;
        return ImVec2(cx, cy);
        };

    // 1. パスの軌道（ライン）を描画
    const size_t ptCount = railPath_.GetPointCount();
    if (ptCount >= 2) {
        const int segments = (static_cast<int>(ptCount) - 1) * 20; // 滑らかさ
        const float step = 1.0f / segments;
        for (int i = 0; i < segments; ++i) {
            Vector3 p0 = railPath_.Evaluate(i * step);
            Vector3 p1 = railPath_.Evaluate((i + 1) * step);
            drawList->AddLine(WorldToCanvas(p0), WorldToCanvas(p1), IM_COL32(0, 255, 255, 200), 1.5f);
        }
    }

    // 2. 制御点（黄色/赤色の円）を描画
    const auto& pts = railPath_.GetPoints();
    for (size_t i = 0; i < pts.size(); ++i) {
        ImU32 color = (static_cast<int>(i) == selectedPointIndex_) ? IM_COL32(255, 50, 50, 255) : IM_COL32(255, 200, 0, 255);
        drawList->AddCircleFilled(WorldToCanvas(pts[i]), 4.0f, color);
    }

    // 3. レールカメラの現在位置（緑色の円）を描画
    if (railController_) {
        Vector3 camPos = railController_->GetPosition();
        ImVec2 camCanvasPos = WorldToCanvas(camPos);

        // カメラ位置の円
        drawList->AddCircleFilled(camCanvasPos, 6.0f, IM_COL32(50, 255, 50, 255));

        // カメラの進行方向（少し先の点を取得して線で繋ぐ）
        float nextT = railController_->GetProgress() + 0.05f;
        if (nextT > 1.0f) nextT -= 1.0f;
        Vector3 forwardPos = railPath_.Evaluate(nextT);
        drawList->AddLine(camCanvasPos, WorldToCanvas(forwardPos), IM_COL32(50, 255, 50, 255), 2.0f);
    }

    drawList->PopClipRect();
}

// -------------------------------------------------------
//  ドラッグ操作
// -------------------------------------------------------

int RailPathEditor::FindHoveredPoint(float mx, float my) const {
    if (!camera_) return -1;

    const Matrix4x4  vp = camera_->GetViewProjectionMatrix();
    const auto& pts = railPath_.GetPoints();
    float            nearestSq = kHoverRadiusPx * kHoverRadiusPx;
    int              result = -1;

    for (size_t i = 0; i < pts.size(); ++i) {
        float sx, sy;
        if (!ProjectToScreen(pts[i], vp, sx, sy)) continue;
        const float dx = mx - sx, dy = my - sy;
        const float distSq = dx * dx + dy * dy;
        if (distSq < nearestSq) {
            nearestSq = distSq;
            result = static_cast<int>(i);
        }
    }
    return result;
}

void RailPathEditor::UpdateDrag() {
    Input* input = Input::GetInstance();
    const auto mPos = input->GetMousePosition();
    const bool imCapture = ImGui::GetIO().WantCaptureMouse;

    const auto& pts = railPath_.GetPoints();

    // --- ホバー & ドラッグ開始 ---
    if (!imCapture && !isDragging_) {
        const int hovered = FindHoveredPoint(static_cast<float>(mPos.x), static_cast<float>(mPos.y));
        if (hovered >= 0) {
            // ツールチップ
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Point %d", hovered);
            ImGui::Separator();
            const Vector3& p = pts[hovered];
            ImGui::Text("X %.2f  Y %.2f  Z %.2f", p.x, p.y, p.z);
            ImGui::TextDisabled("Left-drag to move");
            ImGui::EndTooltip();

            if (input->TriggerMouse(0)) {
                isDragging_ = true;
                draggedPointIndex_ = hovered;
                selectedPointIndex_ = hovered;
            }
        }
    }

    // --- ドラッグ中 ---
    if (isDragging_ && draggedPointIndex_ >= 0
        && draggedPointIndex_ < static_cast<int>(pts.size())) {
        if (input->PushMouse(0)) {
            const Matrix4x4 vp = camera_->GetViewProjectionMatrix();
            const Matrix4x4 invVP = MakeInverseMatrix(vp);
            const Vector3& cur = pts[draggedPointIndex_];

            // 現在の点の NDC 深度を保存（Z スライド面に固定してドラッグ）
            const float cz = cur.x * vp.m[0][2] + cur.y * vp.m[1][2] + cur.z * vp.m[2][2] + vp.m[3][2];
            const float cw = cur.x * vp.m[0][3] + cur.y * vp.m[1][3] + cur.z * vp.m[2][3] + vp.m[3][3];
            const float ndcZ = cz / cw;

            // マウス座標 → NDC
            const float ndcX = static_cast<float>(mPos.x) / kScreenW * 2.0f - 1.0f;
            const float ndcY = -static_cast<float>(mPos.y) / kScreenH * 2.0f + 1.0f;

            // NDC → ワールド（逆行列で Unproject）
            const float wx = ndcX * invVP.m[0][0] + ndcY * invVP.m[1][0] + ndcZ * invVP.m[2][0] + invVP.m[3][0];
            const float wy = ndcX * invVP.m[0][1] + ndcY * invVP.m[1][1] + ndcZ * invVP.m[2][1] + invVP.m[3][1];
            const float wz = ndcX * invVP.m[0][2] + ndcY * invVP.m[1][2] + ndcZ * invVP.m[2][2] + invVP.m[3][2];
            const float ww = ndcX * invVP.m[0][3] + ndcY * invVP.m[1][3] + ndcZ * invVP.m[2][3] + invVP.m[3][3];

            railPath_.SetPoint(draggedPointIndex_, {wx / ww, wy / ww, wz / ww});
        } else {
            isDragging_ = false;
            draggedPointIndex_ = -1;
        }
    }
}

// -------------------------------------------------------
//  ギズモ更新
// -------------------------------------------------------

void RailPathEditor::UpdateGizmos() {
    const size_t ptCount = railPath_.GetPointCount();

    // --- 制御点 Box のプール管理 ---
    while (controlPointGizmos_.size() < ptCount) {
        auto box = std::make_unique<Box>();
        box->Initialize();
        box->SetCamera(camera_);
        box->SetBlendMode(PrimitiveRenderer::BlendMode::kNormal);
        controlPointGizmos_.push_back(std::move(box));
    }
    controlPointGizmos_.resize(ptCount);

    const auto& pts = railPath_.GetPoints();
    for (size_t i = 0; i < ptCount; ++i) {
        const bool isSelected = (static_cast<int>(i) == selectedPointIndex_);
        controlPointGizmos_[i]->SetPosition(pts[i]);
        controlPointGizmos_[i]->SetScale(isSelected ? Vector3{0.5f, 0.5f, 0.5f}
        : Vector3{0.25f, 0.25f, 0.25f});
        controlPointGizmos_[i]->SetColor(isSelected ? Vector4{1.0f, 0.2f, 0.2f, 1.0f}   // 赤: 選択中
        : Vector4{1.0f, 0.8f, 0.0f, 1.0f}); // 黄: 通常
        controlPointGizmos_[i]->Update();
    }

    // --- 軌道線 Box のプール管理 ---
    const size_t requiredSegs = (ptCount >= 2) ? (ptCount - 1) * kSegmentsPerSpan : 0;

    while (lineSegments_.size() < requiredSegs) {
        auto box = std::make_unique<Box>();
        box->Initialize();
        box->SetCamera(camera_);
        box->SetColor({0.0f, 1.0f, 1.0f, 1.0f}); // シアン
        box->SetBlendMode(PrimitiveRenderer::BlendMode::kNormal);
        lineSegments_.push_back(std::move(box));
    }
    lineSegments_.resize(requiredSegs);

    if (requiredSegs == 0) return;

    const float step = 1.0f / static_cast<float>(requiredSegs);
    for (size_t i = 0; i < requiredSegs; ++i) {
        const float   t0 = i * step;
        const float   t1 = t0 + step;
        const Vector3 p0 = railPath_.Evaluate(t0);
        const Vector3 p1 = railPath_.Evaluate(t1);
        const Vector3 dir = p1 - p0;

        float len = Length(dir);
        if (len < 0.001f) len = 0.001f;

        const Vector3 nd = Normalize(dir);
        const float   yaw = atan2f(nd.x, nd.z);
        const float   pitch = -atan2f(nd.y, sqrtf(nd.x * nd.x + nd.z * nd.z));

        lineSegments_[i]->SetPosition(0.5f * (p0 + p1));
        lineSegments_[i]->SetRotation({pitch, yaw, 0.0f});
        lineSegments_[i]->SetScale({0.04f, 0.04f, len * 0.5f});
        lineSegments_[i]->Update();
    }
}