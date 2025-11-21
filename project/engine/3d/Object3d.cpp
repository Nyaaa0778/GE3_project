#include "Object3d.h"
#include "DirectXCommon.h"
#include "MathUtility.h"
#include "Object3dRenderer.h"
#include "TextureManager.h"

#include <cassert>
#include <fstream>
#include <sstream>

using namespace MathUtility;

/// <summary>
/// 初期化
/// </summary>
/// <param name="object3dRenderer">Object3dRendererのポインタ</param>
void Object3d::Initialize(Object3dRenderer *object3dRenderer) {
  // 引数で受け取ってメンバ変数に保存
  object3dRenderer_ = object3dRenderer;

  LoadObjFile("resources", "plane.obj");

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
  // 座標変換行列データの作成
  CreateTransformationMatrixData();
  // 平行光源データの作成
  CreateDirectionalLightData();

  // Transform変数を作成
  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, -3.14f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  cameraTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 4.0f, -10.0f}};
}
/// <summary>
/// 更新
/// </summary>
void Object3d::Update() {
  // transformからworldMatrixを作成
  Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                           transform_.translate);

  // cameraTransformからcameraMatrixを作成
  Matrix4x4 cameraMatrix =
      MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate,
                       cameraTransform_.translate);
  // cameraMatrixからviewMatrixを作成
  Matrix4x4 viewMatrix = MakeInverseMatrix(cameraMatrix);
  // projectionMatrixを作成して透視投影行列を書き込む
  Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
      0.45f,
      float(object3dRenderer_->GetDxCommon()->GetClientWidth()) /
          float(object3dRenderer_->GetDxCommon()->GetClientHeight()),
      0.1f, 100.0f);

  transformationMatrixData_->WVP = worldMatrix * viewMatrix * projectionMatrix;
  transformationMatrixData_->World = worldMatrix;
}
/// <summary>
/// 描画
/// </summary>
void Object3d::Draw() {

  object3dRenderer_->SetupCommonRenderState();

  // vertexBufferViewを設定
  object3dRenderer_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(
      0, 1, &vertexBufferView_);

  // マテリアルのCBufferの場所を設定
  object3dRenderer_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          0, materialBuffer_->GetGPUVirtualAddress());

  // 座標変換行列のCBufferの場所を設定
  object3dRenderer_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          1, transformationMatrixBuffer_->GetGPUVirtualAddress());

  // SRVのDescriptorTableの先頭を設定
  object3dRenderer_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootDescriptorTable(
          2, TextureManager::GetInstance()->GetSrvHandlGPU(
                 modelData_.material.textureIndex));

  // 平行光源CBufferの場所を設定
  object3dRenderer_->GetDxCommon()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          3, directionalLightBuffer_->GetGPUVirtualAddress());

  // 描画
  object3dRenderer_->GetDxCommon()->GetCommandList()->DrawInstanced(
      static_cast<UINT>(modelData_.vertices.size()), 1, 0, 0);
}

/// <summary>
/// .mtlファイルを読む
/// </summary>
/// <param
/// name="directoryPath">.mtlファイルが置いてあるディレクトリのパス</param>
/// <param name="filename">読み込みたい.mtlファイル名</param>
/// <returns>ファイルから読み込んだマテリアル情報</returns>
Object3d::MaterialData
Object3d::LoadMaterialTemplateFile(const std::string &directoryPath,
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
void Object3d::LoadObjFile(const std::string &directoryPath,
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
      positions.push_back(position);
    } else if (identifier == "vt") {
      Vector2 texcoord;
      s >> texcoord.x >> texcoord.y;
      texcoords.push_back(texcoord);
    } else if (identifier == "vn") {
      Vector3 normal;
      s >> normal.x >> normal.y >> normal.z;
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

        position.x *= -1.0f;
        texcoord.y = 1.0f - texcoord.y;
        normal.x *= -1.0f;

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

/// <summary>
/// 頂点データの作成
/// </summary>
void Object3d::CreateVertexData() {
  // vertexResourceを作成
  vertexBuffer_ = object3dRenderer_->GetDxCommon()->CreateBufferResource(
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
void Object3d::CreateMaterialData() {
  // マテリアル用のリソースを作成
  materialBuffer_ =
      object3dRenderer_->GetDxCommon()->CreateBufferResource(sizeof(Material));

  // マテリアルにデータを書き込む
  materialBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));

  // マテリアルデータの初期値を書き込む
  materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  materialData_->enableLighting = false;
  materialData_->uvTransform = MakeIdentityMatrix();
}
/// <summary>
/// 座標変換行列データの作成
/// </summary>
void Object3d::CreateTransformationMatrixData() {
  // 座標変換行列リソースを作成
  transformationMatrixBuffer_ =
      object3dRenderer_->GetDxCommon()->CreateBufferResource(
          sizeof(TransformationMatrix));

  // transformationMatrixResourceに座標変換行列データを書き込む
  transformationMatrixBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));

  // 単位行列を書き込んでおく
  transformationMatrixData_->WVP = MakeIdentityMatrix();
  transformationMatrixData_->World = MakeIdentityMatrix();
}
/// <summary>
/// 平行光源データの作成
/// </summary>
void Object3d::CreateDirectionalLightData() {
  // 平行光源リソースを作成
  directionalLightBuffer_ =
      object3dRenderer_->GetDxCommon()->CreateBufferResource(
          sizeof(DirectionalLight));

  // directionalLightResourceに平行光源データを書き込む
  directionalLightBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData_));

  // 平行光源データの初期値を書き込む
  directionalLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLightData_->direction = {0.0f, -1.0f, 0.0f};
  directionalLightData_->intensity = 1.0f;
}
