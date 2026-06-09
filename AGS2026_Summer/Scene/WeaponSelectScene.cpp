#include "WeaponSelectScene.h"
#include "../Manager/InputManager.h"
#include "../Manager/SceneManager.h"
#include "../Audio/AudioManager.h"
#include "../Object/Player.h" // 装備ID保存用

WeaponSelectScene::WeaponSelectScene(void) {}
WeaponSelectScene::~WeaponSelectScene(void) {}

void WeaponSelectScene::Init(void)
{
    // 本来はCSVやResourceManagerから読み込むのが理想ですが、一旦ここで定義
    armWeapons_ = {
        { "RIFLE", "一般的なライフル" },
        { "MACHINE GUN", "高速連射が可能なマシンガン" },
        { "SNIPER RIFLE", "高射程、高火力なスナイパーライフル" }
    };

    backWeapons_ = {
        { "SMALL MISSILE", "ロック数1、追尾可能なミサイル" },
        { "MULTI MISSILE", "ロック数4、マルチロック可能な連装ミサイル" },
    };

    state_ = STATE::SLOT_SELECT;
}

void WeaponSelectScene::Update(void)
{
    InputManager& ins = InputManager::GetInstance();

    if (state_ == STATE::SLOT_SELECT) {
        // --- 部位選択中 ---
        if (ins.IsActionTrgDown(InputManager::ACTION::MENU_UP)) {
            slotCursor_ = (slotCursor_ + 1) % 2;
            AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
        }
        if (ins.IsActionTrgDown(InputManager::ACTION::MENU_DOWN)) {
            slotCursor_ = (slotCursor_ + 1) % 2;
            AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
        }

        if (ins.IsActionTrgDown(InputManager::ACTION::DECIDE)) {
            state_ = STATE::WEAPON_SELECT;
            weaponCursor_ = (slotCursor_ == 0) ? Player::s_rightArmEquipID : Player::s_rightBackEquipID;
            AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
        }

        if (ins.IsActionTrgDown(InputManager::ACTION::CANCEL)) {
            AudioManager::GetInstance()->PlaySE(SoundID::SE_CANCEL);
            SceneManager::GetInstance().PopScene();
        }
    }
    else {
        // --- 武器選択中 ---
        auto& currentList = (slotCursor_ == 0) ? armWeapons_ : backWeapons_;

        if (ins.IsActionTrgDown(InputManager::ACTION::MENU_UP)) {
            weaponCursor_ = (weaponCursor_ + currentList.size() - 1) % currentList.size();
            AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
        }
        if (ins.IsActionTrgDown(InputManager::ACTION::MENU_DOWN)) {
            weaponCursor_ = (weaponCursor_ + 1) % currentList.size();
            AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
        }

        if (ins.IsActionTrgDown(InputManager::ACTION::DECIDE)) {
            // 装備確定！Playerの静的変数に保存
            if (slotCursor_ == 0) Player::s_rightArmEquipID = weaponCursor_;
            else                  Player::s_rightBackEquipID = weaponCursor_;

            AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
            state_ = STATE::SLOT_SELECT; // 部位選択に戻る
        }

        if (ins.IsActionTrgDown(InputManager::ACTION::CANCEL)) {
            state_ = STATE::SLOT_SELECT;
            AudioManager::GetInstance()->PlaySE(SoundID::SE_CANCEL);
        }
    }
}

void WeaponSelectScene::Draw(void)
{
    unsigned int white = GetColor(255, 255, 255);
    unsigned int cyan = GetColor(0, 255, 255);
    unsigned int gray = GetColor(100, 100, 100);

    DrawString(50, 50, "=== ASSEMBLE ROOM ===", white);

    // 1. スロット選択エリア
    unsigned int armColor = (slotCursor_ == 0 && state_ == STATE::SLOT_SELECT) ? cyan : white;
    unsigned int backColor = (slotCursor_ == 1 && state_ == STATE::SLOT_SELECT) ? cyan : white;

    DrawFormatString(100, 150, armColor, "RIGHT ARM: %s", armWeapons_[Player::s_rightArmEquipID].name.c_str());
    DrawFormatString(100, 180, backColor, "RIGHT BACK: %s", backWeapons_[Player::s_rightBackEquipID].name.c_str());

    // 2. 武器選択ウィンドウ（WEAPON_SELECT時のみ表示）
    if (state_ == STATE::WEAPON_SELECT) {
        DrawBox(400, 100, 800, 500, GetColor(10, 20, 40), TRUE); // 背景
        DrawBox(400, 100, 800, 500, cyan, FALSE);               // 枠

        auto& currentList = (slotCursor_ == 0) ? armWeapons_ : backWeapons_;
        for (int i = 0; i < currentList.size(); ++i) {
            unsigned int color = (weaponCursor_ == i) ? cyan : white;
            std::string prefix = (weaponCursor_ == i) ? "> " : "  ";
            DrawFormatString(420, 130 + i * 30, color, "%s%s", prefix.c_str(), currentList[i].name.c_str());
        }

        // 説明文
        DrawString(420, 400, "--- SPEC ---", white);
        DrawString(420, 430, currentList[weaponCursor_].description.c_str(), gray);
    }

    DrawString(50, 650, "[DECIDE]:Select  [CANCEL]:Back", white);
}