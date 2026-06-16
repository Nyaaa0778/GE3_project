#include "Sprite.h"

#include "DirectXCommon.h"
#include "MathUtility.h"
#include "SpriteRenderer.h"
#include "TextureManager.h"
#include "WinApp.h"

using namespace MathUtility;

//================================================================================
// 初期化 / 更新 / 描画
//================================================================================

/// <summary>
/// 初期化
/// </summary>
/// <param name="filePath">使いたいテクスチャのファイルパス</param>
void Sprite::Initialize(std::string filePath, const Vector2& position, const Vector2& anchorPoint) {
	// 引数で受け取ってメンバ変数に記録する
	filePath_ = filePath;
	position_ = position;
	anchorPoint_ = anchorPoint;

	SetTexture(filePath);

	// 頂点データの作成
	CreateVertexData();
	// インデックスデータの作成
	CreateIndexData();
	// マテリアルデータの作成
	CreateMaterialData();
	// 座標変換行列データの初期化
	worldTransform_.Initialize();

	AdjustTextureSize();
}
/// <summary>
/// 更新
/// </summary>
void Sprite::Update() {
	// アンカーポイント
	float left = 0.0f - anchorPoint_.x;
	float right = 1.0f - anchorPoint_.x;
	float top = 0.0f - anchorPoint_.y;
	float bottom = 1.0f - anchorPoint_.y;

	// 左右反転
	if (isFlipX_) {
		left = -left;
		right = -right;
	}
	// 上下反転
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	// テクスチャのメタデータを取得
	const DirectX::TexMetadata& metadata =
		TextureManager::GetInstance()->GetMetaData(filePath_);

	float textureLeft = textureLeftTop_.x / metadata.width;
	float textureRight = (textureLeftTop_.x + textureSize_.x) / metadata.width;
	float textureTop = textureLeftTop_.y / metadata.height;
	float textureBottom = (textureLeftTop_.y + textureSize_.y) / metadata.height;

	// 頂点リソースにデータを書き込む
	// 左下
	vertexData_[0] = {{left, bottom, 0.0f, 1.0f}, {textureLeft, textureBottom}};
	// 左上
	vertexData_[1] = {{left, top, 0.0f, 1.0f}, {textureLeft, textureTop}};
	// 右下
	vertexData_[2] = {{right, bottom, 0.0f, 1.0f}, {textureRight, textureBottom}};
	// 右上
	vertexData_[3] = {{right, top, 0.0f, 1.0f}, {textureRight, textureTop}};

	// インデックスリソースにデータを書き込む
	indexData_[0] = 0;
	indexData_[1] = 1;
	indexData_[2] = 2;
	indexData_[3] = 1;
	indexData_[4] = 3;
	indexData_[5] = 2;

	// Transform情報を作る
	worldTransform_.scale = {GetDisplaySize().x, GetDisplaySize().y, 1.0f};
	worldTransform_.rotation = {0.0f, 0.0f, rotation_};
	worldTransform_.translation = {position_.x, position_.y, 0.0f};

	worldTransform_.UpdateMatrix();

	if (worldTransform_.constMap) {
		Matrix4x4 matProjection = MakeOrthographicMatrix(
			0.0f, 0.0f, static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 0.0f, 1.0f);
		worldTransform_.constMap->WVP = Multiply(worldTransform_.matWorld, matProjection);
	}
}
/// <summary>
/// 描画
/// </summary>
void Sprite::Draw() {

	SpriteRenderer::GetInstance()->SetupCommonRenderState();

	// vertexBufferViewを設定
	DirectXCommon::GetInstance()->GetCommandList()->IASetVertexBuffers(
		0, 1, &vertexBufferView_);
	// indexBufferViewを設定
	DirectXCommon::GetInstance()->GetCommandList()->IASetIndexBuffer(
		&indexBufferView_);

	// マテリアルのCBufferの場所を設定
	DirectXCommon::GetInstance()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			0, materialBuffer_->GetGPUVirtualAddress());

	// 座標変換行列のCBufferの場所を設定
	DirectXCommon::GetInstance()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			1, worldTransform_.constBuffer->GetGPUVirtualAddress());

	// SRVのDescriptorTableの先頭を設定
	DirectXCommon::GetInstance()
		->GetCommandList()
		->SetGraphicsRootDescriptorTable(
			2, TextureManager::GetInstance()->GetSrvHandleGPU(filePath_));

	// 描画
	DirectXCommon::GetInstance()->GetCommandList()->DrawIndexedInstanced(6, 1, 0,
		0, 0);
}

//================================================================================
// データ作成処理
//================================================================================

/// <summary>
/// 頂点データの作成
/// </summary>
void Sprite::CreateVertexData() {

	// vertexResourceを作成
	vertexBuffer_ = DirectXCommon::GetInstance()->CreateBufferResource(
		sizeof(VertexData) * 4);

	// vertexBufferViewを作成する
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// vertexResourceに頂点データを書き込む
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
}
/// <summary>
/// インデックスデータの作成
/// </summary>
void Sprite::CreateIndexData() {
	// indexResourceを作成
	indexBuffer_ =
		DirectXCommon::GetInstance()->CreateBufferResource(sizeof(uint32_t) * 6);

	// indexBufferViewを作成する
	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// indexResourceにインデックスデータを書き込む
	indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
}
/// <summary>
/// マテリアルデータの作成
/// </summary>
void Sprite::CreateMaterialData() {
	// マテリアルリソースを作る
	materialBuffer_ =
		DirectXCommon::GetInstance()->CreateBufferResource(sizeof(Material));

	// 書き込むためのアドレスを取得して、マテリアルにデータを書き込む
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	// マテリアルデータの初期値を書き込む
	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->uvTransform = MakeIdentityMatrix();
}
// (座標変換行列データの作成はWorldTransformに移管したため削除)

//================================================================================
// テクスチャサイズ
//================================================================================

/// <summary>
/// テクスチャサイズをリソースに合わせる
/// </summary>
void Sprite::AdjustTextureSize() {
	// テクスチャメタデータを取得
	const DirectX::TexMetadata& metadata =
		TextureManager::GetInstance()->GetMetaData(filePath_);

	textureSize_.x = static_cast<float>(metadata.width);
	textureSize_.y = static_cast<float>(metadata.height);

	// 画像サイズをテクスチャの元のサイズに合わせる
	size_ = textureSize_;
}

//================================================================================
// Setter
//================================================================================

// 位置
void Sprite::SetPosition(const Vector2& position) { position_ = position; }
// 回転
void Sprite::SetRotation(float rotation) { rotation_ = rotation; }
// 色
void Sprite::SetColor(const Vector4& color) { materialData_->color = color; }
// 拡縮
void Sprite::SetScale(const Vector2& scale) { scale_ = scale; }
// テクスチャ
void Sprite::SetTexture(const std::string& filePath) {
	std::string fullPath = filePath;

	// パスに "resources/sprites/" が含まれていなければ付与する
	if (fullPath.find("resources/sprites/") == std::string::npos) {
		fullPath = "resources/sprites/" + filePath;
	}

	// 構築したフルパスを使ってテクスチャを読み込む
	TextureManager::GetInstance()->LoadTexture(fullPath);
	textureIndex_ =
		TextureManager::GetInstance()->GetTextureIndexByFilePath(fullPath);

	// メンバ変数の filePath_ にはフルパスを保存しておく
	filePath_ = fullPath;
}

// アンカーポイント
void Sprite::SetAnchorPoint(const Vector2& anchorPoint) {
	anchorPoint_ = anchorPoint;
}
// 左右反転フラグ
void Sprite::SetFlipX(bool isFlipX) { isFlipX_ = isFlipX; }
// 上下反転フラグ
void Sprite::SetFlipY(bool isFlipY) { isFlipY_ = isFlipY; }
// テクスチャ左上座標
void Sprite::SetTextureLeftTop(const Vector2& textureLeftTop) {
	textureLeftTop_ = textureLeftTop;
}
// テクスチャの切り抜きサイズ
void Sprite::SetTextureSize(const Vector2& textureSize) {
	textureSize_ = textureSize;
}