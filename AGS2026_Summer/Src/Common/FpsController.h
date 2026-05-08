#pragma once
#include <vector>
#include <chrono>
#include <string>
#include <DxLib.h>

class FpsController
{

public:

    // コンストラクタ
    // ( DxLib_Init前に呼ぶこと )
    FpsController(int fixedFps);

    // デストラクタ
    ~FpsController(void);

    // 1フレームごとのFPS制御
    // ( ScreenFlip後に呼ぶこと ）
    void Wait(void);

    // 画面にFPSを表示
    void Draw(void);

    // 現在のFPSを取得
    float GetFPS(void) const { return fps_; }

private:

    // 最大FPS
    const int MAX_FPS = 1200;

    // 平均FPS計算に使用するフレーム数
    const int AVG_FPS_COUNT = 60;

    // 平均FPSの描画色
    const unsigned int COLOR = GetColor(255, 255, 255);

    // 平均FPSの右上描画位置調整
    const int MARGIN = 20;

    // 描画フォーマット
    const std::string TEXT_FORMAT = "FPS : %.2f";

    // 指定された固定フレームレート
    const int fixedFps_;

    // 1フレームの理想時間(秒)
    const double idealFrameTime_;

    // 計測用FPS
    float fps_;

    // 平均FPS計測用)(秒単位)
    std::vector<double> timeList_;

    // 前フレームの時間
    std::chrono::high_resolution_clock::time_point prevTime_;

};
