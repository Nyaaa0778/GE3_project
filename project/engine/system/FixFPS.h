#pragma once
#include <chrono>

class FixFPS {
public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize();
  /// <summary>
  /// 更新
  /// </summary>
  void Update();

private:
  // 記録時間(FPS固定用)
  std::chrono::steady_clock::time_point frameStartTime_;
};
