
#include "../../Utility/AsoUtility.h"
#include"../../Manager/SceneManager.h"
#include "../../Object/Player.h"
#include "EnemyBase.h"

EnemyBase::EnemyBase(const EnemyBase::EnemyData& data)
	:
	CharactorBase(),
	type_(data.type),
	hp_(data.hp),
	stateBase_(-1),
	defaultPos_(data.defaultPos),
	movableRange_(data.movableRange),
	searchRadius_(data.searchRadius)
{
	// 初期座標の設定
	transform_.pos = data.defaultPos;
}
EnemyBase::~EnemyBase(void)
{
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
	int drawY = 240 + (static_cast<int>(type_) * 20);
	DrawFormatString(0, drawY, GetColor(255, 255, 0),
		"Enemy Pos: X=%.1f Y=%.1f Z=%.1f",
		transform_.pos.x, transform_.pos.y, transform_.pos.z);
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