#pragma once
#include "TextureManager.h"

#include <Matrix4x4.h>
#include <Transform.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <wrl.h>

class SpriteCommon;

class Sprite {
public:
  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="spriteCommon">SpriteCommonのポインタ</param>
  /// <param name="filePath">使いたいテクスチャのファイルパス</param>
  void Initialize(SpriteCommon *spriteCommon, std::string filePath);
  /// <summary>
  /// 更新
  /// </summary>
  void Update();
  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

public:
  /// <summary>
  /// Getter
  /// </summary>

  // 位置
  const Vector2 &GetPosition() const { return position_; }
  // 回転
  const float &GetRotation() const { return rotation_; }
  // 色
  const Vector4 &GetColor() const { return materialData_->color; }
  // 拡縮
  const Vector2 &GetScale() const { return scale_; }

  /// <summary>
  /// Setter
  /// </summary>

  // 位置
  void SetPosition(const Vector2 &position);
  // 回転
  void SetRotation(float rotation);
  // 色
  void SetColor(const Vector4 &color);
  // 拡縮
  void SetScale(const Vector2 &scale);
  // テクスチャ
  void SetTexture(const std::string &filePath);

private:
  // 頂点データ
  struct VertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
  };

  // マテリアル
  struct Material {
    Vector4 color;
    Matrix4x4 uvTransform;
  };

  // 座標変換行列データ
  struct TransformationMatrix {
    Matrix4x4 WVP;
  };

private:
  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

  // SpriteCommonのポインタ
  SpriteCommon *spriteCommon_ = nullptr;

  // 頂点リソース
  ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
  ComPtr<ID3D12Resource> indexBuffer_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  VertexData *vertexData_ = nullptr;
  uint32_t *indexData_ = nullptr;
  // バッファリソースの使い道を補足するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
  D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

  // マテリアルリソース(定数バッファ)
  ComPtr<ID3D12Resource> materialBuffer_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  Material *materialData_ = nullptr;

  // バッファリソース(定数バッファ)
  ComPtr<ID3D12Resource> transformationMatrixBuffer_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData_ = nullptr;

  // テクスチャ番号
  uint32_t textureIndex_ = 0;
  // ファイルパス
  std::string filePath_;

  // Transform
  Transform transform_{};
  Vector2 scale_ = {640.0f, 360.0f};
  float rotation_ = 0.0f;
  Vector2 position_ = {0.0f, 0.0f};

private:
  /// <summary>
  /// 頂点データの作成
  /// </summary>
  void CreateVertexData();
  /// <summary>
  /// インデックスデータの作成
  /// </summary>
  void CreateIndexData();
  /// <summary>
  /// マテリアルデータの作成
  /// </summary>
  void CreateMaterialData();
  /// <summary>
  /// 座標変換行列データの作成
  /// </summary>
  void CreateTransformationMatrixData();
};
