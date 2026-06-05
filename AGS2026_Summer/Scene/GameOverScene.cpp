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
	if (InputManager::GetInstance().IsTrgDown(KEY_INPUT_P))
	{
		SceneManager::GetInstance().PushScene(SceneManager::SCENE_ID::PAUSE);
		return; // ポーズ画面に遷移するので、今フレームのこれ以降のゲーム処理はスキップ
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
	DrawString(centerX - 120, centerY + 40, "PUSH SPACE TO TITLE SCENE", GetColor(255, 255, 255));
}