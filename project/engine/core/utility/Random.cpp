#include "Random.h"

// 静的メンバ変数の実体を定義
std::mt19937_64 Random::randomEngine_(std::random_device{}());

// 整数用の実装
int Random::RangeInt(int min, int max) {
  // 整数用の一様分布
  std::uniform_int_distribution<int> distribution(min, max);
  // エンジンを使って乱数を生成して返す
  return distribution(randomEngine_);
}

// 小数用の実装
float Random::RangeFloat(float min, float max) {
  // 小数用の一様分布
  std::uniform_real_distribution<float> distribution(min, max);
  // エンジンを使って乱数を生成して返す
  return distribution(randomEngine_);
}

// ベクトル用の実装
Vector3 Random::RangeVector3(float min, float max) {
  return Vector3(RangeFloat(min, max), RangeFloat(min, max),
                 RangeFloat(min, max));
}
Vector3 Random::RangeVector3(const Vector3 &min, const Vector3 &max) {
  return Vector3(RangeFloat(min.x, max.x), RangeFloat(min.y, max.y),
                 RangeFloat(min.z, max.z));
}