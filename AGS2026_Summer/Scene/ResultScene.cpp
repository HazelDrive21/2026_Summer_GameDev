#include <DxLib.h>
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Audio/AudioManager.h"
#include "ResultScene.h"

ResultScene::ResultScene(void)
{
}

ResultScene::~ResultScene(void)
{
}

void ResultScene::Init(void)
{
	AudioManager::GetInstance()->StopBGM();
	AudioManager::GetInstance()->DeleteSceneSound(LoadScene::GAME);

    AudioManager::GetInstance()->LoadSceneSound(LoadScene::RESULT);
	AudioManager::GetInstance()->PlayBGM(SoundID::BGM_RESULT);
}

void ResultScene::Update(void)
{
    InputManager& ins = InputManager::GetInstance();
    if (ins.IsActionTrgDown(InputManager::ACTION::DECIDE))
    {
		AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
		AudioManager::GetInstance()->StopBGM();
		AudioManager::GetInstance()->DeleteSceneSound(LoadScene::RESULT);
        SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::MENU);
    }
}

void ResultScene::Draw(void)
{
    DrawString(10, 10, "=== RESULT SCENE ===", GetColor(255, 255, 255));
    DrawString(10, 40, "Press [SPACE] : Return to Menu", GetColor(255, 255, 255));
}