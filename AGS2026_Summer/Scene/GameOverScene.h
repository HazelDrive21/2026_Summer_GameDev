#pragma once
#include "SceneBase.h"
#include "../Object/Common/Transform.h"

class SkyDome;

class GameOverScene : public SceneBase
{
public:
	GameOverScene(void);
	~GameOverScene(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

private:
	Transform spaceDomeTran_;
	SkyDome* skyDome_; // îwåi
};