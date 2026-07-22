#pragma once

#include <Matrix4x4.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>

#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "LightingType.h"
#include <assimp/scene.h>

class ModelCommon;

class Model {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(const std::string& directoryPath,
		const std::string& filename);
	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

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
		Vector4 color;              // 16 bytes
		int32_t lightingType;       // 4 bytes
		float padding1[3];          // 12 bytes (align Matrix4x4)
		Matrix4x4 uvTransform;      // 64 bytes
		float shininess;            // 4 bytes
		float environmentCoefficient;// 4 bytes
		float dissolveThreshold;    // 4 bytes
		float dissolveEdgeWidth;    // 4 bytes
		Vector4 dissolveEdgeColor;  // 16 bytes
		int32_t isDissolveEnabled;  // 4 bytes
		float padding2[3];          // 12 bytes (align total size to 16 bytes boundary, total 144 bytes)
	};

	// マテリアルデータ
	struct MaterialData {
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};

	// ノード
	struct Node {
		Matrix4x4 localMatrix;
		std::string name;
		std::vector<Node> children;
	};

	// モデルデータ
	struct ModelData {
		std::vector<VertexData> vertices;
		MaterialData material;
		Node rootNode;
	};

public:
	//================================================================================
	// ファイル読み込む（.mtl / .obj）
	//================================================================================

	/// <summary>
	/// .mtlファイルを読む
	/// </summary>
	/// <param
	/// name="directoryPath">.mtlファイルが置いてあるディレクトリのパス</param>
	/// <param name="filename">読み込みたい.mtlファイル名</param>
	/// <returns>ファイルから読み込んだマテリアル情報</returns>
	MaterialData LoadMaterialTemplateFile(const std::string& directoryPath,
		const std::string& filename);

	/// <summary>
	/// .objファイルを読み込む
	/// </summary>
	/// <param
	/// name="directoryPath">.objファイルが置いてあるディレクトリのパス</param>
	/// <param name="filename">読み込む.objファイル名</param>
	void LoadModelFile(const std::string& directoryPath,
		const std::string& filename);

public:
	//================================================================================
	// Getter
	//================================================================================

	// 色
	const Vector4& GetColor() const { return materialData_->color; }

	// 環境光の映り込み具合
	float GetEnvironmentCofficient() { return materialData_->environmentCoefficient; }

	const ModelData& GetModelData() const { return modelData_; }

	// ディゾルブパラメータ
	float GetDissolveThreshold() const { return materialData_->dissolveThreshold; }
	float GetDissolveEdgeWidth() const { return materialData_->dissolveEdgeWidth; }
	const Vector4& GetDissolveEdgeColor() const { return materialData_->dissolveEdgeColor; }
	bool IsDissolveEnabled() const { return materialData_->isDissolveEnabled != 0; }

	//================================================================================
	// Setter
	//================================================================================

	// 色
	void SetColor(const Vector4& color) { materialData_->color = color; }
	// ライティングの種類
	void SetLightingType(LightingType type) { materialData_->lightingType = static_cast<int32_t>(type); }
	// 環境光の映り込み具合
	void SetEnvironmentCoefficient(float coeff) { materialData_->environmentCoefficient = coeff; }
	// ディゾルブパラメータ
	void SetDissolveThreshold(float threshold) { materialData_->dissolveThreshold = threshold; }
	void SetDissolveEdgeWidth(float width) { materialData_->dissolveEdgeWidth = width; }
	void SetDissolveEdgeColor(const Vector4& color) { materialData_->dissolveEdgeColor = color; }
	void SetDissolveEnabled(bool enabled) { materialData_->isDissolveEnabled = enabled ? 1 : 0; }

private:
	//================================================================================
	// 型エイリアス
	//================================================================================

	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
	//================================================================================
	// 外部参照
	//================================================================================

	// ModelCommonのポインタ
	ModelCommon* modelCommon_ = nullptr;
	//================================================================================
	// GPUリソース（頂点）
	//================================================================================

	// 頂点リソース
	ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData_ = nullptr;
	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};

	//================================================================================
	// GPUリソース（定数バッファ）
	//================================================================================

	// マテリアルリソース
	ComPtr<ID3D12Resource> materialBuffer_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	Material* materialData_ = nullptr;

	//================================================================================
	// OBJデータ
	//================================================================================

	// Objファイルのデータ
	ModelData modelData_;

private:

	Node ReadNode(aiNode* node);

	//================================================================================
	// データ作成処理
	//================================================================================

	/// <summary>
	/// 頂点データの作成
	/// </summary>
	void CreateVertexData();
	/// <summary>
	/// マテリアルデータの作成
	/// </summary>
	void CreateMaterialData();
};
