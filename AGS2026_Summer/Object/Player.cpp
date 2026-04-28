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
	float currentTurnSpeed = 0.0015f; // 本来はパーツのステータスから取得

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
	stopTimer_ = STOP_TIME; // 硬直時間をセット
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
	bool isDashKeyNew = (isDashKeyPress && !oldDashKey_);
	oldDashKey_ = isDashKeyPress;

	// --- 2. 移動方向の先行計算 ( combinedDir ) ---
	VECTOR forward = transform_.quaRot.GetForward();
	VECTOR right = transform_.quaRot.GetRight();
	VECTOR combinedDir = AsoUtility::VECTOR_ZERO;

	// 前後移動
	float stickY = padState.AKeyLY / 1000.0f;
	if (abs(stickY) > 0.2f) combinedDir = VAdd(combinedDir, VScale(forward, -stickY));

	// 左右平行移動 (InputManagerのスタックを利用)
	auto hDirType = ins.GetHorizontalDir();
	if (hDirType == InputManager::MoveDir::Left)  combinedDir = VSub(combinedDir, right);
	if (hDirType == InputManager::MoveDir::Right) combinedDir = VAdd(combinedDir, right);

	bool hasMoveInput = (VSize(combinedDir) > 0.1f);

	// --- 3. ダブルタップ & 長押しロジック ---
	if (dashTapTimer_ > 0.0f) {
		dashTapTimer_ -= deltaTime;
		if (dashTapTimer_ <= 0.0f) dashTapCount_ = 0;
	}

	if (isDashKeyPress) {
		dashPressDuration_ += deltaTime;

		// 【上昇】長押し (静止中 or 空中)
		if (dashPressDuration_ > LONG_PRESS_THRESHOLD) {
			if (!hasMoveInput || isJump_) {
				jumpPow_.y += BOOSTER_POW;
				if (jumpPow_.y > MAX_ASCENT_SPEED) jumpPow_.y = MAX_ASCENT_SPEED;

				if (!isJump_) {
					isJump_ = true;
					jumpPow_.y = 1.0f;
					animationController_->Play((int)ANIM_TYPE::JUMP, true, 13.0f, 25.0f);
				}
			}
			dashResidualTimer_ = DASH_RESIDUAL_TIME; // ダッシュ状態維持
		}

		// 【ジャンプ判定】押した瞬間
		if (isDashKeyNew) {
			if (dashTapTimer_ > 0.0f && dashTapCount_ == 1) {
				// ★修正ポイント：
				// 移動中であっても、2回素早く押せばジャンプを許可するように変更
				// もし「静止時のみ」を厳守したい場合は、!hasMoveInput を残しますが、
				// 操作性を上げるために条件を緩和するのが一般的です。
				if (!isJump_) {
					isJump_ = true;
					jumpPow_.y = POW_JUMP;
					animationController_->Play((int)ANIM_TYPE::JUMP, true, 13.0f, 25.0f);
					dashTapCount_ = 0;
					dashTapTimer_ = 0.0f;
				}
			}
			else {
				// 1回目の入力：地上ダッシュ（ブースト）を開始させる
				dashTapCount_ = 1;
				dashTapTimer_ = DOUBLE_TAP_TIME;

				// 移動中なら即座にダッシュ時間を付与して加速させる
				if (hasMoveInput) {
					dashResidualTimer_ = DASH_RESIDUAL_TIME;
				}
			}
		}
	}
	else {
		dashPressDuration_ = 0.0f;
		if (!isJump_) {
			dashResidualTimer_ -= deltaTime;
			if (dashResidualTimer_ < 0.0f) dashResidualTimer_ = 0.0f;
		}
	}

	// --- 4. ダッシュ判定 & 急停止(STOP)の判定 ---
	bool isDashing = (dashResidualTimer_ > 0.0f);

	// 地上での急停止判定 (ここでの return を条件付きにする)
	if (!isJump_ && isDashingBefore_ && !isDashing) {
		isDashingBefore_ = false;
		ChangeState(STATE::STOP);
		return;
	}
	isDashingBefore_ = isDashing;

	// --- 5. 旋回操作 (ここで回転が行われます) ---
	float stickX = padState.AKeyLX / 1000.0f;
	if (abs(stickX) > 0.2f) {
		// 旋回速度 currentTurnSpeed_ を適用
		Quaternion addRot = Quaternion::AngleAxis(stickX * currentTurnSpeed_, AsoUtility::AXIS_Y);
		goalQuaRot_ = Quaternion::Mult(goalQuaRot_, addRot);
	}

	// --- 6. 移動の適用と空中減速 ---
	if (hasMoveInput) {
		moveDir_ = VNorm(combinedDir);
		float targetSpeed = isDashing ? SPEED_RUN : SPEED_MOVE;
		float lerpRatio = 0.1f;

		if (isJump_) {
			float dot = VDot(VNorm(movePow_), moveDir_);
			if (dot < 0.0f) {
				speed_ = 0.0f; // 空中での逆入力による停止
				lerpRatio = 0.01f;
			}
			else {
				lerpRatio = 0.05f;
			}
		}

		speed_ = AsoUtility::Lerp(speed_, targetSpeed, lerpRatio);
		movePow_ = VScale(moveDir_, speed_);

		if (!isJump_ && IsEndLanding()) {
			animationController_->Play(isDashing ? (int)ANIM_TYPE::FAST_RUN : (int)ANIM_TYPE::RUN);
		}
	}
	else {
		// 移動入力なし
		speed_ = AsoUtility::Lerp(speed_, 0.0f, 0.2f);
		movePow_ = VScale(moveDir_, speed_);
		if (!isJump_ && IsEndLanding()) {
			animationController_->Play((int)ANIM_TYPE::IDLE);
		}
	}

	// 空中アニメーション
	if (isJump_) {
		if (isDashKeyPress && dashPressDuration_ > LONG_PRESS_THRESHOLD) {
			animationController_->Play((int)ANIM_TYPE::FLY);
		}
		else if (jumpPow_.y < -1.0f) {
			animationController_->Play((int)ANIM_TYPE::FALLING);
		}
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
	float response = 0.4f;
	playerRotY_ = Quaternion::Slerp(playerRotY_, goalQuaRot_, response);
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
		gravityVal *= 0.8f;  // 落下は少し早めに（お好みで）
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

void Player::UpdateStop(void) {
	float deltaTime = scnMng_.GetDeltaTime();

	// 1. 移動速度を急激に下げる（摩擦演出）
	// 0.2f を大きくするとよりピタッと止まり、小さくすると滑ります
	speed_ = AsoUtility::Lerp(speed_, 0.0f, 0.2f);

	// 入力方向ではなく、直前の移動方向(moveDir_)のまま慣性移動
	movePow_ = VScale(moveDir_, speed_);

	// 2. タイマー更新
	stopTimer_ -= deltaTime;

	// 3. アニメーション
	// 硬直中用のポーズ（踏ん張るなど）があればここで再生
	// なければ IDLE などの適切なものを設定
	if (!isJump_ && IsEndLanding()) {
		animationController_->Play((int)ANIM_TYPE::IDLE);
	}

	// 4. 重力と衝突判定は実行（これがないと空中で止まったり壁を抜けたりする）
	CalcGravityPow();
	Collision();

	// 5. 回転の適用（硬直中もモデルの向きは維持）
	transform_.quaRot = playerRotY_;

	// 6. 時間経過で復帰
	if (stopTimer_ <= 0.0f) {
		// 復帰時に速度を完全にゼロにしておくと、動き出しが綺麗です
		speed_ = 0.0f;
		ChangeState(STATE::PLAY);
	}
}