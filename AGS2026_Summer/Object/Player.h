#pragma once
#include <map>
#include <DxLib.h>
#include "CharactorBase.h"
#include "../Object/Wepon/WeaponFirearm.h"
#include "../Object/Wepon/Bullet.h"


class AnimationController;
class FCS;
class EnemyManager;
class WeaponBase;

class Player : public CharactorBase
{

public:

	// スピード
	static constexpr float SPEED_MOVE = 10.0f;
	static constexpr float SPEED_RUN = 20.0f;

	// 回転完了までの時間
	static constexpr float TIME_ROT = 0.5f;

	// ジャンプ力
	static constexpr float POW_JUMP = 15.0f;

	// ジャンプ受付時間
	static constexpr float TIME_JUMP_IN = 0.5f;

	static constexpr float BOOSTER_POW = 1.5f;       // 1フレームあたりの上昇加速度
	static constexpr float MAX_ASCENT_SPEED = 10.0f;  // 上昇速度の上限

	// 状態
	enum class STATE
	{
		NONE,
		PLAY,
		STOP,
		LANDING,
		WARP_RESERVE,
		WARP_MOVE,
		DEAD,
		VICTORY,
		END
	};

	// アニメーション種別
	enum class ANIM_TYPE
	{
		IDLE,
		RUN,
		FAST_RUN,
		JUMP,
		WARP_PAUSE,
		FLY,
		FALLING,
		LANDING,
		VICTORY
	};

	// コンストラクタ
	Player(void);

	// デストラクタ
	~Player(void);

	void Draw(void) override;
	void Draw2D(void);


	STATE GetState() const { return state_; }

	// 現在の速度（speed_）を取得
	float GetSpeed(void) const { return speed_; }

	// 現在の移動方向（moveDir_）を取得
	VECTOR GetMoveDir(void) const { return moveDir_; }

	void SetEnemyManager(const EnemyManager* enemyMng) { enemyMng_ = enemyMng; }

	/*void EquipWeapon(EquipSlot slot, WeaponBase* newWeapon)
	{
		if (weapons_[static_cast<int>(slot)] != nullptr) {
			delete weapons_[static_cast<int>(slot)];
		}
		weapons_[static_cast<int>(slot)] = newWeapon;
	}*/

protected:
	// リリースロード
	void InitLoad(void) override;
	// 大きさ、回転、座標の初期化
	void InitTransform(void) override;
	// 衝突判定の初期化
	void InitCollider(void) override;
	// アニメーションの初期化
	void InitAnimation(void) override;
	// 初期化後の個別処理
	void InitPost(void) override;

	// 更新系
	virtual void UpdateProcess(void) override;
	virtual void UpdateProcessPost(void) override;

private:

	FCS* fcs_;

	// 装備中の武器のポインタ（今回は右手スロットの仮変数として用意）
	WeaponBase* rightWeapon_ = nullptr;

	// 画面内に存在する、このプレイヤーが放った弾のリスト
	std::vector<Bullet*> activeBullets_;

	const EnemyManager* enemyMng_ = nullptr; // 敵管理クラスへの参照（ロックオンのため）

	// ダブルタップ判定用
	float dashTapTimer_ = 0.0f;    // 入力を受け付ける猶予時間
	int dashTapCount_ = 0;         // 押された回数
	static constexpr float DOUBLE_TAP_TIME = 0.25f; // 0.25秒以内に2回押せばダブルタップ

	// ボタンを押し続けている時間を計測
	float dashPressDuration_ = 0.0f;
	static constexpr float LONG_PRESS_THRESHOLD = 0.2f; // 0.2秒以上で長押し（上昇）と判定

	// 旋回スピードの基本値
	static constexpr float DEFAULT_TURN_SPEED = 0.01f;
	// 現在の旋回速度（パーツ補正後の最終値）
	float currentTurnSpeed_ = 0.0f;

	bool oldDashKey_ = false; // 前フレームの入力状態

	// 状態管理
	STATE state_;

	// 移動後の座標
	VECTOR movedPos_;

	// 回転
	Quaternion playerRotY_;
	Quaternion goalQuaRot_;
	float stepRotTime_;

	// ジャンプ量
	VECTOR jumpPow_;

	bool isDashKeyNew = false; // ダッシュキーが新たに押されたか

	bool isDashKeyPress_ = false; // 現在ダッシュボタンが押されているか
	bool hasMoveInput_ = false;   // 現在移動入力があるか

	bool isBoostAscent_ = false; // 上昇ブースト中かどうかのフラグ

	float rePressWindowTimer_ = 0.3f; // 入れ直し受付タイマー

	bool isDashingBefore_ = false; // 前フレームでダッシュ中だったか

	float stopTimer_ = 0.0f;       // 硬直用タイマー
	const float STOP_TIME = 0.5f;  // 硬直する時間（秒）

	float dashResidualTimer_ = 0.0f; // ダッシュの残響（余韻）タイマー
	const float DASH_RESIDUAL_TIME = 0.4f; // 余韻をどのくらい残すか（秒）

	float landingTimer_ = 0.0f;
	const float LANDING_TIME = 0.8f; // 硬直時間（秒）

	float speed_;
	float stepSpeed_;


	// 衝突チェック
	VECTOR gravHitPosDown_;
	VECTOR gravHitPosUp_;

	// 状態遷移
	void ChangeState(STATE state);
	void ChangeStateNone(void);
	void ChangeStatePlay(void);
	void ChangeStateStop(void);
	void ChangeStateLanding(void);

	// 更新ステップ
	void UpdateNone(void);
	void UpdatePlay(void);
	void UpdateStop(void);
	void UpdateLanding(void);

	// 操作
	void ProcessMove(void);

	void ProcessJump(void);

	// 回転
	void SetGoalRotate(double rotRad);
	void Rotate(void);

	// 衝突判定
	void CollisionReserve(void) override;
	void Collision(void)override;
	void CollisionGravity(void)override;

	// 移動量の計算
	void CalcGravityPow(void);

	// 着地モーション終了
	bool IsEndLanding(void);

	// 衝突判定用線分開始
	static constexpr VECTOR COL_LINE_START_LOCAL_POS = { 0.0f, 140.0f, 0.0f };
	// 衝突判定用線分終了
	static constexpr VECTOR COL_LINE_END_LOCAL_POS = { 0.0f, -10.0f, 0.0f };

	// 衝突判定用線分開始(ジャンプ時)
	static constexpr VECTOR COL_LINE_JUMP_START_LOCAL_POS = { 0.0f, 130.0f, 0.0f };
	// 衝突判定用線分終了(ジャンプ時)
	static constexpr VECTOR COL_LINE_JUMP_END_LOCAL_POS = { 0.0f, 50.0f, 0.0f };

	// 衝突判定用カプセル上部球体
	static constexpr VECTOR COL_CAPSULE_TOP_LOCAL_POS = { 0.0f, 140.0f, 0.0f };
	// 衝突判定用カプセル下部球体
	static constexpr VECTOR COL_CAPSULE_DOWN_LOCAL_POS = { 0.0f, 30.0f, 0.0f };
	// 衝突判定用カプセル球体半径
	static constexpr float COL_CAPSULE_RADIUS = 40.0f;

	// 衝突判定用カプセル上部球体(ジャンプ時)
	static constexpr VECTOR COL_CAPSULE_TOP_JUMP_LOCAL_POS = { 0.0f, 160.0f, 0.0f };
	// 衝突判定用カプセル下部球体(ジャンプ時)
	static constexpr VECTOR COL_CAPSULE_DOWN_JUMP_LOCAL_POS = { 0.0f, 80.0f, 0.0f };

};
