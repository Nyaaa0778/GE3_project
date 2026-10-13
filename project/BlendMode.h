#pragma once

#include <d3d12.h>

//================================================================================
// Blend Mode
//================================================================================

enum class BlendMode {
	kNone,             // なし
	kNormal,           // 通常
	kAdd,              // 加算
	kSubtract,         // 減算
	kMultiply,         // 乗算
	kScreen,           // スクリーン
	kCountOfBlendMode, // カウント用
};

namespace BlendModeHelper {
	D3D12_BLEND_DESC MakeBlendDesc(BlendMode mode);
}