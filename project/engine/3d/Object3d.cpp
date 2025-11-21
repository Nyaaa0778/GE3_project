#include "Object3d.h"
#include "DirectXCommon.h"
#include "MathUtility.h"
#include "Model.h"
#include "Object3dRenderer.h"
#include "TextureManager.h"

using namespace MathUtility;

//================================================================================
// 初期化 / 更新 / 描画
//================================================================================

/// <summary>
/// 初期化
/// </summary>
/// <param name="object3dRenderer">Object3dRendererのポインタ</param>
void Object3d::Initialize(Object3dRenderer *object3dRenderer) {
  // 引数で受け取ってメンバ変数に保存
  object3dRenderer_ = object3dRenderer;

  // 座標変換行列データの作成
  CreateTransformationMatrixData();
  // 平行光源データの作成
  CreateDirectionalLightData();

  // Transform変数を作成
  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, -3.14f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 4.0f, -10.0f}};
}
/// <summary>
/// 更新
/// </summary>
void Object3d::Update() {
  // transformからworldMatrixを作成
  Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                           transform_.translate);

  // cameraTransformからcameraMatrixを作成
  Matrix4x4 cameraMatrix =
      MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate,
                       cameraTransform_.translate);
  // cameraMatrixからviewMatrixを作成
  Matrix4x4 viewMatrix = MakeInverseMatrix(cameraMatrix);
  // projectionMatrixを作成して透視投影行列を書き込む
  Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
      0.45f,
      float(object3dRenderer_->GetDxCommon()->GetClientWidth()) /
          float(object3dRenderer_->GetDxCommon()->GetClientHeight()),
      0.1f, 100.0f);

  transformationMatrixData_->WVP = worldMatrix * viewMatrix * projectionMatrix;
  transformationMatrixData_->World = worldMatrix;
}
/// <summary>
/// 描画
/// </summary>
void Object3d::Draw() {

  object3dRenderer_->SetupCommonRenderState();

  // 座標変換行列のCBufferの場所を設定
  object3dRenderer_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          1, transformationMatrixBuffer_->GetGPUVirtualAddress());

  // 平行光源CBufferの場所を設定
  object3dRenderer_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          3, directionalLightBuffer_->GetGPUVirtualAddress());

  // 3Dモデルが割り当てられていれば描画する
  if (model_) {
    model_->Draw();
  }
}

//================================================================================
// データ作成処理
//================================================================================

/// <summary>
/// 座標変換行列データの作成
/// </summary>
void Object3d::CreateTransformationMatrixData() {
  // 座標変換行列リソースを作成
  transformationMatrixBuffer_ =
      object3dRenderer_->GetDxCommon()->CreateBufferResource(
          sizeof(TransformationMatrix));

  // transformationMatrixResourceに座標変換行列データを書き込む
  transformationMatrixBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));

  // 単位行列を書き込んでおく
  transformationMatrixData_->WVP = MakeIdentityMatrix();
  transformationMatrixData_->World = MakeIdentityMatrix();
}
/// <summary>
/// 平行光源データの作成
/// </summary>
void Object3d::CreateDirectionalLightData() {
  // 平行光源リソースを作成
  directionalLightBuffer_ =
      object3dRenderer_->GetDxCommon()->CreateBufferResource(
          sizeof(DirectionalLight));

  // directionalLightResourceに平行光源データを書き込む
  directionalLightBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData_));

  // 平行光源データの初期値を書き込む
  directionalLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLightData_->direction = {0.0f, -1.0f, 0.0f};
  directionalLightData_->intensity = 1.0f;
}

//================================================================================
// Setter
//================================================================================

// 位置
void Object3d::SetPosition(const Vector2 &position) { position_ = position; }
// 回転
void Object3d::SetRotation(float rotation) { rotation_ = rotation; }
// 拡縮
void Object3d::SetScale(const Vector2 &scale) { scale_ = scale; }