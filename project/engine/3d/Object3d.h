#pragma once

#include <Matrix4x4.h>
#include <Transform.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>

#include <d3d12.h>
#include <string>
#include <wrl.h>

class Object3dRenderer;
class Model;

class Object3d {
public:
  //================================================================================
  // 初期化 / 更新 / 描画
  //================================================================================

  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="object3dRenderer">Object3dRendererのポインタ</param>
  /// /// <param name="filePath">モデルファイルのパス</param>
  void Initialize(Object3dRenderer *object3dRenderer,
                  const std::string &filePath);
  /// <summary>
  /// 更新
  /// </summary>
  void Update();
  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

public:
  //================================================================================
  // Getter
  //================================================================================

  // 位置
  const Vector2 &GetPosition() const { return position_; }
  // 回転
  const float &GetRotation() const { return rotation_; }
  // 拡縮
  const Vector2 &GetScale() const { return scale_; }

  //================================================================================
  // Setter
  //================================================================================

  // 位置
  void SetPosition(const Vector2 &position);
  // 回転
  void SetRotation(float rotation);
  // 拡縮
  void SetScale(const Vector2 &scale);
  // モデル
  void SetModel(const std::string &filePath);

private:
  //================================================================================
  // 内部構造体
  //================================================================================

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

private:
  //================================================================================
  // 型エイリアス
  //================================================================================

  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
  //================================================================================
  // 外部参照
  //================================================================================

  // Object3dRendererのポインタ
  Object3dRenderer *object3dRenderer_ = nullptr;

  // Modelのポインタ
  Model *model_ = nullptr;

  //================================================================================
  // GPUリソース（定数バッファ）
  //================================================================================

  // バッファリソース
  ComPtr<ID3D12Resource> transformationMatrixBuffer_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData_ = nullptr;

  // バッファリソース
  ComPtr<ID3D12Resource> directionalLightBuffer_ = nullptr;
  // バッファリソースないのデータを指すポインタ
  DirectionalLight *directionalLightData_ = nullptr;

  //================================================================================
  // Transform (3Dオブジェクト / カメラ)
  //================================================================================

  // 3DオブジェクトのTransform
  Transform transform_{};
  // 位置
  Vector2 position_ = {0.0f, 0.0f};
  // 回転
  float rotation_ = -3.14f;
  // 拡縮
  Vector2 scale_ = {1.0f, 1.0f};

  // カメラのTransform
  Transform cameraTransform_{};

private:
  //================================================================================
  // データ作成処理
  //================================================================================

  /// <summary>
  /// 座標変換行列データの作成
  /// </summary>
  void CreateTransformationMatrixData();
  /// <summary>
  /// 平行光源データの作成
  /// </summary>
  void CreateDirectionalLightData();
};
