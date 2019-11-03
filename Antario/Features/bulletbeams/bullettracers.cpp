#include "bullettracers.h"
#include "..\..\Utils\Utils.h"
#include "..\..\SDK\IVEngineClient.h"
#include "..\..\SDK\PlayerInfo.h"
#include "..\..\SDK\ICvar.h"
#include "../../SDK/IGameEvent.h"
#include "../../SDK/CGlobalVarsBase.h"
#include "BeamInfo.hpp"
#include "../../Utils/GlobalVars.h"
#include "../../SDK/IVRenderBeam.h"
#include "../../SDK/IVModelInfo.h"
#include "../../Menu/config.h"
#include "../../Utils/Math.h"

void bullettracers::draw_beam(Vector src, Vector end, Color color) {
	BeamInfo_t info;
	info.m_nType = TE_BEAMPOINTS;
	info.m_pszModelName = "sprites/purplelaser1.vmt";
	info.m_nModelIndex = -1;
	info.m_flHaloScale = 0.0f;
	info.m_flLife = 3.0f;
	info.m_flWidth = 5.0f;
	info.m_flEndWidth = 3.0f;
	info.m_flFadeLength = 0.0f;
	info.m_flAmplitude = 2.0f;
	info.m_flBrightness = color.alpha ;
	info.m_flSpeed = 0.5f;
	info.m_nStartFrame = 0.f;
	info.m_flFrameRate = 0.f;
	info.m_flRed = color.red ;
	info.m_flGreen = color.green;
	info.m_flBlue = color.blue ;
	info.m_nSegments = 2;
	info.m_bRenderable = true;
	info.m_nFlags = FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;
	info.m_vecStart = src;
	info.m_vecEnd = end;

	Beam_t* beam = g_pRenderBeams->CreateBeamPoints(info);
	if (beam)
		g_pRenderBeams->DrawBeam(beam);
}

void bullettracers::events(IGameEvent * event) {
	if (!strcmp(event->GetName(), "bullet_impact")) {
		auto index = g_pEngine->GetPlayerForUserID(event->GetInt("userid"));

		Vector position(event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z"));

		if (index == g_pEngine->GetLocalPlayer()) {
			logs.push_back(trace_info(Globals::LocalPlayer->GetEyePosition(), position, g_pGlobalVars->curtime));

			Color color = Color(g_Math.RandomFloat(0.f, 255.f), g_Math.RandomFloat(0.f, 255.f), g_Math.RandomFloat(0.f, 255.f), 255);
			draw_beam(Globals::LocalPlayer->GetEyePosition(), position, color);
		}
	}
}