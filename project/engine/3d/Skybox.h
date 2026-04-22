#pragma once

#include "Vector4.h"
#include "Matrix4x4.h"
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <array>

class Camera;

class Skybox {
public:
    //================================================================================
    // 構造体
    //================================================================================
    struct VertexData {
        Vector4 position;
    };

    struct ConstBufferData {
        Matrix4x4 wvp;
    };

    //================================================================================
    // パブリック関数
    //================================================================================

    // 6枚画像から初期化 (0:+X, 1:-X, 2:+Y, 3:-Y, 4:+Z, 5:-Z の順)
    void Initialize(const std::array<std::string, 6>& filePaths, Camera* camera);

    // DDS から初期化
    void Initialize(const std::string& ddsFilePath, Camera* camera);

    // 描画
    void Draw();

    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU() const { return textureSrvHandleGPU_; }

private:
    //================================================================================
    // プライベート関数
    //================================================================================

    // メッシュ（頂点・インデックス）の生成
    void CreateMesh();

    // 定数バッファの生成
    void CreateConstantBuffer();

    void InitializeCommon(Camera* camera);

private:
    //================================================================================
    // メンバ変数
    //================================================================================
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // 参照カメラ
    Camera* camera_ = nullptr;

    // リソース
    ComPtr<ID3D12Resource> vertexBuffer_;
    ComPtr<ID3D12Resource> indexBuffer_;
    ComPtr<ID3D12Resource> constBuffer_;

    // バッファビュー
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_ {};
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_ {};

    // マッピング先ポインタ
    ConstBufferData* constMap_ = nullptr;
};