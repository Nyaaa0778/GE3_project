#pragma once

#include <d3d12.h>
#include <string>

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "PrimitiveRenderer.h"

class Camera;

class Primitive
{
public:
	// 仮想デストラクタ（基底クラスとして必須）
	virtual ~Primitive() = default;

	// 派生クラスでのオーバーライドを前提とする純粋仮想関数
	virtual void Initialize(const std::string& textureFilePath = "uvChecker.png") = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	// ==========================================
	// Transform Getter/Setter (追加)
	// ==========================================
	virtual const Vector3& GetPosition() const = 0;
	virtual void SetPosition(const Vector3& position) = 0;

	virtual const Vector3& GetRotation() const = 0;
	virtual void SetRotation(const Vector3& rotation) = 0;

	virtual const Vector3& GetScale() const = 0;
	virtual void SetScale(const Vector3& scale) = 0;

	virtual const Vector2& GetUVTranslation() const = 0;
	virtual void SetUVTranslation(const Vector2& translation) = 0;

	// ==========================================
	// 各種パラメータ Setter (追加)
	// ==========================================
	virtual void SetColor(const Vector4& color) = 0;
	virtual void SetBlendMode(PrimitiveRenderer::BlendMode blendMode) = 0;
	virtual void SetTexture(const std::string& textureFilePath) = 0;

	// 共通インターフェース
	virtual void SetCamera(Camera* camera) = 0;
	virtual void DrawImGui(const char* windowName) = 0;

	// Buffer Getters
	virtual const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const = 0;
	virtual const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const = 0;
	virtual uint32_t GetIndexCount() const = 0;
};
