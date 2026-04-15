#pragma once
#include "SceneBase.h"
class Stage;
class SkyDome;
class Player;

class GameScene : public SceneBase
{

public:
	
	// コンストラクタ
	GameScene(void);

	// デストラクタ
	~GameScene(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

private:

	// ステージ
	Stage* stage_;

	// スカイドーム
	SkyDome* skyDome_;

	// プレイヤー
	Player* player_;

};
