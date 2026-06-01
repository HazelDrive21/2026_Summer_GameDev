#include <string>
#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/SceneManager.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/InputManager.h"
#include "../Manager/Camera.h"
#include "../Object/Common/AnimationController.h"
#include "../Object/SkyDome.h"
#include "TitleScene.h"

TitleScene::TitleScene(void)
{
	imgPush_ = -1;
	imgTitle_ = -1;
	skyDome_ = nullptr;
	animationController_ = nullptr;
}

TitleScene::~TitleScene(void)
{
	delete skyDome_;
	delete animationController_;
}

void TitleScene::Init(void)
{

	// 画像読み込み
	imgTitle_ = resMng_.Load(ResourceManager::SRC::TITLE).handleId_;
	imgPush_ = resMng_.Load(ResourceManager::SRC::PUSH_SPACE).handleId_;

	// 背景
	spaceDomeTran_.pos = AsoUtility::VECTOR_ZERO;
	skyDome_ = new SkyDome(spaceDomeTran_);
	skyDome_->Init();

	float size;

	// メイン惑星
	planet_.SetModel(resMng_.LoadModelDuplicate(ResourceManager::SRC::FALL_PLANET));
	planet_.pos = AsoUtility::VECTOR_ZERO;
	planet_.scl = AsoUtility::VECTOR_ONE;
	planet_.Update();

	// 回転する惑星
	movePlanet_.SetModel(resMng_.LoadModelDuplicate(ResourceManager::SRC::LAST_PLANET));
	movePlanet_.pos = { -250.0f, -100.0f, -100.0f };
	size = 0.7f;
	movePlanet_.scl = { size, size, size };
	movePlanet_.quaRotLocal = Quaternion::Euler(
		AsoUtility::Deg2RadF(90.0f), 0.0f, 0.0f);
	movePlanet_.Update();

	// キャラ
	charactor_.SetModel(resMng_.LoadModelDuplicate(ResourceManager::SRC::PLAYER));
	charactor_.pos = { -250.0f, -32.0f, -105.0f };
	size = 0.4f;
	charactor_.scl = { size, size, size };
	charactor_.quaRot = Quaternion::Euler(
		0.0f, AsoUtility::Deg2RadF(90.0f), 0.0f);
	charactor_.Update();

	// アニメーションの設定
	std::string path = Application::PATH_MODEL + "Player/";
	animationController_ = new AnimationController(charactor_.modelId);
	animationController_->Add(0, path + "Run.mv1", 20.0f);
	animationController_->Play(0);

	// 定点カメラ
	SceneManager::GetInstance().GetCamera()->ChangeMode(Camera::MODE::FIXED_POINT);

}

void TitleScene::Update(void)
{
	InputManager& ins = InputManager::GetInstance();

	// 🔥 選択肢の切り替え（上下キーまたはパッドの上下）
	if (ins.IsTrgDown(KEY_INPUT_UP)) {
		cursor_ = (MENU)(((int)cursor_ + (int)MENU::MAX - 1) % (int)MENU::MAX);
	}
	if (ins.IsTrgDown(KEY_INPUT_DOWN)) {
		cursor_ = (MENU)(((int)cursor_ + 1) % (int)MENU::MAX);
	}

	// 🔥 決定処理
	if (ins.IsTrgDown(KEY_INPUT_Z))
	{
		if (cursor_ == MENU::START) {
			SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::GAME);
		}
		else if (cursor_ == MENU::MANUAL) {
			SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::INSTRUCTION);
		}
	}

	// ... 惑星の回転やアニメーション更新（既存コード） ...
	animationController_->Update();
	skyDome_->Update();
}

void TitleScene::Draw(void)
{

	skyDome_->Draw();
	MV1DrawModel(planet_.modelId);
	MV1DrawModel(movePlanet_.modelId);
	MV1DrawModel(charactor_.modelId);

	// タイトルロゴなど
	DrawString(Application::SCREEN_SIZE_X / 2, 300, "TITLE SCENE", GetColor(255, 255, 255));

	// 🔥 選択肢の描画
	int centerX = Application::SCREEN_SIZE_X / 2;
	unsigned int white = GetColor(255, 255, 255);
	unsigned int cyan = GetColor(0, 255, 255); // 選択中の色

	// 1. ゲームスタート
	unsigned int colorStart = (cursor_ == MENU::START) ? cyan : white;
	const char* textStart = (cursor_ == MENU::START) ? "> START" : "START";
	DrawString(centerX - 80, 450, textStart, colorStart);

	// 2. 操作方法
	unsigned int colorManual = (cursor_ == MENU::MANUAL) ? cyan : white;
	const char* textManual = (cursor_ == MENU::MANUAL) ? "> HOW TO PLAY" : "  HOW TO PLAY";
	DrawString(centerX - 80, 500, textManual, colorManual);

	DrawString(centerX - 120, 550, "キーボードの方向キーで選択・Zキーで決定", GetColor(255, 255, 255));

}
