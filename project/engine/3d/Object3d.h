#pragma once

#include "Object3dRenderer.h"
#include "WorldTransform.h"

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
	void Initialize(const std::string& filePath, WorldTransform* worldTransform = nullptr);
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
	const Vector3& GetPosition() const { return worldTransform_.translation; }
	// 回転
	const Vector3& GetRotate() const { return worldTransform_.rotation; }
	// 拡縮
	const Vector3& GetScale() const { return worldTransform_.scale; }
	// 親子関係
	void SetParent(const WorldTransform* parent) { worldTransform_.parent = parent; }
	const WorldTransform* GetParent() const { return worldTransform_.parent; }
	// WorldTransform
	const WorldTransform& GetWorldTransform() const { return worldTransformPtr_ ? *worldTransformPtr_ : worldTransform_; }
	WorldTransform& GetWorldTransform() { return worldTransformPtr_ ? *worldTransformPtr_ : worldTransform_; }
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
	void SetPosition(const Vector3& position) { worldTransform_.translation = position; }
	// 回転
	void SetRotation(Vector3 rotation) { worldTransform_.rotation = rotation; }
	// 拡縮
	void SetScale(const Vector3& scale) { worldTransform_.scale = scale; }
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

	// 平行光源
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
	// WorldTransform
	//================================================================================

	WorldTransform worldTransform_;

	// 外部参照用のWorldTransformへのポインタ
	WorldTransform* worldTransformPtr_ = nullptr;

	// BlendMode
	Object3dRenderer::BlendMode blendMode_ = Object3dRenderer::BlendMode::kNone;

	D3D12_GPU_DESCRIPTOR_HANDLE environmentTextureSrvHandleGPU_ {};

private:
	//================================================================================
	// データ作成処理
	//================================================================================

	/*/// <summary>
	/// 平行光源データの作成
	/// </summary>
	void CreateDirectionalLightData();*/
};
