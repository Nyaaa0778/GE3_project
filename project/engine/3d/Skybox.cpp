#include "Skybox.h"
#include "DirectXCommon.h"
#include "MathUtility.h"
#include "Camera.h"
#include "TextureManager.h"
#include "SkyboxRenderer.h"
#include <cassert>

using namespace MathUtility;

//================================================================================
// パブリック関数
//================================================================================

void Skybox::Initialize(const std::array<std::string, 6>& filePaths, Camera* camera) {
    InitializeCommon(camera);

    // テクスチャのロードとキューブマップ合成
    textureSrvHandleGPU_ = TextureManager::GetInstance()->CreateCubemapFromFiles(filePaths);
}

void Skybox::Initialize(const std::string& ddsFilePath, Camera* camera) {
    InitializeCommon(camera);

    // DDSを読み込む
    TextureManager* textureManager = TextureManager::GetInstance();
    textureManager->LoadTexture(ddsFilePath);

    // 念のため、本当に cubemap か確認
    assert(textureManager->GetMetaData(ddsFilePath).IsCubemap());

    // GPUハンドルを取得
    textureSrvHandleGPU_ = textureManager->GetSrvHandleGPU(ddsFilePath);
}

void Skybox::Draw() {
    auto commandList = DirectXCommon::GetInstance()->GetCommandList();

    // 大きめの箱をワールドに置く
    Matrix4x4 worldMatrix = MakeAffineMatrix(
        {500.0f, 500.0f, 500.0f},
        {0.0f, 0.0f, 0.0f},
        camera_->GetTranslate()
    );

    // カメラのView行列をそのまま使う
    Matrix4x4 viewMatrix = camera_->GetViewMatrix();

    Matrix4x4 wvpMatrix =
        Multiply(worldMatrix, Multiply(viewMatrix, camera_->GetProjectionMatrix()));
    constMap_->wvp = wvpMatrix;

    SkyboxRenderer::GetInstance()->PreDraw();

    commandList->SetGraphicsRootConstantBufferView(0, constBuffer_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(1, textureSrvHandleGPU_);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer(&indexBufferView_);
    commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

//================================================================================
// プライベート関数
//================================================================================

void Skybox::CreateMesh() {
    // --- 頂点バッファの作成 ---
    const uint32_t kNumVertices = 8;
    vertexBuffer_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * kNumVertices);
    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * kNumVertices;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // 頂点データの書き込み
    VertexData* vertexData = nullptr;
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    vertexData[0].position = {1.0f, 1.0f, 1.0f, 1.0f};
    vertexData[1].position = {1.0f, 1.0f, -1.0f, 1.0f};
    vertexData[2].position = {1.0f, -1.0f, 1.0f, 1.0f};
    vertexData[3].position = {1.0f, -1.0f, -1.0f, 1.0f};
    vertexData[4].position = {-1.0f, 1.0f, 1.0f, 1.0f};
    vertexData[5].position = {-1.0f, 1.0f, -1.0f, 1.0f};
    vertexData[6].position = {-1.0f, -1.0f, 1.0f, 1.0f};
    vertexData[7].position = {-1.0f, -1.0f, -1.0f, 1.0f};
    vertexBuffer_->Unmap(0, nullptr);

    // --- インデックスバッファの作成 ---
    const uint32_t kNumIndices = 36;
    indexBuffer_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(uint32_t) * kNumIndices);
    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * kNumIndices;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // インデックスデータの書き込み
    uint32_t* indexData = nullptr;
    indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
    // 右・左面
    indexData[0] = 0;  indexData[1] = 1;  indexData[2] = 2;  indexData[3] = 2;  indexData[4] = 1;  indexData[5] = 3;
    indexData[6] = 5;  indexData[7] = 4;  indexData[8] = 7;  indexData[9] = 7;  indexData[10] = 4; indexData[11] = 6;
    // 前・背面
    indexData[12] = 4; indexData[13] = 0; indexData[14] = 6; indexData[15] = 6; indexData[16] = 0; indexData[17] = 2;
    indexData[18] = 1; indexData[19] = 5; indexData[20] = 3; indexData[21] = 3; indexData[22] = 5; indexData[23] = 7;
    // 上・下面
    indexData[24] = 5; indexData[25] = 1; indexData[26] = 4; indexData[27] = 4; indexData[28] = 1; indexData[29] = 0;
    indexData[30] = 2; indexData[31] = 3; indexData[32] = 6; indexData[33] = 6; indexData[34] = 3; indexData[35] = 7;
    indexBuffer_->Unmap(0, nullptr);
}

void Skybox::CreateConstantBuffer() {
    // 定数バッファの作成とマッピング
    constBuffer_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(ConstBufferData));
    constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));
}

void Skybox::InitializeCommon(Camera* camera) {
    assert(camera);
    camera_ = camera;

    CreateMesh();
    CreateConstantBuffer();
}
