#pragma once

#include <Matrix4x4.h>
#include <Transform.h>

class Camera {
public:
  //================================================================================
  // コンストラクタ / 更新
  //================================================================================

  /// <summary>
  /// コンストラクタ
  /// </summary>
  Camera();

  /// <summary>
  /// 更新
  /// </summary>
  void Update();

private:
  //================================================================================
  // Transform と行列 / 投影パラメータ
  //================================================================================

  Transform transform_;
  Matrix4x4 worldMatrix_;
  Matrix4x4 viewMatrix_;

  Matrix4x4 projectionMatrix_;
  // 水平方向視野角
  float fovY_ = 0.45f;
  // アスペクト比
  float aspectRatio_ = 16.0f / 9.0f;
  // ニアクリップ距離
  float nearClip_ = 0.1f;
  // ファークリップ距離
  float farClip_ = 100.0f;

  Matrix4x4 viewProjectionMatrix_;

public:
  //================================================================================
  // Getter
  //================================================================================

  // ワールド行列
  const Matrix4x4 &GetWorldMatrix() const { return worldMatrix_; }
  // ビュー行列
  const Matrix4x4 &GetViewMatrix() const { return viewMatrix_; }
  // プロジェクション行列
  const Matrix4x4 &GetProjectionMatrix() const { return projectionMatrix_; }
  // ビュープロジェクション行列
  const Matrix4x4 &GetViewProjectionMatrix() const {
    return viewProjectionMatrix_;
  }

  // 回転
  Vector3 GetRotate() const { return transform_.rotation; }
  // 移動
  Vector3 GetTranslate() const { return transform_.translation; }

  //================================================================================
  // Setter
  //================================================================================

  // 回転
  void SetRotate(const Vector3 &rotation) { transform_.rotation = rotation; }
  // 移動
  void SetTranslate(const Vector3 &translation) {
    transform_.translation = translation;
  }

  // 水平方向視野角
  void SetFovY(float fovY) { fovY_ = fovY; }
  // アスペクト比
  void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; }
  // ニアクリップ距離
  void SetNearClip(float nearClip) { nearClip_ = nearClip; }
  // ファークリップ距離
  void SetFarClip(float farClip) { farClip_ = farClip; }
};
