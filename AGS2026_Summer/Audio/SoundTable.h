#pragma once
#include <string>
#include <unordered_map>

enum class SoundID
{
	BGM_TITLE,
	BGM_MENU,
	BGM_GAME,
	BGM_RESULT,
	BGM_GAMEOVER,

	SE_OK,
	SE_CHOICE,
	SE_CANCEL,

	SE_MOVE,
	SE_BOOST,
	SE_BOOSTING,

	SE_LOCKING,
	SE_LOCKON,

	SE_WEAPON_CHANGE,

	SE_BULLET,
	SE_BULLET_EN,
	SE_MISSILE,
	SE_BLADE,
	SE_CANNON,
	SE_CANNON_EN,
};

enum class LoadScene
{
	SYSTEM,
	TITLE,
	MENU,
	GAME,
	RESULT,
};

// 名前空間の宣言のみヘッダーに開示しておく
namespace SoundTable_System { extern const std::unordered_map<SoundID, std::string> Table; }
namespace SoundTable_Title { extern const std::unordered_map<SoundID, std::string> Table; }
namespace SoundTable_Menu { extern const std::unordered_map<SoundID, std::string> Table; }
namespace SoundTable_Game { extern const std::unordered_map<SoundID, std::string> Table; }
namespace SoundTable_Result { extern const std::unordered_map<SoundID, std::string> Table; }