#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Manager/Camera.h"
#include "../Object/SkyDome.h"
#include "GameOverScene.h"

GameOverScene::GameOverScene(void)
{
	skyDome_ = nullptr;
}

GameOverScene::~GameOverScene(void)
{
	delete skyDome_;
}

void GameOverScene::Init(void)
{
	spaceDomeTran_.pos = AsoUtility::VECTOR_ZERO;
	skyDome_ = new SkyDome(spaceDomeTran_);
	skyDome_->Init();

	// 定点カメラモード
	SceneManager::GetInstance().GetCamera()->ChangeMode(Camera::MODE::FIXED_POINT);
}

void GameOverScene::Update(void)
{
	skyDome_->Update();

	InputManager& ins = InputManager::GetInstance();
	// 決定アクション：タイトルに戻る
	if (ins.IsActionTrgDown(InputManager::ACTION::DECIDE))
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
	}
}

void GameOverScene::Draw(void)
{
	skyDome_->Draw();

	int centerX = Application::SCREEN_SIZE_X / 2;
	int centerY = Application::SCREEN_SIZE_Y / 2;

	// AC風の警告赤色（アラートカラー）
	unsigned int redColor = GetColor(255, 60, 60);

	DrawString(centerX - 70, centerY - 20, "MISSION FAILED", redColor);
	DrawString(centerX - 120, centerY + 40, "決定でタイトルに戻ります", GetColor(255, 255, 255));
}