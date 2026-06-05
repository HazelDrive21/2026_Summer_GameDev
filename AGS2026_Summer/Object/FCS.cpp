#include <DxLib.h>
#include <algorithm>
#include "FCS.h"
#include "../Object/Enemy/EnemyBase.h" // 敵の情報取得用
#include "../Object/Player.h" // プレイヤーの情報取得用
#include "../Application.h" // 画面解像度取得用
#include "../Object/CharactorBase.h"
#include "../Object/Collider/ColliderCapsule.h"
#include "../Object/Weapon/WeaponMissile.h"

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
	lockTargets_.clear();
	lockTimer_ = 0;
	maxLockRange_ = 5000.0f;       // 機体に合わせて調整する射程距離
	requiredLockFrame_ = 30;       // ロックオンに必要な時間（30フレーム = 0.5秒）

	maxLockCount_ = 4;             // FCSの最大ロック数性能（ミサイル用）
	lockInterval_ = 12;            // 2体目以降の追加ロックにかかるフレーム間隔（約0.2秒）
}

void FCS::Update(const VECTOR& myPos, const std::vector<EnemyBase*>& enemies)
{
	// 1. サイトサイズの補間（既存の処理）
	siteWidth_ += (targetWidth_ - siteWidth_) * RESIZE_SPEED;
	siteHeight_ += (targetHeight_ - siteHeight_) * RESIZE_SPEED;

	// ─── ★新規：現在の選択武器から「このフレームの最大ロック数」を決定する ───
	int currentMaxLock = 1; // デフォルトは1（通常武器は同時使用せず、シングルロックのみ）

	if (player_ != nullptr)
	{
		WeaponBase* activeWeapon = player_->GetActiveWeapon(); // 現在手に持っている武器を取得

		// 武器がミサイルの場合のみ、FCS本来の最大マルチロック数を許可する
		if (activeWeapon != nullptr && dynamic_cast<WeaponMissile*>(activeWeapon) != nullptr)
		{
			currentMaxLock = maxLockCount_; // ミサイルならマルチロック(例: 4)を解放
		}
	}

	// 武器を切り替えた瞬間などに、現在のロック数が制限を超えていたら古いロックをクリアする安全処理
	if (lockTargets_.size() > static_cast<size_t>(currentMaxLock))
	{
		lockTargets_.clear();
		targetEnemy_ = nullptr;
		lockState_ = LOCK_STATE::NONE;
		lockTimer_ = 0;
	}

	// 2. 現在のターゲットたちが有効かチェック（見失い・死亡判定 ＆ 安全対策）
	// 通常武器用の主ターゲット判定
	if (targetEnemy_ != nullptr)
	{
		bool isExist = false;
		for (auto* enemy : enemies)
		{
			if (enemy == targetEnemy_ && enemy->GetHp() > 0)
			{
				isExist = true;
				break;
			}
		}
		if (!isExist)
		{
			targetEnemy_ = nullptr;
		}
	}

	// ─── ★新規：マルチロックリスト（lockTargets_）の生存チェック ───
	for (auto it = lockTargets_.begin(); it != lockTargets_.end(); )
	{
		bool isExist = false;
		for (auto* enemy : enemies)
		{
			if (enemy == *it && enemy->GetHp() > 0)
			{
				isExist = true;
				break;
			}
		}

		// 敵が死んだ、または存在しない場合はロックリストから排除
		if (!isExist)
		{
			it = lockTargets_.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 3. サイト内にいる、最も中央に近い敵を探索（ロック候補の洗い出し）
	EnemyBase* closestEnemy = nullptr;
	float minCenterDist = FLT_MAX;
	std::vector<EnemyBase*> enemiesInSite; // ★サイト内にいる全敵のリスト（マルチロック用）

	for (auto enemy : enemies) {
		if (enemy == nullptr || enemy->GetHp() <= 0) continue; // 安全対策＆死亡除外

		VECTOR enemyCenterPos = enemy->GetCenterPos();

		// 距離チェック
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
			enemiesInSite.push_back(enemy); // ★マルチロック候補として保存

			float dx = enemy2D.x - centerX_;
			float dy = enemy2D.y - centerY_;
			float centerDist = dx * dx + dy * dy;

			if (centerDist < minCenterDist) {
				minCenterDist = centerDist;
				closestEnemy = enemy;
			}
		}
	}

	// ─── ★新規：マルチロックリストから「サイト外に逃げた敵」を削除 ───
	for (auto it = lockTargets_.begin(); it != lockTargets_.end(); )
	{
		if (std::find(enemiesInSite.begin(), enemiesInSite.end(), *it) == enemiesInSite.end())
		{
			it = lockTargets_.erase(it); // サイト外に出たらロック解除
		}
		else
		{
			++it;
		}
	}

	// 4. ロックオンのステート更新（マルチロック対応）
	if (closestEnemy != nullptr) {
		// 主ターゲット（画面中央に一番近い敵）の選定・維持
		if (targetEnemy_ == nullptr || std::find(enemiesInSite.begin(), enemiesInSite.end(), targetEnemy_) == enemiesInSite.end()) {
			targetEnemy_ = closestEnemy;
		}

		// ─── ロックオンタイマー・リスト管理の本番 ───
		if (lockTargets_.empty()) {
			// 【第一段階】まだ誰もロックしていない（ファーストロック：緑枠から赤枠への遷移中）
			lockState_ = LOCK_STATE::LOCKING;
			lockTimer_++;

			if (lockTimer_ >= requiredLockFrame_) {
				lockState_ = LOCK_STATE::LOCKED; // 赤ロック完了！
				lockTargets_.push_back(targetEnemy_);
				lockTimer_ = 0;
				// TODO: SE再生「ピピッ」（ファーストロック音）
			}
		}
		else if (lockTargets_.size() < static_cast<size_t>(currentMaxLock)) {
			// 【第二段階】すでに1体以上ロックしているが、ミサイルかつ上限に達していない（追加マルチロック中）
			lockState_ = LOCK_STATE::LOCKED; // 画面表示自体はすでに赤ロック状態
			lockTimer_++;

			if (lockTimer_ >= lockInterval_) {
				// ACの仕様（サイト内の敵に均等に割り振り、余ったら重複ロック）を再現
				// サイト内にいる敵の中で、現在ロックされている数が「最も少ない敵」を優先して追加ロックする
				EnemyBase* bestCandidate = nullptr;
				size_t minLockCount = 999;

				for (auto* enemy : enemiesInSite) {
					size_t currentCount = std::count(lockTargets_.begin(), lockTargets_.end(), enemy);
					if (currentCount < minLockCount) {
						minLockCount = currentCount;
						bestCandidate = enemy;
					}
				}

				if (bestCandidate != nullptr) {
					lockTargets_.push_back(bestCandidate);
					// TODO: SE再生「カシャッ」（追加ロック音）
				}
				lockTimer_ = 0;
			}
		}
		else {
			// 【第三段階】フルロック状態
			lockState_ = LOCK_STATE::LOCKED;
			lockTimer_ = 0;
		}
	}
	else {
		// サイト内に誰もいなくなったら完全リセット
		targetEnemy_ = nullptr;
		lockTargets_.clear();
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

	// ─── ★修正：武器がミサイル、かつマルチロックが有効かどうかで描画を分岐 ───
	bool isMissileMode = (player_ && dynamic_cast<WeaponMissile*>(player_->GetActiveWeapon()) != nullptr);

	if (isMissileMode)
	{
		// デバッグ用：現在のマルチロック数を表示
		if (!lockTargets_.empty()) {
			DrawFormatString(centerX_ - 60, centerY_ + (int)(siteHeight_ / 2.0f) + 10,
				GetColor(255, 255, 255), "M-LOCK: %d / %d", lockTargets_.size(), maxLockCount_);
		}

		// 【ミサイル時】ロックしている数だけ、それぞれの敵にマーカーを描画（重複時は少しずらす）
		if (lockState_ == LOCK_STATE::LOCKED && !lockTargets_.empty())
		{
			unsigned int markerColor = GetColor(255, 64, 64); // LOCKED: 赤

			for (size_t i = 0; i < lockTargets_.size(); ++i)
			{
				if (lockTargets_[i] == nullptr) continue;

				VECTOR enemy3DPos = lockTargets_[i]->GetCenterPos();
				VECTOR screenPos = ConvWorldPosToScreenPos(enemy3DPos);

				if (screenPos.z > 0.0f &&
					screenPos.x >= 0 && screenPos.x <= screenWidth &&
					screenPos.y >= 0 && screenPos.y <= screenHeight)
				{
					int x = static_cast<int>(screenPos.x);
					// ★本家AC風演出：同じ敵に複数ロック（重複）している場合、縦に少しずらして「マルチロックが重なっている」ことを表現する
					int y = static_cast<int>(screenPos.y) + (static_cast<int>(i) * 6);
					int radius = 10;

					// ミサイル用は円形か、重ね対応のマーカーにする
					DrawCircle(x, y, radius, markerColor, FALSE);
					DrawCircle(x, y, radius - 3, markerColor, FALSE);
				}
			}
		}
	}
	else
	{
		// 【通常武器時】ご提示いただいた元の「四角枠」のロックオン表示を使用（targetEnemy_ のみを見る）
		if (targetEnemy_ != nullptr)
		{
			VECTOR enemy3DPos = targetEnemy_->GetCenterPos();
			VECTOR screenPos = ConvWorldPosToScreenPos(enemy3DPos);

			if (screenPos.z > 0.0f &&
				screenPos.x >= 0 && screenPos.x <= screenWidth &&
				screenPos.y >= 0 && screenPos.y <= screenHeight)
			{
				int x = static_cast<int>(screenPos.x);
				int y = static_cast<int>(screenPos.y);
				int boxSize = 24;

				unsigned int markerColor = GetColor(0, 255, 128); // LOCKING: 緑
				if (lockState_ == LOCK_STATE::LOCKED)
				{
					markerColor = GetColor(255, 64, 64); // LOCKED: 赤
				}

				// ① 敵を捉えるロックオンボックス（四角枠）
				DrawBox(x - boxSize, y - boxSize, x + boxSize, y + boxSize, markerColor, FALSE);

				// ② AC風演出
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
}

void FCS::ChangeSiteType(SITE_TYPE type)
{
	siteType_ = type;

	// タイプに合わせて目標サイズを設定（AC2AAのイメージ数値）
	switch (siteType_)
	{
	case SITE_TYPE::STANDARD:
		targetWidth_ = 600.0f;
		targetHeight_ = 600.0f;
		maxLockRange_ = 5000.0f;
		break;
	case SITE_TYPE::WIDE_SHALLOW:
		targetWidth_ = 500.0f;
		targetHeight_ = 150.0f;
		maxLockRange_ = 1000.0f;
		break;
	case SITE_TYPE::DEEP_NARROW:
		targetWidth_ = 150.0f;
		targetHeight_ = 500.0f;
		maxLockRange_ = 1000.0f;
		break;
	case SITE_TYPE::LARGE:
		targetWidth_ = 450.0f;
		targetHeight_ = 450.0f;
		maxLockRange_ = 1200.0f;
		break;
	}
}

VECTOR FCS::CalcPredictivePos(float bulletSpeed, const VECTOR& myPos) const
{
	// ★通常武器は同時使用せず単発運用されるため、主ターゲット(targetEnemy_)だけを追えばよく、元のコードのままで完璧に動作します！
	if (targetEnemy_ == nullptr || targetEnemy_->GetHp() <= 0)
	{
		return VGet(0.0f, 0.0f, 0.0f);
	}

	VECTOR enemyPos = targetEnemy_->GetTransform().pos;
	int capsuleKey = static_cast<int>(CharactorBase::COLLIDER_TYPE::CAPSULE);
	const auto& enemyColliders = targetEnemy_->GetOwnColliders();

	if (enemyColliders.count(capsuleKey) > 0)
	{
		const ColliderBase* baseCollider = enemyColliders.at(capsuleKey);
		if (baseCollider != nullptr && baseCollider->GetShape() == ColliderBase::SHAPE::CAPSULE)
		{
			const ColliderCapsule* capsule = static_cast<const ColliderCapsule*>(baseCollider);
			enemyPos = capsule->GetCenter();
		}
	}

	VECTOR enemyVel = targetEnemy_->GetVelocity();
	float dist = VSize(VSub(enemyPos, myPos));

	if (bulletSpeed <= 0.0f) bulletSpeed = 1.0f; // ゼロ除算防止
	float time = dist / bulletSpeed;

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
