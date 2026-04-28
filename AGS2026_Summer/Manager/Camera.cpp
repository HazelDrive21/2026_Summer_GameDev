#include <math.h>
#include <DxLib.h>
#include <EffekseerForDXLib.h>
#include "../Utility/AsoUtility.h"
#include "../Manager/InputManager.h"
#include "../Object/Common/Transform.h"
#include "../Manager/SceneManager.h"
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

	angles_.x = 0.0f;
	angles_.y = 0.0f;
	angles_.z = 0.0f;

	rot_ = Quaternion();

}

void Camera::SyncFollow(void)
{
	if (followTransform_ == nullptr) return;

	// --- AC2AA: プレイヤーの回転(Y軸)をカメラの角度(Y軸)に強制同期 ---
	// これにより「カメラが追従しない」問題が解決し、機体の後ろに固定されます
	angles_.y = followTransform_->quaRot.ToEuler().y;

	// プレイヤーの中心点
	VECTOR playerRotationCenter = VAdd(followTransform_->pos, VGet(0, 100.0f, 0.0f)); // 高さは調整してください

	// 目標の回転を反映
	rot_ = Quaternion::Euler(angles_.x, angles_.y, 0.0f);

	// 遅延追従（interpRotationCenter_）の計算
	interpRotationCenter_ = AsoUtility::Lerp(interpRotationCenter_, playerRotationCenter, followLerpRate_);

	// 計算された中心を基準にカメラ座標を決定
	MATRIX rotMat = rot_.ToMatrix();
	pos_ = VAdd(interpRotationCenter_, VTransform(LOCAL_F2C_POS, rotMat));
	targetPos_ = VAdd(interpRotationCenter_, VTransform(LOCAL_F2T_POS, rotMat));

	cameraUp_ = VGet(0, 1, 0);
}

void Camera::ProcessRot(void)
{
	auto& ins = InputManager::GetInstance();
	float deltaTime = SceneManager::GetInstance().GetDeltaTime(); // デルタタイム取得
	float lookSpeed = 0.02f;

	// タイマーを減らす
	if (resetWaitTimer_ > 0.0f) {
		resetWaitTimer_ -= deltaTime;
	}

	bool lTrigger = ins.IsPadBtnNew(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::L_TRIGGER);
	bool rTrigger = ins.IsPadBtnNew(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::R_TRIGGER);

	// --- 1. 同時押し：リセット中 ---
	if (lTrigger && rTrigger)
	{
		angles_.x = AsoUtility::Lerp(angles_.x, 0.0f, 0.15f);

		// リセット中はタイマーを常に最大値で固定し、操作を拒否し続ける
		resetWaitTimer_ = RESET_WAIT_TIME;

		if (abs(angles_.x) < 0.001f) angles_.x = 0.0f;
	}
	// --- 2. 通常の上下操作（タイマーが0の時だけ受け付ける） ---
	else if (resetWaitTimer_ <= 0.0f)
	{
		if (lTrigger)
		{
			angles_.x -= lookSpeed;
		}
		else if (rTrigger)
		{
			angles_.x += lookSpeed;
		}
	}

	// 上下制限と回転の適用
	if (angles_.x > LIMIT_X_UP_RAD)   angles_.x = LIMIT_X_UP_RAD;
	if (angles_.x < -LIMIT_X_DW_RAD)  angles_.x = -LIMIT_X_DW_RAD;

	rot_ = Quaternion::Euler(angles_.x, angles_.y, 0.0f);
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
