#pragma once

#include <string>
#include <map>
#include <vector>

#include <Vector3.h>
#include <Quaternion.h>

template <typename tValue>
struct Keyframe {
	float time;   // キーフレームの時刻（秒単位）
	tValue value; // キーフレームの値
};

using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

template <typename tValue>
struct AnimationCurve {
	std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
	AnimationCurve<Vector3> translate;
	AnimationCurve<Quaternion> rotate;
	AnimationCurve<Vector3> scalel;
};

struct Animation {
	float duration; // アニメーション全体の尺（秒単位）

	// NodeAnimation の集合、Node 名でひけるようにしておく
	std::map<std::string, NodeAnimation> nodeAnimations;
};

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);
