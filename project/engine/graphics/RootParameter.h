#pragma once
#include <cstdint>

// 共通ルートシグネチャのパラメータスロット定義
enum RootParameterIndex : uint32_t {
	kRootParamMaterialOrParams = 0,    // [0] PS b0 (3Dマテリアル / スプライト色 / ポストプロセスパラメータ)
	kRootParamTransform = 1,           // [1] VS b0 (ワールド座標変換行列)
	kRootParamCamera = 2,              // [2] VS / PS b1 (カメラ)
	kRootParamLight = 3,               // [3] PS b2 (ライト)
	kRootParamMainTexture = 4,         // [4] PS t0 (メインテクスチャ)
	kRootParamEnvironmentOrExtra = 5,  // [5] PS t1 (環境マップ / 追加マスクテクスチャ)
	kRootParamDissolveMask = 6,        // [6] PS t2 (3Dディゾルブ用マスク)
};
