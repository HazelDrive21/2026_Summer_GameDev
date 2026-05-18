#pragma once
#include "ActorBase.h"

class AnimationController;
class Stage;
class Player;

class CharactorBase : public ActorBase
{

public:

	// 衝突判定種別
	enum class COLLIDER_TYPE
	{
		LINE,
		CAPSULE,
		MAX,
	};

	// コンストラクタ
	CharactorBase(void);
	// デストラクタ
	virtual ~CharactorBase(void) override;

	// 更新
	virtual void Update(void) override;

	// 描画
	virtual void Draw(void) override;

	// 丸影描画
	void DrawShadow(void);

	// 解放
	virtual void Release(void) override;

protected:

	// 最大落下速度
	static constexpr float MAX_FALL_SPEED = -10.0f;
	// 衝突時の押し戻し試行回数
	static constexpr int CNT_TRY_COLLISION = 20;
	// 衝突時の押し戻し量
	static constexpr float COLLISION_BACK_DIS = 1.0f;

	static constexpr float DEFAULT_GRAVITY_POW = 10.0f;


	// アニメーション
	AnimationController* animationController_;

	// 移動方向
	VECTOR moveDir_;
	// 移動スピード
	float moveSpeed_;
	// 移動量
	VECTOR movePow_;

	// 移動前の座標
	VECTOR prevPos_;

	// ジャンプ量
	VECTOR jumpPow_;

	// ジャンプ判定
	bool isJump_;

	// 丸影画像
	int imgShadow_;

	// ジャンプの入力受付時間
	float stepJump_;

	// リソースロード
	virtual void InitLoad(void) override;



	// 更新系
	virtual void UpdateProcess(void) = 0;
	virtual void UpdateProcessPost(void) = 0;

	// 移動方向に応じた遅延回転
	void DelayRotate(void);

	// 重力計算
	void CalcGravityPow(void);

	// 衝突判定
	virtual void CollisionReserve(void) {}
	virtual void Collision(void);
	virtual void CollisionGravity(void);
	virtual void CollisionCapsule(void);

};