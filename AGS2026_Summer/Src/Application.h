#pragma once
#include <string>

class FpsController;

class Application
{

public:

	// スクリーンサイズ
	static constexpr int SCREEN_SIZE_X = 1280;
	static constexpr int SCREEN_SIZE_Y = 720;

	// 固定FPS
	static constexpr int FRAME_RATE = 60;

	// 重力
	static constexpr float GRAVITY = 9.81f * 100.0f;
	static constexpr float GRAVITY_SCALE = 0.7f;

	// データパス関連
	//-------------------------------------------
	static const std::string PATH_DATA;
	static const std::string PATH_IMAGE;
	static const std::string PATH_MODEL;
	static const std::string PATH_EFFECT;
	static const std::string PATH_CSV;

	static const std::string PATH_KEY_CONFIG;
	static const std::string PATH_KEY_CONFIG_GAMEPAD;
	static const std::string PATH_KEY_CONFIG_KEYBOARD;
	//-------------------------------------------

	// インスタンスを明示的に生成
	static void CreateInstance(void);

	// インスタンスの取得
	static Application& GetInstance(void);

	// 初期化
	void Init(void);

	// ゲームループの開始
	void Run(void);

	// リソースの破棄
	void Destroy(void);

	// 初期化成功／失敗の判定
	bool IsInitFail(void) const;

	// 解放成功／失敗の判定
	bool IsReleaseFail(void) const;

	// 重力の取得
	float GetGravityPow(void) const { return GRAVITY * GRAVITY_SCALE; }

private:

	// 静的インスタンス
	static Application* instance_;

	// FPS制御
	FpsController* fpsController_;

	// 初期化失敗
	bool isInitFail_;

	// 解放失敗
	bool isReleaseFail_;

	// デフォルトコンストラクタをprivateにして、
	// 外部から生成できない様にする
	Application(void);

	// コピーコンストラクタも同様
	Application(const Application& instance) = default;

	// デストラクタも同様
	~Application(void) = default;

	// エフェクシアの初期化
	void InitEffekseer(void);

};