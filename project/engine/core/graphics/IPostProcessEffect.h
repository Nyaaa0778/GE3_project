#pragma once

#include <d3d12.h>

class DirectXCommon;

class IPostProcessEffect
{
public:
	virtual ~IPostProcessEffect() = default;
	virtual void Initialize(DirectXCommon* dxCommon) = 0;
	virtual void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) = 0;
};
