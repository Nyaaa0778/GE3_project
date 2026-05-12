#pragma once

#include "Object3dRenderer.h"

#include <Matrix4x4.h>
#include <Transform.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>

#include <d3d12.h>
#include <string>
#include <wrl.h>

#include "LightingType.h"

class Model;
class Camera;
class Object3dRenderer;

class Object3d {
public:
	//================================================================================
	// 初期化 / 更新 / 描画
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="filePath">モデルファイルのパス</param>
	/// /// <param name="extension">拡張子 (デフォルトは "obj")</param>
	void Initialize(const std::string& filePath, const std::string& extension = "obj");
	/// <summary>
	/// 更新
	/// </summary>
	void Update();
	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

public:
	//================================================================================
	// Getter
	//================================================================================

	// 位置
	const Vector3& GetPosition() const { return position_; }
	// 回転
	const Vector3& GetRotate() const { return rotation_; }
	// 拡縮
	const Vector3& GetScale() const { return scale_; }
	// 色
	const Vector4& GetColor() const;
	// BlendMode
	Object3dRenderer::BlendMode GetBlendMode() const { return blendMode_; }
	// ライティングの種類
	void SetLightingType(LightingType type);

	//// ライトの色
	//const Vector4& GetLightColor() const { return directionalLightData_->color; }
	//// ライトの向き
	//const Vector3& GetLightDirection() const {
	//	return directionalLightData_->direction;
	//}
	//// ライトの輝度
	//float GetLightIntensity() const { return directionalLightData_->intensity; }

	//================================================================================
	// Setter
	//================================================================================

	// 位置
	void SetPosition(const Vector3& position) { position_ = position; }
	// 回転
	void SetRotation(Vector3 rotation) { rotation_ = rotation; }
	// 拡縮
	void SetScale(const Vector3& scale) { scale_ = scale; }
	// 色
	void SetColor(const Vector4& color);
	// BlendMode
	void SetBlendMode(Object3dRenderer::BlendMode blendMode) {
		blendMode_ = blendMode;
	}
	// モデル
	void SetModel(const std::string& modelName, const std::string& extension = "obj");

	float GetEnvironmentCoefficient();

	//// ライトの色
	//void SetLightColor(const Vector4& color) {
	//	directionalLightData_->color = color;
	//}
	//// ライトの向き
	//void SetLightDirection(const Vector3& direction) {
	//	directionalLightData_->direction = direction;
	//}
	//// ライトの輝度
	//void SetLightIntensity(float intensity) {
	//	directionalLightData_->intensity = intensity;
	//}

	// カメラ
	void SetCamera(Camera* camera) { camera_ = camera; }

	void SetEnvironmentTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
		environmentTextureSrvHandleGPU_ = handle;
	}
	void SetEnvironmentCoefficient(float coeff);

private:
	//================================================================================
	// 内部構造体
	//================================================================================

	// 座標変換行列データ
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInverseTranspose;
	};

	//// 平行光源
	//struct DirectionalLight {
	//	Vector4 color;     // ライトの色
	//	Vector3 direction; // ライトの向き
	//	float intensity;   // 輝度
	//};

private:
	//================================================================================
	// 型エイリアス
	//================================================================================

	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
	//================================================================================
	// 外部参照
	//================================================================================

	// Object3dRendererのポインタ
	Object3dRenderer* object3dRenderer_ = nullptr;

	// Modelのポインタ
	Model* model_ = nullptr;

	// Cameraのポインタ
	Camera* camera_ = nullptr;

	//================================================================================
	// GPUリソース（定数バッファ）
	//================================================================================

	// バッファリソース
	ComPtr<ID3D12Resource> transformationMatrixBuffer_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	TransformationMatrix* transformationMatrixData_ = nullptr;

	//// バッファリソース
	//ComPtr<ID3D12Resource> directionalLightBuffer_ = nullptr;
	//// バッファリソースないのデータを指すポインタ
	//DirectionalLight* directionalLightData_ = nullptr;

	//================================================================================
	// Transform
	//================================================================================

	// 3DオブジェクトのTransform
	Transform transform_ {};
	// 位置
	Vector3 position_ = {0.0f, 0.0f, 0.0f};
	// 回転
	Vector3 rotation_ = {0.0f, 0.0f, 0.0f};
	// 拡縮
	Vector3 scale_ = {1.0f, 1.0f, 1.0f};

	// BlendMode
	Object3dRenderer::BlendMode blendMode_ = Object3dRenderer::BlendMode::kNone;

	D3D12_GPU_DESCRIPTOR_HANDLE environmentTextureSrvHandleGPU_ {};

private:
	//================================================================================
	// データ作成処理
	//================================================================================

	/// <summary>
	/// 座標変換行列データの作成
	/// </summary>
	void CreateTransformationMatrixData();
	/*/// <summary>
	/// 平行光源データの作成
	/// </summary>
	void CreateDirectionalLightData();*/
};
