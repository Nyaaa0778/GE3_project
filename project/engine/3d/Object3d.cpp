#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "MathUtility.h"
#include "Model.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "LightManager.h"

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

	// WorldTransform の初期化
	worldTransform_.Initialize();

	// デフォルトカメラを設定
	camera_ = Object3dRenderer::GetInstance()->GetDefaultCamera();
}
/// <summary>
/// 更新
/// </summary>
void Object3d::Update() {
	// WorldTransformの行列更新
	worldTransform_.UpdateMatrix();

	// WVP（World-View-Projection）の計算
	Matrix4x4 worldViewProjectionMatrix;
	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = worldTransform_.matWorld * viewProjectionMatrix;
	} else {
		worldViewProjectionMatrix = worldTransform_.matWorld;
	}

	// モデルのrootNodeのローカル行列がある場合を考慮してWVPとWorldを書き込み
	if (model_) {
		worldTransform_.constMap->WVP = model_->GetModelData().rootNode.localMatrix * worldViewProjectionMatrix;
		worldTransform_.constMap->World = model_->GetModelData().rootNode.localMatrix * worldTransform_.matWorld;
	} else {
		worldTransform_.constMap->WVP = worldViewProjectionMatrix;
		worldTransform_.constMap->World = worldTransform_.matWorld;
	}
}
/// <summary>
/// 描画
/// </summary>
void Object3d::Draw() {

	object3dRenderer_->SetupCommonRenderState();

	// 座標変換行列のCBufferの場所を設定
	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			1, worldTransform_.constBuffer->GetGPUVirtualAddress());

	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			2, camera_->GetConstantBufferVideoAddress());

	// 平行光源CBufferの場所を設定
	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			3, LightManager::GetInstance()->GetDirectionalLightConstantBufferVideoAddress());

	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			5, LightManager::GetInstance()->GetLocalLightConstantBufferVideoAddress());

	object3dRenderer_->GetDxCommon()->GetCommandList()
		->SetGraphicsRootDescriptorTable(6, environmentTextureSrvHandleGPU_);

	// 3Dモデルが割り当てられていれば描画する
	if (model_) {
		model_->Draw();
	}
}

//================================================================================
// データ作成処理
//================================================================================

/// <summary>
/// 座標変換行列データの作成
/// </summary>
// (CreateTransformationMatrixData 削除済み)


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