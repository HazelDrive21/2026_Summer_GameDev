#include <DxLib.h>
#include <EffekseerForDXLib.h>
#include "Manager/InputManager.h"
#include "Manager/ResourceManager.h"
#include "Manager/SceneManager.h"
#include "Common/FpsController.h"
#include "Audio/AudioManager.h"
#include "Application.h"

Application* Application::instance_ = nullptr;

const std::string Application::PATH_IMAGE = "Data/Image/";
const std::string Application::PATH_MODEL = "Data/Model/";
const std::string Application::PATH_EFFECT = "Data/Effect/";
const std::string Application::PATH_CSV = "Data/CSV/";

void Application::CreateInstance(void)
{
	if (instance_ == nullptr)
	{
		instance_ = new Application();
	}
	instance_->Init();
}

Application& Application::GetInstance(void)
{
	return *instance_;
}

void Application::Init(void)
{

	// アプリケーションの初期設定
	SetWindowText("トレーサー");

	// ウィンドウサイズ
	SetGraphMode(SCREEN_SIZE_X, SCREEN_SIZE_Y, 32);
	ChangeWindowMode(true);

	// FPS制御初期化
	fpsController_ = new FpsController(FRAME_RATE);

	// DxLibの初期化
	SetUseDirect3DVersion(DX_DIRECT3D_11);
	isInitFail_ = false;
	if (DxLib_Init() == -1)
	{
		isInitFail_ = true;
		return;
	}

	// Effekseerの初期化
	InitEffekseer();

	// 乱数のシード値を設定する
	DATEDATA date;

	// 現在時刻を取得する
	GetDateTime(&date);

	// 乱数の初期値を設定する
	// 設定する数値によって、ランダムの出方が変わる
	SRand(date.Year + date.Mon + date.Day + date.Hour + date.Min + date.Sec);

	// 入力制御初期化
	SetUseDirectInputFlag(true);
	InputManager::CreateInstance();

	// リソース管理初期化
	ResourceManager::CreateInstance();

	// オーディオ管理の初期化
	AudioManager::CreateInstance();
	AudioManager::GetInstance()->Init();
	// システム共通の音
	AudioManager::GetInstance()->LoadSceneSound(LoadScene::SYSTEM);

	// シーン管理初期化
	SceneManager::CreateInstance();

}

void Application::Run(void)
{

	InputManager& inputManager = InputManager::GetInstance();
	SceneManager& sceneManager = SceneManager::GetInstance();

	// ゲームループ
	while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_Z) == 0)
	{

		inputManager.Update();
		sceneManager.Update();

		sceneManager.Draw();

#ifdef _DEBUG
		// 平均FPS描画
		fpsController_->Draw();
#endif // _DEBUG

		ScreenFlip();

		// 理想FPS経過待ち
		fpsController_->Wait();

	}

}

void Application::Destroy(void)
{

	// FPS制御メモリ解放
	delete fpsController_;

	InputManager::GetInstance().Destroy();
	ResourceManager::GetInstance().Destroy();

	// シーン管理解放
	SceneManager::GetInstance().Destroy();

	// オーディオ管理の解放
	if (AudioManager::GetInstance() != nullptr)
	{
		AudioManager::GetInstance()->DeleteAll(); // 全てのサウンドハンドルを削除
		AudioManager::DeleteInstance();           // インスタンス自体を delete
	}

	// Effekseerを終了する。
	Effkseer_End();

	// DxLib終了
	if (DxLib_End() == -1)
	{
		isReleaseFail_ = true;
	}

	// インスタンスのメモリ解放
	delete instance_;

}

bool Application::IsInitFail(void) const
{
	return isInitFail_;
}

bool Application::IsReleaseFail(void) const
{
	return isReleaseFail_;
}

Application::Application(void)
	:
	isInitFail_(false),
	isReleaseFail_(false),
	fpsController_(nullptr)
{
}

void Application::InitEffekseer(void)
{
	if (Effekseer_Init(8000) == -1)
	{
		DxLib_End();
	}

	SetChangeScreenModeGraphicsSystemResetFlag(FALSE);

	Effekseer_SetGraphicsDeviceLostCallbackFunctions();
}
