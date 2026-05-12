#pragma once

#include <d3d12.h>
#include <memory>
#include <wrl.h>

class DirectXCommon;

class SkyboxRenderer {
public:
	//================================================================================
	// シングルトン管理
	//================================================================================
	static SkyboxRenderer* GetInstance();
	static void Finalize();

	SkyboxRenderer() = default;
	~SkyboxRenderer() = default;

public:
	//================================================================================
	// パブリック関数
	//================================================================================

	// 初期化
	void Initialize(DirectXCommon* dxCommon);

	// 描画前共通設定
	void PreDraw();

	// DirectXCommonの取得
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
	//================================================================================
	// プライベート関数
	//================================================================================
	SkyboxRenderer(const SkyboxRenderer&) = delete;
	SkyboxRenderer& operator=(const SkyboxRenderer&) = delete;

	// パイプラインの構成要素生成
	void CreateRootSignature();
	void CreateGraphicsPipeline();

private:
	//================================================================================
	// メンバ変数
	//================================================================================
	static std::unique_ptr<SkyboxRenderer> instance;

	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

	// 共通基盤
	DirectXCommon* dxCommon_ = nullptr;

	// パイプラインステート
	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;
};