#pragma once
#include "SceneBase.h"

class WeaponSelectScene : public SceneBase
{
public:
	WeaponSelectScene(void);
	virtual ~WeaponSelectScene(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;
};