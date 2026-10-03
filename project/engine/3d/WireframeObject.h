#pragma once

#include <cstdint>
#include <vector>

#include <Vector3.h>
#include <Vector4.h>
#include <WorldTransform.h>

class Camera;
class WireframeRenderer;

class WireframeObject {
public:
	void Initialize();

	void Update();

	void Draw();

	void CreateBox(const Vector3& size = {1.0f, 1.0f, 1.0f});

	void CreateSphere(float radius = 1.0f, uint32_t subdivision = 16);

	void CreateGrid(float size = 10.0f, uint32_t divisions = 10);

public:
	void SetPosition(const Vector3& pos) { transform_.translation = pos; }
	void SetRotation(const Vector3& rotation) { transform_.rotation = rotation; }
	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	void SetColor(const Vector4& color);
	void SetCamera(Camera* camera) { camera_ = camera; }

	WorldTransform& GetWorldTransform() { return transform_; }

private:
	//================================================================================
	// 型エイリアス
	//================================================================================

	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
	WireframeRenderer* wireframeRenderer_ = nullptr;
	WorldTransform transform_;

	Camera* camera_ = nullptr;

	ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	uint32_t indexCount_ = 0;

	ComPtr<ID3D12Resource> colorResource_;
	Vector4* colorData_ = nullptr;

private:
	void CreateMeshBuffers(const std::vector<Vector4>& vertices, 
						   const std::vector<uint32_t>& indices);
};

