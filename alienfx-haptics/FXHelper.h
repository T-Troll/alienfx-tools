#pragma once
#include "ConfigHaptics.h"
#include "toolkit.h"

class FXHelper: public FXH<ConfigHaptics>
{
private:
	int* freq;
public:
	using FXH::FXH;
	void Refresh(int* freq);
	void FadeToBlack();
};

