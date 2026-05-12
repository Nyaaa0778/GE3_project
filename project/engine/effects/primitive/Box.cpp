#include "Box.h"
#include "DirectXCommon.h"
#include "PrimitiveRenderer.h"
#include "TextureManager.h"
#include "Camera.h"
#include "MathUtility.h"
#include "Object3dRenderer.h" // GetDefaultCamera()を取得するための一時的な対応
#include "ImGuiManager.h"

using namespace MathUtility;

void Box::Initialize(const std::string& textureFilePath)
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

void Box::Update()
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

void Box::Draw()
{
	auto primitiveRenderer = PrimitiveRenderer::GetInstance();
	auto commandList = primitiveRenderer->GetDxCommon()->GetCommandList();

	// 共通の描画設定 (BlendModeを指定し、Solid描画を有効化)
	primitiveRenderer->SetupCommonRenderState(blendMode_, true);

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
	commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

void Box::SetColor(const Vector4& color)
{
	if (materialData_) {
		materialData_->color = color;
	}
}

void Box::SetTexture(const std::string& textureFilePath)
{
	textureFilePath_ = textureFilePath;
	TextureManager::GetInstance()->LoadTexture("resources/sprites/" + textureFilePath_);
}

void Box::CreateMesh()
{
	auto renderer = PrimitiveRenderer::GetInstance();

	// 1. すでにBoxのメッシュが作られて保管されているかチェック
	auto sharedGeom = renderer->GetSharedGeometry("Box");
	if (sharedGeom) {
		// すでに存在する場合は、ビュー（参照情報）だけをコピーして終了（高速化）
		vertexBufferView_ = sharedGeom->vertexBufferView;
		indexBufferView_ = sharedGeom->indexBufferView;
		indexCount_ = sharedGeom->indexCount;
		return;
	}

	// 2. まだ作られていない場合（最初の1個目）はバッファを生成する
	auto dxCommon = DirectXCommon::GetInstance();

	// --- 頂点バッファの作成 ---
	const uint32_t kNumVertices = 24;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer = dxCommon->CreateBufferResource(sizeof(VertexData) * kNumVertices);

	vertexBufferView_.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kNumVertices;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	VertexData* vertexData = nullptr;
	vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	float w = 1.0f;
	float h = 1.0f;
	float d = 1.0f;

	// Front (normal 0, 0, -1)
	vertexData[0] = {{-w,  h, -d, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
	vertexData[1] = {{ w,  h, -d, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
	vertexData[2] = {{-w, -h, -d, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
	vertexData[3] = {{ w, -h, -d, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};

	// Back (normal 0, 0, 1)
	vertexData[4] = {{ w,  h,  d, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	vertexData[5] = {{-w,  h,  d, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	vertexData[6] = {{ w, -h,  d, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
	vertexData[7] = {{-w, -h,  d, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};

	// Top (normal 0, 1, 0)
	vertexData[8] = {{-w,  h,  d, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
	vertexData[9] = {{ w,  h,  d, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
	vertexData[10] = {{-w,  h, -d, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};
	vertexData[11] = {{ w,  h, -d, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};

	// Bottom (normal 0, -1, 0)
	vertexData[12] = {{-w, -h, -d, 1.0f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
	vertexData[13] = {{ w, -h, -d, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
	vertexData[14] = {{-w, -h,  d, 1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};
	vertexData[15] = {{ w, -h,  d, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};

	// Right (normal 1, 0, 0)
	vertexData[16] = {{ w,  h, -d, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
	vertexData[17] = {{ w,  h,  d, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
	vertexData[18] = {{ w, -h, -d, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
	vertexData[19] = {{ w, -h,  d, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};

	// Left (normal -1, 0, 0)
	vertexData[20] = {{-w,  h,  d, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
	vertexData[21] = {{-w,  h, -d, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
	vertexData[22] = {{-w, -h,  d, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};
	vertexData[23] = {{-w, -h, -d, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};

	// --- インデックスバッファの作成 ---
	const uint32_t kNumIndices = 36;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer = dxCommon->CreateBufferResource(sizeof(uint32_t) * kNumIndices);

	indexBufferView_.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * kNumIndices;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexData = nullptr;
	indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	// 6 faces * 2 triangles * 3 indices
	for (int i = 0; i < 6; ++i) {
		indexData[i * 6 + 0] = i * 4 + 0;
		indexData[i * 6 + 1] = i * 4 + 1;
		indexData[i * 6 + 2] = i * 4 + 2;
		indexData[i * 6 + 3] = i * 4 + 2;
		indexData[i * 6 + 4] = i * 4 + 1;
		indexData[i * 6 + 5] = i * 4 + 3;
	}

	indexCount_ = kNumIndices;

	// 3. 生成したバッファをSharedGeometryにまとめ、Rendererに保管してもらう
	PrimitiveRenderer::SharedGeometry newGeom;
	newGeom.vertexBuffer = vertexBuffer;
	newGeom.vertexBufferView = vertexBufferView_;
	newGeom.indexBuffer = indexBuffer;
	newGeom.indexBufferView = indexBufferView_;
	newGeom.indexCount = indexCount_;

	renderer->SetSharedGeometry("Box", newGeom);
}

void Box::CreateMaterialData()
{
	auto dxCommon = DirectXCommon::GetInstance();

	materialBuffer_ = dxCommon->CreateBufferResource(sizeof(Material));
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->uvTransform = MakeIdentityMatrix();
}

void Box::CreateTransformationMatrixData()
{
	auto dxCommon = DirectXCommon::GetInstance();

	transformationMatrixBuffer_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	transformationMatrixBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	transformationMatrixData_->WVP = MakeIdentityMatrix();
}

void Box::DrawImGui(const char* windowName)
{
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

	ImGui::End();
#endif
}
