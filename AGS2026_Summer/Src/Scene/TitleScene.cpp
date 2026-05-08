#include <DxLib.h>
#include "../Utility/AsoUtility.h"
#include "../Manager/InputManager.h"
#include "../Manager/SceneManager.h"
#include "../Manager/Camera.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/Resource.h"
#include "../Application.h"
#include "../Object/Common/AnimationController.h"
#include "../Object/Actor/SkyDome.h"
#include "TitleScene.h"

TitleScene::TitleScene(void)
	:
	imgTitle_(-1),
	imgPushSpace_(-1),
	charactor_(),
	animationController_(nullptr),
	skyDome_(nullptr),
	SceneBase()
{
}

TitleScene::~TitleScene(void)
{
}

void TitleScene::Init(void)
{

	// 画像読み込み
	imgTitle_ = resMng_.Load(ResourceManager::SRC::TITLE).handleId_;
	imgPushSpace_ = resMng_.Load(ResourceManager::SRC::PUSH_SPACE).handleId_;


	// 定点カメラ
	sceMng_.GetCamera()->ChangeMode(Camera::MODE::FIXED_POINT);

	// キャラクター
	charactor_.SetModel(
		resMng_.Load(ResourceManager::SRC::PLAYER).handleId_);
	charactor_.scl = { SCL_CHARACTOR, SCL_CHARACTOR, SCL_CHARACTOR };
	charactor_.quaRot = Quaternion::Euler(ROT_CHARACTOR);
	charactor_.quaRotLocal = Quaternion::Euler(ROT_LOCAL_CHARACTOR);
	charactor_.pos = POS_CHARACTOR;
	charactor_.Update();

	// アニメーションコントローラー
	animationController_ = new AnimationController(charactor_.modelId);
	animationController_->Add(0, 20.0f, Application::PATH_MODEL + "Player/Run.mv1");
	animationController_->Play(0, true);

	// スカイドーム生成
	skyDome_ = new SkyDome(empty_);
	skyDome_->Init();
}

void TitleScene::Update(void)
{

	// シーン遷移
	auto const& ins = InputManager::GetInstance();
	if (ins.IsTrgDown(KEY_INPUT_SPACE))
	{
		sceMng_.ChangeScene(SceneManager::SCENE_ID::GAME);
	}

	// アニメーション更新
	animationController_->Update();

	// スカイドーム更新
	skyDome_->Update();
}

void TitleScene::Draw(void)
{

	// スカイドーム描画
	skyDome_->Draw();

	// モデル描画
	MV1DrawModel(charactor_.modelId);

	//UI描画
	DrawRotaGraph(
		Application::SCREEN_SIZE_X / 2,
		IMG_TITLE_POS_Y,
		1.0, 0.0, imgTitle_, true);

	DrawRotaGraph(
		Application::SCREEN_SIZE_X / 2,
		IMG_PUSH_POS_Y,
		1.0, 0.0, imgPushSpace_, true);
}

void TitleScene::Release(void)
{
	animationController_->Release();
	delete animationController_;

	skyDome_->Release();
	delete skyDome_;
}
