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
	worldTransform_.Initialize();

	// テクスチャの読み込み（デフォルト）
	TextureManager::GetInstance()->LoadTexture("resources/sprites/" + textureFilePath_);
}

void Plane::SetWorldTransform(WorldTransform* worldTransform)
{
	externalWorldTransform_ = worldTransform;
}

void Plane::Update()
{
	Matrix4x4 matWorld;
	if (externalWorldTransform_) {
		matWorld = externalWorldTransform_->matWorld;
		worldTransform_.matWorld = matWorld;
		if (worldTransform_.constMap) {
			worldTransform_.constMap->World = matWorld;
			worldTransform_.constMap->WorldInverseTranspose = MakeTransposeMatrix(MakeInverseMatrix(matWorld));
		}
	} else {
		worldTransform_.UpdateMatrix();
		matWorld = worldTransform_.matWorld;
	}

	Matrix4x4 worldViewProjectionMatrix;
	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = matWorld * viewProjectionMatrix;
	} else {
		worldViewProjectionMatrix = matWorld;
	}

	if (worldTransform_.constMap) {
		worldTransform_.constMap->WVP = worldViewProjectionMatrix;
	}
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
	commandList->SetGraphicsRootConstantBufferView(1, worldTransform_.constBuffer->GetGPUVirtualAddress());

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
	auto renderer = PrimitiveRenderer::GetInstance();

	// 1. すでにPlaneのメッシュが作られて保管されているかチェック
	auto sharedGeom = renderer->GetSharedGeometry("Plane");
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
	const uint32_t kNumVertices = 4;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer = dxCommon->CreateBufferResource(sizeof(VertexData) * kNumVertices);
	
	vertexBufferView_.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kNumVertices;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	VertexData* vertexData = nullptr;
	vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// Planeの頂点データ（ローカル座標系）
	// Z軸のマイナス方向を向くPlaneとする
	// v0 (左上), v1 (右上), v2 (左下), v3 (右下)
	vertexData[0].position = {-1.0f, 1.0f, 0.0f, 1.0f};
	vertexData[0].texcoord = {0.0f, 0.0f};
	vertexData[0].normal = {0.0f, 0.0f, -1.0f};

	vertexData[1].position = {1.0f, 1.0f, 0.0f, 1.0f};
	vertexData[1].texcoord = {1.0f, 0.0f};
	vertexData[1].normal = {0.0f, 0.0f, -1.0f};

	vertexData[2].position = {-1.0f, -1.0f, 0.0f, 1.0f};
	vertexData[2].texcoord = {0.0f, 1.0f};
	vertexData[2].normal = {0.0f, 0.0f, -1.0f};

	vertexData[3].position = {1.0f, -1.0f, 0.0f, 1.0f};
	vertexData[3].texcoord = {1.0f, 1.0f};
	vertexData[3].normal = {0.0f, 0.0f, -1.0f};

	// --- インデックスバッファの作成 ---
	const uint32_t kNumIndices = 6;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer = dxCommon->CreateBufferResource(sizeof(uint32_t) * kNumIndices);

	indexBufferView_.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * kNumIndices;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexData = nullptr;
	indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	
	// 三角形ポリゴン2枚
	indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
	indexData[3] = 2; indexData[4] = 1; indexData[5] = 3;

	indexCount_ = kNumIndices;

	// 3. 生成したバッファをSharedGeometryにまとめ、Rendererに保管してもらう
	PrimitiveRenderer::SharedGeometry newGeom;
	newGeom.vertexBuffer = vertexBuffer;
	newGeom.vertexBufferView = vertexBufferView_;
	newGeom.indexBuffer = indexBuffer;
	newGeom.indexBufferView = indexBufferView_;
	newGeom.indexCount = indexCount_;

	renderer->SetSharedGeometry("Plane", newGeom);
}

void Plane::CreateMaterialData()
{
	auto dxCommon = DirectXCommon::GetInstance();

	materialBuffer_ = dxCommon->CreateBufferResource(sizeof(Material));
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->uvTransform = MakeIdentityMatrix();
	materialData_->alphaReference = 0.0f;
}

// (座標変換行列データの作成はWorldTransformに移管したため削除)

void Plane::DrawImGui(const char* windowName) {
#ifdef USE_IMGUI
	ImGui::Begin(windowName);

	ImGui::DragFloat3("座標", &worldTransform_.translation.x, 0.1f);
	ImGui::DragFloat3("回転", &worldTransform_.rotation.x, 0.01f);
	ImGui::DragFloat3("スケール", &worldTransform_.scale.x, 0.1f);

	static int currentBlendMode = static_cast<int>(blendMode_);
	const char* blendModeNames[] = { "なし", "通常", "加算", "減算", "乗算", "スクリーン" };
	if (ImGui::Combo("ブレンドモード", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
		blendMode_ = static_cast<PrimitiveRenderer::BlendMode>(currentBlendMode);
	}

	ImGui::End();
#endif
}

