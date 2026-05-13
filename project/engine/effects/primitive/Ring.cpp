#include "Ring.h"
#include "DirectXCommon.h"
#include "PrimitiveRenderer.h"
#include "TextureManager.h"
#include "Camera.h"
#include "MathUtility.h"
#include "Object3dRenderer.h" // GetDefaultCamera()を取得するための一時的な対応
#include "ImGuiManager.h"
#include <cmath>

using namespace MathUtility;

void Ring::Initialize(const std::string& textureFilePath)
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

void Ring::Update()
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

void Ring::Draw()
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
	commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}

void Ring::SetColor(const Vector4& color)
{
	if (materialData_) {
		materialData_->color = color;
	}
}

void Ring::SetTexture(const std::string& textureFilePath)
{
	textureFilePath_ = textureFilePath;
	TextureManager::GetInstance()->LoadTexture("resources/sprites/" + textureFilePath_);
}

void Ring::CreateMesh()
{
	auto renderer = PrimitiveRenderer::GetInstance();

	// 1. すでにRingのメッシュが作られて保管されているかチェック
	auto sharedGeom = renderer->GetSharedGeometry("Ring");
	if (sharedGeom) {
		// すでに存在する場合は、ビュー（参照情報）だけをコピーして終了（高速化）
		vertexBufferView_ = sharedGeom->vertexBufferView;
		indexBufferView_ = sharedGeom->indexBufferView;
		indexCount_ = sharedGeom->indexCount;
		return;
	}

	// 2. まだ作られていない場合（最初の1個目）はバッファを生成する
	auto dxCommon = DirectXCommon::GetInstance();

	const uint32_t kRingDivide = 32; // スライドに合わせ変数名変更（機能はkNumSegmentsと同じ）
	const uint32_t kNumVertices = kRingDivide * 4;

	// --- 頂点バッファの作成 ---
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer = dxCommon->CreateBufferResource(sizeof(VertexData) * kNumVertices);

	vertexBufferView_.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kNumVertices;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	VertexData* vertexData = nullptr;
	vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// スライドの設定値
	const float kOuterRadius = 1.0f;
	const float kInnerRadius = 0.2f; // 元の0.5fからスライドの0.2fに変更
	const float PI = 3.141592654f;
	const float radianPerDivide = 2.0f * PI / float(kRingDivide);

	// リングの頂点データ作成
	for (uint32_t index = 0; index < kRingDivide; ++index) {

		// スライド通りのサイン・コサイン計算（12時方向スタートで時計回りになる計算）
		float sin = std::sin(index * radianPerDivide);
		float cos = std::cos(index * radianPerDivide);
		float sinNext = std::sin((index + 1) * radianPerDivide);
		float cosNext = std::cos((index + 1) * radianPerDivide);

		// UV座標（円周に沿ってテクスチャを巻く形）
		float u = float(index) / float(kRingDivide);
		float uNext = float(index + 1) / float(kRingDivide);

		// インデックスオフセット
		uint32_t vIdx = index * 4;

		// ① 頂点0: 外側・現在
		vertexData[vIdx + 0].position = {sin * kOuterRadius, cos * kOuterRadius, 0.0f, 1.0f};
		vertexData[vIdx + 0].texcoord = {u, 0.0f};
		vertexData[vIdx + 0].normal = {0.0f, 0.0f, -1.0f};

		// ② 頂点1: 外側・次
		vertexData[vIdx + 1].position = {sinNext * kOuterRadius, cosNext * kOuterRadius, 0.0f, 1.0f};
		vertexData[vIdx + 1].texcoord = {uNext, 0.0f};
		vertexData[vIdx + 1].normal = {0.0f, 0.0f, -1.0f};

		// ③ 頂点2: 内側・現在
		vertexData[vIdx + 2].position = {sin * kInnerRadius, cos * kInnerRadius, 0.0f, 1.0f};
		vertexData[vIdx + 2].texcoord = {u, 1.0f};
		vertexData[vIdx + 2].normal = {0.0f, 0.0f, -1.0f};

		// ④ 頂点3: 内側・次
		vertexData[vIdx + 3].position = {sinNext * kInnerRadius, cosNext * kInnerRadius, 0.0f, 1.0f};
		vertexData[vIdx + 3].texcoord = {uNext, 1.0f};
		vertexData[vIdx + 3].normal = {0.0f, 0.0f, -1.0f};
	}

	// --- インデックスバッファの作成 ---
	const uint32_t kNumIndices = kRingDivide * 6;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer = dxCommon->CreateBufferResource(sizeof(uint32_t) * kNumIndices);

	indexBufferView_.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * kNumIndices;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexData = nullptr;
	indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	for (uint32_t i = 0; i < kRingDivide; ++i) {
		uint32_t vIdx = i * 4;
		uint32_t iIdx = i * 6;

		// 時計回りになるようにインデックスを設定 (Direct3Dのデフォルトのカリングに対応)
		// スライドの番号に対応: 0(①), 1(②), 2(③), 3(④)

		// 三角形1: ① -> ② -> ③
		indexData[iIdx + 0] = vIdx + 0;
		indexData[iIdx + 1] = vIdx + 1;
		indexData[iIdx + 2] = vIdx + 2;

		// 三角形2: ② -> ④ -> ③
		indexData[iIdx + 3] = vIdx + 1;
		indexData[iIdx + 4] = vIdx + 3;
		indexData[iIdx + 5] = vIdx + 2;
	}

	indexCount_ = kNumIndices;

	// 3. 生成したバッファをSharedGeometryにまとめ、Rendererに保管してもらう
	PrimitiveRenderer::SharedGeometry newGeom;
	newGeom.vertexBuffer = vertexBuffer;
	newGeom.vertexBufferView = vertexBufferView_;
	newGeom.indexBuffer = indexBuffer;
	newGeom.indexBufferView = indexBufferView_;
	newGeom.indexCount = indexCount_;

	renderer->SetSharedGeometry("Ring", newGeom);
}

void Ring::CreateMaterialData()
{
	auto dxCommon = DirectXCommon::GetInstance();

	materialBuffer_ = dxCommon->CreateBufferResource(sizeof(Material));
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->uvTransform = MakeIdentityMatrix();
}

void Ring::CreateTransformationMatrixData()
{
	auto dxCommon = DirectXCommon::GetInstance();

	transformationMatrixBuffer_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	transformationMatrixBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	transformationMatrixData_->WVP = MakeIdentityMatrix();
}

void Ring::DrawImGui(const char* windowName) {
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
