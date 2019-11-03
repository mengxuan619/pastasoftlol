#pragma once

#include "..\Aimbot\Autowall.h"
#include "..\Aimbot\Aimbot.h"
#include "..\..\Utils\GlobalVars.h"
#include "..\..\Utils\Math.h"
#include "..\..\SDK\ICvar.h"
#include "..\..\SDK\CPrediction.h"
#include "..\..\Menu\Menu.h"
#include <iostream>
#include <algorithm>
#include "..\..\Menu\config.h"

/*class sound_info
{
public:
	sound_info(Vector positions, float times, int userids)
	{
		this->position = positions;
		this->time = times;
		this->userid = userids;
	}

	Vector position;
	float time;
	int userid;
};
std::vector<sound_info> sound_logs;
std::vector<sound_info> sound_logs_local;
class soundesp
{
public:
	void draw();
	void draw_circle(Color color, Vector position);
};
soundesp g_soundesp;*/

#define _SOLVEY(a, b, c, d, e, f) ((c * b - d * a) / (c * f - d * e))
#define SOLVEY(...) _SOLVEY(?, ?, ?, ?, ?, ?)
#define SOLVEX(y, world, forward, right) ((world.x - right.x * y) / forward.x)
#define Square(x) ((x)*(x))
auto LoadNamedSky = reinterpret_cast<void(__fastcall*)(const char*)>(Utils::FindSignature("engine.dll", "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45"));
//Find out how many ticks player is choking

float GetNetworkLatency()
{
	INetChannelInfo *nci = g_pEngine->GetNetChannelInfo();
	if (nci)
	{
		return nci->GetAvgLatency(FLOW_INCOMING);
	}
	return 0.0f;
}


int GetNumberOfTicksChoked(C_BaseEntity* pEntity)
{
	float flSimulationTime = pEntity->GetSimulationTime();
	float flSimDiff = g_pGlobalVars->curtime - flSimulationTime;
	float latency = GetNetworkLatency();
	return TIME_TO_TICKS(max(0.0f, flSimDiff - latency));
}

void GetChoked() {
	auto NetChannel = g_pEngine->GetNetChannel();

	if (!NetChannel)
		return;

}

class chat_translator : public singleton<chat_translator> {
public:
	auto client_cmd(const char* cmd, std::string& buf) -> int;
	auto dispatch_user_message(int entity_id, const std::string& msg) -> void;
	auto thread_tick() -> void; // call from another thread, for prevent game freezes.
public:
	auto translate(const std::string& text, const std::string& lang)->std::string;

private:
	std::string lang_me_{ "" };
	std::string lang_other_{ "" };

	struct msg_t {
		int entity_id;
		std::string msg;
	};
	std::deque<msg_t> msgs_;
};

enum EChatTranslatorResult {
	ECTR_None,
	ECTR_SetLang,
	ECTR_Translate
};

class Misc
{
public:
    void OnCreateMove()
    {
        this->pCmd   = Globals::pCmd;
        this->pLocal = Globals::LocalPlayer;

		this->DoAutostrafe();
		this->DoBhop();
		this->AutoRevolver();
		if (!g_pEngine->IsVoiceRecording()) {
			this->DoFakeLag();
		}
		this->FakeDuck();
		this->fakeping();
		this->DoSlowWalk();
		this->Animated_Clantag();
    };

	void FakeDuck() {
		static bool counter = false;

		bool once = false;
		if (GetAsyncKeyState(c_config::get().fakeduck_bind))
		{
				unsigned int chokegoal = c_config::get().fakeduck_test;
				auto choke = *(int*)(uintptr_t(g_pClientState) + 0x4D28);
				bool mexican_tryhard = choke >= chokegoal;

				if (Globals::LocalPlayer->GetFlags() & FL_ONGROUND)
				{
					if (mexican_tryhard)
						Globals::pCmd->buttons |= IN_DUCK;
					else
						Globals::pCmd->buttons &= ~IN_DUCK;
				}
			}
	}

	void MovementFix(Vector& oldang)
	{
		Vector vMovements(Globals::pCmd->forwardmove, Globals::pCmd->sidemove, 0.f);

		if (vMovements.Length2D() == 0)
			return;

		Vector vRealF, vRealR;
		Vector aRealDir = Globals::pCmd->viewangles;
		aRealDir.Clamp();

		g_Math.AngleVectors(aRealDir, &vRealF, &vRealR, nullptr);
		vRealF[2] = 0;
		vRealR[2] = 0;

		VectorNormalize(vRealF);
		VectorNormalize(vRealR);

		Vector aWishDir = oldang;
		aWishDir.Clamp();

		Vector vWishF, vWishR;
		g_Math.AngleVectors(aWishDir, &vWishF, &vWishR, nullptr);

		vWishF[2] = 0;
		vWishR[2] = 0;

		VectorNormalize(vWishF);
		VectorNormalize(vWishR);

		Vector vWishVel;
		vWishVel[0] = vWishF[0] * Globals::pCmd->forwardmove + vWishR[0] * Globals::pCmd->sidemove;
		vWishVel[1] = vWishF[1] * Globals::pCmd->forwardmove + vWishR[1] * Globals::pCmd->sidemove;
		vWishVel[2] = 0;

		float a = vRealF[0], b = vRealR[0], c = vRealF[1], d = vRealR[1];
		float v = vWishVel[0], w = vWishVel[1];

		float flDivide = (a * d - b * c);
		float x = (d * v - b * w) / flDivide;
		float y = (a * w - c * v) / flDivide;

		Globals::pCmd->forwardmove = x;
		Globals::pCmd->sidemove = y;
	}
	template<class T, class U>
	T fine(T in, U low, U high)
	{
		if (in <= low)
			return low;

		if (in >= high)
			return high;

		return in;
	}
	void LinearExtrapolations()
	{
		if (c_config::get().fakelag_prediction)
		{
			auto m_local = Globals::LocalPlayer;
			if (m_local && m_local->IsAlive()) {
				for (int i = 1; i < g_pGlobalVars->maxClients; i++)
				{
					auto m_entity = g_pEntityList->GetClientEntity(i);
					if (m_entity && m_entity->is_valid(Globals::LocalPlayer)) {

						float simtime_delta = m_entity->GetSimulationTime() - m_entity->GetOldSimulationTime();
						int choked_ticks = fine(TIME_TO_TICKS(simtime_delta), 1, 15);
						Vector lastOrig;

						if (lastOrig.Length() != m_entity->GetOrigin().Length())
							lastOrig = m_entity->GetOrigin();

						float delta_distance = (m_entity->GetOrigin() - lastOrig).LengthSqr();
						if (delta_distance > 4096.f)
						{
							Vector velocity_per_tick = m_entity->GetVelocity() * g_pGlobalVars->intervalPerTick;
							auto new_origin = m_entity->GetOrigin() + (velocity_per_tick * choked_ticks);
							m_entity->SetAbsOrigin(new_origin);
						}
					}

				}
			}
		}
	}

	void NightMode() {
		static std::string old_Skyname = "";
		static bool OldNightmode;
		static int OldSky;
		if (!g_pEngine->IsConnected() || !g_pEngine->IsInGame() || !Globals::LocalPlayer || !Globals::LocalPlayer->IsAlive())
		{
			old_Skyname = "";
			OldNightmode = false;
			OldSky = 0;
			return;
		}

		static ConVar*r_DrawSpecificStaticProp;
		if (OldNightmode != c_config::get().nightmode)
		{

			r_DrawSpecificStaticProp = g_pCvar->FindVar("r_DrawSpecificStaticProp");
			r_DrawSpecificStaticProp->SetValue(0);

			for (MaterialHandle_t i = g_pMaterialSys->FirstMaterial(); i != g_pMaterialSys->InvalidMaterial(); i = g_pMaterialSys->NextMaterial(i))
			{
				IMaterial* pMaterial = g_pMaterialSys->GetMaterial(i);
				if (!pMaterial)
					continue;
				if (strstr(pMaterial->GetTextureGroupName(), "World") || strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
				{
					if (c_config::get().nightmode) {
						LoadNamedSky("sky_csgo_night02");

						if (strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
							pMaterial->ColorModulate(c_config::get().nightmodeintensity / 100.f, c_config::get().nightmodeintensity / 100.f, c_config::get().nightmodeintensity / 100.f);
						else
							pMaterial->ColorModulate(c_config::get().nightmodeintensity / 220.f, c_config::get().nightmodeintensity / 220.f, c_config::get().nightmodeintensity / 220.f);
					}
					else {
						LoadNamedSky("sky_cs15_daylight04_hdr");
						pMaterial->ColorModulate(1.0f, 1.0f, 1.0f);
					}
				}
			}
			OldNightmode = c_config::get().nightmode;
		}
	}

	/*void SoundEsp(ClientFrameStage_t curStage)
	{
		if (!g_pEngine->IsInGame() || !g_pEngine->IsConnected() || !Globals::LocalPlayer)
			return;
		if (Globals::LocalPlayer->IsAlive() && curStage == FRAME_NET_UPDATE_START)
			if (c_config::get().SoundEsp)
				g_soundesp.draw();
	}*/

	void metalworld() const {
		if (!g_pEngine->IsConnected() || !g_pEngine->IsInGame() || !Globals::LocalPlayer || !Globals::LocalPlayer->IsAlive())
			return; static ConVar* metal = g_pCvar->FindVar("r_showenvcubemap");
		metal->SetValue(c_config::get().metalworld);
	}
	
	void lefthandknife() const {
		if (!g_pEngine->IsConnected() || !g_pEngine->IsInGame() || !Globals::LocalPlayer || !Globals::LocalPlayer->IsAlive())
			return; static ConVar* metal = g_pCvar->FindVar("cl_righthand");
		if (c_config::get().lefthandknife) {
			if (Globals::LocalPlayer->IsKnife())
				metal->SetValue(0);
			else
				metal->SetValue(1);
		}
	}

	void Viewmodel_Shit() const {
		static auto view_x_backup = g_pCvar->FindVar("viewmodel_offset_x")->GetInt();
		static auto view_y_backup = g_pCvar->FindVar("viewmodel_offset_y")->GetInt();
		static auto view_z_backup = g_pCvar->FindVar("viewmodel_offset_z")->GetInt();
		static ConVar* view_x = g_pCvar->FindVar("viewmodel_offset_x");
		static ConVar* view_y = g_pCvar->FindVar("viewmodel_offset_y");
		static ConVar* view_z = g_pCvar->FindVar("viewmodel_offset_z");
		static ConVar* bob = g_pCvar->FindVar("cl_bobcycle"); // sv_competitive_minspec 0

		ConVar* sv_minspec = g_pCvar->FindVar("sv_competitive_minspec");
		*(int*)((DWORD)& sv_minspec->fnChangeCallback + 0xC) = 0; // ew
		sv_minspec->SetValue(0);

		bob->SetValue(0.98f);
		if (c_config::get().customviewmodel) {
			view_x->SetValue(c_config::get().ViewmodelX - 10);
			view_y->SetValue(c_config::get().ViewmodelY - 10);
			view_z->SetValue(c_config::get().ViewmodelZ - 10);
		}
		else {
			view_x->SetValue(1);
			view_y->SetValue(2);
			view_z->SetValue(-2);
		}
	}

	void keystrokes() {
		if (!c_config::get().misc_keystrokes)
			return;

		if (!g_pEngine->IsConnected() || !g_pEngine->IsInGame() || !Globals::LocalPlayer || !Globals::LocalPlayer->IsAlive())
			return;

		std::wstring forward{ L"W" };
		std::wstring back{ L"S" };
		std::wstring left{ L"A" };
		std::wstring right{ L"D" };
		std::wstring walk{ L"SHIFT" };
		std::wstring jump{ L"SPACE" };
		std::wstring duck{ L"CTRL" };

		int forward_x, forward_y, back_x, back_y, left_x, left_y, right_x, right_y, walk_x, walk_y, jump_x, jump_y, duck_x, duck_y;
		g_pSurface->GetTextSize(Globals::keystrokes, forward.c_str(), forward_x, forward_y);
		g_pSurface->GetTextSize(Globals::keystrokes, back.c_str(), back_x, back_y);
		g_pSurface->GetTextSize(Globals::keystrokes, left.c_str(), left_x, left_y);
		g_pSurface->GetTextSize(Globals::keystrokes, right.c_str(), right_x, right_y);
		g_pSurface->GetTextSize(Globals::keystrokes, walk.c_str(), walk_x, walk_y);
		g_pSurface->GetTextSize(Globals::keystrokes, jump.c_str(), jump_x, jump_y);
		g_pSurface->GetTextSize(Globals::keystrokes, duck.c_str(), duck_x, duck_y);

		int w, h;
		g_pEngine->GetScreenSize(w, h);
		int centerW = w / 2;
		int centerh = h / 2;

		if (GetAsyncKeyState(0x57))
		{
			g_pSurface->DrawSetColor(0, 0, 0, 200);
			g_pSurface->DrawFilledRect(52.67f, centerh - 145.f, 52.67f + 52.67f - 10.f, centerh - 120.f);
			g_pSurface->DrawT(64, (centerh - 145), Color(0, 255, 0, 255), Globals::SmallText, true, "W");
		}
		else
		{
			g_pSurface->DrawSetColor(0, 0, 0, 150);
			g_pSurface->DrawFilledRect(52.67f, centerh - 145.f, 52.67f + 52.67f - 10.f, centerh - 120.f);
			g_pSurface->DrawT(64, (centerh - 145), Color(255, 255, 255, 255), Globals::SmallText, true, "W");
		}

		if (GetAsyncKeyState(0x41))
		{
			g_pSurface->DrawSetColor(0, 0, 0, 200);
			g_pSurface->DrawFilledRect(10.f, centerh - 115.f, 52.67f, centerh - 90.f);
			g_pSurface->DrawT(21, (centerh - 115), Color(0, 255, 0, 255), Globals::SmallText, true, "A");
		}
		else
		{
			g_pSurface->DrawSetColor(0, 0, 0, 150);
			g_pSurface->DrawFilledRect(10.f, centerh - 115.f, 52.67f, centerh - 90.f);
			g_pSurface->DrawT(21, (centerh - 115), Color(255, 255, 255, 255), Globals::SmallText, true, "A");
		}
		if (GetAsyncKeyState(0x53))
		{
			g_pSurface->DrawSetColor(0, 0, 0, 200);
			g_pSurface->DrawFilledRect(52.67f, centerh - 115.f, 52.67f + 52.67f - 10.f, centerh - 90.f);
			g_pSurface->DrawT(65, (centerh - 115), Color(0, 255, 0, 255), Globals::SmallText, true, "S");
		}
		else
		{
			g_pSurface->DrawSetColor(0, 0, 0, 150);
			g_pSurface->DrawFilledRect(52.67f, centerh - 115.f, 52.67f + 52.67f - 10.f, centerh - 90.f);
			g_pSurface->DrawT(65, (centerh - 115), Color(255, 255, 255, 255), Globals::SmallText, true, "S");
		}

		if (GetAsyncKeyState(0x44))
		{
			g_pSurface->DrawSetColor(0, 0, 0, 200);
			g_pSurface->DrawFilledRect(52.67f + 52.67f - 10.f, centerh - 115.f, 52.67f + 52.67f + 52.67f - 10.f - 10.f, centerh - 90.f);
			g_pSurface->DrawT(105, (centerh - 115), Color(0, 255, 0, 255), Globals::SmallText, true, "D");
		}
		else
		{
			g_pSurface->DrawSetColor(0, 0, 0, 150);
			g_pSurface->DrawFilledRect(52.67f + 52.67f - 10.f, centerh - 115.f, 52.67f + 52.67f + 52.67f - 10.f - 10.f, centerh - 90.f);
			g_pSurface->DrawT(105, (centerh - 115), Color(255, 255, 255, 255), Globals::SmallText, true, "D");
		}
		if (GetAsyncKeyState(VK_SPACE))
		{
			g_pSurface->DrawSetColor(0, 0, 0, 200);
			g_pSurface->DrawFilledRect(10, centerh - 85, 138, centerh - 60);
			g_pSurface->DrawT((138 - duck_x) / 2, (centerh - ((145 + duck_y) / 2)), Color(0, 255, 0, 255), Globals::SmallText, true, "SPACE");
		}
		else
		{
			g_pSurface->DrawSetColor(0, 0, 0, 150);
			g_pSurface->DrawFilledRect(10, centerh - 85, 138, centerh - 60);
			g_pSurface->DrawT((138 - duck_x) / 2, (centerh - ((145 + duck_y) / 2)), Color(255, 255, 255, 255), Globals::SmallText, true, "SPACE");
		}
		if (GetAsyncKeyState(VK_SHIFT))
		{
			g_pSurface->DrawSetColor(0, 0, 0, 200);
			g_pSurface->DrawFilledRect(10, centerh - 55, 138, centerh - 30);
			g_pSurface->DrawT((138 - duck_x) / 2, (centerh - ((85 + duck_y) / 2)), Color(0, 255, 0, 255), Globals::SmallText, true, "SHIFT");
		}
		else
		{
			g_pSurface->DrawSetColor(0, 0, 0, 150);
			g_pSurface->DrawFilledRect(10, centerh - 55, 138, centerh - 30);
			g_pSurface->DrawT((138 - duck_x) / 2, (centerh - ((85 + duck_y) / 2)), Color(255, 255, 255, 255), Globals::SmallText, true, "SHIFT");
		}
		if (GetAsyncKeyState(VK_LCONTROL))
		{
			g_pSurface->DrawSetColor(0, 0, 0, 200);
			g_pSurface->DrawFilledRect(10, centerh - 25, 138, centerh);
			g_pSurface->DrawT((138 - duck_x) / 2, (centerh - ((25 + duck_y) / 2)), Color(0, 255, 0, 255), Globals::SmallText, true, "CTRL");
		}
		else
		{
			g_pSurface->DrawSetColor(0, 0, 0, 150);
			g_pSurface->DrawFilledRect(10, centerh - 25, 138, centerh);
			g_pSurface->DrawT((138 - duck_x) / 2, (centerh - ((25 + duck_y) / 2)), Color(255, 255, 255, 255), Globals::SmallText, true, "CTRL");
		}
	}

	void AsusProps() {
		static std::string old_Skyname = "";
		static bool OldNightmode;
		static int OldSky;

		if (!g_pEngine->IsConnected() || !g_pEngine->IsInGame() || !Globals::LocalPlayer || !Globals::LocalPlayer->IsAlive())
		{
			old_Skyname = "";
			OldNightmode = false;
			OldSky = 0;
			return;
		}

		if (OldNightmode != c_config::get().transparent_props)
		{
			for (MaterialHandle_t i = g_pMaterialSys->FirstMaterial(); i != g_pMaterialSys->InvalidMaterial(); i = g_pMaterialSys->NextMaterial(i))
			{
				IMaterial* pMaterial = g_pMaterialSys->GetMaterial(i);
				if (!pMaterial)
					continue;
				if (strstr(pMaterial->GetTextureGroupName(), "StaticProp textures"))
				{
					if (c_config::get().transparent_props) {
						pMaterial->AlphaModulate(0.7f);
					}
					else {
						pMaterial->AlphaModulate(1.f);
					}
						
				}
			}
			OldNightmode = c_config::get().transparent_props;
		}

	}
	void SwapManual()
	{
		if (GetKeyState(c_config::get().manual_swap_bind))
		{
			Globals::Manual_Side = true;
		}
		else {
			Globals::Manual_Side = false;
		}
	}
	void showSpread()
	{
		if (!c_config::get().draw_spread)
			return;
		auto local_player = Globals::LocalPlayer;

		if (!local_player)
			return;

		if (!local_player->IsAlive())
			return;
		auto weapon = local_player->GetActiveWeapon();

		if (!weapon)
			return;

		float spread = weapon->GetInaccuracy() * 500;

		if (spread == 0.f)
			return;

		int screenSizeX, screenCenterX;
		int screenSizeY, screenCenterY;
		g_pEngine->GetScreenSize(screenSizeX, screenSizeY);

		float cx = screenSizeX / 2.f;
		float cy = screenSizeY / 2.f;

		g_pSurface->DrawSetColor(255, 255, 255, 255);
		g_pSurface->DrawOutlinedCircle(cx, cy, spread, 35);
	}
	void ThirdPerson()
	{
		if (!g_pEngine->IsInGame() || !g_pEngine->IsConnected() || !Globals::LocalPlayer)
			return;
			
		static bool init = false;
		static bool set_angle = false;
		auto pLocalEntity = Globals::LocalPlayer;

		static int stored_thirdperson_distance;

		if (stored_thirdperson_distance != c_config::get().thirdperson_distance) {
			std::string command; command += "cam_idealdist "; command += std::to_string(c_config::get().thirdperson_distance + 30);
			g_pEngine->ExecuteClientCmd(command.c_str());

			stored_thirdperson_distance = c_config::get().thirdperson_distance;
		}
		static Vector vecAngles;
		g_pEngine->GetViewAngles(vecAngles);

		if (GetKeyState(c_config::get().thirdperson_bind) && Globals::LocalPlayer->IsAlive())
		{
			if (init)
			{
				ConVar* sv_cheats = g_pCvar->FindVar("sv_cheats");
				*(int*)((DWORD)&sv_cheats->fnChangeCallback + 0xC) = 0; // ew
				sv_cheats->SetValue(1);
				g_pEngine->ExecuteClientCmd("thirdperson");

				std::string command; command += "cam_idealdist "; command += std::to_string(c_config::get().thirdperson_distance + 30);
				g_pEngine->ExecuteClientCmd(command.c_str());
			}
			init = false;
		}
		else
		{
			if (!init)
			{
				ConVar* sv_cheats = g_pCvar->FindVar("sv_cheats");
				*(int*)((DWORD)&sv_cheats->fnChangeCallback + 0xC) = 0; // ew
				sv_cheats->SetValue(1);
				g_pEngine->ExecuteClientCmd("firstperson");
			}
			init = true;
		}


	}

	void Thirdperson_FSN(ClientFrameStage_t curStage) {
		if (curStage == FRAME_RENDER_START && g_GameInput->m_fCameraInThirdPerson && Globals::LocalPlayer && Globals::LocalPlayer->IsAlive())
		{
				g_pPrediction->SetLocalViewAngles(Vector(Globals::RealAngle.x, Globals::RealAngle.y, 0));
		}
	}
	void NormalWalk()
	{

		Globals::pCmd->buttons &= ~IN_MOVERIGHT;
		Globals::pCmd->buttons &= ~IN_MOVELEFT;
		Globals::pCmd->buttons &= ~IN_FORWARD;
		Globals::pCmd->buttons &= ~IN_BACK;

		if (Globals::pCmd->forwardmove > 0.f)
			Globals::pCmd->buttons |= IN_FORWARD;
		else if (Globals::pCmd->forwardmove < 0.f)
			Globals::pCmd->buttons |= IN_BACK;
		if (Globals::pCmd->sidemove > 0.f)
		{
			Globals::pCmd->buttons |= IN_MOVERIGHT;
		}
		else if (Globals::pCmd->sidemove < 0.f)
		{
			Globals::pCmd->buttons |= IN_MOVELEFT;
		}

	}


	void NoRecoil(CUserCmd* cmd)
	{
		if (!c_config::get().aimbot_norecoil)
			return;

		auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
		if (!local_player)
			return;

		auto weapon = local_player->GetActiveWeapon();
		if (weapon)
			weapon->GetAccuracyPenalty();

		cmd->viewangles -= local_player->GetAimPunchAngle() * 2;
	}

	void SetClan(const char* tag, const char* name) {
		static auto pSetClanTag = reinterpret_cast<void(__fastcall*)(const char*, const char*)>(((DWORD)Utils::FindPattern("engine.dll", (PBYTE)"\x53\x56\x57\x8B\xDA\x8B\xF9\xFF\x15\x00\x00\x00\x00\x6A\x24\x8B\xC8\x8B\x30", "xxxxxxxxx????xxxxxx")));
		pSetClanTag(tag, name);
	}


	void Animated_Clantag() {
		if (!c_config::get().misc_clantag)
			return;

		static int counter = 0;
		static std::string clantag = "gamesense ";
		if (++counter > 25) {
			std::rotate(clantag.begin(), clantag.begin() + 1, clantag.end());
			SetClan(clantag.c_str(), clantag.c_str());
			counter = 0;
		}
	}

	void fakeping()
	{
		if (c_config::get().fakelagspoof)
		{
			{
				ConVar* net_fakelag = g_pCvar->FindVar("net_fakelag");
				if (c_config::get().fakelagspoof);
				net_fakelag->SetValue(c_config::get().fakelagspoof);
			}
		}
		else
		{
			{
				ConVar* net_fakelag = g_pCvar->FindVar("net_fakelag");
				if (c_config::get().fakelagspoof);
				net_fakelag->SetValue(c_config::get().fakelagspoof);
			}
		}
	}

	void AutoRevolver()
	{
		auto me = Globals::LocalPlayer;
		auto cmd = Globals::pCmd;
		auto weapon = me->GetActiveWeapon();

		if (!c_config::get().autorevolver)
			return;

		if (!me || !me->IsAlive() || !weapon)
			return;

		if (weapon->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_REVOLVER)
		{
			static int delay = 0; /// pasted delay meme from uc so we'll stop shooting on high ping
			delay++;

			if (delay <= 15)
				Globals::pCmd->buttons |= IN_ATTACK;
			else
				delay = 0;
		}
	}

	void slow_walk(CUserCmd *cmd)
	{
		if (!GetAsyncKeyState(VK_SHIFT))
			return;

		if (!Globals::LocalPlayer)
			return;

		auto weapon_handle = Globals::LocalPlayer->GetActiveWeapon();

		if (!weapon_handle)
			return;

		Vector velocity = Globals::LocalPlayer->GetVelocity();
		Vector direction = velocity.Angle();
		float speed = velocity.Length();

		direction.y = cmd->viewangles.y - direction.y;

		Vector negated_direction = direction * -speed;
		if (velocity.Length() >= (weapon_handle->GetCSWpnData()->max_speed * .34f))
		{
			cmd->forwardmove = negated_direction.x;
			cmd->sidemove = negated_direction.y;
		}
	}

	void MinWalk(CUserCmd* get_cmd, float get_speed) const
	{
		if (get_speed <= 0.f)
			return;

		float min_speed = (float)(FastSqrt(Square(get_cmd->forwardmove) + Square(get_cmd->sidemove) + Square(get_cmd->upmove)));
		if (min_speed <= 0.f)
			return;

		if (get_cmd->buttons & IN_DUCK)
			get_speed *= 2.94117647f;

		if (min_speed <= get_speed)
			return;

		float kys = get_speed / min_speed;

		get_cmd->forwardmove *= kys;
		get_cmd->sidemove *= kys;
		get_cmd->upmove *= kys;
	}
	void DoSlowWalk() const
	{
		if (c_config::get().slowwalk_bind > 0 && !GetAsyncKeyState(c_config::get().slowwalk_bind) || c_config::get().slowwalk_bind <= 0)
		{
			return;
		}

		MinWalk(pCmd, c_config::get().slowwalk_speed);
	}


private:
    CUserCmd*     pCmd;
    C_BaseEntity* pLocal;

    void DoBhop() const
    {
        if (!c_config::get().misc_bhop)
            return;

        static bool bLastJumped = false;
        static bool bShouldFake = false;

        if (!bLastJumped && bShouldFake)
        {
            bShouldFake = false;
            pCmd->buttons |= IN_JUMP;
        }
        else if (pCmd->buttons & IN_JUMP)
        {
            if (pLocal->GetFlags() & FL_ONGROUND)
                bShouldFake = bLastJumped = true;
            else 
            {
                pCmd->buttons &= ~IN_JUMP;
                bLastJumped = false;
            }
        }
        else
            bShouldFake = bLastJumped = false;
    }

	void DoAutostrafe() const
	{
		if (!Globals::LocalPlayer || !g_pEngine->IsConnected() || !g_pEngine->IsInGame() || !c_config::get().misc_autostrafe)
			return;

		if (!Globals::LocalPlayer->IsAlive())
			return;

		if (!(Globals::LocalPlayer->GetFlags() & FL_ONGROUND) && GetAsyncKeyState(VK_SPACE))
		{
			pCmd->forwardmove = (10000.f / Globals::LocalPlayer->GetVelocity().Length2D() > 450.f) ? 450.f : 10000.f / Globals::LocalPlayer->GetVelocity().Length2D();
			pCmd->sidemove = (pCmd->mousedx != 0) ? (pCmd->mousedx < 0.0f) ? -450.f : 450.f : (pCmd->command_number % 2) == 0 ? -450.f : 450.f;	
		}
	}
	int FakeLagVariance() const
	{
		static int factor;
		int maxvalue;

		if (c_config::get().variance > 0) {

			int variance = 0;

			variance = rand() % c_config::get().variance / 7.142857143;

			if (variance > 14) { // cool
				variance = 14;
			}

			return variance;
		}
		else {
			return c_config::get().fakelag;
		}
	}
	void DoFakeLag()
	{
		if (!Globals::LocalPlayer || !g_pEngine->IsConnected() || !g_pEngine->IsInGame() || (c_config::get().fakelag == 0 && !(GetAsyncKeyState(c_config::get().fakeduck_bind) && c_config::get().fakeduck_bind > 1)) || g_Menu.Config.LegitBacktrack)
			return;
		
		if (!Globals::LocalPlayer->IsAlive() || Globals::LocalPlayer->IsNade())
			return;

		auto NetChannel = g_pEngine->GetNetChannel();

		if (!NetChannel)
			return;
		
		if (GetAsyncKeyState(c_config::get().fakeduck_bind) && c_config::get().fakeduck_bind > 1)
		{
			Globals::bSendPacket = (14 <= *(int*)(uintptr_t(g_pClientState) + 0x4D28));
		}
		else {
			Globals::bSendPacket = (NetChannel->m_nChokedPackets >= FakeLagVariance());
		}
		
	}
};

extern Misc g_Misc;

