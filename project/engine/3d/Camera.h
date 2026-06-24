#pragma once

#include <Matrix4x4.h>
#include <Transform.h>
#include <d3d12.h>
#include <wrl.h>

class Camera {
public:
	//================================================================================
	// コンストラクタ / 更新
	//================================================================================

	/// <summary>
	/// コンストラクタ
	/// </summary>
	Camera();

	/*/// <summary>
	/// 更新
	/// </summary>
	void Update();*/

	/// <summary>
	/// 行列の計算
	/// </summary>
	void CalculateMatrix();

public:
	struct CameraForGPU {
		Vector3 worldPosition;
	};

	// 定数バッファ作成関数
	void CreateConstantBuffer();

	// GPUアドレス取得用 Getter
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferVideoAddress() const {
		return cameraBuffer_->GetGPUVirtualAddress();
	}

	// 外部から行列を直接設定した際の同期処理
	void UpdateViewProjection();

public:
	// 行列メンバの公開
	Matrix4x4 matWorld;
	Matrix4x4 matView;
	Matrix4x4 matProjection;

private:
	//================================================================================
	// Transform と行列 / 投影パラメータ
	//================================================================================

	Transform transform_;

	// 水平方向視野角
	float fovY_ = 0.45f;
	// アスペクト比
	float aspectRatio_ = 16.0f / 9.0f;
	// ニアクリップ距離
	float nearClip_ = 0.1f;
	// ファークリップ距離
	float farClip_ = 100.0f;

	Matrix4x4 viewProjectionMatrix_;

	// クラスメンバとしてバッファやデータポインタを持つ
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraBuffer_;
	CameraForGPU* cameraData_ = nullptr;

public:
	//================================================================================
	// Getter
	//================================================================================

	// ワールド行列
	const Matrix4x4& GetWorldMatrix() const { return matWorld; }
	// ビュー行列
	const Matrix4x4& GetViewMatrix() const { return matView; }
	// プロジェクション行列
	const Matrix4x4& GetProjectionMatrix() const { return matProjection; }
	// ビュープロジェクション行列
	const Matrix4x4& GetViewProjectionMatrix() const {
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
	void SetRotate(const Vector3& rotation) { transform_.rotation = rotation; }
	// 移動
	void SetTranslate(const Vector3& translation) {
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
