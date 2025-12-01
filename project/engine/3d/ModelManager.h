#pragma once

#include <map>
#include <memory>
#include <string>

class Model;
class ModelCommon;
class DirectXCommon;

class ModelManager {
public:
  //================================================================================
  // シングルトン管理 / 初期化・終了
  //================================================================================

  /// <summary>
  /// シングルトンインスタンスの取得
  /// </summary>
  /// <returns>ModelManagerの唯一のインスタンス</returns>
  static ModelManager *GetInstance();

  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="dxCommon">DirectXCommonのポインタ</param>
  void Initialize(DirectXCommon *dxCommon);

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

  //================================================================================
  // Modelの読み込み
  //================================================================================

  /// <summary>
  /// モデルファイルの読み込み
  /// </summary>
  /// <param name="ModelName">モデル名</param>
  void LoadModel(const std::string &modelName);

public:
  //================================================================================
  // Getter
  //================================================================================

  /// <summary>
  /// モデルの検索
  /// </summary>
  /// <param name="ModelName">モデル名</param>
  /// <returns>モデル</returns>
  Model *FindModel(const std::string &modelName);

private:
  //================================================================================
  // モデルデータコンテナ
  //================================================================================

  // モデルデータの配列
  std::map<std::string, std::unique_ptr<Model>> models_;

private:
  //================================================================================
  // 外部参照
  //================================================================================

  // ModelCommon
  ModelCommon *modelCommon_ = nullptr;

private:
  //================================================================================
  // シングルトン実装詳細
  //================================================================================

  static ModelManager *instance;

  /// <summary>
  /// コンストラクタ
  /// </summary>
  ModelManager() = default;
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~ModelManager() = default;

  /// <summary>
  /// コピーコンストラクタ禁止
  /// </summary>
  /// <param name="">コピー元（使用不可）</param>
  ModelManager(ModelManager &) = delete;
  /// <summary>
  /// 代入演算子禁止
  /// </summary>
  /// <param name="">代入元（使用不可）</param>
  /// <returns>このオブジェクトを返す</returns>
  ModelManager &operator=(ModelManager &) = delete;
};
