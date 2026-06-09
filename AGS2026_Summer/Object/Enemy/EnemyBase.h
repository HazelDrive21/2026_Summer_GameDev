#pragma once
#include <DxLib.h>
#include <functional>
#include <map>
#include "../../Utility/AsoUtility.h"
#include "../CharactorBase.h"
#include "../../Object/Weapon/WeaponBase.h"

class Player;

class EnemyBase : public CharactorBase
{
public:

	// 種別
	enum class TYPE
	{
		MT1,
		MISSILE_MT,
		MAX,
	};

	// エネミーデータ
	struct EnemyData
	{
		int id;
		EnemyBase::TYPE type;
		int hp;
		VECTOR defaultPos;
		float movableRange;
		float searchRadius;
	};

	// コンストラクタ
	EnemyBase(const EnemyBase::EnemyData& data);

	// デストラクタ
	virtual ~EnemyBase(void) override;

	virtual void Update(void) override;

	// 描画
	virtual void Draw(void) override;

	VECTOR GetVelocity(void) const { return velocity_; }

	// 弾との当たり判定チェック＆被弾処理（当たったら true を返す）
	bool CheckHitBullet(const VECTOR& bulletPrevPos, const VECTOR& bulletPos, float bulletRadius, int damage);

	void ApplyDamage(int damage);

	// 現在のHPを取得するゲッター（デバッグ等用）
	int GetHp(void) const { return hp_; }

	// 死亡しているかどうか（HPが0以下なら死亡）
	bool IsDead(void) const { return hp_ <= 0; }

	// 現在の3D座標を取得するゲッター
	VECTOR GetPos(void) const { return transform_.pos; }

protected:

	WeaponBase* weapon_;

	VECTOR localMuzzlePos_;

	VECTOR velocity_ = AsoUtility::VECTOR_ZERO; // 現在の速度ベクトル
	VECTOR prevPos_ = AsoUtility::VECTOR_ZERO; // 1フレーム前の座標

	// 初期位置
	const VECTOR defaultPos_;

	// 移動可能範囲
	float movableRange_;

	// 探索範囲
	float searchRadius_;

	// 状態管理
	int stateBase_;

	// 状態管理(状態遷移時初期処理)
	std::map<int, std::function<void(void)>> stateChanges_;

	// 状態管理(更新ステップ)
	std::function<void(void)> stateUpdate_;

	// 種別
	TYPE type_;

	// HP
	int hp_;

	// リソースロード
	void InitLoad(void) override {}
	// 大きさ、回転、座標の初期化
	void InitTransform(void) override {}
	// 衝突判定の初期化
	void InitCollider(void) override {}
	// アニメーションの初期化
	void InitAnimation(void) override {}
	// 初期化後の個別処理
	void InitPost(void) override {}

	// 状態遷移
	void ChangeState(int state);

	// 更新系
	virtual void UpdateProcessPost(void) override {}
	// 移動可能範囲判定
	bool InMovableRange(void) const;

	void DrawDebugAxes(void) const;

};