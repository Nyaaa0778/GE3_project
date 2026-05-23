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
void Object3d::Initialize(const std::string& filePath, WorldTransform* worldTransform) {

	object3dRenderer_ = Object3dRenderer::GetInstance();

	// filePath からモデル名（ベース名）と拡張子をパース（.gltf や .obj などの拡張子を除去）
	std::string modelName = filePath;
	std::string extension = "obj"; // デフォルト拡張子

	size_t dotPos = filePath.find_last_of('.');
	if (dotPos != std::string::npos) {
		modelName = filePath.substr(0, dotPos);
		extension = filePath.substr(dotPos + 1);
	}

	// モデルをセット
	SetModel(modelName, extension);

	// WorldTransformの紐付けと初期化
	worldTransformPtr_ = worldTransform;
	if (!worldTransformPtr_) {
		worldTransform_.Initialize();
	}

	// デフォルトカメラを設定
	camera_ = Object3dRenderer::GetInstance()->GetDefaultCamera();
}
/// <summary>
/// 更新
/// </summary>
void Object3d::Update() {

	// 使用する WorldTransform を決定（外部参照があればそれを使う）
	WorldTransform* activeTransform = worldTransformPtr_ ? worldTransformPtr_ : &worldTransform_;

	// 外部参照ポインタがない場合のみ、自動で行列を更新する
	// 外部参照がある場合は、ユーザーが好きなタイミングで UpdateMatrix() を呼ぶ前提
	if (!worldTransformPtr_) {
		activeTransform->UpdateMatrix();
	}

	Matrix4x4 worldMatrix = activeTransform->matWorld;
	Matrix4x4 worldViewProjectionMatrix;

	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	// モデルのローカル行列がある場合を考慮して、最終的なWVPとWorld行列を書き込む
	if (activeTransform->constMap && model_) {
		activeTransform->constMap->WVP = model_->GetModelData().rootNode.localMatrix * worldViewProjectionMatrix;
		activeTransform->constMap->World = model_->GetModelData().rootNode.localMatrix * worldMatrix;
		activeTransform->constMap->WorldInverseTranspose = MakeTransposeMatrix(MakeInverseMatrix(activeTransform->constMap->World));
	}
}
/// <summary>
/// 描画
/// </summary>
void Object3d::Draw() {

	object3dRenderer_->SetupCommonRenderState();

	// 使用する WorldTransform を決定
	WorldTransform* activeTransform = worldTransformPtr_ ? worldTransformPtr_ : &worldTransform_;

	// 座標変換行列のCBufferの場所を設定 (activeTransform を使用)
	object3dRenderer_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			1, activeTransform->GetGPUVirtualAddress());

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