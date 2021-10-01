#pragma once
#include "ConfigAmbient.h"
#include "toolkit.h"

class FXHelper: public FXH<ConfigAmbient>
{
public:
	using FXH::FXH;
	int Refresh(UCHAR* img);
	void FadeToBlack();
	void ChangeState();
};

