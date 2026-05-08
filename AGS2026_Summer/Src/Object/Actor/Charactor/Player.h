#pragma once
#include <map>
#include <DxLib.h>
#include "CharactorBase.h"

class Player : public CharactorBase
{
public:
    // 以前の定数をそのまま使用
    static constexpr float SPEED_MOVE = 10.0f;
    static constexpr float SPEED_RUN = 20.0f;
    static constexpr float POW_JUMP = 20.0f;
    static constexpr float TIME_JUMP_IN = 0.5f;
    static constexpr float BOOSTER_POW = 1.0f;
    static constexpr float MAX_ASCENT_SPEED = 10.0f;
    static constexpr float DEFAULT_TURN_SPEED = 0.1f;

    enum class STATE 
    {
        NONE, PLAY, STOP, WARP_RESERVE, WARP_MOVE, DEAD, VICTORY, END
    };

    enum class ANIM_TYPE 
    {
        IDLE, RUN, FAST_RUN, JUMP, WARP_PAUSE, // 必要に応じて追加
    };

    Player(void);
    virtual ~Player(void) override;

protected:
    virtual void InitLoad(void) override;
    virtual void InitTransform(void) override;
    virtual void InitCollider(void) override;
    virtual void InitAnimation(void) override;
    virtual void UpdateProcess(void) override;
    virtual void UpdateProcessPost(void) override;
    virtual void InitPost(void) override;

private:
    void Input(void);
    void ControlAnimation(void);

    STATE state_;

    // 以前の挙動に必要な変数群
    bool isDashKeyNew_;
    bool isBoostAscent_;
    float rePressWindowTimer_;
    bool isDashingBefore_;
    float stopTimer_;
    float dashResidualTimer_;
    const float DASH_RESIDUAL_TIME = 0.4f;
    const float STOP_TIME = 0.5f;

    float currentTurnSpeed_;
    Quaternion playerRotY_;
    Quaternion goalQuaRot_;
    float stepRotTime_;
};