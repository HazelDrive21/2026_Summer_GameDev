#pragma once
#include "Bullet.h"

// 剣撃判定用のクラス
class BladeBullet : public Bullet
{
public:
    // 弾丸のコンストラクタを利用しつつ、寿命を短く設定
    BladeBullet(const VECTOR& pos, const VECTOR& velocity, int damage)
        : Bullet(pos, velocity, damage, 5, false, 60.0f, GetColor(0, 255, 255)) // 寿命5フレーム、半径60
    {
    }
};