#include "LockOn.h"

#include <MyEngine.h>
#include <MathUtility.h>
#include <Vector2.h>
#include <cmath>
#include <numbers>
#include <algorithm>

#include "EnemyBase.h"
#include "Camera.h"
#include "Player.h"
#include "WireframeObject.h"
#include "AudioManager.h"

using namespace MathUtility;

LockOn::LockOn() = default;
LockOn::~LockOn() = default;

void LockOn::Initialize() {
}

// 1. スリムで繊細な外枠ブラケット [ ]
void LockOn::CreateSlimBracketMesh(WireframeObject* obj, float size, float cornerLen) {
	std::vector<Vector4> vertices = {
		// 左上
		{-size,              size - cornerLen,  0.0f, 1.0f}, {-size,             size,              0.0f, 1.0f},
		{-size,              size,              0.0f, 1.0f}, {-size + cornerLen, size,              0.0f, 1.0f},
		// 右上
		{ size - cornerLen,  size,              0.0f, 1.0f}, { size,             size,              0.0f, 1.0f},
		{ size,              size,              0.0f, 1.0f}, { size,             size - cornerLen,  0.0f, 1.0f},
		// 右下
		{ size,             -size + cornerLen,  0.0f, 1.0f}, { size,            -size,              0.0f, 1.0f},
		{ size,             -size,              0.0f, 1.0f}, { size - cornerLen, -size,             0.0f, 1.0f},
		// 左下
		{-size + cornerLen, -size,              0.0f, 1.0f}, {-size,            -size,              0.0f, 1.0f},
		{-size,             -size,              0.0f, 1.0f}, {-size,            -size + cornerLen,  0.0f, 1.0f}
	};

	std::vector<uint32_t> indices;
	indices.reserve(16);
	for (uint32_t i = 0; i < 16; ++i) {
		indices.push_back(i);
	}

	obj->CreateCustomMesh(vertices, indices);
}

// 2. 3分割セグメントアーク（SF定番の切れ目のある回転円弧）
void LockOn::CreateSegmentArcMesh(WireframeObject* obj, float radius, int segments, float arcRatio) {
	std::vector<Vector4> vertices;
	std::vector<uint32_t> indices;

	const int subdivPerArc = 8;
	const float twoPi = 2.0f * std::numbers::pi_v<float>;
	const float segmentAngle = twoPi / static_cast<float>(segments);
	const float arcAngle = segmentAngle * arcRatio;

	for (int s = 0; s < segments; ++s) {
		float startAngle = static_cast<float>(s) * segmentAngle;
		uint32_t baseIdx = static_cast<uint32_t>(vertices.size());

		for (int i = 0; i <= subdivPerArc; ++i) {
			float angle = startAngle + arcAngle * (static_cast<float>(i) / static_cast<float>(subdivPerArc));
			vertices.push_back({std::cos(angle) * radius, std::sin(angle) * radius, 0.0f, 1.0f});

			if (i > 0) {
				indices.push_back(baseIdx + i - 1);
				indices.push_back(baseIdx + i);
			}
		}
	}

	obj->CreateCustomMesh(vertices, indices);
}

// 3. 中心オープン型精密クロスヘア（中心を抜いた4本の爪 ＋ 極小ダイヤ）
void LockOn::CreateCenterCrossMesh(WireframeObject* obj, float gapRadius, float length) {
	float d = gapRadius * 0.55f; // ダイヤのサイズ

	std::vector<Vector4> vertices = {
		// 4方向の短い照準ライン
		{ gapRadius,  0.0f,       0.0f, 1.0f}, { length,     0.0f,       0.0f, 1.0f}, // 右
		{-gapRadius,  0.0f,       0.0f, 1.0f}, {-length,     0.0f,       0.0f, 1.0f}, // 左
		{ 0.0f,       gapRadius,  0.0f, 1.0f}, { 0.0f,       length,     0.0f, 1.0f}, // 上
		{ 0.0f,      -gapRadius,  0.0f, 1.0f}, { 0.0f,      -length,     0.0f, 1.0f}, // 下

		// 中心極小ダイヤ (8-11)
		{ 0.0f,  d,   0.0f, 1.0f},
		{ d,    0.0f, 0.0f, 1.0f},
		{ 0.0f, -d,   0.0f, 1.0f},
		{-d,    0.0f, 0.0f, 1.0f}
	};

	std::vector<uint32_t> indices = {
		0, 1,  2, 3,  4, 5,  6, 7,      // 4方向ライン
		8, 9,  9, 10, 10, 11, 11, 8     // ダイヤ
	};

	obj->CreateCustomMesh(vertices, indices);
}

void LockOn::Update(Player* player, std::list<EnemyBase*>& enemies, const Camera* camera) {
	// ロックオンモードではない、またはホーミング弾の残弾が0の場合はロックオン対象をクリアして終了
	if (!player->GetIsLockOnMode() || player->GetHomingAmmo() <= 0) {
		ClearTargets();
		return;
	}

	// 自機のワールド座標を取得する
	Vector3 playerPositionWorld = player->GetWorldPosition();
	// ビュー座標に変換する
	Vector3 playerPositionView = TransformCoord(playerPositionWorld, camera->matView);

	// 生存している敵、プレイヤーより前方、かつ「まだホーミング弾が向かっていない敵」だけに絞り込む
	targetInfos_.erase(
		std::remove_if(targetInfos_.begin(), targetInfos_.end(),
			[&enemies, camera, &playerPositionView, player](const TargetInfo& info) {
				// 敵がすでに存在しない、または死亡している場合
				if (std::find(enemies.begin(), enemies.end(), info.enemy) == enemies.end() ||
					!info.enemy->IsAlive()) {
					return true;
				}
				// 既にこの敵に向けてホーミング弾が飛行中の場合はロックオン解除（1体につき1発のみ）
				if (player->IsTargetedByActiveHomingBullet(info.enemy)) {
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

	// targets_ も targetInfos_ に同期させる（残弾数以上のロックオンは除外）
	while (targetInfos_.size() > static_cast<size_t>(player->GetHomingAmmo())) {
		targetInfos_.pop_back();
	}
	targets_.clear();
	for (const auto& info : targetInfos_) {
		targets_.push_back(info.enemy);
	}

	// ロックオン判定処理
	for (EnemyBase* enemy : enemies) {
		// 生存していない敵はスキップ
		if (!enemy || !enemy->IsAlive()) {
			continue;
		}

		// すでに飛行中のホーミング弾のターゲットになっている敵はスキップ（1体の敵には1発のみ！）
		if (player->IsTargetedByActiveHomingBullet(enemy)) {
			continue;
		}

		// すでにロックオンされている場合はスキップ
		auto it = std::find_if(targetInfos_.begin(), targetInfos_.end(),
			[enemy](const TargetInfo& info) { return info.enemy == enemy; });
		if (it != targetInfos_.end()) {
			continue;
		}

		// 残弾数、または最大ロックオン数に達している場合はスキップ
		size_t maxLockOn = (std::min)(kMaxLockOnTargets, static_cast<size_t>(player->GetHomingAmmo()));
		if (targetInfos_.size() >= maxLockOn) {
			break;
		}

		// 敵のワールド座標を取得
		Vector3 positionWorld = enemy->GetWorldPosition();

		// ビュー座標系に変換する
		Vector3 positionView = TransformCoord(positionWorld, camera->matView);

		// 敵のビュー座標.z が 自機のビュー座標.z 以下なら除外
		if (positionView.z <= playerPositionView.z) {
			continue;
		}

		// ワールド座標からスクリーン座標に変換
		Vector3 positionScreen = Project(positionWorld, 0.0f, 0.0f, WinApp::kClientWidth, 
										 WinApp::kClientHeight, camera->matView, camera->matProjection);

		// Vector2に格納
		Vector2 positionScreenV2(positionScreen.x, positionScreen.y);
		// レティクルとの距離
		float distance = Distance(player->GetReticle2DPosition(), positionScreenV2);
		if (distance <= kDistanceLockOn) {
			TargetInfo newInfo;
			newInfo.enemy = enemy;

			auto setupWireframe = [camera](std::unique_ptr<WireframeObject>& obj) {
				obj = std::make_unique<WireframeObject>();
				obj->Initialize();
				obj->SetCamera(const_cast<Camera*>(camera));
				obj->SetDepthEnable(false);         // 最前面に描画
				obj->SetBlendMode(BlendMode::kAdd); // 加算合成でネオン発光
			};

			// 1. 外枠スリムブラケット
			setupWireframe(newInfo.outerBracket);
			CreateSlimBracketMesh(newInfo.outerBracket.get(), 1.0f, 0.22f);

			// 2. 中間ヘキサゴン（逆回転する六角形）
			setupWireframe(newInfo.middleHexagon);
			newInfo.middleHexagon->CreateRing(0.85f, 6, 0);

			// 3. 内側セグメントアーク（3分割された洗練円弧）
			setupWireframe(newInfo.segmentArc);
			CreateSegmentArcMesh(newInfo.segmentArc.get(), 0.68f, 3, 0.72f);

			// 4. 中心オープン型精密クロスヘア
			setupWireframe(newInfo.centerCross);
			CreateCenterCrossMesh(newInfo.centerCross.get(), 0.16f, 0.35f);

			// 5. 第1＆第2 展開パルスリング（ダブル衝撃波）
			setupWireframe(newInfo.shockwaveRing1);
			newInfo.shockwaveRing1->CreateRing(1.0f, 32, 0);

			setupWireframe(newInfo.shockwaveRing2);
			newInfo.shockwaveRing2->CreateRing(1.0f, 32, 0);

			// 6. 自機〜敵を繋ぐサイバーレーザー線
			setupWireframe(newInfo.dataLinkLine);
			newInfo.dataLinkLine->CreateDynamicLineMesh(1);

			newInfo.lockOnTime = 0.0f;
			newInfo.shockwaveTime = 0.0f;

			targetInfos_.push_back(std::move(newInfo));
			targets_.push_back(enemy);
		}
	}

	// 演出更新用のLerpヘルパー
	auto lerp = [](float start, float end, float t) {
		return start + (end - start) * t;
	};

	Vector3 camRot = camera->GetRotate();

	// 各ターゲットに対してワイヤーフレームオブジェクトの位置とアニメーションを更新
	for (auto& info : targetInfos_) {
		info.lockOnTime += 1.0f / 60.0f;

		// ロックオン対象のワールド座標を取得する
		Vector3 targetWorldPos = info.enemy->GetWorldPosition();

		// カメラ空間でのZ距離を計算し、引き締まったサイズ感をキープ
		Vector3 viewPos = TransformCoord(targetWorldPos, camera->matView);
		float depth = (std::max)(viewPos.z, 1.0f);
		float distScale = 1.0f + (depth - 10.0f) * 0.016f;
		distScale = std::clamp(distScale, 0.65f, 1.8f);
		float baseSize = kBaseWireframeSize * distScale;

		// 成立時の高輝度フラッシュ係数 (0.04秒間ピカッと発光)
		float flashBoost = (info.lockOnTime < 0.05f) ? 1.5f : 1.0f;

		// --- 1. 外枠スリムブラケット (オーバーシュート付きガシャン収束) ---
		if (info.outerBracket) {
			float t = (std::min)(info.lockOnTime / 0.14f, 1.0f);
			// BackEaseOut 的なオーバーシュート（一瞬縮みすぎてから定位置へ戻るメカニカル感）
			float c1 = 1.70158f;
			float c3 = c1 + 1.0f;
			float p = t - 1.0f;
			float easeT = 1.0f + c3 * std::pow(p, 3.0f) + c1 * std::pow(p, 2.0f);
			float scaleFactor = lerp(2.5f, 1.0f, easeT);

			// 収束後の微細な呼吸パルス
			if (t >= 1.0f) {
				scaleFactor += 0.025f * std::sin((info.lockOnTime - 0.14f) * 6.0f);
			}

			// ロックオン成立直後の電脳ジッター
			Vector3 jitter = {0.0f, 0.0f, 0.0f};
			if (info.lockOnTime < 0.08f) {
				float rx = ((rand() % 100) / 100.0f - 0.5f) * 0.08f * baseSize;
				float ry = ((rand() % 100) / 100.0f - 0.5f) * 0.08f * baseSize;
				jitter = {rx, ry, 0.0f};
			}

			Vector3 bracketRot = camRot;
			info.outerBracket->SetPosition(targetWorldPos + jitter);
			info.outerBracket->SetRotation(bracketRot);
			float s = baseSize * scaleFactor;
			info.outerBracket->SetScale({s, s, s});

			Vector4 bracketColor = {0.2f * flashBoost, 0.85f * flashBoost, 1.0f * flashBoost, 0.85f};
			info.outerBracket->SetColor(bracketColor);
			info.outerBracket->Update();
		}

		// --- 2. 中間ヘキサゴン（逆回転＆スキャン形成） ---
		if (info.middleHexagon) {
			// 反時計回りにゆったり回転
			Vector3 hexRot = camRot;
			hexRot.z -= info.lockOnTime * 1.0f;

			// 登場時に1周スキャン形成
			float scanRatio = (std::min)(info.lockOnTime / 0.12f, 1.0f);
			info.middleHexagon->SetDrawRatio(scanRatio);

			info.middleHexagon->SetPosition(targetWorldPos);
			info.middleHexagon->SetRotation(hexRot);
			float s = baseSize * 1.0f;
			info.middleHexagon->SetScale({s, s, s});
			// スタイリッシュなネオンゴールド/アンバー
			Vector4 hexColor = {1.0f * flashBoost, 0.75f * flashBoost, 0.2f * flashBoost, 0.75f};
			info.middleHexagon->SetColor(hexColor);
			info.middleHexagon->Update();
		}

		// --- 3. 内側セグメントアーク（3分割円弧・時計回り高速回転） ---
		if (info.segmentArc) {
			Vector3 ringRot = camRot;
			ringRot.z += info.lockOnTime * 2.2f;

			info.segmentArc->SetPosition(targetWorldPos);
			info.segmentArc->SetRotation(ringRot);
			float s = baseSize * 1.0f;
			info.segmentArc->SetScale({s, s, s});
			// 透き通るターコイズミント
			Vector4 arcColor = {0.2f * flashBoost, 1.0f * flashBoost, 0.70f * flashBoost, 0.85f};
			info.segmentArc->SetColor(arcColor);
			info.segmentArc->Update();
		}

		// --- 4. 中心オープン型精密クロスヘア（反時計回り回転） ---
		if (info.centerCross) {
			Vector3 crossRot = camRot;
			crossRot.z -= info.lockOnTime * 2.8f;

			info.centerCross->SetPosition(targetWorldPos);
			info.centerCross->SetRotation(crossRot);
			float s = baseSize * 1.0f;
			info.centerCross->SetScale({s, s, s});
			// 鮮やかなネオンマゼンタレッド
			Vector4 crossColor = {1.0f * flashBoost, 0.25f * flashBoost, 0.45f * flashBoost, 0.95f};
			info.centerCross->SetColor(crossColor);
			info.centerCross->Update();
		}

		// --- 5. ダブル展開パルスリング ---
		info.shockwaveTime += 1.0f / 60.0f;

		// 第1波（瞬発的な大波）
		if (info.shockwaveRing1) {
			float et1 = info.shockwaveTime / 0.18f;
			if (et1 >= 1.0f) {
				info.shockwaveRing1.reset();
			} else {
				float easeEt1 = 1.0f - std::pow(1.0f - et1, 2.0f);
				float ringScale = lerp(0.3f, 2.4f, easeEt1) * baseSize;
				float alpha = lerp(0.9f, 0.0f, et1);

				info.shockwaveRing1->SetPosition(targetWorldPos);
				info.shockwaveRing1->SetRotation(camRot);
				info.shockwaveRing1->SetScale({ringScale, ringScale, ringScale});
				info.shockwaveRing1->SetColor({0.4f, 0.95f, 1.0f, alpha});
				info.shockwaveRing1->Update();
			}
		}

		// 第2波（追撃の繊細な波：0.04秒遅れでスタート）
		if (info.shockwaveRing2) {
			float et2 = (info.shockwaveTime - 0.04f) / 0.18f;
			if (et2 < 0.0f) {
				// まだ待機中（非表示）
				info.shockwaveRing2->SetScale({0.0f, 0.0f, 0.0f});
				info.shockwaveRing2->Update();
			} else if (et2 >= 1.0f) {
				info.shockwaveRing2.reset();
			} else {
				float easeEt2 = 1.0f - std::pow(1.0f - et2, 2.0f);
				float ringScale = lerp(0.2f, 1.9f, easeEt2) * baseSize;
				float alpha = lerp(0.8f, 0.0f, et2);

				info.shockwaveRing2->SetPosition(targetWorldPos);
				info.shockwaveRing2->SetRotation(camRot);
				info.shockwaveRing2->SetScale({ringScale, ringScale, ringScale});
				info.shockwaveRing2->SetColor({1.0f, 0.6f, 0.2f, alpha}); // アンバー色の第2波
				info.shockwaveRing2->Update();
			}
		}

		// --- 6. 自機〜敵を繋ぐサイバーレーザー誘導線 ---
		if (info.dataLinkLine) {
			// ロックオン開始時に自機から敵へ超高速でビビッと伸びる (0.06秒で到達)
			float beamT = (std::min)(info.lockOnTime / 0.06f, 1.0f);
			Vector3 laserEnd = playerPositionWorld + (targetWorldPos - playerPositionWorld) * beamT;

			// 頂点更新
			info.dataLinkLine->UpdateDynamicLines({{playerPositionWorld, laserEnd}});

			// 自機と敵のワールド座標を直接書き込んでいるため transform は単位行列で更新
			info.dataLinkLine->SetPosition({0.0f, 0.0f, 0.0f});
			info.dataLinkLine->SetRotation({0.0f, 0.0f, 0.0f});
			info.dataLinkLine->SetScale({1.0f, 1.0f, 1.0f});

			// 明滅パルス（サイン波でビビビッと光る）
			float pulse = 0.5f + 0.35f * std::sin(info.lockOnTime * 30.0f);
			info.dataLinkLine->SetColor({0.15f, 0.85f, 1.0f, pulse});
			info.dataLinkLine->Update();
		}
	}
}

void LockOn::Draw() {
	// 現在のターゲット数分だけワイヤーフレームを描画する
	for (const auto& info : targetInfos_) {
		// レーザー誘導線
		if (info.dataLinkLine) {
			info.dataLinkLine->Draw();
		}
		// 衝撃波
		if (info.shockwaveRing1) {
			info.shockwaveRing1->Draw();
		}
		if (info.shockwaveRing2) {
			info.shockwaveRing2->Draw();
		}
		// メインHUDパーツ
		if (info.outerBracket) {
			info.outerBracket->Draw();
		}
		if (info.middleHexagon) {
			info.middleHexagon->Draw();
		}
		if (info.segmentArc) {
			info.segmentArc->Draw();
		}
		if (info.centerCross) {
			info.centerCross->Draw();
		}
	}
}

void LockOn::ClearTargets() {
	targets_.clear();
	targetInfos_.clear();
}
