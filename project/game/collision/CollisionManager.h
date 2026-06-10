#pragma once
#include <vector>
#include <list>
#include <memory>

class Player;
class RusherEnemy;

class CollisionManager {
public:
    CollisionManager() = default;
    ~CollisionManager() = default;

    void CheckAllCollisions(Player* player, std::vector<std::unique_ptr<RusherEnemy>>& enemies);

private:
    void CheckPlayerBulletToEnemy(Player* player, std::vector<std::unique_ptr<RusherEnemy>>& enemies);
    void CheckPlayerToEnemy(Player* player, std::vector<std::unique_ptr<RusherEnemy>>& enemies);
};
