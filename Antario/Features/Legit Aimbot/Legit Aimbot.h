#pragma once
#include "..\..\Utils\GlobalVars.h"
#include "..\..\SDK\CGlobalVarsBase.h"
#include "..\..\Utils/Math.h"
#include <deque>

struct lbyRecords
{
	int tick_count;
	float lby;
	Vector headPosition;
};
struct backtrackData
{
	float simtime;
	Vector hitboxPos;
};

extern backtrackData headPositions[64][12];

class LegitAimbot {
private:
	Vector HitBoxPosition(C_BaseEntity* entity, int hitbox_id);
	bool get_weapon_settings(C_BaseCombatWeapon* weapon);
	void auto_pistol();
	int find_target();
public:
	bool SilentCheck();
	void OnCreateMove();
	void CM_Backtrack();
	float GetFov();
	float GetSmooth();
	float GetHitbox();
};

extern LegitAimbot g_LegitAimbot;