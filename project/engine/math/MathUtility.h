#pragma once

#include <cmath>
#include <Matrix4x4.h>
#include <Vector2.h>
#include <Vector3.h>

namespace MathUtility {

	//================================================================================
	// ベクトル演算
	//================================================================================

	// 加算
	inline Vector2 Add(const Vector2& v1, const Vector2& v2) { return {v1.x + v2.x, v1.y + v2.y}; }
	inline Vector3 Add(const Vector3& v1, const Vector3& v2) { return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }

	// 減算
	inline Vector2 Subtract(const Vector2& v1, const Vector2& v2) { return {v1.x - v2.x, v1.y - v2.y}; }
	inline Vector3 Subtract(const Vector3& v1, const Vector3& v2) { return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }

	// 乗算
	inline Vector2 Multiply(const Vector2& v1, const Vector2& v2) { return {v1.x * v2.x, v1.y * v2.y}; }
	inline Vector2 Multiply(float s, const Vector2& v) { return {s * v.x, s * v.y}; }
	inline Vector2 Multiply(const Vector2& v, float s) { return {s * v.x, s * v.y}; }

	inline Vector3 Multiply(const Vector3& v1, const Vector3& v2) { return {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z}; }
	inline Vector3 Multiply(const Vector3& v, float s) { return {s * v.x, s * v.y, s * v.z}; }
	inline Vector3 Multiply(float s, const Vector3& v) { return {s * v.x, s * v.y, s * v.z}; }

	// 外積
	inline Vector3 Cross(const Vector3& a, const Vector3& b) {
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	}

	// 内積
	inline float Dot(const Vector3& a, const Vector3& b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	// 長さ
	inline float Length(const Vector2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
	inline float Length(const Vector3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

	// 距離
	inline float Distance(const Vector2& a, const Vector2& b) {
		Vector2 diff = {b.x - a.x, b.y - a.y};
		return Length(diff);
	}

	// 正規化
	inline Vector3 Normalize(const Vector3& v) {
		float length = Length(v);
		if (length == 0.0f) {
			return {0.0f, 0.0f, 0.0f};
		}
		return {v.x / length, v.y / length, v.z / length};
	}

	Vector3 TransformCoord(const Vector3& v, const Matrix4x4& m);

	// ワールドスクリーン座標変換
	Vector3 Project(const Vector3& worldPosition, float viewportX, float viewportY, float viewportWidth,
					float viewportHeight, const Matrix4x4& matView, const Matrix4x4& viewProjection);

	//================================================================================
	// ベクトル演算子オーバーロード
	//================================================================================

	//---------- Vector2 ----------
	inline Vector2 operator+(const Vector2& v1, const Vector2& v2) { return Add(v1, v2); }
	inline Vector2 operator-(const Vector2& v1, const Vector2& v2) { return Subtract(v1, v2); }

	inline Vector2& operator+=(Vector2& v1, const Vector2& v2) {
		v1.x += v2.x;
		v1.y += v2.y;
		return v1;
	}
	inline Vector2& operator-=(Vector2& v1, const Vector2& v2) {
		v1.x -= v2.x;
		v1.y -= v2.y;
		return v1;
	}
	inline Vector2& operator*=(Vector2& v, float s) {
		v.x *= s;
		v.y *= s;
		return v;
	}
	inline Vector2& operator*=(float s, Vector2& v) {
		v.x *= s;
		v.y *= s;
		return v;
	}

	//---------- Vector3 ----------
	inline Vector3 operator+(const Vector3& v1, const Vector3& v2) { return Add(v1, v2); }
	inline Vector3 operator-(const Vector3& v1, const Vector3& v2) { return Subtract(v1, v2); }
	inline Vector3 operator*(const Vector3& v1, const Vector3& v2) { return Multiply(v1, v2); }
	inline Vector3 operator*(const Vector3& v, float s) { return Multiply(v, s); }
	inline Vector3 operator*(float s, const Vector3& v) { return Multiply(v, s); }

	inline Vector3& operator+=(Vector3& v1, const Vector3& v2) {
		v1.x += v2.x;
		v1.y += v2.y;
		v1.z += v2.z;
		return v1;
	}
	inline Vector3& operator-=(Vector3& v1, const Vector3& v2) {
		v1.x -= v2.x;
		v1.y -= v2.y;
		v1.z -= v2.z;
		return v1;
	}
	inline Vector3& operator*=(Vector3& v, float s) {
		v.x *= s;
		v.y *= s;
		v.z *= s;
		return v;
	}
	inline Vector3& operator*=(float s, Vector3& v) {
		v.x *= s;
		v.y *= s;
		v.z *= s;
		return v;
	}

	//================================================================================
	// 行列演算
	//================================================================================

	/// <summary>
	/// 行列同士の積
	/// </summary>
	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	//================================================================================
	// ベクトルと行列の演算
	//================================================================================

	Vector3 Multiply(const Matrix4x4& m, const Vector3& v);
	Vector3 Multiply(const Vector3& v, const Matrix4x4& m);

	//================================================================================
	// ベクトル演算子オーバーロード
	//================================================================================

	inline Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2) { return Multiply(m1, m2); }

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
	/// 転置行列
	/// </summary>
	/// <param name="m">転置行列を求めたい元の行列</param>
	/// <returns>matrixの転置行列</returns>
	Matrix4x4 MakeTransposeMatrix(const Matrix4x4& m);

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
	/// 3軸の回転行列を合成
	/// </summary>
	/// <param name="rotate">各軸の回転角(ラジアン)</param>
	/// <returns>XYZ順で合成された回転行列</returns>
	Matrix4x4 MakeRotateMatrix(const Vector3& rotate);
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

	/// <summary>
	/// ビューポート変換行列の作成
	/// </summary>
	/// <param name="left">左端の座標（通常は0）</param>
	/// <param name="top">上端の座標（通常は0）</param>
	/// <param name="width">画面の横幅</param>
	/// <param name="height">画面の縦幅</param>
	/// <param name="minDepth">最小深度（通常は0.0f）</param>
	/// <param name="maxDepth">最大深度（通常は1.0f）</param>
	Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
} // namespace MathUtility