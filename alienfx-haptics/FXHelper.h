#pragma once
#include "ConfigHandler.h"
#include "toolkit.h"

class FXHelper: public FXH<ConfigHandler>
{
private:
	int* freq;
public:
	using FXH::FXH;
	void Refresh(int* freq);
	void FadeToBlack();
};

