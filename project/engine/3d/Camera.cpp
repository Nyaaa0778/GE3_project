#include "Camera.h"

#include "MathUtility.h"
#include "DirectXCommon.h"

using namespace MathUtility;

Camera::Camera()
	: transform_({{1.0f, 1.0f, 1.0f},
				 {0.0f, 0.0f, 0.0f},
				 {0.0f, 0.0f, 0.0f}}), // transform
	fovY_(0.45f),                     // 水平方向視野角
	aspectRatio_(static_cast<float>(DirectXCommon::GetInstance()->GetClientWidth()) /
				 static_cast<float>(DirectXCommon::GetInstance()->GetClientHeight())), // アスペクト比
	nearClip_(0.1f),  // ニアクリップ距離
	farClip_(100.0f), // ファークリップ距離
	worldMatrix_(MakeAffineMatrix(transform_.scale, transform_.rotation,
								  transform_.translation)), // ワールド行列
	viewMatrix_(MakeInverseMatrix(worldMatrix_)),           // ビュー行列
	projectionMatrix_(fovY_, aspectRatio_, nearClip_,
					  farClip_), // プロジェクション行列
	viewProjectionMatrix_(viewMatrix_*
						  projectionMatrix_) // ビュープロジェクション行列
{
	CreateConstantBuffer();
}

//void Camera::Update() {
//	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotation,
//		transform_.translation);
//	viewMatrix_ = MakeInverseMatrix(worldMatrix_);
//	projectionMatrix_ =
//		MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);
//	viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
//
//	if (cameraData_) {
//		cameraData_->worldPosition = transform_.translation;
//	}
//}

void Camera::CalculateMatrix() {
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotation, transform_.translation);
	viewMatrix_ = MakeInverseMatrix(worldMatrix_);
	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);
	viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;

	if (cameraData_) {
		cameraData_->worldPosition = transform_.translation;
	}
}

void Camera::CreateConstantBuffer() {
	// 定数バッファの作成
	cameraBuffer_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(CameraForGPU));

	// マッピングしてC++から書き込めるようにする
	cameraBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

	// 初期値を書き込む
	cameraData_->worldPosition = transform_.translation;
}