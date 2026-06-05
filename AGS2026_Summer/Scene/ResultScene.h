#pragma once
#include "SceneBase.h"

class ResultScene : public SceneBase
{
public:
    ResultScene(void);
    virtual ~ResultScene(void);

    void Init(void) override;
    void Update(void) override;
    void Draw(void) override;
};