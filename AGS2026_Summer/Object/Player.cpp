#include <string>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/InputManager.h"
#include "../Manager/SceneManager.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/Camera.h"
#include "Common/AnimationController.h"
#include "Common/Capsule.h"
#include "Common/Collider.h"
#include "Planet.h"
#include "Player.h"

Player::Player(void)
{

	animationController_ = nullptr;
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

	capsule_ = nullptr;

	currentTurnSpeed_ = DEFAULT_TURN_SPEED;

}

Player::~Player(void)
{
	delete capsule_;
	delete animationController_;
}

void Player::Init(void)
{

	SetUseLighting(FALSE);

	// モデルの基本設定
	transform_.SetModel(resMng_.LoadModelDuplicate(
		ResourceManager::SRC::PLAYER));
	transform_.scl = { 8.0f,8.0f,8.0f };
	transform_.pos = { 0.0f, -30.0f, 0.0f };
	transform_.quaRot = Quaternion();
	transform_.quaRotLocal =
		Quaternion::Euler({ 0.0f, AsoUtility::Deg2RadF(180.0f), 0.0f });
	transform_.SetEmissive(GetColorF(0.0f, 0.5f, 1.0f, 1.0f), 1);
	transform_.Update();


	// アニメーションの設定
	InitAnimation();

	// カプセルコライダ
	capsule_ = new Capsule(transform_);
	capsule_->SetLocalPosTop({ 0.0f, 110.0f, 0.0f });
	capsule_->SetLocalPosDown({ 0.0f, 30.0f, 0.0f });
	capsule_->SetRadius(20.0f);

	// 丸影画像
	imgShadow_ = resMng_.Load(ResourceManager::SRC::PLAYER_SHADOW).handleId_;

	// 初期状態
	ChangeState(STATE::PLAY);

}

void Player::Update(void)
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

	// モデル制御更新
	transform_.Update();

	// アニメーション再生
	animationController_->Update();

}

void Player::Draw(void)
{

	// モデルの描画
	MV1DrawModel(transform_.modelId);

	// 丸影描画
	DrawShadow();

	DrawFormatString(0, 80, GetColor(255, 255, 255), "Timer: %f", dashResidualTimer_);
	DrawFormatString(0, 100, GetColor(255, 255, 255), "isJump: %d", isJump_ ? 1 : 0);
	DrawFormatString(0, 120, GetColor(255, 255, 255), "Speed: %f", speed_);

}

void Player::AddCollider(Collider* collider)
{
	colliders_.push_back(collider);
}

void Player::ClearCollider(void)
{
	colliders_.clear();
}

const Capsule* Player::GetCapsule(void) const
{
	return capsule_;
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

	// --- 追加：急停止した瞬間のカメラの正面を向く ---
	Camera* cam = SceneManager::GetInstance().GetCamera();
	VECTOR camForward = cam->GetForward();
	camForward.y = 0.0f; // 水平方向のみ
	if (VSize(camForward) > 0.001f) {
		camForward = VNorm(camForward);
		// 急停止の目標角度をカメラの正面に設定
		goalQuaRot_ = Quaternion::LookRotation(camForward);
	}

	// 急停止アニメーション再生
	animationController_->Play((int)ANIM_TYPE::IDLE); // または専用の停止モーション
}

void Player::UpdateNone(void)
{
}

void Player::UpdatePlay(void)
{

	// 移動処理
	ProcessMove();
	if (state_ != STATE::PLAY) return;

	// ジャンプ処理
	ProcessJump();

	// 移動方向に応じた回転
	Rotate();

	// 重力による移動量
	CalcGravityPow();

	// 衝突判定
	Collision();

	// 回転させる
	transform_.quaRot = playerRotY_;

}

void Player::DrawShadow(void)
{

	float PLAYER_SHADOW_HEIGHT = 300.0f;
	float PLAYER_SHADOW_SIZE = 30.0f;

	int i;
	MV1_COLL_RESULT_POLY_DIM HitResDim;
	MV1_COLL_RESULT_POLY* HitRes;
	VERTEX3D Vertex[3] = { VERTEX3D(), VERTEX3D(), VERTEX3D() };
	VECTOR SlideVec;
	int ModelHandle;

	// ライティングを無効にする
	SetUseLighting(FALSE);

	// Ｚバッファを有効にする
	SetUseZBuffer3D(TRUE);

	// テクスチャアドレスモードを CLAMP にする( テクスチャの端より先は端のドットが延々続く )
	SetTextureAddressMode(DX_TEXADDRESS_CLAMP);

	// 影を落とすモデルの数だけ繰り返し
	for (const auto c : colliders_)
	{

		// チェックするモデルは、jが0の時はステージモデル、1以上の場合はコリジョンモデル
		ModelHandle = c->modelId_;

		// プレイヤーの直下に存在する地面のポリゴンを取得
		HitResDim = MV1CollCheck_Capsule(
			ModelHandle, -1,
			transform_.pos, VAdd(transform_.pos, { 0.0f, -PLAYER_SHADOW_HEIGHT, 0.0f }), PLAYER_SHADOW_SIZE);

		// 頂点データで変化が無い部分をセット
		Vertex[0].dif = GetColorU8(255, 255, 255, 255);
		Vertex[0].spc = GetColorU8(0, 0, 0, 0);
		Vertex[0].su = 0.0f;
		Vertex[0].sv = 0.0f;
		Vertex[1] = Vertex[0];
		Vertex[2] = Vertex[0];

		// 球の直下に存在するポリゴンの数だけ繰り返し
		HitRes = HitResDim.Dim;
		for (i = 0; i < HitResDim.HitNum; i++, HitRes++)
		{
			// ポリゴンの座標は地面ポリゴンの座標
			Vertex[0].pos = HitRes->Position[0];
			Vertex[1].pos = HitRes->Position[1];
			Vertex[2].pos = HitRes->Position[2];

			// ちょっと持ち上げて重ならないようにする
			SlideVec = VScale(HitRes->Normal, 0.5f);
			Vertex[0].pos = VAdd(Vertex[0].pos, SlideVec);
			Vertex[1].pos = VAdd(Vertex[1].pos, SlideVec);
			Vertex[2].pos = VAdd(Vertex[2].pos, SlideVec);

			// ポリゴンの不透明度を設定する
			Vertex[0].dif.a = 0;
			Vertex[1].dif.a = 0;
			Vertex[2].dif.a = 0;
			if (HitRes->Position[0].y > transform_.pos.y - PLAYER_SHADOW_HEIGHT)
				Vertex[0].dif.a = static_cast<int>(roundf(128.0f * (1.0f - fabs(HitRes->Position[0].y - transform_.pos.y) / PLAYER_SHADOW_HEIGHT)));

			if (HitRes->Position[1].y > transform_.pos.y - PLAYER_SHADOW_HEIGHT)
				Vertex[1].dif.a = static_cast<int>(roundf(128.0f * (1.0f - fabs(HitRes->Position[1].y - transform_.pos.y) / PLAYER_SHADOW_HEIGHT)));

			if (HitRes->Position[2].y > transform_.pos.y - PLAYER_SHADOW_HEIGHT)
				Vertex[2].dif.a = static_cast<int>(roundf(128.0f * (1.0f - fabs(HitRes->Position[2].y - transform_.pos.y) / PLAYER_SHADOW_HEIGHT)));

			// ＵＶ値は地面ポリゴンとプレイヤーの相対座標から割り出す
			Vertex[0].u = (HitRes->Position[0].x - transform_.pos.x) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[0].v = (HitRes->Position[0].z - transform_.pos.z) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[1].u = (HitRes->Position[1].x - transform_.pos.x) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[1].v = (HitRes->Position[1].z - transform_.pos.z) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[2].u = (HitRes->Position[2].x - transform_.pos.x) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[2].v = (HitRes->Position[2].z - transform_.pos.z) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;

			// 影ポリゴンを描画
			DrawPolygon3D(Vertex, 1, imgShadow_, TRUE);
		}

		// 検出した地面ポリゴン情報の後始末
		MV1CollResultPolyDimTerminate(HitResDim);
	}

	// ライティングを有効にする
	SetUseLighting(TRUE);

	// Ｚバッファを無効にする
	SetUseZBuffer3D(FALSE);

}

void Player::ProcessMove(void)
{
	auto& ins = InputManager::GetInstance();
	float deltaTime = scnMng_.GetDeltaTime();
	auto padState = ins.GetJPadInputState(InputManager::JOYPAD_NO::PAD1);

	// --- 1. 入力状態の取得 ---
	bool isDashKeyPress = CheckHitKey(KEY_INPUT_LSHIFT) ||
		ins.IsPadBtnNew(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN);
	bool isDashKeyTrg = (isDashKeyPress && !oldDashKey_);
	bool isDashKeyRel = (!isDashKeyPress && oldDashKey_);
	oldDashKey_ = isDashKeyPress;

	if (rePressWindowTimer_ > 0.0f && !isDashKeyPress) {
		rePressWindowTimer_ -= deltaTime;
	}
	if (isDashKeyRel) {
		rePressWindowTimer_ = 0.2f; // 離した瞬間に0.2秒の猶予を開始
	}

	Camera* cam = SceneManager::GetInstance().GetCamera();

	// --- 1. カメラ・旋回処理 (既存のまま) ---
	float stickX = padState.AKeyLX / 1000.0f;
	if (abs(stickX) > 0.2f) cam->AddAngleY(stickX * currentTurnSpeed_);

	VECTOR camForward = VNorm(VGet(cam->GetForward().x, 0.0f, cam->GetForward().z));
	VECTOR camRight = VNorm(VGet(cam->GetQuaRot().GetRight().x, 0.0f, cam->GetQuaRot().GetRight().z));

	VECTOR combinedDir = AsoUtility::VECTOR_ZERO;
	float stickY = padState.AKeyLY / 1000.0f;
	if (abs(stickY) > 0.2f) combinedDir = VAdd(combinedDir, VScale(camForward, -stickY));

	auto hDirType = ins.GetHorizontalDir();
	if (hDirType == InputManager::MoveDir::Left) combinedDir = VSub(combinedDir, camRight);
	if (hDirType == InputManager::MoveDir::Right) combinedDir = VAdd(combinedDir, camRight);

	bool hasMoveInput = (VSize(combinedDir) > 0.1f);
	goalQuaRot_ = Quaternion::LookRotation(camForward);

	// --- 2. ジャンプ判定 (既存のまま) ---
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

	// --- 3. ダッシュ・上昇ロジック ---
	if (isDashKeyPress) {
		// A. 各種判定フラグ
		float staticThreshold = SPEED_MOVE * 0.2f;
		bool isStatic = (!hasMoveInput && speed_ < staticThreshold);

		// ボタンを離して 0.2秒以内の「入れ直し受付期間」かどうか
		bool isRePressed = (rePressWindowTimer_ > 0.0f);

		// B. 長押し時間の蓄積ルール
		// 「空中」または「完全に止まっている」または「入れ直し操作中（一瞬離した直後）」ならカウント
		if (isJump_ || isStatic || isRePressed) {
			dashPressDuration_ += deltaTime;
		}
		else {
			// ★通常の地上ダッシュ走行中は 0 にリセット（勝手に上昇するのを防ぐ）
			dashPressDuration_ = 0.0f;
		}

		// C. 上昇モードへの移行判定
		if (!isBoostAscent_) {
			if (dashPressDuration_ > LONG_PRESS_THRESHOLD) {
				isBoostAscent_ = true;
				rePressWindowTimer_ = 0.0f; // 成功したので猶予終了
			}
		}

		// D. モード実行
		if (isBoostAscent_) {
			// 【上昇中】
			if (!isJump_) {
				isJump_ = true;
				jumpPow_.y = 1.0f;
				animationController_->Play((int)ANIM_TYPE::JUMP, true, 13.0f, 25.0f);
			}
			jumpPow_.y += BOOSTER_POW;
			if (jumpPow_.y > MAX_ASCENT_SPEED) jumpPow_.y = MAX_ASCENT_SPEED;

			dashResidualTimer_ = DASH_RESIDUAL_TIME;
		}
		else if (hasMoveInput && !isJump_) {
			// 【地上ダッシュ中】
			dashResidualTimer_ = DASH_RESIDUAL_TIME;
		}
		else {
			// 【移動停止後の余韻】
			if (!isJump_) {
				dashResidualTimer_ -= deltaTime;
			}
		}
	}
	else {
		// ★★★ ここが最重要ポイント ★★★
		// ボタンを離した瞬間、すぐリセットせずに rePressWindowTimer_ が切れるまで時間を保持する
		if (rePressWindowTimer_ <= 0.0f) {
			dashPressDuration_ = 0.0f;
		}

		isBoostAscent_ = false;

		// 地上にいるなら、ボタンを離した瞬間から余韻タイマー（急停止用）減少開始
		if (!isJump_) {
			dashResidualTimer_ -= deltaTime;
		}
	}

	// 0以下にならないようクランプ
	if (dashResidualTimer_ < 0.0f) dashResidualTimer_ = 0.0f;

	// --- 4. 急停止判定 ---
	bool isDashing = (dashResidualTimer_ > 0.0f);
	if (!isJump_ && isDashingBefore_ && !isDashing && !isDashKeyPress) {
		isDashingBefore_ = false;
		ChangeState(STATE::STOP);
		return;
	}
	isDashingBefore_ = isDashing;

	// --- 5. 移動計算 (以前の挙動を完全維持) ---
	if (hasMoveInput) {
		VECTOR inputDir = VNorm(combinedDir);
		moveDir_ = VNorm(combinedDir);
		float targetSpeed = isDashing ? SPEED_RUN : SPEED_MOVE;
		float accelerationRatio = 0.05f;
		float dot = VDot(moveDir_, inputDir);

		if (isDashing) {
			if (dot < 0.5f) { accelerationRatio = 0.015f; targetSpeed *= 0.7f; }
			else if (speed_ < (SPEED_RUN * 0.5f)) { accelerationRatio = 0.02f; }
		}

		moveDir_ = VNorm(VAdd(VScale(moveDir_, 0.9f), VScale(inputDir, 0.1f)));
		if (isJump_) accelerationRatio = 0.05f;
		if (speed_ < (SPEED_MOVE * 0.3f)) accelerationRatio = 0.01f;

		float turnResponse = 0.1f;
		moveDir_ = VNorm(VAdd(VScale(moveDir_, 1.0f - turnResponse), VScale(inputDir, turnResponse)));

		speed_ = AsoUtility::Lerp(speed_, targetSpeed, accelerationRatio);
		movePow_ = VScale(moveDir_, speed_);

		if (!isJump_ && IsEndLanding()) {
			animationController_->Play(isDashing ? (int)ANIM_TYPE::FAST_RUN : (int)ANIM_TYPE::RUN);
		}
	}
	else {
		// 上昇中（isBoostAscent_）であっても、スティックを離しているなら
	// 水平方向の目標速度は 0.0f にする
		float targetSpeed = 0.0f;

		// 減速のなめらかさ（decelerationRatio）
		// ダッシュの余韻がある間（isDashing）や空中（isJump_）は少し滑らせ、
		// それ以外はピタッと止める
		float decelerationRatio = (isDashing || isJump_) ? 0.05f : 0.2f;

		speed_ = AsoUtility::Lerp(speed_, targetSpeed, decelerationRatio);

		// 速度が極低速になったら完全に 0 にする（滑りすぎ防止）
		if (speed_ < 0.1f) speed_ = 0.0f;

		movePow_ = VScale(moveDir_, speed_);

		if (!isJump_ && IsEndLanding()) {
			animationController_->Play(isDashing ? (int)ANIM_TYPE::FAST_RUN : (int)ANIM_TYPE::IDLE);
		}
	}

	if (isJump_) {
		if (isDashKeyPress && dashPressDuration_ > LONG_PRESS_THRESHOLD) animationController_->Play((int)ANIM_TYPE::FLY);
		else if (jumpPow_.y < -1.0f) animationController_->Play((int)ANIM_TYPE::FALLING);
	}
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
	float rotSpeed = currentTurnSpeed_ * 5.0f;

	// transform_.quaRot ではなく playerRotY_ を更新する
	playerRotY_ = Quaternion::Slerp(playerRotY_, goalQuaRot_, rotSpeed);

	// 最後に transform_ にも反映させておくと他の処理（カメラ追従など）と整合性が取れます
	transform_.quaRot = playerRotY_;
}

void Player::Collision(void)
{

	// 現在座標を起点に移動後座標を決める
	movedPos_ = VAdd(transform_.pos, movePow_);

	// 衝突(カプセル)
	CollisionCapsule();

	// 衝突(重力)
	CollisionGravity();

	// 移動
	transform_.pos = movedPos_;

}

void Player::CollisionGravity(void)
{
	// 上昇・重力の移動量を反映
	movedPos_ = VAdd(movedPos_, jumpPow_);

	// 上昇中は接地判定を行わない（地面に吸い込まれるのを防ぐ）
	if (jumpPow_.y > 0.001f) return;

	VECTOR dirGravity = AsoUtility::DIR_D;
	VECTOR dirUpGravity = AsoUtility::DIR_U;
	float checkPow = 10.0f;

	gravHitPosUp_ = VAdd(movedPos_, VScale(dirUpGravity, 20.0f));
	gravHitPosDown_ = VAdd(movedPos_, VScale(dirGravity, checkPow));

	for (const auto c : colliders_)
	{
		auto hit = MV1CollCheck_Line(c->modelId_, -1, gravHitPosUp_, gravHitPosDown_);

		if (hit.HitFlag > 0)
		{
			// 接地処理
			movedPos_ = VAdd(hit.HitPosition, VScale(dirUpGravity, 2.0f));
			jumpPow_ = AsoUtility::VECTOR_ZERO;

			if (isJump_)
			{
				// ★修正箇所：
				// ここで dashResidualTimer_ = 0.0f; をしていた場合は削除します。
				// 着地アニメーションのみ再生
				animationController_->Play((int)ANIM_TYPE::JUMP, false, 29.0f, 45.0f, false, true);
			}
			isJump_ = false;
		}
	}
}

void Player::CollisionCapsule(void)
{

	// カプセルを移動させる
	Transform trans = Transform(transform_);
	trans.pos = movedPos_;
	trans.Update();
	Capsule cap = Capsule(*capsule_, trans);

	// カプセルとの衝突判定
	for (const auto c : colliders_)
	{

		auto hits = MV1CollCheck_Capsule(
			c->modelId_, -1,
			cap.GetPosTop(), cap.GetPosDown(), cap.GetRadius());

		for (int i = 0; i < hits.HitNum; i++)
		{

			auto hit = hits.Dim[i];

			for (int tryCnt = 0; tryCnt < 10; tryCnt++)
			{

				int pHit = HitCheck_Capsule_Triangle(
					cap.GetPosTop(), cap.GetPosDown(), cap.GetRadius(),
					hit.Position[0], hit.Position[1], hit.Position[2]);

				if (pHit)
				{
					movedPos_ = VAdd(movedPos_, VScale(hit.Normal, 1.0f));
					// カプセルを移動させる
					trans.pos = movedPos_;
					trans.Update();
					continue;
				}

				break;

			}

		}

		// 検出した地面ポリゴン情報の後始末
		MV1CollResultPolyDimTerminate(hits);

	}

}

void Player::CalcGravityPow(void)
{
	// 重力加速度を計算
	float gravityVal = Planet::DEFAULT_GRAVITY_POW;

	// ★ジャンプ中（空中）の時は重力を弱める
	if (isJump_) {
		gravityVal *= 0.05f; // 重力を半分にして「ふわっと」させる
	}
	else {
		gravityVal *= 0.8f;// 落下は少し早めに（お好みで）
	}

	VECTOR gravity = VScale(AsoUtility::DIR_D, gravityVal);
	jumpPow_ = VAdd(jumpPow_, gravity);

	// 終端速度（落下しすぎ防止）
	if (jumpPow_.y < -10.0f) jumpPow_.y = -10.0f;
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