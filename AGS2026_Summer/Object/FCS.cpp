#include <DxLib.h>
#include "FCS.h"
#include "../Object/Enemy/EnemyBase.h" // 敵の情報取得用
#include "../Object/Player.h" // プレイヤーの情報取得用
#include "../Application.h" // 画面解像度取得用
#include "../Object/CharactorBase.h"
#include "../Object/Collider/ColliderCapsule.h"

FCS::FCS(void)
	: player_(nullptr)
	, siteType_(SITE_TYPE::STANDARD)
	, lockState_(LOCK_STATE::NONE)
{
}

FCS::~FCS(void)
{
}

void FCS::Init(void)
{
	// 画面中央を取得
	centerX_ = Application::SCREEN_SIZE_X / 2;
	centerY_ = Application::SCREEN_SIZE_Y / 2;

	// 初期サイト設定
	ChangeSiteType(SITE_TYPE::STANDARD);

	// 現在値を目標値に即座に合わせる
	siteWidth_ = targetWidth_;
	siteHeight_ = targetHeight_;

	lockState_ = LOCK_STATE::NONE;
	siteColor_ = GetColor(255, 255, 255);

	targetEnemy_ = nullptr;
	lockTimer_ = 0;
	maxLockRange_ = 3000.0f;       // 機体に合わせて調整する射程距離
	requiredLockFrame_ = 30;       // ロックオンに必要な時間（30フレーム = 0.5秒）
}

void FCS::Update(const VECTOR& myPos, const std::vector<EnemyBase*>& enemies)
{
	// 1. サイトサイズの補間（既存の処理）
	siteWidth_ += (targetWidth_ - siteWidth_) * RESIZE_SPEED;
	siteHeight_ += (targetHeight_ - siteHeight_) * RESIZE_SPEED;

	// 2. 現在のターゲットが有効かチェック（見失い判定 ＆ ★安全対策）
	if (targetEnemy_ != nullptr)
	{
		bool isExist = false;
		for (auto* enemy : enemies)
		{
			if (enemy == targetEnemy_)
			{
				isExist = true;
				break;
			}
		}

		// リストから消えている、またはHPが0以下の場合は即座にロックを解除する
		if (!isExist || targetEnemy_->GetHp() <= 0)
		{
			targetEnemy_ = nullptr;
			lockState_ = LOCK_STATE::NONE;
			lockTimer_ = 0;
		}
	}

	// 3. サイト内にいる、最も中央に近い敵を探索
	EnemyBase* closestEnemy = nullptr;
	float minCenterDist = FLT_MAX;

	for (auto enemy : enemies) {
		if (enemy == nullptr) continue; // 安全対策

		// ★キャラクターから安全に中心座標を取得する
		VECTOR enemyCenterPos = enemy->GetCenterPos();

		// 距離チェック（中心座標ベース）
		float dist = VSize(VSub(enemyCenterPos, myPos));
		if (dist > maxLockRange_) continue;

		// 3D座標から画面の2D座標に変換
		VECTOR enemy2D = ConvWorldPosToScreenPos(enemyCenterPos);

		// カメラの後ろにいる場合は除外
		if (enemy2D.z < 0.0f || enemy2D.z > 1.0f) continue;

		// 現在のサイトの枠内（矩形内）に入っているか判定
		float halfW = siteWidth_ / 2.0f;
		float halfH = siteHeight_ / 2.0f;
		if (enemy2D.x >= centerX_ - halfW && enemy2D.x <= centerX_ + halfW &&
			enemy2D.y >= centerY_ - halfH && enemy2D.y <= centerY_ + halfH)
		{
			float dx = enemy2D.x - centerX_;
			float dy = enemy2D.y - centerY_;
			float centerDist = dx * dx + dy * dy;

			if (centerDist < minCenterDist) {
				minCenterDist = centerDist;
				closestEnemy = enemy;
			}
		}
	}

	// 4. ロックオンのステート更新
	if (closestEnemy != nullptr) {
		// 新しい敵を捉えた、または同じ敵を継続して捉えている場合
		if (targetEnemy_ != closestEnemy) {
			targetEnemy_ = closestEnemy;
			lockState_ = LOCK_STATE::LOCKING;
			lockTimer_ = 0;
		}

		if (lockState_ == LOCK_STATE::LOCKING) {
			lockTimer_++;
			if (lockTimer_ >= requiredLockFrame_) {
				lockState_ = LOCK_STATE::LOCKED; // ロック完了！
			}
		}
	}
	else {
		// サイト内に誰もいなくなったらリセット
		targetEnemy_ = nullptr;
		lockState_ = LOCK_STATE::NONE;
		lockTimer_ = 0;
	}

	// 5. 色の更新（既存の処理）
	UpdateSiteStyle();
}

void FCS::Draw(void)
{
	int screenWidth, screenHeight;
	GetDrawScreenSize(&screenWidth, &screenHeight);

	// 1. 画面中央のFCS領域（レティクル枠）の描画
	DrawSiteFrame();

	// 2. ロックオン対象（敵機）を追尾するターゲットマーカーの描画
	if (targetEnemy_ != nullptr)
	{
		// ★キャラクターから安全に中心座標を取得する
		VECTOR enemy3DPos = targetEnemy_->GetCenterPos();

		// 3D中心座標を2Dスクリーン座標に変換
		VECTOR screenPos = ConvWorldPosToScreenPos(enemy3DPos);

		// カメラの後方ではなく、かつ画面内に収まっているかチェック
		if (screenPos.z > 0.0f &&
			screenPos.x >= 0 && screenPos.x <= screenWidth &&
			screenPos.y >= 0 && screenPos.y <= screenHeight)
		{
			int x = static_cast<int>(screenPos.x);
			int y = static_cast<int>(screenPos.y);
			int boxSize = 24; // 敵を囲う四角のサイズ

			// ロック状態によってサイトの色を切り替える
			unsigned int markerColor = GetColor(0, 255, 128); // LOCKING: 緑
			if (lockState_ == LOCK_STATE::LOCKED)
			{
				markerColor = GetColor(255, 64, 64); // LOCKED: 赤
			}

			// ① 敵を捉えるロックオンボックス（四角枠）
			DrawBox(x - boxSize, y - boxSize, x + boxSize, y + boxSize, markerColor, FALSE);

			// ② AC風演出：ロック完了（赤）なら、さらにデザインを強化
			if (lockState_ == LOCK_STATE::LOCKED)
			{
				DrawBox(x - boxSize + 4, y - boxSize + 4, x + boxSize - 4, y + boxSize - 4, markerColor, FALSE);
				DrawString(x + boxSize + 6, y - 8, "LOCKED", markerColor);
			}
			else if (lockState_ == LOCK_STATE::LOCKING)
			{
				DrawString(x + boxSize + 6, y - 8, "LOCKING...", markerColor);
			}
		}
	}
}

void FCS::ChangeSiteType(SITE_TYPE type)
{
	siteType_ = type;

	// タイプに合わせて目標サイズを設定（AC2AAのイメージ数値）
	switch (siteType_)
	{
	case SITE_TYPE::STANDARD:
		targetWidth_ = 300.0f;
		targetHeight_ = 300.0f;
		break;
	case SITE_TYPE::WIDE_SHALLOW:
		targetWidth_ = 500.0f;
		targetHeight_ = 150.0f;
		break;
	case SITE_TYPE::DEEP_NARROW:
		targetWidth_ = 150.0f;
		targetHeight_ = 500.0f;
		break;
	case SITE_TYPE::LARGE:
		targetWidth_ = 450.0f;
		targetHeight_ = 450.0f;
		break;
	}
}

VECTOR FCS::CalcPredictivePos(float bulletSpeed, const VECTOR& myPos) const
{
	// ─── ★追加：安全弁（ヌルポインタ・死体チェックのガード句） ───
	if (targetEnemy_ == nullptr || targetEnemy_->GetHp() <= 0)
	{
		// ターゲットが無効な場合は、予測位置として仮の原点（またはプレイヤーの正面など）を返す
		return VGet(0.0f, 0.0f, 0.0f);
	}
	// ──────────────────────────────────────────────────────────

	// 1. 敵の「現在位置」を取得（CharactorBase や ActorBase の構造に合わせて取得してください）
	VECTOR enemyPos = targetEnemy_->GetTransform().pos;

	int capsuleKey = static_cast<int>(CharactorBase::COLLIDER_TYPE::CAPSULE);
	const auto& enemyColliders = targetEnemy_->GetOwnColliders();

	if (enemyColliders.count(capsuleKey) > 0)
	{
		const ColliderBase* baseCollider = enemyColliders.at(capsuleKey);
		if (baseCollider != nullptr && baseCollider->GetShape() == ColliderBase::SHAPE::CAPSULE)
		{
			// 安全にカプセル型へキャストし、中心座標（胴体）を基準座標として上書き
			const ColliderCapsule* capsule = static_cast<const ColliderCapsule*>(baseCollider);
			enemyPos = capsule->GetCenter();
		}
	}

	// 2. 敵の「速度ベクトル」を取得
	VECTOR enemyVel = targetEnemy_->GetVelocity();

	// 3. 自分と敵の現在の距離を計算
	float dist = VSize(VSub(enemyPos, myPos));

	// 4. 弾が敵に届くまでにかかる時間(フレーム数)を計算
	if (bulletSpeed <= 0.0f) bulletSpeed = 1.0f; // ゼロ除算防止
	float time = dist / bulletSpeed;

	// 5. 未来の予測位置を計算
	VECTOR predictPos = VAdd(enemyPos, VScale(enemyVel, time));

	return predictPos;
}

void FCS::UpdateSiteStyle(void)
{
	// 本来はPlayer経由でターゲットとの距離やロック進捗を見て色を変える
	switch (lockState_)
	{
	case LOCK_STATE::NONE:
		siteColor_ = GetColor(0, 255, 0); // 緑（AC2AAの基本色）
		break;
	case LOCK_STATE::LOCKED:
		siteColor_ = GetColor(255, 0, 0); // 赤
		break;
	}
}

void FCS::DrawSiteFrame(void)
{
	// サイトの四隅の座標計算
	int x1 = (int)(centerX_ - siteWidth_ / 2.0f);
	int y1 = (int)(centerY_ - siteHeight_ / 2.0f);
	int x2 = (int)(centerX_ + siteWidth_ / 2.0f);
	int y2 = (int)(centerY_ + siteHeight_ / 2.0f);

	// サイトの枠線描画（AC2風に、四隅に少し線を引くか、単純な矩形）
	// ここでは単純な矩形を描画
	DrawBox(x1, y1, x2, y2, siteColor_, FALSE);

	// AC特有の「L字」の角を描画する場合はここに追加
	int len = 20; // 角の線の長さ
	// 左上
	DrawLine(x1, y1, x1 + len, y1, siteColor_);
	DrawLine(x1, y1, x1, y1 + len, siteColor_);
	// 右上
	DrawLine(x2, y1, x2 - len, y1, siteColor_);
	DrawLine(x2, y1, x2, y1 + len, siteColor_);
	// 左下
	DrawLine(x1, y2, x1 + len, y2, siteColor_);
	DrawLine(x1, y2, x1, y2 - len, siteColor_);
	// 右下
	DrawLine(x2, y2, x2 - len, y2, siteColor_);
	DrawLine(x2, y2, x2, y2 - len, siteColor_);
}
