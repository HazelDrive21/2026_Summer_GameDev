#include <DxLib.h>
#include <algorithm>
#include "FCS.h"
#include "../Object/Enemy/EnemyBase.h" // 敵の情報取得用
#include "../Object/Player.h" // プレイヤーの情報取得用
#include "../Application.h" // 画面解像度取得用
#include "../Object/CharactorBase.h"
#include "../Object/Collider/ColliderCapsule.h"
#include "../Object/Weapon/WeaponMissile.h"
#include "../Audio/AudioManager.h"

FCS::FCS(void)
	: player_(nullptr)
	, siteType_(SITE_TYPE::STANDARD)
	, currentCombinedType_(SITE_TYPE::STANDARD)
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

FCS::SITE_TYPE FCS::CombineSiteType(SITE_TYPE fcsType, SITE_TYPE wpType)
{
	// どちらかが LARGE なら、もう片方の形状がそのまま活きる
	if (fcsType == SITE_TYPE::LARGE) return wpType;
	if (wpType == SITE_TYPE::LARGE) return fcsType;

	// 同じタイプならそのまま
	if (fcsType == wpType) return fcsType;

	// 横広（WIDE）と 縦長（DEEP）が合わさると、重なる中央の正方形（STANDARD）になる
	if ((fcsType == SITE_TYPE::WIDE_SHALLOW && wpType == SITE_TYPE::DEEP_NARROW) ||
		(fcsType == SITE_TYPE::DEEP_NARROW && wpType == SITE_TYPE::WIDE_SHALLOW))
	{
		return SITE_TYPE::STANDARD;
	}

	// それ以外（片方が STANDARD の場合）は、狭い方の形状（STANDARD 以外）に引っ張られる
	return (fcsType != SITE_TYPE::STANDARD) ? fcsType : wpType;
}

// ─── FCS.cpp ───
// 関数の最初（シグネチャ）に「float weaponRange」を追加します
void FCS::Update(const VECTOR& myPos, const std::vector<EnemyBase*>& enemies,
	int weaponMaxLockCount, float weaponRange, SITE_TYPE weaponSiteType)
{
	LOCK_STATE prevLockState = lockState_;
	size_t prevLockCount = lockTargets_.size();

	// ─── ★ここを追加：FCSと武器のサイトタイプを組み合わせて、今の形状を決定 ───
	SITE_TYPE newCombinedType = CombineSiteType(siteType_, weaponSiteType);

	// 形状が変わった瞬間だけ、目標サイズ（targetWidth_ / Height_）を更新する
	if (newCombinedType != currentCombinedType_)
	{
		currentCombinedType_ = newCombinedType;

		// 組み合わせ後のタイプに応じて目標サイズを設定
		// （※数値は現在の画面解像度に合わせて自由に調整してください）
		switch (currentCombinedType_)
		{
		case SITE_TYPE::STANDARD:
			targetWidth_ = 300.0f;		targetHeight_ = 300.0f;
			break;
		case SITE_TYPE::WIDE_SHALLOW:
			targetWidth_ = 400.0f;		targetHeight_ = 400.0f;
			break;
		case SITE_TYPE::DEEP_NARROW:
			targetWidth_ = 200.0f;		targetHeight_ = 200.0f;
			break;
		case SITE_TYPE::LARGE:
			targetWidth_ = 380.0f;		targetHeight_ = 380.0f;
			break;
		}
	}

	// 1. サイトサイズの補間（既存のこの処理によって、変形時にカシャッと滑らかにサイズが変わります！）
	siteWidth_ += (targetWidth_ - siteWidth_) * RESIZE_SPEED;
	siteHeight_ += (targetHeight_ - siteHeight_) * RESIZE_SPEED;

	// ─── ★追加：FCS性能の最大射程と、武器の射程の小さい方を今回の有効射程にする ───
	float currentMaxRange = (std::min)(maxLockRange_, weaponRange);

	// ─── 引数 weaponMaxLockCount を活かし、FCS性能の上限と合わせて最終的な最大ロック数を決定 ───
	int currentMaxLock = 1;
	if (weaponMaxLockCount > 0)
	{
		currentMaxLock = (std::min)(maxLockCount_, weaponMaxLockCount);
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
	std::vector<EnemyBase*> enemiesInSite;

	for (auto enemy : enemies) {
		if (enemy == nullptr || enemy->GetHp() <= 0) continue;

		VECTOR enemyCenterPos = enemy->GetCenterPos();

		// ─── ★修正：maxLockRange_ ではなく、計算した currentMaxRange を使う ───
		float dist = VSize(VSub(enemyCenterPos, myPos));
		if (dist > currentMaxRange) continue;

		VECTOR enemy2D = ConvWorldPosToScreenPos(enemyCenterPos);

		if (enemy2D.z < 0.0f || enemy2D.z > 1.0f) continue;

		float halfW = siteWidth_ / 2.0f;
		float halfH = siteHeight_ / 2.0f;
		if (enemy2D.x >= centerX_ - halfW && enemy2D.x <= centerX_ + halfW &&
			enemy2D.y >= centerY_ - halfH && enemy2D.y <= centerY_ + halfH)
		{
			enemiesInSite.push_back(enemy);

			float dx = enemy2D.x - centerX_;
			float dy = enemy2D.y - centerY_;
			float centerDist = dx * dx + dy * dy;

			if (centerDist < minCenterDist) {
				minCenterDist = centerDist;
				closestEnemy = enemy;
			}
		}
	}

	// サイト内から外れた（または射程外に逃げた）敵をロック解除する
	for (auto it = lockTargets_.begin(); it != lockTargets_.end(); )
	{
		if (std::find(enemiesInSite.begin(), enemiesInSite.end(), *it) == enemiesInSite.end())
		{
			it = lockTargets_.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 4. ロックオンのステート更新（マルチロック対応）
	if (closestEnemy != nullptr) {
		if (targetEnemy_ == nullptr || std::find(enemiesInSite.begin(), enemiesInSite.end(), targetEnemy_) == enemiesInSite.end()) {
			targetEnemy_ = closestEnemy;
		}

		if (lockTargets_.empty()) {
			lockState_ = LOCK_STATE::LOCKING;
			lockTimer_++;

			if (lockTimer_ >= requiredLockFrame_) {
				lockState_ = LOCK_STATE::LOCKED;
				lockTargets_.push_back(targetEnemy_);
				lockTimer_ = 0;
			}
		}
		else if (lockTargets_.size() < static_cast<size_t>(currentMaxLock)) {
			lockState_ = LOCK_STATE::LOCKED;
			lockTimer_++;

			if (lockTimer_ >= lockInterval_) {
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
				}
				lockTimer_ = 0;
			}
		}
		else {
			lockState_ = LOCK_STATE::LOCKED;
			lockTimer_ = 0;
		}
	}
	else {
		targetEnemy_ = nullptr;
		lockTargets_.clear();
		lockState_ = LOCK_STATE::NONE;
		lockTimer_ = 0;
	}

	// 5. 色の更新
	UpdateSiteStyle();

	// 6. SEの再生管理
	if (prevLockState != lockState_)
	{
		if (lockState_ == LOCK_STATE::LOCKING)
		{
			AudioManager::GetInstance()->PlaySE(SoundID::SE_LOCKING);
		}
		else if (lockState_ == LOCK_STATE::LOCKED)
		{
			AudioManager::GetInstance()->PlaySE(SoundID::SE_LOCKON);
		}
	}

	if (lockTargets_.size() > prevLockCount && lockTargets_.size() > 1)
	{
		AudioManager::GetInstance()->PlaySE(SoundID::SE_LOCKON);
	}
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
