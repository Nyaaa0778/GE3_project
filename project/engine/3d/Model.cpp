#include "Model.h"
#include "DirectXCommon.h"
#include "MathUtility.h"
#include "ModelCommon.h"
#include "TextureManager.h"

#include <cassert>
#include <fstream>
#include <sstream>

using namespace MathUtility;

/// <summary>
/// 初期化
/// </summary>
/// <param name="modelCommon">ModelCommonのポインタ</param>
void Model::Initialize(ModelCommon *modelCommon,
                       const std::string &directoryPath,
                       const std::string &filename) {
  modelCommon_ = modelCommon;

  LoadObjFile(directoryPath, filename);

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
      2, TextureManager::GetInstance()->GetSrvHandlGPU(
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
Model::LoadMaterialTemplateFile(const std::string &directoryPath,
                                const std::string &filename) {
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
/// <param name="filename">読み込む.objファイル名</param>
void Model::LoadObjFile(const std::string &directoryPath,
                        const std::string &filename) {
  std::vector<Vector4> positions; // 位置
  std::vector<Vector3> normals;   // 法線
  std::vector<Vector2> texcoords; // テクスチャ座標
  std::string line;               // ファイルから読んだ1行を格納するもの

  std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
  assert(file.is_open()); // とりあえず開けなかったら止める

  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier; // 先頭の識別子を読む

    if (identifier == "v") {
      Vector4 position;
      s >> position.x >> position.y >> position.z;
      position.w = 1.0f;
      position.x *= -1.0f;
      positions.push_back(position);
    } else if (identifier == "vt") {
      Vector2 texcoord;
      s >> texcoord.x >> texcoord.y;
      texcoord.y = 1.0f - texcoord.y;
      texcoords.push_back(texcoord);
    } else if (identifier == "vn") {
      Vector3 normal;
      s >> normal.x >> normal.y >> normal.z;
      normal.x *= -1.0f;
      normals.push_back(normal);
    } else if (identifier == "f") {
      VertexData triangle[3];

      // 面は三角形限定
      for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
        std::string vertexDefinition;
        s >> vertexDefinition;
        // 頂点の要素のIndexは 位置 / UV / 法線
        // で格納されているので、分解してIndexを取得
        std::istringstream v(vertexDefinition);

        uint32_t elementIndices[3];
        for (int32_t element = 0; element < 3; ++element) {
          std::string index;
          std::getline(v, index, '/');
          elementIndices[element] = std::stoi(index);
        }

        // 要素へのIndexから、実際の要素の値を取得して頂点を構築
        Vector4 position = positions[elementIndices[0] - 1];
        Vector2 texcoord = texcoords[elementIndices[1] - 1];
        Vector3 normal = normals[elementIndices[2] - 1];

        triangle[faceVertex] = {position, texcoord, normal};
      }
      // 頂点を逆順で登録することで周り順を逆にする
      modelData_.vertices.push_back(triangle[2]);
      modelData_.vertices.push_back(triangle[1]);
      modelData_.vertices.push_back(triangle[0]);
    } else if (identifier == "mtllib") {
      // materialTemolateLibraryファイルの名前を取得
      std::string materialFilename;
      s >> materialFilename;
      // 基本的にobjファイルと同一階層にmtlは存在させるのでディレクトリ名とファイル名を渡す
      modelData_.material =
          LoadMaterialTemplateFile(directoryPath, materialFilename);
    }
  }
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
  vertexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
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
  materialBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));

  // マテリアルデータの初期値を書き込む
  materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  materialData_->enableLighting = true;
  materialData_->uvTransform = MakeIdentityMatrix();
}