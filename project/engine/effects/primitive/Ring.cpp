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
	CreateMaterialData();
	CreateTransformationMatrixData();
	CreateMesh();

	// テクスチャの読み込み（デフォルト）
	TextureManager::GetInstance()->LoadTexture("resources/sprites/" + textureFilePath_);
}

void Ring::Update()
{
	if (isMeshDirty_) {
		CreateMesh();
	}

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
	auto dxCommon = DirectXCommon::GetInstance();

	const uint32_t kRingDivide = 32;
	const uint32_t kNumVertices = kRingDivide * 4;
	const uint32_t kNumIndices = kRingDivide * 6;

	// 1. バッファが未作成なら生成する（インスタンス固有のバッファ）
	if (!vertexBuffer_) {
		vertexBuffer_ = dxCommon->CreateBufferResource(sizeof(VertexData) * kNumVertices);
		vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = sizeof(VertexData) * kNumVertices;
		vertexBufferView_.StrideInBytes = sizeof(VertexData);
	}
	if (!indexBuffer_) {
		indexBuffer_ = dxCommon->CreateBufferResource(sizeof(uint32_t) * kNumIndices);
		indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
		indexBufferView_.SizeInBytes = sizeof(uint32_t) * kNumIndices;
		indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
		indexCount_ = kNumIndices;
	}

	// 2. 頂点データの計算
	VertexData* vertexData = nullptr;
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	float totalAngle = endAngle_ - startAngle_;
	float radianPerDivide = totalAngle / float(kRingDivide);

	for (uint32_t index = 0; index < kRingDivide; ++index) {
		// 進行度 (0.0 ～ 1.0)
		float t = float(index) / float(kRingDivide);
		float tNext = float(index + 1) / float(kRingDivide);

		// 実際の角度
		float currentAngle = startAngle_ + index * radianPerDivide;
		float nextAngle = startAngle_ + (index + 1) * radianPerDivide;

		float sin = std::sin(currentAngle);
		float cos = std::cos(currentAngle);
		float sinNext = std::sin(nextAngle);
		float cosNext = std::cos(nextAngle);

		// 【スプライン計算】 2次ベジェ曲線で外径を滑らかに変化させる
		float currentOuterRadius = std::pow(1.0f - t, 2.0f) * startOuterRadius_ +
			2.0f * (1.0f - t) * t * midOuterRadius_ +
			std::pow(t, 2.0f) * endOuterRadius_;

		float nextOuterRadius = std::pow(1.0f - tNext, 2.0f) * startOuterRadius_ +
			2.0f * (1.0f - tNext) * tNext * midOuterRadius_ +
			std::pow(tNext, 2.0f) * endOuterRadius_;

		// 【UV方向の計算】
		float u1, v1, u2, v2, u3, v3, u4, v4;
		if (uvDirection_ == RingUVDirection::Horizon) {
			u1 = t;       v1 = 0.0f; // 外側・現在
			u2 = tNext;   v2 = 0.0f; // 外側・次
			u3 = t;       v3 = 1.0f; // 内側・現在
			u4 = tNext;   v4 = 1.0f; // 内側・次
		} else {
			// Vertical: 半径方向がU、円周方向がV
			u1 = 0.0f; v1 = t;
			u2 = 0.0f; v2 = tNext;
			u3 = 1.0f; v3 = t;
			u4 = 1.0f; v4 = tNext;
		}

		uint32_t vIdx = index * 4;

		// ① 頂点0: 外側・現在
		vertexData[vIdx + 0].position = {sin * currentOuterRadius, cos * currentOuterRadius, 0.0f, 1.0f};
		vertexData[vIdx + 0].texcoord = {u1, v1};
		vertexData[vIdx + 0].normal = {0.0f, 0.0f, -1.0f};

		// ② 頂点1: 外側・次
		vertexData[vIdx + 1].position = {sinNext * nextOuterRadius, cosNext * nextOuterRadius, 0.0f, 1.0f};
		vertexData[vIdx + 1].texcoord = {u2, v2};
		vertexData[vIdx + 1].normal = {0.0f, 0.0f, -1.0f};

		// ③ 頂点2: 内側・現在
		vertexData[vIdx + 2].position = {sin * innerRadius_, cos * innerRadius_, 0.0f, 1.0f};
		vertexData[vIdx + 2].texcoord = {u3, v3};
		vertexData[vIdx + 2].normal = {0.0f, 0.0f, -1.0f};

		// ④ 頂点3: 内側・次
		vertexData[vIdx + 3].position = {sinNext * innerRadius_, cosNext * innerRadius_, 0.0f, 1.0f};
		vertexData[vIdx + 3].texcoord = {u4, v4};
		vertexData[vIdx + 3].normal = {0.0f, 0.0f, -1.0f};
	}
	vertexBuffer_->Unmap(0, nullptr);

	// 3. インデックスデータの計算
	uint32_t* indexData = nullptr;
	indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	for (uint32_t i = 0; i < kRingDivide; ++i) {
		uint32_t vIdx = i * 4;
		uint32_t iIdx = i * 6;

		indexData[iIdx + 0] = vIdx + 0;
		indexData[iIdx + 1] = vIdx + 1;
		indexData[iIdx + 2] = vIdx + 2;

		indexData[iIdx + 3] = vIdx + 1;
		indexData[iIdx + 4] = vIdx + 3;
		indexData[iIdx + 5] = vIdx + 2;
	}
	indexBuffer_->Unmap(0, nullptr);

	// 更新完了
	isMeshDirty_ = false;
}

void Ring::SetAngle(float startAngle, float endAngle) {
	startAngle_ = startAngle;
	endAngle_ = endAngle;
	isMeshDirty_ = true;
}

void Ring::SetRadius(float startOuter, float midOuter, float endOuter, float inner) {
	startOuterRadius_ = startOuter;
	midOuterRadius_ = midOuter;
	endOuterRadius_ = endOuter;
	innerRadius_ = inner;
	isMeshDirty_ = true;
}

void Ring::SetUVDirection(RingUVDirection direction) {
	uvDirection_ = direction;
	isMeshDirty_ = true;
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
	const char* blendModeNames[] = {"None", "Normal", "Add", "Subtract", "Multiply", "Screen"};
	if (ImGui::Combo("Blend Mode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
		blendMode_ = static_cast<PrimitiveRenderer::BlendMode>(currentBlendMode);
	}

	ImGui::Separator();
	ImGui::Text("Ring Extensions");

	// パラメータが変更されたらフラグを立てる
	if (ImGui::DragFloat2("Angle (Start/End)", &startAngle_, 0.05f)) { isMeshDirty_ = true; }
	if (ImGui::DragFloat("Start Outer Radius", &startOuterRadius_, 0.01f)) { isMeshDirty_ = true; }
	if (ImGui::DragFloat("Mid Outer Radius", &midOuterRadius_, 0.01f)) { isMeshDirty_ = true; }
	if (ImGui::DragFloat("End Outer Radius", &endOuterRadius_, 0.01f)) { isMeshDirty_ = true; }
	if (ImGui::DragFloat("Inner Radius", &innerRadius_, 0.01f)) { isMeshDirty_ = true; }

	static int currentUV = static_cast<int>(uvDirection_);
	const char* uvNames[] = {"Horizon", "Vertical"};
	if (ImGui::Combo("UV Direction", &currentUV, uvNames, IM_ARRAYSIZE(uvNames))) {
		uvDirection_ = static_cast<RingUVDirection>(currentUV);
		isMeshDirty_ = true;
	}

	ImGui::End();
#endif
}