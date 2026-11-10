#pragma once

#include "Collider.h"
#include "WorldTransform.h"
#include <memory>
#include <vector>
#include <string>

class Camera;
class WireframeObject;

/// <summary>
/// ゴール地点演出オブジェクト（3Dサイバーワイヤーフレームゲート）
/// 連続する多重ワープトンネルリング・外枠トラスゲート・対向回転アパーチャ・頭上立体「GOAL」ネオンサイン
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
	/// 毎フレーム更新（アパーチャ回転・ネオンパルス・サイン浮遊）
	/// </summary>
	void Update();

	/// <summary>
	/// 3D描画
	/// </summary>
	void Draw();

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
	void BuildTunnelMesh();
	void BuildMainGateMesh();
	void BuildApertureMesh();
	void BuildGoalSignMesh();
	void BuildChevronMesh();

	struct LetterData {
		std::vector<Vector3> localVertices;
		std::vector<std::pair<int, int>> baseEdges;
		float width = 1.0f;
	};
	void GenerateCharMesh(char c, float w, float h, float t, LetterData& l);

private:
	WorldTransform worldTransform_;
	Camera* camera_ = nullptr;
	Vector3 basePosition_ = {};

	// 1. ワープトンネル（多重八角形リング ＋ 縦梁レール ＋ トラス補強）
	std::unique_ptr<WireframeObject> tunnelObject_;

	// 2. メインゲート外枠（左右パイロントラス ＋ 上部ガントリーブリッジ）
	std::unique_ptr<WireframeObject> gateFrameObject_;

	// 3. 対向回転アパーチャリング（外周時計回り、内周反時計回り）
	std::unique_ptr<WireframeObject> outerApertureObject_;
	std::unique_ptr<WireframeObject> innerApertureObject_;

	// 4. 頭上立体「GOAL」ネオンブロックサイン
	std::vector<std::unique_ptr<WireframeObject>> goalLetterObjects_;
	std::vector<float> letterXOffsets_;

	// 5. 進入誘導シェブロン（>>> 矢印）
	std::unique_ptr<WireframeObject> chevronObject_;

	// アニメーション用タイマー・回転角
	float timer_ = 0.0f;
	float rotAngleOuter_ = 0.0f;
	float rotAngleInner_ = 0.0f;

	// フォグ（距離減衰）パラメータ（背景都市と共通化）
	float fogNear_ = 80.0f;
	float fogFar_ = 300.0f;
};
