#include "../Collider/ColliderCapsule.h"
#include "../Collider/ColliderLine.h"
#include "../../Utility/AsoUtility.h"
#include "../Common/AnimationController.h"
#include "../../Manager/ResourceManager.h"
#include "../../Manager/SceneManager.h"
#include "../../Application.h"
#include "../../Object/Player.h"
#include "../../Object/Weapon/WeaponEnemyMissile.h"
#include "EnemyMissileMT.h"

EnemyMissileMT::EnemyMissileMT(const EnemyBase::EnemyData& data)
	:
	EnemyBase(data),
	state_(STATE::SEARCH)
{
}

EnemyMissileMT::~EnemyMissileMT(void)
{
}

void EnemyMissileMT::Draw(void)
{
	EnemyBase::Draw();

#ifdef _DEBUG
	DrawFormatString(0, 100, GetColor(255, 255, 255), "--- ENEMY MISSILE_MT DEB_ROT ---");
	DrawFormatString(0, 120, GetColor(255, 255, 255), "Current State : %d", (int)state_);
	DrawFormatString(0, 140, GetColor(255, 255, 255), "Transform RotY: %.3f", transform_.rot.y);
#endif
}

void EnemyMissileMT::InitLoad(void)
{
	EnemyBase::InitLoad();
	transform_.SetModel(resMng_.LoadModelDuplicate(ResourceManager::SRC::MISSILE_MT));
}

void EnemyMissileMT::InitTransform(void)
{
	transform_.scl = { 8.0f, 8.0f, 8.0f };
	transform_.quaRot = Quaternion::Identity();
	transform_.quaRotLocal = Quaternion::Euler(ROT);
	transform_.Update();
}

void EnemyMissileMT::InitCollider(void)
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

void EnemyMissileMT::InitAnimation(void)
{
	std::string path = Application::PATH_MODEL + "Player/";
	animationController_ = new AnimationController(transform_.modelId);
	animationController_->Add((int)ANIM_TYPE::IDLE, path + "Idle.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::WALK, path + "Run.mv1", 20.0f);

	animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
}

void EnemyMissileMT::InitPost(void)
{
	player_ = scnMng_.GetPlayer();

	stateChanges_.emplace(static_cast<int>(STATE::SEARCH), std::bind(&EnemyMissileMT::ChangeStateSearch, this));
	stateChanges_.emplace(static_cast<int>(STATE::COMBAT), std::bind(&EnemyMissileMT::ChangeStateCombat, this));

	state_ = STATE::SEARCH;
	ChangeStateSearch();

	// ⭕ 敵専用ミサイルを装備（武器名、無限弾、リロードフレーム、弾速、威力、寿命）
	weapon_ = new WeaponEnemyMissile("MT_MISSILE", 999, 10, 40.0f, 300, 600);

	// ⭕ 銃口位置を背中のミサイルポッド付近（少し高く、少し後ろ）に設定
	localMuzzlePos_ = VGet(-20.0f, 150.0f, -30.0f);
}

void EnemyMissileMT::UpdateProcess(void)
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

		// 武器側の共通インターフェース Fire を呼び出す
		weapon_->Fire(muzzlePos, targetPos, scnMng_.GetBulletList(), true);

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

void EnemyMissileMT::UpdateProcessPost(void)
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

	if (!InMovableRange())
	{
		transform_.pos = prevPos_;
		transform_.Update();
		ChangeState(STATE::SEARCH);
	}
}

void EnemyMissileMT::ChangeState(STATE state)
{
	if (state_ == state) return;
	state_ = state;

	int stateKey = static_cast<int>(state_);
	if (stateChanges_.count(stateKey) > 0)
	{
		stateChanges_[stateKey]();
	}
}

void EnemyMissileMT::ChangeStateSearch(void)
{
	stateUpdate_ = std::bind(&EnemyMissileMT::UpdateSearch, this);
	moveDir_ = AsoUtility::VECTOR_ZERO;
	movePow_ = AsoUtility::VECTOR_ZERO;

	if (animationController_ != nullptr)
	{
		animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
	}
}

void EnemyMissileMT::UpdateSearch(void)
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

void EnemyMissileMT::ChangeStateCombat(void)
{
	stateUpdate_ = std::bind(&EnemyMissileMT::UpdateCombat, this);
	directionTimer_ = 0.0f;
	shotTimer_ = 0.8f; // 遭遇後、ロックオンをまねて少し間をあける

	burstCount_ = 0;
	burstDelayTimer_ = 0.0f;

	if (animationController_ != nullptr)
	{
		animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
	}
}

void EnemyMissileMT::UpdateCombat(void)
{
	if (player_ == nullptr) return;

	directionTimer_ -= scnMng_.GetDeltaTime();
	if (directionTimer_ <= 0.0f)
	{
		sideMoveSign_ *= -1.0f;
		directionTimer_ = 1.5f + (rand() % 150) / 100.0f;
	}

	VECTOR localSideVec = VGet(sideMoveSign_, 0.0f, 0.0f);

	float dist = VSize(VSub(player_->GetTransform().pos, transform_.pos));
	VECTOR localForwardVec = AsoUtility::VECTOR_ZERO;

	// ミサイル機なので、通常のMT（200〜400）より一歩下がった遠距離（350〜550）をキープさせる
	if (dist > 550.0f)      localForwardVec = VGet(0.0f, 0.0f, 0.4f);
	else if (dist < 350.0f) localForwardVec = VGet(0.0f, 0.0f, -0.4f);

	VECTOR localCombinedDir = VAdd(localSideVec, localForwardVec);

	moveDir_ = transform_.quaRot.PosAxis(localCombinedDir);
	moveDir_ = AsoUtility::VNormalize(moveDir_);
	movePow_ = VScale(moveDir_, COMBAT_SPEED);

	if (dist > searchRadius_ * 1.2f)
	{
		ChangeState(STATE::SEARCH);
	}
}

void EnemyMissileMT::RotateToPlayer(void)
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