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

	// 1/60秒ピッタリの時間 (16.667ms)
	const std::chrono::microseconds kTargetTime(1000000 / 60);

	// 残り時間をスリープとビジーウェイトで待つ
	while (true) {
		std::chrono::steady_clock::time_point tempNow = std::chrono::steady_clock::now();
		std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(tempNow - frameStartTime_);
		std::chrono::microseconds remaining = kTargetTime - elapsed;

		if (remaining.count() <= 0) {
			break;
		}

		// 残り時間が1.5ms以上あれば、1msスリープしてCPUを休ませる
		if (remaining.count() > 1500) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		} else {
			// 残りわずかな時間はビジーウェイト（スピンロック）で正確に待つ
#if defined(_MSC_VER)
			_mm_pause(); // CPUに少しだけ休止のヒントを与え、発熱や電力を抑える
#endif
		}
	}

	std::chrono::steady_clock::time_point endOfFrame = std::chrono::steady_clock::now();
	std::chrono::duration<float> delta = endOfFrame - frameStartTime_;
	deltaTime_ = delta.count();

	// 現在の時間を記録する
	frameStartTime_ = endOfFrame;
}

