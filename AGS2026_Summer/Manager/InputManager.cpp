#include <DxLib.h>
#include "InputManager.h"

InputManager* InputManager::instance_ = nullptr;

bool InputManager::IsActionTrgDown(ACTION action) const
{
	int targetKey = -1;
	JOYPAD_BTN targetBtn = JOYPAD_BTN::MAX;

	// ★ ここを変更するだけで、ゲーム全体の操作方法を一括で編集できます！
	switch (action)
	{
	case ACTION::DECIDE:
		targetKey = KEY_INPUT_SPACE; // 決定は SPACE キー
		targetBtn = JOYPAD_BTN::DOWN;
		break;
	case ACTION::CANCEL:
		targetKey = KEY_INPUT_ESCAPE;  // キャンセルは ESC キー
		targetBtn = JOYPAD_BTN::RIGHT;
		break;
	case ACTION::PAUSE:
		targetKey = KEY_INPUT_TAB;     // ポーズは Tab キー
		targetBtn = JOYPAD_BTN::START;
		break;
	case ACTION::SUB_FUNC:
		targetKey = KEY_INPUT_Z;     // サブ機能は Z キー
		targetBtn = JOYPAD_BTN::LEFT;
		break;
	case ACTION::FIRE_RIGHT:
		targetKey = KEY_INPUT_NUMPAD7;
		targetBtn = JOYPAD_BTN::LEFT;       // 例：右側の左ボタン（SwitchのY / XboxのXで射撃）
	case ACTION::FIRE_LEFT:
		targetKey = KEY_INPUT_NUMPAD9;
		targetBtn = JOYPAD_BTN::TOP;    // 例：別の攻撃用ボタンなど
	case ACTION::WEAPON_CHANGE:
		targetKey = KEY_INPUT_LSHIFT; // 武器変更は左 Shift キー
		targetBtn = JOYPAD_BTN::TOP;
		break;
	case ACTION::BOOST:
		targetKey = KEY_INPUT_SPACE; // ブーストは Space キー
		targetBtn = JOYPAD_BTN::DOWN;
		break;
	case ACTION::MENU_UP:
		targetKey = KEY_INPUT_UP;    // メニュー上は ↑ キー
		targetBtn = JOYPAD_BTN::DPAD_UP;
		break;
	case ACTION::MENU_DOWN:
		targetKey = KEY_INPUT_DOWN;  // メニュー下は ↓ キー
		targetBtn = JOYPAD_BTN::DPAD_DOWN;
		break;
	case ACTION::MENU_LEFT:
		targetKey = KEY_INPUT_LEFT;  // メニュー左は ← キー
		targetBtn = JOYPAD_BTN::DPAD_LEFT;
		break;
	case ACTION::MENU_RIGHT:
		targetKey = KEY_INPUT_RIGHT; // メニュー右は → キー
		targetBtn = JOYPAD_BTN::DPAD_RIGHT;
		break;
	}

	// ① キーボードの判定
	bool isKeyTrg = (targetKey != -1) && IsTrgDown(targetKey);

	// ② コントローラーの判定（純粋なパッド1の入力を監視するため JOYPAD_NO::PAD1 を使用）
	bool isPadTrg = (targetBtn != JOYPAD_BTN::MAX) && IsPadBtnTrgDown(JOYPAD_NO::PAD1, targetBtn);

	// どちらか一方が押された瞬間なら true を返す
	return (isKeyTrg || isPadTrg);
}

bool InputManager::IsActionPush(ACTION action) const
{
	int targetKey = -1;
	JOYPAD_BTN targetBtn = JOYPAD_BTN::MAX;

	switch (action)
	{
		// --- 左手側：移動系 ---
	case ACTION::MOVE_FORWARD:
		targetKey = KEY_INPUT_W;
		targetBtn = JOYPAD_BTN::L_STICK_UP;
		break;
	case ACTION::MOVE_BACK:
		targetKey = KEY_INPUT_S;
		targetBtn = JOYPAD_BTN::L_STICK_DOWN;
		break;
	case ACTION::MOVE_LEFT:
		targetKey = KEY_INPUT_A;
		targetBtn = JOYPAD_BTN::L1;
		break;
	case ACTION::MOVE_RIGHT:
		targetKey = KEY_INPUT_D;
		targetBtn = JOYPAD_BTN::R1;
		break;
	case ACTION::BOOST:
		targetKey = KEY_INPUT_SPACE;
		targetBtn = JOYPAD_BTN::DOWN;
		break;

		// --- 右手側：旋回・視点・攻撃系 ---
	case ACTION::TURN_LEFT:
		targetKey = KEY_INPUT_NUMPAD4;
		targetBtn = JOYPAD_BTN::L_STICK_LEFT;
		break;
	case ACTION::TURN_RIGHT:
		targetKey = KEY_INPUT_NUMPAD6;
		targetBtn = JOYPAD_BTN::L_STICK_RIGHT;
		break;
	case ACTION::LOOK_UP:
		targetKey = KEY_INPUT_NUMPAD8;
		targetBtn = JOYPAD_BTN::L_TRIGGER;         // 例：L2ボタンで上を見上げる
		break;
	case ACTION::LOOK_DOWN:
		targetKey = KEY_INPUT_NUMPAD2;
		targetBtn = JOYPAD_BTN::R_TRIGGER;         // 例：R2ボタンで下を見下ろす
		break;
	case ACTION::FIRE_RIGHT:
		targetKey = KEY_INPUT_NUMPAD5;
		targetBtn = JOYPAD_BTN::LEFT;       // 例：右側の左ボタン（SwitchのY / XboxのXで射撃）
		break;
	case ACTION::FIRE_LEFT:
		targetKey = KEY_INPUT_NUMPAD9;
		targetBtn = JOYPAD_BTN::TOP;    // 例：別の攻撃用ボタンなど
		break;
	default:
		return false;
	}

	// ① キーボードの判定（※IsPushはご自身の環境の押しっぱなし関数名に合わせてください）
	bool isKeyPush = (targetKey != -1) && IsNew(targetKey);

	// ② コントローラーの判定（既存の IsPadBtnPush を使用）
	bool isPadPush = (targetBtn != JOYPAD_BTN::MAX) && IsPadBtnPush(JOYPAD_NO::PAD1, targetBtn);

	// どちらか一方が押しっぱなしなら true を返す
	return (isKeyPush || isPadPush);
}

void InputManager::CreateInstance(void)
{
	if (instance_ == nullptr)
	{
		instance_ = new InputManager();
	}
	instance_->Init();
}

InputManager& InputManager::GetInstance(void)
{
	if (instance_ == nullptr)
	{
		InputManager::CreateInstance();
	}
	return *instance_;
}

void InputManager::Init(void)
{

	// ゲームで使用したいキーを、
	// 事前にここで登録しておいてください
	InputManager::GetInstance().Add(KEY_INPUT_SPACE);

	InputManager::GetInstance().Add(KEY_INPUT_W);
	InputManager::GetInstance().Add(KEY_INPUT_A);
	InputManager::GetInstance().Add(KEY_INPUT_S);
	InputManager::GetInstance().Add(KEY_INPUT_D);
	InputManager::GetInstance().Add(KEY_INPUT_LSHIFT);

	InputManager::GetInstance().Add(KEY_INPUT_NUMPAD8); // 視点上
	InputManager::GetInstance().Add(KEY_INPUT_NUMPAD2); // 視点下
	InputManager::GetInstance().Add(KEY_INPUT_NUMPAD4); // 左旋回
	InputManager::GetInstance().Add(KEY_INPUT_NUMPAD6); // 右旋回
	InputManager::GetInstance().Add(KEY_INPUT_NUMPAD5); // 右手武器（射撃）
	InputManager::GetInstance().Add(KEY_INPUT_NUMPAD9); // 左手武器（ブレード）

	InputManager::GetInstance().Add(KEY_INPUT_TAB);    // ポーズ (Tab)
	InputManager::GetInstance().Add(KEY_INPUT_ESCAPE); // キャンセル (Esc)
	InputManager::GetInstance().Add(KEY_INPUT_UP);     // メニュー上 (↑)
	InputManager::GetInstance().Add(KEY_INPUT_DOWN);   // メニュー下 (↓)
	InputManager::GetInstance().Add(KEY_INPUT_LEFT);   // メニュー左 (←)
	InputManager::GetInstance().Add(KEY_INPUT_RIGHT);  // メニュー右 (→)


	InputManager::MouseInfo info;

	// 左クリック
	info = InputManager::MouseInfo();
	info.key = MOUSE_INPUT_LEFT;
	info.keyOld = false;
	info.keyNew = false;
	info.keyTrgDown = false;
	info.keyTrgUp = false;
	mouseInfos_.emplace(info.key, info);

	// 右クリック
	info = InputManager::MouseInfo();
	info.key = MOUSE_INPUT_RIGHT;
	info.keyOld = false;
	info.keyNew = false;
	info.keyTrgDown = false;
	info.keyTrgUp = false;
	mouseInfos_.emplace(info.key, info);

}

void InputManager::Update(void)
{
	// デバッグ用：押されているボタン番号をすべて表示
	for (int i = 0; i < 32; i++) {
		if (joyDInState_.Buttons[i]) {
			DrawFormatString(0, 200, GetColor(255, 255, 255), "Button Number: %d", i);
		}
	}

	// キーボード検知
	for (auto& p : keyInfos_)
	{
		p.second.keyOld = p.second.keyNew;
		p.second.keyNew = CheckHitKey(p.second.key);
		p.second.keyTrgDown = p.second.keyNew && !p.second.keyOld;
		p.second.keyTrgUp = !p.second.keyNew && p.second.keyOld;
	} // ★バグ修正：forループをここでしっかり閉じる！

	// ★バグ修正：キーボードのスタック更新はループの外で行う！
	// 左右スタック (A と D)
	UpdateStack(KEY_INPUT_A, MoveDir::Left, horizontalStack_);
	UpdateStack(KEY_INPUT_D, MoveDir::Right, horizontalStack_);

	// 上下スタック (W と S)
	UpdateStack(KEY_INPUT_W, MoveDir::Up, verticalStack_);
	UpdateStack(KEY_INPUT_S, MoveDir::Down, verticalStack_);

	// マウス検知
	mouseInput_ = GetMouseInput();
	GetMousePoint(&mousePos_.x, &mousePos_.y);

	for (auto& p : mouseInfos_)
	{
		p.second.keyOld = p.second.keyNew;
		p.second.keyNew = mouseInput_ == p.second.key;
		p.second.keyTrgDown = p.second.keyNew && !p.second.keyOld;
		p.second.keyTrgUp = !p.second.keyNew && p.second.keyOld;
	}

	// パッド情報
	SetJPadInState(JOYPAD_NO::KEY_PAD1);
	SetJPadInState(JOYPAD_NO::PAD1);
	SetJPadInState(JOYPAD_NO::PAD2);
	SetJPadInState(JOYPAD_NO::PAD3);
	SetJPadInState(JOYPAD_NO::PAD4);

	// ★バグ修正＆改善：PAD1 の移動スタック更新
	auto& pad = padInfos_[static_cast<int>(JOYPAD_NO::PAD1)];

	// マジックナンバーを排除し、安全な列挙型のインデックスを取得
	int idxL1 = static_cast<int>(JOYPAD_BTN::L1);
	int idxR1 = static_cast<int>(JOYPAD_BTN::R1);
	int idxStickUp = static_cast<int>(JOYPAD_BTN::L_STICK_UP);
	int idxStickDown = static_cast<int>(JOYPAD_BTN::L_STICK_DOWN);

	// 左右平行移動 ( L1 / R1 )
	if (pad.IsTrgDown[idxL1]) horizontalStack_.push_back(MoveDir::Left);
	if (pad.IsTrgUp[idxL1])   horizontalStack_.remove(MoveDir::Left);

	if (pad.IsTrgDown[idxR1]) horizontalStack_.push_back(MoveDir::Right);
	if (pad.IsTrgUp[idxR1])   horizontalStack_.remove(MoveDir::Right);

	// ★重要バグ修正：上下移動（前進・後退）は、L2/R2ではなく「左スティックの上下」に連動させる
	// 左スティック・上 (前進)
	if (pad.IsTrgDown[idxStickUp]) verticalStack_.push_back(MoveDir::Up);
	if (pad.IsTrgUp[idxStickUp])   verticalStack_.remove(MoveDir::Up);

	// 左スティック・下 (後退)
	if (pad.IsTrgDown[idxStickDown]) verticalStack_.push_back(MoveDir::Down);
	if (pad.IsTrgUp[idxStickDown])   verticalStack_.remove(MoveDir::Down);
}

void InputManager::Destroy(void)
{
	keyInfos_.clear();
	mouseInfos_.clear();
	delete instance_;
}

void InputManager::Add(int key)
{
	InputManager::Info info = InputManager::Info();
	info.key = key;
	info.keyOld = false;
	info.keyNew = false;
	info.keyTrgDown = false;
	info.keyTrgUp = false;
	keyInfos_.emplace(key, info);
}

void InputManager::Clear(void)
{
	keyInfos_.clear();
}

bool InputManager::IsNew(int key) const
{
	return Find(key).keyNew;
}

bool InputManager::IsTrgDown(int key) const
{
	return Find(key).keyTrgDown;
}

bool InputManager::IsTrgUp(int key) const
{
	return Find(key).keyTrgUp;
}

Vector2 InputManager::GetMousePos(void) const
{
	return mousePos_;
}

int InputManager::GetMouse(void) const
{
	return mouseInput_;
}

bool InputManager::IsClickMouseLeft(void) const
{
	return mouseInput_ == MOUSE_INPUT_LEFT;
}

bool InputManager::IsClickMouseRight(void) const
{
	return mouseInput_ == MOUSE_INPUT_RIGHT;
}

bool InputManager::IsTrgMouseLeft(void) const
{
	return FindMouse(MOUSE_INPUT_LEFT).keyTrgDown;
}

bool InputManager::IsTrgMouseRight(void) const
{
	return FindMouse(MOUSE_INPUT_RIGHT).keyTrgDown;
}

InputManager::InputManager(void)
{
	mouseInput_ = -1;
}

const InputManager::Info& InputManager::Find(int key) const
{

	auto it = keyInfos_.find(key);
	if (it != keyInfos_.end())
	{
		return it->second;
	}

	return infoEmpty_;

}

const InputManager::MouseInfo& InputManager::FindMouse(int key) const
{
	auto it = mouseInfos_.find(key);
	if (it != mouseInfos_.end())
	{
		return it->second;
	}

	return mouseInfoEmpty_;
}

InputManager::JOYPAD_TYPE InputManager::GetJPadType(JOYPAD_NO no)
{
	return static_cast<InputManager::JOYPAD_TYPE>(GetJoypadType(static_cast<int>(no)));
}

DINPUT_JOYSTATE InputManager::GetJPadDInputState(JOYPAD_NO no)
{
	// コントローラ情報
	GetJoypadDirectInputState(static_cast<int>(no), &joyDInState_);
	return joyDInState_;
}

XINPUT_STATE InputManager::GetJPadXInputState(JOYPAD_NO no)
{
	// コントローラ情報
	GetJoypadXInputState(static_cast<int>(no), &joyXInState_);
	return joyXInState_;
}

void InputManager::SetJPadInState(JOYPAD_NO jpNo)
{
	int no = static_cast<int>(jpNo);
	auto stateNew = GetJPadInputState(jpNo);
	auto& stateNow = padInfos_[no];

	int max = static_cast<int>(JOYPAD_BTN::MAX);
	for (int i = 0; i < max; i++)
	{
		stateNow.ButtonsOld[i] = stateNow.ButtonsNew[i];
		stateNow.ButtonsNew[i] = stateNew.ButtonsNew[i];

		stateNow.IsOld[i] = stateNow.IsNew[i];
		stateNow.IsNew[i] = stateNow.ButtonsNew[i] > 0;

		stateNow.IsTrgDown[i] = stateNow.IsNew[i] && !stateNow.IsOld[i];
		stateNow.IsTrgUp[i] = !stateNow.IsNew[i] && stateNow.IsOld[i];
	}

	// ★【改善】ループの外で1回だけ安全に代入
	stateNow.AKeyLX = stateNew.AKeyLX;
	stateNow.AKeyLY = stateNew.AKeyLY;
	stateNow.AKeyRX = stateNew.AKeyRX;
	stateNow.AKeyRY = stateNew.AKeyRY;
}

InputManager::JOYPAD_IN_STATE InputManager::GetJPadInputState(JOYPAD_NO no)
{
	JOYPAD_IN_STATE ret = JOYPAD_IN_STATE();
	auto type = GetJPadType(no);

	switch (type)
	{
	case InputManager::JOYPAD_TYPE::OTHER:
		break;

		// ★【改善】360とONEを共通化して、どちらで認識されても動くように
	case InputManager::JOYPAD_TYPE::XBOX_360:
	case InputManager::JOYPAD_TYPE::XBOX_ONE:
	{
		auto d = GetJPadDInputState(no);
		auto x = GetJPadXInputState(no);
		int idx;

		idx = static_cast<int>(JOYPAD_BTN::TOP);
		ret.ButtonsNew[idx] = d.Buttons[3];// Y
		idx = static_cast<int>(JOYPAD_BTN::LEFT);
		ret.ButtonsNew[idx] = d.Buttons[2];// X
		idx = static_cast<int>(JOYPAD_BTN::RIGHT);
		ret.ButtonsNew[idx] = d.Buttons[1];// B
		idx = static_cast<int>(JOYPAD_BTN::DOWN);
		ret.ButtonsNew[idx] = d.Buttons[0];// A
		idx = static_cast<int>(JOYPAD_BTN::L1);
		ret.ButtonsNew[idx] = d.Buttons[4];// LB
		idx = static_cast<int>(JOYPAD_BTN::R1);
		ret.ButtonsNew[idx] = d.Buttons[5]; // RB
		idx = static_cast<int>(JOYPAD_BTN::R_TRIGGER);
		ret.ButtonsNew[idx] = x.RightTrigger;// R_TRIGGER
		idx = static_cast<int>(JOYPAD_BTN::L_TRIGGER);
		ret.ButtonsNew[idx] = x.LeftTrigger; // L_TRIGGER

		// 十字キー
		ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::DPAD_UP)] = x.Buttons[XINPUT_BUTTON_DPAD_UP];
		ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::DPAD_DOWN)] = x.Buttons[XINPUT_BUTTON_DPAD_DOWN];
		ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::DPAD_LEFT)] = x.Buttons[XINPUT_BUTTON_DPAD_LEFT];
		ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::DPAD_RIGHT)] = x.Buttons[XINPUT_BUTTON_DPAD_RIGHT];

		ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::START)] = x.Buttons[XINPUT_BUTTON_START];

		// スティック
		ret.AKeyLX = d.X;
		ret.AKeyLY = d.Y;
		ret.AKeyRX = d.Rx;
		ret.AKeyRY = d.Ry;
	}
	break;

	// ★【改善】DUAL_SHOCK_4もボタン配置が同じなのでDUAL_SENSEと共通化
	case InputManager::JOYPAD_TYPE::DUAL_SHOCK_4:
	case InputManager::JOYPAD_TYPE::DUAL_SENSE:
	{
		auto d = GetJPadDInputState(no);
		int idx;

		idx = static_cast<int>(JOYPAD_BTN::TOP);
		ret.ButtonsNew[idx] = d.Buttons[3];// △
		idx = static_cast<int>(JOYPAD_BTN::LEFT);
		ret.ButtonsNew[idx] = d.Buttons[0];// □
		idx = static_cast<int>(JOYPAD_BTN::RIGHT);
		ret.ButtonsNew[idx] = d.Buttons[2];// 〇
		idx = static_cast<int>(JOYPAD_BTN::DOWN);
		ret.ButtonsNew[idx] = d.Buttons[1];// ×
		idx = static_cast<int>(JOYPAD_BTN::L1);
		ret.ButtonsNew[idx] = d.Buttons[4]; // L1
		idx = static_cast<int>(JOYPAD_BTN::R1);
		ret.ButtonsNew[idx] = d.Buttons[5]; // R1
		idx = static_cast<int>(JOYPAD_BTN::L_TRIGGER);
		ret.ButtonsNew[idx] = d.Buttons[6]; // L_TRIGGER
		idx = static_cast<int>(JOYPAD_BTN::R_TRIGGER);
		ret.ButtonsNew[idx] = d.Buttons[7]; // R_TRIGGER

		// 十字キー処理 (POV角度判定)
		unsigned int pov = d.POV[0];
		if (pov != 0xFFFFFFFF)
		{
			if (pov >= 31500 || pov <= 4500)   ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::DPAD_UP)] = 1;
			if (pov >= 4500 && pov <= 13500)  ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::DPAD_RIGHT)] = 1;
			if (pov >= 13500 && pov <= 22500) ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::DPAD_DOWN)] = 1;
			if (pov >= 22500 && pov <= 31500) ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::DPAD_LEFT)] = 1;
		}

		// ★【修正】if (pov != 0xFFFFFFFF) の外に出しました！
		ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::START)] = d.Buttons[9];
		ret.ButtonsNew[static_cast<int>(JOYPAD_BTN::SELECT)] = d.Buttons[8];

		// スティック
		ret.AKeyLX = d.X;
		ret.AKeyLY = d.Y;
		ret.AKeyRX = d.Z;
		ret.AKeyRY = d.Rz;
	}
	break;

	default:
		break;
	}

	// 末尾のスティックのデジタル化処理などはそのまま
	constexpr int STICK_THRESHOLD = 100;
	int idxLeft = static_cast<int>(JOYPAD_BTN::L_STICK_LEFT);
	int idxRight = static_cast<int>(JOYPAD_BTN::L_STICK_RIGHT);
	int idxUp = static_cast<int>(JOYPAD_BTN::L_STICK_UP);
	int idxDown = static_cast<int>(JOYPAD_BTN::L_STICK_DOWN);

	ret.ButtonsNew[idxLeft] = (ret.AKeyLX < -STICK_THRESHOLD) ? 1 : 0;
	ret.ButtonsNew[idxRight] = (ret.AKeyLX > STICK_THRESHOLD) ? 1 : 0;
	ret.ButtonsNew[idxUp] = (ret.AKeyLY < -STICK_THRESHOLD) ? 1 : 0;
	ret.ButtonsNew[idxDown] = (ret.AKeyLY > STICK_THRESHOLD) ? 1 : 0;

	return ret;
}

bool InputManager::IsPadBtnNew(JOYPAD_NO no, JOYPAD_BTN btn) const
{
	return padInfos_[static_cast<int>(no)].IsNew[static_cast<int>(btn)];
}

bool InputManager::IsPadBtnTrgDown(JOYPAD_NO no, JOYPAD_BTN btn) const
{
	return padInfos_[static_cast<int>(no)].IsTrgDown[static_cast<int>(btn)];
}

bool InputManager::IsPadBtnTrgUp(JOYPAD_NO no, JOYPAD_BTN btn) const
{
	return padInfos_[static_cast<int>(no)].IsTrgUp[static_cast<int>(btn)];
}

bool InputManager::IsPadBtnPush(JOYPAD_NO no, JOYPAD_BTN btn) const {
	int padIdx = static_cast<int>(no);
	int btnIdx = static_cast<int>(btn);
	return padInfos_[padIdx].IsNew[btnIdx]; // 現在のフレームで押されているか
}

void InputManager::UpdateStack(int key, MoveDir dir, std::list<MoveDir>& stack)
{
	if (IsTrgDown(key)) {
		// 押されたら末尾に追加
		stack.push_back(dir);
	}
	if (IsTrgUp(key)) {
		// 離されたらリスト内から削除
		stack.remove(dir);
	}
}

InputManager::MoveDir InputManager::GetHorizontalDir() const 
{
	if (horizontalStack_.empty()) return MoveDir::None;
	return horizontalStack_.back(); // 一番最後に残っている（押された）ものを返す
}

InputManager::MoveDir InputManager::GetVerticalDir() const 
{
	if (verticalStack_.empty()) return MoveDir::None;
	return verticalStack_.back();
}

