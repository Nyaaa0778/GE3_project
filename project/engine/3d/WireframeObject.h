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
	WireframeObject() = default;
	~WireframeObject();

	void Initialize();

	void Update();

	void Draw();

	void CreateBox(const Vector3& size = {1.0f, 1.0f, 1.0f});

	void CreateSphere(float radius = 1.0f, uint32_t subdivision = 16);

	void CreateGrid(float size = 10.0f, uint32_t divisions = 10);
	void CreateGrid(float sizeX, float sizeZ, uint32_t divisionsX, uint32_t divisionsZ);

	/// <summary>
	/// 動的ライン描画用のメッシュバッファを初期化
	/// </summary>
	/// <param name="maxLines">最大ライン本数</param>
	void CreateDynamicLineMesh(uint32_t maxLines);

	/// <summary>
	/// 動的ラインの頂点座標を更新 (未描画ラインはcountに含まない)
	/// </summary>
	/// <param name="lines">描画する各ラインの始点と終点のペア配列</param>
	void UpdateDynamicLines(const std::vector<std::pair<Vector3, Vector3>>& lines);

public:
	struct MaterialData {
		Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		float fogNear = 0.0f;
		float fogFar = 0.0f;
		float pad[2] = {0.0f, 0.0f};
	};

	void SetPosition(const Vector3& pos) { transform_.translation = pos; }
	void SetRotation(const Vector3& rotation) { transform_.rotation = rotation; }
	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	void SetColor(const Vector4& color);
	void SetFog(float fogNear, float fogFar);
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
	MaterialData* materialData_ = nullptr;

	Vector4* mappedVertexData_ = nullptr;
	uint32_t maxVertexCount_ = 0;

private:
	void CreateMeshBuffers(const std::vector<Vector4>& vertices, 
						   const std::vector<uint32_t>& indices);
};

