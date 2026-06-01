#include <DxLib.h>
#include <string>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/InputManager.h"
#include "../Manager/SceneManager.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/Camera.h"
#include "Common/AnimationController.h"
#include "../Object/Collider/ColliderLine.h"
#include "../Object/Collider/ColliderCapsule.h"
#include "../Object/Collider/ColliderModel.h"
#include "../Object/Enemy/EnemyManager.h"
#include "FCS.h"
#include "Player.h"

Player::Player(void)
{

	animationController_ = nullptr;
	fcs_ = nullptr;
	state_ = STATE::NONE;

	speed_ = 0.0f;
	moveDir_ = AsoUtility::VECTOR_ZERO;
	movePow_ = AsoUtility::VECTOR_ZERO;
	movedPos_ = AsoUtility::VECTOR_ZERO;

	playerRotY_ = Quaternion();
	goalQuaRot_ = Quaternion();
	stepRotTime_ = 0.0f;

	jumpPow_ = AsoUtility::VECTOR_ZERO;
	isJump_ = false;
	stepJump_ = 0.0f;

	// 衝突チェック
	gravHitPosDown_ = AsoUtility::VECTOR_ZERO;
	gravHitPosUp_ = AsoUtility::VECTOR_ZERO;

	imgShadow_ = -1;

	currentTurnSpeed_ = DEFAULT_TURN_SPEED;

	airDashTime_ = 0.0f;

	en_ = MAX_EN;

	maxHp_ = 4000;
	hp_ = maxHp_;

	isCharging_ = false;

	// 例：名前"RIFLE", 総弾数, 発射間隔, 弾速, 威力, 寿命
	rightWeapon_ = new WeaponFirearm("MACHINE GUN", 800, 5, 100.0f, 50, 600);

}

Player::~Player(void)
{

	// 武器の解放
	if (rightWeapon_ != nullptr) { delete rightWeapon_; }

	// 残っている弾の解放
	for (auto bullet : activeBullets_) { delete bullet; }
	activeBullets_.clear();

	delete animationController_;
	delete fcs_;
}

void Player::ApplyDamage(int damage)
{
	// すでに死亡している場合は重ねて処理しない
	if (hp_ <= 0) return;

	hp_ -= damage;

	// マイナスにならないようにクランプ
	if (hp_ < 0)
	{
		hp_ = 0;
		// 必要であれば、ここに死亡状態（STATE::DEADなど）への遷移を書く
	}
}

void Player::InitLoad(void)
{
	transform_.SetModel(
		resMng_.LoadModelDuplicate(ResourceManager::SRC::PLAYER));

	// 丸影画像
	imgShadow_ = resMng_.Load(ResourceManager::SRC::PLAYER_SHADOW).handleId_;
}

void Player::InitTransform(void)
{
	transform_.scl = { 8.0f,8.0f,8.0f };
	transform_.pos = { 0.0f, 5000.0f, 0.0f };
	transform_.quaRot = Quaternion();
	transform_.quaRotLocal = Quaternion::Euler({ 0.0f, AsoUtility::Deg2RadF(180.0f), 0.0f });
	transform_.SetEmissive(GetColorF(0.0f, 0.5f, 1.0f, 1.0f), 1);
	transform_.Update();
}

void Player::InitCollider(void)
{
	// 主に地面との衝突で仕様する線分コライダ
	ColliderLine* colLine = new ColliderLine(
		ColliderBase::TAG::PLAYER, &transform_,
		COL_LINE_START_LOCAL_POS, COL_LINE_END_LOCAL_POS);
	ownColliders_.emplace(static_cast<int>(COLLIDER_TYPE::LINE), colLine);

	// 主に壁や木などの衝突で仕様するカプセルコライダ
	ColliderCapsule* colCapsule = new ColliderCapsule(
		ColliderBase::TAG::PLAYER, &transform_,
		COL_CAPSULE_TOP_LOCAL_POS, COL_CAPSULE_DOWN_LOCAL_POS,
		COL_CAPSULE_RADIUS);
	ownColliders_.emplace(static_cast<int>(COLLIDER_TYPE::CAPSULE), colCapsule);
}

void Player::InitAnimation(void)
{

	std::string path = Application::PATH_MODEL + "Player/";
	animationController_ = new AnimationController(transform_.modelId);
	animationController_->Add((int)ANIM_TYPE::IDLE, path + "Idle.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::RUN, path + "Run.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::FAST_RUN, path + "FastRun.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::JUMP, path + "Jump.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::WARP_PAUSE, path + "WarpPose.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::FLY, path + "Flying.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::FALLING, path + "Falling.mv1", 80.0f);
	animationController_->Add((int)ANIM_TYPE::VICTORY, path + "Victory.mv1", 60.0f);

	animationController_->Play((int)ANIM_TYPE::IDLE);

}

void Player::InitPost(void)
{
	speed_ = 0.0f;
	moveDir_ = VGet(0, 0, 1); // 初期方向をZ正(正面)に設定
	movePow_ = AsoUtility::VECTOR_ZERO;
	jumpPow_ = AsoUtility::VECTOR_ZERO;
	dashResidualTimer_ = 0.0f;

	// 高度が空中（例えば Y > 100.0f）なら、最初から空中状態にする
	if (transform_.pos.y > 140.0f)
	{
		isJump_ = true;
		isGrounded_ = false;
	}
	else
	{
		isJump_ = false;
		isGrounded_ = true;
	}

	isBoostAscent_ = false;

	playerRotY_ = transform_.quaRot;
	goalQuaRot_ = transform_.quaRot;

	// ★★★ FCSの生成と初期化 ★★★
	fcs_ = new FCS();
	fcs_->Init();
	fcs_->SetPlayer(this); // FCS側に自分（Player）への参照を渡す

	SetUseLighting(FALSE);

	en_ = MAX_EN;

	ChangeState(STATE::PLAY);
}

void Player::UpdateProcess(void)
{
	float currentTurnSpeed = 0.0001f; // 本来はパーツのステータスから取得

	auto* camera = SceneManager::GetInstance().GetCamera();
	camera->SetRotationSpeed(currentTurnSpeed);

	// ★ 1. 硬直中であっても、パッドによる旋回入力は常に受け付ける
	if (state_ == STATE::PLAY || state_ == STATE::STOP || state_ == STATE::LANDING_STIFF)
	{
		ProcessTurn();
	}

	// 2. 更新ステップ（ここには純粋な移動や硬直タイマーの処理だけが残る）
	switch (state_)
	{
	case Player::STATE::NONE:
		UpdateNone();
		break;
	case Player::STATE::PLAY:
		UpdatePlay();
		break;
	case Player::STATE::STOP:
		UpdateStop();
		break;
	case Player::STATE::LANDING_STIFF:
		UpdateLandingStiff();
		break;
	}

	// ★ 3. 硬直中であっても、FCS・武器・弾丸は常に更新・発射できるようにする
	if (state_ == STATE::PLAY || state_ == STATE::STOP || state_ == STATE::LANDING_STIFF)
	{
		UpdateEnergy(scnMng_.GetDeltaTime());
		UpdateCommonMechanics();
	}
}

void Player::UpdateProcessPost(void)
{
}

void Player::Draw(void)
{

	CharactorBase::Draw();

#ifdef _DEBUG
	DrawFormatString(0, 180, GetColor(255, 255, 255),
		"Pos: X=%.1f Y=%.1f Z=%.1f",
		transform_.pos.x, transform_.pos.y, transform_.pos.z);
	DrawFormatString(0, 200, GetColor(255, 255, 255), "jumpPow.y: %.2f", jumpPow_.y);
	DrawFormatString(0, 220, GetColor(255, 255, 255), "MoveSpeed: %.2f", debugCurrentSpeed_);
	DrawFormatString(0, 240, GetColor(255, 255, 255), "gravityScale: %.1f", gravityScale_);
	DrawFormatString(0, 260, GetColor(255, 255, 255), "isGrounded: %d", isGrounded_ ? 1 : 0);
	DrawFormatString(0, 280, GetColor(255, 255, 255), "EN: %.1f / %.1f", en_, MAX_EN);
#endif // _DEBUG

	// モデルの描画
	MV1DrawModel(transform_.modelId);

	// 丸影描画
	DrawShadow();

	for (auto bullet : activeBullets_)
	{
		if (bullet != nullptr)
		{
			bullet->Draw();
		}
	}

	// 画面左下にHP（AP）をメーターと数値で表示する例
	int barX = 50;
	int barY = Application::SCREEN_SIZE_Y - 80;
	int barWidth = 300;
	int barHeight = 15;

	// ① HPバーの背景（黒い枠）
	DrawBox(barX, barY, barX + barWidth, barY + barHeight, GetColor(50, 50, 50), TRUE);

	// ② 現在のHPの割合を計算
	float hpRate = static_cast<float>(hp_) / maxHp_;
	int currentBarWidth = static_cast<int>(barWidth * hpRate);

	// ③ HPバーの本体（AC2AA風なら緑や青緑系、ピンチで赤にするなど）
	unsigned int barColor = GetColor(0, 255, 200); // 通常時はシアン（青緑）
	if (hpRate < 0.3f)
	{
		barColor = GetColor(255, 0, 0); // 残り3割以下で赤ゲージに
	}
	DrawBox(barX, barY, barX + currentBarWidth, barY + barHeight, barColor, TRUE);

	// ④ 文字で数値を表示
	// AC風に "AP  8000" のような表記にすると気分が上がります！
	DrawFormatString(barX, barY - 25, GetColor(255, 255, 255), "AP %4d / %4d", hp_, maxHp_);

	//DrawFormatString(0, 300, GetColor(255, 0, 0), "isCharging: %d", isCharging_ ? 1 : 0);

	/*DrawFormatString(0, 80, GetColor(255, 255, 255), "Timer: %f", dashResidualTimer_);
	DrawFormatString(0, 100, GetColor(255, 255, 255), "isJump: %d", isJump_ ? 1 : 0);
	DrawFormatString(0, 120, GetColor(255, 255, 255), "Speed: %f", speed_);
	DrawFormatString(0, 140, GetColor(255, 255, 255), "DashDuration: %f", dashPressDuration_);
	DrawFormatString(0, 160, GetColor(255, 255, 255), "isDashPress: %d", isDashKeyPress_ ? 1 : 0);
	DrawFormatString(0, 180, GetColor(255, 255, 255), "isBoostAscent: %d", isBoostAscent_ ? 1 : 0);*/

}

void Player::ChangeState(STATE state)
{

	// 状態変更
	state_ = state;

	// 各状態遷移の初期処理
	switch (state_)
	{
	case Player::STATE::NONE:
		ChangeStateNone();
		break;
	case Player::STATE::PLAY:
		ChangeStatePlay();
		break;
	case Player::STATE::STOP:
		ChangeStateStop();
		break;
	case Player::STATE::LANDING_STIFF:
		landingStiffTimer_ = TIME_LANDING_STIFF;
		speed_ = 0.0f;
		movePow_ = AsoUtility::VECTOR_ZERO;
		// ズドンと膝をつくような着地モーション（既存のJUMP等の後半フレームを固定しても良いです）
		if (animationController_ != nullptr) {
			animationController_->Play((int)ANIM_TYPE::JUMP, false, 35.0f, 45.0f, false, false);
		}
		break;
	}

}

void Player::ChangeStateNone(void)
{
}

void Player::ChangeStatePlay(void)
{
}

void Player::ChangeStateStop(void)
{
	state_ = STATE::STOP;
	stopTimer_ = STOP_TIME;
	speed_ = 0.0f;
	movePow_ = AsoUtility::VECTOR_ZERO;

	// 急停止した瞬間のカメラの正面を向く（Yを0にして正規化）
	Camera* cam = SceneManager::GetInstance().GetCamera();
	VECTOR camForward = cam->GetForward();
	camForward.y = 0.0f;
	if (VSize(camForward) > 0.001f) {
		camForward = VNorm(camForward);
		goalQuaRot_ = Quaternion::LookRotation(camForward);
	}

	animationController_->Play((int)ANIM_TYPE::IDLE);
}

void Player::UpdateNone(void)
{
}

void Player::UpdatePlay(void)
{
	float deltaTime = scnMng_.GetDeltaTime();

	// --- ダッシュボタンのダブルタップ & 長押し解析システム ---
	InputManager& input = InputManager::GetInstance();
	bool isBoostKeyTrg = input.IsPadBtnTrgDown(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN);
	bool isBoostKeyPress = input.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN);

	// ダブルタップタイマーの更新
	if (dashButtonTapTimer_ > 0.0f) {
		dashButtonTapTimer_ -= deltaTime;
		if (dashButtonTapTimer_ <= 0.0f) {
			dashButtonTapCount_ = 0; // 猶予時間切れ
		}
	}

	if (isBoostKeyTrg) {
		dashButtonTapCount_++;
		if (dashButtonTapCount_ == 1) {
			dashButtonTapTimer_ = DOUBLE_TAP_LIMIT_TIME;
		}
	}

	// 長押しタイマーの更新
	if (isBoostKeyPress) {
		dashPressDuration_ += deltaTime;
	}
	else {
		dashPressDuration_ = 0.0f;
	}

	// 移動入力・ダッシュ状態の処理
	ProcessMove();
	// ブースト・ジャンプ・上昇の処理
	ProcessJump();

	if (state_ != STATE::PLAY) return;

	// --- アニメーション制御 ---
	if (!isGrounded_)
	{
		if (isBoostAscent_) {
			animationController_->Play((int)ANIM_TYPE::FLY); // 長押し上昇中
		}
		else if (jumpPow_.y < 0.0f) {
			animationController_->Play((int)ANIM_TYPE::FALLING); // 自由落下
		}
	}
	else
	{
		if (boostMode_ == BOOST_MODE::DASH) {
			animationController_->Play((int)ANIM_TYPE::FAST_RUN); // ダッシュ移動
		}
		else if (VSize(movePow_) > 0.1f) {
			animationController_->Play((int)ANIM_TYPE::RUN); // 通常移動
		}
		else {
			animationController_->Play((int)ANIM_TYPE::IDLE);
		}
	}

}

void Player::ProcessMove(void)
{
	InputManager& input = InputManager::GetInstance();
	float deltaTime = scnMng_.GetDeltaTime();

	// --- 1. スティック入力・スライド移動取得 ---
	float stickY = 0.0f; // ★stickXはProcessTurnで処理するため削除
	XINPUT_STATE xinput;
	if (GetJoypadXInputState(DX_INPUT_PAD1, &xinput) == ERROR_SUCCESS) {
		if (abs(xinput.ThumbLY) > 7849)  stickY = (float)xinput.ThumbLY / 32767.0f;
	}
	bool isL1 = input.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::L1);
	bool isR1 = input.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::R1);

	// ★ここに古い旋回処理がありましたが、ProcessTurnへ移行したため削除しました。

	// 移動方向ベクトルの合成
	VECTOR localCombinedMoveDir = AsoUtility::VECTOR_ZERO;
	if (abs(stickY) > 0.1f) localCombinedMoveDir.z = stickY;

	// L1とR1が同時押しされた場合はL1（左）を優先する
	if (isL1) {
		localCombinedMoveDir.x -= 1.0f;
	}
	else if (isR1) {
		localCombinedMoveDir.x += 1.0f;
	}

	bool isMovingInput = (VSize(localCombinedMoveDir) > 0.1f);
	if (isMovingInput) {
		float sinY = sinf(transform_.rot.y); float cosY = cosf(transform_.rot.y);
		moveDir_.x = localCombinedMoveDir.x * cosY + localCombinedMoveDir.z * sinY;
		moveDir_.y = 0.0f;
		moveDir_.z = -localCombinedMoveDir.x * sinY + localCombinedMoveDir.z * cosY;
		moveDir_ = AsoUtility::VNormalize(moveDir_);
	}
	else {
		moveDir_ = AsoUtility::VECTOR_ZERO;
	}

	// --- 2. ダッシュ（ブースト移動）状態の判定 ---
	bool isDashKeyPress = input.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN);

	if (isGrounded_) {
		// ... (ブレーキ処理はそのまま) ...

		// 🔥 ENが0より大きい場合のみダッシュを開始できる
		if (isMovingInput && isDashKeyPress && !isCharging_ && en_ > 0.0f) {
			boostMode_ = BOOST_MODE::DASH;
			dashResidualTimer_ = TIME_DASH_RESIDUAL;
		}
		else if (boostMode_ == BOOST_MODE::DASH) {
			dashResidualTimer_ -= deltaTime;
			if (dashResidualTimer_ <= 0.0f) {
				ChangeState(STATE::STOP);
				boostMode_ = BOOST_MODE::BRAKE;
				return;
			}
		}
	}
	else {
		// 空中時
		if (isMovingInput) {
			if (boostMode_ == BOOST_MODE::DASH || isDashKeyPress) {
				// 🔥 ENが残っている場合のみダッシュを維持・開始
				if (!isCharging_ && en_ > 0.0f) {
					boostMode_ = BOOST_MODE::DASH;
					if (isDashKeyPress) {
						airDashTime_ += deltaTime;
					}
					else {
						airDashTime_ = 0.0f;
					}
				}
				else {
					// ENが切れたら通常移動に落とす
					boostMode_ = BOOST_MODE::NORMAL;
					airDashTime_ = 0.0f;
				}
			}
			else {
				boostMode_ = BOOST_MODE::NORMAL;
				airDashTime_ = 0.0f;
			}
		}
		else {
			boostMode_ = BOOST_MODE::NORMAL;
			airDashTime_ = 0.0f;
		}
	}

	// --- 3. 加減速慣性計算 ---
	float currentAccel = isGrounded_ ? ACCEL_GROUND : ACCEL_AIR;
	if (!isGrounded_) {
		currentAccel = ACCEL_GROUND;

		// 空中切り返し慣性ロジック
		VECTOR flatVelocity = movePow_;
		flatVelocity.y = 0.0f;
		if (VSize(flatVelocity) > 5.0f && VSize(moveDir_) > 0.1f) {
			VECTOR curVelDir = AsoUtility::VNormalize(flatVelocity);
			float dot = curVelDir.x * moveDir_.x + curVelDir.z * moveDir_.z;

			if (dot < 0.5f) {
				float rate = (dot - (-1.0f)) / (0.5f - (-1.0f));
				if (rate < 0.0f) rate = 0.0f;

				float turnInertia = MIN_TURN_ACCEL + (1.0f - MIN_TURN_ACCEL) * rate;
				currentAccel *= turnInertia;
			}
		}
	}

	float currentFriction = isGrounded_ ? FRICTION_GROUND : FRICTION_AIR;

	// 最高速度（基準値）の決定
	float maxSpeed = SPEED_RUN;
	if (boostMode_ == BOOST_MODE::DASH) {
		if (isGrounded_) {
			maxSpeed = SPEED_DASH;
		}
		else {
			// 空中ダッシュの可変加速処理
			constexpr float ACCEL_TIME = 0.6f;

			float ratio = airDashTime_ / ACCEL_TIME;
			if (ratio > 1.0f) ratio = 1.0f;

			float startSpeed = SPEED_DASH * 0.8f;
			float endSpeed = SPEED_DASH;
			maxSpeed = startSpeed + (endSpeed - startSpeed) * ratio;
		}
	}
	else {
		maxSpeed = SPEED_RUN;
	}

	if (VSize(moveDir_) > 0.1f) {
		float inputLength = VSize(localCombinedMoveDir);
		if (inputLength > 1.0f) inputLength = 1.0f;

		VECTOR targetVelocity = VScale(moveDir_, maxSpeed * inputLength);
		movePow_.x += (targetVelocity.x - movePow_.x) * currentAccel * deltaTime * 10.0f;
		movePow_.z += (targetVelocity.z - movePow_.z) * currentAccel * deltaTime * 10.0f;
	}
	else {
		movePow_.x *= (1.0f - (1.0f - currentFriction) * deltaTime * 60.0f);
		movePow_.z *= (1.0f - (1.0f - currentFriction) * deltaTime * 60.0f);
	}
	debugCurrentSpeed_ = VSize(movePow_);
}



void Player::ProcessJump(void)
{
	InputManager& input = InputManager::GetInstance();
	float deltaTime = scnMng_.GetDeltaTime();

	// ボタンが押された瞬間、および長押し判定
	bool isBoostKeyTrg = input.IsPadBtnTrgDown(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN);
	bool isBoostKeyPress = input.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN);

	// ダブルタップが成立したか？
	bool isDoubleTap = (isBoostKeyTrg && dashButtonTapCount_ == 2 && dashButtonTapTimer_ > 0.0f && !isCharging_);

	if (isGrounded_)
	{
		isBoostAscent_ = false;
		gravityScale_ = 1.0f;

		if (isDoubleTap)
		{
			isJump_ = true;
			isGrounded_ = false;
			dashButtonTapCount_ = 0; // カウントリセット

			if (boostMode_ == BOOST_MODE::DASH) {
				// ① ダッシュ中に素早く2回：前方の勢いを保ったままジャンプ！
				// ★ en_ -= 100.0f;  <-- 【削除】一瞬で減る処理を無くしました
				jumpPow_.y = POW_JUMP * 0.5f;
			}
			else {
				// ② 静止時（通常時）に2回：少しジャンプ力の高い大ジャンプ！
				jumpPow_.y = POW_JUMP * 1.5f; // 大ジャンプ
			}
			boostMode_ = BOOST_MODE::NORMAL;
			return;
		}

		// ③ 静止時にダッシュボタン長押し（0.2秒以上）で上昇に移行
		if (boostMode_ != BOOST_MODE::DASH && isBoostKeyPress && dashPressDuration_ > 0.2f && !isCharging_ && en_ > 0.0f)
		{
			isJump_ = true;
			isGrounded_ = false;
			isBoostAscent_ = true;
			jumpPow_.y = 5.0f;
		}
	}
	else
	{
		// --- 空中にいるとき ---
		// ④ 空中での長押し上昇判定
		if (isBoostKeyPress && !isCharging_ && en_ > 0.0f)
		{
			isBoostAscent_ = true;
			gravityScale_ = 0.0f;

			jumpPow_.y += BOOSTER_POW * deltaTime * 60.0f;
			if (jumpPow_.y > MAX_ASCENT_SPEED) {
				jumpPow_.y = MAX_ASCENT_SPEED;
			}
		}
		else
		{
			// ボタンを離した、またはENが切れた時は通常の自由落下
			isBoostAscent_ = false;
			gravityScale_ = 2.2f;

			float gravity = 1.0f * gravityScale_ * deltaTime * 60.0f;
			jumpPow_.y -= gravity;

			if (jumpPow_.y < -50.0f) {
				jumpPow_.y = -50.0f;
			}
		}
	}
}

void Player::SetGoalRotate(double rotRad)
{
	// カメラの向きを基準に、入力された移動方向への目標クォータニオンを計算
	VECTOR cameraRot = SceneManager::GetInstance().GetCamera()->GetAngles();
	Quaternion axis = Quaternion::AngleAxis((double)cameraRot.y + rotRad, AsoUtility::AXIS_Y);

	// 目標の向きを更新するだけでOK！
	// あとは ProcessTurn() が毎フレームこの目標に向かって勝手に回ってくれます
	goalQuaRot_ = axis;
}

void Player::Rotate(void)
{
	// 目標回転（旋回によって変化したクォータニオン）を同期
	playerRotY_ = goalQuaRot_;
	transform_.quaRot = playerRotY_;
}

void Player::Collision(void)
{
	// 1. 水平移動量(movePow_) と 垂直移動量(jumpPow_) を現在の座標に直接加算する
	transform_.pos = VAdd(transform_.pos, movePow_);
	transform_.pos = VAdd(transform_.pos, jumpPow_);

	// 移動した座標をコライダに反映させるため、一度行列を更新
	transform_.Update();

	// 2. 壁や障害物との衝突判定（カプセルによる横押し戻し）
	// これにより、移動して壁にめり込んだ transform_.pos が正しくその場で押し戻されます
	CollisionCapsule();

	// 3. 床（ステージポリゴン）との接地判定
	// 上昇推進力が強くかかっている間、またはブースター上昇フラグが立っている間は、
	// 床に吸い付くのを防ぐために接地チェックをスキップして空中状態にする
	if (jumpPow_.y > 0.05f || isBoostAscent_)
	{
		isGrounded_ = false;
	}
	else
	{
		// 下向きのレイ（線分）を飛ばして床をチェック
		// 壁の押し戻しが完了した「現在の最新の transform_.pos」を基準にする
		VECTOR dirGravity = AsoUtility::DIR_D;
		VECTOR dirUpGravity = AsoUtility::DIR_U;
		float checkPow = 50.0f;

		gravHitPosUp_ = VAdd(transform_.pos, VScale(dirUpGravity, 20.0f));
		gravHitPosDown_ = VAdd(transform_.pos, VScale(dirGravity, checkPow));

		bool isHitFloor = false;
		float highestFloorY = -999999.0f;

		// 登録されているステージコライダをすべてループ
		for (const auto c : hitColliders_)
		{
			if (c->GetShape() == ColliderBase::SHAPE::MODEL)
			{
				auto modelCollider = dynamic_cast<const ColliderModel*>(c);
				if (modelCollider != nullptr)
				{
					int modelId = modelCollider->GetFollow()->modelId;
					// DxLibのポリゴン線分交差判定
					auto hit = MV1CollCheck_Line(modelId, -1, gravHitPosUp_, gravHitPosDown_);

					if (hit.HitFlag > 0)
					{
						if (hit.HitPosition.y > highestFloorY)
						{
							highestFloorY = hit.HitPosition.y;
							isHitFloor = true;
						}
					}
				}
			}
		}

		if (isHitFloor)
		{
			// 接地したため、Y座標を床の高さに合わせる
			transform_.pos.y = highestFloorY + 0.0f;

			// 着地した瞬間の落下速度（引力）を記録しておく
			float landSpeed = jumpPow_.y;
			jumpPow_.y = 0.0f;

			if (isJump_ || !isGrounded_)
			{
				// 高所着地硬直の判定
				if (landSpeed < LIMIT_LANDING_SPEED)
				{
					ChangeState(STATE::LANDING_STIFF);
					boostMode_ = BOOST_MODE::NORMAL;
				}
				else
				{
					// 通常の着地アニメーション
					if (animationController_ != nullptr) {
						animationController_->Play((int)ANIM_TYPE::JUMP, false, 29.0f, 45.0f, false, true);
					}
				}
			}
			isJump_ = false;
			isGrounded_ = true;
		}
		else
		{
			// 下に床がなければ空中（落下状態）
			isGrounded_ = false;
		}
	}

	// 全ての押し戻しと接地処理が完了した最終的な座標を3Dモデル側に即時同期
	transform_.Update();

	// 4. 入力によって毎フレーム生成される水平移動量だけをリセット
	movePow_ = AsoUtility::VECTOR_ZERO;
}

void Player::CollisionGravity(void)
{
	// 処理はすべて上の Collision() に統合したため、ここは空っぽ（または親を呼ばない形）にします
}


bool Player::IsEndLanding(void)
{

	bool ret = true;

	// アニメーションがジャンプではない
	if (animationController_->GetPlayType() != (int)ANIM_TYPE::JUMP)
	{
		return ret;
	}

	// アニメーションが終了しているか
	if (animationController_->IsEnd())
	{
		return ret;
	}

	return false;

}

void Player::ProcessTurn(void)
{
	InputManager& input = InputManager::GetInstance();
	float deltaTime = scnMng_.GetDeltaTime();

	// --- 1. 旋回入力の取得 ---
	float turnInput = 0.0f;
	XINPUT_STATE xinput;
	if (GetJoypadXInputState(DX_INPUT_PAD1, &xinput) == ERROR_SUCCESS) {
		if (abs(xinput.ThumbLX) > 7849) {
			turnInput = (float)xinput.ThumbLX / 32767.0f;
		}
	}

	if (CheckHitKey(KEY_INPUT_LEFT))  turnInput = -1.0f;
	if (CheckHitKey(KEY_INPUT_RIGHT)) turnInput = 1.0f;

	// --- 2. 旋回処理の分岐 ---
	if (abs(turnInput) > 0.1f)
	{
		// 【手動旋回】入力がある時は、目標を挟まず「現在の向き」を直接回す！
		float turnAmountDeg = turnInput * TURN_SPEED * deltaTime;
		float turnAmountRad = AsoUtility::Deg2RadF(turnAmountDeg);
		Quaternion deltaRot = Quaternion::AngleAxis(turnAmountRad, AsoUtility::AXIS_Y);

		// 現在の回転に直接乗算（これで遅延ゼロ、100%キビキビ動く）
		transform_.quaRot = transform_.quaRot.Mult(deltaRot);

		// 手動旋回中は、目標の向き（goalQuaRot_）も現在地に完全同期させておく
		// これをしないと、旋回をやめた瞬間に古い目標に向かって勝手に逆戻りしたりする
		goalQuaRot_ = transform_.quaRot;
	}
	else
	{
		// 【自動旋回】入力がない時だけ、急停止（STATE::STOP）などで設定された目標へ滑らかに補間
		Quaternion currentRot = transform_.quaRot;
		float maxDelta = TURN_SPEED * deltaTime;

		transform_.quaRot = Quaternion::RotateTowards(currentRot, goalQuaRot_, maxDelta);
	}

	// トランスフォームのオイラー角を同期
	transform_.rot = Quaternion::ToEuler(transform_.quaRot);
}

void Player::UpdateCommonMechanics(void)
{
	// --- 武器の発射処理 ---
	if (rightWeapon_ != nullptr)
	{
		rightWeapon_->Update();
		auto& ins = InputManager::GetInstance();

		bool isFirePressed = CheckHitKey(KEY_INPUT_Z) ||
			ins.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::LEFT);

		if (isFirePressed)
		{
			VECTOR localMuzzlePos = VGet(50.0f, 120.0f, 80.0f);
			VECTOR muzzlePos = VAdd(transform_.pos, transform_.quaRot.PosAxis(localMuzzlePos));
			VECTOR targetPos;

			if (fcs_ != nullptr && fcs_->GetLockState() == FCS::LOCK_STATE::LOCKED)
			{
				targetPos = fcs_->CalcPredictivePos(rightWeapon_->GetBulletSpeed(), transform_.pos);
			}
			else
			{
				VECTOR forwardDir = transform_.quaRot.PosAxis(VGet(0.0f, 0.0f, 1.0f));
				targetPos = VAdd(muzzlePos, VScale(forwardDir, 1000.0f));
			}

			rightWeapon_->Fire(muzzlePos, targetPos, activeBullets_,false);
		}
	}

	// --- 弾丸の更新・削除 ---
	for (auto it = activeBullets_.begin(); it != activeBullets_.end(); )
	{
		(*it)->Update();
		bool isHit = false;

		if (enemyMng_ != nullptr)
		{
			const auto& enemies = enemyMng_->GetEemies();
			for (auto* enemy : enemies)
			{
				if (enemy->CheckHitBullet((*it)->GetPos(), 2.0f, (*it)->GetDamage()))
				{
					isHit = true;
					break;
				}
			}
		}

		if ((*it)->IsDead() || isHit)
		{
			delete (*it);
			it = activeBullets_.erase(it);
		}
		else
		{
			++it;
		}
	}

	// --- FCSの更新 ---
	if (fcs_ != nullptr && enemyMng_ != nullptr)
	{
		fcs_->Update(transform_.pos, enemyMng_->GetEemies());
	}
}

void Player::UpdateEnergy(float deltaTime)
{
	bool isConsuming = false;

	InputManager& input = InputManager::GetInstance();
	bool isDashKeyPress = input.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN);

	// 1. ブーストダッシュ移動による消費（チャージング中でない場合のみ）
	if (boostMode_ == BOOST_MODE::DASH && isDashKeyPress && !isCharging_)
	{
		en_ -= EN_CONSUME_DASH * deltaTime;
		isConsuming = true;

		// 走行中にENが切れた場合の強制停止 ＆ チャージング開始
		if (en_ <= 0.0f)
		{
			en_ = 0.0f;
			isCharging_ = true; // 🔥 チャージング状態へ
			if (isGrounded_) {
				ChangeState(STATE::STOP); // 急ブレーキ硬直へ
				boostMode_ = BOOST_MODE::BRAKE;
			}
			else {
				boostMode_ = BOOST_MODE::NORMAL; // 空中なら通常落下へ
				airDashTime_ = 0.0f;
			}
		}
	}

	// 2. ブースト上昇による消費（チャージング中でない場合のみ）
	if (isBoostAscent_ && !isCharging_)
	{
		en_ -= EN_CONSUME_ASCENT * deltaTime;
		isConsuming = true;

		// 上昇中にENが切れた場合の強制落下 ＆ チャージング開始
		if (en_ <= 0.0f)
		{
			en_ = 0.0f;
			isCharging_ = true; // 🔥 チャージング状態へ
			isBoostAscent_ = false;
		}
	}

	// 3. ブースターを何も使っていない（またはチャージング中）場合はENが回復する
	if (!isConsuming)
	{
		en_ += EN_RECOVER * deltaTime;

		if (en_ > MAX_EN)
		{
			en_ = MAX_EN;

			// 🔥 ENが最大まで全回復したらチャージング解除！
			if (isCharging_)
			{
				isCharging_ = false;
			}
		}
	}

	isBoosterOn_ = isConsuming;
}

void Player::UpdateStop(void)
{
	float deltaTime = scnMng_.GetDeltaTime();
	stopTimer_ -= deltaTime;

	// ★Rotate()はProcessTurn内で自動実行されるようになったため削除しました。

	if (stopTimer_ <= 0.0f) {
		ChangeState(STATE::PLAY);
	}
}

void Player::UpdateLandingStiff(void)
{
	float deltaTime = scnMng_.GetDeltaTime();
	landingStiffTimer_ -= deltaTime;

	// 硬直中も XZ軸の残存エネルギーがあれば摩擦で強制停止させる
	movePow_.x *= (1.0f - 0.2f * deltaTime * 60.0f);
	movePow_.z *= (1.0f - 0.2f * deltaTime * 60.0f);

	// 時間が来たら通常のプレイ状態に復帰
	if (landingStiffTimer_ <= 0.0f) {
		ChangeState(STATE::PLAY);
	}
}

void Player::CollisionReserve(void)
{
	// アニメーションごとの線分調整
	if (animationController_->GetPlayType() == static_cast<int>(ANIM_TYPE::JUMP))
	{
		if (ownColliders_.count(static_cast<int>(COLLIDER_TYPE::LINE)) != 0)
		{
			ColliderLine* colLine = dynamic_cast<ColliderLine*>(
				ownColliders_.at(static_cast<int>(COLLIDER_TYPE::LINE)));
			colLine->SetLocalPosStart(COL_LINE_JUMP_START_LOCAL_POS);
			colLine->SetLocalPosEnd(COL_LINE_JUMP_END_LOCAL_POS);
		}

		// ジャンプ中はカプセルを上に上げる
		if (ownColliders_.count(static_cast<int>(COLLIDER_TYPE::CAPSULE)) != 0)
		{
			ColliderCapsule* colCapsule = dynamic_cast<ColliderCapsule*>(
				ownColliders_.at(static_cast<int>(COLLIDER_TYPE::CAPSULE)));
			colCapsule->SetLocalPosTop(COL_CAPSULE_TOP_JUMP_LOCAL_POS);
			colCapsule->SetLocalPosDown(COL_CAPSULE_DOWN_JUMP_LOCAL_POS);
			colCapsule->SetRadius(COL_CAPSULE_RADIUS);
		}
	}
	else
	{
		// 通常時の線分に戻す
		if (ownColliders_.count(static_cast<int>(COLLIDER_TYPE::LINE)) != 0)
		{
			ColliderLine* colLine = dynamic_cast<ColliderLine*>(
				ownColliders_.at(static_cast<int>(COLLIDER_TYPE::LINE)));
			colLine->SetLocalPosStart(COL_LINE_START_LOCAL_POS);
			colLine->SetLocalPosEnd(COL_LINE_END_LOCAL_POS);
		}

		// 通常時のカプセルに戻す
		if (ownColliders_.count(static_cast<int>(COLLIDER_TYPE::CAPSULE)) != 0)
		{
			ColliderCapsule* colCapsule = dynamic_cast<ColliderCapsule*>(
				ownColliders_.at(static_cast<int>(COLLIDER_TYPE::CAPSULE)));
			colCapsule->SetLocalPosTop(COL_CAPSULE_TOP_LOCAL_POS);
			colCapsule->SetLocalPosDown(COL_CAPSULE_DOWN_LOCAL_POS);
			colCapsule->SetRadius(COL_CAPSULE_RADIUS);
		}
	}
}

void Player::Draw2D(void)
{
	// プレイ中、または硬直中であり、FCSが正常な時のみ描画
	if ((state_ == STATE::PLAY || state_ == STATE::STOP || state_ == STATE::LANDING_STIFF) && fcs_ != nullptr)
	{
		// 1. FCSの描画（サイト枠やロックマーカー）
		fcs_->Draw();

		// 2. 画面解像度（サイズ）の自動取得
		int screenWidth, screenHeight;
		GetDrawScreenSize(&screenWidth, &screenHeight);

		// =========================================================
		// 【左下】ENゲージの描画 (前回実装分)
		// =========================================================
		int enX = 80;
		int enY = screenHeight - 120;
		int enWidth = 200;
		int enHeight = 12;
		float enRatio = en_ / MAX_EN;
		DrawBox(enX, enY, enX + enWidth, enY + enHeight, GetColor(40, 40, 40), TRUE);

		// 通常時はシアン/緑系、残り2割で赤、チャージング中は警告用の赤に固定
		unsigned int enColor = (enRatio < 0.2f) ? GetColor(255, 64, 64) : GetColor(0, 255, 128);
		if (isCharging_)
		{
			enColor = GetColor(255, 0, 0); // チャージング中は真っ赤なゲージに
		}

		DrawBox(enX, enY, enX + static_cast<int>(enWidth * enRatio), enY + enHeight, enColor, TRUE);
		DrawBox(enX, enY, enX + enWidth, enY + enHeight, GetColor(200, 200, 200), FALSE);

		// 🔥 チャージング中は警告テキストを表示
		if (isCharging_)
		{
			// AC風の警告表示（GetNowCountを使用して点滅させるとさらに雰囲気が出ます）
			if ((GetNowCount() / 200) % 2 == 0)
			{
				DrawString(enX, enY - 18, "CHARGING...", GetColor(255, 0, 0));
			}
		}
		else
		{
			DrawString(enX, enY - 18, "EN GAUGE", GetColor(255, 255, 255));
		}


		// =========================================================
		// 【右下】武器・残弾数UIの描画 (新規追加)
		// =========================================================
		if (rightWeapon_ != nullptr)
		{
			int wpX = screenWidth - 280; // 右端からのオフセット
			int wpY = screenHeight - 120;
			int wpWidth = 200;
			int wpHeight = 10;

			int currentAmmo = rightWeapon_->GetCurrentAmmo();
			int maxAmmo = rightWeapon_->GetMaxAmmo();
			float ammoRatio = (maxAmmo > 0) ? static_cast<float>(currentAmmo) / maxAmmo : 0.0f;

			// ① 武器名の描画 (少し大きめのフォントや、目立つ色で)
			DrawString(wpX, wpY - 35, rightWeapon_->GetName().c_str(), GetColor(255, 255, 255));

			// ② 残弾数のデジタル数値表示
			DrawFormatString(wpX, wpY - 18, GetColor(0, 255, 255), "AMMO: %d / %d", currentAmmo, maxAmmo);

			// ③ 残弾ゲージ（背景枠）
			DrawBox(wpX, wpY, wpX + wpWidth, wpY + wpHeight, GetColor(40, 40, 40), TRUE);

			// ④ 残弾ゲージ（本体：弾数が減ると青→黄→赤に変化するAC風演出）
			unsigned int ammoColor = GetColor(0, 200, 255); // 通常は綺麗なシアン
			if (ammoRatio < 0.1f)      ammoColor = GetColor(255, 64, 64);   // 残り1割で赤
			else if (ammoRatio < 0.3f) ammoColor = GetColor(255, 255, 64);  // 残り3割で黄

			int currentBarWidth = static_cast<int>(wpWidth * ammoRatio);
			DrawBox(wpX, wpY, wpX + currentBarWidth, wpY + wpHeight, ammoColor, TRUE);

			// ⑤ ゲージの外枠
			DrawBox(wpX, wpY, wpX + wpWidth, wpY + wpHeight, GetColor(200, 200, 200), FALSE);
		}
	}
}

bool Player::CheckHitBullet(const VECTOR& bulletPos, float bulletRadius, int damage)
{
	if (hp_ <= 0) return false;

	// CharactorBase等で定義されているコライダの取得（EnemyBaseと同じ仕組みと仮定）
	int capsuleKey = static_cast<int>(CharactorBase::COLLIDER_TYPE::CAPSULE);
	const auto& ownColliders = GetOwnColliders();

	if (ownColliders.count(capsuleKey) > 0)
	{
		auto* baseCollider = ownColliders.at(capsuleKey);
		if (baseCollider != nullptr && baseCollider->GetShape() == ColliderBase::SHAPE::CAPSULE)
		{
			// 安全に ColliderCapsule にキャスト
			auto* capsule = static_cast<ColliderCapsule*>(baseCollider);

			if (HitCheck_Sphere_Capsule(
				bulletPos,
				bulletRadius,
				capsule->GetPosTop(),
				capsule->GetPosDown(),
				capsule->GetRadius()) == TRUE)
			{
				// ★修正：自前のApplyDamageを呼ぶことで、クランプや死亡処理を共通化
				ApplyDamage(damage);

				return true; // 当たった
			}
		}
	}
	return false;
}