#pragma once
#include "SceneBase.h"
#include <vector>
#include <string>

class WeaponSelectScene : public SceneBase
{
public:
    WeaponSelectScene(void);
    virtual ~WeaponSelectScene(void);

    void Init() override;
    void Update() override;
    void Draw() override;

private:
    // 選択状態
    enum class STATE {
        SLOT_SELECT,   // どの部位を変えるか選択
        WEAPON_SELECT, // 具体的にどの武器にするか選択
    };

    struct WeaponData {
        std::string name;
        std::string description;
    };

    STATE state_ = STATE::SLOT_SELECT;
    int slotCursor_ = 0;   // 0:右手, 1:右肩
    int weaponCursor_ = 0;

    // 武器リストの定義
    std::vector<WeaponData> armWeapons_;
    std::vector<WeaponData> backWeapons_;
};