
#include <string>
#include <fstream>
#include "../../Application.h"
#include "../../Utility/AsoUtility.h"
#include "EnemyMT.h"
#include "EnemyMissileMT.h"
#include "EnemyManager.h"

static EnemyManager* s_instance = nullptr;

EnemyManager::EnemyManager(void)
{
	s_instance = this; // ⚡【追加】生成された実体をここに登録する
}

EnemyManager::~EnemyManager(void)
{
	if (s_instance == this)
	{
		s_instance = nullptr; // ⚡【追加】破棄されたらクリア
	}
}

// ⚡【追加】GetInstanceの実装
EnemyManager* EnemyManager::GetInstance(void)
{
	return s_instance;
}

void EnemyManager::Init(void)
{
	// エネミーのデータ読み込み
	LoadCsvData();


}

void EnemyManager::Update(void)
{
	// イテレータを使って、要素の更新と削除を安全に行うループ
	for (auto it = enemies_.begin(); it != enemies_.end(); )
	{
		// すでに死亡（HPが0以下）しているエネミーを見つけたら
		if ((*it)->GetHp() <= 0)
		{
			(*it)->Release(); // 3Dモデルなどのアセット解放（必要であれば）
			delete* it;       // インスタンスのメモリを解放
			it = enemies_.erase(it); // ベクターからポインタを除外（自動的に次の要素を指す）
		}
		else
		{
			// 生きているエネミーのみ通常更新
			(*it)->Update();
			++it; // 次の要素へ進める
		}
	}
}

void EnemyManager::Draw(void)
{
	for (auto& enemy : enemies_)
	{
		enemy->Draw();
	}
}

void EnemyManager::Release(void)
{
	for (auto& enemy : enemies_)
	{
		enemy->Release();
		delete enemy;
	}
}

void EnemyManager::AddHitCollider(const ColliderBase* hitCollider)
{
	for (auto& enemy : enemies_)
	{
		enemy->AddHitCollider(hitCollider);

	}
}

void EnemyManager::LoadCsvData(void)
{
	// ファイルの読込
	std::ifstream ifs = std::ifstream(Application::PATH_CSV + "EnemyData.csv");
	if (!ifs)
	{
		// エラーが発生
		return;
	}
	// ファイルを１行ずつ読み込む
	std::string line;// 1行の文字情報
	std::vector<std::string> strSplit; // 1行を1文字の動的配列に分割
	bool isHeader = true;
	while (getline(ifs, line))
	{
		if (isHeader)
		{
			isHeader = false;
			continue;
		}
		// １行をカンマ区切りで分割
		strSplit = AsoUtility::Split(line, ',');
		EnemyBase* enemy = nullptr;
		// 構造体に合わせて読込データを格納
		EnemyBase::EnemyData data = EnemyBase::EnemyData();
		int idx = 0;
		// ID
		data.id = stoi(strSplit[idx++]);
		// 種別
		data.type = static_cast<EnemyBase::TYPE>(stoi(strSplit[idx++]));
		// HP
		data.hp = stoi(strSplit[idx++]);
		// 初期座標
		data.defaultPos =
		{
		stof(strSplit[idx++]),
		stof(strSplit[idx++]),
		stof(strSplit[idx++])
		};
		// 移動可能範囲
		data.movableRange = stof(strSplit[idx++]);
		// 探索範囲
		data.searchRadius = stof(strSplit[idx++]);
		// エネミー生成
		Create(data);
	}
	ifs.close();
}

EnemyBase* EnemyManager::Create(const EnemyBase::EnemyData& data)
{
	EnemyBase* enemy = nullptr;

	switch (data.type)
	{
	case EnemyBase::TYPE::MT1:
		enemy = new EnemyMT(data);
		break;
	case EnemyBase::TYPE::MISSILE_MT:
		enemy = new EnemyMissileMT(data);
		break;
	default:
		break;
	}

	if (enemy != nullptr)
	{
		enemy->Init();
		enemies_.emplace_back(enemy);
	}
	return enemy;
}
