#pragma once
#include <Matrix4x4.h>
#include <Vector3.h>
#include <d3d12.h>
#include <wrl.h>

// 定数バッファ用構造体 (シェーダーの TransformationMatrix と一致させる)
struct ConstBufferDataWorldTransform {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Matrix4x4 WorldInverseTranspose;
};

struct WorldTransform {
	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

	// スケール
	Vector3 scale = {1.0f, 1.0f, 1.0f};
	// 回転
	Vector3 rotation = {0.0f, 0.0f, 0.0f};
	// 座標
	Vector3 translation = {0.0f, 0.0f, 0.0f};
	// ローカル・ワールド変換行列
	Matrix4x4 matWorld;
	// 親ワールド変換へのポインタ
	const WorldTransform* parent = nullptr;

	// 定数バッファ
	ComPtr<ID3D12Resource> constBuffer = nullptr;
	// マッピング用ポインタ
	ConstBufferDataWorldTransform* constMap = nullptr;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 行列の更新
	/// </summary>
	void UpdateMatrix();
};
