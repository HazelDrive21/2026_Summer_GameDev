#pragma once
#include <DxLib.h>
#include "../Common/Quaternion.h"
class Transform;
class Player;

class Camera
{

public:

	// カメラスピード(度)
	static constexpr float SPEED = 1.0f;

	// カメラクリップ：NEAR
	static constexpr float CAMERA_NEAR = 10.0f;

	// カメラクリップ：NEAR
	static constexpr float CAMERA_FAR = 30000.0f;

	// カメラの初期座標
	static constexpr VECTOR DEFAULT_CAMERA_POS = { 0.0f, 0.0f, 0.0f };

	// 追従位置からカメラ位置までの相対座標
	static constexpr VECTOR LOCAL_F2C_POS = { 0.0f, 120.0f, -250.0f };

	// 追従位置から注視点までの相対座標
	static constexpr VECTOR LOCAL_F2T_POS = { 0.0f, 120.0f, 300.0f };

	// カメラのX回転上限度角
	static constexpr float LIMIT_X_UP_RAD = 67.0f * (DX_PI_F / 180.0f);
	static constexpr float LIMIT_X_DW_RAD = 67.0f * (DX_PI_F / 180.0f);
	
	// カメラモード
	enum class MODE
	{
		NONE,
		FIXED_POINT,
		FOLLOW,
		SELF_SHOT
	};

	Camera(void);
	~Camera(void);

	void Init(void);
	void Update(void);
	void SetBeforeDraw(void);
	void Draw(void);

	// カメラ位置
	VECTOR GetPos(void) const;
	// カメラの操作角度
	VECTOR GetAngles(void) const;
	// カメラの注視点
	VECTOR GetTargetPos(void) const;

	// カメラ角度
	Quaternion GetQuaRot(void) const;
	// X回転を抜いたカメラ角度
	Quaternion GetQuaRotOutX(void) const;
	// カメラの前方方向
	VECTOR GetForward(void) const;

	// カメラモードの変更
	void ChangeMode(MODE mode);

	// 追従対象の設定
	void SetFollow(const Transform* follow);

	// 旋回速度を設定するメソッド
	void SetRotationSpeed(float speed) { rotationSpeed_ = speed; }

	// Y軸（左右）の角度を加算する関数
	void AddAngleY(float add) { angles_.y += add; }

	void SetPlayer(Player* player) { player_ = player; }

private:

	Player* player_ = nullptr;

	// カメラの回転中心（遅延追従させる座標）
	VECTOR interpRotationCenter_ = { 0, 0, 0 };

	// 追従の滑らかさ (0.0f ～ 1.0f) : 小さいほどゆっくり付いてくる
	float followLerpRate_ = 0.1f;

	// 遊びの距離（この半径内に機体がいる間はカメラは動かない）
	float followDeadZone_ = 50.0f;

	// 1フレームあたりの回転速度（係数）
	float rotationSpeed_ = 0.0015f;

	// リセット後の操作不能タイマー
	float resetWaitTimer_ = 0.0f;
	// 操作を受け付けない時間（秒） 0.5秒程度が一般的です
	static constexpr float RESET_WAIT_TIME = 0.1f;

	// カメラが追従対象とするTransform
	const Transform* followTransform_;

	// カメラモード
	MODE mode_;

	// カメラの位置
	VECTOR pos_;

	// カメラ角度(rad)
	VECTOR angles_;

	// X軸回転が無い角度
	Quaternion rotOutX_;

	// カメラ角度
	Quaternion rot_;

	// 注視点
	VECTOR targetPos_;

	// カメラの上方向
	VECTOR cameraUp_;

	// カメラを初期位置に戻す
	void SetDefault(void);

	// 追従対象との位置同期を取る
	void SyncFollow(void);

	// カメラ操作
	void ProcessRot(void);

	// モード別更新ステップ
	void SetBeforeDrawFixedPoint(void);
	void SetBeforeDrawFollow(void);
	void SetBeforeDrawSelfShot(void);

};

