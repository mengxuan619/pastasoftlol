#pragma once

class Indicators
{
public:
	void test();
	struct
	{
		bool Aimbot;
		int Hitchance;
		int HitchanceValue;
		int Mindmg;
		bool Resolver;
		int BodyScale;
		int HeadScale;
		bool MultiPoint;
		bool DelayShot;
		bool IgnoreLimbs;
		bool Autostop;
		bool OneTickChoke;
		bool BaimLethal;
		bool BaimPitch;
		bool BaimInAir;

		bool Antiaim;
		bool DesyncAngle;
		bool RandJitterInRange;
		int	JitterRange;
		int	Fakelag;
		bool FakeLagOnPeek;
		bool ChokeShotOnPeek;

		bool Esp;
		int Name;
		int HealthVal;
		int Weapon;
		bool Box;
		bool HealthBar;
		bool HitboxPoints;
		bool NoZoom;
		int Fov;
		bool Crosshair;

		bool Bhop;
		bool AutoStrafe;
		bool LegitBacktrack;
		bool Ak47meme;
		bool RemoveScope;
		int	Test;
	};

private:
	Vector _pos = Vector(500, 200, 0);
	int ControlsX;
	int GroupTabBottom;
	int OffsetY;
	int screen_width;
	int screen_height;
	int y_offset;
	int x_offset;
	int MenuAlpha_Main;
	int MenuAlpha_Text;
	int groupbox_scroll_add;
	int groupbox_width;
	int groupbox_bottom;
	int groupbox_top;
	bool we_are_clipping;
	int how_many_controls;
	typedef void(*ButtonCallback_t)(void);
}; 
