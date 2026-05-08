#pragma once
#include "../Object/Common/Transform.h"
#include "SceneBase.h"

class AnimationController;
class SkyDome;

class TitleScene : public SceneBase
{

public:

	// コンストラクタ
	TitleScene(void);

	// デストラクタ
	~TitleScene(void) override;

	// 初期化
	void Init(void) override;

	// 更新
	void Update(void) override;

	// 描画
	void Draw(void) override;

	// 解放
	void Release(void) override;

private:

	// タイトル画像のY座標
	static constexpr int IMG_TITLE_POS_Y = 250;

	// PushSpace画像のY座標
	static constexpr int IMG_PUSH_POS_Y = 500;

	// 小さい惑星の大きさ
	static constexpr float SCL_SMALL_PLANET = 0.7f;

	// 小さい惑星の回転
	static constexpr VECTOR ROT_SMALL_PLANET = { 90.0f * DX_PI_F / 180.0f, 0.0f, 0.0f };

	// 小さい惑星の座標
	static constexpr VECTOR POS_SMALL_PLANET = { -250.0f, -100.0f, -100.0f };

	// キャラクターの大きさ
	static constexpr float SCL_CHARACTOR = 0.4f;

	// キャラクターの回転
	static constexpr VECTOR ROT_CHARACTOR = { 0.0f, -90.0f * DX_PI_F / 180.0f, 0.0f };
	static constexpr VECTOR ROT_LOCAL_CHARACTOR = { 0.0f, 180.0f * DX_PI_F / 180.0f, 0.0f };

	// キャラクターの座標
	static constexpr VECTOR POS_CHARACTOR = { -250.0f, -32.0f, -105.0f };


	int imgTitle_;

	int imgPushSpace_;

	// 惑星
	Transform bigPlanet_;

	// 球体惑星
	Transform spherePlanet_;

	// キャラクター
	Transform charactor_;

	// アニメーション制御
	AnimationController* animationController_;

	// スカイドーム
	SkyDome* skyDome_;

	// スカイドーム用の空Transform
	Transform empty_;
};
