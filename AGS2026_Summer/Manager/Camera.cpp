#include <math.h>
#include <DxLib.h>
#include <EffekseerForDXLib.h>
#include "../Utility/AsoUtility.h"
#include "../Manager/InputManager.h"
#include "../Object/Common/Transform.h"
#include "../Manager/SceneManager.h"
#include "../Application.h"
#include "../Object/Player.h"
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

	// ゲーム開始時やシーン切り替え時にカメラが遠くから飛んでくるのを防ぐフラグ
	isFirstFollow_ = true;

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
	/*DrawFormatString(0, 0, GetColor(255, 255, 255), "Camera Pos    : X=%.1f Y=%.1f Z=%.1f", pos_.x, pos_.y, pos_.z);
	DrawFormatString(0, 20, GetColor(255, 255, 255), "Camera Target : X=%.1f Y=%.1f Z=%.1f", targetPos_.x, targetPos_.y, targetPos_.z);

	// 現在の相対設定値も表示しておくと調整しやすいです
	DrawFormatString(0, 40, GetColor(0, 255, 255), "F2C (Current Setting) : X=%.1f Y=%.1f Z=%.1f", LOCAL_F2C_POS.x, LOCAL_F2C_POS.y, LOCAL_F2C_POS.z);*/
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

	float deltaTime = SceneManager::GetInstance().GetDeltaTime();

	// 左右の旋回角（Y軸回転）は、プレイヤーの回転をそのまま100%同期
	angles_.y = followTransform_->rot.y;

	// 回転行列の作成
	MATRIX rotMat = MGetRotX(angles_.x);
	rotMat = MMult(rotMat, MGetRotY(angles_.y));

	if (isFirstFollow_)
	{
		interpRotationCenter_ = followTransform_->pos;
		isFirstFollow_ = false;
	}
	else
	{
		// プレイヤーの正面方向（F）と右方向（R）の単位ベクトル
		VECTOR dirF = VTransform(VGet(0, 0, 1), MGetRotY(angles_.y));
		VECTOR dirR = VTransform(VGet(1, 0, 0), MGetRotY(angles_.y));

		// 現時点のカメラ中心点からプレイヤーへのワールド差分
		VECTOR toPlayer = VSub(followTransform_->pos, interpRotationCenter_);

		// 各ローカル距離に分解
		float diffF = toPlayer.x * dirF.x + toPlayer.y * dirF.y + toPlayer.z * dirF.z;
		float diffR = toPlayer.x * dirR.x + toPlayer.y * dirR.y + toPlayer.z * dirR.z;
		float diffY = toPlayer.y;

		// --------------------------------------------------------
		// 1. 【前後（Z軸）追従】★前進と後退で処理を完全分離
		// --------------------------------------------------------
		float rateF = 0.0;

		if (diffF >= 0.0f)
		{
			// 【前進（奥へ行く動き）】とことんルーズに、深く食い込ませる

			float ratioF = diffF / COMFORT_ZONE_F_FRONT;
			if (ratioF > 1.0f) ratioF = 1.0f;

			// 通常時は 0.015f という超極小の力で引っ張る（ヌルヌル感を極限に）
			float baseRateF = AsoUtility::Lerp(0.015f, 0.20f, ratioF);
			rateF = baseRateF * deltaTime * 60.0f;
		}
		else
		{
			// 【後退（手前に迫る動き）】カメラの突き抜けを防ぐためタイトに追従

			float ratioF = fabsf(diffF) / COMFORT_ZONE_F_BACK;
			if (ratioF > 1.0f) ratioF = 1.0f;

			// 手前に来たら 0.12f〜0.45f という強い力ですぐに押し返す
			float baseRateF = AsoUtility::Lerp(0.12f, 0.45f, ratioF);
			rateF = baseRateF * deltaTime * 60.0f;
		}
		if (rateF > 1.0f) rateF = 1.0f;

		// --------------------------------------------------------
		// 2. 【左右（X軸）追従】（前回の仕様を維持：タイト）
		// --------------------------------------------------------
		float baseRateR = 0.09f;
		if (fabsf(diffR) > 0.0f)
		{
			float ratioR = fabsf(diffR) / COMFORT_ZONE_R;
			if (ratioR > 1.0f) ratioR = 1.0f;
			baseRateR = AsoUtility::Lerp(0.09f, 0.35f, ratioR);
		}
		float rateR = baseRateR * deltaTime * 60.0f;
		if (rateR > 1.0f) rateR = 1.0f;

		// --------------------------------------------------------
		// 3. 【上下（Y軸）追従】（前回の仕様を維持）
		// --------------------------------------------------------
		float baseRateY = 0.08f;
		if (fabsf(diffY) > 0.0f)
		{
			float ratioY = fabsf(diffY) / COMFORT_ZONE_Y;
			if (ratioY > 1.0f) ratioY = 1.0f;
			baseRateY = AsoUtility::Lerp(0.08f, 0.35f, ratioY);
		}
		float rateY = baseRateY * deltaTime * 60.0f;
		if (rateY > 1.0f) rateY = 1.0f;

		// --------------------------------------------------------
		// 4. 各ローカル軸の移動量をワールド座標に還元して中心点を動かす
		// --------------------------------------------------------
		float moveF = diffF * rateF;
		float moveR = diffR * rateR;
		float moveY = diffY * rateY;

		interpRotationCenter_ = VAdd(interpRotationCenter_, VScale(dirF, moveF));
		interpRotationCenter_ = VAdd(interpRotationCenter_, VScale(dirR, moveR));
		interpRotationCenter_.y += moveY;
	}

	// --------------------------------------------------------
	// 5. 【各軸独立の絶対安全ガード】★前後を非対称ガードに修正
	// --------------------------------------------------------
	VECTOR centerToPlayer = VSub(followTransform_->pos, interpRotationCenter_);
	VECTOR dirF = VTransform(VGet(0, 0, 1), MGetRotY(angles_.y));
	VECTOR dirR = VTransform(VGet(1, 0, 0), MGetRotY(angles_.y));

	float checkF = centerToPlayer.x * dirF.x + centerToPlayer.y * dirF.y + centerToPlayer.z * dirF.z;
	float checkR = centerToPlayer.x * dirR.x + centerToPlayer.y * dirR.y + centerToPlayer.z * dirR.z;

	// 前方のガード
	if (checkF > ABSOLUTE_MAX_F_FRONT)
	{
		interpRotationCenter_ = VAdd(interpRotationCenter_, VScale(dirF, checkF - ABSOLUTE_MAX_F_FRONT));
	}
	// 後方のガード（カメラの突き抜け防止）
	else if (checkF < -ABSOLUTE_MAX_F_BACK)
	{
		interpRotationCenter_ = VAdd(interpRotationCenter_, VScale(dirF, checkF - (-ABSOLUTE_MAX_F_BACK)));
	}

	if (fabsf(checkR) > ABSOLUTE_MAX_R)
	{
		float clampR = (checkR > 0.0f) ? ABSOLUTE_MAX_R : -ABSOLUTE_MAX_R;
		interpRotationCenter_ = VAdd(interpRotationCenter_, VScale(dirR, checkR - clampR));
	}
	if (fabsf(centerToPlayer.y) > ABSOLUTE_MAX_Y)
	{
		if (centerToPlayer.y > 0.0f) interpRotationCenter_.y = followTransform_->pos.y - ABSOLUTE_MAX_Y;
		else                         interpRotationCenter_.y = followTransform_->pos.y + ABSOLUTE_MAX_Y;
	}

	// 6. 位置と注視点の算出（遅延中心点ベース）
	pos_ = VAdd(interpRotationCenter_, VTransform(LOCAL_F2C_POS, rotMat));
	targetPos_ = VAdd(interpRotationCenter_, VTransform(LOCAL_F2T_POS, rotMat));

	cameraUp_ = VGet(0, 1, 0);

	// DxLibのカメラに座標を即座に反映
	SetCameraPositionAndTargetAndUpVec(pos_, targetPos_, cameraUp_);
}

void Camera::ProcessRot(void)
{
	auto& ins = InputManager::GetInstance();
	float deltaTime = SceneManager::GetInstance().GetDeltaTime();

	// ★【今回の修正】リセット後の操作不能タイマーのカウントダウン処理
	if (resetWaitTimer_ > 0.0f)
	{
		resetWaitTimer_ -= deltaTime;
		if (resetWaitTimer_ < 0.0f) resetWaitTimer_ = 0.0f;
	}

	bool isL2 = ins.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::L_TRIGGER);
	bool isR2 = ins.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::R_TRIGGER);

	// カメラの上下旋回スピード（ラジアン/秒）
	constexpr float CAM_LOOK_SPEED = 1.2f;

	// 1. 同時押し：水平リセット（0.0f に向かって滑らかに補間）
	if (isL2 && isR2)
	{
		angles_.x = AsoUtility::Lerp(angles_.x, 0.0f, 0.15f * deltaTime * 60.0f);

		// ★同時押しされている間は、常にタイマーを最大値(0.1秒)で維持し続ける
		resetWaitTimer_ = RESET_WAIT_TIME;

		if (abs(angles_.x) < 0.001f) angles_.x = 0.0f;
	}
	// 2. 単体押し：上下を見上げる・見下ろす（★タイマーが 0 になっている時だけ受け付ける）
	else if (resetWaitTimer_ <= 0.0f)
	{
		if (isL2)
		{
			angles_.x -= CAM_LOOK_SPEED * deltaTime;
		}
		else if (isR2)
		{
			angles_.x += CAM_LOOK_SPEED * deltaTime;
		}
	}

	// X軸（上下）の回転制限
	if (angles_.x > LIMIT_X_UP_RAD)  angles_.x = LIMIT_X_UP_RAD;
	if (angles_.x < -LIMIT_X_DW_RAD) angles_.x = -LIMIT_X_DW_RAD;

	// クォータニオンの更新
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