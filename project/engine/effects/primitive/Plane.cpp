#include "Plane.h"
#include "DirectXCommon.h"
#include "PrimitiveRenderer.h"
#include "TextureManager.h"
#include "Camera.h"
#include "MathUtility.h"
#include "Object3dRenderer.h" // GetDefaultCamera()を取得するための一時的な対応
#include "ImGuiManager.h"

using namespace MathUtility;

void Plane::Initialize(const std::string& textureFilePath)
{
	textureFilePath_ = textureFilePath;

	// カメラの初期設定（指定されていなければデフォルトカメラを使用）
	if (!camera_) {
		camera_ = Object3dRenderer::GetInstance()->GetDefaultCamera();
	}

	// 各種リソースの作成
	CreateMesh();
	CreateMaterialData();
	CreateTransformationMatrixData();

	// テクスチャの読み込み（デフォルト）
	TextureManager::GetInstance()->LoadTexture("resources/sprites/" + textureFilePath_);
}

void Plane::Update()
{
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
}

void Plane::Draw()
{
	auto primitiveRenderer = PrimitiveRenderer::GetInstance();
	auto commandList = primitiveRenderer->GetDxCommon()->GetCommandList();

	// 共通の描画設定 (BlendModeを指定)
	primitiveRenderer->SetupCommonRenderState(blendMode_, false);

	// 頂点バッファとインデックスバッファのセット
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);

	// b0: Material
	commandList->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());

	// b1 (VS): TransformationMatrix
	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixBuffer_->GetGPUVirtualAddress());

	// t0: Texture
	auto srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("resources/sprites/" + textureFilePath_);
	commandList->SetGraphicsRootDescriptorTable(2, srvHandle);

	// 描画コマンド (インデックスを使用)
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Plane::SetColor(const Vector4& color)
{
	if (materialData_) {
		materialData_->color = color;
	}
}

void Plane::SetTexture(const std::string& textureFilePath)
{
	textureFilePath_ = textureFilePath;
	TextureManager::GetInstance()->LoadTexture("resources/sprites/" + textureFilePath_);
}

void Plane::CreateMesh()
{
	auto dxCommon = DirectXCommon::GetInstance();

	// --- 頂点バッファの作成 ---
	const uint32_t kNumVertices = 4;
	vertexBuffer_ = dxCommon->CreateBufferResource(sizeof(VertexData) * kNumVertices);
	
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kNumVertices;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	// Planeの頂点データ（ローカル座標系）
	// Z軸のマイナス方向を向くPlaneとする
	// v0 (左上), v1 (右上), v2 (左下), v3 (右下)
	vertexData_[0].position = {-1.0f, 1.0f, 0.0f, 1.0f};
	vertexData_[0].texcoord = {0.0f, 0.0f};
	vertexData_[0].normal = {0.0f, 0.0f, -1.0f};

	vertexData_[1].position = {1.0f, 1.0f, 0.0f, 1.0f};
	vertexData_[1].texcoord = {1.0f, 0.0f};
	vertexData_[1].normal = {0.0f, 0.0f, -1.0f};

	vertexData_[2].position = {-1.0f, -1.0f, 0.0f, 1.0f};
	vertexData_[2].texcoord = {0.0f, 1.0f};
	vertexData_[2].normal = {0.0f, 0.0f, -1.0f};

	vertexData_[3].position = {1.0f, -1.0f, 0.0f, 1.0f};
	vertexData_[3].texcoord = {1.0f, 1.0f};
	vertexData_[3].normal = {0.0f, 0.0f, -1.0f};

	// --- インデックスバッファの作成 ---
	const uint32_t kNumIndices = 6;
	indexBuffer_ = dxCommon->CreateBufferResource(sizeof(uint32_t) * kNumIndices);

	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * kNumIndices;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
	
	// 三角形ポリゴン2枚
	indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
	indexData_[3] = 2; indexData_[4] = 1; indexData_[5] = 3;

	indexCount_ = kNumIndices;
}

void Plane::CreateMaterialData()
{
	auto dxCommon = DirectXCommon::GetInstance();

	materialBuffer_ = dxCommon->CreateBufferResource(sizeof(Material));
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->uvTransform = MakeIdentityMatrix();
}

void Plane::CreateTransformationMatrixData()
{
	auto dxCommon = DirectXCommon::GetInstance();

	transformationMatrixBuffer_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	transformationMatrixBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	transformationMatrixData_->WVP = MakeIdentityMatrix();
}

void Plane::DrawImGui(const char* windowName) {
#ifdef USE_IMGUI
	ImGui::Begin(windowName);

	ImGui::DragFloat3("Position", &position_.x, 0.1f);
	ImGui::DragFloat3("Rotation", &rotation_.x, 0.01f);
	ImGui::DragFloat3("Scale", &scale_.x, 0.1f);

	static int currentBlendMode = static_cast<int>(blendMode_);
	const char* blendModeNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
	if (ImGui::Combo("Blend Mode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
		blendMode_ = static_cast<PrimitiveRenderer::BlendMode>(currentBlendMode);
	}

	ImGui::End();
#endif
}

