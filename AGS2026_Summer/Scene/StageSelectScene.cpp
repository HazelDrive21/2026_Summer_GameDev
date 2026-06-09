#include <DxLib.h>
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Audio/AudioManager.h"
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
    InputManager& ins = InputManager::GetInstance();

	// 決定アクション：ゲームシーンへ進む
    if (ins.IsActionTrgDown(InputManager::ACTION::DECIDE))
    {
		AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
		AudioManager::GetInstance()->StopBGM();
		AudioManager::GetInstance()->DeleteSceneSound(LoadScene::MENU);
        SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::GAME);
    }

    // キャンセルアクション：メニュー画面に戻る
    if (ins.IsActionTrgDown(InputManager::ACTION::CANCEL))
    {
		AudioManager::GetInstance()->PlaySE(SoundID::SE_CANCEL);
        SceneManager::GetInstance().PopScene();
    }
}

void StageSelectScene::Draw(void)
{
    DrawString(10, 10, "=== STAGE SELECT SCENE ===", GetColor(255, 255, 255));
    DrawString(10, 40, "Press [DECIDE]     : Start Game", GetColor(255, 255, 255));
    DrawString(10, 60, "Press [CANCEL] : Return to Menu", GetColor(255, 255, 255));
}