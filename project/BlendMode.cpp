#include "BlendMode.h"

namespace BlendModeHelper
{
	D3D12_BLEND_DESC BlendModeHelper::MakeBlendDesc(BlendMode mode) {
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.RenderTarget[0].RenderTargetWriteMask =
			D3D12_COLOR_WRITE_ENABLE_ALL;

		switch (mode)
		{
		case BlendMode::kNone:
			blendDesc.RenderTarget[0].BlendEnable = FALSE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
			break;

		case BlendMode::kNormal:
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			break;

		case BlendMode::kAdd:
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			break;

		case BlendMode::kSubtract:
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			break;

		case BlendMode::kMultiply:
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
			break;

		case BlendMode::kScreen:
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			break;
		}

		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

		return blendDesc;
	}
}
