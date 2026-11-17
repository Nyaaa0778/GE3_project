#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "MathUtility.h"
#include "Model.h"
#include "ModelManager.h"
#include "Quaternion.h"
#include "TextureManager.h"
#include "TimeManager.h"
#include "LightManager.h"
#include <RootParameter.h>
#include <algorithm>
#include <cmath>

using namespace MathUtility;

//================================================================================
// 初期化 / 更新 / 描画
//================================================================================

/// <summary>
/// 初期化
/// </summary>
/// <param name="modelName">モデル名</param>
	/// /// <param name="extension">拡張子 (デフォルトは "obj")</param>
void Object3d::Initialize(const std::string& modelName, const std::string& extension) {

	object3dRenderer_ = Object3dRenderer::GetInstance();

	// モデルをセット
	SetModel(modelName, extension);

	// 座標変換行列データの初期化
	worldTransform_.Initialize();

	// デフォルトカメラを設定
	camera_ = Object3dRenderer::GetInstance()->GetDefaultCamera();
}
/// <summary>
/// 更新
/// </summary>
void Object3d::Update(WorldTransform* worldTransform) {

	WorldTransform* wt = worldTransform;
	if (!wt) {
		wt = externalWorldTransform_ ? externalWorldTransform_ : &worldTransform_;
	}

	Matrix4x4 viewProjectionMatrix = camera_ ? camera_->GetViewProjectionMatrix() : MakeIdentityMatrix();

	if (!worldTransform && !externalWorldTransform_) {
		worldTransform_.UpdateMatrix();
	}

	Matrix4x4 worldViewProjectionMatrix = wt->matWorld * viewProjectionMatrix;

	if (wt->constMap) {
		if (model_) {
			Matrix4x4 localMatrix = model_->GetModelData().rootNode.localMatrix;

			if (hasAnimation_) {
				if (isAnimationPlaying_) {
					animationTime_ += TimeManager::GetInstance()->GetDeltaTime();
				}
				if (animation_.duration > 0.0f) {
					if (isAnimationLoop_) {
						animationTime_ = std::fmod(animationTime_, animation_.duration);
					} else {
						animationTime_ = std::min(animationTime_, animation_.duration);
					}
				}

				auto it = animation_.nodeAnimations.find(model_->GetModelData().rootNode.name);
				if (it != animation_.nodeAnimations.end()) {
					const NodeAnimation& rootNodeAnimation = it->second;
					Vector3 translate = rootNodeAnimation.translate.keyframes.empty()
						? Vector3{ 0.0f, 0.0f, 0.0f }
						: CalculateValue(rootNodeAnimation.translate, animationTime_);
					Quaternion rotate = rootNodeAnimation.rotate.keyframes.empty()
						? Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f }
						: CalculateValue(rootNodeAnimation.rotate, animationTime_);
					Vector3 scale = rootNodeAnimation.scale.keyframes.empty()
						? Vector3{ 1.0f, 1.0f, 1.0f }
						: CalculateValue(rootNodeAnimation.scale, animationTime_);
					localMatrix = MakeAffineMatrix(scale, rotate, translate);
				}
			}

			wt->constMap->WVP = localMatrix * worldViewProjectionMatrix;
			wt->constMap->World = localMatrix * wt->matWorld;
			wt->constMap->WorldInverseTranspose = MakeTransposeMatrix(MakeInverseMatrix(wt->constMap->World));
		} else {
			wt->constMap->WVP = worldViewProjectionMatrix;
			wt->constMap->World = wt->matWorld;
			wt->constMap->WorldInverseTranspose = MakeTransposeMatrix(MakeInverseMatrix(wt->constMap->World));
		}
	}
}
/// <summary>
/// 描画
/// </summary>
void Object3d::Draw(WorldTransform* worldTransform) {

	object3dRenderer_->SetupCommonRenderState(blendMode_);

	WorldTransform* wt = worldTransform;
	if (!wt) {
		wt = externalWorldTransform_ ? externalWorldTransform_ : &worldTransform_;
	}

	// 座標変換行列のCBufferの場所を設定
	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			kRootParamTransform, wt->constBuffer->GetGPUVirtualAddress());

	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			kRootParamCamera, camera_->GetConstantBufferVideoAddress());

	// ライトCBufferの場所を設定
	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			kRootParamLight, LightManager::GetInstance()->GetConstantBufferVideoAddress());

	if (environmentTextureSrvHandleGPU_.ptr != 0) {
		object3dRenderer_->GetDxCommon()->GetCommandList()
			->SetGraphicsRootDescriptorTable(kRootParamEnvironmentOrExtra, environmentTextureSrvHandleGPU_);
	}

	// 3Dモデルが割り当てられていれば描画する
	if (model_) {
		// ディゾルブ用パラメータをマテリアルに適用
		model_->SetDissolveEnabled(isDissolveEnabled_);
		model_->SetDissolveThreshold(dissolveThreshold_);
		model_->SetDissolveEdgeWidth(dissolveEdgeWidth_);
		model_->SetDissolveEdgeColor(dissolveEdgeColor_);

		// ディゾルブマスク用テクスチャのバインド（レジスタt2 -> スロット6）
		if (isDissolveEnabled_ && !dissolveMaskTexturePath_.empty()) {
			auto srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU(dissolveMaskTexturePath_);
			if (srvHandle.ptr != 0) {
				object3dRenderer_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(kRootParamDissolveMask, srvHandle);
			}
		} else {
			// ディゾルブ無効時も、何かダミーテクスチャを割り当てておく（未割り当てによるバグ防止）
			auto dummyHandle = TextureManager::GetInstance()->GetSrvHandleGPU(model_->GetModelData().material.textureFilePath);
			if (dummyHandle.ptr != 0) {
				object3dRenderer_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(kRootParamDissolveMask, dummyHandle);
			}
		}

		model_->Draw();
	}
}

//================================================================================
// Getter
//================================================================================

// 色
const Vector4& Object3d::GetColor() const { return model_->GetColor(); }

float Object3d::GetEnvironmentCoefficient() {
	return model_->GetEnvironmentCofficient();
}

//================================================================================
// Setter
//================================================================================

// 色
void Object3d::SetColor(const Vector4& color) { model_->SetColor(color); }
// モデル
void Object3d::SetModel(const std::string& modelName, const std::string& extension) {
	auto modelManager = ModelManager::GetInstance();

	// ★ ModelManager::LoadModel に extension を渡す
	modelManager->LoadModel(modelName, extension);

	// 読み込んだモデルを検索してセット
	model_ = modelManager->FindModel(modelName);
	animationTime_ = 0.0f;
}

void Object3d::SetEnvironmentCoefficient(float coeff) {
	if (model_) {
		model_->SetEnvironmentCoefficient(coeff);
	}
}

// ライティングの種類
void Object3d::SetLightingType(LightingType type) {
	return model_->SetLightingType(type);
}