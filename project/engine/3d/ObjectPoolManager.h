#pragma once

template <class T>

class ObjectPoolManager {
public:
	ObjectPoolManager() {}

	// 仮想デストラクタ
	virtual ~ObjectPoolManager() {}

	/// <summary>
	/// オブジェクトプールの生成
	/// </summary>
	/// <param name="capacity">同時に存在させられるオブジェクトの最大数</param>
	void Create(int capacity);

	/// <summary>
	/// オブジェクトプールの破棄
	/// </summary>
	void Destroy();

	/// <summary>
	/// 未使用のオブジェクトを取得する
	/// </summary>
	/// <returns></returns>
	T* Recycle();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 生存数をカウントする
	/// </summary>
	/// <returns></returns>
	int Count();

protected:
	// オブジェクトプール
	T* pool_ = nullptr;

	// 同時に存在させられるオブジェクトの最大数
	int capacity_ = 0;

};


