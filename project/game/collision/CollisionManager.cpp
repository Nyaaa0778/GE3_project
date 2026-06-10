#include "CollisionManager.h"
#include "../player/Player.h"
#include "../player/PlayerBullet.h"
#include "../enemy/RusherEnemy.h"
#include <MathUtility.h>
#include <cmath>
#include <algorithm>

namespace {
    bool CheckAABB(const Vector3& posA, const Vector3& sizeA, const Vector3& posB, const Vector3& sizeB) {
        float halfAx = sizeA.x * 0.5f;
        float halfAy = sizeA.y * 0.5f;
        float halfAz = sizeA.z * 0.5f;

        float halfBx = sizeB.x * 0.5f;
        float halfBy = sizeB.y * 0.5f;
        float halfBz = sizeB.z * 0.5f;

        return (std::abs(posA.x - posB.x) < (halfAx + halfBx)) &&
               (std::abs(posA.y - posB.y) < (halfAy + halfBy)) &&
               (std::abs(posA.z - posB.z) < (halfAz + halfBz));
    }
}

void CollisionManager::CheckAllCollisions(Player* player, std::vector<std::unique_ptr<RusherEnemy>>& enemies) {
    if (!player) return;
    
    CheckPlayerBulletToEnemy(player, enemies);
    CheckPlayerToEnemy(player, enemies);
}

void CollisionManager::CheckPlayerBulletToEnemy(Player* player, std::vector<std::unique_ptr<RusherEnemy>>& enemies) {
    const auto& bullets = player->GetBullets();
    
    for (const auto& bullet : bullets) {
        if (bullet->IsDead()) continue;
        
        for (auto& enemy : enemies) {
            if (!enemy || !enemy->IsAlive()) continue;
            
            if (CheckAABB(bullet->GetPosition(), bullet->GetCollisionSize(), enemy->GetWorldPosition(), enemy->GetCollisionSize())) {
                bullet->OnCollision();
                enemy->TakeDamage(1); // 弾が当たると1ダメージ
                break; // 1発の弾は1体の敵にのみ当たる
            }
        }
    }
}

void CollisionManager::CheckPlayerToEnemy(Player* player, std::vector<std::unique_ptr<RusherEnemy>>& enemies) {
    for (auto& enemy : enemies) {
        if (!enemy || !enemy->IsAlive()) continue;
        
        if (CheckAABB(player->GetWorldPos(), player->GetCollisionSize(), enemy->GetWorldPosition(), enemy->GetCollisionSize())) {
            // 自機が敵と衝突した場合、敵に大ダメージを与える
            enemy->TakeDamage(10);
            // （将来的にプレイヤーのHPが実装されたら、ここで player->TakeDamage(...) を呼ぶことができます）
        }
    }
}
