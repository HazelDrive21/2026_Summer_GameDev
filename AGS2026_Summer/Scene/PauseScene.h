#pragma once
#include "SceneBase.h"

class PauseScene : public SceneBase
{
public:
	PauseScene(void);
	virtual ~PauseScene(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;
};