#pragma once
#include "SceneBase.h"

class LogoScene : public SceneBase
{
public:
	LogoScene(void);
	virtual ~LogoScene(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

private:
	int movieHandle_;    // ロゴムービーのハンドル
	bool isStopping_;   // 停止中（遷移待ち）フラグ
	int stopTimer_;     // 停止してからの時間を計るタイマー
	const int WAIT_TIME = 20; // 停止してから遷移するまでのフレーム数（20フレーム = 約0.3秒）
};