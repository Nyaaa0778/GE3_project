#include "ParticleEmitter.h"

#include "ParticleManager.h"

//================================================================================
// コンストラクタ / 更新 / 描画
//================================================================================

/// <summary>
/// パーティクルエミッタを生成
/// </summary>
/// <param name="particleManager">ParticleManagerのポインタ</param>
/// <param name="groupName">発生させるパーティクルグループ名</param>
/// <param name="targetTransform">パーティクルの発生位置</param>
/// <param name="frequency">パーティクルの発生間隔（秒）</param>
/// <param name="count">1回の発生で生成するパーティクル数</param>
/// <param name="active">エミッタの有効／無効フラグ</param>
ParticleEmitter::ParticleEmitter(const std::string &groupName,
                                 Transform *targetTransform, float frequency,
                                 uint32_t count, bool active)
    : groupName_(groupName), transform_(targetTransform),
      frequency_(frequency), count_(count), isActive_(active) {}

/// <summary>
/// 更新
/// </summary>
void ParticleEmitter::Update() {
  if (!isActive_ || !transform_) {
    return;
  }

  const float kDeltaTime = 1.0f / 60.0f;
  frequencyTime_ += kDeltaTime;

  // 発生頻度より大きいなら発生
  if (frequencyTime_ >= frequency_) {
    Emit();

    // 余計に過ぎた時間も加味
    frequencyTime_ -= frequency_;
  }
}

/// <summary>
/// パーティクル生成
/// </summary>
void ParticleEmitter::Emit() {
  // 「呼び出すだけの関数」（資料の意図）
  ParticleManager::GetInstance()->Emit(groupName_, transform_->translation, count_);
}