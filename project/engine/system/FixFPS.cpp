#include "FixFPS.h"
#include <thread>

/// <summary>
/// 初期化
/// </summary>
void FixFPS::Initialize() {
  // 現在の時刻を記録する
  frameStartTime_ = std::chrono::steady_clock::now();
}
/// <summary>
/// 更新
/// </summary>
void FixFPS::Update() {
  // 1/60秒ピッタリの時間
  const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
  // 1/60秒よりわずかに短い時間
  const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

  // 現在時刻を取得する
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  // 前回記録からの経過時間を取得する
  std::chrono::microseconds elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(now -
                                                            frameStartTime_);
  // 1/60秒（よりわずかに短い時間）経っていない場合
  if (elapsed < kMinCheckTime) {
    // 1/60秒経過するまで微小なスリープを繰り返す
    while (std::chrono::steady_clock::now() - frameStartTime_ < kMinTime) {
      // 1マイクロ秒スリープ
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }
  // 現在の時間を記録する
  frameStartTime_ = std::chrono::steady_clock::now();
}
