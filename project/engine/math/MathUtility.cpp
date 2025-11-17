#include "MathUtility.h"

namespace MathUtility {

/// <summary>
/// ベクトル同士の計算
/// </summary>
/// <param name="v1"></param>
/// <param name="v2"></param>
/// <returns></returns>

// 加算
Vector3 Add(const Vector3 &v1, const Vector3 &v2) {
  return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}
// 減算
Vector3 Subtract(const Vector3 &v1, const Vector3 &v2) {
  return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}
// 乗算
Vector3 Multiply(float s, const Vector3 &v1) {
  return {s * v1.x, s * v1.y, s * v1.z};
}

Vector3 operator+(const Vector3 &v1, const Vector3 &v2) { return Add(v1, v2); }
Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
  return Subtract(v1, v2);
}
Vector3 operator*(float s, const Vector3 &v2) { return Multiply(s, v2); }
Vector3 operator*(const Vector3 &v2, float s) { return Multiply(s, v2); }

/// <summary>
/// 行列の積
/// </summary>
/// <param name="m1"></param>
/// <param name="m2"></param>
/// <returns></returns>
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

/// <summary>
/// 単位行列の作成
/// </summary>
/// <returns></returns>
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
/// <param name="m"></param>
/// <returns></returns>
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
/// <param name="scale"></param>
/// <returns></returns>
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
/// <param name="radian"></param>
/// <returns></returns>
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
/// <param name="radian"></param>
/// <returns></returns>
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
/// <param name="radian"></param>
/// <returns></returns>
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
/// <param name="translate"></param>
/// <returns></returns>
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
/// <param name="scale">拡縮</param>
/// <param name="rotate">回転</param>
/// <param name="translate">移動</param>
/// <returns></returns>
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

} // namespace MathUtility