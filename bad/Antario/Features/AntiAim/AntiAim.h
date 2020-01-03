#pragma once
#include "..\..\Utils\GlobalVars.h"
#include "..\..\SDK\CGlobalVarsBase.h"

class AntiAim
{
public:
	void OnCreateMove();
	float MaxDelta(C_BaseEntity* pEnt);

private:

};
extern AntiAim g_AntiAim;