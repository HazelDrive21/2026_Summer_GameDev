#pragma once
#include "SceneBase.h"
#include "../Object/Common/Transform.h"

class SkyDome;

class ClearScene : public SceneBase
{
public:
	ClearScene(void);
	~ClearScene(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

private:
	Transform spaceDomeTran_;
	SkyDome* skyDome_; // 背景用にスカイドームを流用
};