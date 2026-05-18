
#include "../Utility/AsoUtility.h"
#include "../Application.h"
#include "../Manager/SceneManager.h"
#include "../Manager/ResourceManager.h"
#include "Common/AnimationController.h"
#include "Collider/ColliderLine.h"
#include "Collider/ColliderModel.h"
#include "Collider/ColliderCapsule.h"
#include "Stage.h"
#include "Player.h"
#include "CharactorBase.h"

CharactorBase::CharactorBase(void)
	:
	ActorBase(),
	animationController_(nullptr),
	moveDir_(AsoUtility::VECTOR_ZERO),
	moveSpeed_(0.0f),
	movePow_(AsoUtility::VECTOR_ZERO),
	isJump_(false),
	imgShadow_(-1),
	stepJump_(0.0f)
{
}

CharactorBase::~CharactorBase(void)
{
}

void CharactorBase::Update(void)
{
	// 移動前座標を更新
	prevPos_ = transform_.pos;

	// 各キャラクターごとの更新処理
	UpdateProcess();
	// 移動方向に応じた遅延回転
	DelayRotate();

	// 重力による移動量
	CalcGravityPow();

	// 衝突判定前準備
	CollisionReserve();

	// 衝突判定
	Collision();

	// モデル制御更新
	transform_.Update();

	// アニメーション再生
	animationController_->Update();

	// 各キャラクターごとの更新後処理
	UpdateProcessPost();

}

void CharactorBase::Draw(void)
{

	// 基底クラスの描画処理
	ActorBase::Draw();
	// 丸影の描画
	DrawShadow();

}

void CharactorBase::InitLoad(void)
{
	// 丸影画像
	imgShadow_ = resMng_.Load(ResourceManager::SRC::PLAYER_SHADOW).handleId_;
}

void CharactorBase::DrawShadow(void)
{
	float PLAYER_SHADOW_HEIGHT = 800.0f;
	float PLAYER_SHADOW_SIZE = 30.0f;

	// 影画像が読み込まれていない場合は処理しない
	if (imgShadow_ == -1) return;

	// ライティングを無効にする
	SetUseLighting(FALSE);
	// Zバッファを有効にする
	SetUseZBuffer3D(TRUE);
	// テクスチャアドレスモードをCLAMPにする
	SetTextureAddressMode(DX_TEXADDRESS_CLAMP);

	// 頂点データで変化が無い部分をセット
	VERTEX3D Vertex[3];
	Vertex[0].dif = GetColorU8(255, 255, 255, 255);
	Vertex[0].spc = GetColorU8(0, 0, 0, 0);
	Vertex[0].su = 0.0f;
	Vertex[0].sv = 0.0f;
	Vertex[1] = Vertex[0];
	Vertex[2] = Vertex[0];

	// 登録されている衝突物を全てチェック
	for (const auto& hitCol : hitColliders_)
	{
		// ステージ以外は処理を飛ばす
		if (hitCol->GetTag() != ColliderBase::TAG::STAGE) continue;

		// 派生クラスへキャスト
		const ColliderModel* colliderModel =
			dynamic_cast<const ColliderModel*>(hitCol);
		if (colliderModel == nullptr) continue;

		// キャラクターの直下に存在する地面のポリゴンを取得
		VECTOR startPos = transform_.pos;
		VECTOR endPos = VAdd(transform_.pos, VGet(0.0f, -PLAYER_SHADOW_HEIGHT, 0.0f));

		auto HitResDim = MV1CollCheck_Capsule(
			colliderModel->GetFollow()->modelId, -1,
			startPos, endPos, PLAYER_SHADOW_SIZE);

		// 直下に存在するポリゴンの数だけ繰り返し
		for (int i = 0; i < HitResDim.HitNum; i++)
		{
			auto HitRes = HitResDim.Dim[i];

			// 除外フレームは無視する
			if (colliderModel->IsExcludeFrame(HitRes.FrameIndex))
			{
				continue;
			}

			// ポリゴンの座標は地面ポリゴンの座標
			Vertex[0].pos = HitRes.Position[0];
			Vertex[1].pos = HitRes.Position[1];
			Vertex[2].pos = HitRes.Position[2];

			// ちょっと持ち上げて重ならないようにする
			VECTOR SlideVec = VScale(HitRes.Normal, 0.5f);
			Vertex[0].pos = VAdd(Vertex[0].pos, SlideVec);
			Vertex[1].pos = VAdd(Vertex[1].pos, SlideVec);
			Vertex[2].pos = VAdd(Vertex[2].pos, SlideVec);

			// ポリゴンの不透明度を設定する(固定値)
			Vertex[0].dif.a = 128;
			Vertex[1].dif.a = 128;
			Vertex[2].dif.a = 128;

			// UV値は地面ポリゴンとキャラクターの相対座標から割り出す
			Vertex[0].u = (HitRes.Position[0].x - transform_.pos.x) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[0].v = (HitRes.Position[0].z - transform_.pos.z) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[1].u = (HitRes.Position[1].x - transform_.pos.x) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[1].v = (HitRes.Position[1].z - transform_.pos.z) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[2].u = (HitRes.Position[2].x - transform_.pos.x) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[2].v = (HitRes.Position[2].z - transform_.pos.z) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;

			// 影ポリゴンを描画
			DrawPolygon3D(Vertex, 1, imgShadow_, TRUE);
		}

		// 検出した地面ポリゴン情報の後始末
		MV1CollResultPolyDimTerminate(HitResDim);
	}

	// ライティングを有効にする
	SetUseLighting(FALSE);
	// Zバッファを無効にする
	SetUseZBuffer3D(FALSE);
}

void CharactorBase::Release(void)
{
	if (animationController_ != nullptr)
	{
		animationController_->Release();
		delete animationController_;
	}
	ActorBase::Release();
}

void CharactorBase::DelayRotate(void)
{
	// 移動方向から回転に変換する
	Quaternion goalRot = Quaternion::LookRotation(moveDir_);

	// 回転の補間
	transform_.quaRot =
		Quaternion::Slerp(transform_.quaRot, goalRot, 0.2f);
}

void CharactorBase::CalcGravityPow(void)
{
	// 重力方向
	VECTOR dirGravity = AsoUtility::DIR_D;
	// 重力の強さ
	float gravityPow = Application::GetInstance().GetGravityPow() * scnMng_.GetDeltaTime();
	// 重力
	VECTOR gravity = VScale(dirGravity, gravityPow);
	jumpPow_ = VAdd(jumpPow_, gravity);

	if (jumpPow_.y < MAX_FALL_SPEED)
	{
		jumpPow_.y = MAX_FALL_SPEED;
	}

}

void CharactorBase::Collision(void)
{
	// 移動処理
	transform_.pos = VAdd(transform_.pos, movePow_);
	// 衝突(カプセル)
	CollisionCapsule();
	// ジャンプ量を加算
	transform_.pos = VAdd(transform_.pos, jumpPow_);
	// 衝突(重力)
	CollisionGravity();
}
void CharactorBase::CollisionGravity(void)
{
	// 落下中しか判定しない
	if (!(VDot(AsoUtility::DIR_D, jumpPow_) > 0.9f))
	{
		return;
	}

	// 線分コライダ
	int lineType = static_cast<int>(COLLIDER_TYPE::LINE);
	// 線分コライダが無ければ処理を抜ける
	if (ownColliders_.count(lineType) == 0) return;
	// 線分コライダ情報
	ColliderLine* colliderLine_ =
		dynamic_cast<ColliderLine*>(ownColliders_.at(lineType));
	if (colliderLine_ == nullptr) return;

	// 登録されている衝突物を全てチェック
	for (const auto& hitCol : hitColliders_)
	{
		// ステージ以外は処理を飛ばす
		if (hitCol->GetTag() != ColliderBase::TAG::STAGE) continue;

		// 派生クラスへキャスト
		const ColliderModel* colliderModel =
			dynamic_cast<const ColliderModel*>(hitCol);

		if (colliderModel == nullptr) continue;

		// 衝突したポリゴンの上に押し戻す
		bool isHit = colliderLine_->PushBackUp(
			colliderModel, transform_, 2.0f, true, false);

		if (isHit)
		{
			// ジャンプ判定
			isJump_ = false;
		}
	}
	if (!isJump_)
	{
		// ジャンプリセット
		jumpPow_ = AsoUtility::VECTOR_ZERO;
		// ジャンプの入力受付時間をリセット
		stepJump_ = 0.0f;
	}
}


void CharactorBase::CollisionCapsule(void)
{

	// カプセルコライダ
	int capsuleType = static_cast<int>(COLLIDER_TYPE::CAPSULE);

	// カプセルコライダが無ければ処理を抜ける
	if (ownColliders_.count(capsuleType) == 0) return;

	// カプセルコライダ情報
	ColliderCapsule* colliderCapsule =
		dynamic_cast<ColliderCapsule*>(ownColliders_.at(capsuleType));

	if (colliderCapsule == nullptr) return;

	// 登録されている衝突物を全てチェック
	for (const auto& hitCol : hitColliders_)
	{
		// モデル以外は処理を飛ばす
		if (hitCol->GetShape() != ColliderBase::SHAPE::MODEL) continue;

		// 派生クラスへキャスト
		const ColliderModel* colliderModel =
			dynamic_cast<const ColliderModel*>(hitCol);

		if (colliderModel == nullptr) continue;

		// 指定された回数と距離で三角形の法線方向に押し戻す
		colliderCapsule->PushBackAlongNormal(
			colliderModel, transform_,
			CNT_TRY_COLLISION, COLLISION_BACK_DIS,
			true, false);
	}

}
