#pragma once

#include <memory>
#include <string>

class IScene;
class ISceneFactory;

class SceneManager {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static SceneManager* GetInstance();

	static void Finalize();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	SceneManager();
	/// <summary>
	/// デストラクタ
	/// </summary>
	~SceneManager();

private:
	static std::unique_ptr<SceneManager> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	SceneManager(SceneManager&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	SceneManager& operator=(SceneManager&) = delete;

public:
	void Update();

	void Draw();

public:
	//================================================================================
	// Setter
	//================================================================================

	// シーンファクトリー
	void SetSceneFactory(ISceneFactory* sceneFactory) {
		sceneFactory_ = sceneFactory;
	}

	// 次のシーンを予約
	void ChangeScene(const std::string& sceneName);

private:
	// 今のシーン (unique_ptr で管理)
	std::unique_ptr<IScene> scene_ = nullptr;
	// 次のシーン (unique_ptr で管理)
	std::unique_ptr<IScene> nextScene_ = nullptr;

	// シーンファクトリー
	ISceneFactory* sceneFactory_ = nullptr;

private:
	void ChangeSceneInternal();
};
