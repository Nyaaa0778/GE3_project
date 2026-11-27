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
	if (models_.contains(modelName))
	{
		return;
	}

	std::string directoryPath;
	std::string fileName;

	size_t slashPos = modelName.find_last_of('/');
	if (slashPos != std::string::npos)
	{
		// 例: "human/walk" -> directoryPath = "resources/models/human", fileName = "walk.gltf"
		directoryPath = "resources/models/" + modelName.substr(0, slashPos);
		fileName = modelName.substr(slashPos + 1) + "." + extension;
	} else
	{
		// 従来通り: "simpleSkin" -> "resources/models/simpleSkin", "simpleSkin.gltf"
		directoryPath = "resources/models/" + modelName;
		fileName = modelName + "." + extension;
	}

	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(directoryPath, fileName);

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
