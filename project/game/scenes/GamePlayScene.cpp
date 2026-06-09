#include "GamePlayScene.h"

#include <MyEngine.h>
#include <Random.h>

#include "Player.h"
#include "RusherEnemy.h"
#include "RailCameraController.h"
#include "RailPathEditor.h"
#include "Skybox.h"

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

// -------------------------------------------------------
//  初期化
// -------------------------------------------------------

void GamePlayScene::Initialize() {
    InitCamera();
    InitPlayer();
    InitEnemies();

    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize("resources/sprites/rostock_laage_airport_4k.dds", camera_.get());
}

void GamePlayScene::InitCamera() {
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate(kInitialCameraPos);

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize();
    debugCamera_->SetTranslate(kInitialCameraPos);
    debugCamera_->CalculateMatrix();

    railCameraController_ = std::make_unique<RailCameraController>();
    railCameraController_->Initialize(camera_.get(), kInitialCameraPos);

    railPathEditor_ = std::make_unique<RailPathEditor>();
    railPathEditor_->Initialize(camera_.get());
    railPathEditor_->GetRailPath()->LoadFromJson("resources/paths/railPath.json");
}

void GamePlayScene::InitPlayer() {
    playerModel_ = std::make_unique<Object3d>();
    playerModel_->Initialize("player");

    playerBulletModel_ = std::make_unique<Object3d>();
    playerBulletModel_->Initialize("bullet");

    // プレイヤーモデルの親にレール Transform をセット
    playerModel_->GetWorldTransform().parent = &railCameraController_->GetWorldTransform();

    player_ = std::make_unique<Player>();
    player_->Initialize(camera_.get(), {0.0f, 0.0f, 0.0f}, playerModel_.get(), "bullet");
}

void GamePlayScene::InitEnemies() {
    enemies_.reserve(kMaxEnemyCount);
    for (int i = 0; i < kMaxEnemyCount; ++i) {
        SpawnEnemy();
    }
}

// -------------------------------------------------------
//  更新
// -------------------------------------------------------

void GamePlayScene::Update() {
    if (railPathEditor_) railPathEditor_->Update();

    DrawImGui();
    UpdateCamera();

    player_->Update(railCameraController_->GetWorldTransform());
    UpdateEnemies();
}

void GamePlayScene::UpdateCamera() {
    if (useDebugCamera_) {
        debugCamera_->Update(camera_.get());
        return;
    }

    camera_->CalculateMatrix();

    // エディタの設定をレールカメラに反映
    if (railPathEditor_ && railCameraController_) {
        railCameraController_->SetRailPath(railPathEditor_->GetRailPath());
        railCameraController_->SetScrollActive(railPathEditor_->IsScrollActive());
        railCameraController_->SetScrollSpeed(railPathEditor_->GetCameraSpeed());
    }
    railCameraController_->Update();
}

void GamePlayScene::UpdateEnemies() {
    for (auto& e : enemies_) {
        if (e) e->Update(player_.get());
    }

    // 死亡した敵を除去
    enemies_.erase(
        std::remove_if(enemies_.begin(), enemies_.end(),
                       [](const std::unique_ptr<RusherEnemy>& e) {
                           return !e || !e->IsAlive();
                       }),
        enemies_.end());

    // 不足分を補充
    while (static_cast<int>(enemies_.size()) < kMaxEnemyCount) {
        SpawnEnemy();
    }
}

void GamePlayScene::SpawnEnemy() {
    const EnemyStatus status = {10, 10, 1, 0.1f};
    Vector3 spawnPos = {0.0f, 0.0f, 0.0f};

    // レールカメラがパスに沿って移動しているか
    if (railCameraController_ && railPathEditor_ && railPathEditor_->GetRailPath() && railPathEditor_->GetRailPath()->GetPointCount() >= 2) {
        float currentT = railCameraController_->GetProgress();
        float spawnT = currentT + 0.40f; // プレイヤーの少し先（約15%分先）にスポーン
        if (spawnT > 1.0f) spawnT -= 1.0f;

        Vector3 railPos = railPathEditor_->GetRailPath()->Evaluate(spawnT);
        
        // レールの周囲に少しランダムオフセットを加える（左右・上下）
        float offsetX = Random::RangeFloat(-10.0f, 10.0f);
        float offsetY = Random::RangeFloat(-5.0f, 5.0f);
        float offsetZ = Random::RangeFloat(-5.0f, 5.0f);
        spawnPos = { railPos.x + offsetX, railPos.y + offsetY, railPos.z + offsetZ };
    } else {
        // 直線移動時のフォールバック
        Vector3 camPos = railCameraController_ ? railCameraController_->GetPosition() : Vector3{0.0f, 0.0f, 0.0f};
        spawnPos = {
            Random::RangeFloat(-kSpawnRangeX, kSpawnRangeX),
            kSpawnRangeY,
            camPos.z + kSpawnZ
        };
    }

    auto enemy = std::make_unique<RusherEnemy>(status);
    // カメラに追尾しないため、親トランスフォームは nullptr に設定
    enemy->Initialize(camera_.get(), spawnPos, kEnemyModelName, nullptr);

    if (player_) enemy->Update(player_.get());

    enemies_.push_back(std::move(enemy));
}

// -------------------------------------------------------
//  描画
// -------------------------------------------------------

void GamePlayScene::Draw() {
    if (railPathEditor_) railPathEditor_->Draw();

    player_->Draw();

    for (auto& e : enemies_) {
        if (e) e->Draw();
    }

    // skybox_->Draw();
}

void GamePlayScene::Finalize() {}

// -------------------------------------------------------
//  ImGui
// -------------------------------------------------------

void GamePlayScene::DrawImGui() {
#ifdef USE_IMGUI
    // 3つのウィンドウを統合したワイドなコントロールセンター
    // 左右に並んだレイアウトが綺麗に収まるように横幅を広く取ります
    ImGui::SetNextWindowSize(ImVec2(560, 550), ImGuiCond_FirstUseEver);
    ImGui::Begin("Control Center", nullptr, ImGuiWindowFlags_None); // スクロールバーは必要に応じて自動で表示されるようNoneにする

    if (ImGui::BeginTabBar("ControlTabBar")) {
        
        // -------------------------------------------------------
        // [1] Camera & Stats タブ
        // -------------------------------------------------------
        if (ImGui::BeginTabItem("🎥 Camera & Stats")) {
            if (ImGui::CollapsingHeader("🎥 Viewport Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Use Debug Camera", &useDebugCamera_);
                ImGui::SameLine();
                ImGui::TextDisabled("| Mode: %s", useDebugCamera_ ? "Debug" : "Rail");
                ImGui::Separator();

                Camera* cam = camera_.get();

                Vector3 pos = cam->GetTranslate();
                ImGui::Text("Translate");
                ImGui::SetNextItemWidth(-1);
                if (ImGui::DragFloat3("##PositionCam", &pos.x, 0.1f, 0.0f, 0.0f, "X: %.1f  Y: %.1f  Z: %.1f")) {
                    cam->SetTranslate(pos);
                }

                Vector3 rot = cam->GetRotate();
                ImGui::Text("Rotation");
                ImGui::SetNextItemWidth(-1);
                if (ImGui::DragFloat3("##RotationCam", &rot.x, 0.01f, 0.0f, 0.0f, "P: %.2f  Y: %.2f  R: %.2f")) {
                    cam->SetRotate(rot);
                }

                ImGui::Spacing();
                if (ImGui::Button("Reset Camera", ImVec2(-1, 30))) {
                    cam->SetTranslate({0.0f, 0.0f, -10.0f});
                    cam->SetRotate({0.0f, 0.0f, 0.0f});
                }
            }

            ImGui::Spacing();

            if (ImGui::CollapsingHeader("📊 Engine Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Performance: %.1f FPS", ImGui::GetIO().Framerate);
                ImGui::TextDisabled("Frame Time:  %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
                ImGui::Separator();
                ImGui::TextDisabled("Press [TAB] to Hide/Show UI");
            }
            ImGui::EndTabItem();
        }

        // -------------------------------------------------------
        // [2] Player Settings タブ
        // -------------------------------------------------------
        if (ImGui::BeginTabItem("👤 Player Settings")) {
            if (player_) {
                player_->DrawImGuiInline();
            }
            ImGui::EndTabItem();
        }

        // -------------------------------------------------------
        // [3] Rail Editor タブ
        // -------------------------------------------------------
        if (ImGui::BeginTabItem("🛤️ Rail Editor")) {
            if (railPathEditor_) {
                railPathEditor_->DrawImGuiInline();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
#endif
}