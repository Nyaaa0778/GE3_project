#include "LockOn.h"

#include <MyEngine.h>
#include <MathUtility.h>
#include <Vector2.h>
#include <cmath>

#include "EnemyBase.h"
#include "Camera.h"
#include "Player.h"
#include "Sprite.h"
#include "AudioManager.h"

using namespace MathUtility;

LockOn::LockOn() = default;
LockOn::~LockOn() = default;

void LockOn::Initialize() {
	// ロックオンSEのロード
	lockOnSeHandle_ = AudioManager::LoadAudio("Alarm01.wav", SoundGroup::SE);
	isSeLoaded_ = true;
}

void LockOn::Update(Player* player, std::list<EnemyBase*>& enemies, const Camera* camera) {
	// ロックオンモードではない場合はロックオン対象をクリアして終了
	if (!player->GetIsLockOnMode()) {
		ClearTargets();
		return;
	}

	// 自機のワールド座標を取得する
	Vector3 playerPositionWorld = player->GetWorldPosition();
	// ビュー座標に変換する
	Vector3 playerPositionView = TransformCoord(playerPositionWorld, camera->matView);

	// 生存している敵、かつプレイヤーより前方にいる敵だけに絞り込む
	targetInfos_.erase(
		std::remove_if(targetInfos_.begin(), targetInfos_.end(),
			[&enemies, camera, &playerPositionView](const TargetInfo& info) {
				// 敵がすでに存在しない場合
				if (std::find(enemies.begin(), enemies.end(), info.enemy) == enemies.end()) {
					return true;
				}
				// 敵のビュー座標を計算
				Vector3 positionWorld = info.enemy->GetWorldPosition();
				Vector3 positionView = TransformCoord(positionWorld, camera->matView);
				// 自機より手前（ビュー空間でZが自機以下）にいる場合は除外
				if (positionView.z <= playerPositionView.z) {
					return true;
				}
				return false;
			}),
		targetInfos_.end());

	// targets_ も targetInfos_ に同期させる
	targets_.clear();
	for (const auto& info : targetInfos_) {
		targets_.push_back(info.enemy);
	}

	// ロックオン判定処理
	for (EnemyBase* enemy : enemies) {
		// すでにロックオンされている場合はスキップ
		auto it = std::find_if(targetInfos_.begin(), targetInfos_.end(),
			[enemy](const TargetInfo& info) { return info.enemy == enemy; });
		if (it != targetInfos_.end()) {
			continue;
		}

		// 最大ロックオン数に達している場合はスキップ
		if (targetInfos_.size() >= kMaxLockOnTargets) {
			break;
		}

		// 敵のワールド座標を取得
		Vector3 positionWorld = enemy->GetWorldPosition();

		// ビュー座標系に変換する
		Vector3 positionView = TransformCoord(positionWorld, camera->matView);

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
			TargetInfo newInfo;
			newInfo.enemy = enemy;

			// ロックオンスプライトの生成
			newInfo.sprite = std::make_unique<Sprite>();
			newInfo.sprite->Initialize("reticle.png", {640.0f, 360.0f}, {0.5f, 0.5f});
			newInfo.sprite->SetScale({kReticleDrawSize.x * 3.0f, kReticleDrawSize.y * 3.0f}); // 初期サイズは大きく
			newInfo.sprite->SetColor({1.0f, 0.9f, 0.2f, 1.0f}); // 初期カラーは黄色

			// 衝撃波（演出用）スプライトの生成
			newInfo.effectSprite = std::make_unique<Sprite>();
			newInfo.effectSprite->Initialize("shockwave.png", {640.0f, 360.0f}, {0.5f, 0.5f});
			newInfo.effectSprite->SetScale({0.0f, 0.0f});
			newInfo.effectSprite->SetColor({1.0f, 0.5f, 0.1f, 0.8f}); // オレンジ色の光
			newInfo.effectSprite->SetBlendMode(SpriteRenderer::BlendMode::kAdd); // 加算合成で発光
			newInfo.effectTime = 0.0f;

			newInfo.lockOnTime = 0.0f;

			targetInfos_.push_back(std::move(newInfo));
			targets_.push_back(enemy);

			// SEの再生
			if (isSeLoaded_) {
				AudioManager::PlayOneShot(lockOnSeHandle_, 0.4f);
			}
		}
	}

	// 演出更新用のLerpヘルパー
	auto lerp = [](float start, float end, float t) {
		return start + (end - start) * t;
	};

	// 各ターゲットに対してスプライトの位置とアニメーションを更新
	for (auto& info : targetInfos_) {
		// ロックオン対象のワールド座標を取得する
		Vector3 targetWorldPos = info.enemy->GetWorldPosition();

		// ワールド座標からスクリーン座標に変換する
		Vector3 targetScreenPos = Project(targetWorldPos, 0.0f, 0.0f, WinApp::kClientWidth,
										  WinApp::kClientHeight, camera->matView, camera->matProjection);

		// 経過時間を更新（1フレームを 1/60 秒と仮定）
		info.lockOnTime += 1.0f / 60.0f;

		// 1. メインロックオンスプライトの更新
		float t = info.lockOnTime / 0.15f; // 0.15秒で登場アニメーションを完了
		if (t > 1.0f) t = 1.0f;

		// EaseOutCubic のイージング
		float easeT = 1.0f - std::pow(1.0f - t, 3.0f);

		// スケール: 3.0f -> 1.0f に収縮
		float scaleFactor = lerp(3.0f, 1.0f, easeT);
		
		// 定常時のパルス効果 (呼吸エフェクト: サイン波)
		if (t >= 1.0f) {
			scaleFactor += 0.08f * std::sin((info.lockOnTime - 0.15f) * 10.0f);
		}

		info.sprite->SetScale({kReticleDrawSize.x * scaleFactor, kReticleDrawSize.y * scaleFactor});

		// 回転: 登場時の急速スピン + 定常の低速スピン
		float rotation = lerp(3.14f, 0.0f, easeT) + info.lockOnTime * 1.5f;
		info.sprite->SetRotation(rotation);

		// カラー: 白黄色 -> 赤色 に遷移
		Vector4 color;
		color.x = lerp(1.0f, 1.0f, easeT);
		color.y = lerp(0.9f, 0.1f, easeT);
		color.z = lerp(0.2f, 0.1f, easeT);
		color.w = 1.0f;
		info.sprite->SetColor(color);

		// 座標設定
		info.sprite->SetPosition({targetScreenPos.x, targetScreenPos.y});
		info.sprite->Update();

		// 2. 衝撃波エフェクトの更新
		if (info.effectSprite) {
			info.effectTime += 1.0f / 60.0f;
			float et = info.effectTime / 0.25f; // 0.25秒でエフェクト完了

			if (et >= 1.0f) {
				info.effectSprite.reset(); // 役目を終えたら破棄
			} else {
				// スケール: 0.0 -> 3.5倍 に拡大
				float effectScaleFactor = lerp(0.0f, 3.5f, et);
				info.effectSprite->SetScale({kReticleDrawSize.x * effectScaleFactor, kReticleDrawSize.y * effectScaleFactor});

				// 不透明度: フェードアウト
				float alpha = lerp(0.8f, 0.0f, et);
				info.effectSprite->SetColor({1.0f, 0.5f, 0.1f, alpha});

				info.effectSprite->SetPosition({targetScreenPos.x, targetScreenPos.y});
				info.effectSprite->Update();
			}
		}
	}
}

void LockOn::Draw() {
	// 現在のターゲット数分だけスプライトを描画する
	for (const auto& info : targetInfos_) {
		if (info.effectSprite) {
			info.effectSprite->Draw();
		}
		if (info.sprite) {
			info.sprite->Draw();
		}
	}
}

void LockOn::ClearTargets() {
	targets_.clear();
	targetInfos_.clear();
}

