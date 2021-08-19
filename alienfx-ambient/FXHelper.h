#pragma once
#include "ConfigHandler.h"
#include "toolkit.h"

class FXHelper: public FXH<ConfigHandler>
{
public:
	using FXH::FXH;
	int Refresh(UCHAR* img);
	void FadeToBlack();
	void ChangeState();
};

