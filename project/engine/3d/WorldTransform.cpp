#include "WorldTransform.h"
#include "DirectXCommon.h"
#include "MathUtility.h"

using namespace MathUtility;

void WorldTransform::Initialize() {
	// DirectXCommonのインスタンスを取得して定数バッファを作成
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	constBuffer = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));

	// マッピング
	constBuffer->Map(0, nullptr, reinterpret_cast<void**>(&constMap));

	// 単位行列で初期化
	matWorld = MakeIdentityMatrix();
	if (constMap) {
		constMap->WVP = MakeIdentityMatrix();
		constMap->World = MakeIdentityMatrix();
		constMap->WorldInverseTranspose = MakeIdentityMatrix();
	}
}

void WorldTransform::UpdateMatrix() {
	// 1. ローカル行列の計算
	Matrix4x4 matLocal = MakeAffineMatrix(scale, rotation, translation);

	// 2. 階層構造（親子関係）の処理
	if (parent) {
		matWorld = matLocal * parent->matWorld;
	} else {
		matWorld = matLocal;
	}

	// 3. 定数バッファへの書き込み
	if (constMap) {
		constMap->World = matWorld;
		constMap->WorldInverseTranspose = MakeTransposeMatrix(MakeInverseMatrix(matWorld));
	}
}
