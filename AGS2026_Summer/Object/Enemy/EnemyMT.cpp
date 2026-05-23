
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

void EnemyMT::Draw(void)
{
	// ① まず親クラス（EnemyBase）の描画を絶対に呼ぶ！
	// これにより、3Dモデルの描画や、他のデバッグ球体が表示されます
	EnemyBase::Draw();

#ifdef _DEBUG
	DrawFormatString(0, 100, GetColor(255, 255, 255), "--- ENEMY DEB_ROT ---");
	DrawFormatString(0, 120, GetColor(255, 255, 255), "Current State : %d", (int)state_);
	DrawFormatString(0, 140, GetColor(255, 255, 255), "Transform RotY: %.3f", transform_.rot.y);
#endif
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
	transform_.scl = { 8.0f, 8.0f, 8.0f };
	transform_.quaRot = Quaternion::Identity();
	transform_.quaRotLocal = Quaternion::Euler(ROT); // 元通りにする
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
	player_ = scnMng_.GetPlayer();

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
		// Y座標の小数を切り捨てて、地面に完全に固定する
		transform_.pos.y = floorf(transform_.pos.y * 10.0f) / 10.0f;
	}

	// 【修正】IF文のブレースを外に出しました！
	// 索敵中（SEARCH）または戦闘中（COMBAT）のときは、移動方向に関係なく常にプレイヤーを滑らかに向かせる
	if (state_ == STATE::COMBAT || state_ == STATE::SEARCH)
	{
		Player* player = scnMng_.GetPlayer();
		if (player != nullptr)
		{
			VECTOR playerPos = player->GetTransform().pos;
			VECTOR toPlayer = VSub(playerPos, transform_.pos);
			toPlayer.y = 0.0f; // 水平方向のベクトルにする

			if (VSize(toPlayer) > 0.5f)
			{
				// プレイヤーの方向を向く回転
				Quaternion targetLook = Quaternion::LookRotation(toPlayer);
				// モデル固有の初期回転を合成
				Quaternion targetRot = Quaternion::Mult(targetLook, transform_.quaRotLocal);

				Quaternion flipRot = Quaternion::AngleAxis(DX_PI_F, VGet(0, 1, 0));
				targetRot = Quaternion::Mult(targetRot, flipRot);

				// Slerpで滑らかに旋回
				float rotateSpeed = 0.1f;
				transform_.quaRot = Quaternion::Slerp(transform_.quaRot, targetRot, rotateSpeed);

				// デバッグ用にオイラー角のYも更新して画面に同期させる
				transform_.rot = transform_.quaRot.ToEuler();

				// 最終的な行列を確定させる
				transform_.Update();
			}
		}
	}

	// 【修正】移動範囲外のチェックのみを独立させました
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

	// 【修正】InitPost() で stateChanges_ に登録した関数を安全に呼び出す
	int stateKey = static_cast<int>(state_);
	if (stateChanges_.count(stateKey) > 0)
	{
		stateChanges_[stateKey]();
	}

	// マップに登録していない状態の個別処理
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
	movePow_ = AsoUtility::VECTOR_ZERO;
}

// --- 索敵状態の更新 ---
void EnemyMT::UpdateSearch(void)
{
	if (player_ == nullptr) return;

	// プレイヤーとの距離を計算
	float dist = VSize(VSub(player_->GetTransform().pos, transform_.pos));

	// 索敵半径の中に入ったら戦闘状態（移動開始）に遷移
	if (dist <= searchRadius_)
	{
		ChangeState(STATE::COMBAT);
	}
	else
	{
		// 半径外なら移動しない
		moveDir_ = AsoUtility::VECTOR_ZERO;
		movePow_ = AsoUtility::VECTOR_ZERO;
	}
}

// --- 戦闘状態への遷移 ---
void EnemyMT::ChangeStateCombat(void)
{
	stateUpdate_ = std::bind(&EnemyMT::UpdateCombat, this);
	directionTimer_ = 0.0f; // 遷移直後の最初のフレームで即座に方向を選択させる

	if (animationController_ != nullptr)
	{
		animationController_->Play(static_cast<int>(ANIM_TYPE::IDLE), true);
	}
}

// --- 戦闘状態の更新 ---
void EnemyMT::UpdateCombat(void)
{
	if (player_ == nullptr) return;

	// 1. 向きとは関係なく、定期的に前後左右を選んで移動する
	directionTimer_ -= scnMng_.GetDeltaTime();
	if (directionTimer_ <= 0.0f)
	{
		// 0:前, 1:後, 2:左, 3:右 をランダムで選択
		int dirType = rand() % 4;
		VECTOR localDir = AsoUtility::VECTOR_ZERO;

		switch (dirType)
		{
		case 0: localDir = VGet(0, 0, 1);  break; // 前進
		case 1: localDir = VGet(0, 0, -1); break; // 後退
		case 2: localDir = VGet(-1, 0, 0); break; // 左スライド
		case 3: localDir = VGet(1, 0, 0);  break; // 右スライド
		}

		// プレイヤーを向いている自分の回転を基準に、ローカル方向をワールド方向に変換（平行移動用）
		moveDir_ = transform_.quaRot.PosAxis(localDir);
		moveDir_ = AsoUtility::VNormalize(moveDir_); // ベクトルの長さを1に正規化

		// 次に移動方向を変えるまでの時間（1.0秒 〜 2.5秒 の間でランダム）
		directionTimer_ = 1.0f + (rand() % 150) / 100.0f;
	}

	// 移動量に速度を適用
	movePow_ = VScale(moveDir_, COMBAT_SPEED);

	// 2. プレイヤーが索敵半径から完全に離れたら索敵（SEARCH）に戻る
	float dist = VSize(VSub(player_->GetTransform().pos, transform_.pos));
	if (dist > searchRadius_ * 1.2f) // チャタリング防止バッファ
	{
		ChangeState(STATE::SEARCH);
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

void EnemyMT::TurnToPlayer(void)
{
	Player* player = scnMng_.GetPlayer();
	if (player == nullptr) return;

	VECTOR playerPos = player->GetTransform().pos;
	VECTOR toPlayer = VSub(playerPos, transform_.pos);

	// atan2f を使い、XZ平面上でのプレイヤーへの角度（ラジアン）を直接計算
	// DxLibの標準（Z前、X右）では、通常 (x, z) の順で正確な角度が求まります
	float targetAngleY = atan2f(toPlayer.x, toPlayer.z);

	// 【重要】直接オイラー角の Y 回転に代入する
	transform_.rot.y = targetAngleY;

	// 念のため、オイラー角からクォータニオン側にも同期させておく
	transform_.quaRot = Quaternion::Euler(transform_.rot);
}