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

    DrawImGuiCamera();
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
    const Vector3 spawnPos = {
        Random::RangeFloat(-kSpawnRangeX, kSpawnRangeX),
        kSpawnRangeY,
        kSpawnZ
    };

    auto enemy = std::make_unique<RusherEnemy>(status);
    enemy->Initialize(camera_.get(), spawnPos, kEnemyModelName);

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

void GamePlayScene::DrawImGuiCamera() {
    ImGui::SetNextWindowSize(ImVec2(300, 220), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug Window");

    if (!ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Use Debug Camera", &useDebugCamera_);
    ImGui::TextDisabled("Mode: %s", useDebugCamera_ ? "Debug" : "Rail");
    ImGui::Separator();

    Camera* cam = camera_.get();

    Vector3 pos = cam->GetTranslate();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::DragFloat3("Position##cam", &pos.x, 0.1f)) {
        cam->SetTranslate(pos);
    }

    Vector3 rot = cam->GetRotate();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::DragFloat3("Rotation##cam", &rot.x, 0.01f)) {
        cam->SetRotate(rot);
    }

    if (ImGui::Button("Reset", ImVec2(-1, 0))) {
        cam->SetTranslate({0.0f, 0.0f, -10.0f});
        cam->SetRotate({0.0f, 0.0f, 0.0f});
    }

    ImGui::End();
}