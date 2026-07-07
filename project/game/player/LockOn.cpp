#include "LockOn.h"

#include <MyEngine.h>
#include <MathUtility.h>
#include <Vector2.h>

#include "EnemyBase.h"
#include "Camera.h"
#include "Player.h"

using namespace MathUtility;

LockOn::LockOn() = default;
LockOn::~LockOn() = default;

void LockOn::Initialize() {
	// 複数ロックオン用に動的にスプライトを生成するため、初期化時は何もしない
}

void LockOn::Update(Player* player, std::list<EnemyBase*>& enemies, const Camera* camera) {
	// ロックオンモードではない場合はロックオン対象をクリアして終了
	if (!player->GetIsLockOnMode()) {
		targets_.clear();
		return;
	}

	// 自機のワールド座標を取得する
	Vector3 playerPositionWorld = player->GetWorldPosition();
	// ビュー座標に変換する
	Vector3 playerPositionView = MathUtility::Transform(playerPositionWorld, camera->matView);

	// 生存している敵だけに絞り込む（死亡した敵はロックオン対象から外す）
	targets_.remove_if([&enemies](EnemyBase* enemy) {
		return std::find(enemies.begin(), enemies.end(), enemy) == enemies.end();
	});

	// ロックオン判定処理
	for (EnemyBase* enemy : enemies) {
		// すでにロックオンされている場合はスキップ
		if (std::find(targets_.begin(), targets_.end(), enemy) != targets_.end()) {
			continue;
		}

		// 最大ロックオン数に達している場合はスキップ
		if (targets_.size() >= kMaxLockOnTargets) {
			break;
		}

		// 敵のワールド座標を取得
		Vector3 positionWorld = enemy->GetWorldPosition();

		// ビュー座標系に変換する
		Vector3 positionView = MathUtility::Transform(positionWorld, camera->matView);

		// 敵のビュー座標.z が 自機のビュー座標.z 以下なら
		if (positionView.z <= playerPositionView.z) {
			continue; // 自機より手前にいる場合は除外
		}

		// ワールド座標からスクリーン座標に変換
		Vector3 positionScreen = Project(positionWorld, 0.0f, 0.0f, WinApp::kClientWidth, 
										 WinApp::kClientHeight, camera->matView, camera->matProjection);

		// Vector2に格納
		Vector2 positionScreenV2(positionScreen.x, positionScreen.y);
		// スプライトの中心の距離
		float distance = Distance(player->GetReticle2DPosition(), positionScreenV2);
		// 2Dレティクルからのスクリーン距離が基底範囲内なら
		if (distance <= kDistanceLockOn) {
			targets_.push_back(enemy);
		}
	}

	// スプライトプールのサイズを調整
	if (sprites_.size() < targets_.size()) {
		size_t oldSize = sprites_.size();
		sprites_.resize(targets_.size());
		for (size_t i = oldSize; i < sprites_.size(); ++i) {
			sprites_[i] = std::make_unique<Sprite>();
			sprites_[i]->Initialize("reticle.png", {640.0f, 360.0f}, {0.5f, 0.5f});
			sprites_[i]->SetScale({kReticleDrawSize.x, kReticleDrawSize.y});
			sprites_[i]->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 視認性向上のため白に変更
		}
	}

	// 各ターゲットに対してスプライトの位置を更新
	size_t index = 0;
	for (EnemyBase* enemy : targets_) {
		// ロックオン対象のワールド座標を取得する
		Vector3 targetWorldPos = enemy->GetWorldPosition();

		// ワールド座標からスクリーン座標に変換する
		Vector3 targetScreenPos = Project(targetWorldPos, 0.0f, 0.0f, WinApp::kClientWidth,
										  WinApp::kClientHeight, camera->matView, camera->matProjection);

		// ロックオンマーク（スプライト）の座標を設定する
		sprites_[index]->SetPosition({targetScreenPos.x, targetScreenPos.y});
		sprites_[index]->Update();
		index++;
	}
}

void LockOn::Draw() {
	// 現在のターゲット数分だけスプライトを描画する
	for (size_t i = 0; i < targets_.size(); ++i) {
		sprites_[i]->Draw();
	}
}
