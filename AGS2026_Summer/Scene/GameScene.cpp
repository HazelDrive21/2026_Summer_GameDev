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
#include "../Audio/AudioManager.h"
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

	AudioManager::GetInstance()->LoadSceneSound(LoadScene::GAME);
	AudioManager::GetInstance()->PlayBGM(SoundID::BGM_GAME);

	// ステージ
	stage_ = new Stage();
	stage_->Init();

	SceneManager::GetInstance().SetStageModelHandle(stage_->GetModelHandle());

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
	// 1. 各オブジェクトの更新
	skyDome_->Update();
	stage_->Update();

	// エネミーの更新（死亡判定とdelete）
	enemyManager_->Update();

	// 生存エネミーリストが空になったらクリア画面へ
	if (enemyManager_->GetEemies().empty())
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::RESULT);
	}

	// プレイヤーの更新
	player_->Update();

	if (player_->GetHp() <= 0)
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::RESULT);
	}

	auto& bullets = SceneManager::GetInstance().GetBulletList();
	const auto& enemies = enemyManager_->GetEemies();

	int stageHandle = (stage_ != nullptr) ? stage_->GetModelHandle() : -1;

	for (auto it = bullets.begin(); it != bullets.end(); )
	{
		if (*it != nullptr)
		{
			// ⚡ ここで弾の位置が更新され、内部で pos_ と prevPos_ が確定する
			(*it)->Update(stageHandle);

			bool isHit = false;

			// ⚡ 判定に必要な弾の情報をあらかじめ取得しておく
			VECTOR bPrev = (*it)->GetPrevPos();
			VECTOR bCurr = (*it)->GetPos();
			float bRadius = (*it)->GetRadius();
			int bDamage = (*it)->GetDamage();

			// 弾の種類によって判定対象を分ける
			if ((*it)->IsEnemyBullet())
			{
				// ① 敵の弾なら ⇒ プレイヤーとの判定
				// ⚡ 【修正】前フレーム座標、現フレーム座標、半径、ダメージを渡す
				if (player_->CheckHitBullet(bPrev, bCurr, bRadius, bDamage))
				{
					isHit = true;
				}
			}
			else
			{
				// ② プレイヤーの弾なら ⇒ すべての敵との判定
				for (auto* enemy : enemies)
				{
					// ⚡ 【修正】敵側の CheckHitBullet も同様の引数に合わせる
					if (enemy != nullptr && enemy->CheckHitBullet(bPrev, bCurr, bRadius, bDamage))
					{
						isHit = true;
						break;
					}
				}
			}

			// 当たった、または寿命が尽きた（地形に当たった場合も含む）弾は削除
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

	// ポーズメニューへの遷移
	if (InputManager::GetInstance().IsActionTrgDown(InputManager::ACTION::PAUSE))
	{
		AudioManager::GetInstance()->PlaySE(SoundID::SE_CANCEL);
		SceneManager::GetInstance().PushScene(SceneManager::SCENE_ID::PAUSE);
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
