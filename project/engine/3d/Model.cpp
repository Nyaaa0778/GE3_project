#include "Model.h"
#include "DirectXCommon.h"
#include "MathUtility.h"
#include "ModelCommon.h"
#include "TextureManager.h"

#include <cassert>
#include <fstream>
#include <sstream>

#include <assimp/Importer.hpp>
//#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace MathUtility;

/// <summary>
/// 初期化
/// </summary>
void Model::Initialize(const std::string& directoryPath,
	const std::string& filename) {

	modelCommon_ = ModelCommon::GetInstance();

	LoadModelFile(directoryPath, filename);

	//.objの参照しているテクスチャファイルを読み込む
	TextureManager::GetInstance()->LoadTexture(
		modelData_.material.textureFilePath);
	// 読み込んだテクスチャの番号を取得
	modelData_.material.textureIndex =
		TextureManager::GetInstance()->GetTextureIndexByFilePath(
			modelData_.material.textureFilePath);

	// 頂点データの作成
	CreateVertexData();
	// マテリアルデータの作成
	CreateMaterialData();
}

/// <summary>
/// 描画
/// </summary>
void Model::Draw() {
	assert(modelCommon_->GetDxCommon());
	assert(modelCommon_->GetDxCommon()->GetCommandList());

	assert(!modelData_.material.textureFilePath.empty());
	auto h = TextureManager::GetInstance()->GetSrvHandleGPU(
		modelData_.material.textureFilePath);
	assert(h.ptr != 0);

	// vertexBufferViewを設定
	modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(
		0, 1, &vertexBufferView_);

	// マテリアルのCBufferの場所を設定
	modelCommon_->GetDxCommon()
		->GetCommandList()
		->SetGraphicsRootConstantBufferView(
			0, materialBuffer_->GetGPUVirtualAddress());

	// SRVのDescriptorTableの先頭を設定
	modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(
		4, TextureManager::GetInstance()->GetSrvHandleGPU(
			modelData_.material.textureFilePath));

	// 描画
	modelCommon_->GetDxCommon()->GetCommandList()->DrawInstanced(
		static_cast<UINT>(modelData_.vertices.size()), 1, 0, 0);
}

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
Model::MaterialData
Model::LoadMaterialTemplateFile(const std::string& directoryPath,
	const std::string& filename) {
	// 構築するMaterialData
	MaterialData materialData;
	// ファイルから読んだ1行を格納するもの
	std::string line;
	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::stringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 凍結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	return materialData;
}

/// <summary>
/// .objファイルを読み込む
/// </summary>
/// <param
/// name="directoryPath">.objファイルが置いてあるディレクトリのパス</param>
/// <param name="filename">読み込む .obj ファイル名</param>
void Model::LoadModelFile(const std::string& directoryPath,
	const std::string& filename) {
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene->HasMeshes()); // メッシュがないものは非対応

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals()); // 法線がないメッシュは非対応
		assert(mesh->HasTextureCoords(0)); // Texcoord がないメッシュは非対応

		// メッシュの中身(Face)の解析
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // 三角形のみサポート

			// Face の中身(Vertex)の解析
			for (uint32_t element = 0; element < face.mNumIndices; ++element) {
				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

				VertexData vertex;
				vertex.position = {position.x, position.y, position.z, 1.0f};
				vertex.normal = {normal.x, normal.y, normal.z};
				vertex.texcoord = {texcoord.x, texcoord.y};

				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;
				modelData_.vertices.push_back(vertex);
			}
		}
	}

	// マテリアルの解析
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
		aiMaterial* material = scene->mMaterials[materialIndex];

		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData_.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
		}
	}

	modelData_.rootNode = ReadNode(scene->mRootNode);
}

Model::Node Model::ReadNode(aiNode* node) {
	Node result;

	// ノードの localMatrix を取得
	aiMatrix4x4 aiLocalMatrix = node->mTransformation;
	aiLocalMatrix.Transpose(); // 列ベクトル形式を行ベクトル形式に転置
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.localMatrix.m[i][j] = aiLocalMatrix[i][j];
		}
	}

	result.name = node->mName.C_Str(); // Node名を格納
	result.children.resize(node->mNumChildren); // 子どもの数だけ確保

	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		// 再帰的に読んで階層構造を作る
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}

	return result;
}

//================================================================================
// データ作成処理
//================================================================================

/// <summary>
/// 頂点データの作成
/// </summary>
void Model::CreateVertexData() {
	// vertexResourceを作成
	vertexBuffer_ = modelCommon_->GetDxCommon()->CreateBufferResource(
		sizeof(VertexData) * modelData_.vertices.size());

	// vertexBufferViewを作成
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes =
		UINT(sizeof(VertexData) * modelData_.vertices.size());
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// vertexResourceに頂点データを書き込む
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	std::memcpy(vertexData_, modelData_.vertices.data(),
		sizeof(VertexData) * modelData_.vertices.size());
}
/// <summary>
/// マテリアルデータの作成
/// </summary>
void Model::CreateMaterialData() {
	// マテリアル用のリソースを作成
	materialBuffer_ =
		modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));

	// マテリアルにデータを書き込む
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	// マテリアルデータの初期値を書き込む
	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->lightingType = static_cast<int32_t>(LightingType::kBlinnPhong);
	materialData_->uvTransform = MakeIdentityMatrix();
	materialData_->shininess = 70.0f;
	materialData_->environmentCoefficient = 0.0f;
}