#include "ModelManager.h"
#include "Model.h"
#include "ModelCommon.h"

ModelManager *ModelManager::instance = nullptr;

//================================================================================
// シングルトン管理 / 初期化・終了
//================================================================================

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>TextureManager の唯一のインスタンス</returns>
ModelManager *ModelManager::GetInstance() {
  if (instance == nullptr) {
    instance = new ModelManager;
  }

  return instance;
}

/// <summary>
/// 初期化
/// </summary>
/// <param name="dxCommon">DirectXCommonのポインタ</param>
void ModelManager::Initialize(DirectXCommon *dxCommon) {
  // ModelCommonの生成
  modelCommon_ = new ModelCommon();
  // ModelCommonの初期化
  modelCommon_->Initialize(dxCommon);
}

/// <summary>
/// 終了
/// </summary>
void ModelManager::Finalize() {
  delete instance;
  instance = nullptr;
}

//================================================================================
// Modelの読み込み
//================================================================================

/// <summary>
/// モデルファイルの読み込み
/// </summary>
/// <param name="filePath">モデルファイルのパス</param>
void ModelManager::LoadModel(const std::string &filePath) {
  // 読み込み済みのモデルを検索
  if (models_.contains(filePath)) {
    // 読み込み済みなら早期return
    return;
  }

  // モデルの生成とファイル読み込み、初期化
  std::unique_ptr<Model> model = std::make_unique<Model>();
  model->Initialize(modelCommon_, "resources", filePath);

  // モデルをmapコンテナに格納する
  models_.insert(std::make_pair(filePath, std::move(model)));
}

//================================================================================
// Getter
//================================================================================

/// <summary>
/// モデルの検索
/// </summary>
/// <param name="filePath">モデルファイルのパス</param>
/// <returns>モデル</returns>
Model *ModelManager::FindModel(const std::string &filePath) {
  // 読み込み済みモデルを検索
  if (models_.contains(filePath)) {
    // 読み込みモデルを戻り値としてreturn
    return models_.at(filePath).get();
  }

  // ファイル名一致なし
  return nullptr;
}
