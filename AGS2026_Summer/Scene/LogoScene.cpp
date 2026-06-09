#include <DxLib.h>
#include "../Application.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Audio/AudioManager.h"
#include "../Manager/Camera.h"
#include "LogoScene.h"

LogoScene::LogoScene(void)
{
	isStopping_ = false;
	stopTimer_ = 0;
	movieHandle_ = -1;
}

LogoScene::~LogoScene(void)
{
	if (movieHandle_ != -1)
	{
		// ムービーの解放
		DeleteGraph(movieHandle_);
		movieHandle_ = -1;
	}
}

void LogoScene::Init(void)
{
	movieHandle_ = LoadGraph("Data/Movie/仮ムービー.mp4");

	if (movieHandle_ != -1)
	{
		// 2. 動画の再生を開始する（バックグラウンド再生モード）
		PlayMovieToGraph(movieHandle_);
	}
	else
	{
		// エラー対策：もし動画が読み込めなかったら即タイトルへ
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
	}
}

void LogoScene::Update(void)
{
	InputManager& ins = InputManager::GetInstance();

	if (!isStopping_)
	{
		bool isSkipPressed = ins.IsActionTrgDown(InputManager::ACTION::DECIDE);
		bool isMovieFinished = (GetMovieStateToGraph(movieHandle_) == 0);

		if (isSkipPressed || isMovieFinished)
		{
			if (isSkipPressed)
			{
				AudioManager::GetInstance()->PlaySE(SoundID::SE_OK); // 決定音
			}

			// 🔥 映像をその場のフレームでピタッと一時停止させる
			PauseMovieToGraph(movieHandle_);
			isStopping_ = true; // 停止モード（遷移待ち）に切り替え
		}
	}
	// 🔥 2. 映像が停止した後の処理（余韻タイム）
	else
	{
		stopTimer_++;

		// 指定したフレーム数（WAIT_TIME）だけ静止画を維持したら、タイトルへ
		if (stopTimer_ >= WAIT_TIME)
		{
			SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
		}
	}
}

void LogoScene::Draw(void)
{
	if (movieHandle_ != -1)
	{
		// 一時停止中であっても、その瞬間のフレームがそのまま描画され続けます
		DrawExtendGraph(0, 0, Application::SCREEN_SIZE_X, Application::SCREEN_SIZE_Y, movieHandle_, FALSE);
	}
}