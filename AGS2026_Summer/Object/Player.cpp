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

}

Player::~Player(void)
{
	delete animationController_;
	delete fcs_;
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
	isJump_ = false;
	isBoostAscent_ = false;

	playerRotY_ = transform_.quaRot;
	goalQuaRot_ = transform_.quaRot;

	// ★★★ FCSの生成と初期化 ★★★
	fcs_ = new FCS();
	fcs_->Init();
	fcs_->SetPlayer(this); // FCS側に自分（Player）への参照を渡す

	SetUseLighting(FALSE);
	// 初期状態
	ChangeState(STATE::PLAY);
}

void Player::UpdateProcess(void)
{

	// パーツの性能に応じてカメラの旋回速度を更新
	// 例: headPart->GetTurnSpeed() など
	float currentTurnSpeed = 0.0001f; // 本来はパーツのステータスから取得

	// 装備重量が重いと旋回が鈍くなる、などの補正もここで可能
	/*if (isHeavyWeight) {
		currentTurnSpeed *= 0.8f;
	}*/

	auto* camera = SceneManager::GetInstance().GetCamera();
	camera->SetRotationSpeed(currentTurnSpeed);


	// 更新ステップ
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
	}

}

void Player::UpdateProcessPost(void)
{
}

void Player::Draw(void)
{

	CharactorBase::Draw();

#ifdef _DEBUG
	DrawFormatString(0, 220, GetColor(255, 255, 255),
		"Pos: X=%.1f Y=%.1f Z=%.1f",
		transform_.pos.x, transform_.pos.y, transform_.pos.z);
#endif // _DEBUG

	// モデルの描画
	MV1DrawModel(transform_.modelId);

	// 丸影描画
	DrawShadow();

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

	// 移動処理
	ProcessMove();
	if (state_ != STATE::PLAY) return;

	if (fcs_ != nullptr)
	{
		fcs_->Update(transform_.pos, enemyMng_->GetEemies());
	}

}

void Player::ProcessMove(void)
{
	auto& ins = InputManager::GetInstance();
	float deltaTime = scnMng_.GetDeltaTime();
	auto padState = ins.GetJPadInputState(InputManager::JOYPAD_NO::PAD1);

	// --- 1. 入力状態の取得 ---
	isDashKeyPress_ = CheckHitKey(KEY_INPUT_LSHIFT) ||
		ins.IsPadBtnPush(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN);

	bool isDashKeyTrg = (isDashKeyPress_ && !oldDashKey_);
	bool isDashKeyRel = (!isDashKeyPress_ && oldDashKey_);
	oldDashKey_ = isDashKeyPress_;

	if (rePressWindowTimer_ > 0.0f && !isDashKeyPress_) {
		rePressWindowTimer_ -= deltaTime;
	}
	if (isDashKeyRel) {
		rePressWindowTimer_ = 0.2f;
	}

	Camera* cam = SceneManager::GetInstance().GetCamera();

	// --- 2. 左スティックの左右入力によるカメラ角度の変更 ---
	float stickX = padState.AKeyLX / 1000.0f; // -1.0f ～ 1.0f
	if (abs(stickX) > 0.2f) {
		float rotAmount = stickX * 0.04f;
		cam->AddAngleY(rotAmount);
	}

	// 現在のカメラの正面ベクトルをベースに水平回転クォータニオンを計算
	VECTOR camForward = cam->GetForward();
	camForward.y = 0.0f;
	if (VSize(camForward) < 0.001f) {
		camForward = VGet(0.0f, 0.0f, 1.0f);
	}
	camForward = VNorm(camForward);

	Quaternion camRotY = Quaternion::LookRotation(camForward);
	VECTOR camRight = camRotY.GetRight();

	// --- 3. 移動方向ベクトルの計算 ---
	VECTOR combinedDir = AsoUtility::VECTOR_ZERO;
	float stickY = padState.AKeyLY / 1000.0f;

	if (abs(stickY) > 0.2f) {
		combinedDir = VAdd(combinedDir, VScale(camForward, -stickY));
	}

	auto hDirType = ins.GetHorizontalDir();
	if (hDirType == InputManager::MoveDir::Left) combinedDir = VSub(combinedDir, camRight);
	if (hDirType == InputManager::MoveDir::Right) combinedDir = VAdd(combinedDir, camRight);

	bool hasMoveInput = (VSize(combinedDir) > 0.1f);

	// ★機体の目標回転は、何があろうと「常にカメラの正面」
	goalQuaRot_ = camRotY;

	// --- 4. ジャンプ・ダッシュ・移動物理の計算 ---
		// ★【追加】前フレームの「実際の移動方向」をフレームをまたいで記憶する静的変数
	static VECTOR lastActualDir = camForward;

	if (dashTapTimer_ > 0.0f) {
		dashTapTimer_ -= deltaTime;
		if (dashTapTimer_ <= 0.0f) dashTapCount_ = 0;
	}
	if (isDashKeyTrg) {
		dashTapCount_++;
		if (dashTapCount_ == 2 && dashTapTimer_ > 0.0f) {
			if (!isJump_) {
				isJump_ = true;
				jumpPow_.y = POW_JUMP;
				animationController_->Play((int)ANIM_TYPE::JUMP, true, 13.0f, 25.0f);
				dashTapCount_ = 0;
				dashTapTimer_ = 0.0f;
			}
		}
		else { dashTapTimer_ = DOUBLE_TAP_TIME; }
	}

	if (isDashKeyPress_) {
		float staticThreshold = SPEED_MOVE * 0.2f;
		bool isStatic = (!hasMoveInput && speed_ < staticThreshold);
		bool isRePressed = (rePressWindowTimer_ > 0.0f);

		if (isJump_ || isRePressed || !hasMoveInput) {
			dashPressDuration_ += deltaTime;
		}
		else {
			dashPressDuration_ = 0.0f;
		}

		if (!isBoostAscent_) {
			if (dashPressDuration_ > LONG_PRESS_THRESHOLD) {
				isBoostAscent_ = true;
				isJump_ = true;
				jumpPow_.y = 2.0f;
			}
		}

		if (isBoostAscent_) {
			isJump_ = true;
			jumpPow_.y += BOOSTER_POW;
			if (jumpPow_.y > MAX_ASCENT_SPEED) jumpPow_.y = MAX_ASCENT_SPEED;
			dashResidualTimer_ = DASH_RESIDUAL_TIME;
		}
		else if (hasMoveInput && !isJump_) {
			dashResidualTimer_ = DASH_RESIDUAL_TIME;
		}
		else {
			if (!isJump_) dashResidualTimer_ -= deltaTime;
		}
	}
	else {
		if (rePressWindowTimer_ <= 0.0f) {
			dashPressDuration_ = 0.0f;
		}
		isBoostAscent_ = false;
		if (!isJump_) dashResidualTimer_ -= deltaTime;
	}

	if (dashResidualTimer_ < 0.0f) dashResidualTimer_ = 0.0f;

	bool isDashing = (dashResidualTimer_ > 0.0f);
	if (!isJump_ && isDashingBefore_ && !isDashing && !isDashKeyPress_) {
		isDashingBefore_ = false;
		ChangeState(STATE::STOP);
		return;
	}
	isDashingBefore_ = isDashing;

	if (hasMoveInput) {
		VECTOR inputDir = VNorm(combinedDir);

		// ★【バグ修正】毎フレーム消えるmovePow_ではなく、記憶しておいた「実際の移動方向」と入力方向を比較します
		float dot = VDot(lastActualDir, inputDir);
		float targetSpeed = isDashing ? SPEED_RUN : SPEED_MOVE;

		if (speed_ > 1.0f && dot < 0.5f) {
			float baseBrake = 0.1f + (fabsf(fminf(dot, 0.0f)) * 0.2f);
			if (isJump_) {
				baseBrake *= 0.6f;
			}
			speed_ = AsoUtility::Lerp(speed_, 0.0f, baseBrake);
		}
		else {
			float accel = 0.2f;
			if (speed_ < targetSpeed * 0.5f) {
				accel = 0.05f;
			}
			if (isJump_) {
				accel *= 0.4f;
			}
			speed_ = AsoUtility::Lerp(speed_, targetSpeed, accel);
		}

		// システムハック用のダミー正面ベクトル
		moveDir_ = camForward;

		// 実際の物理移動パワーには、入力された方向を反映
		movePow_ = VScale(inputDir, speed_);

		// ★【追加】現在の正しい移動方向を、次のフレームのために記憶する
		lastActualDir = inputDir;

		if (!isJump_ && IsEndLanding()) {
			animationController_->Play(isDashing ? (int)ANIM_TYPE::FAST_RUN : (int)ANIM_TYPE::RUN);
		}
	}
	else {
		float targetSpeed = 0.0f;
		float decelerationRatio = (isDashing || isJump_) ? 0.05f : 0.2f;

		speed_ = AsoUtility::Lerp(speed_, targetSpeed, decelerationRatio);
		if (speed_ < 0.1f) {
			speed_ = 0.0f;
		}

		// キーを離した減速中（慣性移動）も、最後に進んでいた方向をキープして滑らせる
		if (speed_ > 0.0f) {
			movePow_ = VScale(lastActualDir, speed_);
		}
		else {
			movePow_ = AsoUtility::VECTOR_ZERO;
			lastActualDir = camForward; // ★完全に静止したら記憶を正面にリセット
		}

		moveDir_ = camForward;

		if (!isJump_ && IsEndLanding()) {
			animationController_->Play(isDashing ? (int)ANIM_TYPE::FAST_RUN : (int)ANIM_TYPE::IDLE);
		}
	}

	if (isJump_) {
		if (isDashKeyPress_ && dashPressDuration_ > LONG_PRESS_THRESHOLD) animationController_->Play((int)ANIM_TYPE::FLY);
		else if (jumpPow_.y < -1.0f) animationController_->Play((int)ANIM_TYPE::FALLING);
	}

	// --- 5. 物理計算と衝突判定の一元管理 ---
	CollisionReserve();
	CalcGravityPow();
	Collision();

	if (fcs_ != nullptr && enemyMng_ != nullptr)
	{
		// 自身の3D座標(transform_.pos) と EnemyManagerから取得した敵リストをFCSに渡す
		fcs_->Update(transform_.pos, enemyMng_->GetEemies());
	}

	// 6. 最終決定したカメラ向きをプレイヤーの姿勢に同期
	Rotate();

	// 移動量をリセット
	movePow_ = AsoUtility::VECTOR_ZERO;
}

void Player::ProcessJump(void)
{
	// ジャンプ中でなければ何もしない
	if (!isJump_) return;
}

void Player::SetGoalRotate(double rotRad)
{

	VECTOR cameraRot = SceneManager::GetInstance().GetCamera()->GetAngles();
	Quaternion axis = Quaternion::AngleAxis((double)cameraRot.y + rotRad, AsoUtility::AXIS_Y);

	// 現在設定されている回転との角度差を取る
	double angleDiff = Quaternion::Angle(axis, goalQuaRot_);

	// しきい値
	if (angleDiff > 0.1)
	{
		stepRotTime_ = TIME_ROT;
	}

	goalQuaRot_ = axis;

}

void Player::Rotate(void)
{
	// 目標回転（カメラの水平回転）をダイレクトに代入
	playerRotY_ = goalQuaRot_;

	// 最終決定された回転をトランスフォームへ適用
	transform_.quaRot = playerRotY_;
}

void Player::Collision(void)
{
	// 2重加算を防ぐため、現在の確定座標（transform_.pos）に純粋な移動量を足してスタートする
	movedPos_ = VAdd(transform_.pos, movePow_);

	// 衝突判定 (壁や障害物などのカプセル押し戻し)
	CollisionCapsule();

	// 衝突判定 (重力・落下・床の設置処理)
	CollisionGravity();

	// 最終的に安全が保証された座標をプレイヤーに反映
	transform_.pos = movedPos_;

	//movePow_ = AsoUtility::VECTOR_ZERO;
}

void Player::CollisionGravity(void)
{
	movedPos_ = VAdd(movedPos_, jumpPow_);

	// 上昇中（jumpPow_.y > 0）かつ上昇ボタン入力中なら接地判定を完全にスキップ
	if (jumpPow_.y > 0.0f || isBoostAscent_) return;

	VECTOR dirGravity = AsoUtility::DIR_D;
	VECTOR dirUpGravity = AsoUtility::DIR_U;
	float checkPow = 10.0f;

	gravHitPosUp_ = VAdd(movedPos_, VScale(dirUpGravity, 20.0f));
	gravHitPosDown_ = VAdd(movedPos_, VScale(dirGravity, checkPow));

	bool isHitFloor = false;
	float highestFloorY = -999999.0f; // ★最も高い床のY座標を記録する変数

	for (const auto c : hitColliders_)
	{
		// 1. まず形状が「MODEL」であるか安全にチェック
		if (c->GetShape() == ColliderBase::SHAPE::MODEL)
		{
			// 2. ColliderModelポインタへのキャスト
			auto modelCollider = dynamic_cast<const ColliderModel*>(c);
			if (modelCollider != nullptr)
			{
				// 3. 正しいモデルIDの取得
				int modelId = modelCollider->GetFollow()->modelId;

				// 4. DxLibの衝突判定を実行
				auto hit = MV1CollCheck_Line(modelId, -1, gravHitPosUp_, gravHitPosDown_);

				if (hit.HitFlag > 0)
				{
					// ★複数ヒットした場合は、一番高い（上にある）床のY座標をキープする
					if (hit.HitPosition.y > highestFloorY)
					{
						highestFloorY = hit.HitPosition.y;
						isHitFloor = true;
					}
				}
			}
		}
	}

	// ★全てのコライダを調べ終わった後、一番高い床を基準に「1回だけ」位置を確定させる
	if (isHitFloor)
	{
		// XZ座標はそのままキープし、Y座標（高さ）だけを床の高さに補正する
		// ※ 2.0f だと浮きすぎてカプセルと喧嘩することがあるため、少し低め（0.1fなど）に調整できるようにします
		movedPos_.y = highestFloorY + 0.0f;
		jumpPow_ = AsoUtility::VECTOR_ZERO;

		if (isJump_)
		{
			// 着地アニメーション再生
			animationController_->Play((int)ANIM_TYPE::JUMP, false, 29.0f, 45.0f, false, true);
		}
		isJump_ = false;
	}
}

void Player::CalcGravityPow(void)
{
	// 重力加速度を計算
	float gravityVal = Application::GRAVITY;

	// ★ジャンプ中（空中）の時は重力を弱める
	if (isJump_) {
		gravityVal *= 0.1f; // 重力を半分にして「ふわっと」させる
	}
	else {
		gravityVal *= 0.5f;// 落下は少し早めに（お好みで）
	}

	VECTOR gravity = VScale(AsoUtility::DIR_D, gravityVal);
	jumpPow_ = VAdd(jumpPow_, gravity);

	// 終端速度（落下しすぎ防止）
	if (jumpPow_.y < -30.0f) jumpPow_.y = -30.0f;
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

void Player::UpdateStop(void)
{
	float deltaTime = scnMng_.GetDeltaTime();
	stopTimer_ -= deltaTime;

	// 急停止中も目標の向き（カメラ正面）へ回転させる
	Rotate();

	if (stopTimer_ <= 0.0f) {
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
	// プレイ中、かつFCSが正常に生成されている時だけ2Dサイトを描画
	if (state_ == STATE::PLAY && fcs_ != nullptr)
	{
		fcs_->Draw();
	}
}