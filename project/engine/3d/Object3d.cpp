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
void Object3d::Initialize(const std::string& modelName) {

	object3dRenderer_ = Object3dRenderer::GetInstance();

	// モデルをセット
	SetModel(modelName);

	// 座標変換行列データの作成
	CreateTransformationMatrixData();
	//// 平行光源データの作成
	//CreateDirectionalLightData();

	// Transform変数を作成
	transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	// デフォルトカメラを設定
	camera_ = Object3dRenderer::GetInstance()->GetDefaultCamera();
}
/// <summary>
/// 更新
/// </summary>
void Object3d::Update() {

	transform_.scale = {scale_.x, scale_.y, scale_.z};
	transform_.rotation = {rotation_.x, rotation_.y, rotation_.z};
	transform_.translation = {position_.x, position_.y, position_.z};

	// transformからworldMatrixを作成
	Matrix4x4 worldMatrix = MakeAffineMatrix(
		transform_.scale, transform_.rotation, transform_.translation);

	Matrix4x4 worldViewProjectionMatrix;

	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	transformationMatrixData_->WVP = worldViewProjectionMatrix;
	transformationMatrixData_->World = worldMatrix;

	transformationMatrixData_->WorldInverseTranspose = MakeTransposeMatrix(MakeInverseMatrix(worldMatrix));
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
			1, transformationMatrixBuffer_->GetGPUVirtualAddress());

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
void Object3d::CreateTransformationMatrixData() {
	// 座標変換行列リソースを作成
	transformationMatrixBuffer_ =
		object3dRenderer_->GetDxCommon()->CreateBufferResource(
			sizeof(TransformationMatrix));

	// transformationMatrixResourceに座標変換行列データを書き込む
	transformationMatrixBuffer_->Map(
		0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	// 単位行列を書き込んでおく
	transformationMatrixData_->WVP = MakeIdentityMatrix();
	transformationMatrixData_->World = MakeIdentityMatrix();

	transformationMatrixData_->WorldInverseTranspose = MakeIdentityMatrix();
}
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

//================================================================================
// Setter
//================================================================================

// 色
void Object3d::SetColor(const Vector4& color) { model_->SetColor(color); }
// モデル
void Object3d::SetModel(const std::string& modelName) {
	auto modelManager = ModelManager::GetInstance();

	// モデルファイルを読み込む
	modelManager->LoadModel(modelName);
	// モデルの検索
	model_ = modelManager->FindModel(modelName);
	assert(model_);
}

// ライティングの種類
void Object3d::SetLightingType(LightingType type) {
	return model_->SetLightingType(type);
}