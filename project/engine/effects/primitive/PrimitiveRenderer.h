#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl.h>

#include "BlendMode.h"

class DirectXCommon;

class PrimitiveRenderer {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	static PrimitiveRenderer* GetInstance();

	PrimitiveRenderer() = default;
	~PrimitiveRenderer() = default;
	static void Finalize();

	friend std::default_delete<PrimitiveRenderer>;

private:
	static std::unique_ptr<PrimitiveRenderer> instance;

	PrimitiveRenderer(PrimitiveRenderer&) = delete;
	PrimitiveRenderer& operator=(PrimitiveRenderer&) = delete;

public:
	//================================================================================
	// Blend Mode
	//================================================================================

	using BlendMode = ::BlendMode;

	//================================================================================
	// 共有ジオメトリ (Flyweightパターン用)
	//================================================================================

	// 複数のPrimitive（例：BoxやPlane）間で頂点・インデックスバッファを共有するための構造体
	struct SharedGeometry {
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
		D3D12_INDEX_BUFFER_VIEW indexBufferView{};
		uint32_t indexCount = 0;
	};

	// 共有ジオメトリの取得（まだ作られていなければ nullptr を返す）
	SharedGeometry* GetSharedGeometry(const std::string& name);

	// 生成した共有ジオメトリを登録する
	void SetSharedGeometry(const std::string& name, const SharedGeometry& geometry);

public:
	//================================================================================
	// 初期化 / 描画設定
	//================================================================================

	void Initialize(DirectXCommon* dxCommon);

	// 特定のBlendModeで描画設定を行う (isSolid = true の場合、CullMode=BACK, DepthWrite=ALLになります)
	void SetupCommonRenderState(BlendMode blendMode, bool isSolid = false);

	DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

	// 登録された共有ジオメトリを保管するマップ
	std::unordered_map<std::string, SharedGeometry> sharedGeometries_;

	DirectXCommon* dxCommon_ = nullptr;

	ComPtr<IDxcBlob> vsBlob_;
	ComPtr<IDxcBlob> psBlob_;
};
