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
	// リソースロード
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
	void UpdateProcess(void) override;
	void UpdateProcessPost(void) override;

	VECTOR debugCurForward_;
	VECTOR debugTarForward_;
private:
	// モデルの大きさ
	static constexpr float SCALE = 0.5f;
	// モデルのローカル回転
	static constexpr VECTOR ROT = { 0.0f, 180.0f * DX_PI_F / 180.0f, 0.0f };
	// 衝突判定用線分開始
	static constexpr VECTOR COL_LINE_START_LOCAL_POS = { 0.0f, 120.0f, 0.0f };
	// 衝突判定用線分終了
	static constexpr VECTOR COL_LINE_END_LOCAL_POS = { 0.0f, -10.0f, 0.0f };
	// 衝突判定用カプセル上部球体
	static constexpr VECTOR COL_CAPSULE_TOP_LOCAL_POS = { 0.0f, 140.0f, 0.0f };
	// 衝突判定用カプセル下部球体
	static constexpr VECTOR COL_CAPSULE_DOWN_LOCAL_POS = { 0.0f, 50.0f, 0.0f };
	// 衝突判定用カプセル球体半径
	static constexpr float COL_CAPSULE_RADIUS = 40.0f;

	// 状態
	STATE state_ = STATE::NONE;

	static constexpr float SEARCH_RANGE = 600.0f;     // プレイヤーを発見する距離
	static constexpr float ROT_SPEED = 0.05f;         // プレイヤーへの旋回速度 (Lerp係数)
	static constexpr float COMBAT_SPEED = 10.0f;       // 戦闘時の移動速度

	// --- 戦闘用タイマー・フラグ ---
	float sideMoveSign_ = 1.0f;         // 1.0f = 右移動, -1.0f = 左移動
	float directionTimer_ = 0.0f;       // 左右切り返し用タイマー
	float directionChangeInterval_ = 2.0f; // 何秒ごとに左右を切り替えるか

	float shotTimer_ = 0.0f;            // 射撃間隔タイマー
	static constexpr float SHOT_INTERVAL = 1.5f; // 1.5秒に1回発射

	Player* player_ = nullptr;

	// 更新ステップ
	float step_;
	// 状態遷移
	void ChangeState(STATE state);
	void ChangeStateNone(void);
	void ChangeStateThink(void);
	void ChangeStateIdle(void);
	void ChangeStateWander(void);
	void ChangeStateSearch(void);
	void ChangeStateCombat(void);
	void ChangeStateEnd(void);

	// 更新系
	void UpdateNone(void);
	void UpdateThink(void);
	void UpdateIdle(void);
	void UpdateWander(void);
	void UpdateSearch(void);
	void UpdateCombat(void);
	void UpdateEnd(void);

	void RotateToPlayer(const VECTOR& toPlayerDimXZ);

	void TurnToPlayer(void);

	
};