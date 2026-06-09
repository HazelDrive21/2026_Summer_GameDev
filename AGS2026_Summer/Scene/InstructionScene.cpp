#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Manager/Camera.h"
#include "../Audio/AudioManager.h"
#include "InstructionScene.h"

InstructionScene::InstructionScene(void)
{
	img_ = -1;
	img2_ = -1;
	img3_ = -1;
	currentPage_ = 1;
}

InstructionScene::~InstructionScene(void)
{
}

void InstructionScene::Init(void)
{

	// 定点カメラモードにしておく
	SceneManager::GetInstance().GetCamera()->ChangeMode(Camera::MODE::FIXED_POINT);

	// 操作説明の画像
	img_ = resMng_.Load(ResourceManager::SRC::PAD).handleId_;
	img2_ = resMng_.Load(ResourceManager::SRC::P1).handleId_;
	img3_ = resMng_.Load(ResourceManager::SRC::P2).handleId_;
}

void InstructionScene::Update(void)
{

	InputManager& ins = InputManager::GetInstance();

	// 右キー または SPACEキー で次のページへ
	if (ins.IsActionTrgDown(InputManager::ACTION::MENU_RIGHT))
	{
		if (currentPage_ < MAX_PAGES)
		{
			currentPage_++;
			AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
		}
		else
		{
			// 最終ページで進もうとしたらタイトルに戻る
			AudioManager::GetInstance()->PlaySE(SoundID::SE_CANCEL);
			SceneManager::GetInstance().PopScene();
		}
	}

	// 左キー で前のページに戻る
	if (ins.IsActionTrgDown(InputManager::ACTION::MENU_LEFT))
	{
		if (currentPage_ > 1)
		{
			currentPage_--;
			AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
		}
	}

	// どのページからでも即タイトルに戻る
	if (ins.IsActionTrgDown(InputManager::ACTION::CANCEL))
	{
		AudioManager::GetInstance()->PlaySE(SoundID::SE_CANCEL);
		SceneManager::GetInstance().PopScene();
	}
}

void InstructionScene::Draw(void)
{
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
		DrawString(x-60, bottomY, "→キーで次のページ", gray);
	}
	else
	{
		DrawString(x-60, bottomY, "キーでタイトルに戻る", gray);
	}

	if (currentPage_ > 1)
	{
		DrawString(x-60 + 400, bottomY, "←キーで前のページ", gray);
	}

	DrawString(20, 20, "SPACE: RETURN TO TITLE", GetColor(100, 100, 100));
}