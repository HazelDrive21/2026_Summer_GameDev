#include "../Collider/ColliderCapsule.h"
#include "../Collider/ColliderLine.h"
#include "../../Utility/AsoUtility.h"
#include "../Common/AnimationController.h"
#include "../../Manager/ResourceManager.h"
#include "../../Manager/SceneManager.h"
#include "../../Application.h"
#include "../../Object/Player.h"
#include "../../Object/Wepon/WeaponFirearm.h"
#include "EnemyMT.h"

EnemyMT::EnemyMT(const EnemyBase::EnemyData& data)
	:
	EnemyBase(data),
	state_(STATE::NONE),
	step_(0.0f)
{
}

EnemyMT::~EnemyMT(void)
{
}

void EnemyMT::Draw(void)
{
	EnemyBase::Draw();

#ifdef _DEBUG
	DrawFormatString(0, 100, GetColor(255, 255, 255), "--- ENEMY DEB_ROT ---");
	DrawFormatString(0, 120, GetColor(255, 255, 255), "Current State : %d", (int)state_);
	DrawFormatString(0, 140, GetColor(255, 255, 255), "Transform RotY: %.3f", transform_.rot.y);
#endif
}

void EnemyMT::InitLoad(void)
{
	EnemyBase::InitLoad();
	transform_.SetModel(resMng_.LoadModelDuplicate(ResourceManager::SRC::ENEMY_MT1));
}

void EnemyMT::InitTransform(void)
{
	transform_.scl = { 8.0f, 8.0f, 8.0f };
	transform_.quaRot = Quaternion::Identity();
	transform_.quaRotLocal = Quaternion::Euler(ROT);
	transform_.Update();
}

void EnemyMT::InitCollider(void)
{
	ColliderLine* colLine = new ColliderLine(
		ColliderBase::TAG::ENEMY, &transform_,
		COL_LINE_START_LOCAL_POS, COL_LINE_END_LOCAL_POS);
	ownColliders_.emplace(static_cast<int>(COLLIDER_TYPE::LINE), colLine);

	ColliderCapsule* colCapsule = new ColliderCapsule(
		ColliderBase::TAG::ENEMY, &transform_,
		COL_CAPSULE_TOP_LOCAL_POS, COL_CAPSULE_DOWN_LOCAL_POS,
		COL_CAPSULE_RADIUS);
	ownColliders_.emplace(static_cast<int>(COLLIDER_TYPE::CAPSULE), colCapsule);
}

void EnemyMT::InitAnimation(void)
{
	std::string path = Application::PATH_MODEL + "Player/";
	animationController_ = new AnimationController(transform_.modelId);
	animationController_->Add((int)ANIM_TYPE::IDLE, path + "Idle.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::WALK, path + "Run.mv1", 20.0f);

	animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
}

void EnemyMT::InitPost(void)
{
	player_ = scnMng_.GetPlayer();

	// すべての状態遷移関数をマップに完全登録
	stateChanges_.emplace(static_cast<int>(STATE::NONE), std::bind(&EnemyMT::ChangeStateNone, this));
	stateChanges_.emplace(static_cast<int>(STATE::THINK), std::bind(&EnemyMT::ChangeStateThink, this));
	stateChanges_.emplace(static_cast<int>(STATE::IDLE), std::bind(&EnemyMT::ChangeStateIdle, this));
	stateChanges_.emplace(static_cast<int>(STATE::WANDER), std::bind(&EnemyMT::ChangeStateWander, this));
	stateChanges_.emplace(static_cast<int>(STATE::SEARCH), std::bind(&EnemyMT::ChangeStateSearch, this));
	stateChanges_.emplace(static_cast<int>(STATE::COMBAT), std::bind(&EnemyMT::ChangeStateCombat, this));
	stateChanges_.emplace(static_cast<int>(STATE::END), std::bind(&EnemyMT::ChangeStateEnd, this));

	// 初期状態設定
	ChangeState(STATE::SEARCH);

	weapon_ = new WeaponFirearm("MT_RIFLE", 999, 5, 250.0f, 200, 120);
	localMuzzlePos_ = VGet(0.0f, 80.0f, 50.0f);
}

void EnemyMT::UpdateProcess(void)
{
	if (player_ == nullptr) return;

	if (stateUpdate_ != nullptr)
	{
		stateUpdate_();
	}

	if (state_ != STATE::COMBAT) return;

	// --- 1. タイマー更新セクション ---
	// バーストとバーストの間のインターバルタイマー
	if (shotTimer_ > 0.0f)
	{
		shotTimer_ -= scnMng_.GetDeltaTime();
	}
	// バースト中の弾と弾の間のディレイタイマー
	if (burstDelayTimer_ > 0.0f)
	{
		burstDelayTimer_ -= scnMng_.GetDeltaTime();
	}

	// --- 2. 射撃実行セクション ---
	// 条件A: 次のバーストのトリガーを引く瞬間
	// 条件B: 絶賛バースト連射中かつ、弾間のディレイが終了した瞬間
	bool isBurstTrigger = (burstCount_ == 0 && weapon_->IsReady() && shotTimer_ <= 0.0f);
	bool isNextBurstShot = (burstCount_ > 0 && burstDelayTimer_ <= 0.0f);

	if (isBurstTrigger || isNextBurstShot)
	{
		// ターゲット座標の計算（プレイヤーの中心を狙う）
		VECTOR targetPos = player_->GetTransform().pos;
		int capsuleKey = static_cast<int>(CharactorBase::COLLIDER_TYPE::CAPSULE);
		const auto& playerColliders = player_->GetOwnColliders();

		if (playerColliders.count(capsuleKey) > 0)
		{
			const ColliderBase* baseCollider = playerColliders.at(capsuleKey);
			if (baseCollider != nullptr && baseCollider->GetShape() == ColliderBase::SHAPE::CAPSULE)
			{
				const ColliderCapsule* capsule = static_cast<const ColliderCapsule*>(baseCollider);
				targetPos = capsule->GetCenter();
			}
		}

		// 発射！
		VECTOR muzzlePos = VAdd(transform_.pos, localMuzzlePos_);
		weapon_->Fire(muzzlePos, targetPos, scnMng_.GetBulletList());

		// カウントを1増やす
		burstCount_++;

		if (burstCount_ >= BURST_SHOT_NUM)
		{
			// 指定発数（3発など）を撃ち切ったらバースト終了
			burstCount_ = 0;
			burstDelayTimer_ = 0.0f;
			shotTimer_ = SHOT_INTERVAL; // 次のバーストまでの長いインターバルを開始
		}
		else
		{
			// まだバーストの途中なら、次の1発までの短いディレイを設定
			burstDelayTimer_ = BURST_DELAY;
		}
	}
}

void EnemyMT::UpdateProcessPost(void)
{
	EnemyBase::UpdateProcessPost();

	if (!isJump_)
	{
		transform_.pos.y = floorf(transform_.pos.y * 10.0f) / 10.0f;
	}

	// 索敵中または戦闘中はプレイヤーを滑らかに旋回して見つめ続ける
	if (state_ == STATE::COMBAT || state_ == STATE::SEARCH)
	{
		RotateToPlayer();
	}

	// 移動範囲外のチェック
	if (!InMovableRange())
	{
		transform_.pos = prevPos_;
		transform_.Update();
		ChangeState(STATE::THINK);
	}
}

void EnemyMT::ChangeState(STATE state)
{
	if (state_ == state) return;
	state_ = state;

	int stateKey = static_cast<int>(state_);
	if (stateChanges_.count(stateKey) > 0)
	{
		stateChanges_[stateKey](); // switch文を通さず一発で各初期化関数を呼び出す
	}
}

void EnemyMT::ChangeStateNone(void) { stateUpdate_ = std::bind(&EnemyMT::UpdateNone, this); }
void EnemyMT::ChangeStateEnd(void) { stateUpdate_ = std::bind(&EnemyMT::UpdateEnd, this); }

void EnemyMT::ChangeStateThink(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateThink, this);

	// 30%で待機、70%で徘徊
	int rand = GetRand(100);
	if (rand < 30) ChangeState(STATE::IDLE);
	else           ChangeState(STATE::WANDER);
}

void EnemyMT::ChangeStateIdle(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateIdle, this);
	step_ = 3.0f + static_cast<float>(GetRand(3));
	movePow_ = AsoUtility::VECTOR_ZERO;
	animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
}

void EnemyMT::ChangeStateWander(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateWander, this);
	float angle = static_cast<float>(GetRand(360)) * DX_PI_F / 180.0f;
	moveDir_ = VGet(cosf(angle), 0.0f, sinf(angle));
	step_ = 2.0f + static_cast<float>(GetRand(5));
	moveSpeed_ = 3.0f;
	animationController_->Play(static_cast<int>(ANIM_TYPE::WALK), true);
}

void EnemyMT::ChangeStateSearch(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateSearch, this);
	moveDir_ = AsoUtility::VECTOR_ZERO;
	movePow_ = AsoUtility::VECTOR_ZERO;
}

void EnemyMT::ChangeStateCombat(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateCombat, this);
	directionTimer_ = 0.0f; // 戦闘移行直後に即座に方向を選択させる
	shotTimer_ = 0.5f;      // 遭遇後、少しだけ間を置いてから撃ち始める（ACの先制猶予演出）

	burstCount_ = 0;
	burstDelayTimer_ = 0.0f;

	if (animationController_ != nullptr)
	{
		animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
	}
}

void EnemyMT::UpdateNone(void) {}
void EnemyMT::UpdateThink(void) {}
void EnemyMT::UpdateEnd(void) {}

void EnemyMT::UpdateIdle(void)
{
	step_ -= scnMng_.GetDeltaTime();
	if (step_ <= 0.0f) ChangeState(STATE::THINK);
}

void EnemyMT::UpdateWander(void)
{
	step_ -= scnMng_.GetDeltaTime();
	if (step_ <= 0.0f)
	{
		ChangeState(STATE::THINK);
		return;
	}
	movePow_ = VScale(moveDir_, moveSpeed_);
}

void EnemyMT::UpdateSearch(void)
{
	if (player_ == nullptr) return;

	float dist = VSize(VSub(player_->GetTransform().pos, transform_.pos));
	if (dist <= searchRadius_)
	{
		ChangeState(STATE::COMBAT);
	}
	else
	{
		moveDir_ = AsoUtility::VECTOR_ZERO;
		movePow_ = AsoUtility::VECTOR_ZERO;
	}
}

// --- 【AC風サークリング移動】戦闘状態の更新 ---
void EnemyMT::UpdateCombat(void)
{
	if (player_ == nullptr) return;

	// 1. 左右スライドの切り返しタイマーの更新
	directionTimer_ -= scnMng_.GetDeltaTime();
	if (directionTimer_ <= 0.0f)
	{
		// 左右の移動方向を反転（1.0f ⇄ -1.0f）
		sideMoveSign_ *= -1.0f;
		// 切り返しまでの時間をランダムに設定（1.5秒 〜 3.0秒）
		directionTimer_ = 1.5f + (rand() % 150) / 100.0f;
	}

	// 2. プレイヤーを常に見ている自分の回転から、ローカルの左右スライドベクトルを抽出
	VECTOR localSideVec = VGet(sideMoveSign_, 0.0f, 0.0f);

	// 3. 【ACらしさの肝】距離に応じた前進・後退の調整（間合い管理）
	float dist = VSize(VSub(player_->GetTransform().pos, transform_.pos));
	VECTOR localForwardVec = AsoUtility::VECTOR_ZERO;

	if (dist > 400.0f)      localForwardVec = VGet(0.0f, 0.0f, 0.4f);  // 離れすぎなら少し近づく
	else if (dist < 200.0f) localForwardVec = VGet(0.0f, 0.0f, -0.4f); // 近すぎなら少し離れる

	// 横移動ベクトルと前後ベクトルを合成
	VECTOR localCombinedDir = VAdd(localSideVec, localForwardVec);

	// ローカル方向ベクトルをワールドの移動ベクトルへ変換
	moveDir_ = transform_.quaRot.PosAxis(localCombinedDir);
	moveDir_ = AsoUtility::VNormalize(moveDir_);

	// 移動量に戦闘速度を適用
	movePow_ = VScale(moveDir_, COMBAT_SPEED);

	// 4. プレイヤーが索敵半径から完全に離れたら索敵（SEARCH）に戻る
	if (dist > searchRadius_ * 1.2f)
	{
		ChangeState(STATE::SEARCH);
	}
}

// --- 【クリーンアップ】プレイヤーの方をゆっくり向く旋回ヘルパー ---
void EnemyMT::RotateToPlayer(void)
{
	if (player_ == nullptr) return;

	VECTOR playerPos = player_->GetTransform().pos;
	VECTOR toPlayer = VSub(playerPos, transform_.pos);
	toPlayer.y = 0.0f; // 水平方向のみのベクトルにする

	if (VSize(toPlayer) > 0.5f)
	{
		// ターゲット方向（プレイヤー）へのLookRotationを計算
		Quaternion targetLook = Quaternion::LookRotation(toPlayer);

		// モデル固有の初期回転を合成
		Quaternion targetRot = Quaternion::Mult(targetLook, transform_.quaRotLocal);

		// 反転補正
		Quaternion flipRot = Quaternion::AngleAxis(DX_PI_F, VGet(0, 1, 0));
		targetRot = Quaternion::Mult(targetRot, flipRot);

		// ヘッダーで定義された ROT_SPEED (0.05f) を使って滑らかに補間旋回
		transform_.quaRot = Quaternion::Slerp(transform_.quaRot, targetRot, ROT_SPEED);

		// オイラー角のYも同期
		transform_.rot = transform_.quaRot.ToEuler();
		transform_.Update();
	}
}