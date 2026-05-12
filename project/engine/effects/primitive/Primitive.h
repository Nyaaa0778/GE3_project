#pragma once

#include <d3d12.h>
#include <string>

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

	// 共通インターフェース
	virtual void SetCamera(Camera* camera) = 0;
	virtual void DrawImGui(const char* windowName) = 0;

	// Buffer Getters
	virtual const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const = 0;
	virtual const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const = 0;
	virtual uint32_t GetIndexCount() const = 0;
};
