#pragma once

#include <memory>
#include <vector>
#include <string>

class Camera;
class Object3d;

class PlayerBulletPool {
public:
    // プールの初期化（maxSize 個の Object3d を事前生成）
    void Initialize(Camera* camera, const std::string& modelName, int maxSize);

    // 未使用の Object3d を借りる（なければ nullptr）
    Object3d* Rent();

    // 使い終わった Object3d を返却
    void Return(Object3d* model);

private:
    struct Slot {
        std::unique_ptr<Object3d> model;
        bool inUse = false;
    };

    std::vector<Slot> slots_;
};