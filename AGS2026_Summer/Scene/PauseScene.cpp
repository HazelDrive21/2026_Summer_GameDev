#include <DxLib.h>
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
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
	// Pキー または BACKSPACEキー：ゲーム本編に戻る
	if (InputManager::GetInstance().IsTrgDown(KEY_INPUT_P) || InputManager::GetInstance().IsTrgDown(KEY_INPUT_BACK))
	{
		SceneManager::GetInstance().PopScene();
	}
}

void PauseScene::Draw(void)
{
	// 画面全体に薄い黒のフィルターをかけるとポーズ画面らしくなります
	DrawBox(0, 0, 1280, 720, GetColor(0, 0, 0), TRUE); // サイズは環境に合わせて調整してください

	DrawString(10, 10, "=== PAUSE SCENE ===", GetColor(255, 128, 0));
	DrawString(10, 40, "Press [P] or [BACKSPACE] : Resume Game", GetColor(255, 255, 255));
}