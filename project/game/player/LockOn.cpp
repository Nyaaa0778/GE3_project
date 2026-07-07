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
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("reticle.png", {640.0f, 360.0f}, {0.5f, 0.5f});
	sprite_->SetScale({kReticleDrawSize.x, kReticleDrawSize.y});
	sprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f}); // 視認性向上のため白に変更
}

void LockOn::Update(Player* player, std::list<EnemyBase*>& enemies, const Camera* camera) {
	// 自機のワールド座標を取得する
	Vector3 playerPositionWorld = player->GetWorldPosition();
	// ビュー座標に変換する
	Vector3 playerPositionView = MathUtility::Transform(playerPositionWorld, camera->matView);

	// ロックオン対象リスト
	std::list<std::pair<float, EnemyBase*>> targets;
	
	// ロックオン判定処理
	for (EnemyBase* enemy : enemies) {
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
			targets.emplace_back(std::make_pair(distance, enemy));
		}
	}

	// 一旦ロックオンを解除
	target_ = nullptr;
	// 対象を絞り込んで座標設定する
	if (!targets.empty()) {
		// 距離で昇順にソート
		targets.sort();
		// 距離が一番小さい敵をロックオン対象とする
		target_ = targets.front().second;
		
		// 1. ロックオン対象（target_）のワールド座標を取得する
		Vector3 targetWorldPos = target_->GetWorldPosition();

		// 2. ワールド座標からスクリーン座標に変換する
		Vector3 targetScreenPos = Project(targetWorldPos, 0.0f, 0.0f, WinApp::kClientWidth,
										  WinApp::kClientHeight, camera->matView, camera->matProjection);

		// 3. ロックオンマーク（スプライト）の座標を設定する
		// ※ お使いのライブラリの関数名（SetPosition や SetTranslate など）に合わせて調整してください
		sprite_->SetPosition({targetScreenPos.x, targetScreenPos.y});
	}

	// 描画情報をGPUに反映するため、スプライトのUpdateを呼ぶ
	sprite_->Update();
}

void LockOn::Draw() {
	// ロックオン対象が存在するときのみ描画する
	if (target_) {
		sprite_->Draw();
	}
}
