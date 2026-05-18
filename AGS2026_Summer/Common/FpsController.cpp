#include "../Application.h"
#include "FpsController.h"

FpsController::FpsController(int fixedFps)
    :
    // 最大FPSを超えないように制限
    fixedFps_(fixedFps > MAX_FPS ? MAX_FPS : fixedFps),
    // 1フレームの理想時間を計算
    idealFrameTime_(1.0f / static_cast<double>(fixedFps_)),
    fps_(0.0f),
    timeList_(),
    prevTime_()
{

    prevTime_ = std::chrono::high_resolution_clock::now();

    // DxLibの垂直同期待ちを無効化
    SetWaitVSyncFlag(false);

}

FpsController::~FpsController(void)
{
}

void FpsController::Wait()
{

    // 現在時間
    auto nowTime = std::chrono::high_resolution_clock::now();

    // 前回からの経過時間
    std::chrono::duration<double> delta = nowTime - prevTime_;

    // 経過時間(秒)
    double deltaTime = delta.count();

    // 経過時間が理想時間よりも短ければ待機
    if (deltaTime < idealFrameTime_)
    {

        // 待つべき時間(ミリ秒)
        double waitMiliSecond = (idealFrameTime_ - deltaTime) * 1000.0;

        // Sleepで待ち時間分を待機
        if (waitMiliSecond >= 1.0)
        {
            // 指定ミリ秒数待つ(DxLib関数)
            WaitTimer(static_cast<int>(waitMiliSecond));

        }

        // 指定時間になるまでbusyになるが待つ
        while (deltaTime < idealFrameTime_)
        {
            // 再計測
            nowTime = std::chrono::high_resolution_clock::now();
            delta = nowTime - prevTime_;
            deltaTime = delta.count();
        }

    }

    // 前回時間を更新
    prevTime_ = nowTime;

    // FPS計測(指定された最新フレーム数分の平均）
    timeList_.emplace_back(deltaTime);
    if (timeList_.size() > AVG_FPS_COUNT)
    {
        timeList_.erase(timeList_.begin());
    }

    // 合計時間
    double total = 0.0;
    for (double time : timeList_)
    {
        total += time;
    }

    // 平均FPS
    fps_ = static_cast<float>(timeList_.size() / total);

}

void FpsController::Draw()
{

    // 描画する文字列の幅を取得(デフォルトフォントの場合)
    int textWidth = GetDrawFormatStringWidth(TEXT_FORMAT.c_str(), fps_);

    // 右上位置を計算
    int x = Application::SCREEN_SIZE_X - textWidth - MARGIN;
    int y = MARGIN;

    // 右寄せ描画
    DrawFormatString(x, y, COLOR, TEXT_FORMAT.c_str(), fps_);

}
