
#include "../Collider/ColliderCapsule.h"
#include "../Collider/ColliderLine.h"
#include "../../Utility/AsoUtility.h"
#include "../Common/AnimationController.h"
#include "../../Manager/ResourceManager.h"
#include "../../Manager/SceneManager.h"
#include "../../Application.h"
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

void EnemyMT::InitLoad(void)
{
	// 基底クラスのリソースロード
	EnemyBase::InitLoad();

	// モデルのロード
	transform_.SetModel(
		resMng_.LoadModelDuplicate(ResourceManager::SRC::ENEMY_MT1));
}

void EnemyMT::InitTransform(void)
{
	// モデルの基本設定
	transform_.scl = {8.0f, 8.0f, 8.0f};
	transform_.quaRot = Quaternion::Identity();
	transform_.quaRotLocal = Quaternion::Euler(ROT);
	transform_.Update();
}

void EnemyMT::InitCollider(void)
{
	// 主に地面との衝突で仕様する線分コライダ
	ColliderLine* colLine = new ColliderLine(
		ColliderBase::TAG::ENEMY, &transform_,
		COL_LINE_START_LOCAL_POS, COL_LINE_END_LOCAL_POS);
	ownColliders_.emplace(static_cast<int>(COLLIDER_TYPE::LINE), colLine);

	// 主に壁や木などの衝突で仕様するカプセルコライダ
	ColliderCapsule* colCapsule = new ColliderCapsule(
		ColliderBase::TAG::ENEMY, &transform_,
		COL_CAPSULE_TOP_LOCAL_POS, COL_CAPSULE_DOWN_LOCAL_POS,
		COL_CAPSULE_RADIUS);
	ownColliders_.emplace(static_cast<int>(COLLIDER_TYPE::CAPSULE), colCapsule);
}

void EnemyMT::InitAnimation(void)
{
	animationController_ = new AnimationController(transform_.modelId);

	//int type = -1;

	std::string path = Application::PATH_MODEL + "Player/";
	animationController_ = new AnimationController(transform_.modelId);
	animationController_->Add((int)ANIM_TYPE::IDLE, path + "Idle.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::WALK, path + "Run.mv1", 20.0f);

	// 初期アニメーション再生
	animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
}

void EnemyMT::InitPost(void)
{

	// 状態遷移初期処理登録
	stateChanges_.emplace(static_cast<int>(STATE::NONE),
		std::bind(&EnemyMT::ChangeStateNone, this));
	stateChanges_.emplace(static_cast<int>(STATE::THINK),
		std::bind(&EnemyMT::ChangeStateThink, this));
	stateChanges_.emplace(static_cast<int>(STATE::IDLE),
		std::bind(&EnemyMT::ChangeStateIdle, this));
	stateChanges_.emplace(static_cast<int>(STATE::WANDER),
		std::bind(&EnemyMT::ChangeStateWander, this));
	stateChanges_.emplace(static_cast<int>(STATE::END),
		std::bind(&EnemyMT::ChangeStateEnd, this));

	// 初期状態設定
	ChangeState(STATE::THINK);
}

void EnemyMT::UpdateProcess(void)
{
	// 状態別更新
	stateUpdate_();
}

void EnemyMT::UpdateProcessPost(void)
{
	EnemyBase::UpdateProcessPost();

	if (!InMovableRange())
	{
		// 移動範囲外に出たら移動前座標に戻す
		transform_.pos = prevPos_;
		transform_.Update();

		// 思考状態に戻す
		ChangeState(STATE::THINK);
	}
}

void EnemyMT::ChangeState(STATE state)
{
	state_ = state;
	// 各状態遷移の初期処理
	EnemyBase::ChangeState(static_cast<int>(state_));
}

void EnemyMT::ChangeStateNone(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateNone, this);
}
void EnemyMT::ChangeStateThink(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateThink, this);

	// 思考
// ランダムに次の行動を決定
// 30%で待機、70%で徘徊
	int rand = GetRand(100);
	if (rand < 30)
	{
		ChangeState(STATE::IDLE);
	}
	else
	{
		ChangeState(STATE::WANDER);
	}

}
void EnemyMT::ChangeStateIdle(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateIdle, this);

	// ランダムな待機時間
	step_ = 3.0f + static_cast<float>(GetRand(3));
	// 移動量ゼロ
	movePow_ = AsoUtility::VECTOR_ZERO;
	// 待機アニメーション再生
	animationController_->Play(
		static_cast<int>(ANIM_TYPE::IDLE), true);

}
void EnemyMT::ChangeStateWander(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateWander, this);

	// ランダムな角度
	float angle = static_cast<float>(GetRand(360)) * DX_PI_F / 180.0f;
	// 移動方向
	moveDir_ = VGet(cosf(angle), 0.0f, sinf(angle));
	// ランダムな移動時間
	step_ = 2.0f + static_cast<float>(GetRand(5));
	// 移動スピード
	moveSpeed_ = 3.0f;
	// 歩きアニメーション再生
	animationController_->Play(
		static_cast<int>(ANIM_TYPE::WALK), true);

}
void EnemyMT::ChangeStateEnd(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateEnd, this);
}

void EnemyMT::UpdateNone(void)
{
}

void EnemyMT::UpdateThink(void)
{
}

void EnemyMT::UpdateIdle(void)
{
	step_ -= scnMng_.GetDeltaTime();
	if (step_ <= 0.0f)
	{
		// 待機終了
		ChangeState(STATE::THINK);
		return;
	}
}

void EnemyMT::UpdateWander(void)
{
	step_ -= scnMng_.GetDeltaTime();
	if (step_ <= 0.0f)
	{
		// 移動終了
		ChangeState(STATE::THINK);
		return;
	}

	// 移動量
	movePow_ = VScale(moveDir_, moveSpeed_);
}

void EnemyMT::UpdateEnd(void)
{
}
