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
    struct VertexData {
        Vector4 position;
    };

    struct ConstBufferData {
        Matrix4x4 wvp;
    };

    // 6枚の画像パスを受け取るように変更
    // 順番: 0:+X, 1:-X, 2:+Y, 3:-Y, 4:+Z, 5:-Z
    void Initialize(const std::array<std::string, 6>& filePaths, Camera* camera);

    void Draw();

private:
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12Resource> vertexBuffer_;
    ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_ {};

    ComPtr<ID3D12Resource> constBuffer_;
    ConstBufferData* constMap_ = nullptr;

    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_ {};

    Camera* camera_ = nullptr;
};