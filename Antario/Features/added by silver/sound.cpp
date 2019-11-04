#include "sound.hpp"

c_sound_esp sound_esp;

std::vector<c_sound_info> sound_logs;

void c_sound_esp::draw_circle(color colors, vec3_t position) noexcept {
	BeamInfo_t beam_info;
	beam_info.m_nType = TE_BEAMRINGPOINT;
	beam_info.m_pszModelName = "sprites/purplelaser1.vmt";
	beam_info.m_nModelIndex = g_pModelInfo->GetModelIndex("sprites/purplelaser1.vmt");
	beam_info.m_nHaloIndex = -1;
	beam_info.m_flHaloScale = 5;
	beam_info.m_flLife = .50f;
	beam_info.m_flWidth = 10.f;
	beam_info.m_flFadeLength = 1.0f;
	beam_info.m_flAmplitude = 0.f;
	beam_info.m_flRed = colors.r;
	beam_info.m_flGreen = colors.g;
	beam_info.m_flBlue = colors.b;
	beam_info.m_flBrightness = colors.a;
	beam_info.m_flSpeed = 0.f;
	beam_info.m_nStartFrame = 0.f;
	beam_info.m_flFrameRate = 60.f;
	beam_info.m_nSegments = -1;
	beam_info.m_nFlags = FBEAM_SHADEIN; //FBEAM_FADEOUT
	beam_info.m_vecCenter = position + vec3_t(0, 0, 5);
	beam_info.m_flStartRadius = 20.f;
	beam_info.m_flEndRadius = 640.f;

	auto beam = g_pRenderBeams->CreateBeamRingPoint(beam_info);
	if (beam)
		g_pRenderBeams->DrawBeam(beam);
}

void c_sound_esp::event_player_footstep(IGameEvent * event) noexcept {
	if (c_config::get().sound_footstep)
		return;

	if (g_pEngine->IsConnected() && g_pEngine->IsInGame())
		return;

	if (!event)
		return;

	auto local_player = reinterpret_cast<C_BaseEntity*>(g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer()));

	if (!local_player)
		return;

	auto walker = reinterpret_cast<C_BaseEntity*>(g_pEntityList->GetClientEntity(g_pEngine->GetPlayerForUserID(event->GetInt("userid"))));

	if (!walker)
		return;

	if (walker->IsDormant()) {
		return;
	}

	static int timer;
	timer += 1;

	if (timer > 1)
		timer = 0;

	if (walker->GetTeam() != local_player->GetTeam()) {
		if (walker && timer < 1) {
			sound_logs.push_back(c_sound_info(walker->GetAbsOrigin(), g_pGlobalVars->curtime, g_pEngine->GetPlayerForUserID(event->GetInt("userid"))));
		}
	}

}

void c_sound_esp::event_player_hurt(IGameEvent * event) noexcept {
	if (c_config::get().sound_footstep)
		return;

	if (g_pEngine->IsConnected() && g_pEngine->IsInGame())
		return;

	if (!event)
		return;

	auto local_player = reinterpret_cast<C_BaseEntity*>(g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer()));

	if (!local_player)
		return;

	auto attacker = g_pEntityList->GetClientEntity(g_pEngine->GetPlayerForUserID(event->GetInt("attacker")));

	if (!attacker)
		return;

	auto victim = reinterpret_cast<C_BaseEntity*>(g_pEntityList->GetClientEntity(g_pEngine->GetPlayerForUserID(event->GetInt("userid"))));

	if (!victim)
		return;

	static int timer;

	timer += 1;

	if (timer > 2)
		timer = 0;

	if (attacker == local_player) {
		if (timer < 1) {
			sound_logs.push_back(c_sound_info(victim->GetAbsOrigin(), g_pGlobalVars->curtime, event->GetInt("userid")));
		}
	}
}

void c_sound_esp::draw() noexcept {
	if (c_config::get().sound_footstep)
		return;

	if (g_pEngine->IsConnected() && g_pEngine->IsInGame())
		return;

	auto red = c_config::get().clr_footstep_r * 255;
	auto green = c_config::get().clr_footstep_g * 255;
	auto blue = c_config::get().clr_footstep_b * 255;
	auto alpha = c_config::get().clr_footstep_a * 255;

	for (unsigned int i = 0; i < sound_logs.size(); i++) {
		draw_circle(color(red, green, blue, alpha), sound_logs[i].position);
		sound_logs.erase(sound_logs.begin() + i);
	}
}