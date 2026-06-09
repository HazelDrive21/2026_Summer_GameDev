
#include "../../Utility/AsoUtility.h"
#include"../../Manager/SceneManager.h"
#include "../../Object/Player.h"
#include "../../Object/Collider/ColliderSphere.h"
#include "../../Object/Collider/ColliderCapsule.h"
#include "EnemyBase.h"

EnemyBase::EnemyBase(const EnemyBase::EnemyData& data)
	:
	CharactorBase(),
	type_(data.type),
	hp_(data.hp),
	stateBase_(-1),
	defaultPos_(data.defaultPos),
	movableRange_(data.movableRange),
	searchRadius_(data.searchRadius),
	weapon_(nullptr), 
	localMuzzlePos_(VGet(0.0f, 0.0f, 0.0f))
{
	// 初期座標の設定
	transform_.pos = data.defaultPos;
	velocity_ = AsoUtility::VECTOR_ZERO;
	prevPos_ = data.defaultPos;
}
EnemyBase::~EnemyBase(void)
{
	if (weapon_ != nullptr)
	{
		delete weapon_;
		weapon_ = nullptr;
	}
}

void EnemyBase::Update(void)
{
	// 1. 基底クラス（CharactorBase）の本来の更新処理（移動やAIの実行）を行う
	CharactorBase::Update();

	// 2. 移動した結果の「現在の座標」と「1フレーム前の座標」の差分から速度ベクトルを計算
	// 速度 ＝ 今の座標 － 過去の座標
	velocity_ = VSub(transform_.pos, prevPos_);

	// 3. 次のフレームのために、現在の座標を「1フレーム前の座標」として保存する
	prevPos_ = transform_.pos;

	if (weapon_ != nullptr)
	{
		weapon_->Update();
	}
}

void EnemyBase::Draw(void)
{
	CharactorBase::Draw();

#ifdef _DEBUG
	// 移動可能範囲のデバッグ描画
	DrawSphere3D(defaultPos_, movableRange_, 16, 0x000099, 0x000099, false);
	// 探索範囲のデバッグ描画
	DrawSphere3D(defaultPos_, searchRadius_, 16, 0x990000, 0x990000, false);

	DrawDebugAxes();

	// 画面の座標を動的にずらして表示（エネミーIDごとにY座標を+20する）
	// ※ 厳密にはIDなどを使って描画位置を管理する必要があります
	int drawY = 280 + (static_cast<int>(type_) * 20);
	DrawFormatString(0, drawY, GetColor(255, 255, 0),
		"Enemy HP:%d Pos: X=%.1f Y=%.1f Z=%.1f", // ← HP:%d を追加
		hp_, transform_.pos.x, transform_.pos.y, transform_.pos.z);
#endif // _DEBUG
}

void EnemyBase::DrawDebugAxes(void) const
{
	float length = 200.0f; // 線の長さ（見やすいように調整してください）
	VECTOR pos = transform_.pos;

	// 現在の回転(quaRot)を使って各方向ベクトルを回転させる
	// X軸(右): Red, Y軸(上): Green, Z軸(前): Blue
	VECTOR right = transform_.quaRot.PosAxis(VGet(1, 0, 0));
	VECTOR up = transform_.quaRot.PosAxis(VGet(0, 1, 0));
	VECTOR forward = transform_.quaRot.PosAxis(VGet(0, 0, 1));

	// 描画
	DrawLine3D(pos, VAdd(pos, VScale(right, length)), GetColor(255, 0, 0));
	DrawLine3D(pos, VAdd(pos, VScale(up, length)), GetColor(0, 255, 0));
	DrawLine3D(pos, VAdd(pos, VScale(forward, length)), GetColor(0, 0, 255));

	Player* player = SceneManager::GetInstance().GetPlayer();
	if (player) {
		VECTOR toPlayer = VSub(player->GetTransform().pos, transform_.pos);
		DrawLine3D(transform_.pos, VAdd(transform_.pos, toPlayer), GetColor(255, 255, 0));
	}
}

void EnemyBase::ChangeState(int state)
{
	stateBase_ = state;

	// 各状態遷移の初期処理
	stateChanges_[stateBase_]();
}

bool EnemyBase::InMovableRange(void) const
{
	bool ret = false;
	// 初期位置からの距離
	float dis = static_cast<float>(
		AsoUtility::SqrMagnitude(defaultPos_, transform_.pos));
	// 指定距離判定
	if (dis < movableRange_ * movableRange_)
	{
		return true;
	}
	return ret;
}

void EnemyBase::ApplyDamage(int damage)
{
	// すでに死亡している場合は重ねて処理しない
	if (hp_ <= 0) return;

	hp_ -= damage;

	// マイナスにならないようにクランプ
	if (hp_ < 0)
	{
		hp_ = 0;
		// 必要であれば、ここに死亡状態（STATE::DEADなど）への遷移を書く
	}
}

bool EnemyBase::CheckHitBullet(const VECTOR& bulletPrevPos, const VECTOR& bulletPos, float bulletRadius, int damage)
{
	if (hp_ <= 0) return false;

	// CharactorBase等で定義されているコライダの取得
	int capsuleKey = static_cast<int>(CharactorBase::COLLIDER_TYPE::CAPSULE);
	const auto& ownColliders = GetOwnColliders();

	if (ownColliders.count(capsuleKey) > 0)
	{
		auto* baseCollider = ownColliders.at(capsuleKey);
		if (baseCollider != nullptr && baseCollider->IsValid()) // 有効フラグもチェック
		{
			// ⚡ ご提示いただいた ColliderCapsule クラスへ安全にキャスト
			auto* capsuleCollider = dynamic_cast<ColliderCapsule*>(baseCollider);
			if (capsuleCollider != nullptr)
			{
				// ⚡ 実際のゲッター名（GetPosTop, GetPosDown, GetRadius）を適用
				VECTOR charTop = capsuleCollider->GetPosTop();   // カプセル上部球体のワールド座標
				VECTOR charDown = capsuleCollider->GetPosDown();  // カプセル下部球体のワールド座標
				float charRadius = capsuleCollider->GetRadius();   // キャラクターの判定半径

				// ⚡ カプセル vs カプセル (HitCheckCapsuleCapsule) で超高速弾のすり抜けを完全に防ぐ
				// 引数: カプセル1の線分両端・半径、カプセル2の線分両端・半径
				if (HitCheck_Capsule_Capsule(bulletPrevPos, bulletPos, bulletRadius,
					charTop, charDown, charRadius) == 1)
				{
					// 被弾処理 (ダメージ適用など)
					ApplyDamage(damage);
					return true; // 衝突した
				}
			}
		}
	}
	return false; // 衝突しなかった
}