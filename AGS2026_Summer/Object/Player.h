#pragma once
#include <map>
#include <DxLib.h>
#include "CharactorBase.h"
#include "../Object/Weapon/WeaponFirearm.h"
#include "../Object/Weapon/WeaponMissile.h"
#include "../Object/Weapon/Bullet.h"


class AnimationController;
class FCS;
class EnemyManager;
class WeaponBase;
class WeaponBlade;

class Player : public CharactorBase
{

public:

	// スピード
	static constexpr float SPEED_RUN = 25.0f;
	static constexpr float SPEED_DASH = 100.0f;          // ダッシュ時の最高速度（SPEED_RUNより速く）

	// 回転完了までの時間
	static constexpr float TURN_SPEED = 75.0f; // 1秒間にn度旋回する（パーツ性能に変えられるようにする）

	// ジャンプ力
	static constexpr float POW_JUMP = 50.0f;

	static constexpr float BOOSTER_POW = 2.0f;       // 1フレームあたりの上昇加速度
	static constexpr float MAX_ASCENT_SPEED = 20.0f;  // 上昇速度の上限


	static constexpr float TIME_DASH_RESIDUAL = 0.3f;   // ダッシュをやめてから急ブレーキがかかるまでの時間
	static constexpr float TIME_LANDING_STIFF = 0.6f;   // 高所着地硬直の時間
	static constexpr float LIMIT_LANDING_SPEED = -40.0f;// 硬直が発生する落下速度の閾値（これより速いとガタつく）
	static constexpr float DOUBLE_TAP_LIMIT_TIME = 0.25f;// ダブルタップと認識する猶予時間

	static constexpr float ACCEL_GROUND = 1.2f;       // 地上での加速力（値が大きいほど最高速にすぐ到達）
	static constexpr float FRICTION_GROUND = 0.95f;    // 地上での摩擦・減速係数（1.0未満。0.95だと長く滑り、0.8だとすぐ止まる）
	static constexpr float ACCEL_AIR = 0.6f;          // 空中での横移動加速力（地上より少し慣性を重くする）
	static constexpr float FRICTION_AIR = 0.5f;       // 空中での空気抵抗（空中の方がズサッと止まれない）
	static constexpr float MIN_TURN_ACCEL = 0.01f;

	static constexpr float AIR_SPEED_RATIO = 0.8f;    // 空中での速度制限（地上ブーストの80%に低下）

	static constexpr float MAX_EN = 3000.0f;             // ENの最大値
	static constexpr float EN_CONSUME_DASH = 250.0f;     // ダッシュ時の1秒あたりのEN消費量
	static constexpr float EN_CONSUME_ASCENT = 350.0f;   // 上昇時の1秒あたりのEN消費量
	static constexpr float EN_RECOVER = 120.0f;   // 1秒あたりのEN回復量

	static int s_rightArmEquipID;
	static int s_rightBackEquipID;
	static int s_leftArmEquipID;
	static int s_leftBackEquipID;
	
	// 状態
	enum class STATE
	{
		NONE,
		PLAY,
		STOP,
		LANDING_STIFF,
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

	enum class BOOST_MODE {
		NORMAL,    // 通常移動
		DASH,      // ダッシュ中
		BRAKE,     // 急ブレーキ中（残存慣性）
	};
	BOOST_MODE boostMode_ = BOOST_MODE::NORMAL;

	struct ExplosionEffect {
		VECTOR pos;
		float currentRadius;
		float maxRadius;
		int life;
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

	float GetEN(void) const { return en_; }
	float GetMaxEN(void) const { return MAX_EN; }

	void ConsumeEN(float amount) { en_ = (std::max)(0.0f, en_ - amount); }

	void SetEnemyManager(const EnemyManager* enemyMng) { enemyMng_ = enemyMng; }

	int GetHp(void) const { return hp_; }
	int GetMaxHp(void) const { return maxHp_; }

	// ダメージを受ける関数（外部や弾の衝突判定から呼ばれる）
	void ApplyDamage(int damage);

	bool CheckHitBullet(const VECTOR& bulletPrevPos, const VECTOR& bulletPos, float bulletRadius, int damage);

	WeaponBase* GetActiveWeapon(void) const;

	bool IsDead(void) const { return hp_ <= 0; }

	void SetWeaponL(WeaponBlade* weapon) { leftWeapon_ = weapon; }

	// アセンブル画面で選んだIDに基づいて武器を生成する関数
	void InitEquippedWeapons(void);

	// 現在の装備総重量を計算して返す関数
	int CalcTotalWeaponWeight(void) const;

	// 重量による速度補正を計算する関数
	float GetWeightSpeedMultiplier(void) const;

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

	// 装備中の武器のポインタ
	WeaponBase* rightWeapon_ = nullptr;
	WeaponBase* rightBackWeapon_ = nullptr;
	WeaponBlade* leftWeapon_ = nullptr;
	WeaponBase* leftBackWeapon_ = nullptr;

	EquipSlot activeWeaponSlot_ = EquipSlot::R_ARM;

	// 画面内に存在する、このプレイヤーが放った弾のリスト
	std::vector<Bullet*> activeBullets_;

	const EnemyManager* enemyMng_ = nullptr; // 敵管理クラスへの参照（ロックオンのため）

	// ダブルタップ判定用
	float dashTapTimer_ = 0.0f;    // 入力を受け付ける猶予時間
	int dashTapCount_ = 0;         // 押された回数
	static constexpr float DOUBLE_TAP_TIME = 0.25f; // 0.25秒以内に2回押せばダブルタップ

	static constexpr float LONG_PRESS_THRESHOLD = 0.2f; // 0.2秒以上で長押し（上昇）と判定

	// 旋回スピードの基本値
	static constexpr float DEFAULT_TURN_SPEED = 0.01f;
	// 現在の旋回速度（パーツ補正後の最終値）
	float currentTurnSpeed_ = 0.1f;

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

	VECTOR velocity_;

	bool isDashKeyNew = false; // ダッシュキーが新たに押されたか

	bool isDashKeyPress_ = false; // 現在ダッシュボタンが押されているか
	bool hasMoveInput_ = false;   // 現在移動入力があるか

	bool isBoostAscent_ = false; // 上昇ブースト中かどうかのフラグ

	float rePressWindowTimer_ = 0.3f; // 入れ直し受付タイマー

	bool isDashingBefore_ = false; // 前フレームでダッシュ中だったか

	float stopTimer_ = 0.0f;       // 硬直用タイマー
	const float STOP_TIME = 0.5f;  // 硬直する時間（秒）

	const float DASH_RESIDUAL_TIME = 0.4f; // 余韻をどのくらい残すか（秒）

	float landingTimer_ = 0.0f;
	const float LANDING_TIME = 0.8f; // 硬直時間（秒）

	float speed_;

	float debugCurrentSpeed_ = 0.0f;

	float en_ = 0.0f;               // 現在のEN量

	int hp_;
	int maxHp_;

	bool isCharging_ = false; // チャージング状態フラグ（trueの間はEN消費行動が不可）

	float antiMissileRange_ = 1000.0f;     // 迎撃対象にする距離（射程）
	int antiMissileReloadFrame_ = 1;    // 迎撃の間隔（nフレームに1回）
	int antiMissileTimer_ = 0;           // 迎撃リロードタイマー

	float footstepTimer_ = 0.0f;


	// 衝突チェック
	VECTOR gravHitPosDown_;
	VECTOR gravHitPosUp_;

	// 状態遷移
	void ChangeState(STATE state);
	void ChangeStateNone(void);
	void ChangeStatePlay(void);
	void ChangeStateStop(void);

	// 更新ステップ
	void UpdateNone(void);
	void UpdatePlay(void);
	void UpdateStop(void);
	void UpdateLandingStiff(void);

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
	//void CalcGravityPow(void);
	void UpdateMovementSound(float deltaTime);

	// 着地モーション終了
	bool IsEndLanding(void);

	void ProcessTurn(void);              // ★旋回入力を独立化
	void UpdateCommonMechanics(void);    // ★FCS、武器、弾丸の共通更新
	void UpdateAntiMissile();

	void UpdateEnergy(float deltaTime); // ★ENの消費・回復を一括管理する関数

	// 衝突判定用線分開始
	static constexpr VECTOR COL_LINE_START_LOCAL_POS = { 0.0f, 160.0f, 0.0f };
	// 衝突判定用線分終了
	static constexpr VECTOR COL_LINE_END_LOCAL_POS = { 0.0f, -10.0f, 0.0f };

	// 衝突判定用線分開始(ジャンプ時)
	static constexpr VECTOR COL_LINE_JUMP_START_LOCAL_POS = { 0.0f, 160.0f, 0.0f };
	// 衝突判定用線分終了(ジャンプ時)
	static constexpr VECTOR COL_LINE_JUMP_END_LOCAL_POS = { 0.0f, -10.0f, 0.0f };

	// 衝突判定用カプセル上部球体
	static constexpr VECTOR COL_CAPSULE_TOP_LOCAL_POS = { 0.0f, 160.0f, 0.0f };
	// 衝突判定用カプセル下部球体
	static constexpr VECTOR COL_CAPSULE_DOWN_LOCAL_POS = { 0.0f, 30.0f, 0.0f };
	// 衝突判定用カプセル球体半径
	static constexpr float COL_CAPSULE_RADIUS = 40.0f;

	// 衝突判定用カプセル上部球体(ジャンプ時)
	static constexpr VECTOR COL_CAPSULE_TOP_JUMP_LOCAL_POS = { 0.0f, 160.0f, 0.0f };
	// 衝突判定用カプセル下部球体(ジャンプ時)
	static constexpr VECTOR COL_CAPSULE_DOWN_JUMP_LOCAL_POS = { 0.0f, 30.0f, 0.0f };

	
	bool isBoosterOn_ = false;        // 現在ブースターが火を噴いているか（EN消費の判定用）

	float dashResidualTimer_ = 0.0f;  // ダッシュをやめた後の残存時間タイマー
	float landingStiffTimer_ = 0.0f;  // 着地硬直タイマー
	float dashButtonTapTimer_ = 0.0f; // ダブルタップ検知用タイマー
	int   dashButtonTapCount_ = 0;    // ダッシュボタンのタップ回数カウント
	float dashPressDuration_ = 0.0f;  // 長押し判定用の時間蓄積
	float airDashTime_ = 0.0f;        // 空中ダッシュの継続時間タイマー
	
	struct AntiMissileEffect {
		VECTOR start; // プレイヤーの座標
		VECTOR end;   // ミサイルの座標
		int life;     // 残り寿命（フレーム）
	};

	std::vector<AntiMissileEffect> antiMissileEffects_;
	std::vector<ExplosionEffect> explosionEffects_;
};
