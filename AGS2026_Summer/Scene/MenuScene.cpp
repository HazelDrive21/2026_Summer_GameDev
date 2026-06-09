#include <DxLib.h>
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Audio/AudioManager.h"
#include "MenuScene.h"

MenuScene::MenuScene(void)
{
}

MenuScene::~MenuScene(void)
{
}

void MenuScene::Init(void)
{
	AudioManager::GetInstance()->LoadSceneSound(LoadScene::MENU);
	AudioManager::GetInstance()->PlayBGM(SoundID::BGM_MENU);
}

void MenuScene::Update(void)
{
    // ステージ選択画面へ進む
    if (InputManager::GetInstance().IsActionTrgDown(InputManager::ACTION::DECIDE))
    {
		AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
        SceneManager::GetInstance().PushScene(SceneManager::SCENE_ID::STAGE_SELECT);
    }

    // 武装選択画面へ進む
    if (InputManager::GetInstance().IsActionTrgDown(InputManager::ACTION::SUB_FUNC))
    {
		AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
        SceneManager::GetInstance().PushScene(SceneManager::SCENE_ID::WEAPON_SELECT);
    }

    // タイトルへ
    if (InputManager::GetInstance().IsActionTrgDown(InputManager::ACTION::CANCEL))
    {
        AudioManager::GetInstance()->PlaySE(SoundID::SE_CANCEL);
		AudioManager::GetInstance()->StopBGM();
		AudioManager::GetInstance()->DeleteSceneSound(LoadScene::MENU);
        
        SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
	}
}

void MenuScene::Draw(void)
{
    DrawString(10, 10, "=== MENU SCENE ===", GetColor(255, 255, 255));
    DrawString(10, 40, "Press [DECIDE] : Open Stage Select", GetColor(255, 255, 255));
    DrawString(10, 60, "Press [SUB_FUNC] : Open Weapon Select", GetColor(255, 255, 255));
}