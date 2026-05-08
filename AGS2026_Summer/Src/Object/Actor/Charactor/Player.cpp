#include "../../../Application.h"
#include "../../../Manager/InputManager.h"
#include "../../../Manager/ResourceManager.h"
#include "../../../Manager/SceneManager.h"
#include "../../../Manager/Camera.h"
#include "../../../Utility/AsoUtility.h"
#include "../../Common/AnimationController.h"
#include "../../Collider/ColliderLine.h"
#include "../../Collider/ColliderCapsule.h"
#include "Player.h"

Player::Player(void)
    : CharactorBase()
    , state_(STATE::NONE)
    , isDashKeyNew_(false)
    , isBoostAscent_(false)
    , rePressWindowTimer_(0.3f)
    , isDashingBefore_(false)
    , stopTimer_(0.0f)
    , dashResidualTimer_(0.0f)
    , currentTurnSpeed_(DEFAULT_TURN_SPEED)
    , stepRotTime_(0.0f)
{
}

Player::~Player(void)
{
}

void Player::InitLoad(void)
{
    transform_.modelId = resMng_.LoadModelDuplicate(ResourceManager::SRC::PLAYER);
}

void Player::InitTransform(void)
{
    // 初期座標などは環境に合わせて調整してください
    transform_.pos = VGet(0, 50, 0);
    transform_.scl = VGet(1, 1, 1);
    transform_.Update();
    prevPos_ = transform_.pos;
}

void Player::InitCollider(void)
{
    // 地面判定用ライン
    auto line = new ColliderLine(ColliderBase::TAG::PLAYER, &transform_, VGet(0, 10, 0), VGet(0, -10, 0));
    ownColliders_[static_cast<int>(COLLIDER_TYPE::LINE)] = line;

    // 押し戻し用カプセル (以前のPlayerのカプセル設定を反映)
    auto capsule = new ColliderCapsule(ColliderBase::TAG::PLAYER, &transform_, VGet(0, 15, 0), VGet(0, 5, 0), 5.0f);
    ownColliders_[static_cast<int>(COLLIDER_TYPE::CAPSULE)] = capsule;
}

void Player::InitAnimation(void)
{
    animationController_ = new AnimationController(transform_.modelId);
    // モデルに合わせたアニメーション登録をここで行う
}

#include "Player.h"
#include "../../../Manager/InputManager.h"
#include "../../../Manager/Camera.h"
#include "../../../Utility/AsoUtility.h"

// ... (Constructor, Destructor, InitLoad, InitCollider は現状維持)

void Player::UpdateProcess(void)
{
    // 状態がSTOPならタイマー更新のみ
    if (state_ == STATE::STOP) {
        stopTimer_ -= 0.016f;
        if (stopTimer_ <= 0) state_ = STATE::PLAY;
        moveSpeed_ = 0.0f;
        return;
    }

    Input();

    // 1. 旋回処理 (Armored Core風の旋回制限)
    if (VSize(moveDir_) > 0.1f) {
        float targetRot = atan2(moveDir_.x, moveDir_.z);
        // 現在の角度からターゲット角度へ、旋回速度 currentTurnSpeed_ を用いて補間
        transform_.rot.y = AsoUtility::LerpRad(transform_.rot.y, targetRot, currentTurnSpeed_);
    }

    // 2. 移動速度の適用と重力計算
    if (!isJump_) {
        jumpPow_.y = 0.0f; // 接地時は重力リセット
    }
    else {
        jumpPow_.y -= 0.5f; // 重力加速度 (仮値)
    }

    // 最終的な移動ベクトル
    VECTOR velocity = VScale(moveDir_, moveSpeed_);
    velocity.y = jumpPow_.y;

    transform_.pos = VAdd(transform_.pos, velocity);
    transform_.Update();

    ControlAnimation();
}

void Player::UpdateProcessPost(void)
{
}

void Player::InitPost(void)
{
}

void Player::Input(void)
{
    auto& input = InputManager::GetInstance();

    // --- 1. スタック方式による移動方向の取得 ---
    VECTOR inputVec = AsoUtility::VECTOR_ZERO;
    auto hDir = input.GetHorizontalDir();
    auto vDir = input.GetVerticalDir();

    if (vDir == InputManager::MoveDir::Up)    inputVec.z = 1.0f;
    if (vDir == InputManager::MoveDir::Down)  inputVec.z = -1.0f;
    if (hDir == InputManager::MoveDir::Left)  inputVec.x = -1.0f;
    if (hDir == InputManager::MoveDir::Right) inputVec.x = 1.0f;

    // --- 2. カメラ方向に基づいたベクトル変換 ---
    float camRotY = Camera::GetInstance().GetAngles().y;
    moveDir_ = VGet(
        inputVec.x * cos(camRotY) + inputVec.z * sin(camRotY),
        0,
        -inputVec.x * sin(camRotY) + inputVec.z * cos(camRotY)
    );

    // --- 3. 速度とブースト(L1相当)の判定 ---
    if (VSize(moveDir_) > 0.1f) {
        moveDir_ = VNorm(moveDir_);

        // シフトキーまたはパッドのブーストボタン（過去の設計に基づく）
        if (input.IsNew(KEY_INPUT_LSHIFT)) {
            moveSpeed_ = SPEED_RUN; // ブースト走行速度
            dashResidualTimer_ = DASH_RESIDUAL_TIME;
        }
        else {
            if (dashResidualTimer_ > 0.0f) {
                moveSpeed_ = SPEED_RUN;
                dashResidualTimer_ -= 0.016f;
            }
            else {
                moveSpeed_ = SPEED_MOVE; // 通常巡航速度
            }
        }
    }
    else {
        moveSpeed_ = 0.0f;
        dashResidualTimer_ = 0.0f;
    }

    // --- 4. 上昇ブースト挙動 (AC2AA風) ---
    // スペースキーまたはパッドの上昇ボタン
    if (input.IsNew(KEY_INPUT_SPACE)) {
        if (!isJump_) {
            jumpPow_.y = POW_JUMP; // 初動ジャンプ
            isJump_ = true;
        }

        // 長押しでブースト上昇 (空中時)
        jumpPow_.y += BOOSTER_POW;
        if (jumpPow_.y > MAX_ASCENT_SPEED) {
            jumpPow_.y = MAX_ASCENT_SPEED;
        }
        isBoostAscent_ = true;
    }
    else {
        isBoostAscent_ = false;
    }
}

void Player::ControlAnimation(void)
{
    if (!animationController_) return;

    // 以前のロジックに基づいたアニメーション切り替え
    if (isJump_) {
        // animationController_->ChangeAnimation(...) // JUMP
    }
    else if (moveSpeed_ > SPEED_MOVE + 1.0f) {
        // animationController_->ChangeAnimation(...) // FAST_RUN
    }
    else if (moveSpeed_ > 0.1f) {
        // animationController_->ChangeAnimation(...) // RUN
    }
    else {
        // animationController_->ChangeAnimation(...) // IDLE
    }

    animationController_->Update();
}