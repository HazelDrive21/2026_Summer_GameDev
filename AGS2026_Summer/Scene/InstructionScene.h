#pragma once
#include "SceneBase.h"
#include "../Object/Common/Transform.h"

class SkyDome;

class InstructionScene : public SceneBase
{
public:
	InstructionScene(void);
	~InstructionScene(void);
	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

private:
	Transform spaceDomeTran_;
	SkyDome* skyDome_; // 背景用にスカイドームを流用

	int img_;
	int img2_;
	int img3_;

	int currentPage_;         // 現在のページ
	const int MAX_PAGES = 3;  // 最大ページ数
};