#pragma once

#include <Matrix4x4.h>
#include <Vector2.h>
#include <Vector3.h>

namespace MathUtility {

	//================================================================================
	// ベクトル演算
	//================================================================================

	// 加算
	Vector2 Add(const Vector2& v1, const Vector2& v2);
	Vector3 Add(const Vector3& v1, const Vector3& v2);

	// 減算
	Vector2 Subtract(const Vector2& v1, const Vector2& v2);
	Vector3 Subtract(const Vector3& v1, const Vector3& v2);

	// 乗算
	Vector2 Multiply(const Vector2& v1, const Vector2& v2);
	Vector2 Multiply(float s, const Vector2& v);
	Vector2 Multiply(const Vector2& v, float s);

	Vector3 Multiply(const Vector3& v1, const Vector3& v2);
	Vector3 Multiply(const Vector3& v, float s);
	Vector3 Multiply(float s, const Vector3& v);

	// 外積
	Vector3 Cross(const Vector3& a, const Vector3& b);

	// 内積
	float Dot(const Vector3& a, const Vector3& b);

	// 長さ
	float Length(const Vector2& v);
	float Length(const Vector3& v);

	// 正規化
	Vector3 Normalize(const Vector3& v);

	//================================================================================
	// ベクトル演算子オーバーロード
	//================================================================================

	//---------- Vector2 ----------
	Vector2 operator+(const Vector2& v1, const Vector2& v2);
	Vector2 operator-(const Vector2& v1, const Vector2& v2);

	Vector2& operator+=(Vector2& v1, const Vector2& v2);
	Vector2& operator-=(Vector2& v1, const Vector2& v2);
	Vector2& operator*=(Vector2& v, float s);
	Vector2& operator*=(float s, Vector2& v);

	//---------- Vector3 ----------
	Vector3 operator+(const Vector3& v1, const Vector3& v2);
	Vector3 operator-(const Vector3& v1, const Vector3& v2);
	Vector3 operator*(const Vector3& v1, const Vector3& v2);
	Vector3 operator*(const Vector3& v, float s);
	Vector3 operator*(float s, const Vector3& v);

	Vector3& operator+=(Vector3& v1, const Vector3& v2);
	Vector3& operator-=(Vector3& v1, const Vector3& v2);
	Vector3& operator*=(Vector3& v, float s);
	Vector3& operator*=(float s, Vector3& v);

	//================================================================================
	// 行列演算
	//================================================================================

	/// <summary>
	/// 行列同士の積
	/// </summary>
	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	//================================================================================
	// ベクトル演算子オーバーロード
	//================================================================================

	Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);

	//================================================================================
	// 基本行列の生成
	//================================================================================

	/// <summary>
	/// 単位行列の作成
	/// </summary>
	/// <returns>対角成分が1、それ以外が0の単位行列</returns>
	Matrix4x4 MakeIdentityMatrix();
	/// <summary>
	/// 逆行列
	/// </summary>
	/// <param name="m">逆行列を求めたい元の行列</param>
	/// <returns>matrixの逆行列</returns>
	Matrix4x4 MakeInverseMatrix(const Matrix4x4& matrix);

	/// <summary>
	/// 拡縮行列
	/// </summary>
	/// <param name="scale">各軸の拡縮量</param>
	/// <returns>scaleに基づいた拡縮行列</returns>
	Matrix4x4 MakeScaleMatrix(const Vector3& scale);
	/// <summary>
	/// X軸周りの回転行列
	/// </summary>
	/// <param name="radian">回転角（ラジアン）</param>
	/// <returns>指定角度だけX軸周りに回転させる回転行列</returns>
	Matrix4x4 MakeRotateXMatrix(float radian);
	/// <summary>
	/// Y軸周りの回転行列
	/// </summary>
	/// <param name="radian">回転角（ラジアン）</param>
	/// <returns>指定角度だけY軸周りに回転させる回転行列</returns>
	Matrix4x4 MakeRotateYMatrix(float radian);
	/// <summary>
	/// Z軸周りの回転行列
	/// </summary>
	/// <param name="radian">回転角（ラジアン）</param>
	/// <returns>指定角度だけZ軸周りに回転させる回転行列</returns>
	Matrix4x4 MakeRotateZMatrix(float radian);
	/// <summary>
	/// 平行移動行列
	/// </summary>
	/// <param name="translate">各軸方向の移動量</param>
	/// <returns>translateに基づいた平行移動行列</returns>
	Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
	/// <summary>
	/// アフィン変換行列
	/// </summary>
	/// <param name="scale"> 各軸の拡縮量</param>
	/// <param name="rotate">各軸の回転角</param>
	/// <param name="translate">各軸方向の移動量</param>
	/// <returns>scale・rotate・translateを合成したアフィン変換行列</returns>
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate,
		const Vector3& translate);

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
		float bottom, float nearClip, float farClip);

	/// <summary>
	/// 透視投影行列
	/// </summary>
	/// <param name="fovY">垂直方向の視野角（ラジアン）</param>
	/// <param name="aspectRatio">アスペクト比（画面の横幅 ÷ 高さ）</param>
	/// <param name="nearClip">手前側のクリップ距離</param>
	/// <param name="farClip">奥側のクリップ距離</param>
	/// <returns>指定されたパラメータで遠近感を持った透視投影を行う行列。遠ざかるほど小さく見える</returns>
	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
		float nearClip, float farClip);
} // namespace MathUtility