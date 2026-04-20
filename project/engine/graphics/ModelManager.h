#pragma once

#include <map>
#include <memory>
#include <string>

class Model;

class ModelManager {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static ModelManager* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	ModelManager() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~ModelManager() = default;


	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<ModelManager>;

private:
	static std::unique_ptr<ModelManager> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	ModelManager(ModelManager&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	ModelManager& operator=(ModelManager&) = delete;

public:
	//================================================================================
	// Modelの読み込み
	//================================================================================

	/// モデルファイルの読み込み
	/// </summary>
	/// <param name="modelName">モデル名</param>
	/// <param name="extension">拡張子（デフォルトは "obj"）</param>
	void LoadModel(const std::string& modelName, const std::string& extension = "obj");

public:
	//================================================================================
	// Getter
	//================================================================================

	/// <summary>
	/// モデルの検索
	/// </summary>
	/// <param name="ModelName">モデル名</param>
	/// <returns>モデル</returns>
	Model* FindModel(const std::string& modelName);

private:
	//================================================================================
	// モデルデータコンテナ
	//================================================================================

	// モデルデータの配列
	std::map<std::string, std::unique_ptr<Model>> models_;
};
