
#include "../Collider/ColliderCapsule.h"
#include "../Collider/ColliderLine.h"
#include "../../Utility/AsoUtility.h"
#include "../Common/AnimationController.h"
#include "../../Manager/ResourceManager.h"
#include "../../Manager/SceneManager.h"
#include "../../Application.h"
#include "../../Object/Player.h"
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
	ChangeState(STATE::SEARCH);
}

void EnemyMT::UpdateProcess(void)
{
	Player* player = SceneManager::GetInstance().GetPlayer();
	if (player != nullptr && state_ != STATE::COMBAT)
	{
		VECTOR playerPos = player->GetTransform().pos;

		// 自機とMTのベクトルの差を取り、長さを計算（距離）
		float dist = VSize(VSub(playerPos, transform_.pos));

		// 索敵半径内に入ったら戦闘状態へ！
		if (dist <= searchRadius_)
		{
			ChangeState(STATE::COMBAT);
		}
	}

	// 既存のステート更新
	if (stateUpdate_ != nullptr)
	{
		stateUpdate_();
	}
}

void EnemyMT::UpdateProcessPost(void)
{
	EnemyBase::UpdateProcessPost();

	if (!isJump_)
	{
		// Y座標の小数を切り捨てて、地面に完全に固定する（必要に応じて調整）
		transform_.pos.y = floorf(transform_.pos.y * 10.0f) / 10.0f;
	}

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
	if (state_ == state) return;
	state_ = state;

	switch (state_)
	{
	case STATE::SEARCH:
		ChangeStateSearch();
		break;
	case STATE::COMBAT:
		ChangeStateCombat();
		break;
	default:
		break;
	}
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

// --- 索敵状態への遷移 ---
void EnemyMT::ChangeStateSearch(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateSearch, this);

	moveDir_ = AsoUtility::VECTOR_ZERO;
	moveSpeed_ = 0.0f;

	// 静止アニメーションをループ再生
	animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
}

// --- 索敵状態の更新 ---
void EnemyMT::UpdateSearch(void)
{
	// SceneManagerからプレイヤーを取得
	const Player* player = scnMng_.GetPlayer();
	if (player == nullptr) return;

	// プレイヤーとの距離を計算
	VECTOR playerPos = player->GetTransform().pos;
	float distance = VSize(VSub(playerPos, transform_.pos));

	// 索敵範囲内にプレイヤーが入ったら戦闘状態へ
	if (distance <= SEARCH_RANGE)
	{
		ChangeState(STATE::COMBAT);
	}
}

// --- 戦闘状態への遷移 ---
void EnemyMT::ChangeStateCombat(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateCombat, this);

	// 戦闘状態に入ったら、WANDER（徘徊）時の移動量をリセットして立ち止まる
	movePow_ = AsoUtility::VECTOR_ZERO;
	moveSpeed_ = 0.0f;

	// 待機用のアニメーションなどを再生（既にあれば）
	if (animationController_ != nullptr)
	{
		animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
	}
}

// --- 戦闘状態の更新 ---
void EnemyMT::UpdateCombat(void)
{
	// SceneManager からプレイヤーを取得
	Player* player = scnMng_.GetPlayer();
	if (player == nullptr) return;

	// プレイヤーの現在座標を取得
	VECTOR playerPos = player->GetTransform().pos;

	// 自身からプレイヤーへ向かうベクトルを計算
	VECTOR toPlayer = VSub(playerPos, transform_.pos);

	// Y軸回転（水平旋回）のみにするため、高低差をゼロにする
	toPlayer.y = 0.0f;

	if (VSize(toPlayer) > 0.1f)
	{
		// 1. プレイヤーの方向（XZ平面）への角度を計算 (ラジアン)
		float targetAngle = atan2f(toPlayer.x, toPlayer.z);

		// 2. Y軸の回転クォータニオンを作成
		Quaternion targetRot = Quaternion::Euler({ 0.0f, targetAngle, 0.0f });

		// 3. ★【モデルの向き補正】
		// モデルが元々逆を向いている等の場合、ここでローカルの逆回転を掛けます。
		// もしこれで直らない場合、順番を逆（Mult(transform_.quaRotLocal.Inverse(), targetRot)）にしてみてください。
		targetRot = Quaternion::Mult(transform_.quaRotLocal.Inverse(), targetRot);

		// 4. Slerpを使って滑らかに回転させる（3.0fだと大きすぎるので 0.05f 〜 0.1f 程度に）
		// ※ ROT_SPEED という定数があればそれ（例: 0.05f）を、なければ直接数値を入れます。
		float rotSpeed = 0.05f;
		transform_.quaRot = Quaternion::Slerp(transform_.quaRot, targetRot, rotSpeed);
	}

	// --- 索敵範囲外に出たかどうかの判定（ロスト処理） ---
	float dist = VSize(VSub(playerPos, transform_.pos));
	if (dist > searchRadius_ * 1.2f)
	{
		ChangeState(STATE::THINK);
	}
}

// --- プレイヤーの方をゆっくり向く旋回ヘルパー ---
void EnemyMT::RotateToPlayer(const VECTOR& toPlayerDimXZ)
{
	// ターゲット方向へのクォータニオンを計算
	// ※ ユーザー様のQuaternionクラスの仕様に合わせて調整してください。
	// 一般的には、前方ベクトル(0,0,1)からtoPlayerDimXZへの回転を求めるか、
	// atan2から角度を出してY軸回転のクォータニオンを作ります。

	float targetAngle = atan2f(toPlayerDimXZ.x, toPlayerDimXZ.z);
	Quaternion targetRot = Quaternion::Euler({ 0.0f, targetAngle, 0.0f });

	// 現在の回転からターゲットの回転へLerp（またはSlerp）で補間して滑らかに旋回
	// ※ もし Quaternion::Lerp もしくは Slerp があればそれを利用
	transform_.quaRot = Quaternion::Slerp(transform_.quaRot, targetRot, ROT_SPEED);
}