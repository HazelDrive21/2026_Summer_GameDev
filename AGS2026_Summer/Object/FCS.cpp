#include "Fcs.h"
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
	maxLockRange_ = 5000.0f;       // 機体に合わせて調整する射程距離
	requiredLockFrame_ = 30;       // ロックオンに必要な時間（30フレーム = 0.5秒）
}

void FCS::Update(const VECTOR& myPos, const std::vector<EnemyBase*>& enemies)
{
	// 1. サイトサイズの補間（既存の処理）
	siteWidth_ += (targetWidth_ - siteWidth_) * RESIZE_SPEED;
	siteHeight_ += (targetHeight_ - siteHeight_) * RESIZE_SPEED;

	// 2. 現在のターゲットが有効かチェック（見失い判定 ＆ ★安全対策）
	if (targetEnemy_ != nullptr) {
		// 渡された敵リストの中に、現在のターゲットがまだ存在しているか探す
		bool isExist = false;
		for (auto enemy : enemies) {
			if (enemy == targetEnemy_) {
				isExist = true;
				break;
			}
		}

		// リストから消えている（死んだ）、または距離が離れすぎたらロック解除
		float dist = isExist ? VSize(VSub(targetEnemy_->GetTransform().pos, myPos)) : FLT_MAX;
		if (!isExist || dist > maxLockRange_) {
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

	if (targetEnemy_ != nullptr)
	{
		// 敵の3D座標を画面の2D座標に変換
		VECTOR enemy2D = ConvWorldPosToScreenPos(targetEnemy_->GetTransform().pos);

		// カメラの画面内（前方）にある場合のみ描画
		if (enemy2D.z >= 0.0f && enemy2D.z <= 1.0f)
		{
			int markerSize = 16; // マークのサイズ

			if (lockState_ == LOCK_STATE::LOCKING)
			{
				// ロックオン進行中：緑色の少し細い枠
				DrawBox(
					static_cast<int>(enemy2D.x - markerSize), static_cast<int>(enemy2D.y - markerSize),
					static_cast<int>(enemy2D.x + markerSize), static_cast<int>(enemy2D.y + markerSize),
					GetColor(0, 255, 0), FALSE
				);
			}
			else if (lockState_ == LOCK_STATE::LOCKED)
			{
				// ロックオン完了：赤色の太い枠（2回重ねて太く見せるなど）
				DrawBox(
					static_cast<int>(enemy2D.x - markerSize), static_cast<int>(enemy2D.y - markerSize),
					static_cast<int>(enemy2D.x + markerSize), static_cast<int>(enemy2D.y + markerSize),
					GetColor(255, 0, 0), FALSE
				);
				DrawBox(
					static_cast<int>(enemy2D.x - markerSize - 1), static_cast<int>(enemy2D.y - markerSize - 1),
					static_cast<int>(enemy2D.x + markerSize + 1), static_cast<int>(enemy2D.y + markerSize + 1),
					GetColor(255, 0, 0), FALSE
				);
			}
		}
	}

	if (lockState_ == LOCK_STATE::LOCKED && player_ != nullptr)
	{
		// 仮の弾速（例：1フレームに 30.0f 進むライフルを想定）
		float virtualBulletSpeed = 100.0f;

		// プレイヤーの座標は player_->GetTransform().pos で取得できると仮定
		VECTOR myPos = player_->GetTransform().pos;

		// 未来予測位置を計算
		VECTOR predPos3D = CalcPredictivePos(virtualBulletSpeed, myPos);

		// 3Dの予測位置を画面の2D座標に変換
		VECTOR predPos2D = ConvWorldPosToScreenPos(predPos3D);

		// 画面内なら青い＋マークを描画
		if (predPos2D.z >= 0.0f && predPos2D.z <= 1.0f)
		{
			int len = 8; // プラス線の長さ
			DrawLine(static_cast<int>(predPos2D.x - len), static_cast<int>(predPos2D.y),
				static_cast<int>(predPos2D.x + len), static_cast<int>(predPos2D.y), GetColor(0, 128, 255), 2);
			DrawLine(static_cast<int>(predPos2D.x), static_cast<int>(predPos2D.y - len),
				static_cast<int>(predPos2D.x), static_cast<int>(predPos2D.y + len), GetColor(0, 128, 255), 2);

			// 文字で「PREDICT」と添えるとさらにデバッグしやすいです
			DrawString(static_cast<int>(predPos2D.x + 10), static_cast<int>(predPos2D.y - 5), "PREDICT", GetColor(0, 128, 255));
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

VECTOR FCS::CalcPredictivePos(float bulletSpeed, const VECTOR& myPos) const
{
	// ターゲットがいない、または弾速が0以下の場合は計算できないので安全対策
	if (targetEnemy_ == nullptr || bulletSpeed <= 0.0f)
	{
		return (targetEnemy_ != nullptr) ? targetEnemy_->GetTransform().pos : AsoUtility::VECTOR_ZERO;
	}

	// 1. 敵の「現在の座標」を取得（デフォルトは足元の座標）
	VECTOR enemyPos = targetEnemy_->GetTransform().pos;

	// ────────────────────────────────────────────────────────────────
	// ★★★【追加】敵のカプセルコライダの中心（胴体中心）を取得して基準にする ★★★
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
	// ────────────────────────────────────────────────────────────────

	// 2. 敵の「速度ベクトル」を取得
	VECTOR enemyVel = targetEnemy_->GetVelocity();

	// 3. 自分と敵の現在の距離を計算（胴体中心からの距離になります）
	float dist = VSize(VSub(enemyPos, myPos));

	// 4. 弾が敵に届くまでにかかる時間(フレーム数)を計算
	float time = dist / bulletSpeed;

	// 5. 未来の予測位置を計算 (予測位置 ＝ 現在の胴体位置 ＋ 速度 × 時間)
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