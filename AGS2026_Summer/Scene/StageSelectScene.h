#pragma once
#include "SceneBase.h"

class StageSelectScene : public SceneBase
{
public:
    StageSelectScene(void);
    virtual ~StageSelectScene(void);

    void Init(void) override;
    void Update(void) override;
    void Draw(void) override;
};