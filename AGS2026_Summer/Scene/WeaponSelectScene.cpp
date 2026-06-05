#include <DxLib.h>
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "WeaponSelectScene.h"

WeaponSelectScene::WeaponSelectScene(void)
{
}

WeaponSelectScene::~WeaponSelectScene(void)
{
}

void WeaponSelectScene::Init(void)
{
}

void WeaponSelectScene::Update(void)
{
	// BACKSPACEキー：メニュー画面に戻る（スタックからポップ）
	if (InputManager::GetInstance().IsTrgDown(KEY_INPUT_BACK))
	{
		SceneManager::GetInstance().PopScene();
	}
}

void WeaponSelectScene::Draw(void)
{
	DrawString(10, 10, "=== WEAPON SELECT SCENE (SUB SCREEN) ===", GetColor(255, 255, 255));
	DrawString(10, 40, "Press [BACKSPACE] : Return to Menu", GetColor(255, 255, 255));
}