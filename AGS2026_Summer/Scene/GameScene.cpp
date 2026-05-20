#include <DxLib.h>
#include "../Utility/AsoUtility.h"
#include "../Manager/SceneManager.h"
#include "../Manager/Camera.h"
#include "../Manager/InputManager.h"
#include "../Object/Common/Capsule.h"
#include "../Object/Common/Collider.h"
#include "../Object/SkyDome.h"
#include "../Object/Stage.h"
#include "../Object/Player.h"
#include "../Object/Enemy/EnemyManager.h"
#include "GameScene.h"

GameScene::GameScene(void)
{
	player_ = nullptr;
	skyDome_ = nullptr;
	stage_ = nullptr;
}

GameScene::~GameScene(void)
{
	delete player_;
	delete stage_;
	delete skyDome_;
}

void GameScene::Init(void)
{
	// ステージ
	stage_ = new Stage();
	stage_->Init();

	// プレイヤー
	player_ = new Player();
	player_->Init();
	SceneManager::GetInstance().SetPlayer(player_);

	// カメラ
	Camera* camera = SceneManager::GetInstance().GetCamera();
	if (camera != nullptr) {
		// カメラに Player 自体を登録（急停止時の Lerp 用）
		camera->SetPlayer(player_);

		// カメラに追従対象の Transform を登録
		// ※GetTransform() が参照を返す場合は & を付け、ポインタを返す場合はそのまま渡します
		camera->SetFollow(&player_->GetTransform());

		// カメラモードを追従モードに設定
		camera->ChangeMode(Camera::MODE::FOLLOW);
	}

	
	const ColliderBase* stageCollider =
		stage_->GetOwnCollider(static_cast<int>(Stage::COLLIDER_TYPE::MODEL));
	player_->AddHitCollider(stageCollider);

	// エネミー
	enemyManager_ = new EnemyManager();
	enemyManager_->Init();
	enemyManager_->AddHitCollider(stageCollider);

	// スカイドーム
	skyDome_ = new SkyDome(player_->GetTransform());
	skyDome_->Init();

	SceneManager::GetInstance().GetCamera()->SetFollow(&player_->GetTransform());
	SceneManager::GetInstance().GetCamera()->ChangeMode(Camera::MODE::FOLLOW);

}

void GameScene::Update(void)
{

	// シーン遷移
	InputManager& ins = InputManager::GetInstance();
	if (ins.IsTrgDown(KEY_INPUT_SPACE))
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
	}

	skyDome_->Update();

	stage_->Update();

	player_->Update();

	enemyManager_->Update();

}

void GameScene::Draw(void)
{

	// 背景
	skyDome_->Draw();
	stage_->Draw();
	
	player_->Draw();

	enemyManager_->Draw();

}
