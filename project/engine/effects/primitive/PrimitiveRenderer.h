#pragma once

#include <d3d12.h>
#include <memory>
#include <wrl.h>

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

	enum class BlendMode {
		kNone,
		kNormal,
		kAdd,
		kSubtract,
		kMultiply,
		kScreen,
		kCountOfBlendMode,
	};

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

	DirectXCommon* dxCommon_ = nullptr;

	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	
	// BlendModeごと、かつSolid設定(0=Effects, 1=Solid)のPSOを保持
	ComPtr<ID3D12PipelineState> graphicsPipelineStates_[static_cast<size_t>(BlendMode::kCountOfBlendMode)][2];

	void CreateRootSignature();
	void CreateGraphicsPipelines();
	D3D12_BLEND_DESC MakeBlendDesc(BlendMode mode);
};
