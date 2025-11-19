#include "Sprite.h"
#include "MathUtility.h"
#include "SpriteCommon.h"

using namespace MathUtility;

/// <summary>
/// 初期化
/// </summary>
/// <param name="spriteCommon">SpriteCommonの初期化</param>
/// <param name="filePath">使いたいテクスチャのファイルパス</param>
void Sprite::Initialize(SpriteCommon *spriteCommon, std::string filePath) {
  // 引数で受け取ってメンバ変数に記録する
  spriteCommon_ = spriteCommon;
  filePath_ = filePath;

  TextureManager::GetInstance()->LoadTexture(filePath_);
  // 単位行列を書き込んでおく
  textureIndex_ =
      TextureManager::GetInstance()->GetSrvIndexByFilePath(filePath_);

  // 頂点データの作成
  CreateVertexData();
  // インデックスデータの作成
  CreateIndexData();
  // マテリアルデータの作成
  CreateMaterialData();
  // 座標変換行列データの作成
  CreateTransformationMatrixData();
}
/// <summary>
/// 更新
/// </summary>
void Sprite::Update() {
  // 頂点リソースにデータを書き込む
  // 左下
  vertexData_[0] = {{0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
  // 左上
  vertexData_[1] = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}};
  // 右下
  vertexData_[2] = {{1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0}};
  // 右上
  vertexData_[3] = {{1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}};

  // インデックスリソースにデータを書き込む
  indexData_[0] = 0;
  indexData_[1] = 1;
  indexData_[2] = 2;
  indexData_[3] = 1;
  indexData_[4] = 3;
  indexData_[5] = 2;

  // Transform情報を作る
  transform_ = {{scale_.x, scale_.y, 1.0f},
                {0.0f, 0.0f, rotation_},
                {position_.x, position_.y, 0.0f}};

  Matrix4x4 worldMatrix;
  Matrix4x4 viewMatrix;
  Matrix4x4 projectionMatrix;
  Matrix4x4 worldViewProjectionMatrix;

  worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                 transform_.translate);
  viewMatrix = MakeIdentityMatrix();
  projectionMatrix = MakeOrthographicMatrix(
      0.0f, 0.0f, static_cast<float>(WinApp::kClientWidth),
      static_cast<float>(WinApp::kClientHeight), 0.0f, 100.0f);
  worldViewProjectionMatrix =
      Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
  transformationMatrixData_->WVP = worldViewProjectionMatrix;
}
/// <summary>
/// 描画
/// </summary>
void Sprite::Draw() {

  spriteCommon_->SetupCommonRenderState();

  // VertexBufferViewを設定
  spriteCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(
      0, 1, &vertexBufferView_);
  // IndexBufferViewを設定
  spriteCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(
      &indexBufferView_);

  // マテリアルのCBufferの場所を設定
  spriteCommon_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          0, materialBuffer_->GetGPUVirtualAddress());

  // TransformationMatrixのCBufferの場所を設定
  spriteCommon_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          1, transformationMatrixBuffer_->GetGPUVirtualAddress());

  // SRVのデスクリプタヒープテーブルの先頭を設定
  spriteCommon_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootDescriptorTable(
          2, TextureManager::GetInstance()->GetSrvHandlGPU(textureIndex_));

  // 描画
  spriteCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(6, 1, 0,
                                                                       0, 0);
}

/// <summary>
/// 頂点データの作成
/// </summary>
void Sprite::CreateVertexData() {

  // VertexResourceを作成
  vertexBuffer_ = spriteCommon_->GetDxCommon()->CreateBufferResource(
      sizeof(VertexData) * 4);

  // VertexBufferViewを作成する
  vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
  vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
  vertexBufferView_.StrideInBytes = sizeof(VertexData);

  // VertexResourceにデータを書き込む
  vertexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
}
/// <summary>
/// インデックスデータの作成
/// </summary>
void Sprite::CreateIndexData() {
  // indexResourceを作成
  indexBuffer_ =
      spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 6);

  // indexBufferViewを作成する
  indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
  indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
  indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

  // IndexResourceにデータを書き込む
  indexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&indexData_));
}
/// <summary>
/// マテリアルデータの作成
/// </summary>
void Sprite::CreateMaterialData() {
  // マテリアルリソースを作る
  materialBuffer_ =
      spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));

  // 書き込むためのアドレスを取得して、マテリアルにデータを書き込む
  materialBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));

  // マテリアルデータの初期値を書き込む
  materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  materialData_->uvTransform = MakeIdentityMatrix();
}
/// <summary>
/// 座標変換行列データの作成
/// </summary>
void Sprite::CreateTransformationMatrixData() {
  // 座標変換行列リソースを作る
  transformationMatrixBuffer_ =
      spriteCommon_->GetDxCommon()->CreateBufferResource(
          sizeof(TransformationMatrix));

  // 書き込むためのアドレスを取得して、座標変換行列データに書き込む
  transformationMatrixBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));
  // 単位行列に書き込んでおく
  transformationMatrixData_->WVP = MakeIdentityMatrix();
}

// 位置
void Sprite::SetPosition(const Vector2 &position) { position_ = position; }
// 回転
void Sprite::SetRotation(float rotation) { rotation_ = rotation; }
// 色
void Sprite::SetColor(const Vector4 &color) { materialData_->color = color; }
// 拡縮
void Sprite::SetScale(const Vector2 &scale) { scale_ = scale; }
// テクスチャ
void Sprite::SetTexture(const std::string &filePath) {
  TextureManager *textureManager = TextureManager::GetInstance();
  textureManager->LoadTexture(filePath);
  textureIndex_ = textureManager->GetSrvIndexByFilePath(filePath);
}