#pragma once

#include <d3d12.h>
#include <memory>
#include <wrl.h>

class DirectXCommon;
class Camera;

class SkyboxRenderer {
public:
	// シングルトンインスタンスの取得
	static SkyboxRenderer* GetInstance();

	SkyboxRenderer() = default;
	~SkyboxRenderer() = default;

	// 終了処理
	static void Finalize();

	// 初期化
	void Initialize(DirectXCommon* dxCommon);

	// 描画前共通設定
	void PreDraw();

	// デフォルトカメラの設定
	void SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }
	Camera* GetDefaultCamera() const { return defaultCamera_; }

	// DirectXCommonの取得
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
	static std::unique_ptr<SkyboxRenderer> instance;

	SkyboxRenderer(SkyboxRenderer&) = delete;
	SkyboxRenderer& operator=(SkyboxRenderer&) = delete;

	// ルートシグネチャの作成
	void CreateRootSignature();
	// グラフィックスパイプラインの生成
	void CreateGraphicsPipeline();

private:
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

	DirectXCommon* dxCommon_ = nullptr;
	Camera* defaultCamera_ = nullptr;

	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;
};