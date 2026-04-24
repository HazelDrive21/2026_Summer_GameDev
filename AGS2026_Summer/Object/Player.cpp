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

	// 1. 入力状態の取得
	bool isDashKeyNew = ins.IsNew(KEY_INPUT_LSHIFT) || ins.IsNew(KEY_INPUT_RSHIFT);
	bool isDashKeyPress = CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT);


	// 2. 【最優先】ジャンプ判定
	// 「タイマーが残っている（＝1回目がすでに押された）」かつ「今新しく押された（＝2回目）」
	if (!isJump_ && dashResidualTimer_ > 0.0f && isDashKeyNew) {

		DrawString(0, 200, "JUMP TRIGGERED!", GetColor(255, 0, 0));

		isJump_ = true;
		jumpPow_.y = POW_JUMP;

		animationController_->Play((int)ANIM_TYPE::JUMP, true, 13.0f, 25.0f);
		animationController_->SetEndLoop(23.0f, 25.0f, 5.0f);

		dashResidualTimer_ = 0.0f; // ジャンプしたのでタイマーリセット
		isDashingBefore_ = false;
		return;
	}

	// 3. ダッシュタイマーの更新（ジャンプしなかった場合のみ）
	if (isDashKeyPress) {
		// 押されている間は最大値で固定
		dashResidualTimer_ = DASH_RESIDUAL_TIME;
	}
	else {
		// 離している間はカウントダウン
		dashResidualTimer_ -= deltaTime;
		if (dashResidualTimer_ < 0.0f) dashResidualTimer_ = 0.0f;
	}

	bool isDashing = (dashResidualTimer_ > 0.0f);

	

	// --- 硬直遷移判定 ---
	if (!isJump_ && isDashingBefore_ && !isDashing) {
		isDashingBefore_ = false;
		ChangeState(STATE::STOP);
		return;
	}
	isDashingBefore_ = isDashing;

	// --- 以下、移動方向計算 ---
	auto* camera = SceneManager::GetInstance().GetCamera();
	float camY = camera->GetAngles().y;
	VECTOR camForward = VGet(sinf(camY), 0.0f, cosf(camY));
	VECTOR camRight = VGet(cosf(camY), 0.0f, -sinf(camY));

	auto hDirType = ins.GetHorizontalDir();
	auto vDirType = ins.GetVerticalDir();

	VECTOR combinedDir = AsoUtility::VECTOR_ZERO;
	if (vDirType == InputManager::MoveDir::Up)    combinedDir = VAdd(combinedDir, camForward);
	if (vDirType == InputManager::MoveDir::Down)  combinedDir = VSub(combinedDir, camForward);
	if (hDirType == InputManager::MoveDir::Right) combinedDir = VAdd(combinedDir, camRight);
	if (hDirType == InputManager::MoveDir::Left)   combinedDir = VSub(combinedDir, camRight);

	if (!AsoUtility::EqualsVZero(combinedDir)) {
		moveDir_ = VNorm(combinedDir);
		float targetSpeed = isDashing ? SPEED_RUN : SPEED_MOVE;
		speed_ = AsoUtility::Lerp(speed_, targetSpeed, 0.1f);
		movePow_ = VScale(moveDir_, speed_);
		goalQuaRot_ = Quaternion::AngleAxis(camY, AsoUtility::AXIS_Y);

		if (!isJump_ && IsEndLanding()) {
			animationController_->Play(isDashing ? (int)ANIM_TYPE::FAST_RUN : (int)ANIM_TYPE::RUN);
		}
	}
	else {
		// 入力がない場合
		speed_ = AsoUtility::Lerp(speed_, 0.0f, 0.2f);
		movePow_ = VScale(moveDir_, speed_);
		goalQuaRot_ = Quaternion::AngleAxis(camY, AsoUtility::AXIS_Y);
		if (!isJump_ && IsEndLanding()) {
			animationController_->Play((int)ANIM_TYPE::IDLE);
		}
	}
}

void Player::ProcessJump(void)
{
	// ジャンプ中でなければ何もしない
	if (!isJump_) return;

	auto& ins = InputManager::GetInstance();
	// ダッシュボタン（LShift/RShift）が押しっぱなしなら少し高く飛ぶ設定
	bool isHitHold = CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT);

	if (isHitHold && stepJump_ < TIME_JUMP_IN)
	{
		stepJump_ += scnMng_.GetDeltaTime();
		// 重力を相殺して上昇を維持
		jumpPow_.y = POW_JUMP;
	}
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
	// 旋回性能（パーツ性能）
	// 1.0f だと瞬時に向き、値を小さくするとゆっくり回る
	float turnSpeed = 0.2f;

	// 現在の回転から目標の回転へ一定速度で近づける
	playerRotY_ = Quaternion::Slerp(playerRotY_, goalQuaRot_, turnSpeed);
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
	// ジャンプ・重力の移動量を反映
	movedPos_ = VAdd(movedPos_, jumpPow_);

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
			// 【重要】上昇中(y > 0)は接地判定をスルーする
			if (jumpPow_.y <= 0.0f)
			{
				movedPos_ = VAdd(hit.HitPosition, VScale(dirUpGravity, 2.0f));

				// 接地したので値をリセット
				jumpPow_ = AsoUtility::VECTOR_ZERO;
				stepJump_ = 0.0f;

				if (isJump_)
				{
					// 着地アニメーション（終了フレーム指定）
					animationController_->Play((int)ANIM_TYPE::JUMP, false, 29.0f, 45.0f, false, true);
				}
				isJump_ = false;
			}
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
	// 重力加速度を加算するだけにする
	VECTOR gravity = VScale(AsoUtility::DIR_D, Planet::DEFAULT_GRAVITY_POW);
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