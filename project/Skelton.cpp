#include "Skelton.h"

#include "Camera.h"
#include "WireframeObject.h"
#include <MathUtility.h>

#include <utility>

using namespace MathUtility;

namespace {

Vector3 TransformPoint(const Vector3& v, const Matrix4x4& m) {
	return {
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2]
	};
}

} // namespace

Skelton::Skelton() = default;
Skelton::~Skelton() = default;

void Skelton::Initialize(const Node& rootNode) {
	skeltonData_ = CreateSkelton(rootNode);

	wireframe_ = std::make_unique<WireframeObject>();
	wireframe_->Initialize();
	wireframe_->SetDepthEnable(false);
	wireframe_->SetColor({0.2f, 1.0f, 1.0f, 1.0f});

	// 各ジョイントあたり：親へのボーン1本 + 八面体12本 = 最大13本
	uint32_t maxLines = static_cast<uint32_t>(skeltonData_.joints.size()) * 13 + 16;
	wireframe_->CreateDynamicLineMesh(maxLines);
}

SkeltonData Skelton::CreateSkelton(const Node& rootNode) {
	SkeltonData skeltonData;

	skeltonData.root = CreateJoint(rootNode, {}, skeltonData.joints);

	// 名前と Index のマッピングを行ってアクセスしやすくする
	for (const Joint& joint : skeltonData.joints) {
		skeltonData.jointMap.emplace(joint.name, joint.index);
	}

	// 初期姿勢の行列を計算
	Update(skeltonData);

	return skeltonData;
}

int32_t Skelton::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeltonSpaceMatrix = MakeIdentityMatrix();
	joint.transform = node.transform;
	joint.index = int32_t(joints.size()); // 現在登録されている数を Index に
	joint.parent = parent;

	joints.push_back(joint);

	for (const Node& child : node.children) {
		// 子Joint を作成して、その Index を登録
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	// 自身の Index を返す
	return joint.index;
}

void Skelton::ApplyAnimation(SkeltonData& skeltonData, const Animation& animation, float animationTime) {
	for (Joint& joint : skeltonData.joints) {
		// 対象の　Joint の　Animation があれば、値の適用を行う
		if (auto it = animation.nodeAnimations.find(joint.name); it != animation.nodeAnimations.end())
		{
			const NodeAnimation& rootNodeAnimation = (*it).second;
			joint.transform.translation = CalculateValue(rootNodeAnimation.translate, animationTime);
			joint.transform.rotation = CalculateValue(rootNodeAnimation.rotate, animationTime);
			joint.transform.scale = CalculateValue(rootNodeAnimation.scale, animationTime);
		}
	}
}

void Skelton::Update(SkeltonData& skeltonData) {
	// 全ての Joint を更新、親が若いので通常ループで処理可能
	for (Joint& joint : skeltonData.joints) {
		joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotation, joint.transform.translation);

		// 親がいるのなら
		if (joint.parent) {
			// 親の行列をかける
			joint.skeltonSpaceMatrix = joint.localMatrix * skeltonData.joints[*joint.parent].skeltonSpaceMatrix;
		} else {
			// localMatrix と skeltonSpaceMatrix は一致する
			joint.skeltonSpaceMatrix = joint.localMatrix;
		}
	}
}

void Skelton::ApplyAnimation(const Animation& animation, float animationTime) {
	ApplyAnimation(skeltonData_, animation, animationTime);
}

void Skelton::Update() {
	Update(skeltonData_);
}

void Skelton::Draw(Camera* camera, const Matrix4x4& worldMatrix) {
	if (!wireframe_ || skeltonData_.joints.empty()) {
		return;
	}

	std::vector<std::pair<Vector3, Vector3>> lines;
	lines.reserve(skeltonData_.joints.size() * 13);

	constexpr float kJointRadius = 0.08f;
	static const Vector3 kOctaVerts[6] = {
		{ kJointRadius,  0.0f,          0.0f},
		{-kJointRadius,  0.0f,          0.0f},
		{ 0.0f,          kJointRadius,  0.0f},
		{ 0.0f,         -kJointRadius,  0.0f},
		{ 0.0f,          0.0f,          kJointRadius},
		{ 0.0f,          0.0f,         -kJointRadius},
	};
	static const int kOctaEdges[12][2] = {
		{0, 2}, {2, 1}, {1, 3}, {3, 0},
		{0, 4}, {4, 1}, {1, 5}, {5, 0},
		{2, 4}, {4, 3}, {3, 5}, {5, 2},
	};

	for (const Joint& joint : skeltonData_.joints) {
		Matrix4x4 jointWorld = joint.skeltonSpaceMatrix * worldMatrix;
		Vector3 jointPos = {jointWorld.m[3][0], jointWorld.m[3][1], jointWorld.m[3][2]};

		// 親ジョイントからこのジョイントへのボーン線分
		if (joint.parent) {
			Matrix4x4 parentWorld = skeltonData_.joints[*joint.parent].skeltonSpaceMatrix * worldMatrix;
			Vector3 parentPos = {parentWorld.m[3][0], parentWorld.m[3][1], parentWorld.m[3][2]};
			lines.emplace_back(parentPos, jointPos);
		}

		// ジョイント位置・回転を可視化する八面体ワイヤーフレーム
		Vector3 transformedVerts[6];
		for (int i = 0; i < 6; ++i) {
			transformedVerts[i] = TransformPoint(kOctaVerts[i], jointWorld);
		}
		for (const auto& edge : kOctaEdges) {
			lines.emplace_back(transformedVerts[edge[0]], transformedVerts[edge[1]]);
		}
	}

	wireframe_->SetCamera(camera);
	wireframe_->UpdateDynamicLines(lines);
	wireframe_->Update();
	wireframe_->Draw();
}