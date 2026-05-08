
#include "../../Manager/ResourceManager.h"
#include "../../Utility/AsoUtility.h"
#include "../../Manager/SceneManager.h"
#include "SkyDome.h"

SkyDome::SkyDome(const Transform& followTransform)
	:
	ActorBase(),
	followTransform_(followTransform),
	state_(STATE::NONE)
{
}

SkyDome::~SkyDome(void)
{
}

void SkyDome::Update(void)
{
	switch (state_)
	{
	case STATE::NONE:
		UpdateNone();
		break;
	case STATE::STAY:
		UpdateStay();
		break;
	case STATE::FOLLOW:
		UpdateFollow();
		break;
	}
}

void SkyDome::Draw(void)
{
	SetUseLighting(FALSE);
	MV1DrawModel(transform_.modelId);
	SetUseLighting(TRUE);
}

void SkyDome::InitLoad(void)
{
	// モデルの読み込み
	transform_.SetModel(
		resMng_.Load(ResourceManager::SRC::SKY_DOME).handleId_);
}

void SkyDome::InitTransform(void)
{

	// モデルの基本設定
	transform_.scl = SCALES;
	transform_.quaRot = Quaternion::Identity();
	transform_.quaRotLocal = Quaternion::Euler(DEFAULT_ROT_LOCAL);
	transform_.pos = AsoUtility::VECTOR_ZERO;
	transform_.Update();
}

void SkyDome::InitCollider(void)
{
}

void SkyDome::InitAnimation(void)
{
}

void SkyDome::InitPost(void)
{
	// Zバッファ無効(突き抜け対策)
	MV1SetUseZBuffer(transform_.modelId, false);
	MV1SetWriteZBuffer(transform_.modelId, false);

	// 初期状態設定
	SceneManager::SCENE_ID sceneId = scnMng_.GetSceneID();
	if (sceneId == SceneManager::SCENE_ID::GAME)
	{
		ChangeState(STATE::FOLLOW);
	}
	else
	{
		ChangeState(STATE::STAY);
	}

}

void SkyDome::ChangeState(STATE state)
{
	// 状態変更
	state_ = state;

	// 状態ごとの初期化処理
	switch (state_)
	{
	case STATE::NONE:
		ChangeStateNone();
		break;
	case STATE::STAY:
		ChangeStateStay();
		break;
	case STATE::FOLLOW:
		ChangeStateFollow();
		break;
	}
}

void SkyDome::ChangeStateNone(void)
{
}

void SkyDome::ChangeStateStay(void)
{
}

void SkyDome::ChangeStateFollow(void)
{
	// 追従開始
	transform_.pos = followTransform_.pos;
	transform_.Update();
}

void SkyDome::UpdateNone(void)
{
}

void SkyDome::UpdateStay(void)
{
	// モデルのY軸回転
	Quaternion rot = Quaternion::AngleAxis(
		AsoUtility::Deg2RadF(0.1f), AsoUtility::AXIS_Y);
	transform_.quaRot = transform_.quaRot.Mult(rot);
	transform_.Update();
}

void SkyDome::UpdateFollow(void)
{
	// モデルのY軸回転
	Quaternion rot = Quaternion::AngleAxis(
		AsoUtility::Deg2RadF(0.1f), AsoUtility::AXIS_Y);
	transform_.quaRot = transform_.quaRot.Mult(rot);

	//追従
	transform_.pos = followTransform_.pos;

	transform_.Update();
}
