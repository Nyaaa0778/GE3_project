#pragma once

#include <d3d12.h>
#include <dxcapi.h>

class DirectXCommon;

/// <summary>
/// ポストプロセス各エフェクトの基底インターフェース
/// パラメータ定数バッファとピクセルシェーダの準備・提供を担当
/// </summary>
class IPostProcessEffect
{
public:
	virtual ~IPostProcessEffect() = default;

	/// <summary>
	/// 初期化（シェーダ読み込み・定数バッファ生成など）
	/// </summary>
	virtual void Initialize(DirectXCommon* dxCommon) = 0;

	/// <summary>
	/// 描画直前のパラメータ更新（経過時間の加算など）
	/// </summary>
	virtual void PreDraw() {}

	/// <summary>
	/// ピクセルシェーダBlobの取得
	/// </summary>
	virtual IDxcBlob* GetPixelShader() const = 0;

	/// <summary>
	/// 定数バッファのアドレス取得（不要な場合は0を返す）
	/// </summary>
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const { return 0; }

	/// <summary>
	/// 追加テクスチャ（t1: マスクやノイズなど）の有無
	/// </summary>
	virtual bool HasExtraTexture() const { return false; }

	/// <summary>
	/// 追加テクスチャ（t1）のGPUハンドル取得
	/// </summary>
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetExtraTextureHandle() const { return {}; }
};
