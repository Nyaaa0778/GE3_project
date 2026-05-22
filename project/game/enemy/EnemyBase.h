#pragma once

#include <Vector3.h>
#include <memory>
#include <string>

// 前方宣言
class Camera;
class Object3d;
class Player;

struct EnemyStatus {
	int maxHp; // 最大HP
	int currentHp; // 現在のHP
	int attackPower; // 攻撃力
	float speed; // 移動の速さ
};

class EnemyBase {
protected:
	// ステータスデータ
	EnemyStatus status_;

	// 生存フラグ
	bool isAlive_;

	// 3Dモデル
	std::unique_ptr<Object3d> model_;

	// 座標
	Vector3 pos_ = {0.0f, 0.0f, 0.0f};

public:
	// コンストラクタ
	EnemyBase(const EnemyStatus& initialStatus);

	// 仮想デストラクタ
	virtual ~EnemyBase();

	// 初期化
	virtual void Initialize(Camera* camera, const Vector3& pos, const std::string& modelName);

	// 更新
	virtual void Update(Player* player);

	// 描画
	virtual void Draw();

	// ダメージを受ける
	void TakeDamage(int damage);

	// 生存フラグの取得
	bool IsAlive() const { return isAlive_; }

	// 位置の取得
	const Vector3& GetPosition() const { return pos_; }

protected:
	virtual void Die();
};

