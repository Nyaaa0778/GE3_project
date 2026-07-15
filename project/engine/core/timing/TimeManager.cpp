#include "TimeManager.h"

#include <thread>

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<TimeManager> TimeManager::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>TimeManagerの唯一のインスタンス</returns>
TimeManager* TimeManager::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<TimeManager>();
	}

	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void TimeManager::Finalize() { instance.reset(); }

//================================================================================
// 初期化 / 更新
//================================================================================

/// <summary>
/// 初期化
/// </summary>
void TimeManager::Initialize() {
	// 現在の時刻を記録する
	frameStartTime_ = std::chrono::steady_clock::now();

	// 最初の時間を記録
	lastUpdateTime_ = frameStartTime_;
}
/// <summary>
/// 更新
/// </summary>
void TimeManager::Update() {
	// 実際の経過時間(deltaTime)の計算
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::chrono::microseconds realElapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdateTime_);

	// マイクロ秒(100万分の1秒)を秒(float)に変換して保持
	deltaTime_ = static_cast<float>(realElapsed.count()) / 1000000.0f;
	lastUpdateTime_ = now;

	// 1/60秒ピッタリの時間
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));

	// 前回記録からの経過時間を取得する
	std::chrono::microseconds elapsed =
		std::chrono::duration_cast<std::chrono::microseconds>(now - frameStartTime_);

	// 1/60秒経っていない場合
	if (elapsed < kMinTime) {
		// 1/60秒経過するまで、スリープ（sleep_for）を使わずにループで待つ
		while (std::chrono::steady_clock::now() - frameStartTime_ < kMinTime) {
			// OSに深く眠らされるのを防ぐため、一瞬だけ他のスレッドに処理を譲る
			std::this_thread::yield();
		}
	}

	std::chrono::steady_clock::time_point endOfFrame =
		std::chrono::steady_clock::now();
	// 今回のフレームで実際にかかった時間を秒(float)で算出
	std::chrono::duration<float> delta = endOfFrame - frameStartTime_;
	deltaTime_ = delta.count();

	// 現在の時間を記録する
	frameStartTime_ = endOfFrame;
}