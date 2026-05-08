#pragma once
#include <DxLib.h>

class Player;

class FCS
{
public:
	// サイトの種類（AC2AA準拠：横長、縦長、正方形など）
	enum class SITE_TYPE
	{
		STANDARD,	// 標準（正方形）
		WIDE_SHALLOW,	// 横広・浅
		DEEP_NARROW,	// 縦長・深
		LARGE		// 広域
	};

	// ロックオン状態
	enum class LOCK_STATE
	{
		NONE,		// 索敵中
		LOCKING,	// ロック中（緑）
		LOCKED		// ロック完了（赤）
	};

	FCS(void);
	~FCS(void);

	void Init(void);
	void Update(void);
	void Draw(void);

	// サイトタイプの変更
	void ChangeSiteType(SITE_TYPE type);

	// プレイヤー参照の設定
	void SetPlayer(Player* player) { player_ = player; }

private:
	Player* player_ = nullptr;

	// サイトの中心座標（スクリーン座標系）
	float centerX_;
	float centerY_;

	// サイトの現在の幅と高さ
	float siteWidth_;
	float siteHeight_;

	// 目標とするサイトの幅と高さ（タイプ切り替え時の演出用）
	float targetWidth_;
	float targetHeight_;

	// サイトの伸縮速度
	static constexpr float RESIZE_SPEED = 0.2f;

	// ロックオン状態
	LOCK_STATE lockState_;

	// サイトの色
	unsigned int siteColor_;

	// サイトのタイプ
	SITE_TYPE siteType_;

	// サイトのサイズ・色を更新
	void UpdateSiteStyle(void);

	// 2D枠の描画処理
	void DrawSiteFrame(void);
};