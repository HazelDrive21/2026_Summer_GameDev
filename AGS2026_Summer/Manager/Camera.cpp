#include <math.h>
#include <DxLib.h>
#include <EffekseerForDXLib.h>
#include "../Utility/AsoUtility.h"
#include "../Manager/InputManager.h"
#include "../Object/Common/Transform.h"
#include "../Application.h"
#include "Camera.h"

Camera::Camera(void)
{
	angles_ = VECTOR();
	cameraUp_ = VECTOR();
	mode_ = MODE::NONE;
	pos_ = AsoUtility::VECTOR_ZERO;
	targetPos_ = AsoUtility::VECTOR_ZERO;
	followTransform_ = nullptr;
}

Camera::~Camera(void)
{
}

void Camera::Init(void)
{

	ChangeMode(MODE::FOLLOW);

	// 完全にリセット
	interpRotationCenter_ = AsoUtility::VECTOR_ZERO;
	pos_ = AsoUtility::VECTOR_ZERO;
	targetPos_ = AsoUtility::VECTOR_ZERO;
	followTransform_ = nullptr; // 追従対象も一度クリア

	// 回転角のリセット
	angles_ = VGet(AsoUtility::Deg2RadF(0.0f), 0.0f, 0.0f);
	rot_ = Quaternion::Euler(angles_.x, angles_.y, angles_.z);

}

void Camera::Update(void)
{
}

void Camera::SetBeforeDraw(void)
{

	// クリップ距離を設定する(SetDrawScreenでリセットされる)
	SetCameraNearFar(CAMERA_NEAR, CAMERA_FAR);

	switch (mode_)
	{
	case Camera::MODE::FIXED_POINT:
		SetBeforeDrawFixedPoint();
		break;
	case Camera::MODE::FOLLOW:
		SetBeforeDrawFollow();
		break;
	}

	// カメラの設定(位置と注視点による制御)
	SetCameraPositionAndTargetAndUpVec(
		pos_, 
		targetPos_, 
		cameraUp_
	);

	// DXライブラリのカメラとEffekseerのカメラを同期する。
	Effekseer_Sync3DSetting();

}

void Camera::Draw(void)
{
	// デバッグ表示：カメラの座標と注視点を画面左上に表示
	DrawFormatString(0, 0, GetColor(255, 255, 255), "Camera Pos    : X=%.1f Y=%.1f Z=%.1f", pos_.x, pos_.y, pos_.z);
	DrawFormatString(0, 20, GetColor(255, 255, 255), "Camera Target : X=%.1f Y=%.1f Z=%.1f", targetPos_.x, targetPos_.y, targetPos_.z);

	// 現在の相対設定値も表示しておくと調整しやすいです
	DrawFormatString(0, 40, GetColor(0, 255, 255), "F2C (Current Setting) : X=%.1f Y=%.1f Z=%.1f", LOCAL_F2C_POS.x, LOCAL_F2C_POS.y, LOCAL_F2C_POS.z);
}

void Camera::SetFollow(const Transform* follow)
{
	followTransform_ = follow;

	if (followTransform_ != nullptr)
	{
		// 【重要】セットされた瞬間に、遅延追従の計算をスキップして座標を同期させる
		// これにより、(0,0,0)や前回終了地点からプレイヤーへ飛んでいく挙動を防ぎます
		interpRotationCenter_ = followTransform_->pos;

		// もしSyncFollow内でオフセット（高さなど）を加算しているなら、ここでも合わせる
		interpRotationCenter_ = VAdd(followTransform_->pos, VGet(0, 250.0f, 0.0f));

		// その場で即座に最終的な pos_ と targetPos_ を確定させる
		SyncFollow();
	}
}

VECTOR Camera::GetPos(void) const
{
	return pos_;
}

VECTOR Camera::GetAngles(void) const
{
	return angles_;
}

VECTOR Camera::GetTargetPos(void) const
{
	return targetPos_;
}

Quaternion Camera::GetQuaRot(void) const
{
	return rot_;
}

Quaternion Camera::GetQuaRotOutX(void) const
{
	return rotOutX_;
}

VECTOR Camera::GetForward(void) const
{
	return VNorm(VSub(targetPos_, pos_));
}

void Camera::ChangeMode(MODE mode)
{

	// カメラの初期設定
	SetDefault();

	// カメラモードの変更
	mode_ = mode;

	// 変更時の初期化処理
	switch (mode_)
	{
	case Camera::MODE::FIXED_POINT:
		break;
	case Camera::MODE::FOLLOW:
		break;
	}

}

void Camera::SetDefault(void)
{

	// カメラの初期設定
	pos_ = DEFAULT_CAMERA_POS;

	// 注視点
	targetPos_ = AsoUtility::VECTOR_ZERO;

	// カメラの上方向
	cameraUp_ = AsoUtility::DIR_U;

	angles_.x = AsoUtility::Deg2RadF(30.0f);
	angles_.y = 0.0f;
	angles_.z = 0.0f;

	rot_ = Quaternion();

}

void Camera::SyncFollow(void) 
{
	if (followTransform_ == nullptr) return;

	// A. プレイヤーの現在の「真の」中心点（高さオフセット込み）
	VECTOR playerRotationCenter = VAdd(followTransform_->pos, VGet(0, 250.0f, 0.0f));

	// B. 目標地点の計算
	VECTOR goalPos;

	// 現在の仮想中心とプレイヤーの距離
	VECTOR diff = VSub(playerRotationCenter, interpRotationCenter_);
	float distance = VSize(diff);

	if (distance <= followDeadZone_) {
		// 【重要】遊びの範囲内であっても、少しずつプレイヤーに近づける
		// これにより、停止した時にピタッと中央へ戻るようになります。
		// 第3引数の値を小さくすると、停止時の戻りがゆっくりになります。
		goalPos = AsoUtility::Lerp(interpRotationCenter_, playerRotationCenter, 0.5f);
	}
	else {
		goalPos = AsoUtility::Lerp(interpRotationCenter_, playerRotationCenter, 0.5f);
	}

	// C. 遅延追従（Lerp）を実行
	interpRotationCenter_ = AsoUtility::Lerp(interpRotationCenter_, goalPos, followLerpRate_);

	// --- 以下、計算された interpRotationCenter_ を基準にカメラ座標を決定 ---
	MATRIX rotMat = rot_.ToMatrix();
	pos_ = VAdd(interpRotationCenter_, VTransform(LOCAL_F2C_POS, rotMat));
	targetPos_ = VAdd(interpRotationCenter_, VTransform(LOCAL_F2T_POS, rotMat));

	cameraUp_ = VGet(0, 1, 0);
}

void Camera::ProcessRot(void)
{
	// マウスの現在の座標を取得
	int mouseX, mouseY;
	GetMousePoint(&mouseX, &mouseY);

	// 画面中央の座標
	int centerX = Application::SCREEN_SIZE_X / 2;
	int centerY = Application::SCREEN_SIZE_Y / 2;

	// 中央からの移動量を計算
	int diffX = mouseX - centerX;
	int diffY = mouseY - centerY;

	// マウスを中央に戻す（これで無限に回転可能になる）
	SetMousePoint(centerX, centerY);

	// マウス感度（好みに合わせて調整）
	float sensitivity = 0.0015f;

	// 左右回転（Y軸まわりの回転）
	angles_.y += diffX * rotationSpeed_;
	// 上下回転（X軸まわりの回転）
	angles_.x += diffY * rotationSpeed_;

	rot_ = Quaternion::Euler(angles_.x, angles_.y, angles_.z);

	// X軸（上下）の回転制限
	if (angles_.x > LIMIT_X_UP_RAD)   angles_.x = LIMIT_X_UP_RAD;
	if (angles_.x < -LIMIT_X_DW_RAD)  angles_.x = -LIMIT_X_DW_RAD;
}

void Camera::SetBeforeDrawFixedPoint(void)
{
	// 何もしない
}

void Camera::SetBeforeDrawFollow(void)
{

	// カメラ操作
	ProcessRot();

	// 追従対象との相対位置を同期
	SyncFollow();

}

void Camera::SetBeforeDrawSelfShot(void)
{
}
