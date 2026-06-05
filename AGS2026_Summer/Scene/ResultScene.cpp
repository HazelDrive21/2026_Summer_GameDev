#include <DxLib.h>
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "ResultScene.h"

ResultScene::ResultScene(void)
{
}

ResultScene::~ResultScene(void)
{
}

void ResultScene::Init(void)
{
}

void ResultScene::Update(void)
{
    InputManager& ins = InputManager::GetInstance();
    if (ins.IsActionTrgDown(InputManager::ACTION::DECIDE))
    {
        SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::MENU);
    }
}

void ResultScene::Draw(void)
{
    DrawString(10, 10, "=== RESULT SCENE ===", GetColor(255, 255, 255));
    DrawString(10, 40, "Press [SPACE] : Return to Menu", GetColor(255, 255, 255));
}