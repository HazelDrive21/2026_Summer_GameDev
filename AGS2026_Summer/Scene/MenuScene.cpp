#include <DxLib.h>
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "MenuScene.h"

MenuScene::MenuScene(void)
{
}

MenuScene::~MenuScene(void)
{
}

void MenuScene::Init(void)
{
}

void MenuScene::Update(void)
{
    // SPACEキー：ステージ選択画面へ進む (直接遷移)
    if (InputManager::GetInstance().IsTrgDown(KEY_INPUT_SPACE))
    {
        SceneManager::GetInstance().PushScene(SceneManager::SCENE_ID::STAGE_SELECT);
    }

    // Zキー：武装選択画面へ行く (スタック型遷移で重ねる)
    if (InputManager::GetInstance().IsTrgDown(KEY_INPUT_Z))
    {
        SceneManager::GetInstance().PushScene(SceneManager::SCENE_ID::WEAPON_SELECT);
    }
}

void MenuScene::Draw(void)
{
    DrawString(10, 10, "=== MENU SCENE ===", GetColor(255, 255, 255));
    DrawString(10, 40, "Press [SPACE] : Go to Stage Select", GetColor(255, 255, 255));
    DrawString(10, 60, "Press [Z]     : Open Weapon Select (Sub Screen)", GetColor(255, 255, 255));
}