#pragma once

// ライトの種類
enum class LightingType {
	kNone,         // 0: ライティングなし
	kLambert,      // 1: ランバート
	kHalfLambert,  // 2: ハーフランバート
	kPhong,        // 3: フォン
	kBlinnPhong,   // 4: ブリン・フォン
};