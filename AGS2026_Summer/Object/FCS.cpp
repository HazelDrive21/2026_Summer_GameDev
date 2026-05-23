#include "Fcs.h"
#include "../Object/Enemy/EnemyBase.h" // 敵の情報取得用
#include "../Application.h" // 画面解像度取得用

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
	maxLockRange_ = 5000.0f;       // 機体に合わせて調整する射程距離
	requiredLockFrame_ = 30;       // ロックオンに必要な時間（30フレーム = 0.5秒）
}

void FCS::Update(const VECTOR& myPos, const std::vector<EnemyBase*>& enemies)
{
	// 1. サイトサイズの補間（既存の処理）
	siteWidth_ += (targetWidth_ - siteWidth_) * RESIZE_SPEED;
	siteHeight_ += (targetHeight_ - siteHeight_) * RESIZE_SPEED;

	// 2. 現在のターゲットが有効かチェック（見失い判定）
	if (targetEnemy_ != nullptr) {
		// 敵が死亡している、または距離が離れすぎたらロック解除（仕様に合わせて追加）
		float dist = VSize(VSub(targetEnemy_->GetTransform().pos, myPos));
		if (dist > maxLockRange_) {
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

		// 距離チェック
		float dist = VSize(VSub(enemy->GetTransform().pos, myPos));
		if (dist > maxLockRange_) continue;

		// 3D座標から画面の2D座標に変換
		VECTOR enemy2D = ConvWorldPosToScreenPos(enemy->GetTransform().pos);

		// カメラの後ろにいる場合は除外 (DxLibの画面深度判定値 z をチェック)
		if (enemy2D.z < 0.0f || enemy2D.z > 1.0f) continue;

		// 現在のサイトの枠内（矩形内）に入っているか判定
		float halfW = siteWidth_ / 2.0f;
		float halfH = siteHeight_ / 2.0f;
		if (enemy2D.x >= centerX_ - halfW && enemy2D.x <= centerX_ + halfW &&
			enemy2D.y >= centerY_ - halfH && enemy2D.y <= centerY_ + halfH)
		{
			// 画面中央（centerX_, centerY_）からの距離を計算
			float dx = enemy2D.x - centerX_;
			float dy = enemy2D.y - centerY_;
			float centerDist = dx * dx + dy * dy; // ルートを省いた平方距離

			// 最も画面中央に近い敵を選ぶ
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
	DrawSiteFrame();

	// デバッグ情報
	DrawFormatString(0, 80, GetColor(255, 255, 255), "FCS State: %s",
		lockState_ == LOCK_STATE::LOCKED ? "LOCKED" : "SEARCHING");
}

void FCS::ChangeSiteType(SITE_TYPE type)
{
	siteType_ = type;

	// タイプに合わせて目標サイズを設定（AC2AAのイメージ数値）
	switch (siteType_)
	{
	case SITE_TYPE::STANDARD:
		targetWidth_ = 500.0f;
		targetHeight_ = 500.0f;
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