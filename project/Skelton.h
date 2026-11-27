
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <memory>

#include <Transform.h>
#include <Matrix4x4.h>
#include <Node.h>
#include "KeyframeAnimation.h"

class Camera;
class WireframeObject;

struct Joint {
	QuaternionTransform transform; // Transform 情報
	Matrix4x4 localMatrix;
	Matrix4x4 skeltonSpaceMatrix;  // skeltonSpace での変換行列
	std::string name;              // 名前
	std::vector<int32_t> children; // 子Joint の Indexリスト、いなければ空
	int32_t index;                 // 自身の Index
	std::optional<int32_t> parent; // 親Joint の Index、なければ null
};

struct SkeltonData {
	int32_t root = 0;                        // RootJoint の Index
	std::map<std::string, int32_t> jointMap; // Joint名と Index との辞書
	std::vector<Joint> joints;               // 所属しているジョイント
};

class Skelton {
public:
	Skelton();
	~Skelton();

	/// <summary>
	/// ルートノードからスケルトンを初期化し、デバッグ描画用メッシュも構築する
	/// </summary>
	void Initialize(const Node& rootNode);

	/// <summary>
	/// スケルトンデータを生成する
	/// </summary>
	static SkeltonData CreateSkelton(const Node& rootNode);

	/// <summary>
	/// ノード階層からジョイントを再帰的に生成する
	/// </summary>
	static int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

	/// <summary>
	/// アニメーションをスケルトンデータに適用する
	/// </summary>
	static void ApplyAnimation(SkeltonData& skeltonData, const Animation& animation, float animationTime);

	/// <summary>
	/// 全ジョイントの localMatrix / skeltonSpaceMatrix を更新する
	/// </summary>
	static void Update(SkeltonData& skeltonData);

	/// <summary>
	/// 自身が保持するスケルトンデータにアニメーションを適用する
	/// </summary>
	void ApplyAnimation(const Animation& animation, float animationTime);

	/// <summary>
	/// 自身が保持するスケルトンデータを更新する
	/// </summary>
	void Update();

	/// <summary>
	/// ボーンとジョイントをワイヤーフレームでデバッグ描画する
	/// </summary>
	void Draw(Camera* camera, const Matrix4x4& worldMatrix);

	SkeltonData& GetSkeltonData() { return skeltonData_; }
	const SkeltonData& GetSkeltonData() const { return skeltonData_; }

private:
	SkeltonData skeltonData_;
	std::unique_ptr<WireframeObject> wireframe_;
};

