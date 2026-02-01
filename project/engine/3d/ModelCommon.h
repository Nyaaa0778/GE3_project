#pragma once

#include <memory>

class DirectXCommon;

class ModelCommon {
public:
  //================================================================================
  // シングルトン
  //================================================================================

  // 唯一のインスタンス取得
  static ModelCommon *GetInstance();

  /// <summary>
  /// 終了
  /// </summary>
  static void Finalize();

  // unique_ptrからの削除を許可
  friend std::default_delete<ModelCommon>;

private:
  static std::unique_ptr<ModelCommon> instance;

  /// <summary>
  /// コンストラクタ
  /// </summary>
  ModelCommon() = default;
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~ModelCommon() = default;

  /// <summary>
  /// コピーコンストラクタ禁止
  /// </summary>
  /// <param name="">コピー元（使用不可）</param>
  ModelCommon(ModelCommon &) = delete;
  /// <summary>
  /// 代入演算子禁止
  /// </summary>
  /// <param name="">代入元（使用不可）</param>
  /// <returns>このオブジェクトを返す</returns>
  ModelCommon &operator=(ModelCommon &) = delete;

public:
  //================================================================================
  // 初期化
  //================================================================================

  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="dxCommon">DirectXCommonのポインタ</param>
  void Initialize(DirectXCommon *dxCommon);

public:
  //================================================================================
  // Getter
  //================================================================================

  // DirectXCommon
  DirectXCommon *GetDxCommon() const { return dxCommon_; }

private:
  //================================================================================
  // 外部参照
  //================================================================================
  DirectXCommon *dxCommon_ = nullptr;
};
