#pragma once
#include <map>
#include <DxLib.h>
#include "ActorBase.h"
class AnimationController;
class Collider;
class Capsule;
class FCS;

class Player : public ActorBase
{

public:

	// スピード
	static constexpr float SPEED_MOVE = 10.0f;
	static constexpr float SPEED_RUN = 20.0f;

	// 回転完了までの時間
	static constexpr float TIME_ROT = 0.5f;

	// ジャンプ力
	static constexpr float POW_JUMP = 20.0f;

	// ジャンプ受付時間
	static constexpr float TIME_JUMP_IN = 0.5f;

	static constexpr float BOOSTER_POW = 1.0f;       // 1フレームあたりの上昇加速度
	static constexpr float MAX_ASCENT_SPEED = 10.0f;  // 上昇速度の上限

	// 状態
	enum class STATE
	{
		NONE,
		PLAY,
		STOP,
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
		VICTORY
	};

	// コンストラクタ
	Player(void);

	// デストラクタ
	~Player(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

	// 衝突判定に用いられるコライダ制御
	void AddCollider(Collider* collider);
	void ClearCollider(void);

	// 衝突用カプセルの取得
	const Capsule* GetCapsule(void) const;

	STATE GetState() const { return state_; }

	// 現在の速度（speed_）を取得
	float GetSpeed(void) const { return speed_; }

	// 現在の移動方向（moveDir_）を取得
	VECTOR GetMoveDir(void) const { return moveDir_; }

private:

	FCS* fcs_;

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

	// アニメーション
	AnimationController* animationController_;

	// 状態管理
	STATE state_;

	// 移動スピード
	float speed_;

	// 移動方向
	VECTOR moveDir_;

	// 移動量
	VECTOR movePow_;

	// 移動後の座標
	VECTOR movedPos_;

	// 回転
	Quaternion playerRotY_;
	Quaternion goalQuaRot_;
	float stepRotTime_;

	// ジャンプ量
	VECTOR jumpPow_;

	// ジャンプ判定
	bool isJump_;

	bool isDashKeyNew = false; // ダッシュキーが新たに押されたか

	bool isBoostAscent_ = false; // 上昇ブースト中かどうかのフラグ

	float rePressWindowTimer_ = 0.3f; // 入れ直し受付タイマー

	// ジャンプの入力受付時間
	float stepJump_;

	bool isDashingBefore_ = false; // 前フレームでダッシュ中だったか

	float stopTimer_ = 0.0f;       // 硬直用タイマー
	const float STOP_TIME = 0.5f;  // 硬直する時間（秒）

	float dashResidualTimer_ = 0.0f; // ダッシュの残響（余韻）タイマー
	const float DASH_RESIDUAL_TIME = 0.4f; // 余韻をどのくらい残すか（秒）

	// 衝突判定に用いられるコライダ
	std::vector<Collider*> colliders_;
	Capsule* capsule_;

	// 衝突チェック
	VECTOR gravHitPosDown_;
	VECTOR gravHitPosUp_;

	// 丸影
	int imgShadow_;

	void InitAnimation(void);

	// 状態遷移
	void ChangeState(STATE state);
	void ChangeStateNone(void);
	void ChangeStatePlay(void);
	void ChangeStateStop(void);

	// 更新ステップ
	void UpdateNone(void);
	void UpdatePlay(void);
	void UpdateStop(void);
	
	// 描画系
	void DrawShadow(void);

	// 操作
	void ProcessMove(void);

	void ProcessJump(void);

	// 回転
	void SetGoalRotate(double rotRad);
	void Rotate(void);

	// 衝突判定
	void Collision(void);
	void CollisionGravity(void);
	void CollisionCapsule(void);

	// 移動量の計算
	void CalcGravityPow(void);

	// 着地モーション終了
	bool IsEndLanding(void);

};
