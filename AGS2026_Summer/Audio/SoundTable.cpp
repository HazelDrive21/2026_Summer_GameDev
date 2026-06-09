#include "SoundTable.h"

namespace SoundTable_System
{
	const std::unordered_map<SoundID, std::string> Table =
	{
		{ SoundID::SE_OK, "Data/Sound/SE/nc371111.wav" },
		{ SoundID::SE_CHOICE, "Data/Sound/SE/nc294280.wav" },
		{ SoundID::SE_CANCEL, "Data/Sound/SE/nc296403.mp3" },
	};
}

namespace SoundTable_Title
{
	const std::unordered_map<SoundID, std::string> Table =
	{
		{ SoundID::BGM_TITLE, "Data/Sound/BGM/nc419539.mp3" },
	};
}

namespace SoundTable_Menu
{
	const std::unordered_map<SoundID, std::string> Table =
	{
		{ SoundID::BGM_MENU, "Data/Sound/BGM/nc109698_Dew.mp3" },
	};
}

namespace SoundTable_Game
{
	const std::unordered_map<SoundID, std::string> Table =
	{
		{ SoundID::BGM_GAME, "Data/Sound/BGM/nc6087.mp3" },
		{ SoundID::SE_MOVE,	"Data/Sound/SE/nc64959.wav" },
		{ SoundID::SE_BOOST, "Data/Sound/SE/スーパーロボ急制動時のスラスター音.wav" },
		{ SoundID::SE_BOOSTING, "Data/Sound/SE/nc483177.mp3" },

		{ SoundID::SE_LOCKING, "Data/Sound/SE/meka_ge_mouse_h02.mp3" },
		{ SoundID::SE_LOCKON, "Data/Sound/SE/決定6.mp3" },
		{ SoundID::SE_WEAPON_CHANGE, "Data/Sound/SE/宇宙ステーションスラスタの起動.mp3" },

		{ SoundID::SE_BULLET, "Data/Sound/SE/nc122208.wav" },
		{ SoundID::SE_BULLET_EN, "Data/Sound/SE/bullet_en.wav" },
		{ SoundID::SE_MISSILE, "Data/Sound/SE/nc46982.wav" },
		{ SoundID::SE_BLADE, "Data/Sound/SE/attack.wav" },
	};
}

namespace SoundTable_Result
{
	const std::unordered_map<SoundID, std::string> Table =
	{
		{ SoundID::BGM_RESULT, "Data/Sound/BGM/nc176177.wav" },
	};
}