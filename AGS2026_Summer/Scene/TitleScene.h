#pragma once
#include "SceneBase.h"
#include "../Object/Common/Transform.h"
class SceneManager;
class SkyDome;
class AnimationController;

class TitleScene : public SceneBase
{

public:

	// コンストラクタ
	TitleScene(void);

	// デストラクタ
	~TitleScene(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

private:

	enum class MENU {
		START,
		MANUAL,
		EXIT,
		MAX
	};

	enum class CONFIRM {
		YES,
		NO,
		MAX
	};

	MENU cursor_ = MENU::START; // 現在のカーソル位置
	bool isConfirmExit_ = false; // 確認ダイアログ表示中フラグ
	CONFIRM confirmCursor_ = CONFIRM::NO; // 確認ダイアログのカーソル位置

	// 画像
	int imgTitle_;
	int imgPush_;

};
