#include <DxLib.h>
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "StageSelectScene.h"

StageSelectScene::StageSelectScene(void)
{
}

StageSelectScene::~StageSelectScene(void)
{
}

void StageSelectScene::Init(void)
{
}

void StageSelectScene::Update(void)
{
    // SPACEキー：ゲーム本編へ進む
    if (InputManager::GetInstance().IsTrgDown(KEY_INPUT_SPACE))
    {
        SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::GAME);
    }

    // BACKSPACEキー：メニュー画面に戻る
    if (InputManager::GetInstance().IsTrgDown(KEY_INPUT_BACK))
    {
        SceneManager::GetInstance().PopScene();
    }
}

void StageSelectScene::Draw(void)
{
    DrawString(10, 10, "=== STAGE SELECT SCENE ===", GetColor(255, 255, 255));
    DrawString(10, 40, "Press [SPACE]     : Start Game", GetColor(255, 255, 255));
    DrawString(10, 60, "Press [BACKSPACE] : Return to Menu", GetColor(255, 255, 255));
}