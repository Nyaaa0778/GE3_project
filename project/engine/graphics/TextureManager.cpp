#include "TextureManager.h"

#include "DirectXCommon.h"
#include "ShaderResourceViewManager.h"
#include "StringUtility.h"

using namespace StringUtility;

std::unique_ptr<TextureManager> TextureManager::instance = nullptr;

// ImGuiで0番を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

//================================================================================
// シングルトン管理 / 初期化・終了
//================================================================================

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>TextureManager の唯一のインスタンス</returns>
TextureManager* TextureManager::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<TextureManager>();
	}

	return instance.get();
}

/// <summary>
/// 初期化
/// </summary>
void TextureManager::Initialize(DirectXCommon* dxCommon,
	ShaderResourceViewManager* srvManager) {

	// メンバ変数を記録
	dxCommon_ = dxCommon;
	srvManager_ = srvManager;

	// SRVの数と同数
	textureDatas_.reserve(DirectXCommon::GetInstance()->kMaxSRVCount);
}

/// <summary>
/// 終了
/// </summary>
void TextureManager::Finalize() { instance.reset(); }

//================================================================================
// テクスチャ読み込み / 中間リソース解放
//================================================================================

/// <summary>
/// 画像ファイルを読み込みテクスチャに変換
/// </summary>
/// <param name="filePath"></param>
/// <returns></returns>
void TextureManager::LoadTexture(const std::string& filePath) {

	// 読み込み済みテクスチャを検索、読み込み済みなら早期に return
	if (textureDatas_.contains(filePath)) {
		return;
	}

	TextureData& textureData = textureDatas_[filePath];

	// テクスチャ枚数上限チェック
	assert(ShaderResourceViewManager::GetInstance()->CanAllocate());

	// テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image {};
	std::wstring filePathW = ConvertString(filePath); // Wはワイド文字列を意味する
	HRESULT hr;

	// 拡張子が .dds かどうかで読み込み関数を分岐
	if (filePathW.ends_with(L".dds")) {
		hr = DirectX::LoadFromDDSFile(
			filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		hr = DirectX::LoadFromWICFile(
			filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}

	// ミニマップの作成
	// mipMap: 元画像より小さなテクスチャ群
	DirectX::ScratchImage mipImages {};

	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		mipImages = std::move(image);
	} else {
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
			image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
			4, mipImages);
	}

	assert(SUCCEEDED(hr));

	textureData.metadata = mipImages.GetMetadata();
	textureData.metadata.format = DirectX::MakeSRGB(textureData.metadata.format);
	textureData.resource =
		DirectXCommon::GetInstance()->CreateTextureResource(textureData.metadata);

	textureData.srvIndex = ShaderResourceViewManager::GetInstance()->Allocate();
	textureData.srvHandleCPU =
		ShaderResourceViewManager::GetInstance()->GetCPUDescriptorHandle(
			textureData.srvIndex);
	textureData.srvHandleGPU =
		ShaderResourceViewManager::GetInstance()->GetGPUDescriptorHandle(
			textureData.srvIndex);

	/*ShaderResourceViewManager::GetInstance()->CreateSRVfortexture2D(
		textureData.srvIndex, textureData.resource.Get(),
		textureData.metadata.format,
		static_cast<UINT>(textureData.metadata.mipLevels));*/

	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
	srvDesc.Format = textureData.metadata.format; // テクスチャのフォーマット
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (textureData.metadata.IsCubemap()) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 通常の2Dテクスチャとして扱う
		srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels); // ミップマップの数を指定}
	}

	// SRVの生成
	// デバイスを使って、取得したCPUハンドルの場所にSRVを作る
	dxCommon_->GetDevice()->CreateShaderResourceView(
		textureData.resource.Get(),
		&srvDesc,
		textureData.srvHandleCPU
	);

	// 転送用に生成した中間リソースをテクスチャデータ構造体に格納
	textureData.intermediateResource =
		DirectXCommon::GetInstance()->UploadTextureData(textureData.resource,
			mipImages);
}

/// <summary>
/// 中間リソースの解放
/// </summary>
void TextureManager::ReleaseIntermediateResources() {
	for (std::pair<const std::string, TextureData>& pair : textureDatas_) {

		// 2番目（value側）を取り出す
		TextureData& textureData = pair.second;

		// 中間リソースを解放
		textureData.intermediateResource.Reset();
	}
}

/// <summary>
/// 6枚の画像ファイルから1つのキューブマップを生成
/// </summary>
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::CreateCubemapFromFiles(const std::array<std::string, 6>& filePaths) {
	// 識別用のキー作成（最初のファイル名などを利用）
	std::string key = filePaths[0] + "_cubemap";
	if (textureDatas_.contains(key)) {
		return textureDatas_[key].srvHandleGPU;
	}

	TextureData& textureData = textureDatas_[key];

	// 1. まず6枚の画像を個別に一時ロードしてサイズ等を確認する
	std::array<DirectX::ScratchImage, 6> loadedImages;
	for (int i = 0; i < 6; ++i) {
		std::wstring filePathW = StringUtility::ConvertString(filePaths[i]);
		HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, loadedImages[i]);
		assert(SUCCEEDED(hr));
	}

	// 2. キューブマップとしてのメタデータを構成
	const DirectX::TexMetadata& baseMeta = loadedImages[0].GetMetadata();
	DirectX::TexMetadata cubeMeta = baseMeta;
	cubeMeta.arraySize = 6; // 6枚
	cubeMeta.miscFlags |= DirectX::TEX_MISC_TEXTURECUBE; // キューブマップフラグ

	// 3. 合成用の ScratchImage を作成
	DirectX::ScratchImage mipImages;
	mipImages.Initialize2D(
		cubeMeta.format,
		cubeMeta.width,
		cubeMeta.height,
		cubeMeta.arraySize,
		cubeMeta.mipLevels,
		DirectX::CP_FLAGS_NONE
	);

	// 4. 各画像を合成用 ScratchImage の各スライスにコピー
	for (int i = 0; i < 6; ++i) {
		const DirectX::Image* srcImage = loadedImages[i].GetImage(0, 0, 0);
		const DirectX::Image* destImage = mipImages.GetImage(0, i, 0); // i番目のスライス

		assert(srcImage->width == destImage->width && srcImage->height == destImage->height);

		// ピクセルデータのコピー
		std::memcpy(destImage->pixels, srcImage->pixels, srcImage->slicePitch);
	}

	// 5. リソース作成
	textureData.metadata = cubeMeta;
	textureData.metadata.format = DirectX::MakeSRGB(textureData.metadata.format);
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

	// 6. 指定された形式の UploadTextureData を使用して転送
	// 返り値の中間リソースを保持しておく（TextureData構造体にメンバがある場合）
	textureData.intermediateResource = dxCommon_->UploadTextureData(textureData.resource, mipImages);

	// 7. SRV作成
	textureData.srvIndex = srvManager_->Allocate();
	textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE; // ここでCubeとして認識させる
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = (UINT) textureData.metadata.mipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	dxCommon_->GetDevice()->CreateShaderResourceView(
		textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

	return textureData.srvHandleGPU;
}

//================================================================================
// Getter
//================================================================================

/// <summary>
/// SRVインデックスの取得
/// </summary>
/// <param name="filePath">検索対象となるテクスチャのファイルパス</param>
/// <returns>見つかったテクスチャのSRVインデックス、
/// 該当するテクスチャが存在しない場合は assert によりプログラムを停止する</returns>
uint32_t
TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {
	// map からテクスチャを検索
	std::unordered_map<std::string, TextureData>::const_iterator it =
		textureDatas_.find(filePath);

	// 存在しなければ停止
	assert(it != textureDatas_.end());

	// value の中にある SRV index を返す
	return it->second.srvIndex;
}

/// <summary>
/// SRVハンドルの取得
/// </summary>
/// <param name="filePath">テクスチャのファイルパス</param>
/// <returns>指定インデックスのSRVのGPUデスクリプタハンドル</returns>
D3D12_GPU_DESCRIPTOR_HANDLE
TextureManager::GetSrvHandleGPU(const std::string& filePath) {
	// map からテクスチャを検索
	std::unordered_map<std::string, TextureData>::iterator it =
		textureDatas_.find(filePath);

	// 見つからなかったら停止
	assert(it != textureDatas_.end());

	// テクスチャデータ（value）を取得
	TextureData& textureData = it->second;

	// そのテクスチャの GPU SRV ハンドルを返す
	return textureData.srvHandleGPU;
}

/// <summary>
/// メタデータを取得
/// </summary>
/// <param name="filePath">テクスチャのファイルパス</param>
/// <returns>指定テクスチャの幅・高さ・フォーマットなどのメタデータ</returns>
const DirectX::TexMetadata&
TextureManager::GetMetaData(const std::string& filePath) {
	// キーでテクスチャを検索
	std::unordered_map<std::string, TextureData>::iterator it =
		textureDatas_.find(filePath);

	// 見つからなかったら停止
	assert(it != textureDatas_.end());

	// テクスチャデータを取得してメタデータを返す
	return it->second.metadata;
}
