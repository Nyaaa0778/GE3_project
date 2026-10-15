#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include <BlendMode.h>
#include <RootParameter.h>

class DirectXCommon;

// PSO を識別するための設定キー
struct PipelineDescKey {
	ID3D12RootSignature* rootSignature = nullptr; // ルートシグネチャ
	IDxcBlob* vsBlob = nullptr;                   // 頂点シェーダ
	IDxcBlob* psBlob = nullptr;                   // ピクセルシェーダ

	// インプットレイアウト
	const D3D12_INPUT_ELEMENT_DESC* inputElements = nullptr;
	uint32_t numInputElements = 0;

	// ステートの設定
	BlendMode blendMode = BlendMode::kNormal;
	D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
	D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
	bool depthEnable = true;
	bool depthWrite = true;
	D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// 比較演算子（マップのキー判定）
	bool operator==(const PipelineDescKey& other) const;

};

// ハッシュ関数
struct PipelineDescKeyHash {
	size_t operator()(const PipelineDescKey& key) const;
};

class PipelineStateCache {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static PipelineStateCache* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	PipelineStateCache() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~PipelineStateCache() = default;

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<PipelineStateCache>;

private:
	static std::unique_ptr<PipelineStateCache> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	PipelineStateCache(PipelineStateCache&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	PipelineStateCache& operator=(PipelineStateCache&) = delete;

public:
	void Initialize(DirectXCommon* dxCommon);

	// キーに対応するPSOを取得（未作成なら生成してキャッシュ）
	ID3D12PipelineState* GetOrCreate(const PipelineDescKey& key);

	// 共通ルートシグネチャの取得
	ID3D12RootSignature* GetCommonRootSignature() const { return commonRootSignature_.Get(); }

private:
	//================================================================================
	// 型エイリアス
	//================================================================================

	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
	DirectXCommon* dxCommon_ = nullptr;

	std::unordered_map<PipelineDescKey, ComPtr<ID3D12PipelineState>, PipelineDescKeyHash> psoCache_;

	ComPtr<ID3D12RootSignature> commonRootSignature_;

private:
	void CreateCommonRootSignature();
};