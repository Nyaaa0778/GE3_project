#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <Matrix4x4.h>
#include <Vector3.h>

// 座標変換行列データ (定数バッファ用)
struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Matrix4x4 WorldInverseTranspose;
};

class WorldTransform {
public:
	// ローカルパラメータ
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector3 rotation = { 0.0f, 0.0f, 0.0f };
	Vector3 translation = { 0.0f, 0.0f, 0.0f };

	// ローカル・ワールド行列
	Matrix4x4 matWorld;

	// 親へのポインタ
	const WorldTransform* parent = nullptr;

	// 定数バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer = nullptr;
	// マッピング済みポインタ
	TransformationMatrix* constMap = nullptr;

public:
	/// <summary>
	/// 初期化（定数バッファの作成とマッピング）
	/// </summary>
	void Initialize();

	/// <summary>
	/// 行列の計算・更新
	/// </summary>
	void UpdateMatrix();

	/// <summary>
	/// GPU仮想アドレスの取得
	/// </summary>
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const {
		return constBuffer->GetGPUVirtualAddress();
	}
};
