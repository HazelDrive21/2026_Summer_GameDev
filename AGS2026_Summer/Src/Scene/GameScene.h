#pragma once
#include "SceneBase.h"

class Stage;
class SkyDome;
class Player;
class EnemyManager;

class GameScene : public SceneBase
{

public:

	// コンストラクタ
	GameScene(void);

	// デストラクタ
	~GameScene(void) override;

	// 初期化
	void Init(void) override;

	// 更新
	void Update(void) override;

	// 描画
	void Draw(void) override;

	// 解放
	void Release(void) override;

private:

	// ステージ
	Stage* stage_;

	// スカイドーム
	SkyDome* skyDome_;

	// プレイヤー
	Player* player_;

	// エネミー情報
	EnemyManager* enemyManager_;

};
