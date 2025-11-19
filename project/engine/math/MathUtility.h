#pragma once

#include <Matrix4x4.h>
#include <Vector2.h>
#include <Vector3.h>
#include <cmath>

namespace MathUtility {

/// <summary>
/// ベクトル同士の計算
/// </summary>
/// <param name="v1"></param>
/// <param name="v2"></param>
/// <returns></returns>

// 加算
Vector2 Add(const Vector2 &v1, const Vector2 &v2);
Vector3 Add(const Vector3 &v1, const Vector3 &v2);
// 減算
Vector2 Subtract(const Vector2 &v1, const Vector2 &v2);
Vector3 Subtract(const Vector3 &v1, const Vector3 &v2);
// 乗算
Vector2 Multiply(float s, const Vector2 &v2);
Vector3 Multiply(float s, const Vector3 &v1);

// --------------------------
// Vector2
// --------------------------
Vector2 &operator+=(Vector2 &v1, const Vector2 &v2);

Vector2 &operator-=(Vector2 &v1, const Vector2 &v2);

Vector2 &operator*=(Vector2 &v, float s);

// --------------------------
// Vector3
// --------------------------
Vector3 &operator+=(Vector3 &v1, const Vector3 &v2);

Vector3 &operator-=(Vector3 &v1, const Vector3 &v2);

Vector3 &operator*=(Vector3 &v, float s);

/// <summary>
/// 行列の積
/// </summary>
/// <param name="m1"></param>
/// <param name="m2"></param>
/// <returns></returns>
Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2);

/// <summary>
/// 単位行列の作成
/// </summary>
/// <returns></returns>
Matrix4x4 MakeIdentityMatrix();
/// <summary>
/// 逆行列
/// </summary>
/// <param name="m"></param>
/// <returns></returns>
Matrix4x4 MakeInverseMatrix(const Matrix4x4 &matrix);

/// <summary>
/// 拡縮行列
/// </summary>
/// <param name="scale"></param>
/// <returns></returns>
Matrix4x4 MakeScaleMatrix(const Vector3 &scale);
/// <summary>
/// X軸周りの回転行列
/// </summary>
/// <param name="radian"></param>
/// <returns></returns>
Matrix4x4 MakeRotateXMatrix(float radian);
/// <summary>
/// Y軸周りの回転行列
/// </summary>
/// <param name="radian"></param>
/// <returns></returns>
Matrix4x4 MakeRotateYMatrix(float radian);
/// <summary>
/// Z軸周りの回転行列
/// </summary>
/// <param name="radian"></param>
/// <returns></returns>
Matrix4x4 MakeRotateZMatrix(float radian);
/// <summary>
/// 平行移動行列
/// </summary>
/// <param name="translate"></param>
/// <returns></returns>
Matrix4x4 MakeTranslateMatrix(const Vector3 &translate);
/// <summary>
/// アフィン変換行列
/// </summary>
/// <param name="scale">拡縮</param>
/// <param name="rotate">回転</param>
/// <param name="translate">移動</param>
/// <returns></returns>
Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate,
                           const Vector3 &translate);

/// <summary>
/// 正射影行列(3次元版)
/// </summary>
/// <param name="left"></param>
/// <param name="top"></param>
/// <param name="right"></param>
/// <param name="bottom"></param>
/// <param name="nearClip"></param>
/// <param name="farClip"></param>
/// <returns></returns>
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip);

} // namespace MathUtility