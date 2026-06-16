#include "WorldTransform.h"
#include "DirectXCommon.h"
#include "MathUtility.h"

#include "Camera.h"

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
    matWorld = MathUtility::MakeAffineMatrix(scale, rotation, translation);

    if (parent) {
        matWorld = MathUtility::Multiply(matWorld, parent->matWorld);
    }

    if (constMap) {
        constMap->World = matWorld;
        // cameraがセットされていればWVPを計算
        if (camera_) {
            constMap->WVP = MathUtility::Multiply(matWorld, camera_->GetViewProjectionMatrix());
        }
        constMap->WorldInverseTranspose = MathUtility::MakeTransposeMatrix(MathUtility::MakeInverseMatrix(matWorld));
    }
}

Vector3 WorldTransform::GetWorldPosition() const {
	return { matWorld.m[3][0], matWorld.m[3][1], matWorld.m[3][2] };
}