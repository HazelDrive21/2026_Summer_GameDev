#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Manager/Camera.h"
#include "../Object/SkyDome.h"
#include "InstructionScene.h"

InstructionScene::InstructionScene(void)
{
	skyDome_ = nullptr;
	img_ = -1;
	img2_ = -1;
	img3_ = -1;
	currentPage_ = 1;
}

InstructionScene::~InstructionScene(void)
{
	delete skyDome_;
}

void InstructionScene::Init(void)
{
	// 背景の設定
	spaceDomeTran_.pos = AsoUtility::VECTOR_ZERO;
	skyDome_ = new SkyDome(spaceDomeTran_);
	skyDome_->Init();

	// 定点カメラモードにしておく
	SceneManager::GetInstance().GetCamera()->ChangeMode(Camera::MODE::FIXED_POINT);

	// 操作説明の画像
	img_ = resMng_.Load(ResourceManager::SRC::PAD).handleId_;
	img2_ = resMng_.Load(ResourceManager::SRC::P1).handleId_;
	img3_ = resMng_.Load(ResourceManager::SRC::P2).handleId_;
}

void InstructionScene::Update(void)
{
	skyDome_->Update();

	InputManager& ins = InputManager::GetInstance();

	// 右キー または SPACEキー で次のページへ
	if (ins.IsTrgDown(KEY_INPUT_RIGHT))
	{
		if (currentPage_ < MAX_PAGES)
		{
			currentPage_++;
		}
		else
		{
			// 最終ページで進もうとしたらタイトルに戻る
			SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
		}
	}

	// 左キー で前のページに戻る
	if (ins.IsTrgDown(KEY_INPUT_LEFT))
	{
		if (currentPage_ > 1)
		{
			currentPage_--;
		}
	}

	// SPACEが押されたら、どのページからでも即タイトルに戻る（親切設計）
	if (ins.IsTrgDown(KEY_INPUT_SPACE))
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
	}
}

void InstructionScene::Draw(void)
{
	skyDome_->Draw();
	int x = (Application::SCREEN_SIZE_X / 2);
	int y = Application::SCREEN_SIZE_Y / 2;
	unsigned int color = GetColor(255, 255, 255);

	if(currentPage_ == 1)
	{
		DrawRotaGraph(x, y, 0.68, 0.0, img_, true);
	}
	else if(currentPage_ == 2)
	{
		DrawRotaGraph(x, y, 0.68, 0.0, img2_, true);
	}
	else if(currentPage_ == 3)
	{
		DrawRotaGraph(x, y, 0.68, 0.0, img3_, true);
	}

	int bottomY = 700;
	unsigned int gray = GetColor(180, 180, 180);

	if (currentPage_ < MAX_PAGES)
	{
		DrawString(x-60, bottomY, " NEXT PAGE: PRESS RIGHT", gray);
	}
	else
	{
		DrawString(x-60, bottomY, " RETURN TO TITLE: PRESS SPACE", gray);
	}

	if (currentPage_ > 1)
	{
		DrawString(x-60 + 400, bottomY, " PREV PAGE: PRESS LEFT", gray);
	}

	DrawString(20, 20, "SPACE: RETURN TO TITLE", GetColor(100, 100, 100));
}