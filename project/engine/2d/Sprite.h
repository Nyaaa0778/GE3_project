#pragma once

#include <Matrix4x4.h>
#include <Transform.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <wrl.h>

class Sprite {
public:
	//================================================================================
	// 初期化 / 更新 / 描画
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="filePath">使いたいテクスチャのファイルパス</param>
	void Initialize(std::string filePath);
	/// <summary>
	/// 更新
	/// </summary>
	void Update();
	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

public:
	//================================================================================
	// Getter
	//================================================================================

	// 位置
	const Vector2& GetPosition() const { return position_; }
	// 回転
	const float& GetRotate() const { return rotation_; }
	// 色
	const Vector4& GetColor() const { return materialData_->color; }
	// 拡縮
	const Vector2& GetScale() const { return scale_; }

	// アンカーポイント
	const Vector2& GetAnchorPoint() const { return anchorPoint_; }
	// 左右反転フラグ
	bool GetFlipX() const { return isFlipX_; }
	// 上下反転フラグ
	bool GetFlipY() const { return isFlipY_; }
	// テクスチャ左上座標
	const Vector2& GetTextureLeftTop() const { return textureLeftTop_; }
	// テクスチャの切り抜きサイズ
	const Vector2& GetTextureSize() const { return textureSize_; }

	//================================================================================
	// Setter
	//================================================================================

	// 位置
	void SetPosition(const Vector2& position);
	// 回転
	void SetRotation(float rotation);
	// 色
	void SetColor(const Vector4& color);
	// 拡縮
	void SetScale(const Vector2& scale);

	// テクスチャ
	void SetTexture(const std::string& filePath);

	// アンカーポイント
	void SetAnchorPoint(const Vector2& anchorPoint);
	// 左右反転フラグ
	void SetFlipX(bool isFlipX);
	// 上下反転フラグ
	void SetFlipY(bool isFlipY);
	// テクスチャ左上座標
	void SetTextureLeftTop(const Vector2& textureLeftTop);
	// テクスチャの切り抜きサイズ
	void SetTextureSize(const Vector2& textureSize);

private:
	//================================================================================
	// 内部構造体
	//================================================================================

	// 頂点データ
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	// マテリアル
	struct Material {
		Vector4 color;
		Matrix4x4 uvTransform;
	};

	// 座標変換行列データ
	struct TransformationMatrix {
		Matrix4x4 WVP;
	};

private:
	//================================================================================
	// 型エイリアス
	//================================================================================

	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
	//================================================================================
	// GPUリソース（頂点/インデックス）
	//================================================================================

	// 頂点リソース
	ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
	ComPtr<ID3D12Resource> indexBuffer_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_ {};

	//================================================================================
	// GPUリソース（定数バッファ）
	//================================================================================

	// マテリアルリソース(定数バッファ)
	ComPtr<ID3D12Resource> materialBuffer_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	Material* materialData_ = nullptr;

	// バッファリソース(定数バッファ)
	ComPtr<ID3D12Resource> transformationMatrixBuffer_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	TransformationMatrix* transformationMatrixData_ = nullptr;

	//================================================================================
	// テクスチャ情報
	//================================================================================

	// テクスチャ番号
	uint32_t textureIndex_ = 0;
	// ファイルパス
	std::string filePath_;

	// テクスチャの元のサイズ
	Vector2 size_ = {0.0f, 0.0f};

	// テクスチャ左上座標
	Vector2 textureLeftTop_ = {0.0f, 0.0f};
	// テクスチャの切り抜きサイズ
	Vector2 textureSize_ = {100.0f, 100.0f};

	//================================================================================
	// Transform / 見た目
	//================================================================================

	// Transform
	Transform transform_ {};
	// 位置
	Vector2 position_ = {0.0f, 0.0f};
	// 回転
	float rotation_ = 0.0f;
	// 拡縮
	Vector2 scale_ = {1.0f, 1.0f};

	// アンカーポイント
	Vector2 anchorPoint_ = {0.0f, 0.0f};
	// 左右反転フラグ
	bool isFlipX_ = false;
	// 上下反転フラグ
	bool isFlipY_ = false;

private:
	//================================================================================
	// データ作成処理
	//================================================================================

	/// <summary>
	/// 頂点データの作成
	/// </summary>
	void CreateVertexData();
	/// <summary>
	/// インデックスデータの作成
	/// </summary>
	void CreateIndexData();
	/// <summary>
	/// マテリアルデータの作成
	/// </summary>
	void CreateMaterialData();
	/// <summary>
	/// 座標変換行列データの作成
	/// </summary>
	void CreateTransformationMatrixData();

	//================================================================================
	// テクスチャサイズ
	//================================================================================

	/// <summary>
	/// テクスチャサイズをリソースに合わせる
	/// </summary>
	void AdjustTextureSize();

private:
	//================================================================================
	// Getter
	//================================================================================

	/// <summary>
	/// 描画時のテクスチャサイズ
	/// </summary>
	/// <returns>実際に描画するサイズ</returns>
	Vector2 GetDisplaySize() const {
		return {size_.x * scale_.x, size_.y * scale_.y};
	}
};
