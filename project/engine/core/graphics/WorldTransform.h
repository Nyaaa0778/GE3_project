#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <Vector3.h>
#include <Matrix4x4.h>

class Camera;

// 座標変換行列データ（GPU送信用）
struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Matrix4x4 WorldInverseTranspose;
};

struct WorldTransform {
	// ローカル座標
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector3 rotation = { 0.0f, 0.0f, 0.0f };
	Vector3 translation = { 0.0f, 0.0f, 0.0f };
	// ローカル・ワールド変換行列
	Matrix4x4 matWorld;
	// 親となるワールド変換へのポインタ
	const WorldTransform* parent = nullptr;

	// 定数バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer;
	// マッピング済みデータのアドレス
	TransformationMatrix* constMap = nullptr;

	Camera* camera_ = nullptr;

	// 初期化
	void Initialize();
	// 行列の更新
	void UpdateMatrix();
	// ワールド座標の取得
	Vector3 GetWorldPosition() const;
};
