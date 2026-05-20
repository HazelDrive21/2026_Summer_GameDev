#include "../Common/Transform.h"
#include "../../Object/Collider/ColliderModel.h"
#include "../../Utility/AsoUtility.h"
#include "ColliderLine.h"
ColliderLine::ColliderLine(
	TAG tag, const Transform* follow,
	const VECTOR& localPosStart, const VECTOR& localPosEnd)
	:
	ColliderBase(SHAPE::LINE, tag, follow),
	localPosStart_(localPosStart),
	localPosEnd_(localPosEnd)
{
}
ColliderLine::~ColliderLine(void)
{
}
void ColliderLine::SetLocalPosStart(const VECTOR& pos)
{
	localPosStart_ = pos;
}
void ColliderLine::SetLocalPosEnd(const VECTOR& pos)
{
	localPosEnd_ = pos;
}
const VECTOR& ColliderLine::GetLocalPosStart(void) const
{
	return localPosStart_;
}
const VECTOR& ColliderLine::GetLocalPosEnd(void) const
{
	return localPosEnd_;
}
VECTOR ColliderLine::GetPosStart(void) const
{
	return GetRotPos(localPosStart_);
}
VECTOR ColliderLine::GetPosEnd(void) const
{
	return GetRotPos(localPosEnd_);
}

bool ColliderLine::PushBackUp(const ColliderModel* colliderModel, Transform& transform, float pushDistance, bool isExclude, bool isTarget) const
{
    bool ret = false;

    // ステージモデル(地面)との衝突
    auto hits = MV1CollCheck_LineDim(
        colliderModel->GetFollow()->modelId, -1, GetPosStart(), GetPosEnd());

    for (int i = 0; i < hits.HitNum; i++)
    {
        auto hit = hits.Dim[i];
        // （除外・対象フレームのチェック処理は省略）

        // 線分のローカル終端のY座標の絶対値（例：-40.0f なら 40.0f ＝ 原点から足元までの距離）
        // ※ localPosEnd_.y を用いて動的に取得するのが理想です
        float offsetToFeet = fabsf(localPosEnd_.y);

        // キャラクターの足元が地面よりも下に沈み込んでいるか判定
        //（現在の位置 + 重力適用後、足元が地面より低ければ）
        if ((transform.pos.y - offsetToFeet) < hit.HitPosition.y)
        {
            // 固定値で跳ね上げるのではなく、地面の高さに足元から原点までの距離をぴったり合わせる
            transform.pos.y = hit.HitPosition.y + offsetToFeet + pushDistance;

            ret = true;
        }
    }

    // 検出した地面ポリゴン情報の後始末
    MV1CollResultPolyDimTerminate(hits);

    return ret;
}

void ColliderLine::DrawDebug(int color)
{
	VECTOR s = GetPosStart();
	VECTOR e = GetPosEnd();
	// 線分を描画
	DrawLine3D(s, e, color);
	// 始点・終点を球体で補助表示
	DrawSphere3D(s, RADIUS, DIV_NUM, color, color, true);
	DrawSphere3D(e, RADIUS, DIV_NUM, color, color, true);
}