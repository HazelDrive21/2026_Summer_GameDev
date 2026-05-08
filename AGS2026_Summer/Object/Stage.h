#pragma once
#include <map>
#include <vector>
#include "Common/Transform.h"
class ResourceManager;
class Player;

struct StageData {
	int modelHandle;        // 地形モデル
	Transform transform;     // 位置・回転・スケール・当たり判定
	// 必要なら、そのステージ独自のBGMや背景画像などの情報
};

class Stage
{

public:

	// ステージ識別用の名前を定義
	enum class NAME {
		MAIN_BASE,
		DESERT,       // 砂漠
		STATION,      // 基地
		// 必要に応じて追加
	};

	// コンストラクタ
	Stage(Player* player);

	// デストラクタ
	~Stage(void);

	void Init(void);
	void Update(void);
	void Draw(void);

	void AddStage(NAME name, int modelHandle); // ステージの登録
	void SetActiveStage(NAME name);            // ステージの切り替え

private:

	std::map<NAME, StageData> stageMap_; // 複数のステージを登録
	NAME activeStageName_;               // 現在表示中のステージ名

	// シングルトン参照
	ResourceManager& resMng_;

	Player* player_;

	float step_;

};
