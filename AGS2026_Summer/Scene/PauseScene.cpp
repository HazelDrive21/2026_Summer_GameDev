#include <DxLib.h>
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Audio/AudioManager.h"
#include "../Application.h"
#include "PauseScene.h"

PauseScene::PauseScene(void)
{
}

PauseScene::~PauseScene(void)
{
}

void PauseScene::Init(void)
{
}

void PauseScene::Update(void)
{
	InputManager& ins = InputManager::GetInstance();

	// 決定アクション：ポーズ解除
	if (ins.IsActionTrgDown(InputManager::ACTION::PAUSE))
	{
		SceneManager::GetInstance().PopScene();
	}

	if (ins.IsActionTrgDown(InputManager::ACTION::CANCEL))
	{
		// SEの再生と、ゲーム中に鳴っていたBGMの停止
		AudioManager::GetInstance()->PlaySE(SoundID::SE_CANCEL);
		AudioManager::GetInstance()->StopBGM();

		// 必要であれば、ゲームシーン用のサウンドメモリの解放を入れる
		AudioManager::GetInstance()->DeleteSceneSound(LoadScene::GAME);

		// シーンをメニュー画面に切り替える
		// ※お使いの SceneManager の ID 定義（MENU や MENU_SCENE など）に合わせてください
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::MENU);
	}
}

void PauseScene::Draw(void)
{
	// 画面全体に薄い黒のフィルターをかけるとポーズ画面らしくなります
	DrawBox(0, 0, Application::SCREEN_SIZE_X, Application::SCREEN_SIZE_Y, GetColor(0, 0, 0), TRUE);

	DrawString(10, 10, "=== PAUSE SCENE ===", GetColor(255, 128, 0));
	DrawString(10, 40, "Press [PAUSE] : Resume Game", GetColor(255, 255, 255));
	DrawString(10, 70, "Press [CANCEL] : Return to Menu", GetColor(255, 255, 255));
}