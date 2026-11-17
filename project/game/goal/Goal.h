#pragma once

#include "Collider.h"
#include "WorldTransform.h"
#include "Vector4.h"
#include <memory>
#include <vector>
#include <string>

class Camera;
class WireframeObject;

/// <summary>
/// ゴール地点演出オブジェクト（ホログラム・セキュリティゲート）
/// ・ゴール接近時に進行コースを塞ぐ巨大な光の壁と「GOAL」ホログラムが出現
/// ・自機激突時のヒットストップ中にガラス亀裂が拡散
/// ・突破時にゲート全体が無数のサイバー発光キューブ（ボクセル）となって前方へ弾け飛ぶ
/// </summary>
class Goal : public Collider {
public:
	Goal();
	~Goal() override;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(const Vector3& pos, Camera* camera);

	/// <summary>
	/// 毎フレーム更新
	/// </summary>
	/// <param name="timeScale">ヒットストップ・スローモーション用の時間倍率（通常1.0f）</param>
	void Update(float timeScale = 1.0f);

	/// <summary>
	/// 3D描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 状態リセット（リプレイ・初期化用）
	/// </summary>
	void Reset();

	/// <summary>
	/// ゴール接近時のホログラムゲート展開を開始
	/// </summary>
	void TriggerAppear();

	/// <summary>
	/// ホログラムゲートを即座に完全展開状態にする
	/// </summary>
	void ForceAppearImmediate();

	bool IsAppearingOrAppeared() const { return isAppearing_ || appearProgress_ >= 1.0f; }
	float GetAppearProgress() const { return appearProgress_; }

	/// <summary>
	/// 自機がゲートに激突した瞬間のヒットストップ演出（ガラス亀裂・過負荷発光）開始
	/// </summary>
	void StartImpactHitStop(const Vector3& impactWorldPos);

	/// <summary>
	/// ゲート突破・ガラス破砕＆サイバーボクセル飛散演出の開始
	/// </summary>
	void Shatter(const Vector3& impactWorldPos, const Vector3& forwardDir = { 0.0f, 0.0f, 1.0f });

	bool IsInHitStop() const { return isInHitStop_; }
	bool IsShattered() const { return isShattered_; }

	/// <summary>
	/// 位置の再設定
	/// </summary>
	void SetPosition(const Vector3& pos);

	/// <summary>
	/// フォグ（背景都市同様の距離減衰フェード）の設定
	/// </summary>
	void SetFog(float fogNear, float fogFar);
	float GetFogNear() const { return fogNear_; }
	float GetFogFar() const { return fogFar_; }

	/// <summary>
	/// デバッグUI
	/// </summary>
	void DrawImGui(const char* windowName);

	// コライダーの仮想関数
	void OnCollision() override;
	Vector3 GetWorldPosition() override;

	const WorldTransform& GetWorldTransform() const { return worldTransform_; }
	WorldTransform& GetWorldTransform() { return worldTransform_; }

private:
	// 各構成メッシュの構築
	void BuildSecurityBarrierMesh();
	void BuildCenterGoalHologramMesh();
	void BuildImpactCrackMesh(const Vector3& impactWorldPos);
	void InitializeVoxelInstancing();

	struct LetterData {
		std::vector<Vector3> localVertices;
		std::vector<std::pair<int, int>> baseEdges;
		float width = 1.0f;
	};
	void GenerateCharMesh(char c, float w, float h, float t, LetterData& l);

	// 破砕時に飛び散る3Dサイバー発光ボクセルデータ
	struct VoxelPiece {
		Vector3 position = {};
		Vector3 velocity = {};
		Vector3 rotation = {};
		Vector3 angularVelocity = {};
		Vector3 baseScale = { 0.28f, 0.28f, 0.28f };
		Vector4 color = { 0.0f, 0.95f, 1.0f, 1.0f };
		float lifeTime = 2.0f;
		float elapsed = 0.0f;
	};

private:
	static constexpr uint32_t kMaxVoxels = 420;
	static constexpr float kAppearDuration = 0.65f; // ホログラムゲート展開時間（秒）

	WorldTransform worldTransform_;
	Camera* camera_ = nullptr;
	Vector3 basePosition_ = {};

	// 1. コースを塞ぐ巨大な光の壁（セキュリティゲート・ホログラムバリア）
	std::unique_ptr<WireframeObject> barrierWallObject_;
	std::unique_ptr<WireframeObject> barrierCoreObject_;
	std::unique_ptr<WireframeObject> scanlineBeamObject_;

	// 2. コース中央を塞ぐ巨大ホログラム「GOAL」文字
	std::vector<std::unique_ptr<WireframeObject>> centerGoalLetterObjects_;
	std::vector<float> centerLetterXOffsets_;
	std::vector<Vector3> centerLetterSamplePoints_; // ボクセル生成用の文字形状サンプル点

	// 3. ヒットストップ時のガラス亀裂（インパクトクラック）メッシュ
	std::unique_ptr<WireframeObject> impactCrackObject_;

	// 4. ゲート破砕時の3Dサイバー発光ボクセル（外枠キューブ ＋ 内部コアキューブ）
	std::unique_ptr<WireframeObject> voxelOuterObject_;
	std::unique_ptr<WireframeObject> voxelInnerObject_;
	std::vector<VoxelPiece> voxels_;

	// 演出状態フラグ・タイマー
	bool isAppearing_ = false;
	float appearProgress_ = 0.0f; // 0.0f: 未出現 〜 1.0f: 完全展開

	bool isInHitStop_ = false;
	float hitStopTimer_ = 0.0f;
	Vector3 impactPos_ = {};

	bool isShattered_ = false;
	float shatterTimer_ = 0.0f;

	// アニメーション用タイマー
	float timer_ = 0.0f;

	// フォグ（距離減衰）パラメータ（背景都市と共通化）
	float fogNear_ = 80.0f;
	float fogFar_ = 300.0f;
};
