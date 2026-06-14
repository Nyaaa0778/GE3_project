#pragma once

#include "IScene.h"
#include <Vector3.h>
#include <memory>
#include <vector>
#include <string>

class Object3d;
class Camera;
class DebugCamera;
class Player;
class RusherEnemy;
class RailCameraController;
class RailPathEditor;
class Skybox;
class CollisionManager;

class GamePlayScene : public IScene {
public:
    GamePlayScene();
    ~GamePlayScene();

    void Initialize() override;
    void Update()     override;
    void Draw()       override;
    void Finalize()   override;

private:
    // --- 初期化ヘルパー ---
    void InitCamera();
    void InitPlayer();
    void InitEnemies();

    // --- 更新ヘルパー ---
    void UpdateCamera();
    void UpdateEnemies();

    // --- 敵管理 ---
    void SpawnEnemy();

    // --- ImGui ---
    void DrawImGui();

    // -------------------------------------------------------
    //  カメラ
    // -------------------------------------------------------
    std::unique_ptr<Camera>               camera_;
    std::unique_ptr<DebugCamera>          debugCamera_;
    std::unique_ptr<RailCameraController> railCameraController_;
    std::unique_ptr<RailPathEditor>       railPathEditor_;

    bool useDebugCamera_ = true;

    static constexpr Vector3 kInitialCameraPos = {0.0f, 0.0f, -20.0f};

    // -------------------------------------------------------
    //  自機
    // -------------------------------------------------------
    std::unique_ptr<Player>    player_;
    std::unique_ptr<Object3d>  playerModel_;
    std::unique_ptr<Object3d>  playerBulletModel_;

    // -------------------------------------------------------
    //  敵
    // -------------------------------------------------------
    std::vector<std::unique_ptr<RusherEnemy>> enemies_;

    static constexpr int         kMaxEnemyCount = 3;
    static constexpr float       kSpawnRangeX = 10.0f;
    static constexpr float       kSpawnRangeY = 0.0f;
    static constexpr float       kSpawnZ = 15.0f;
    static constexpr const char* kEnemyModelName = "sphere";

    // -------------------------------------------------------
    //  天球
    // -------------------------------------------------------
    std::unique_ptr<Skybox> skybox_;
    std::unique_ptr<Object3d> reflectionSphere_;

    // -------------------------------------------------------
    //  当たり判定
    // -------------------------------------------------------
    std::unique_ptr<CollisionManager> collisionManager_;
};