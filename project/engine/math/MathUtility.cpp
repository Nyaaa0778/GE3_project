#include "MathUtility.h"

#include <cmath>

namespace MathUtility {

//================================================================================
// ベクトル演算
//================================================================================

// 加算
Vector2 Add(const Vector2 &v1, const Vector2 &v2) {
  return {v1.x + v2.x, v1.y + v2.y};
}
Vector3 Add(const Vector3 &v1, const Vector3 &v2) {
  return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

// 減算
Vector2 Subtract(const Vector2 &v1, const Vector2 &v2) {
  return {v1.x - v2.x, v1.y - v2.y};
}
Vector3 Subtract(const Vector3 &v1, const Vector3 &v2) {
  return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

// 乗算
Vector2 Multiply(const Vector2 &v1, const Vector2 &v2) {
  return {v1.x * v2.x, v1.y * v2.y};
}
Vector3 Multiply(float s, const Vector3 &v1) {
  return {s * v1.x, s * v1.y, s * v1.z};
}
Vector3 Multiply(const Vector3 &v1, float s) {
  return {s * v1.x, s * v1.y, s * v1.z};
}

// 外積
Vector3 Cross(const Vector3 &a, const Vector3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// 内積
float Dot(const Vector3 &a, const Vector3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 正規化
Vector3 Normalize(const Vector3 &v) {
  float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  if (len == 0.0f) {
    return {0.0f, 0.0f, 0.0f}; // ゼロ割り防止
  }
  return {v.x / len, v.y / len, v.z / len};
}

//================================================================================
// ベクトル演算子オーバーロード
//================================================================================

//---------- Vector2 ----------

Vector2 operator+(const Vector2 &v1, const Vector2 &v2) {
  return {v1.x + v2.x, v1.y + v2.y};
}

Vector2 operator-(const Vector2 &v1, const Vector2 &v2) {
  return {v1.x - v2.x, v1.y - v2.y};
}

Vector2 &operator+=(Vector2 &v1, const Vector2 &v2) {
  v1.x += v2.x;
  v1.y += v2.y;
  return v1;
}

Vector2 &operator-=(Vector2 &v1, const Vector2 &v2) {
  v1.x -= v2.x;
  v1.y -= v2.y;
  return v1;
}

Vector2 &operator*=(Vector2 &v, float s) {
  v.x *= s;
  v.y *= s;
  return v;
}
Vector2 &operator*=(float s, Vector2 &v) {
  v.x *= s;
  v.y *= s;
  return v;
}

//---------- Vector3 ----------

Vector3 operator+(const Vector3 &v1, const Vector3 &v2) {
  return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
  return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

Vector3 operator*(const Vector3 &v1, float s) {
  return {v1.x * s, v1.y * s, v1.z * s};
}
Vector3 operator*(float s, const Vector3 &v1) {
  return {v1.x * s, v1.y * s, v1.z * s};
}

Vector3 &operator+=(Vector3 &v1, const Vector3 &v2) {
  v1.x += v2.x;
  v1.y += v2.y;
  v1.z += v2.z;
  return v1;
}

Vector3 &operator-=(Vector3 &v1, const Vector3 &v2) {
  v1.x -= v2.x;
  v1.y -= v2.y;
  v1.z -= v2.z;
  return v1;
}

Vector3 &operator*=(Vector3 &v, float s) {
  v.x *= s;
  v.y *= s;
  v.z *= s;
  return v;
}

//================================================================================
// 行列演算
//================================================================================

/// <summary>
/// 行列の積
/// </summary>
Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2) {
  Matrix4x4 result;

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0;
      for (int k = 0; k < 4; ++k) {
        result.m[i][j] += m1.m[i][k] * m2.m[k][j];
      }
    }
  }

  return result;
}

//================================================================================
// 行列演算子オーバーロード
//================================================================================

Matrix4x4 operator*(const Matrix4x4 &m1, const Matrix4x4 &m2) {
  return Multiply(m1, m2);
}

//================================================================================
// 基本行列の生成
//================================================================================

/// <summary>
/// 単位行列の作成
/// </summary>
/// <returns>対角成分が1、それ以外が0の単位行列</returns>
Matrix4x4 MakeIdentityMatrix() {
  Matrix4x4 result;

  // いったん全部0にする
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
    }
  }

  for (int i = 0; i < 4; ++i) {
    result.m[i][i] = 1.0f;
  }

  return result;
}
/// <summary>
/// 逆行列
/// </summary>
/// <param name="m">逆行列を求めたい元の行列</param>
/// <returns>matrixの逆行列</returns>
Matrix4x4 MakeInverseMatrix(const Matrix4x4 &matrix) {
  Matrix4x4 result;

  // 2×2 の小行列式を具体的な名前で計算
  // 上部 2×2 ブロックの小行列式
  float minor00to11 =
      matrix.m[0][0] * matrix.m[1][1] - matrix.m[0][1] * matrix.m[1][0];
  float minor00to12 =
      matrix.m[0][0] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][0];
  float minor00to13 =
      matrix.m[0][0] * matrix.m[1][3] - matrix.m[0][3] * matrix.m[1][0];
  float minor01to12 =
      matrix.m[0][1] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][1];
  float minor01to13 =
      matrix.m[0][1] * matrix.m[1][3] - matrix.m[0][3] * matrix.m[1][1];
  float minor02to13 =
      matrix.m[0][2] * matrix.m[1][3] - matrix.m[0][3] * matrix.m[1][2];

  // 下部 2×2 ブロックの小行列式
  float minor20to31 =
      matrix.m[2][0] * matrix.m[3][1] - matrix.m[2][1] * matrix.m[3][0];
  float minor20to32 =
      matrix.m[2][0] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][0];
  float minor20to33 =
      matrix.m[2][0] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][0];
  float minor21to32 =
      matrix.m[2][1] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][1];
  float minor21to33 =
      matrix.m[2][1] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][1];
  float minor22to33 =
      matrix.m[2][2] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][2];

  // 2) 全行列式を計算
  float determinant = minor00to11 * minor22to33 - minor00to12 * minor21to33 +
                      minor00to13 * minor21to32 + minor01to12 * minor20to33 -
                      minor01to13 * minor20to32 + minor02to13 * minor20to31;

  if (determinant == 0.0f) {
    // 特異行列ならゼロ行列を返す
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        result.m[r][c] = 0.0f;
      }
    }
    return result;
  }

  float invDet = 1.0f / determinant;

  // 余因子行列を計算しつつ逆行列を構成
  result.m[0][0] =
      (matrix.m[1][1] * minor22to33 - matrix.m[1][2] * minor21to33 +
       matrix.m[1][3] * minor21to32) *
      invDet;

  result.m[0][1] =
      (-matrix.m[0][1] * minor22to33 + matrix.m[0][2] * minor21to33 -
       matrix.m[0][3] * minor21to32) *
      invDet;

  result.m[0][2] =
      (matrix.m[3][1] * minor02to13 - matrix.m[3][2] * minor01to13 +
       matrix.m[3][3] * minor01to12) *
      invDet;

  result.m[0][3] =
      (-matrix.m[2][1] * minor02to13 + matrix.m[2][2] * minor01to13 -
       matrix.m[2][3] * minor01to12) *
      invDet;

  result.m[1][0] =
      (-matrix.m[1][0] * minor22to33 + matrix.m[1][2] * minor20to33 -
       matrix.m[1][3] * minor20to32) *
      invDet;

  result.m[1][1] =
      (matrix.m[0][0] * minor22to33 - matrix.m[0][2] * minor20to33 +
       matrix.m[0][3] * minor20to32) *
      invDet;

  result.m[1][2] =
      (-matrix.m[3][0] * minor02to13 + matrix.m[3][2] * minor00to13 -
       matrix.m[3][3] * minor00to12) *
      invDet;

  result.m[1][3] =
      (matrix.m[2][0] * minor02to13 - matrix.m[2][2] * minor00to13 +
       matrix.m[2][3] * minor00to12) *
      invDet;

  result.m[2][0] =
      (matrix.m[1][0] * minor21to33 - matrix.m[1][1] * minor20to33 +
       matrix.m[1][3] * minor20to31) *
      invDet;

  result.m[2][1] =
      (-matrix.m[0][0] * minor21to33 + matrix.m[0][1] * minor20to33 -
       matrix.m[0][3] * minor20to31) *
      invDet;

  result.m[2][2] =
      (matrix.m[3][0] * minor01to13 - matrix.m[3][1] * minor00to13 +
       matrix.m[3][3] * minor00to11) *
      invDet;

  result.m[2][3] =
      (-matrix.m[2][0] * minor01to13 + matrix.m[2][1] * minor00to13 -
       matrix.m[2][3] * minor00to11) *
      invDet;

  result.m[3][0] =
      (-matrix.m[1][0] * minor21to32 + matrix.m[1][1] * minor20to32 -
       matrix.m[1][2] * minor20to31) *
      invDet;

  result.m[3][1] =
      (matrix.m[0][0] * minor21to32 - matrix.m[0][1] * minor20to32 +
       matrix.m[0][2] * minor20to31) *
      invDet;

  result.m[3][2] =
      (-matrix.m[3][0] * minor01to12 + matrix.m[3][1] * minor00to12 -
       matrix.m[3][2] * minor00to11) *
      invDet;

  result.m[3][3] =
      (matrix.m[2][0] * minor01to12 - matrix.m[2][1] * minor00to12 +
       matrix.m[2][2] * minor00to11) *
      invDet;

  return result;
}

/// <summary>
/// 拡縮行列
/// </summary>
/// <param name="scale">各軸の拡縮量</param>
/// <returns>scaleに基づいた拡縮行列</returns>
Matrix4x4 MakeScaleMatrix(const Vector3 &scale) {
  Matrix4x4 result = MakeIdentityMatrix();

  result.m[0][0] = scale.x;
  result.m[1][1] = scale.y;
  result.m[2][2] = scale.z;

  return result;
}
/// <summary>
/// X軸周りの回転行列
/// </summary>
/// <param name="radian">回転角（ラジアン）</param>
/// <returns>指定角度だけX軸周りに回転させる回転行列</returns>
Matrix4x4 MakeRotateXMatrix(float radian) {
  Matrix4x4 result = MakeIdentityMatrix();

  result.m[1][1] = std::cos(radian);
  result.m[1][2] = std::sin(radian);
  result.m[2][1] = -std::sin(radian);
  result.m[2][2] = std::cos(radian);

  return result;
}
/// <summary>
/// Y軸周りの回転行列
/// </summary>
/// <param name="radian">回転角（ラジアン）</param>
/// <returns>指定角度だけY軸周りに回転させる回転行列</returns>
Matrix4x4 MakeRotateYMatrix(float radian) {
  Matrix4x4 result = MakeIdentityMatrix();

  result.m[0][0] = std::cos(radian);
  result.m[0][2] = -std::sin(radian);
  result.m[2][0] = std::sin(radian);
  result.m[2][2] = std::cos(radian);

  return result;
}
/// <summary>
/// Z軸周りの回転行列
/// </summary>
/// <param name="radian">回転角（ラジアン）</param>
/// <returns>指定角度だけZ軸周りに回転させる回転行列</returns>
Matrix4x4 MakeRotateZMatrix(float radian) {
  Matrix4x4 result = MakeIdentityMatrix();

  result.m[0][0] = std::cos(radian);
  result.m[0][1] = std::sin(radian);
  result.m[1][0] = -std::sin(radian);
  result.m[1][1] = std::cos(radian);

  return result;
}
/// <summary>
/// 平行移動行列
/// </summary>
/// <param name="translate">各軸方向の移動量</param>
/// <returns>translateに基づいた平行移動行列</returns>
Matrix4x4 MakeTranslateMatrix(const Vector3 &translate) {
  Matrix4x4 result = MakeIdentityMatrix();

  result.m[3][0] = translate.x;
  result.m[3][1] = translate.y;
  result.m[3][2] = translate.z;

  return result;
}
/// <summary>
/// アフィン変換行列
/// </summary>
/// <param name="scale"> 各軸の拡縮量</param>
/// <param name="rotate">各軸の回転角</param>
/// <param name="translate">各軸方向の移動量</param>
/// <returns>scale・rotate・translateを合成したアフィン変換行列</returns>
Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate,
                           const Vector3 &translate) {

  Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
  Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
  Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
  Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
  Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

  Matrix4x4 worldMatrix = Multiply(
      Multiply(Multiply(Multiply(scaleMatrix, rotateXMatrix), rotateYMatrix),
               rotateZMatrix),
      translateMatrix);

  return worldMatrix;
}

//================================================================================
// 投影行列
//================================================================================

/// <summary>
/// 正射影行列(3次元版)
/// </summary>
/// <param name="left">ビュー空間での左端の位置</param>
/// <param name="top">ビュー空間での上端の位置</param>
/// <param name="right">ビュー空間での右端の位置</param>
/// <param name="bottom">ビュー空間での下端の位置</param>
/// <param name="nearClip">手前側のクリップ距離</param>
/// <param name="farClip">奥側のクリップ距離</param>
/// <returns>指定された範囲を正射影するための行列。奥行きによる縮小が発生しない投影</returns>
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip) {
  Matrix4x4 result;
  result.m[0][0] = 2.0f / (right - left);
  result.m[0][1] = 0.0f;
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;

  result.m[1][0] = 0.0f;
  result.m[1][1] = 2.0f / (top - bottom);
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;

  result.m[2][0] = 0.0f;
  result.m[2][1] = 0.0f;
  result.m[2][2] = 1.0f / (farClip - nearClip);
  result.m[2][3] = 0.0f;

  result.m[3][0] = (left + right) / (left - right);
  result.m[3][1] = (top + bottom) / (bottom - top);
  result.m[3][2] = nearClip / (nearClip - farClip);
  result.m[3][3] = 1.0f;

  return result;
}

/// <summary>
/// 透視投影行列
/// </summary>
/// <param name="fovY">垂直方向の視野角（ラジアン）</param>
/// <param name="aspectRatio">アスペクト比（画面の横幅 ÷ 高さ）</param>
/// <param name="nearClip">手前側のクリップ距離</param>
/// <param name="farClip">奥側のクリップ距離</param>
/// <returns>指定されたパラメータで遠近感を持った透視投影を行う行列。遠ざかるほど小さく見える</returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip) {
  Matrix4x4 result;
  result.m[0][0] = 1.0f / aspectRatio * 1.0f / std::tan(fovY / 2.0f);
  result.m[0][1] = 0.0f;
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;

  result.m[1][0] = 0.0f;
  result.m[1][1] = 1.0f / std::tan(fovY / 2.0f);
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;

  result.m[2][0] = 0.0f;
  result.m[2][1] = 0.0f;
  result.m[2][2] = farClip / (farClip - nearClip);
  result.m[2][3] = 1.0f;

  result.m[3][0] = 0.0f;
  result.m[3][1] = 0.0f;
  result.m[3][2] = -nearClip * farClip / (farClip - nearClip);
  result.m[3][3] = 0.0f;

  return result;
}

} // namespace MathUtility