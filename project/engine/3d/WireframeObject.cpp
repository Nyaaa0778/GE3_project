#include "WireframeObject.h"

#include <cmath>
#include <numbers>

#include "WireframeRenderer.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "MathUtility.h"

using namespace MathUtility;

WireframeObject::~WireframeObject() {
	if (mappedVertexData_ && vertexBuffer_) {
		vertexBuffer_->Unmap(0, nullptr);
		mappedVertexData_ = nullptr;
	}
	if (materialData_ && colorResource_) {
		colorResource_->Unmap(0, nullptr);
		materialData_ = nullptr;
	}
}

void WireframeObject::Initialize() {
	wireframeRenderer_ = WireframeRenderer::GetInstance();

	// 座標変換データの初期化
	transform_.Initialize();
	// デフォルトカメラの取得（設定されていれば）
	camera_ = wireframeRenderer_->GetDefaultCamera();
	// マテリアル用定数バッファの作成 (PS b1)
	auto dxCommon = DirectXCommon::GetInstance();
	colorResource_ = dxCommon->CreateBufferResource(sizeof(MaterialData));
	colorResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	// デフォルトカラー（白）、デフォルトはフォグなし (fogFar <= fogNear)
	if (materialData_)
	{
		materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		materialData_->fogNear = 0.0f;
		materialData_->fogFar = 0.0f;
		materialData_->pad[0] = 0.0f;
		materialData_->pad[1] = 0.0f;
	}
}
void WireframeObject::Update() {
	// 行列の更新
	transform_.UpdateMatrix();
	// カメラのViewProjection行列を取得
	Matrix4x4 viewProjectionMatrix = camera_ ? camera_->GetViewProjectionMatrix() : MakeIdentityMatrix();
	// WVP行列を計算して定数バッファに転送
	if (transform_.constMap)
	{
		transform_.constMap->World = transform_.matWorld;
		transform_.constMap->WVP = transform_.matWorld * viewProjectionMatrix;
	}
}
void WireframeObject::Draw() {
	if (indexCount_ == 0 || !vertexBuffer_ || !indexBuffer_)
	{
		return;
	}

	// 共通描画設定（シーン側で呼ぶ必要をなくす）
	wireframeRenderer_->SetupCommonRenderState();

	auto commandList = DirectXCommon::GetInstance()->GetCommandList();
	// 頂点バッファ・インデックスバッファをセット
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	// b0: TransformMatrix
	commandList->SetGraphicsRootConstantBufferView(0, transform_.constBuffer->GetGPUVirtualAddress());
	// b1: Material (Color + Fog)
	commandList->SetGraphicsRootConstantBufferView(1, colorResource_->GetGPUVirtualAddress());
	// 描画実行
	commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}
void WireframeObject::SetColor(const Vector4& color) {
	if (materialData_)
	{
		materialData_->color = color;
	}
}
void WireframeObject::SetFog(float fogNear, float fogFar) {
	if (materialData_)
	{
		materialData_->fogNear = fogNear;
		materialData_->fogFar = fogFar;
	}
}
void WireframeObject::CreateMeshBuffers(const std::vector<Vector4>& vertices, const std::vector<uint32_t>& indices) {
	if (mappedVertexData_ && vertexBuffer_) {
		vertexBuffer_->Unmap(0, nullptr);
		mappedVertexData_ = nullptr;
		maxVertexCount_ = 0;
	}

	auto dxCommon = DirectXCommon::GetInstance();
	// --- 頂点バッファ作成 ---
	size_t vertexBufferSize = sizeof(Vector4) * vertices.size();
	vertexBuffer_ = dxCommon->CreateBufferResource(vertexBufferSize);
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	vertexBufferView_.StrideInBytes = sizeof(Vector4);
	Vector4* vertexData = nullptr;
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, vertices.data(), vertexBufferSize);
	// --- インデックスバッファ作成 ---
	size_t indexBufferSize = sizeof(uint32_t) * indices.size();
	indexBuffer_ = dxCommon->CreateBufferResource(indexBufferSize);
	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast<UINT>(indexBufferSize);
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	uint32_t* indexData = nullptr;
	indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData, indices.data(), indexBufferSize);
	indexCount_ = static_cast<uint32_t>(indices.size());
}

void WireframeObject::CreateDynamicLineMesh(uint32_t maxLines) {
	if (mappedVertexData_ && vertexBuffer_) {
		vertexBuffer_->Unmap(0, nullptr);
		mappedVertexData_ = nullptr;
	}

	maxVertexCount_ = maxLines * 2;
	auto dxCommon = DirectXCommon::GetInstance();

	// 頂点バッファ作成 (Uploadヒープで常時マップ可能)
	size_t vertexBufferSize = sizeof(Vector4) * maxVertexCount_;
	vertexBuffer_ = dxCommon->CreateBufferResource(vertexBufferSize);
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	vertexBufferView_.StrideInBytes = sizeof(Vector4);

	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData_));

	// インデックスバッファ作成 (0, 1, 2, 3, 4, 5, ...)
	std::vector<uint32_t> indices(maxVertexCount_);
	for (uint32_t i = 0; i < maxVertexCount_; ++i) {
		indices[i] = i;
	}
	size_t indexBufferSize = sizeof(uint32_t) * indices.size();
	indexBuffer_ = dxCommon->CreateBufferResource(indexBufferSize);
	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast<UINT>(indexBufferSize);
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	uint32_t* indexData = nullptr;
	indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData, indices.data(), indexBufferSize);
	indexBuffer_->Unmap(0, nullptr);

	indexCount_ = 0; // 初期状態では描画ライン0
}

void WireframeObject::UpdateDynamicLines(const std::vector<std::pair<Vector3, Vector3>>& lines) {
	if (!mappedVertexData_ || maxVertexCount_ == 0) {
		return;
	}

	uint32_t lineCount = (std::min)(static_cast<uint32_t>(lines.size()), maxVertexCount_ / 2);
	for (uint32_t i = 0; i < lineCount; ++i) {
		mappedVertexData_[i * 2 + 0] = {lines[i].first.x, lines[i].first.y, lines[i].first.z, 1.0f};
		mappedVertexData_[i * 2 + 1] = {lines[i].second.x, lines[i].second.y, lines[i].second.z, 1.0f};
	}
	indexCount_ = lineCount * 2;
}

// ------------------------------------------------------------
// 斜線なし Cube の生成（12本のエッジ）
// ------------------------------------------------------------

void WireframeObject::CreateBox(const Vector3& size) {
	float hx = size.x * 0.5f;
	float hy = size.y * 0.5f;
	float hz = size.z * 0.5f;

	// 8つの角の頂点座標
	std::vector<Vector4> vertices = {
		{-hx, -hy, -hz, 1.0f}, // 0
		{-hx, hy, -hz, 1.0f},  // 1
		{hx, hy, -hz, 1.0f},   // 2
		{hx, -hy, -hz, 1.0f},  // 3
		{-hx, -hy, hz, 1.0f},  // 4
		{-hx, hy, hz, 1.0f},   // 5
		{hx, hy, hz, 1.0f},    // 6
		{hx, -hy, hz, 1.0f},   // 7
	};

	// 12本の辺を結ぶ（24個のインデックス）
	std::vector<uint32_t> indices = {
		0, 1, 1, 2, 2, 3, 3, 0, // 手前の4辺
		4, 5, 5, 6, 6, 7, 7, 4, // 奥の4辺
		0, 4, 1, 5, 2, 6, 3, 7, // 手前と奥をつなぐ4辺
	};

	CreateMeshBuffers(vertices, indices);
}


// ------------------------------------------------------------
// Grid（床の格子ライン）の生成
// ------------------------------------------------------------
void WireframeObject::CreateGrid(float size, uint32_t divisions) {
	CreateGrid(size, size, divisions, divisions);
}

void WireframeObject::CreateGrid(float sizeX, float sizeZ, uint32_t divisionsX, uint32_t divisionsZ) {
	std::vector<Vector4> vertices;
	std::vector<uint32_t> indices;
	float halfX = sizeX * 0.5f;
	float halfZ = sizeZ * 0.5f;
	float stepX = sizeX / static_cast<float>(divisionsX);
	float stepZ = sizeZ / static_cast<float>(divisionsZ);
	uint32_t currentIndex = 0;

	// X軸に平行なライン（横線）
	for (uint32_t i = 0; i <= divisionsZ; ++i)
	{
		float z = -halfZ + i * stepZ;
		vertices.push_back({-halfX, 0.0f, z, 1.0f});
		vertices.push_back({halfX, 0.0f, z, 1.0f});
		indices.push_back(currentIndex++);
		indices.push_back(currentIndex++);
	}
	// Z軸に平行なライン（縦線・奥行き方向）
	for (uint32_t i = 0; i <= divisionsX; ++i)
	{
		float x = -halfX + i * stepX;
		vertices.push_back({x, 0.0f, -halfZ, 1.0f});
		vertices.push_back({x, 0.0f, halfZ, 1.0f});
		indices.push_back(currentIndex++);
		indices.push_back(currentIndex++);
	}
	CreateMeshBuffers(vertices, indices);
}
// ------------------------------------------------------------
// Sphere（XYZ各軸の3つの円リングによるワイヤーフレーム）の生成
// ------------------------------------------------------------
void WireframeObject::CreateSphere(float radius, uint32_t subdivision) {
	std::vector<Vector4> vertices;
	std::vector<uint32_t> indices;
	float angleStep = 2.0f * std::numbers::pi_v<float> / static_cast<float>(subdivision);
	// 3平面（XY平面, XZ平面, YZ平面）に円リングを作成
	auto createCircle = [&](int planeType) {
		uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
		for (uint32_t i = 0; i < subdivision; ++i)
		{
			float angle = i * angleStep;
			float c = std::cos(angle) * radius;
			float s = std::sin(angle) * radius;
			if (planeType == 0)
			{ // XY平面
				vertices.push_back({c, s, 0.0f, 1.0f});
			} else if (planeType == 1)
			{ // XZ平面
				vertices.push_back({c, 0.0f, s, 1.0f});
			} else
			{ // YZ平面
				vertices.push_back({0.0f, c, s, 1.0f});
			}
			uint32_t nextIndex = (i + 1 == subdivision) ? baseIndex : baseIndex + i + 1;
			indices.push_back(baseIndex + i);
			indices.push_back(nextIndex);
		}
	};
	createCircle(0); // XY
	createCircle(1); // XZ
	createCircle(2); // YZ
	CreateMeshBuffers(vertices, indices);
}

