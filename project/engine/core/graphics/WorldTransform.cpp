#include "WorldTransform.h"
#include "DirectXCommon.h"
#include "MathUtility.h"

void WorldTransform::Initialize() {
	// 定数バッファの作成
	constBuffer = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));

	// マッピングしてアドレスを取得
	constBuffer->Map(0, nullptr, reinterpret_cast<void**>(&constMap));

	// 行列の初期化
	matWorld = MathUtility::MakeIdentityMatrix();
	if (constMap) {
		constMap->WVP = MathUtility::MakeIdentityMatrix();
		constMap->World = MathUtility::MakeIdentityMatrix();
		constMap->WorldInverseTranspose = MathUtility::MakeIdentityMatrix();
	}
}

void WorldTransform::UpdateMatrix() {
	// アフィン変換行列の作成
	matWorld = MathUtility::MakeAffineMatrix(scale, rotation, translation);

	// 親があれば合成する
	if (parent) {
		matWorld = MathUtility::Multiply(matWorld, parent->matWorld);
	}

	// 転送（GPUバッファへの書き込み）
	if (constMap) {
		constMap->World = matWorld;
		constMap->WorldInverseTranspose = MathUtility::MakeTransposeMatrix(MathUtility::MakeInverseMatrix(matWorld));
	}
}
