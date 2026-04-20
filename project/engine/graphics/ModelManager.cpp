#include "ModelManager.h"

#include "Model.h"

std::unique_ptr<ModelManager> ModelManager::instance = nullptr;

//================================================================================
// シングルトン
//================================================================================

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>TextureManager の唯一のインスタンス</returns>
ModelManager* ModelManager::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<ModelManager>();
	}

	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void ModelManager::Finalize() { instance.reset(); }

//================================================================================
// Modelの読み込み
//================================================================================

/// <summary>
/// モデルファイルの読み込み
/// </summary>
/// <param name="modelName">モデル名</param>
/// <param name="extension">拡張子（デフォルトは "obj"）</param>
void ModelManager::LoadModel(const std::string& modelName, const std::string& extension) {
	// 読み込み済みのモデルを検索
	if (models_.contains(modelName)) {
		// 読み込み済みなら早期に return
		return;
	}

	std::string directoryPath = "resources/models/" + modelName;

	// 拡張子を指定
	std::string fileName = modelName + "." + extension;

	// モデルの生成とファイル読み込み、初期化
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(directoryPath, fileName);

	// モデルを mapコンテナ に格納する
	models_.insert(std::make_pair(modelName, std::move(model)));
}

//================================================================================
// Getter
//================================================================================

/// <summary>
/// モデルの検索
/// </summary>
/// <param name="modelName">モデル名</param>
/// <returns>モデル</returns>
Model* ModelManager::FindModel(const std::string& modelName) {
	// 読み込み済みモデルを検索
	if (models_.contains(modelName)) {
		// 読み込みモデルを戻り値として return
		return models_.at(modelName).get();
	}

	// ファイル名一致なし
	return nullptr;
}
