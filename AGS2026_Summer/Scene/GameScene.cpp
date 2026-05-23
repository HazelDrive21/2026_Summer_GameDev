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
#include "../Object/Wepon/Bullet.h"
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

	// ★★★ ここにこの1行を追加！ ★★★
	// プレイヤーに生成したエネミーマネージャーを登録します
	if (player_ != nullptr) {
		player_->SetEnemyManager(enemyManager_);
	}

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

	// ★追加：すべての弾（プレイヤー・敵両方）の更新と自動クリーンアップ
	auto& bullets = SceneManager::GetInstance().GetBulletList();
	for (auto it = bullets.begin(); it != bullets.end(); )
	{
		if (*it != nullptr)
		{
			(*it)->Update(); // 弾を一歩進める

			// 💡 弾が寿命や衝突で消滅しているかチェック
			// ※ お使いのBulletクラスの「死亡フラグ取得関数（IsDeadやGetIsDeadなど）」に合わせてください
			if ((*it)->IsDead())
			{
				delete* it;             // メモリの解放
				it = bullets.erase(it); // リストから除外して次の要素へ
				continue;
			}
		}
		++it;
	}

}

void GameScene::Draw(void)
{

	// 背景
	skyDome_->Draw();
	stage_->Draw();
	
	player_->Draw();

	enemyManager_->Draw();

	// ★追加：すべての弾の描画
	auto& bullets = SceneManager::GetInstance().GetBulletList();
	for (auto* bullet : bullets)
	{
		if (bullet != nullptr)
		{
			bullet->Draw();
		}
	}

	player_->Draw2D();

}
