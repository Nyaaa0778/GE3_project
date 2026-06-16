#include "Cylinder.h"
#include "DirectXCommon.h"
#include "PrimitiveRenderer.h"
#include "TextureManager.h"
#include "Camera.h"
#include "MathUtility.h"
#include "Object3dRenderer.h"
#include "ImGuiManager.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

using namespace MathUtility;

void Cylinder::Initialize(const std::string& textureFilePath)
{
	textureFilePath_ = textureFilePath;

	// カメラの初期設定（指定されていなければデフォルトカメラを使用）
	if (!camera_) {
		camera_ = Object3dRenderer::GetInstance()->GetDefaultCamera();
	}

	// 各種リソースの作成
	CreateMesh();
	CreateMaterialData();
	worldTransform_.Initialize();

	// テクスチャの読み込み（デフォルト）
	TextureManager::GetInstance()->LoadTexture("resources/sprites/" + textureFilePath_);
}

void Cylinder::SetWorldTransform(WorldTransform* worldTransform)
{
	externalWorldTransform_ = worldTransform;
}

void Cylinder::Update()
{
	Matrix4x4 viewProjectionMatrix = camera_ ? camera_->GetViewProjectionMatrix() : MakeIdentityMatrix();

	if (externalWorldTransform_) {
		worldTransform_.matWorld = externalWorldTransform_->matWorld;
		worldTransform_.constMap->World = worldTransform_.matWorld;
		worldTransform_.constMap->WVP = Multiply(worldTransform_.matWorld, viewProjectionMatrix);
		worldTransform_.constMap->WorldInverseTranspose = MakeTransposeMatrix(MakeInverseMatrix(worldTransform_.matWorld));
	} else {
		worldTransform_.UpdateMatrix();
	}

	if (materialData_) {
		// UVなのでZ軸は使わず、XYの移動量だけを行列化します
		materialData_->uvTransform = MakeAffineMatrix(
			{1.0f, 1.0f, 1.0f}, // Scale
			{0.0f, 0.0f, 0.0f}, // Rotation
			{uvTranslation_.x, uvTranslation_.y, 0.0f} // Translation
		);
	}
}

void Cylinder::Draw()
{
	auto primitiveRenderer = PrimitiveRenderer::GetInstance();
	auto commandList = primitiveRenderer->GetDxCommon()->GetCommandList();

	// 共通の描画設定 (BlendModeを指定し、Solid描画を有効化)
	primitiveRenderer->SetupCommonRenderState(blendMode_, false);

	// 頂点バッファとインデックスバッファのセット
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);

	// b0: Material
	commandList->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());

	// b1 (VS): TransformationMatrix
	commandList->SetGraphicsRootConstantBufferView(1, worldTransform_.constBuffer->GetGPUVirtualAddress());

	// t0: Texture
	auto srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("resources/sprites/" + textureFilePath_);
	commandList->SetGraphicsRootDescriptorTable(2, srvHandle);

	// 描画コマンド (インデックスを使用)
	commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}

void Cylinder::SetColor(const Vector4& color)
{
	if (materialData_) {
		materialData_->color = color;
	}
}

void Cylinder::SetTexture(const std::string& textureFilePath)
{
	textureFilePath_ = textureFilePath;
	TextureManager::GetInstance()->LoadTexture("resources/sprites/" + textureFilePath_);
}

void Cylinder::CreateMesh() {
	auto renderer = PrimitiveRenderer::GetInstance();

	// スライス数をキーにしてキャッシュを確認
	std::string geometryKey = "Cylinder_SlideStyle_" + std::to_string(numSlices_);
	auto sharedGeom = renderer->GetSharedGeometry(geometryKey);
	if (sharedGeom) {
		vertexBufferView_ = sharedGeom->vertexBufferView;
		indexBufferView_ = sharedGeom->indexBufferView;
		indexCount_ = sharedGeom->indexCount;
		return;
	}

	auto dxCommon = DirectXCommon::GetInstance();

	// ==========================================
	// 画像のスライドに合わせた定数定義
	// ==========================================
	const uint32_t kCylinderDivide = numSlices_; // スライス数（クラスのメンバに依存させます）
	const float kTopRadius = 1.0f;
	const float kBottomRadius = 1.0f;
	const float kHeight = 3.0f;

	// 画像の std::numbers::pi_v<float> の代わりに、既存定義の M_PI を使います
	const float radianPerDivide = 2.0f * static_cast<float>(M_PI) / static_cast<float>(kCylinderDivide);

	// ==========================================
	// 頂点バッファの生成（側面のみ: 1分割につき6頂点）
	// ==========================================
	const uint32_t kNumVertices = kCylinderDivide * 6;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer = dxCommon->CreateBufferResource(sizeof(VertexData) * kNumVertices);

	vertexBufferView_.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kNumVertices;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	VertexData* vertexData = nullptr;
	vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	uint32_t vIdx = 0;

	for (uint32_t index = 0; index < kCylinderDivide; ++index) {
		float sin = std::sin(index * radianPerDivide);
		float cos = std::cos(index * radianPerDivide);
		float sinNext = std::sin((index + 1) * radianPerDivide);
		float cosNext = std::cos((index + 1) * radianPerDivide);
		float u = float(index) / float(kCylinderDivide);
		float uNext = float(index + 1) / float(kCylinderDivide);

		// position, texcoord, normal (画像通りの順序で代入)

		// 1つ目の三角形
		vertexData[vIdx++] = {{-sin * kTopRadius, kHeight, cos * kTopRadius, 1.0f}, {u, 0.0f}, {-sin, 0.0f, cos}};
		vertexData[vIdx++] = {{-sinNext * kTopRadius, kHeight, cosNext * kTopRadius, 1.0f}, {uNext, 0.0f}, {-sinNext, 0.0f, cosNext}};
		vertexData[vIdx++] = {{-sin * kBottomRadius, 0.0f, cos * kBottomRadius, 1.0f}, {u, 1.0f}, {-sin, 0.0f, cos}};

		// 2つ目の三角形
		vertexData[vIdx++] = {{-sin * kBottomRadius, 0.0f, cos * kBottomRadius, 1.0f}, {u, 1.0f}, {-sin, 0.0f, cos}};
		vertexData[vIdx++] = {{-sinNext * kTopRadius, kHeight, cosNext * kTopRadius, 1.0f}, {uNext, 0.0f}, {-sinNext, 0.0f, cosNext}};
		vertexData[vIdx++] = {{-sinNext * kBottomRadius, 0.0f, cosNext * kBottomRadius, 1.0f}, {uNext, 1.0f}, {-sinNext, 0.0f, cosNext}};
	}

	// ==========================================
	// インデックスバッファの生成
	// ==========================================
	// 既存の描画処理（DrawIndexedInstanced）を崩さずにそのまま使うため、
	// 「0, 1, 2, 3, 4, 5...」という連番のインデックスを作成します。
	const uint32_t kNumIndices = kNumVertices;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer = dxCommon->CreateBufferResource(sizeof(uint32_t) * kNumIndices);

	indexBufferView_.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * kNumIndices;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexData = nullptr;
	indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	for (uint32_t i = 0; i < kNumIndices; ++i) {
		indexData[i] = i;
	}
	indexCount_ = kNumIndices;

	// ==========================================
	// 共有ジオメトリへの登録
	// ==========================================
	PrimitiveRenderer::SharedGeometry newGeom;
	newGeom.vertexBuffer = vertexBuffer;
	newGeom.vertexBufferView = vertexBufferView_;
	newGeom.indexBuffer = indexBuffer;
	newGeom.indexBufferView = indexBufferView_;
	newGeom.indexCount = indexCount_;

	renderer->SetSharedGeometry(geometryKey, newGeom);
}

void Cylinder::CreateMaterialData()
{
	auto dxCommon = DirectXCommon::GetInstance();

	materialBuffer_ = dxCommon->CreateBufferResource(sizeof(Material));
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->uvTransform = MakeIdentityMatrix();
	materialData_->alphaReference = 0.5f;
}

// (座標変換行列データの作成はWorldTransformに移管したため削除)

void Cylinder::DrawImGui(const char* windowName)
{
#ifdef USE_IMGUI
	ImGui::Begin(windowName);

	ImGui::DragFloat3("Position", &worldTransform_.translation.x, 0.1f);
	ImGui::DragFloat3("Rotation", &worldTransform_.rotation.x, 0.01f);
	ImGui::DragFloat3("Scale", &worldTransform_.scale.x, 0.1f);

	static int currentBlendMode = static_cast<int>(blendMode_);
	const char* blendModeNames[] = {"None", "Normal", "Add", "Subtract", "Multiply", "Screen"};
	if (ImGui::Combo("Blend Mode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
		blendMode_ = static_cast<PrimitiveRenderer::BlendMode>(currentBlendMode);
	}

	ImGui::End();
#endif
}
