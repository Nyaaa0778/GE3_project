#include "KeyframeAnimation.h"

#include <assimp/anim.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <cassert>

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename) {

	// 作るアニメーション
	Animation animation;

	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene->mNumAnimations != 0); // アニメーションがない
	aiAnimation* animationAssimp = scene->mAnimations[0]; // とりあえず最初だけ（あとで複数化）
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond); // 時間の単位を秒に変換

	// Node ごとの Animation を回して NodeAnimation の情報を取ってくる
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex)
	{
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex)
		{
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;

			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 時間の単位を秒に変換
			keyframe.value = {-keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z}; // 右手から左手
		}

	}
}
