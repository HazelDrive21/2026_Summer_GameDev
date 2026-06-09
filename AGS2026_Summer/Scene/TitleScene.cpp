#include <string>
#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/SceneManager.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/InputManager.h"
#include "../Manager/Camera.h"
#include "../Audio/AudioManager.h"
#include "TitleScene.h"

TitleScene::TitleScene(void)
{
	imgPush_ = -1;
	imgTitle_ = -1;

}

TitleScene::~TitleScene(void)
{

}

void TitleScene::Init(void)
{

	// 画像読み込み
	imgTitle_ = resMng_.Load(ResourceManager::SRC::TITLE).handleId_;
	imgPush_ = resMng_.Load(ResourceManager::SRC::PUSH_SPACE).handleId_;

	// 定点カメラ
	SceneManager::GetInstance().GetCamera()->ChangeMode(Camera::MODE::FIXED_POINT);

	AudioManager::GetInstance()->LoadSceneSound(LoadScene::TITLE);
	AudioManager::GetInstance()->PlayBGM(SoundID::BGM_TITLE);

}

void TitleScene::Update(void)
{
	InputManager& ins = InputManager::GetInstance();

	// 🔥 【追加】終了確認中の更新処理
	if (isConfirmExit_)
	{
		// 上下で YES / NO の切り替え
		if (ins.IsActionTrgDown(InputManager::ACTION::MENU_UP)) {
			confirmCursor_ = (CONFIRM)(((int)confirmCursor_ + (int)CONFIRM::MAX - 1) % (int)CONFIRM::MAX);
			AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
		}
		if (ins.IsActionTrgDown(InputManager::ACTION::MENU_DOWN)) {
			confirmCursor_ = (CONFIRM)(((int)confirmCursor_ + 1) % (int)CONFIRM::MAX);
			AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
		}

		// 決定処理
		if (ins.IsActionTrgDown(InputManager::ACTION::DECIDE))
		{
			if (confirmCursor_ == CONFIRM::YES) {
				AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
				PostQuitMessage(0); // 安全にゲームを終了
			}
			else {
				// NO を選んだらメニューに戻る
				AudioManager::GetInstance()->PlaySE(SoundID::SE_CANCEL);
				isConfirmExit_ = false;
			}
		}
		return; // 終了確認中は、これ以降のメインメニュー処理をスキップ
	}


	// ─── 従来のメインメニュー更新処理 ───
	if (ins.IsActionTrgDown(InputManager::ACTION::MENU_UP)) {
		cursor_ = (MENU)(((int)cursor_ + (int)MENU::MAX - 1) % (int)MENU::MAX);
		AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
	}
	if (ins.IsActionTrgDown(InputManager::ACTION::MENU_DOWN)) {
		cursor_ = (MENU)(((int)cursor_ + 1) % (int)MENU::MAX);
		AudioManager::GetInstance()->PlaySE(SoundID::SE_CHOICE);
	}

	// 決定処理
	if (ins.IsActionTrgDown(InputManager::ACTION::DECIDE))
	{
		if (cursor_ == MENU::START) {
			AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
			AudioManager::GetInstance()->StopBGM();
			AudioManager::GetInstance()->DeleteSceneSound(LoadScene::TITLE);
			SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::MENU);
		}
		else if (cursor_ == MENU::MANUAL) {
			AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
			SceneManager::GetInstance().PushScene(SceneManager::SCENE_ID::INSTRUCTION);
		}
		// 🔥 EXIT選択時は、確認フラグを true にするだけにする
		else if (cursor_ == MENU::EXIT) {
			AudioManager::GetInstance()->PlaySE(SoundID::SE_OK);
			isConfirmExit_ = true;
			confirmCursor_ = CONFIRM::NO; // 初期カーソルを安全な「NO」にセット
		}
	}
}

void TitleScene::Draw(void)
{
	// （既存のタイトルロゴやメインメニューの描画はそのまま残す）
	int centerX = Application::SCREEN_SIZE_X / 2;
	int centerY = Application::SCREEN_SIZE_Y / 2;
	unsigned int white = GetColor(255, 255, 255);
	unsigned int cyan = GetColor(0, 255, 255);
	unsigned int startColor = (cursor_ == MENU::START) ? cyan : white;
	unsigned int manualColor = (cursor_ == MENU::MANUAL) ? cyan : white;
	unsigned int exitColor = (cursor_ == MENU::EXIT) ? cyan : white;

	// ～ 中略（従来の文字描画など） ～
	DrawString(centerX - 50, 400, "GAME START", startColor);
	DrawString(centerX - 50, 440, "MANUAL", manualColor);
	DrawString(centerX - 50, 480, "QUIT GAME", exitColor);


	// 🔥 【追加】終了確認ポップアップの描画
	if (isConfirmExit_)
	{
		// ウィンドウのサイズと位置の設定
		int boxW = 360;
		int boxH = 160;
		int x1 = centerX - boxW / 2;
		int y1 = centerY - boxH / 2;
		int x2 = centerX + boxW / 2;
		int y2 = centerY + boxH / 2;

		// ① 背景を少し暗く塗りつぶす枠（黒に近い濃紺など、ACのシステムウィンドウ風）
		DrawBox(x1, y1, x2, y2, GetColor(10, 15, 25), TRUE);
		// ② 枠線（選択中と同じシアカラーで細い線を引く）
		DrawBox(x1, y1, x2, y2, cyan, FALSE);

		// ③ メッセージの描画
		DrawString(centerX - 90, y1 + 25, "ゲームを終了しますか？", white);

		// ④ YES/NO の色判定と描画
		unsigned int yesColor = (confirmCursor_ == CONFIRM::YES) ? cyan : white;
		unsigned int noColor = (confirmCursor_ == CONFIRM::NO) ? cyan : white;

		// カーソルがついている方に「▶」や「>」を付けるとさらにACらしくなります
		std::string yesStr = (confirmCursor_ == CONFIRM::YES) ? "  YES" : "  YES";
		std::string noStr = (confirmCursor_ == CONFIRM::NO) ? "  NO" : "  NO";

		DrawString(centerX - 30, y1 + 75, yesStr.c_str(), yesColor);
		DrawString(centerX - 30, y1 + 110, noStr.c_str(), noColor);
	}
}