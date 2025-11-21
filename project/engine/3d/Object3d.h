#pragma once

#include <Matrix4x4.h>
#include <Transform.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>

#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

class Object3dRenderer;

class Object3d {
public:
  //========================================
  // 初期化 / 更新 / 描画
  //========================================

  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="object3dRenderer">Object3dRendererのポインタ</param>
  void Initialize(Object3dRenderer *object3dRenderer);
  /// <summary>
  /// 更新
  /// </summary>
  void Update();
  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

private:
  //========================================
  // 内部構造体
  //========================================

  // 頂点データ
  struct VertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
  };

  // マテリアル
  struct Material {
    Vector4 color;
    int32_t enableLighting;
    float padding[3];
    Matrix4x4 uvTransform;
  };

  // マテリアルデータ
  struct MaterialData {
    std::string textureFilePath;
    uint32_t textureIndex = 0;
  };

  // モデルデータ
  struct ModelData {
    std::vector<VertexData> vertices;
    MaterialData material;
  };

  // 座標変換行列データ
  struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
  };

  // 平行光源
  struct DirectionalLight {
    Vector4 color;     // ライトの色
    Vector3 direction; // ライトの向き
    float intensity;   // 輝度
  };

public:
  //========================================
  // ファイル読み込む（.mtl / .obj）
  //========================================

  /// <summary>
  /// .mtlファイルを読む
  /// </summary>
  /// <param
  /// name="directoryPath">.mtlファイルが置いてあるディレクトリのパス</param>
  /// <param name="filename">読み込みたい.mtlファイル名</param>
  /// <returns>ファイルから読み込んだマテリアル情報</returns>
  MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                        const std::string &filename);

  /// <summary>
  /// .objファイルを読み込む
  /// </summary>
  /// <param
  /// name="directoryPath">.objファイルが置いてあるディレクトリのパス</param>
  /// <param name="filename">読み込む.objファイル名</param>
  void LoadObjFile(const std::string &directoryPath,
                   const std::string &filename);

private:
  //========================================
  // 型エイリアス
  //========================================

  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
  //========================================
  // 外部参照
  //========================================

  // Object3dRendererのポインタ
  Object3dRenderer *object3dRenderer_ = nullptr;

  //========================================
  // GPUリソース（頂点）
  //========================================

  // 頂点リソース
  ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  VertexData *vertexData_ = nullptr;
  // バッファリソースの使い道を補足するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

  //========================================
  // GPUリソース（定数バッファ）
  //========================================

  // マテリアルリソース
  ComPtr<ID3D12Resource> materialBuffer_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  Material *materialData_ = nullptr;

  // バッファリソース
  ComPtr<ID3D12Resource> transformationMatrixBuffer_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData_ = nullptr;

  // バッファリソース
  ComPtr<ID3D12Resource> directionalLightBuffer_ = nullptr;
  // バッファリソースないのデータを指すポインタ
  DirectionalLight *directionalLightData_ = nullptr;

  //========================================
  // OBJデータ
  //========================================

  // Objファイルのデータ
  ModelData modelData_;

  //========================================
  // Transform (3Dオブジェクト / カメラ)
  //========================================

  // 3DオブジェクトのTransform
  Transform transform_{};
  // 位置
  Vector2 position_ = {0.0f, 0.0f};
  // 回転
  float rotation_ = 0.0f;
  // 拡縮
  Vector2 scale_ = {1.0f, 1.0f};

  // カメラのTransform
  Transform cameraTransform_{};

private:
  //========================================
  // データ作成処理
  //========================================

  /// <summary>
  /// 頂点データの作成
  /// </summary>
  void CreateVertexData();
  /// <summary>
  /// マテリアルデータの作成
  /// </summary>
  void CreateMaterialData();
  /// <summary>
  /// 座標変換行列データの作成
  /// </summary>
  void CreateTransformationMatrixData();
  /// <summary>
  /// 平行光源データの作成
  /// </summary>
  void CreateDirectionalLightData();
};
