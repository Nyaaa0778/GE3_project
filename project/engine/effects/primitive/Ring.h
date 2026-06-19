#pragma once

#include "Primitive.h"
#include <d3d12.h>
#include <wrl.h>
#include <string>

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "Transform.h"
#include "PrimitiveRenderer.h"

class Camera;

enum class RingUVDirection {
	Horizon, // 円周に沿ってUが変化
	Vertical // 半径に沿ってUが変化（円周はV）
};

class Ring : public Primitive {
public:
	Ring() = default;
	~Ring() override = default;

	void Initialize(const std::string& textureFilePath = "uvChecker.png") override;
	void Update() override;
	void Draw() override;

	// Transform Getter/Setter
	const Vector3& GetPosition() const { return position_; }
	void SetPosition(const Vector3& position) { position_ = position; }

	const Vector3& GetRotation() const { return rotation_; }
	void SetRotation(const Vector3& rotation) { rotation_ = rotation; }

	const Vector3& GetScale() const { return scale_; }
	void SetScale(const Vector3& scale) { scale_ = scale; }

	// Color Setter
	void SetColor(const Vector4& color);

	// BlendMode Setter
	void SetBlendMode(PrimitiveRenderer::BlendMode blendMode) { blendMode_ = blendMode; }

	// Camera Setter
	void SetCamera(Camera* camera) override { camera_ = camera; }

	// 角度の指定（ラジアン）
	void SetAngle(float startAngle, float endAngle);
	// 半径の指定（スプライン補間用に中間点の外径も指定）
	void SetRadius(float startOuter, float midOuter, float endOuter, float inner);
	// UVの方向指定
	void SetUVDirection(RingUVDirection direction);

	// ImGui
	void DrawImGui(const char* windowName) override;

	// Texture Setter
	void SetTexture(const std::string& textureFilePath);

	// Buffer Getters
	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const override { return vertexBufferView_; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const override { return indexBufferView_; }
	uint32_t GetIndexCount() const override { return indexCount_; }

private:
	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

	// 内部構造体 (PrimitiveRendererのシェーダ仕様に合わせる)
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal; // シェーダでは使用しないが頂点バッファの整合性のため維持
	};

	struct Material {
		Vector4 color;
		Matrix4x4 uvTransform;
	};

	struct TransformationMatrix {
		Matrix4x4 WVP;
	};

	// Transform
	Transform transform_ {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
	Vector3 position_ = {0.0f, 0.0f, 0.0f};
	Vector3 rotation_ = {0.0f, 0.0f, 0.0f};
	Vector3 scale_ = {1.0f, 1.0f, 1.0f};

	// 外部参照
	Camera* camera_ = nullptr;
	std::string textureFilePath_ = "uvChecker.png";
	PrimitiveRenderer::BlendMode blendMode_ = PrimitiveRenderer::BlendMode::kNormal;

	// GPUリソース (共有メッシュを参照するためのViewのみ保持)
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};

	D3D12_INDEX_BUFFER_VIEW indexBufferView_ {};
	uint32_t indexCount_ = 0;

	ComPtr<ID3D12Resource> materialBuffer_ = nullptr;
	Material* materialData_ = nullptr;

	ComPtr<ID3D12Resource> transformationMatrixBuffer_ = nullptr;
	TransformationMatrix* transformationMatrixData_ = nullptr;

	ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
	ComPtr<ID3D12Resource> indexBuffer_ = nullptr;

	bool isMeshDirty_ = true; // パラメータが変わったら再生成するフラグ

	float startAngle_ = 0.0f;
	float endAngle_ = 6.283185307f; // 2PI
	float startOuterRadius_ = 1.0f;
	float midOuterRadius_ = 1.0f; // スプライン補間用
	float endOuterRadius_ = 1.0f;
	float innerRadius_ = 0.2f;
	RingUVDirection uvDirection_ = RingUVDirection::Horizon;

	// データ作成処理
	void CreateMaterialData();
	void CreateTransformationMatrixData();
	void CreateMesh(); // CreateMesh()の代わりに動的生成を行う関数
};
