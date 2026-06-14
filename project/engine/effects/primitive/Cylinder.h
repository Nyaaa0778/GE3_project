#pragma once

#include "Primitive.h"
#include <d3d12.h>
#include <wrl.h>
#include <string>

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "WorldTransform.h"
#include "PrimitiveRenderer.h"

class Camera;

class Cylinder : public Primitive
{
public:
	Cylinder() = default;
	~Cylinder() override = default;

	void Initialize(const std::string& textureFilePath = "uvChecker.png") override;
	void Update() override;
	void Draw() override;

	// Transform Getter/Setter (override を追加)
	const Vector3& GetPosition() const override { return worldTransform_.translation; }
	void SetPosition(const Vector3& position) override { worldTransform_.translation = position; }

	const Vector3& GetRotation() const override { return worldTransform_.rotation; }
	void SetRotation(const Vector3& rotation) override { worldTransform_.rotation = rotation; }

	const Vector3& GetScale() const override { return worldTransform_.scale; }
	void SetScale(const Vector3& scale) override { worldTransform_.scale = scale; }

	// 親子関係の設定
	const WorldTransform* GetWorldTransform() const { return &worldTransform_; }
	void SetParent(const WorldTransform* parent) { worldTransform_.parent = parent; }

	// Color Setter (override を追加)
	void SetColor(const Vector4& color) override;

	// BlendMode Setter (override を追加)
	void SetBlendMode(PrimitiveRenderer::BlendMode blendMode) override { blendMode_ = blendMode; }

	// Camera Setter
	void SetCamera(Camera* camera) override { camera_ = camera; }

	// ImGui
	void DrawImGui(const char* windowName) override;

	// Texture Setter (override を追加)
	void SetTexture(const std::string& textureFilePath) override;

	// Overrideとして関数を追加
	const Vector2& GetUVTranslation() const override { return uvTranslation_; }
	void SetUVTranslation(const Vector2& translation) override { uvTranslation_ = translation; }

	// Buffer Getters
	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const override { return vertexBufferView_; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const override { return indexBufferView_; }
	uint32_t GetIndexCount() const override { return indexCount_; }

private:
	// (座標変換行列データはWorldTransformに移管)

	// ワールド変換データ
	WorldTransform worldTransform_;

	Vector2 uvTranslation_ = {0.0f, 0.0f};

private:
	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

	// 内部構造体 (PrimitiveRendererのシェーダ仕様に合わせる)
	struct VertexData
	{
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal; // シェーダでは使用しないが頂点バッファの整合性のため維持
	};

	struct Material
	{
		Vector4 color;
		Matrix4x4 uvTransform;
		float alphaReference; 
		float padding[3];
	};

	struct TransformationMatrix
	{
		Matrix4x4 WVP;
	};

	// 外部参照
	Camera* camera_ = nullptr;
	std::string textureFilePath_ = "uvChecker.png";
	PrimitiveRenderer::BlendMode blendMode_ = PrimitiveRenderer::BlendMode::kNormal;

	// ジオメトリ設定
	const uint32_t numSlices_ = 16; // デフォルトのスライス数

	// GPUリソース (共有メッシュを参照するためのViewのみ保持)
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	uint32_t indexCount_ = 0;

	ComPtr<ID3D12Resource> materialBuffer_ = nullptr;
	Material* materialData_ = nullptr;

	// データ作成処理
	void CreateMesh();
	void CreateMaterialData();
};
