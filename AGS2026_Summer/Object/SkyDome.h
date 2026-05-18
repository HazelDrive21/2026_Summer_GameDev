#pragma once
#include "ActorBase.h"

class SkyDome : public ActorBase
{
public:

	// 状態
	enum class STATE
	{
		NONE,
		STAY,
		FOLLOW,
	};

	// コンストラクタ
	SkyDome(const Transform& followTransform_);
	// デストラクタ
	~SkyDome(void) override;
	// 更新
	void Update(void) override;

	// 描画
	void Draw(void) override;

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

private:

	// 追従対象のTransform
	const Transform& followTransform_;

	// 状態
	STATE state_;

	// 大きさ
	static constexpr float SCALE = 100.0f;
	static constexpr VECTOR SCALES = { SCALE, SCALE, SCALE };

	// ローカル回転
	static constexpr VECTOR DEFAULT_ROT_LOCAL = { 0.0f,180.0f * DX_PI_F / 180.0f, 0.0f };

	// 状態遷移
	void ChangeState(STATE state);
	void ChangeStateNone(void);
	void ChangeStateStay(void);
	void ChangeStateFollow(void);

	// 更新
	void UpdateNone(void);
	void UpdateStay(void);
	void UpdateFollow(void);
};

