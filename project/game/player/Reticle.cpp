#include "Reticle.h"

#include "Camera.h"
#include "WireframeObject.h"
#include <MathUtility.h>
#include <TimeManager.h>
#include <WinApp.h>
#include <cmath>
#include <numbers>
#include <algorithm>

using namespace MathUtility;

Reticle::Reticle() = default;
Reticle::~Reticle() = default;

void Reticle::Initialize(Camera* camera) {
	camera_ = camera;

	// 1. インナー（中心：静的・高密度精密ドット/二重十字/データスケール/コーナー枠）
	innerReticle_ = std::make_unique<WireframeObject>();
	innerReticle_->Initialize();
	innerReticle_->SetCamera(camera_);
	innerReticle_->SetDepthEnable(false); // HUDとして最前面表示
	innerReticle_->SetBlendMode(BlendMode::kNormal); // 明瞭なソリッド発色
	CreateInnerMesh();

	// 2. アウター主枠（外枠：二重六角フレーム/コーナーピン/シェブロン矢印/左右ステップゲージ）
	outerReticle_ = std::make_unique<WireframeObject>();
	outerReticle_->Initialize();
	outerReticle_->SetCamera(camera_);
	outerReticle_->SetDepthEnable(false);
	outerReticle_->SetBlendMode(BlendMode::kAdd); // ネオン加算発光
	CreateOuterMesh();

	// 3. ミドル装飾リング（逆回転ジャイロ/二重セグメント円弧/8方位コンパス目盛り）
	middleRing_ = std::make_unique<WireframeObject>();
	middleRing_->Initialize();
	middleRing_->SetCamera(camera_);
	middleRing_->SetDepthEnable(false);
	middleRing_->SetBlendMode(BlendMode::kAdd); // ネオン加算発光
	CreateRingMesh();

	// 4. 慣性トラッキングリンク（インナーとアウターのズレを結ぶダイナミックレーザー）
	inertiaLink_ = std::make_unique<WireframeObject>();
	inertiaLink_->Initialize();
	inertiaLink_->SetCamera(camera_);
	inertiaLink_->SetDepthEnable(false);
	inertiaLink_->SetBlendMode(BlendMode::kAdd);
	CreateInertiaLinkMesh();

	// カラーの初期適用（デフォルトはシアン/マゼンタ背景に最も映えるCyber Amber & Solar White）
	UpdateColors();

	isFirstUpdate_ = true;
	outerSpinAngle_ = 0.0f;
	ringSpinAngle_ = 0.0f;
	breatheTimer_ = 0.0f;
}

void Reticle::Update(const WorldTransform& playerWorldTransform) {
	const float kDistancePlayerToReticle = 50.0f;
	const float kReticleScaleX = 2.5f; // 照準の横方向の可動域倍率
	const float kReticleScaleY = 2.5f; // 照準の縦方向の可動域倍率

	Vector3 reticleWorldPos = {};

	if (playerWorldTransform.parent) {
		// 親（レールカメラ）のローカル空間で照準位置を計算（自機の位置に比例してさらに外側へ）
		Vector3 reticleLocalPos = {};
		reticleLocalPos.x = playerWorldTransform.translation.x * kReticleScaleX;
		reticleLocalPos.y = playerWorldTransform.translation.y * kReticleScaleY;
		reticleLocalPos.z = playerWorldTransform.translation.z + kDistancePlayerToReticle;

		reticleWorldPos = TransformCoord(reticleLocalPos, playerWorldTransform.parent->matWorld);
	} else {
		// 親がいない場合のフォールバック（正面方向へ配置）
		Vector3 forward = { playerWorldTransform.matWorld.m[2][0], playerWorldTransform.matWorld.m[2][1], playerWorldTransform.matWorld.m[2][2] };
		if (Length(forward) > 0.0001f) {
			forward = Normalize(forward);
		} else {
			forward = { 0.0f, 0.0f, 1.0f };
		}
		reticleWorldPos = playerWorldTransform.GetWorldPosition() + forward * kDistancePlayerToReticle;
	}

	targetWorldPos_ = reticleWorldPos;

	float dt = TimeManager::GetInstance()->GetDeltaTime();
	if (dt <= 0.0f) {
		dt = 1.0f / 60.0f;
	}

	// 呼吸パルス（サイン波でわずかに脈動し、生命感を出す）
	breatheTimer_ += dt * 3.5f;
	float breathe = 1.0f + 0.035f * std::sin(breatheTimer_);

	// ----------------------------------------------------
	// 慣性ラグ（エイムの慣性）の計算
	// ----------------------------------------------------
	if (isFirstUpdate_) {
		laggedWorldPos_ = targetWorldPos_;
		isFirstUpdate_ = false;
	} else {
		// 指数減衰による追従（自機移動時は遅れて慣性が働き、停止時は素早く収束）
		const float kLagFollowSpeed = 11.0f;
		float followFactor = 1.0f - std::exp(-kLagFollowSpeed * dt);
		laggedWorldPos_.x += (targetWorldPos_.x - laggedWorldPos_.x) * followFactor;
		laggedWorldPos_.y += (targetWorldPos_.y - laggedWorldPos_.y) * followFactor;
		laggedWorldPos_.z += (targetWorldPos_.z - laggedWorldPos_.z) * followFactor;
	}

	// インナーとアウターのオフセット（慣性・移動速度の目安）
	Vector3 offsetVec = Subtract(targetWorldPos_, laggedWorldPos_);
	float offsetDist = Length(offsetVec);
	// 慣性強度 (0.0f 〜 1.0f)
	float lagIntensity = (std::min)(offsetDist / 2.2f, 1.0f);

	// ----------------------------------------------------
	// アウターの動的変形（伸縮・回転・変形）
	// ----------------------------------------------------
	// 1. 伸縮（高速移動時は拡散・拡大、静止時は収束＋微細な呼吸）
	float dynamicScale = (1.0f + lagIntensity * 0.45f) * breathe;
	Vector3 outerScale = { dynamicScale, dynamicScale, 1.0f };

	// 2. 変形（自機の移動方向に応じた扁平率のダイナミックストレッチ）
	if (offsetDist > 0.02f) {
		Vector3 dir = Normalize(offsetVec);
		outerScale.x += std::abs(dir.x) * lagIntensity * 0.25f;
		outerScale.y += std::abs(dir.y) * lagIntensity * 0.25f;
	}

	// 3. 回転（アイドル時は緩やか、移動時はスピン急加速、主枠とリングは逆回転ジャイロ）
	float outerSpinSpeed = 0.45f + lagIntensity * 3.0f;
	float ringSpinSpeed  = 0.75f + lagIntensity * 4.2f;
	outerSpinAngle_ += outerSpinSpeed * dt;
	ringSpinAngle_  += ringSpinSpeed * dt;

	// ----------------------------------------------------
	// カメラ正対（ビルボード）トランスフォームの適用
	// ----------------------------------------------------
	Vector3 camRot = camera_ ? camera_->GetRotate() : Vector3{ 0.0f, 0.0f, 0.0f };

	// 1. インナー（即時位置・静的・精密照準）
	if (innerReticle_) {
		innerReticle_->SetPosition(targetWorldPos_);
		innerReticle_->SetRotation(camRot);
		innerReticle_->SetScale({ 1.0f, 1.0f, 1.0f });
		innerReticle_->Update();
	}

	// 2. アウター主枠（慣性ラグ位置・伸縮・時計回りスピン・変形）
	if (outerReticle_) {
		outerReticle_->SetPosition(laggedWorldPos_);
		outerReticle_->SetRotation({ camRot.x, camRot.y, camRot.z + outerSpinAngle_ });
		outerReticle_->SetScale(outerScale);
		outerReticle_->Update();
	}

	// 3. ミドル装飾リング（慣性ラグ位置・反時計回り逆回転ジャイロ）
	if (middleRing_) {
		middleRing_->SetPosition(laggedWorldPos_);
		middleRing_->SetRotation({ camRot.x, camRot.y, camRot.z - ringSpinAngle_ });
		middleRing_->SetScale({ dynamicScale * 0.95f, dynamicScale * 0.95f, 1.0f });
		middleRing_->Update();
	}

	// 4. 慣性トラッキングリンク（インナーとアウターのズレを繋ぐ動的ライン）
	if (inertiaLink_) {
		// 慣性のズレの大きさに応じて描画比率を動的に変化（静止時は完全に非表示、移動時にパッと出現）
		float linkRatio = (std::clamp)((offsetDist - 0.08f) / 0.5f, 0.0f, 1.0f);
		inertiaLink_->SetDrawRatio(linkRatio);

		// 位置はインナーとアウターの中間点に配置
		Vector3 midPos = {
			(targetWorldPos_.x + laggedWorldPos_.x) * 0.5f,
			(targetWorldPos_.y + laggedWorldPos_.y) * 0.5f,
			(targetWorldPos_.z + laggedWorldPos_.z) * 0.5f
		};
		inertiaLink_->SetPosition(midPos);
		inertiaLink_->SetRotation(camRot);
		// ズレの大きさに応じてスケール
		float stretch = (std::max)(offsetDist * 0.5f, 0.2f);
		inertiaLink_->SetScale({ stretch, stretch, 1.0f });
		inertiaLink_->Update();
	}
}

void Reticle::Draw() {
	// 1. 慣性トラッキングリンク（最背面）
	if (inertiaLink_ && inertiaLink_->GetDrawRatio() > 0.01f) {
		inertiaLink_->Draw();
	}
	// 2. ミドル装飾リング（逆回転ジャイロ）
	if (middleRing_) {
		middleRing_->Draw();
	}
	// 3. アウター二重六角主枠
	if (outerReticle_) {
		outerReticle_->Draw();
	}
	// 4. インナー精密中心核（最前面にソリッド高輝度表示）
	if (innerReticle_) {
		innerReticle_->Draw();
	}
}

void Reticle::SetColorTheme(ColorTheme theme) {
	colorTheme_ = theme;
	UpdateColors();
}

void Reticle::UpdateColors() {
	// 背景（シアン＆マゼンタ）と同化しない高視認性カラーテーマ
	Vector4 outerColor;
	Vector4 ringColor;
	Vector4 innerColor;
	Vector4 linkColor;

	switch (colorTheme_) {
	case ColorTheme::kCyberAmberWhite:
	default:
		// [推奨] 鮮烈なエレクトリック・アンバー ＆ ソーラーピュアホワイト
		// シアン（青）ともマゼンタ（赤紫）とも絶対にかぶらない最高峰の補色コントラスト
		outerColor = { 1.0f, 0.52f, 0.02f, 1.0f }; // 鮮烈なエレクトリック・アンバー（橙/琥珀）
		ringColor  = { 1.0f, 0.88f, 0.12f, 0.95f }; // ネオンゴールド/イエロー
		innerColor = { 1.0f, 1.0f, 1.0f, 1.0f };    // ソーラーピュアホワイト（核）
		linkColor  = { 1.0f, 0.60f, 0.05f, 0.85f }; // アンバーレーザー
		break;

	case ColorTheme::kElectricLime:
		// 超蛍光アシッドライム ＆ ハイパーシトロン
		outerColor = { 0.15f, 1.0f, 0.20f, 1.0f }; // 超蛍光ライムグリーン
		ringColor  = { 0.95f, 1.0f, 0.05f, 0.95f }; // ハイパーシトロンイエロー
		innerColor = { 1.0f, 1.0f, 1.0f, 1.0f };    // ピュアホワイト
		linkColor  = { 0.30f, 1.0f, 0.30f, 0.85f }; // ライムレーザー
		break;

	case ColorTheme::kNeonGoldMagenta:
		// ネオンゴールド ＆ サイバーマゼンタ（華麗なネオンツートン）
		outerColor = { 1.0f, 0.85f, 0.10f, 1.0f }; // ネオンゴールド
		ringColor  = { 1.0f, 0.15f, 0.85f, 0.95f }; // ネオンマゼンタ
		innerColor = { 1.0f, 1.0f, 1.0f, 1.0f };    // ピュアホワイト
		linkColor  = { 1.0f, 0.85f, 0.15f, 0.85f };
		break;

	case ColorTheme::kSolarCrimson:
		// プラズマ・クリムゾン ＆ ホワイト（戦闘機HUDテイスト）
		outerColor = { 1.0f, 0.18f, 0.22f, 1.0f }; // プラズマクリムゾン
		ringColor  = { 1.0f, 0.68f, 0.10f, 0.95f }; // ネオンアンバー
		innerColor = { 1.0f, 1.0f, 1.0f, 1.0f };    // ピュアホワイト
		linkColor  = { 1.0f, 0.25f, 0.30f, 0.85f };
		break;
	}

	if (outerReticle_) {
		outerReticle_->SetColor(outerColor);
	}
	if (middleRing_) {
		middleRing_->SetColor(ringColor);
	}
	if (innerReticle_) {
		innerReticle_->SetColor(innerColor);
	}
	if (inertiaLink_) {
		inertiaLink_->SetColor(linkColor);
	}
}

void Reticle::CreateInnerMesh() {
	// インナー（中心）：静的・高密度精密ドット/二重十字/データスケール/コーナー枠
	std::vector<Vector4> vertices;
	std::vector<uint32_t> indices;

	// A. 中心高輝度ダイヤモンド（核）
	const float rDia = 0.13f;
	uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({  0.0f,   rDia, 0.0f, 1.0f }); // 0: 上
	vertices.push_back({  rDia,   0.0f, 0.0f, 1.0f }); // 1: 右
	vertices.push_back({  0.0f,  -rDia, 0.0f, 1.0f }); // 2: 下
	vertices.push_back({ -rDia,   0.0f, 0.0f, 1.0f }); // 3: 左
	indices.push_back(baseIdx + 0); indices.push_back(baseIdx + 1);
	indices.push_back(baseIdx + 1); indices.push_back(baseIdx + 2);
	indices.push_back(baseIdx + 2); indices.push_back(baseIdx + 3);
	indices.push_back(baseIdx + 3); indices.push_back(baseIdx + 0);

	// B. 精密中心微小ドット十字（クロス）
	const float rDot = 0.05f;
	baseIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({ -rDot,  0.0f, 0.0f, 1.0f });
	vertices.push_back({  rDot,  0.0f, 0.0f, 1.0f });
	vertices.push_back({  0.0f, -rDot, 0.0f, 1.0f });
	vertices.push_back({  0.0f,  rDot, 0.0f, 1.0f });
	indices.push_back(baseIdx + 0); indices.push_back(baseIdx + 1);
	indices.push_back(baseIdx + 2); indices.push_back(baseIdx + 3);

	// C. 4方向の二重精密十字ライン（平行ラインで線の太さと存在感を大幅アップ）
	const float gap = 0.20f;
	const float len = 0.58f;
	const float thick = 0.018f; // 平行線の隙間

	// 上十字（二重ライン）
	baseIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({ -thick, gap, 0.0f, 1.0f });
	vertices.push_back({ -thick, len, 0.0f, 1.0f });
	vertices.push_back({  thick, gap, 0.0f, 1.0f });
	vertices.push_back({  thick, len, 0.0f, 1.0f });
	// 先端を繋ぐ
	vertices.push_back({ -thick, len, 0.0f, 1.0f });
	vertices.push_back({  thick, len, 0.0f, 1.0f });
	indices.push_back(baseIdx + 0); indices.push_back(baseIdx + 1);
	indices.push_back(baseIdx + 2); indices.push_back(baseIdx + 3);
	indices.push_back(baseIdx + 4); indices.push_back(baseIdx + 5);

	// 下十字（二重ライン）
	baseIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({ -thick, -gap, 0.0f, 1.0f });
	vertices.push_back({ -thick, -len, 0.0f, 1.0f });
	vertices.push_back({  thick, -gap, 0.0f, 1.0f });
	vertices.push_back({  thick, -len, 0.0f, 1.0f });
	vertices.push_back({ -thick, -len, 0.0f, 1.0f });
	vertices.push_back({  thick, -len, 0.0f, 1.0f });
	indices.push_back(baseIdx + 0); indices.push_back(baseIdx + 1);
	indices.push_back(baseIdx + 2); indices.push_back(baseIdx + 3);
	indices.push_back(baseIdx + 4); indices.push_back(baseIdx + 5);

	// 左十字（二重ライン）
	baseIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({ -gap, -thick, 0.0f, 1.0f });
	vertices.push_back({ -len, -thick, 0.0f, 1.0f });
	vertices.push_back({ -gap,  thick, 0.0f, 1.0f });
	vertices.push_back({ -len,  thick, 0.0f, 1.0f });
	vertices.push_back({ -len, -thick, 0.0f, 1.0f });
	vertices.push_back({ -len,  thick, 0.0f, 1.0f });
	indices.push_back(baseIdx + 0); indices.push_back(baseIdx + 1);
	indices.push_back(baseIdx + 2); indices.push_back(baseIdx + 3);
	indices.push_back(baseIdx + 4); indices.push_back(baseIdx + 5);

	// 右十字（二重ライン）
	baseIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({ gap, -thick, 0.0f, 1.0f });
	vertices.push_back({ len, -thick, 0.0f, 1.0f });
	vertices.push_back({ gap,  thick, 0.0f, 1.0f });
	vertices.push_back({ len,  thick, 0.0f, 1.0f });
	vertices.push_back({ len, -thick, 0.0f, 1.0f });
	vertices.push_back({ len,  thick, 0.0f, 1.0f });
	indices.push_back(baseIdx + 0); indices.push_back(baseIdx + 1);
	indices.push_back(baseIdx + 2); indices.push_back(baseIdx + 3);
	indices.push_back(baseIdx + 4); indices.push_back(baseIdx + 5);

	// D. 3段階のHUD目盛りスケールバー（0.30m, 0.42m, 0.52m）
	const float ticks[3] = { 0.32f, 0.44f, 0.54f };
	const float tickLens[3] = { 0.04f, 0.07f, 0.05f };

	for (int t = 0; t < 3; ++t) {
		float pos = ticks[t];
		float h = tickLens[t];
		baseIdx = static_cast<uint32_t>(vertices.size());
		// 上軸
		vertices.push_back({ -h, pos, 0.0f, 1.0f });
		vertices.push_back({  h, pos, 0.0f, 1.0f });
		// 下軸
		vertices.push_back({ -h, -pos, 0.0f, 1.0f });
		vertices.push_back({  h, -pos, 0.0f, 1.0f });
		// 左軸
		vertices.push_back({ -pos, -h, 0.0f, 1.0f });
		vertices.push_back({ -pos,  h, 0.0f, 1.0f });
		// 右軸
		vertices.push_back({ pos, -h, 0.0f, 1.0f });
		vertices.push_back({ pos,  h, 0.0f, 1.0f });

		for (uint32_t i = 0; i < 4; ++i) {
			indices.push_back(baseIdx + i * 2);
			indices.push_back(baseIdx + i * 2 + 1);
		}
	}

	// E. 4隅の精密ターゲティングブラケット（L字＋角ピン）
	const float cPos = 0.38f;
	const float cArm = 0.10f;
	const float cPin = 0.04f;

	// 各クアドラント (右上, 左上, 左下, 右下)
	float sgnX[4] = { 1.0f, -1.0f, -1.0f,  1.0f };
	float sgnY[4] = { 1.0f,  1.0f, -1.0f, -1.0f };

	for (int i = 0; i < 4; ++i) {
		float cx = sgnX[i] * cPos;
		float cy = sgnY[i] * cPos;
		float ax = sgnX[i] * (cPos - cArm);
		float ay = sgnY[i] * (cPos - cArm);

		baseIdx = static_cast<uint32_t>(vertices.size());
		vertices.push_back({ ax, cy, 0.0f, 1.0f }); // 0
		vertices.push_back({ cx, cy, 0.0f, 1.0f }); // 1: 角
		vertices.push_back({ cx, ay, 0.0f, 1.0f }); // 2
		// 角から斜め外向きのピン
		vertices.push_back({ cx + sgnX[i] * cPin, cy + sgnY[i] * cPin, 0.0f, 1.0f }); // 3

		indices.push_back(baseIdx + 0); indices.push_back(baseIdx + 1);
		indices.push_back(baseIdx + 1); indices.push_back(baseIdx + 2);
		indices.push_back(baseIdx + 1); indices.push_back(baseIdx + 3);
	}

	innerReticle_->CreateCustomMesh(vertices, indices);
}

void Reticle::CreateOuterMesh() {
	// アウター主枠（外枠）：二重六角フレーム ＋ コーナーリブ ＋ シェブロン矢印 ＋ 左右ステップゲージ
	std::vector<Vector4> vertices;
	std::vector<uint32_t> indices;

	const float rInner = 1.45f;
	const float rOuter = 1.72f;
	const uint32_t sides = 6;

	// 1. 内側六角形
	uint32_t innerHexStart = static_cast<uint32_t>(vertices.size());
	for (uint32_t i = 0; i < sides; ++i) {
		float angle = i * (2.0f * std::numbers::pi_v<float> / sides) + (std::numbers::pi_v<float> / 6.0f);
		float x = std::cos(angle) * rInner;
		float y = std::sin(angle) * rInner;
		vertices.push_back({ x, y, 0.0f, 1.0f });
	}
	for (uint32_t i = 0; i < sides; ++i) {
		uint32_t next = (i + 1) % sides;
		indices.push_back(innerHexStart + i);
		indices.push_back(innerHexStart + next);
	}

	// 2. 外側六角形
	uint32_t outerHexStart = static_cast<uint32_t>(vertices.size());
	for (uint32_t i = 0; i < sides; ++i) {
		float angle = i * (2.0f * std::numbers::pi_v<float> / sides) + (std::numbers::pi_v<float> / 6.0f);
		float x = std::cos(angle) * rOuter;
		float y = std::sin(angle) * rOuter;
		vertices.push_back({ x, y, 0.0f, 1.0f });
	}
	for (uint32_t i = 0; i < sides; ++i) {
		uint32_t next = (i + 1) % sides;
		indices.push_back(outerHexStart + i);
		indices.push_back(outerHexStart + next);
	}

	// 3. 6箇所のコーナーリブ（内側と外側の頂点を結ぶ）＆ 外向きピン
	const float pinLen = 0.25f;
	for (uint32_t i = 0; i < sides; ++i) {
		float angle = i * (2.0f * std::numbers::pi_v<float> / sides) + (std::numbers::pi_v<float> / 6.0f);
		float px = std::cos(angle) * (rOuter + pinLen);
		float py = std::sin(angle) * (rOuter + pinLen);

		// 内側と外側をつなぐ
		indices.push_back(innerHexStart + i);
		indices.push_back(outerHexStart + i);

		// 外側からさらに外向きピン
		uint32_t pIdx = static_cast<uint32_t>(vertices.size());
		vertices.push_back({ px, py, 0.0f, 1.0f });
		indices.push_back(outerHexStart + i);
		indices.push_back(pIdx);
	}

	// 4. 上下左右の4つのシェブロン（外向き矢印 `▲` `▼` `◀` `▶`）
	const float chevDist = rOuter;
	const float chevW = 0.16f;
	const float chevH = 0.14f;

	// 上シェブロン `▲`
	uint32_t cIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({ -chevW, chevDist, 0.0f, 1.0f });
	vertices.push_back({   0.0f, chevDist + chevH, 0.0f, 1.0f });
	vertices.push_back({  chevW, chevDist, 0.0f, 1.0f });
	indices.push_back(cIdx + 0); indices.push_back(cIdx + 1);
	indices.push_back(cIdx + 1); indices.push_back(cIdx + 2);

	// 下シェブロン `▼`
	cIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({ -chevW, -chevDist, 0.0f, 1.0f });
	vertices.push_back({   0.0f, -chevDist - chevH, 0.0f, 1.0f });
	vertices.push_back({  chevW, -chevDist, 0.0f, 1.0f });
	indices.push_back(cIdx + 0); indices.push_back(cIdx + 1);
	indices.push_back(cIdx + 1); indices.push_back(cIdx + 2);

	// 左シェブロン `◀`
	cIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({ -chevDist, -chevW, 0.0f, 1.0f });
	vertices.push_back({ -chevDist - chevH, 0.0f, 0.0f, 1.0f });
	vertices.push_back({ -chevDist,  chevW, 0.0f, 1.0f });
	indices.push_back(cIdx + 0); indices.push_back(cIdx + 1);
	indices.push_back(cIdx + 1); indices.push_back(cIdx + 2);

	// 右シェブロン `▶`
	cIdx = static_cast<uint32_t>(vertices.size());
	vertices.push_back({ chevDist, -chevW, 0.0f, 1.0f });
	vertices.push_back({ chevDist + chevH, 0.0f, 0.0f, 1.0f });
	vertices.push_back({ chevDist,  chevW, 0.0f, 1.0f });
	indices.push_back(cIdx + 0); indices.push_back(cIdx + 1);
	indices.push_back(cIdx + 1); indices.push_back(cIdx + 2);

	// 5. 左右のHUDバーティカルステップゲージ（三連スリットバー）
	const float gaugeX = rOuter + 0.28f;
	const float gaugeHalfLen = 0.10f;
	const float gaugeYs[3] = { -0.35f, 0.0f, 0.35f };

	for (int g = 0; g < 3; ++g) {
		float gy = gaugeYs[g];
		// 左ゲージ
		uint32_t gIdx = static_cast<uint32_t>(vertices.size());
		vertices.push_back({ -gaugeX - gaugeHalfLen, gy, 0.0f, 1.0f });
		vertices.push_back({ -gaugeX + gaugeHalfLen, gy, 0.0f, 1.0f });
		indices.push_back(gIdx + 0);
		indices.push_back(gIdx + 1);

		// 右ゲージ
		gIdx = static_cast<uint32_t>(vertices.size());
		vertices.push_back({ gaugeX - gaugeHalfLen, gy, 0.0f, 1.0f });
		vertices.push_back({ gaugeX + gaugeHalfLen, gy, 0.0f, 1.0f });
		indices.push_back(gIdx + 0);
		indices.push_back(gIdx + 1);
	}

	outerReticle_->CreateCustomMesh(vertices, indices);
}

void Reticle::CreateRingMesh() {
	// ミドル装飾リング：二重セグメント円弧 ＋ 8方位コンパス目盛り（ジャイロHUDリング）
	std::vector<Vector4> vertices;
	std::vector<uint32_t> indices;

	const float rOuterArc = 1.18f;
	const float rInnerArc = 1.06f;
	const int segmentsPerQuadrant = 6;
	const float degToRad = std::numbers::pi_v<float> / 180.0f;

	// 4つの象限に二重の円弧を生成（15度〜75度、105度〜165度、195度〜255度、285度〜345度）
	float arcStarts[4] = { 15.0f, 105.0f, 195.0f, 285.0f };
	float arcSpan = 60.0f;

	for (int q = 0; q < 4; ++q) {
		float startAngle = arcStarts[q] * degToRad;
		float step = (arcSpan * degToRad) / static_cast<float>(segmentsPerQuadrant);

		// 外側円弧
		uint32_t outStart = static_cast<uint32_t>(vertices.size());
		for (int s = 0; s <= segmentsPerQuadrant; ++s) {
			float angle = startAngle + step * static_cast<float>(s);
			vertices.push_back({ std::cos(angle) * rOuterArc, std::sin(angle) * rOuterArc, 0.0f, 1.0f });
			if (s > 0) {
				indices.push_back(outStart + s - 1);
				indices.push_back(outStart + s);
			}
		}

		// 内側円弧
		uint32_t inStart = static_cast<uint32_t>(vertices.size());
		for (int s = 0; s <= segmentsPerQuadrant; ++s) {
			float angle = startAngle + step * static_cast<float>(s);
			vertices.push_back({ std::cos(angle) * rInnerArc, std::sin(angle) * rInnerArc, 0.0f, 1.0f });
			if (s > 0) {
				indices.push_back(inStart + s - 1);
				indices.push_back(inStart + s);
			}
		}

		// 円弧の両端をつなぐラジアルバー
		indices.push_back(outStart);
		indices.push_back(inStart);
		indices.push_back(outStart + segmentsPerQuadrant);
		indices.push_back(inStart + segmentsPerQuadrant);

		// 中央に外向きアクセントピン
		float midAngle = startAngle + (arcSpan * 0.5f) * degToRad;
		uint32_t midIdx = static_cast<uint32_t>(vertices.size());
		vertices.push_back({ std::cos(midAngle) * rOuterArc, std::sin(midAngle) * rOuterArc, 0.0f, 1.0f });
		vertices.push_back({ std::cos(midAngle) * (rOuterArc + 0.10f), std::sin(midAngle) * (rOuterArc + 0.10f), 0.0f, 1.0f });
		indices.push_back(midIdx + 0);
		indices.push_back(midIdx + 1);
	}

	// 8方向のコンパス・ディグリーマーク（0°, 45°, 90°, 135°, 180°, 225°, 270°, 315°）
	for (int i = 0; i < 8; ++i) {
		float angle = i * (std::numbers::pi_v<float> / 4.0f);
		float x1 = std::cos(angle) * (rInnerArc - 0.08f);
		float y1 = std::sin(angle) * (rInnerArc - 0.08f);
		float x2 = std::cos(angle) * (rOuterArc + 0.06f);
		float y2 = std::sin(angle) * (rOuterArc + 0.06f);

		uint32_t tIdx = static_cast<uint32_t>(vertices.size());
		vertices.push_back({ x1, y1, 0.0f, 1.0f });
		vertices.push_back({ x2, y2, 0.0f, 1.0f });
		indices.push_back(tIdx + 0);
		indices.push_back(tIdx + 1);
	}

	middleRing_->CreateCustomMesh(vertices, indices);
}

void Reticle::CreateInertiaLinkMesh() {
	// 慣性トラッキングリンク：インナーとアウターのズレを繋ぐ動的HUDライン
	std::vector<Vector4> vertices;
	std::vector<uint32_t> indices;

	// 中心から四方へ広がる動的テザーライン
	const float s = 1.0f;
	// X字テザー
	vertices.push_back({ -s, -s, 0.0f, 1.0f }); // 0
	vertices.push_back({  s,  s, 0.0f, 1.0f }); // 1
	vertices.push_back({ -s,  s, 0.0f, 1.0f }); // 2
	vertices.push_back({  s, -s, 0.0f, 1.0f }); // 3
	indices.push_back(0); indices.push_back(1);
	indices.push_back(2); indices.push_back(3);

	// 中心サークル
	const int segs = 12;
	uint32_t cStart = static_cast<uint32_t>(vertices.size());
	for (int i = 0; i < segs; ++i) {
		float angle = i * (2.0f * std::numbers::pi_v<float> / segs);
		vertices.push_back({ std::cos(angle) * 0.4f, std::sin(angle) * 0.4f, 0.0f, 1.0f });
	}
	for (int i = 0; i < segs; ++i) {
		indices.push_back(cStart + i);
		indices.push_back(cStart + (i + 1) % segs);
	}

	inertiaLink_->CreateCustomMesh(vertices, indices);
}

const Matrix4x4& Reticle::GetMatWorld() const {
	static const Matrix4x4 identity = MakeIdentityMatrix();
	if (innerReticle_) {
		return innerReticle_->GetWorldTransform().matWorld;
	}
	return identity;
}

Vector3 Reticle::Get3DPosition() const {
	if (innerReticle_) {
		return innerReticle_->GetWorldTransform().GetWorldPosition();
	}
	return targetWorldPos_;
}

Vector2 Reticle::Get2DPosition() const {
	if (!camera_) {
		return { 0.0f, 0.0f };
	}

	Vector3 positionReticle = Get3DPosition();
	// ビューポート行列
	Matrix4x4 matViewport = MakeViewportMatrix(0.0f, 0.0f, static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 0.0f, 1.0f);
	// ビュー行列、プロジェクション行列、ビューポート行列を合成する
	Matrix4x4 matViewProjectionViewport = camera_->matView * camera_->matProjection * matViewport;
	// ワールド→スクリーン座標変換
	positionReticle = TransformCoord(positionReticle, matViewProjectionViewport);

	return Vector2(positionReticle.x, positionReticle.y);
}

