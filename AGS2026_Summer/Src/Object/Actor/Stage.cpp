
#include "../../Manager/ResourceManager.h"
#include "../../Utility/AsoUtility.h"
#include "../Collider/ColliderModel.h"
#include "Stage.h"

Stage::Stage(void)
	:
	ActorBase()
{
}

Stage::~Stage(void)
{
}

void Stage::Update(void)
{
}

void Stage::InitLoad(void)
{

	// モデルの読み込み
	transform_.SetModel(
		resMng_.Load(ResourceManager::SRC::STAGE).handleId_);

}

void Stage::InitTransform(void)
{
	// モデルの基本設定
	transform_.scl = AsoUtility::VECTOR_ONE;
	transform_.quaRot = Quaternion::Identity();
	transform_.quaRotLocal = Quaternion::Identity();
	transform_.pos = DEFALT_POS;
	transform_.Update();
}

void Stage::InitCollider(void)
{
	// DxLib側の衝突情報セットアップ
	MV1SetupCollInfo(transform_.modelId);
	// モデルのコライダ
	ColliderModel* colModel =
		new ColliderModel(ColliderBase::TAG::STAGE, &transform_);
	for (const std::string& name : TARGET_FRAME_NAMES)
	{
		colModel->AddTargetFrameIds(name);
	}
	ownColliders_.emplace(static_cast<int>(COLLIDER_TYPE::MODEL), colModel);
}


void Stage::InitAnimation(void)
{
}

void Stage::InitPost(void)
{
}
