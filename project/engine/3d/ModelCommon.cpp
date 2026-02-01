#include "ModelCommon.h"

#include "DirectXCommon.h"

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<ModelCommon> ModelCommon::instance = nullptr;

// 唯一のインスタンス取得
ModelCommon *ModelCommon::GetInstance() {
  if (instance == nullptr) {
    instance.reset(new ModelCommon());
  }

  return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void ModelCommon::Finalize() { instance.reset(); }

//================================================================================
// 初期化
//================================================================================

/// <summary>
/// 初期化
/// </summary>
/// <param name="dxCommon">DirectXCommonのポインタ</param>
void ModelCommon::Initialize(DirectXCommon *dxCommon) { dxCommon_ = dxCommon; }