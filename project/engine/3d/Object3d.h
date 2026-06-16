#pragma once

#include "Object3dRenderer.h"

#include <Matrix4x4.h>
#include "WorldTransform.h"
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>

struct ModelData;

#include <d3d12.h>
#include <string>
#include <wrl.h>

#include "LightingType.h"
#include "Model.h"

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
	/// <summary>
	/// 更新
	/// </summary>
	void Update(WorldTransform* worldTransform = nullptr);
	/// <summary>
	/// 描画
	/// </summary>
	void Draw(WorldTransform* worldTransform = nullptr);

public:
	//================================================================================
	// Getter
	//================================================================================

	// 位置
	const Vector3& GetPosition() const {
		return externalWorldTransform_ ? externalWorldTransform_->translation : worldTransform_.translation;
	}
	// 回転
	const Vector3& GetRotate() const {
		return externalWorldTransform_ ? externalWorldTransform_->rotation : worldTransform_.rotation;
	}
	// 拡縮
	const Vector3& GetScale() const {
		return externalWorldTransform_ ? externalWorldTransform_->scale : worldTransform_.scale;
	}
	// 色
	const Vector4& GetColor() const;
	// BlendMode
	Object3dRenderer::BlendMode GetBlendMode() const { return blendMode_; }
	// ライティングの種類
	void SetLightingType(LightingType type);

	// 親子関係の設定
	const WorldTransform* GetWorldTransform() const { return externalWorldTransform_ ? externalWorldTransform_ : &worldTransform_; }
	void SetParent(const WorldTransform* parent) {
		if (externalWorldTransform_) {
			const_cast<WorldTransform*>(externalWorldTransform_)->parent = parent;
		} else {
			worldTransform_.parent = parent;
		}
	}
	// 外部トランスフォームの設定
	void SetWorldTransform(WorldTransform* worldTransform) { externalWorldTransform_ = worldTransform; }

	//================================================================================
	// Setter
	//================================================================================

	// 位置
	void SetPosition(const Vector3& position) {
		if (externalWorldTransform_) {
			externalWorldTransform_->translation = position;
		} else {
			worldTransform_.translation = position;
		}
	}
	// 回転
	void SetRotation(Vector3 rotation) {
		if (externalWorldTransform_) {
			externalWorldTransform_->rotation = rotation;
		} else {
			worldTransform_.rotation = rotation;
		}
	}
	// 拡縮
	void SetScale(const Vector3& scale) {
		if (externalWorldTransform_) {
			externalWorldTransform_->scale = scale;
		} else {
			worldTransform_.scale = scale;
		}
	}
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

	// (座標変換行列データはWorldTransformに移管)

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
	// GPUリソース（定数バッファ）と座標
	//================================================================================

	// ワールド変換データ
	WorldTransform worldTransform_;
	// 外部参照用ワールド変換データ（指定がある場合はこちらを優先して使用）
	WorldTransform* externalWorldTransform_ = nullptr;

	// BlendMode
	Object3dRenderer::BlendMode blendMode_ = Object3dRenderer::BlendMode::kNone;

	D3D12_GPU_DESCRIPTOR_HANDLE environmentTextureSrvHandleGPU_ {};

private:
	//================================================================================
	// データ作成処理
	//================================================================================

	// (初期化はworldTransform_.Initialize()にて行うため削除)
	/*/// <summary>
	/// 平行光源データの作成
	/// </summary>
	void CreateDirectionalLightData();*/
};
