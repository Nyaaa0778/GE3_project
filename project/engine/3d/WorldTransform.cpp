#include "WorldTransform.h"
#include "DirectXCommon.h"
#include "MathUtility.h"

using namespace MathUtility;

void WorldTransform::Initialize() {
	// 定数バッファを作成
	constBuffer = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(ConstBufferDataWorldTransform));

	// 定数バッファをマップ
	constBuffer->Map(0, nullptr, reinterpret_cast<void**>(&constMap));

	// 単位行列で初期化
	constMap->WVP = MakeIdentityMatrix();
	constMap->World = MakeIdentityMatrix();
	constMap->WorldInverseTranspose = MakeIdentityMatrix();

	matWorld = MakeIdentityMatrix();
}

void WorldTransform::UpdateMatrix() {
	// スケール・回転・平行移動からローカルアフィン変換行列を計算
	matWorld = MakeAffineMatrix(scale, rotation, translation);

	// 親があれば合成
	if (parent) {
		matWorld = matWorld * parent->matWorld;
	}

	// 定数バッファに転送 (WVP はカメラ情報が必要なので Object3d 側で転送する)
	constMap->World = matWorld;
	constMap->WorldInverseTranspose = MakeTransposeMatrix(MakeInverseMatrix(matWorld));
}
