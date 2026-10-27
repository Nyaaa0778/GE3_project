#pragma once

#include <memory>
#include <vector>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>
#include <WorldTransform.h>

class Camera;
class WireframeObject;

class Reticle {
public:
	enum class ColorTheme {
		kCyberAmberWhite,    // エレクトリック・アンバー＆ソーラーホワイト（シアン/マゼンタ背景に対して最高峰の視認性）
		kElectricLime,       // アシッド・ライムグリーン＆ネオンゴールド（蛍光ライム・ハイコントラスト）
		kNeonGoldMagenta,    // ネオンゴールド＆サイバーマゼンタ（華麗なネオンツートン）
		kSolarCrimson,       // プラズマ・クリムゾン＆ソーラーホワイト（戦闘機HUDテイスト）
	};

public:
	Reticle();
	~Reticle();

	// 照準の初期化
	void Initialize(Camera* camera);

	// 照準の更新（プレイヤーのワールドトランスフォームを受け取る）
	void Update(const WorldTransform& playerWorldTransform);

	// 照準の描画
	void Draw();

	// カラーテーマ切り替え
	void SetColorTheme(ColorTheme theme);
	ColorTheme GetColorTheme() const { return colorTheme_; }

public:
	// ゲッター群
	const Matrix4x4& GetMatWorld() const;
	Vector3 Get3DPosition() const;
	Vector2 Get2DPosition() const;

private:
	// 各プロシージャルメッシュの生成
	void CreateInnerMesh();
	void CreateOuterMesh();
	void CreateRingMesh();
	void CreateInertiaLinkMesh();

	// カラーの適用
	void UpdateColors();

private:
	Camera* camera_ = nullptr;

	// インナー（中心：静的・高密度精密ドット/十字/データスケール）
	std::unique_ptr<WireframeObject> innerReticle_;

	// アウター（外枠：二重六角フレーム/コーナーブラケット/垂直ゲージ）
	std::unique_ptr<WireframeObject> outerReticle_;

	// ミドル（装飾リング：逆回転ジャイロ/8方位コンパスリング）
	std::unique_ptr<WireframeObject> middleRing_;

	// 慣性トラッキングリンク（インナーとアウターのズレを繋ぐダイナミックHUDライン）
	std::unique_ptr<WireframeObject> inertiaLink_;

	// 慣性ラグ制御用座標
	Vector3 targetWorldPos_ = {0.0f, 0.0f, 0.0f};
	Vector3 laggedWorldPos_ = {0.0f, 0.0f, 0.0f};
	bool isFirstUpdate_ = true;

	// アニメーション用
	float outerSpinAngle_ = 0.0f;
	float ringSpinAngle_ = 0.0f;
	float breatheTimer_ = 0.0f;

	// カラー設定
	ColorTheme colorTheme_ = ColorTheme::kCyberAmberWhite;
};

