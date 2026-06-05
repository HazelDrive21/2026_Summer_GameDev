#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Manager/Camera.h"
#include "../Object/SkyDome.h"
#include "ClearScene.h"

ClearScene::ClearScene(void)
{
	skyDome_ = nullptr;
}

ClearScene::~ClearScene(void)
{
	delete skyDome_;
}

void ClearScene::Init(void)
{
	// 背景の設定
	spaceDomeTran_.pos = AsoUtility::VECTOR_ZERO;
	skyDome_ = new SkyDome(spaceDomeTran_);
	skyDome_->Init();

	// 定点カメラモードにしておく
	SceneManager::GetInstance().GetCamera()->ChangeMode(Camera::MODE::FIXED_POINT);
}

void ClearScene::Update(void)
{
	skyDome_->Update();

	InputManager& ins = InputManager::GetInstance();

	// 決定アクション：メニューに戻る
	if (ins.IsActionTrgDown(InputManager::ACTION::DECIDE))
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::MENU);
	}
}

void ClearScene::Draw(void)
{
	skyDome_->Draw();

	// 画面中央に文字を表示（アセット画像がある場合は DrawRotaGraph 等に差し替えてください）
	int centerX = Application::SCREEN_SIZE_X / 2;
	int centerY = Application::SCREEN_SIZE_Y / 2;

	// AC風のフォントカラー（お好みで調整してください）
	unsigned int textColor = GetColor(0, 255, 200);

	DrawString(centerX - 100, centerY - 20, "OPERATION COMPLETE", textColor);
	DrawString(centerX - 120, centerY + 40, "PUSH SPACE TO TITLE SCENE", GetColor(255, 255, 255));
}