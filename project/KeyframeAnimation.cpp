#include "KeyframeAnimation.h"
#include "MathUtility.h"

#include <assimp/anim.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <cassert>

namespace {

template <typename T, typename InterpolateFn>
T CalculateCurveValue(const std::vector<Keyframe<T>>& keyframes, float time, InterpolateFn interpolate) {
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			float duration = keyframes[nextIndex].time - keyframes[index].time;
			float t = (duration > 0.0f) ? (time - keyframes[index].time) / duration : 0.0f;
			return interpolate(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	return keyframes.back().value;
}

} // namespace

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename) {

	// 作るアニメーション
	Animation animation;

	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene && scene->mNumAnimations != 0); // アニメーションがない
	aiAnimation* animationAssimp = scene->mAnimations[0]; // とりあえず最初だけ（あとで複数化）
	double ticksPerSecond = animationAssimp->mTicksPerSecond != 0.0 ? animationAssimp->mTicksPerSecond : 1.0;
	animation.duration = float(animationAssimp->mDuration / ticksPerSecond); // 時間の単位を秒に変換

	// Node ごとの Animation を回して NodeAnimation の情報を取ってくる
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex)
	{
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

		// ---------------------------------------------------------
		// Translate (位置)
		// ---------------------------------------------------------
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex)
		{
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;

			keyframe.time = float(keyAssimp.mTime / ticksPerSecond); // 時間の単位を秒に変換
			keyframe.value = {-keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z}; // 右手から左手

			nodeAnimation.translate.keyframes.push_back(keyframe); // ※保存処理
		}

		// ---------------------------------------------------------
		// Rotate (回転)
		// ---------------------------------------------------------
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex)
		{
			aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;

			keyframe.time = float(keyAssimp.mTime / ticksPerSecond);
			keyframe.value = {keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w};

			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}

		// ---------------------------------------------------------
		// Scale (拡縮)
		// ---------------------------------------------------------
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex)
		{
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe;

			keyframe.time = float(keyAssimp.mTime / ticksPerSecond);
			keyframe.value = {keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z};

			nodeAnimation.scale.keyframes.push_back(keyframe);
		}
	}

	return animation;
}

template <>
Vector3 CalculateValue(const AnimationCurve<Vector3>& curve, float time) {

	assert(!curve.keyframes.empty());

	if (curve.keyframes.size() == 1 || time <= curve.keyframes[0].time)
	{
		return curve.keyframes[0].value;
	}

	for (size_t index = 0; index < curve.keyframes.size() - 1; ++index)
	{
		size_t nextIndex = index + 1;

		// index と nextIndex の2つの keydframe を取得して範囲内に時刻があるかを判定
		if (curve.keyframes[index].time <= time && curve.keyframes[nextIndex].time)
		{
			// 範囲内を補間する
			float t = (time - curve.keyframes[index].time) / (curve.keyframes[nextIndex].time - curve.keyframes[index].time);

			return MathUtility::Lerp(curve.keyframes[index].value, curve.keyframes[nextIndex].value, t);
		}
	}

	return (*curve.keyframes.rbegin()).value;
}

template <>
Quaternion Ca