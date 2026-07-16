#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "MathUtility.h"
#include "Model.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "LightManager.h"
#include "DevelopEditor.h"

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
			wt->constMap->WVP = model_->GetModelData().rootNode.localMatrix * worldViewProjectionMatrix;
			wt->constMap->World = model_->GetModelData().rootNode.localMatrix * wt->matWorld;
		} else {
			wt->constMap->WVP = worldViewProjectionMatrix;
			wt->constMap->World = wt->matWorld;
		}
	}
}
/// <summary>
/// 描画
/// </summary>
void Object3d::Draw(WorldTransform* worldTransform) {

	object3dRenderer_->SetupCommonRenderState();

	WorldTransform* wt = worldTransform;
	if (!wt) {
		wt = externalWorldTransform_ ? externalWorldTransform_ : &worldTransform_;
	}

	// 一時停止中で、かつエディタモードのときは、カメラ移動に合わせて描画情報を更新する
	if (DevelopEditor::GetInstance()->IsPaused() && DevelopEditor::GetInstance()->IsEditorMode()) {
		Update(worldTransform);
	}

	// 座標変換行列のCBufferの場所を設定
	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			1, wt->constBuffer->GetGPUVirtualAddress());

	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			2, camera_->GetConstantBufferVideoAddress());

	// ライトCBufferの場所を設定
	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			3, LightManager::GetInstance()->GetConstantBufferVideoAddress());

	object3dRenderer_->GetDxCommon()->GetCommandList()
		->SetGraphicsRootDescriptorTable(5, environmentTextureSrvHandleGPU_);

	// 3Dモデルが割り当てられていれば描画する
	if (model_) {
		model_->Draw();
	}
}

//================================================================================
// データ作成処理
//================================================================================

// (座標変換行列データの作成はWorldTransformに移管したため削除)
///// <summary>
///// 平行光源データの作成
///// </summary>
//void Object3d::CreateDirectionalLightData() {
//	// 平行光源リソースを作成
//	directionalLightBuffer_ =
//		object3dRenderer_->GetDxCommon()->CreateBufferResource(
//			sizeof(DirectionalLight));
//
//	// directionalLightResourceに平行光源データを書き込む
//	directionalLightBuffer_->Map(
//		0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
//
//	// 平行光源データの初期値を書き込む
//	directionalLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
//	directionalLightData_->direction = {0.0f, -1.0f, 0.5f};
//	directionalLightData_->intensity = 1.5f;
//}

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