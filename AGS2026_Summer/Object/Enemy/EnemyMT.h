#pragma once
#include <DxLib.h>
#include "EnemyBase.h"

class Player;

class EnemyMT : public EnemyBase
{
public:
	// アニメーション種別
	enum class ANIM_TYPE
	{
		IDLE = 8,
		WALK = 13,
	};

	// 状態
	enum class STATE
	{
		NONE,
		THINK,
		IDLE,
		WANDER,
		SEARCH,
		COMBAT,
		END
	};

	// コンストラクタ
	EnemyMT(const EnemyBase::EnemyData& data);
	// デストラクタ
	~EnemyMT(void) override;

	void Draw(void) override;

protected:
	// 各種初期化・更新
	void InitLoad(void) override;
	void InitTransform(void) override;
	void InitCollider(void) override;
	void InitAnimation(void) override;
	void InitPost(void) override;
	void UpdateProcess(void) override;
	void UpdateProcessPost(void) override;

private:
	// モデルの大きさ
	static constexpr float SCALE = 0.5f;
	// モデルのローカル回転
	static constexpr VECTOR ROT = { 0.0f, 180.0f * DX_PI_F / 180.0f, 0.0f };

	// 衝突判定用パラメータ
	static constexpr VECTOR COL_LINE_START_LOCAL_POS = { 0.0f, 120.0f, 0.0f };
	static constexpr VECTOR COL_LINE_END_LOCAL_POS = { 0.0f, -10.0f, 0.0f };
	static constexpr VECTOR COL_CAPSULE_TOP_LOCAL_POS = { 0.0f, 140.0f, 0.0f };
	static constexpr VECTOR COL_CAPSULE_DOWN_LOCAL_POS = { 0.0f, 50.0f, 0.0f };
	static constexpr float  COL_CAPSULE_RADIUS = 40.0f;

	// 状態管理
	STATE state_ = STATE::NONE;

	// AI・移動・戦闘パラメータ
	static constexpr float ROT_SPEED = 1.0f;          // プレイヤーへの旋回速度 (Slerp係数)
	static constexpr float COMBAT_SPEED = 10.0f;       // 戦闘時の移動速度

	// --- 戦闘用タイマー・フラグ ---
	float sideMoveSign_ = 1.0f;            // 1.0f = 右移動, -1.0f = 左移動
	float directionTimer_ = 0.0f;          // 左右切り返し用タイマー

	float shotTimer_ = 0.0f;               // 射撃間隔タイマー
	static constexpr float SHOT_INTERVAL = 1.5f; // 1.5秒に1回発射

	// ★追加：バースト射撃用パラメータ
	int burstCount_ = 0;                   // 現在何発まで撃ったか
	float burstDelayTimer_ = 0.0f;         // 弾と弾の間のディレイ用タイマー

	static constexpr int BURST_SHOT_NUM = 3;      // 1回につき何連射するか（3点バースト）
	static constexpr float BURST_DELAY = 0.1f;    // 連射時の弾間隔（0.1秒間隔でパパパンと撃つ）

	Player* player_ = nullptr;
	float step_ = 0.0f; // 更新ステップ用タイマー

	// 状態遷移・各種初期化関数
	void ChangeState(STATE state);
	void ChangeStateNone(void);
	void ChangeStateThink(void);
	void ChangeStateIdle(void);
	void ChangeStateWander(void);
	void ChangeStateSearch(void);
	void ChangeStateCombat(void);
	void ChangeStateEnd(void);

	// 状態別更新関数
	void UpdateNone(void);
	void UpdateThink(void);
	void UpdateIdle(void);
	void UpdateWander(void);
	void UpdateSearch(void);
	void UpdateCombat(void);
	void UpdateEnd(void);

	// プレイヤーへの旋回処理ヘルパー
	void RotateToPlayer(void);
};