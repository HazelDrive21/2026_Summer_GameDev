#include <vector>
#include <map>
#include <DxLib.h>
#include "../Utility/AsoUtility.h"
#include "../Manager/ResourceManager.h"
#include "Player.h"
#include "Common/Collider.h"
#include "Common/Transform.h"
#include "Stage.h"

Stage::Stage(Player* player)
    : resMng_(ResourceManager::GetInstance())
{
    player_ = player;
    step_ = 0.0f;
    // 初期値として何か設定しておく
    activeStageName_ = NAME::MAIN_BASE;
}

Stage::~Stage(void)
{
    // モデルハンドルはResourceManagerが管理しているならここでは何もしない
    // StageData内のTransformで特別な解放が必要ならここで行う
    stageMap_.clear();
}

void Stage::Init(void)
{
    // ここでステージを登録する
    // 例：メインステージの登録
    int model = resMng_.LoadModelDuplicate(ResourceManager::SRC::STAGE);
    AddStage(NAME::MAIN_BASE, model);

    // 登録したステージを有効化
    SetActiveStage(NAME::MAIN_BASE);

    step_ = -1.0f;
}

void Stage::Update(void)
{
    // 現在のステージの行列更新などが必要な場合
    if (stageMap_.count(activeStageName_) > 0)
    {
        stageMap_[activeStageName_].transform.Update();
    }
}

void Stage::Draw(void)
{
    if (stageMap_.count(activeStageName_) == 0) return;
    
    auto& current = stageMap_[activeStageName_];

    SetUseLighting(FALSE);

    // モデルの描画
    MV1DrawModel(current.modelHandle);
}

void Stage::AddStage(NAME name, int modelHandle) 
{
    StageData data;
    data.modelHandle = modelHandle;

    // Transformの設定
    data.transform.SetModel(data.modelHandle);
    data.transform.pos = VGet(0, 0, 0);
    data.transform.scl = { 500.0f,500.0f,500.0f };
    data.transform.SetEmissive(GetColorF(0.0f, 0.0f, 0.0f, 0.0f), -1);
    data.transform.quaRot = Quaternion();

    // 当たり判定を「ステージ（メッシュ判定）」として作成
    data.transform.MakeCollider(Collider::TYPE::STAGE);
    data.transform.Update();

    stageMap_[name] = data;

    
}

void Stage::SetActiveStage(NAME name) 
{
    if (stageMap_.count(name) == 0) return;

    activeStageName_ = name;

    // プレイヤーの古い当たり判定を消して、新しいステージの判定を追加
    player_->ClearCollider();
    player_->AddCollider(stageMap_[name].transform.collider);
}