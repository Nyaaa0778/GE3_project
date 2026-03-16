#pragma once

#include <numbers>

#include <random>

#include <Vector3.h>

class Random {
public:
	// 整数用min ～ max のランダムな整数を返す
	static int RangeInt(int min, int max);

	// 小数用min ～ max のランダムな小数を返す
	static float RangeFloat(float min, float max);

	// ベクトル用min ～ max のランダムな小数を返す
	static Vector3 RangeVector3(float min, float max);
	static Vector3 RangeVector3(const Vector3& min, const Vector3& max);

private:
	// 乱数生成エンジン（メルセンヌ・ツイスター 64bit）
	static std::mt19937_64 randomEngine_;
};