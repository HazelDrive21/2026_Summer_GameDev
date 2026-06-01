#include "../Collider/ColliderCapsule.h"
#include "../Collider/ColliderLine.h"
#include "../../Utility/AsoUtility.h"
#include "../Common/AnimationController.h"
#include "../../Manager/ResourceManager.h"
#include "../../Manager/SceneManager.h"
#include "../../Application.h"
#include "../../Object/Player.h"
#include "../../Object/Weapon/WeaponFirearm.h"
#include "EnemyMT.h"

EnemyMT::EnemyMT(const EnemyBase::EnemyData& data)
	:
	EnemyBase(data),
	state_(STATE::SEARCH)
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

	// ★状態遷移関数をSEARCHとCOMBATのみに絞って登録
	stateChanges_.emplace(static_cast<int>(STATE::SEARCH), std::bind(&EnemyMT::ChangeStateSearch, this));
	stateChanges_.emplace(static_cast<int>(STATE::COMBAT), std::bind(&EnemyMT::ChangeStateCombat, this));

	// 初期状態設定（索敵から開始）
	state_ = STATE::SEARCH;
	ChangeStateSearch();

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
	if (shotTimer_ > 0.0f)
	{
		shotTimer_ -= scnMng_.GetDeltaTime();
	}
	if (burstDelayTimer_ > 0.0f)
	{
		burstDelayTimer_ -= scnMng_.GetDeltaTime();
	}

	// --- 2. 射撃実行セクション ---
	bool isBurstTrigger = (burstCount_ == 0 && weapon_->IsReady() && shotTimer_ <= 0.0f);
	bool isNextBurstShot = (burstCount_ > 0 && burstDelayTimer_ <= 0.0f);

	if (isBurstTrigger || isNextBurstShot)
	{
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

		VECTOR muzzlePos = VAdd(transform_.pos, localMuzzlePos_);
		weapon_->Fire(muzzlePos, targetPos, scnMng_.GetBulletList(),true);

		burstCount_++;

		if (burstCount_ >= BURST_SHOT_NUM)
		{
			burstCount_ = 0;
			burstDelayTimer_ = 0.0f;
			shotTimer_ = SHOT_INTERVAL;
		}
		else
		{
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

	if (state_ == STATE::COMBAT || state_ == STATE::SEARCH)
	{
		RotateToPlayer();
	}

	// 移動範囲外のチェック
	if (!InMovableRange())
	{
		transform_.pos = prevPos_;
		transform_.Update();

		// ★THINK状態が無くなったため、範囲外に出たら即座にSEARCHに戻す
		ChangeState(STATE::SEARCH);
	}
}

void EnemyMT::ChangeState(STATE state)
{
	if (state_ == state) return;
	state_ = state;

	int stateKey = static_cast<int>(state_);
	if (stateChanges_.count(stateKey) > 0)
	{
		stateChanges_[stateKey]();
	}
}

// --- 🔎 索敵状態への遷移と更新 ---
void EnemyMT::ChangeStateSearch(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateSearch, this);
	moveDir_ = AsoUtility::VECTOR_ZERO;
	movePow_ = AsoUtility::VECTOR_ZERO;

	if (animationController_ != nullptr)
	{
		animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
	}
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

// --- ⚔️ 戦闘状態への遷移と更新 ---
void EnemyMT::ChangeStateCombat(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateCombat, this);
	directionTimer_ = 0.0f;
	shotTimer_ = 0.5f; // 遭遇後、少しだけ間を置いてから撃ち始める演出

	burstCount_ = 0;
	burstDelayTimer_ = 0.0f;

	if (animationController_ != nullptr)
	{
		animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
	}
}

void EnemyMT::UpdateCombat(void)
{
	if (player_ == nullptr) return;

	// 1. 左右スライドの切り返しタイマー
	directionTimer_ -= scnMng_.GetDeltaTime();
	if (directionTimer_ <= 0.0f)
	{
		sideMoveSign_ *= -1.0f;
		directionTimer_ = 1.5f + (rand() % 150) / 100.0f;
	}

	VECTOR localSideVec = VGet(sideMoveSign_, 0.0f, 0.0f);

	// 2. 距離に応じた前後移動（間合い管理）
	float dist = VSize(VSub(player_->GetTransform().pos, transform_.pos));
	VECTOR localForwardVec = AsoUtility::VECTOR_ZERO;

	if (dist > 400.0f)      localForwardVec = VGet(0.0f, 0.0f, 0.4f);
	else if (dist < 200.0f) localForwardVec = VGet(0.0f, 0.0f, -0.4f);

	VECTOR localCombinedDir = VAdd(localSideVec, localForwardVec);

	// 3. 移動ベクトルの確定
	moveDir_ = transform_.quaRot.PosAxis(localCombinedDir);
	moveDir_ = AsoUtility::VNormalize(moveDir_);
	movePow_ = VScale(moveDir_, COMBAT_SPEED);

	// 4. プレイヤーが離れたら索敵に戻る
	if (dist > searchRadius_ * 1.2f)
	{
		ChangeState(STATE::SEARCH);
	}
}

// --- 🔄 旋回ヘルパー ---
void EnemyMT::RotateToPlayer(void)
{
	if (player_ == nullptr) return;

	VECTOR playerPos = player_->GetTransform().pos;
	VECTOR toPlayer = VSub(playerPos, transform_.pos);
	toPlayer.y = 0.0f;

	if (VSize(toPlayer) > 0.5f)
	{
		Quaternion targetLook = Quaternion::LookRotation(toPlayer);
		Quaternion targetRot = Quaternion::Mult(targetLook, transform_.quaRotLocal);

		Quaternion flipRot = Quaternion::AngleAxis(DX_PI_F, VGet(0, 1, 0));
		targetRot = Quaternion::Mult(targetRot, flipRot);

		transform_.quaRot = Quaternion::Slerp(transform_.quaRot, targetRot, ROT_SPEED);
		transform_.rot = transform_.quaRot.ToEuler();
		transform_.Update();
	}
}