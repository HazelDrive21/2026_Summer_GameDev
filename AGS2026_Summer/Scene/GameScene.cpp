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
#include "../Object/Weapon/Bullet.h"
#include "GameScene.h"

GameScene::GameScene(void)
{
	player_ = nullptr;
	skyDome_ = nullptr;
	stage_ = nullptr;
	enemyManager_ = nullptr;
}

GameScene::~GameScene(void)
{
	delete player_;
	delete stage_;
	delete skyDome_;
	delete enemyManager_;
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

	// 1. 各オブジェクトの更新
	skyDome_->Update();
	stage_->Update();

	// ★★★ 修正：エネミーの更新（死亡判定とdelete）を先に持ってくる ★★★
	enemyManager_->Update();

	// enemyManager_ の中の生存エネミーリストが空になったらクリア画面へ
	if (enemyManager_->GetEemies().empty())
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::CLEAR);
	}

	// ★★★ 修正：その後にプレイヤーを更新する ★★★
	player_->Update();

	if (player_->GetHp() <= 0)
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::GAMEOVER);
	}

	auto& bullets = SceneManager::GetInstance().GetBulletList();
	const auto& enemies = enemyManager_->GetEemies(); // 敵のリストを取得

	for (auto it = bullets.begin(); it != bullets.end(); )
	{
		if (*it != nullptr)
		{
			(*it)->Update();

			bool isHit = false;

			// 弾の種類によって判定対象を分ける（AC風のスマートな振分け）
			if ((*it)->IsEnemyBullet())
			{
				// ① 敵の弾なら ⇒ プレイヤーとの判定
				// 弾の半径は仮で 2.0f としています
				if (player_->CheckHitBullet((*it)->GetPos(), 2.0f, (*it)->GetDamage()))
				{
					isHit = true;
				}
			}
			else
			{
				// ② プレイヤーの弾なら ⇒ すべての敵との判定
				for (auto* enemy : enemies)
				{
					if (enemy != nullptr && enemy->CheckHitBullet((*it)->GetPos(), 2.0f, (*it)->GetDamage()))
					{
						isHit = true;
						break; // 1つの弾が同時に複数の敵に当たらないように抜ける
					}
				}
			}

			// 当たった、または寿命が尽きた弾は削除
			if (isHit || (*it)->IsDead())
			{
				delete* it;
				it = bullets.erase(it);
			}
			else
			{
				++it;
			}
		}
		else
		{
			it = bullets.erase(it);
		}
	}

	// 2. カメラの更新
	Camera* camera = SceneManager::GetInstance().GetCamera();
	if (camera != nullptr)
	{
		camera->ProcessRot();
		camera->SyncFollow();
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
