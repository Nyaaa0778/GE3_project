#pragma once

#include <chrono>
#include <memory>

class TimeManager {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static TimeManager* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	TimeManager() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~TimeManager() = default;

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<TimeManager>;

private:
	static std::unique_ptr<TimeManager> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	TimeManager(TimeManager&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	TimeManager& operator=(TimeManager&) = delete;

public:
	//================================================================================
	// 初期化 / 更新
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();
	/// <summary>
	/// 更新
	/// </summary>
	void Update();

public:
	//================================================================================
	// Getter
	//================================================================================

	// 経過時間
	float GetDeltaTime() const { return deltaTime_; }

private:
	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point frameStartTime_;

	// 経過時間
	float deltaTime_ = 0.0f;
};
