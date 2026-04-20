#pragma once

#include "Vector4.h"
#include "Matrix4x4.h" // 行列用
#include <d3d12.h>
#include <wrl.h>
#include <string>

class Camera; // カメラの事前宣言

class Skybox {
public:
    struct VertexData {
        Vector4 position;
    };

    // GPUに送る定数バッファの構造体（ワールドビュープロジェクション行列）
    struct ConstBufferData {
        Matrix4x4 wvp;
    };

    // 初期化関数にテクスチャのファイルパスを追加
    void Initialize(const std::string& textureFilePath, Camera* camera);

    // 描画関数（カメラの情報が必要）
    void Draw();

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_ {};

    // 定数バッファ（行列用）
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
    ConstBufferData* constMap_ = nullptr;

    // テクスチャのファイルパス
    std::string textureFilePath_;

    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_ {};

    Camera* camera_ = nullptr;
};