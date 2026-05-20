#include "PlayerBulletPool.h"

#include <MyEngine.h>

void PlayerBulletPool::Initialize(Camera* camera, const std::string& modelName, int maxSize) {
    slots_.resize(maxSize);
    for (auto& slot : slots_) {
        slot.model = std::make_unique<Object3d>();
        slot.model->Initialize(modelName); // まとめてファイル読み込み
        slot.model->SetCamera(camera);
        slot.inUse = false;
    }
}

Object3d* PlayerBulletPool::Rent() {
    for (auto& slot : slots_) {
        if (!slot.inUse) {
            slot.inUse = true;
            return slot.model.get();
        }
    }
    return nullptr; // プールが枯渇
}

void PlayerBulletPool::Return(Object3d* obj) {
    for (auto& slot : slots_) {
        if (slot.model.get() == obj) {
            slot.inUse = false;
            return;
        }
    }
}