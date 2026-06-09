#include <DxLib.h>
#include "../Application.h"
#include "Resource.h"
#include "ResourceManager.h"

ResourceManager* ResourceManager::instance_ = nullptr;

void ResourceManager::CreateInstance(void)
{
	if (instance_ == nullptr)
	{
		instance_ = new ResourceManager();
	}
	instance_->Init();
}

ResourceManager& ResourceManager::GetInstance(void)
{
	return *instance_;
}

void ResourceManager::Init(void)
{

	// 推奨しませんが、どうしても使いたい方は
	using RES = Resource;
	using RES_T = RES::TYPE;
	static std::string PATH_IMG = Application::PATH_IMAGE;
	static std::string PATH_MDL = Application::PATH_MODEL;
	static std::string PATH_EFF = Application::PATH_EFFECT;

	Resource* res;

	// タイトル画像
	res = new RES(RES_T::IMG, PATH_IMG + "Title.png");
	resourcesMap_.emplace(SRC::TITLE, res);

	// コントローラーの画像
	res = new RES(RES_T::IMG, PATH_IMG + "Pad.png");
	resourcesMap_.emplace(SRC::PAD, res);

	// 操作説明1
	res = new RES(RES_T::IMG, PATH_IMG + "P1.png");
	resourcesMap_.emplace(SRC::P1, res);

	// 操作説明2
	res = new RES(RES_T::IMG, PATH_IMG + "P2.png");
	resourcesMap_.emplace(SRC::P2, res);
	
	// プレイヤー
	res = new RES(RES_T::MODEL, PATH_MDL + "Player/NBmv1/ナインボール.mv1");
	resourcesMap_.emplace(SRC::PLAYER, res);

	// プレイヤー影
	res = new RES(RES_T::IMG, PATH_IMG + "Shadow.png");
	resourcesMap_.emplace(SRC::PLAYER_SHADOW, res);

	// HUBEN
	res = new RES(RES_T::MODEL, PATH_MDL + "Wepon/HUBEN/HUBEN.mv1");
	resourcesMap_.emplace(SRC::WEPON, res);

	// 敵MT1
	res = new RES(RES_T::MODEL, PATH_MDL + "Enemy/MT/MT08-OSTRICH.mv1");
	resourcesMap_.emplace(SRC::ENEMY_MT1, res);

	// ミサイルMT
	res = new RES(RES_T::MODEL, PATH_MDL + "Enemy/MT/MT08M-OSTRICH.mv1");
	resourcesMap_.emplace(SRC::MISSILE_MT, res);

	// スカイドーム
	res = new RES(RES_T::MODEL, PATH_MDL + "SkyDome/SkyDome.mv1");
	resourcesMap_.emplace(SRC::SKY_DOME, res);

	// ステージ
	res = new RES(RES_T::MODEL, PATH_MDL + "Stage/テスト部屋/初代テスト部屋.mv1");
	resourcesMap_.emplace(SRC::STAGE, res);

}

void ResourceManager::Release(void)
{
	for (auto& p : loadedMap_)
	{
		p.second.Release();
	}

	loadedMap_.clear();
}

void ResourceManager::Destroy(void)
{
	Release();
	for (auto& res : resourcesMap_)
	{
		res.second->Release();
		delete res.second;
	}
	resourcesMap_.clear();
	delete instance_;
}

const Resource& ResourceManager::Load(SRC src)
{
	Resource& res = _Load(src);
	if (res.type_ == Resource::TYPE::NONE)
	{
		return dummy_;
	}
	return res;
}

int ResourceManager::LoadModelDuplicate(SRC src)
{
	Resource& res = _Load(src);
	if (res.type_ == Resource::TYPE::NONE)
	{
		return -1;
	}

	int duId = MV1DuplicateModel(res.handleId_);
	res.duplicateModelIds_.push_back(duId);

	return duId;
}

ResourceManager::ResourceManager(void)
{
}

Resource& ResourceManager::_Load(SRC src)
{

	// ロード済みチェック
	const auto& lPair = loadedMap_.find(src);
	if (lPair != loadedMap_.end())
	{
		return *resourcesMap_.find(src)->second;
	}

	// リソース登録チェック
	const auto& rPair = resourcesMap_.find(src);
	if (rPair == resourcesMap_.end())
	{
		// 登録されていない
		return dummy_;
	}

	// ロード処理
	rPair->second->Load();

	// 念のためコピーコンストラクタ
	loadedMap_.emplace(src, *rPair->second);

	return *rPair->second;

}
