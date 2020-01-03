#include "../include_cheat.h"
#ifndef NOMINMAX2

#ifndef max2
#define max2(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min2
#define min2(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX2 */
#define DEBUG
#define RELEASE


int missed_shots[64];
std::vector<float>brutelist
{
	118.f,
	-120.f,
	180.f,
	-30.f,
	98,
	-2,
	+150.f,
	-119.f
};
float NormalizeYaw180(float yaw)
{
	if (yaw > 180)
		yaw -= (round(yaw / 360) * 360.f);
	else if (yaw < -180)
		yaw += (round(yaw / 360) * -360.f);

	return yaw;
}


float angle_difference1(float a, float b) {
	auto diff = NormalizeYaw180(a - b);

	if (diff < 180)
		return diff;
	return diff - 360;
}

float approach(float cur, float target, float inc) {
	inc = abs(inc);

	if (cur < target)
		return min2(cur + inc, target);
	if (cur > target)
		return max2(cur - inc, target);

	return target;
}

float angle_difference(float a, float b) {
	auto diff = NormalizeYaw180(a - b);

	if (diff < 180)
		return diff;
	return diff - 360;
}

float approach_angle(float cur, float target, float inc) {
	auto diff = angle_difference(target, cur);
	return approach(cur, cur + diff, inc);
}



bool delta_58(float first, float second)
{
	if (first - second < 58.f && first - second > -58.f)
	{
		return true;
	}
	return false;
}

bool delta_35(float first, float second)
{
	if (first - second <= 35.f && first - second >= -35.f)
	{
		return true;
	}
	return false;
}

bool breaking_lby_animations(C_CSPlayer* e)
{
	if (!e || e->IsDormant() || !e->get_alive())
		return false;

	for (size_t i = 0; i < e->get_anim_layer_count(); i++)
	{
		auto layer = e->get_anim_overlay_index(i);
		if (e->get_sequence_act(layer->m_nSequence) == 979)
		{
			if (layer->m_flCycle != layer->m_flCycle || layer->m_flWeight == 1.f)
				return true;
		}
	}

	return false;
}

bool solve_desync_simple(C_CSPlayer* e) // 979
{
	if (!e || e->IsDormant() || !e->get_alive())
		return false;

	for (size_t i = 0; i < e->GetNumAnimOverlays(); i++)
	{
		auto layer = e->get_anim_overlay_index(i);
		if (!layer)
			continue;

		if (e->get_sequence_act(layer->m_nSequence) == 979)
		{
			if (layer->m_flWeight == 0.0f && (layer->m_flCycle == 0.0f || layer->m_flCycle != layer->m_flPrevCycle))
				return true;
		}
	}
	return false;
}

void resolver::AnimationFix(C_CSPlayer* pEnt)
{


	if (pEnt) {
		pEnt->ClientAnimations(true);
		auto player_animation_state = pEnt->get_anim_state();
		player_animation_state->m_flLeanAmount = 94;
		player_animation_state->m_flCurrentTorsoYaw += 28;
	 pEnt->update_clientside_anim();
		pEnt->set_abs_angles(Vector(0, player_animation_state->m_flGoalFeetYaw, 0));
		pEnt->ClientAnimations(false);
	}
	else {
		auto player_index = pEnt->EntIndex() - 1;

		pEnt->ClientAnimations(true);

		auto old_curtime = g_pGlobals->curtime;
		auto old_frametime = g_pGlobals->frametime;

		g_pGlobals->curtime = pEnt->get_simtime();
		g_pGlobals->frametime = g_pGlobals->interval_per_tick;

		auto player_animation_state = pEnt->get_anim_state();
		auto player_model_time = reinterpret_cast<int*>(player_animation_state + 112);
		if (player_animation_state != nullptr && player_model_time != nullptr)
			if (*player_model_time == g_pGlobals->framecount)
				*player_model_time = g_pGlobals->framecount - 1;


		pEnt->update_clientside_anim();

		g_pGlobals->curtime = old_curtime;
		g_pGlobals->frametime = old_frametime;

		pEnt->set_abs_angles(Vector(0, player_animation_state->m_flGoalFeetYaw, 0));

		pEnt->ClientAnimations(false);

	}

}
float flAngleModx(float flAngle)
{
	return((360.0f / 65536.0f) * ((int32_t)(flAngle * (65536.0f / 360.0f)) & 65535));
}
float ApproachAnglex(float target, float value, float speed)
{
	target = flAngleModx(target);
	value = flAngleModx(value);

	float delta = target - value;
	if (speed < 0)
		speed = -speed;

#pragma region release nn shit
#ifdef RELEASE
	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;
#endif

#pragma region debug nn shit
#ifdef DEBUG
	if (delta > 180)
		delta -= 360;
	else if (delta < 180)
		delta += 360;
#endif

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}

void resolver::resolve(C_CSPlayer* player)
{
	

	__(weapon_accuracy_nospread, "weapon_accuracy_nospread");

	static auto nospread = g_pCVar->FindVar(weapon_accuracy_nospread);

	if (!vars.aim.resolve_team.get<bool>() && !player->is_enemy())
		return;

	auto& log = player_log::get().get_log(player->EntIndex());

	log.m_bOverride = false;
	log.m_bShot = false;

	

	if (!vars.aim.resolver.get<bool>())
		return;

	if (player->IsDormant())
		return;


	int angle1[64];
	int angle2[64];
	int angle3[64];

	int using_fake_angles[64];
	static float old_simtime[65];
	if (*g_send_packet)
		angle1[player->EntIndex()] = player->get_eye_angles().y;
	else
		angle2[player->EntIndex()] = player->get_eye_angles().y;

	if (angle1[player->EntIndex()] != angle2[player->EntIndex()])
		using_fake_angles[player->EntIndex()] = true;
	else
		using_fake_angles[player->EntIndex()] = false;


	//baim::desync1[player->EntIndex()] = angle1[player->EntIndex()];
//	baim::desync2[player->EntIndex()] = angle2[player->EntIndex()];



	if (using_fake_angles[player->EntIndex()])
	{

		auto animState = player->get_anim_state(); // get animstate
		auto local_player = g_pLocalPlayer;

		bool is_local_player = player == local_player;//check
		bool is_teammate = local_player->get_team() == player->get_team() && !is_local_player; // teammate

		if (is_local_player) //return func on local
			return;

		if (is_teammate) //return on team
			return;

		auto idx = player->EntIndex(); //ent index
		auto lby = player->get_lby(); // lower body yaw

		auto& record = __player[idx];

		static float oldSimtimeShot[65];
		static float storedSimtimeShot[65];
		static float ShotTime[65];
		bool shot = false;
		//		baim::enemyshot[idx] = false;

			/*	const auto entity_weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(player->get_current_weapon_handle()));

				if (!entity_weapon)
					return;*/
		auto entity_weapon = get_weapon(g_pLocalPlayer->get_active_weapon());
		if (!entity_weapon)
			return;



		if (storedSimtimeShot[idx] != player->get_simtime())
		{
			if (player->get_active_weapon() && !entity_weapon->get_weapon_id() == WEAPON_KNIFE || !entity_weapon->get_weapon_id() == WEAPON_KNIFE_T || !entity_weapon->is_grenade())
			{
				if (ShotTime[idx] != entity_weapon->get_last_shot_time())
				{
					shot = true;
					//enemyshot[idx] = true;
					ShotTime[idx] = entity_weapon->get_last_shot_time();
				}
				else
				{
					shot = false;
					//baim::enemyshot[idx] = false;
				}
			}
			else
			{
				//	baim::enemyshot[idx] = false;
				shot = false;
				ShotTime[idx] = 0.f;
			}

			oldSimtimeShot[idx] = storedSimtimeShot[idx];

			storedSimtimeShot[idx] = player->get_simtime();
		}

		if (animState) // ?
		{
			//setUp velocity some changes by maatikk
			if (animState)
			{
				animState->m_flEyeYaw = player->get_eye_angles().y;
				animState->m_flEyePitch = player->get_eye_angles().x;
				player->setAnimationState(animState);
			}

			if (animState && animState->speed_2d > 0.1f || fabs(animState->flUpVelocity) > 100.0f)
			{
				animState->m_flGoalFeetYaw = approach_angle(animState->m_flEyeYaw, animState->m_flGoalFeetYaw,
					((animState->m_flUnknownFraction * 20.0f) + 30.0f) * animState->m_flLastClientSideAnimationUpdateTime);

				player->setAnimationState(animState);
			}
			else
			{
				if (animState)
				{
					animState->m_flGoalFeetYaw = approach_angle(lby, animState->m_flGoalFeetYaw,
						animState->m_flLastClientSideAnimationUpdateTime * 100.0f);

					player->setAnimationState(animState);
				}
			}
			// :/
			// its got pasted but some rework
			record.current_tick.eyes = player->get_eye_angles().y;

			if (record.current_tick.last_lby == FLT_MAX)
			{
				record.current_tick.last_lby = lby;
			}

			bool one = false;
			bool two = false;

			record.current_tick.lby_delta = record.current_tick.last_lby - lby;
			record.current_tick.eye_lby_delta = lby - record.current_tick.eyes;
			if (record.current_tick.last_lby != FLT_MAX)
			{
				if (player->get_velocity().Length2D() > 0.1f)
				{
					if (record.current_tick.last_lby != lby)
						record.current_tick.last_lby = lby;

					one = false, two = false;
				}

				else
				{
					record.current_tick.lby_delta = record.current_tick.last_lby - lby;

					if (record.current_tick.last_lby != lby)
					{
						if (solve_desync_simple(player)) //979 lby
						{
							if (delta_58(record.current_tick.last_lby, lby))
							{
								record.current_tick.low_delta = true;
								record.current_tick.high_delta = false;
							}
							else
							{
								record.current_tick.low_delta = false;
								record.current_tick.high_delta = true;
							}

							if (delta_58(lby, record.current_tick.eyes) && delta_58(record.current_tick.last_lby, record.current_tick.eyes))
							{
								if (!one || !two)
									record.current_tick.jitter_desync = false;

								if (one && two)
									record.current_tick.jitter_desync = true;
							}

							if (record.current_tick.low_delta && !delta_58(record.current_tick.last_lby, record.current_tick.eyes))
							{
								if (!one)
									one = true;

								if (!two && one)
									two = true;

								if (one && two)
									record.current_tick.jitter_desync = true;
							}

							//		else
							//			record.current_tick.low_delta = true;
						}
						else
							record.current_tick.static_desync = true; // if not 979 its static without lby
					}
					else
					{
						if (!solve_desync_simple(player)) //jitter record
						{
							one = false, two = false;

							record.current_tick.static_desync = true;
							record.current_tick.jitter_desync = false;

						}
						else
						{
							if (!delta_58(record.current_tick.last_lby, record.current_tick.eyes))
							{
								record.current_tick.low_delta = false;
								record.current_tick.high_delta = true;
								record.current_tick.jitter_desync = true;
							}
							else
							{
								record.current_tick.low_delta = true;
								record.current_tick.high_delta = false;
								record.current_tick.jitter_desync = true;
							}
						}
					}

				}
			}

			float m_flLastClientSideAnimationUpdateTimeDelta = fabs(animState->m_iLastClientSideAnimationUpdateFramecount - animState->m_flLastClientSideAnimationUpdateTime);
			// :)
			auto delta = angle_difference(player->get_eye_angles().y, player->get_anim_state()->m_flCurrentFeetYaw);


			if (record.current_tick.low_delta && !record.current_tick.jitter_desync && record.current_tick.lby_delta < 58.f)
			{
			//	player->GetEyeAnglesXY_xyo()->y = record.current_tick.last_lby;
			}

			else if (record.current_tick.jitter_desync && (!record.current_tick.high_delta || record.current_tick.low_delta))
			{
				// record.current_tick.eye_lby_delta

			//	player->GetEyeAnglesXY_xyo()->y = lby + (-record.current_tick.eye_lby_delta);
			}

			else if (record.current_tick.jitter_desync && record.current_tick.high_delta)
			{
			//	player->GetEyeAnglesXY_xyo()->y = record.current_tick.last_lby;
			}

			else if (record.current_tick.low_delta && record.current_tick.jitter_desync)
			{
			//	player->GetEyeAnglesXY_xyo()->y = lby - record.current_tick.lby_delta;
			}


			if (animState)
			{
				static auto GetSmoothedVelocity = [](float min_delta, Vector a, Vector b) {
					Vector delta = a - b;
					float delta_length = delta.Length();

					if (delta_length <= min_delta) {
						Vector result;
						if (-min_delta <= delta_length) {
							return a;
						}
						else {
							float iradius = 1.0f / (delta_length + FLT_EPSILON);
							return b - ((delta * iradius) * min_delta);
						}
					}
					else {
						float iradius = 1.0f / (delta_length + FLT_EPSILON);
						return b + ((delta * iradius) * min_delta);
					}
				};
				float v25;
				v25 = std::clamp(animState->m_fLandingDuckAdditiveSomething + player->get_duck_amt(), 1.0f, 0.0f);
				float v26 = animState->m_fDuckAmount;
				float v27 = g_pClientState->m_nChokedCommands * 6.0f;
				float v28;

				// clamp 
				if ((v25 - v26) <= v27) {
					if (-v27 <= (v25 - v26))
						v28 = v25;
					else
						v28 = v26 - v27;
				}
				else {
					v28 = v26 + v27;
				}

				Vector velocity = player->get_velocity();
				float flDuckAmount = std::clamp(v28, 1.0f, 0.0f);

				Vector animationVelocity = GetSmoothedVelocity(g_pClientState->m_nChokedCommands * 2000.0f, velocity, animState->m_flVelocity());
				float speed = std::fminf(animationVelocity.Length(), 260.0f);



				float flMaxMovementSpeed = 260.0f;
				if (entity_weapon) {
					flMaxMovementSpeed = std::fmaxf(entity_weapon->get_wpn_data()->flMaxSpeed, 0.001f);
				}

				float flRunningSpeed = speed / (flMaxMovementSpeed * 0.520f);
				float flDuckingSpeed = speed / (flMaxMovementSpeed * 0.340f);

				flRunningSpeed = std::clamp(flRunningSpeed, 0.0f, 1.0f);

				float flYawModifier = (((animState->m_bOnGround * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f;
				if (flDuckAmount > 0.0f) {
					float flDuckingSpeed = std::clamp(flDuckingSpeed, 0.0f, 1.0f);
					flYawModifier += (flDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier);
				}

				float m_flMaxBodyYaw = *(float*)(uintptr_t(animState) + 0x334) * flYawModifier;
				float m_flMinBodyYaw = *(float*)(uintptr_t(animState) + 0x330) * flYawModifier;

				float flEyeYaw = player->get_eye_angles().y;
				float flEyeDiff = std::remainderf(flEyeYaw - resolverinfoo.fakegoalfeetyaw, 360.f);

				if (flEyeDiff <= m_flMaxBodyYaw) {
					if (m_flMinBodyYaw > flEyeDiff)
						resolverinfoo.fakegoalfeetyaw = fabs(m_flMinBodyYaw) + flEyeYaw;
				}
				else {
					resolverinfoo.fakegoalfeetyaw = flEyeYaw - fabs(m_flMaxBodyYaw);
				}

				resolverinfoo.fakegoalfeetyaw = std::remainderf(resolverinfoo.fakegoalfeetyaw, 360.f);

				if (speed > 0.1f || fabs(velocity.z) > 100.0f) {
					resolverinfoo.fakegoalfeetyaw = approach_angle(
						flEyeYaw,
						resolverinfoo.fakegoalfeetyaw,
						((animState->m_bOnGround * 20.0f) + 30.0f)
						* g_pClientState->m_nChokedCommands);
				}
				else {
					resolverinfoo.fakegoalfeetyaw = approach_angle(
						player->get_lby(),
						resolverinfoo.fakegoalfeetyaw,
						g_pClientState->m_nChokedCommands * 100.0f);
				}

				float Left = flEyeYaw - m_flMinBodyYaw;
				float Right = flEyeYaw + m_flMaxBodyYaw;

				float resolveYaw;

				bool brutforce = false;

				
				float flMaxYawModifier = animState->pad10[516] * flYawModifier;
				float flMinYawModifier = animState->pad10[512] * flYawModifier;
				auto eyeYaw = animState->m_flEyeYaw;

				auto lbyYaw = animState->m_flGoalFeetYaw;

				float eye_feet_delta = fabs(eyeYaw - lbyYaw);

				if (eye_feet_delta <= flMaxYawModifier)
				{
					if (flMinYawModifier > eye_feet_delta)
					{
						resolveYaw = fabs(flMinYawModifier) + eyeYaw;
					}
				}
				else
				{
					resolveYaw = eyeYaw - fabs(flMaxYawModifier);
				}

				float desync_delta = flYawModifier;

				//an->m_flGoalFeetYaw = v136;


				if (player->get_velocity().Length2D() > 0.3f)
				{
					resolveYaw = ApproachAnglex(player->get_lby(), animState->m_flGoalFeetYaw, (animState->m_flStopToFullRunningFraction * 20.0f) + 30.0f * animState->m_flLastClientSideAnimationUpdateTime);
				}
				else
				{
					resolveYaw = ApproachAnglex(player->get_lby(), animState->m_flGoalFeetYaw, (m_flLastClientSideAnimationUpdateTimeDelta * 100.0f));
				}


			

				switch (missed_shots[player->EntIndex()] % 5)
				{
				case 0: {
					resolveYaw = desync_delta + desync_delta;

					_(MISS, "[ RESOLVER ] ");
					_(missed, "desync_delta + desync_delta \n");

					g_pCVar->ConsoleColorPrintf(Color(51, 171, 249, 255), MISS);
					util::print_dev_console(true, Color(255, 255, 255, 255), missed);
					//logging->debug("[DEBUG] <- left <-");
				}break;
				case 1: {


					resolveYaw = desync_delta * 0.5;
					_(MISS, "[ RESOLVER ] ");
					_(missed, "desync_delta \n");

					g_pCVar->ConsoleColorPrintf(Color(51, 171, 249, 255), MISS);
					util::print_dev_console(true, Color(255, 255, 255, 255), missed);
					//logging->debug("[DEBUG] <-> inverse <->");
				}break;


				case 2: {
					resolveYaw = desync_delta * -0.5;
					_(MISS, "[ RESOLVER ] ");
					_(missed, "desync_delta -\n");

					g_pCVar->ConsoleColorPrintf(Color(51, 171, 249, 255), MISS);
					util::print_dev_console(true, Color(255, 255, 255, 255), missed);
					//logging->debug("[DEBUG] -> right ->");
				}break;

				case 3: {
					resolveYaw = desync_delta + desync_delta;
					_(MISS, "[ RESOLVER ] ");
					_(missed, "desync_delta + desync_delta\n");

					g_pCVar->ConsoleColorPrintf(Color(51, 171, 249, 255), MISS);
					util::print_dev_console(true, Color(255, 255, 255, 255), missed);
					//logging->debug("[DEBUG] -> right ->");
				}break;

				case 4: {
					resolveYaw = +120.0;
					_(MISS, "[ RESOLVER ] ");
					_(missed, "+120\n");

					g_pCVar->ConsoleColorPrintf(Color(51, 171, 249, 255), MISS);
					util::print_dev_console(true, Color(255, 255, 255, 255), missed);
					//logging->debug("[DEBUG] -> right ->");
				}break;

				case 5: {
					resolveYaw = -120.;
					_(MISS, "[ RESOLVER ] ");
					_(missed, "-120\n");

					g_pCVar->ConsoleColorPrintf(Color(51, 171, 249, 255), MISS);
					util::print_dev_console(true, Color(255, 255, 255, 255), missed);
					//logging->debug("[DEBUG] -> right ->");
				}break;
				default:
					break;
				}
				float v136 = fmod(resolveYaw, 360.0);

				if (v136 > 180.0)
				{
					v136 = v136 - 360.0;
				}

				if (v136 < 180.0)
				{
					v136 = v136 + 360.0;
				}

				//	AnimationFix(player);

				animState->m_flGoalFeetYaw = v136;
			//	player->set_abs_angles(Vector(0, v136, 0));
				//	player->set_abs_angles = v136;





			}


		}
	}

}

bool resolver::is_spin( player_log_t * log )
{
	log->step = 0;

	if ( log->record.size() < 3 )
		return false;

	log->spindelta = ( log->record[ 0 ].m_body - log->record[ 1 ].m_body ) / log->record[ 1 ].m_lag;
	log->spinbody = log->record[ 0 ].m_body;
	const auto delta2 = ( log->record[ 1 ].m_body - log->record[ 2 ].m_body ) / log->record[ 2 ].m_lag;

	return false;

	return log->spindelta == delta2 && log->spindelta > 0.5f;
}
void resolver::anim(const ClientFrameStage_t stage)
{
	if (stage != FRAME_START)
		return;

	if (!g_pLocalPlayer)
		return;

	for (auto i = 1; i < g_pGlobals->maxClients; i++)
	{
		//auto& log = player_log::get().get_log(i);
		auto player = get_entity(i);

		if (!player || player == g_pLocalPlayer)
		{
			continue;
		}

		if (!player->is_enemy() && !vars.aim.resolve_team.get<bool>())
		{
			continue;
		}

		if (!player->get_alive())
		{
			continue;
		}

		if (player->IsDormant())
		{
			continue;
		}

		AnimationFix(player);
	}
}
void resolver::collect_wall_detect( const ClientFrameStage_t stage )
{
	if ( stage != FRAME_NET_UPDATE_POSTDATAUPDATE_START )
		return;

	if ( !g_pLocalPlayer )
		return;

	for ( auto i = 1; i < g_pGlobals->maxClients; i++ )
	{
		auto& log = player_log::get().get_log( i );
		auto player = get_entity( i );

		if ( !player || player == g_pLocalPlayer )
		{
			continue;
		}

		if ( !player->is_enemy() && !vars.aim.resolve_team.get<bool>() )
		{
			continue;
		}

		if ( !player->get_alive() )
		{
			continue;
		}

		if ( player->IsDormant() )
		{
			continue;
		}

		if ( player->get_velocity().Length2D() > 0.1f )
		{
			continue;
		}

		if ( !log.record.empty() && player->get_simtime() - log.record[ 0 ].m_sim_time == 0 )
			continue;

		if ( log.m_vecLastNonDormantOrig != player->get_origin() && g_pLocalPlayer->get_alive() )
		{
			log.m_iMode = RMODE_WALL;
		}

		if ( player->get_simtime() - log.m_flLastLowerBodyYawTargetUpdateTime > 1.35f && log.m_vecLastNonDormantOrig == player->get_origin() && log.m_iMode == RMODE_MOVING )
		{
			if ( player->get_simtime() - log.m_flLastLowerBodyYawTargetUpdateTime > 1.65f )
			{
				log.m_iMode = RMODE_WALL;
			}
		}

		if ( log.m_iMode == RMODE_WALL )
		{
			const auto at_target_angle = math::get().calc_angle( player->get_origin(), last_eye );

			Vector left_dir, right_dir, back_dir;
			math::get().angle_vectors( Vector( 0.f, at_target_angle.y - 90.f, 0.f ), &left_dir );
			math::get().angle_vectors( Vector( 0.f, at_target_angle.y + 90.f, 0.f ), &right_dir );
			math::get().angle_vectors( Vector( 0.f, at_target_angle.y + 180.f, 0.f ), &back_dir );

			const auto eye_pos = player->get_eye_pos();
			auto left_eye_pos = eye_pos + ( left_dir * 16.f );
			auto right_eye_pos = eye_pos + ( right_dir * 16.f );
			auto back_eye_pos = eye_pos + ( back_dir * 16.f );

			static CCSWeaponData big_fucking_gun{};
			auto get_big_fucking_gun = [ & ]() -> CCSWeaponData*
			{
				big_fucking_gun.iDamage = 200;
				big_fucking_gun.flRangeModifier = 1.0f;
				big_fucking_gun.flPenetration = 6.0f;
				big_fucking_gun.flArmorRatio = 2.0f;
				big_fucking_gun.flWeaponRange = 8192.f;
				return &big_fucking_gun;
			};

			penetration::get().get_damage( g_pLocalPlayer, player, left_eye_pos, &left_damage[ i ], get_big_fucking_gun(), &last_eye );
			penetration::get().get_damage( g_pLocalPlayer, player, right_eye_pos, &right_damage[ i ], get_big_fucking_gun(), &last_eye );
			penetration::get().get_damage( g_pLocalPlayer, player, back_eye_pos, &back_damage[ i ], get_big_fucking_gun(), &last_eye );
		}
	}
}

void resolver::extrpolate_players( const ClientFrameStage_t stage ) const
{
	if ( stage != FRAME_NET_UPDATE_POSTDATAUPDATE_START )
		return;

	if ( !g_pLocalPlayer )
		return;

	if ( !vars.aim.fakelag_compensation.get<bool>() )
		return;

	for ( auto i = 1; i < g_pGlobals->maxClients; i++ )
	{
		auto& log = player_log::get().get_log( i );
		auto player = get_entity( i );

		if ( !player || player == g_pLocalPlayer )
			continue;

		if ( player->get_simtime() <= player->get_oldsimtime() )
			continue;

		if ( log.record.empty() )
			continue;

		const auto previous = &log.record[ 0 ];

		if ( previous->m_dormant )
			continue;

		if ( !player->is_enemy() && !vars.aim.resolve_team.get<bool>() )
			continue;

		if ( !player->get_alive() )
			continue;

		if ( player->IsDormant() )
			continue;

		const auto simulation_tick_delta = time_to_ticks( player->get_simtime() - player->get_oldsimtime() );
		if ( simulation_tick_delta > 15 && simulation_tick_delta < 2 )
			return;

		const auto nci = g_pEngine->GetNetChannelInfo();
		const auto delta_ticks = ( std::clamp( time_to_ticks( nci->GetLatency( FLOW_INCOMING ) + nci->GetLatency( FLOW_INCOMING ) ) + g_pGlobals->tickcount - time_to_ticks( player->get_simtime() + lagcomp::get_lerp_time() ), 0, 100 ) ) - simulation_tick_delta;

		auto in_air = false;
		if ( !( player->get_flags() & FL_ONGROUND ) || !( previous->m_flags & FL_ONGROUND ) )
			in_air = true;

		auto ticks_left = static_cast< int >( delta_ticks );
		ticks_left = std::clamp( ticks_left, 0, 10 );
		while ( ticks_left > 0 )
		{
			auto data_origin = player->get_origin();
			auto data_velocity = player->get_velocity();
			auto data_flags = player->get_flags();

			lagcomp::extrapolate( player, data_origin, data_velocity, data_flags, in_air );

			player->get_simtime() += g_pGlobals->interval_per_tick;
			player->get_origin() = data_origin;
			player->get_velocity() = data_velocity;
			player->get_flags() = data_flags;
			--ticks_left;
		}
	}
}

bool resolver::wall_detect( player_log_t* log, lag_record_t* record, float& angle ) const
{
	if ( !g_pLocalPlayer->get_alive() )
		return false;

	const auto at_target_angle = math::get().calc_angle( record->m_origin, last_eye );

	auto set = false;

	const auto left = left_damage[ record->index ];
	const auto right = right_damage[ record->index ];
	const auto back = back_damage[ record->index ];

	auto max_dmg = std::max( left, std::max( right, back ) ) - 1.f;
	if ( left < max_dmg )
	{
		max_dmg = left;
		angle = math::normalize_float( at_target_angle.y + 90.f );
		set = true;
	}
	if ( right < max_dmg )
	{
		max_dmg = right;
		angle = math::normalize_float( at_target_angle.y - 90.f );
		set = true;
	}
	if ( back < max_dmg || !set )
	{
		max_dmg = back;
		angle = math::normalize_float( at_target_angle.y + 180.f );
	}

	return true;
}

void resolver::nospread_resolve( C_CSPlayer* player, lag_record_t* record )
{
	auto& log = player_log::get().get_log( player->EntIndex() );


	const auto simtime = record->m_sim_time;
	const auto choked_ticks = record->m_lag - 1;
	if ( choked_ticks || log.m_flProxyPitch == 180.f )
	{
		if ( log.m_nShots % 5 == 4 )
		{
			const float random_pitch = RandomFlt( -90.0f, 0.0f );
			record->m_eye_angles.x = random_pitch;
		}
		else
		{
			record->m_eye_angles.x = 90.f;
		}
	}

	const auto lby_update_delta = simtime - log.m_flLastLowerBodyYawTargetUpdateTime;
	const auto lby_moving_update_delta = simtime - log.m_flLastMovingLowerBodyYawTargetTime;

	if ( lby_update_delta > 0.1875f && choked_ticks )
	{
		const bool onground = record->m_flags & FL_ONGROUND;
		auto speed = record->m_velocity.Length();
		if ( log.fakewalking ) speed = 0.f;

		if ( onground && speed <= 0.1 && lby_moving_update_delta <= 1.33f )
		{
			if ( log.m_nShots > 0 )
			{
				auto index = log.m_nShots + log.last_hit_bute;
				while ( index > 6 )
					index -= 6;

				if ( log.m_nShots != log.oldshots )
				{
					log.last_hit_bute = index;
					log.oldshots = log.m_nShots;
				}

				log.m_flLastMovingLowerBodyYawTarget = math::normalize_float( log.m_flLowerBodyYawTarget + brutelist[ index ] );
			}

			record->m_eye_angles.y = log.m_flLastMovingLowerBodyYawTarget;
		}
		if ( onground && speed > 0.1f )
		{
			log.m_flLastMovingLowerBodyYawTargetTime = record->m_anim_time;
		}

		if ( speed <= 0.1f || onground )
		{
			if ( onground && speed <= 0.1 && lby_moving_update_delta > 1.11f )
			{
				lag_record_t* ongroundrecord = nullptr;
				for ( auto& current : log.record )
				{
					if ( current.m_flags & FL_ONGROUND )
					{
						ongroundrecord = &current;
						break;
					}
				}
				if ( ongroundrecord )
				{
					if ( speed > 260.f )
					{
						log.onground_nospread = true;
					}
					else
					{
						auto average_lby = log.m_flLowerBodyYawTarget + log.m_flLowerBodyYawTarget - log.m_flOldLowerBodyYawTarget;

						if ( log.m_nShots > 0 )
						{
							auto index = log.m_nShots + log.last_hit_bute;
							while ( index > 6 )
								index -= 6;

							if ( log.m_nShots != log.oldshots )
							{
								log.last_hit_bute = index;
								log.oldshots = log.m_nShots;
							}


							average_lby = log.m_flLowerBodyYawTarget + brutelist[ index ];
						}

						record->m_eye_angles.y = math::get().normalize_float( average_lby );
					}

				}
			}
		}
		else if ( !onground && log.m_nShots > 0 )
		{
			auto index = log.m_nShots;
			while ( index > 6 )
				index -= 6;
			const auto ang = log.m_flLowerBodyYawTarget + brutelist[ index ];

			record->m_eye_angles.y = math::get().normalize_float( ang );
		}

		const auto random_correction = RandomFlt( -35.f, 35.f );

		record->m_eye_angles.y += random_correction;
	}
}

void resolver::override( C_CSPlayer* player )
{


	static auto had_taget = false;
	if ( !g_pLocalPlayer || !vars.aim.override.get<bool>() || !GetAsyncKeyState( vars.key.override.get<int>() ) )
	{
		had_taget = false;
		return;
	}

	static std::vector<C_CSPlayer*> targets;

	_( weapon_recoil_scale_s, "weapon_recoil_scale" );
	static auto weapon_recoil_scale = g_pCVar->FindVar( weapon_recoil_scale_s );

	static auto last_checked = 0.f;
	if ( last_checked != g_pGlobals->curtime )
	{
		last_checked = g_pGlobals->curtime;
		targets.clear();

		QAngle viewangles;
		g_pEngine->GetViewAngles( viewangles );

		const auto needed_fov = 20.f;
		for ( auto i = 1; i <= g_pGlobals->maxClients; i++ )
		{
			auto ent = get_entity( i );
			if ( !ent || !ent->get_alive() || ent->IsDormant() || !ent->is_enemy() || ent == g_pLocalPlayer || ent->get_player_info().fakeplayer )
				continue;

			const auto fov = math::get().get_fov( viewangles + g_pLocalPlayer->get_aim_punch() * weapon_recoil_scale->get_float(), math::get().calc_angle( g_pLocalPlayer->get_eye_pos(), ent->get_eye_pos() ) );
			if ( fov < needed_fov )
			{
				targets.push_back( ent );
			}
		}
	}

	if ( targets.empty() )
	{
		had_taget = false;
		return;
	}

	auto found = false;
	for ( auto& target : targets )
	{
		if ( player == target )
		{
			found = true;
			break;
		}
	}

	if ( !found )
		return;

	const auto log = &player_log::get().get_log( player->EntIndex() );
	log->m_bOverride = true;

	QAngle viewangles;
	g_pEngine->GetViewAngles( viewangles );

	static auto last_delta = 0.f;
	static auto last_angle = 0.f;

	const auto at_target_yaw = math::get().calc_angle( last_eye, player->get_eye_pos() ).y;
	auto delta = math::get().normalize_float( viewangles.y - at_target_yaw );

	if ( had_taget && fabsf( viewangles.y - last_angle ) < 0.1f )
	{
		viewangles.y = last_angle;
		delta = last_delta;
	}

	had_taget = true;

	if ( player->get_velocity().Length2D() < 75.f )
	{
		if ( delta > 1.2f )
			player->get_eye_angles().y= math::get().normalize_float( at_target_yaw + 90.f );
		else if ( delta < -1.2f )
			player->get_eye_angles().y = math::get().normalize_float( at_target_yaw - 90.f );
		else
			player->get_eye_angles().y = math::get().normalize_float( at_target_yaw );

	//	record->m_override = true;
	}

	last_angle = viewangles.y;
	last_delta = delta;
}

void resolver::resolve_proxy( const C_CSPlayer * player, float * m_float )
{
	auto PlayerRecord = &player_log::get().get_log( player->EntIndex() );

	PlayerRecord->m_flProxyYaw = math::get().normalize_float( *m_float );
}

void resolver::on_lby_proxy( C_CSPlayer * entity, float* LowerBodyYaw )
{
	float oldBodyYaw; // xmm4_4
	float nextPredictedSimtime; // xmm3_4
	float nextBodyUpdate = 0.f; // xmm3_4

	auto PlayerRecord = &player_log::get().get_log( entity->EntIndex() );

	oldBodyYaw = PlayerRecord->m_flLowerBodyYawTarget;
	if ( oldBodyYaw != *LowerBodyYaw )
	{
		if ( entity != g_pLocalPlayer && entity->get_flags() & FL_ONGROUND )
		{
			nextPredictedSimtime = entity->get_oldsimtime() + g_pGlobals->interval_per_tick;
			float vel = entity->get_velocity().Length2D();
			if ( vel > 0.1f )
				nextBodyUpdate = nextPredictedSimtime + 0.22f;
			else /*if ( PlayerRecord->nextBodyUpdate <= nextPredictedSimtime )*/
				nextBodyUpdate = nextPredictedSimtime + 1.1f;

			if ( nextBodyUpdate != 0.f )
				PlayerRecord->nextBodyUpdate = nextBodyUpdate;
		}

		PlayerRecord->m_flLastLowerBodyYawTargetUpdateTime = entity->get_oldsimtime() + g_pGlobals->interval_per_tick;
		PlayerRecord->m_flOldLowerBodyYawTarget = oldBodyYaw;
		PlayerRecord->m_flLowerBodyYawTarget = math::normalize_float( *LowerBodyYaw );

		//util::print_dev_console( true, Color::Green(), "update time = %f\n", PlayerRecord->m_flLastLowerBodyYawTargetUpdateTime );
	}

	if ( entity->get_velocity().Length2D() > 75.f || entity->get_flags() & FL_DUCKING && entity->get_velocity().Length2D() > 20.f )
	{
		PlayerRecord->m_flLastMovingLowerBodyYawTarget = math::normalize_float( *LowerBodyYaw );
		PlayerRecord->m_flLastMovingLowerBodyYawTargetTime = entity->get_oldsimtime() + g_pGlobals->interval_per_tick;
	}
}

void resolver::resolve_poses( C_CSPlayer * player, player_log_t* log )
{
	auto flPitch = log->record[ 0 ].m_eye_angles.x;
	if ( log->record.size() > 2 && vars.aim.resolver.get<bool>() )
	{
		if ( std::fabsf( flPitch - log->record[ 1 ].m_eye_angles.x ) > 10.f && std::fabsf( flPitch - log->record[ 2 ].m_eye_angles.x ) > 10.f )
		{
			flPitch = log->record[ 1 ].m_eye_angles.x;
			if ( player->get_velocity().Length2D() > 0.1 && player->get_velocity().Length2D() < 75.f || log->record[ 0 ].m_fake_walk || player->get_flags() & FL_DUCKING && player->get_velocity().Length2D() > 20.f || log->record[ 0 ].m_lag > 12 )
				log->record[ 0 ].m_eye_angles.x = flPitch;
		}
	}

	if ( flPitch > 180.0f )
	{
		flPitch -= 360.0f;
	}
	flPitch = std::clamp( flPitch, -90.f, 90.f );
	player->set_pose_param( BODY_PITCH, flPitch );
}

bool resolver::update_lby_timer( C_CSPlayer* player )
{
	const auto playerRecord = &player_log::get().get_log( player->EntIndex() );

	const auto simtime_delta = player->get_simtime() - player->get_oldsimtime();
	if ( !simtime_delta )
		return false;

	const auto animtime = player->get_oldsimtime() + g_pGlobals->interval_per_tick;

	playerRecord->m_bRunningTimer = true;

	if ( player->get_velocity().Length2D() > 0.1f )
	{
		playerRecord->m_bRunningTimer = false;
		return false;
	}

	const auto update_delta = animtime - playerRecord->nextBodyUpdate;
	const auto recorded_update_delta = animtime - playerRecord->m_flLastLowerBodyYawTargetUpdateTime;
	if ( update_delta <= 0 && update_delta > -1.35f && recorded_update_delta < 0.245f )
	{
		playerRecord->nextBodyUpdate += 1.1f;
		return true;
	}

	return false;
}

void resolver::add_shot( const Vector & shotpos, lag_record_t * record, const int& enemy_index )
{
	static auto last_tick = 0;
	if ( g_pGlobals->tickcount != last_tick )
	{
		shots.emplace_back( shotpos, g_pGlobals->tickcount, enemy_index, record );
		last_tick = g_pGlobals->tickcount;
	}
}

void resolver::update_missed_shots( const ClientFrameStage_t& stage )
{
	if ( stage != FRAME_NET_UPDATE_START )
		return;

	auto it = shots.begin();
	while ( it != shots.end() )
	{
		const auto shot = *it;
		if ( shot.tick + 64 < g_pGlobals->tickcount || shot.tick - 2 > g_pGlobals->tickcount )
		{
			it = shots.erase( it );
		}
		else
		{
			++it;
		}
	}

	auto it2 = current_shots.begin();
	while ( it2 != current_shots.end() )
	{
		const auto shot = *it2;
		if ( shot.tick + 64 < g_pGlobals->tickcount || shot.tick - 2 > g_pGlobals->tickcount )
		{
			it2 = current_shots.erase( it2 );
		}
		else
		{
			++it2;
		}
	}
}

std::deque<shot_t>& resolver::get_shots()
{
	return shots;
}

void resolver::hurt_listener( IGameEvent * game_event )
{
	if ( shots.empty() )
		return;

	_( attacker_s, "attacker" );
	_( userid_s, "userid" );
	_( hitgroup_s, "hitgroup" );
	_( dmg_health_s, "dmg_health" );

	const auto attacker = g_pEngine->GetPlayerForUserID( game_event->GetInt( attacker_s ) );
	const auto victim = g_pEngine->GetPlayerForUserID( game_event->GetInt( userid_s ) );
	const auto hitgroup = game_event->GetInt( hitgroup_s );
	const auto damage = game_event->GetInt( dmg_health_s );

	if ( attacker != g_pEngine->GetLocalPlayer() )
		return;

	if ( victim == g_pEngine->GetLocalPlayer() )
		return;

	auto player = get_entity( victim );
	if ( !player || !player->is_enemy() )
		return;

	if ( unapproved_shots.empty() )
		return;

	for ( auto& shot : unapproved_shots )
	{
		if ( !shot.hurt )
		{
			shot.hurt = true;
			shot.hitinfo.victim = victim;
			shot.hitinfo.hitgroup = hitgroup;
			shot.hitinfo.damage = damage;
			return;
		}
	}
}

shot_t* resolver::closest_shot( int tickcount )
{
	shot_t* closest_shot = nullptr;
	auto closest_diff = 64;
	for ( auto& shot : shots )
	{
		const auto diff = abs( tickcount - shot.tick );
		if ( diff <= closest_diff )
		{
			closest_shot = &shot;
			closest_diff = diff;
			continue;
		}


		break;
	}

	return closest_shot;
}

void resolver::record_shot( IGameEvent * game_event )
{
	_( userid_s, "userid" );

	const auto userid = g_pEngine->GetPlayerForUserID( game_event->GetInt( userid_s ) );
	const auto player = get_entity( userid );

	if ( player != g_pLocalPlayer )
		return;

	const auto shot = closest_shot( g_pGlobals->tickcount - time_to_ticks( g_pEngine->GetNetChannelInfo()->GetLatency( FLOW_OUTGOING ) ) );
	if ( !shot )
		return;

	current_shots.push_back( *shot );
}

void resolver::listener( IGameEvent * game_event )
{
	_( weapon_fire, "weapon_fire" );

	if ( !strcmp( game_event->GetName(), weapon_fire ) )
	{
		record_shot( game_event );
		return;
	}

	if ( current_shots.empty() )
		return;

	_( userid_s, "userid" );

	const auto userid = g_pEngine->GetPlayerForUserID( game_event->GetInt( userid_s ) );
	const auto player = get_entity( userid );
	if ( !player || player != g_pLocalPlayer )
		return;

	_( x, "x" );
	_( y, "y" );
	_( z, "z" );

	const Vector pos( game_event->GetFloat( x ), game_event->GetFloat( y ), game_event->GetFloat( z ) );

	if ( vars.misc.impacts.get<bool>() )
		g_pDebugOverlay->AddBoxOverlay( pos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 0, 0, 155, 127, 4 );

	auto shot = &current_shots[ 0 ];

	static auto last_curtime = 0.f;
	static auto last_length = 0.f;
	static auto counter = 0;

	if ( last_curtime == g_pGlobals->curtime )
		counter++;
	else
	{
		counter = 0;
		last_length = 0.f;
	}

	if ( last_curtime == g_pGlobals->curtime && shot->shotpos.Dist( pos ) <= last_length )
		return;

	last_length = shot->shotpos.Dist( pos );

	if ( counter )
		unapproved_shots.pop_front();

	shot->hitpos = pos;
	unapproved_shots.push_back( *shot );

	last_curtime = g_pGlobals->curtime;
}

void resolver::approve_shots( const ClientFrameStage_t& stage )
{

	if ( stage != FRAME_NET_UPDATE_START )
		return;

	for ( auto& shot : unapproved_shots )
	{
		const auto angles = math::get().calc_angle( shot.shotpos, shot.hitpos );
		Vector direction;
		math::get().angle_vectors( angles, &direction );
		direction.Normalize();

		if ( shot.record.index == -1 )
		{
			if ( shot.hurt )
			{
				const auto player = get_entity( shot.hitinfo.victim );
				if ( player )
				{
					auto origin = player->get_origin();
					origin.z = Vector( shot.shotpos + direction * shot.shotpos.Dist( origin ) ).z;
					auto hitpos = shot.shotpos + direction * shot.shotpos.Dist( origin );

					trace_t tr;
					Ray_t ray;
					ray.Init( shot.shotpos, shot.hitpos );
					g_pTrace->ClipRayToEntity( ray, MASK_SHOT_HULL | CONTENTS_HITBOX, player, &tr );

					if ( tr.m_pEnt == player )
						hitpos = tr.endpos;

					hitmarker::get().add_hit( hitmarker_t( g_pGlobals->curtime, shot.hitinfo.victim, shot.hitinfo.damage, shot.hitinfo.hitgroup, hitpos ) );

					if ( vars.visuals.beams.local.enabled.get<bool>() && vars.visuals.beams.enabled.get<bool>() )
						beams::get().add_local_beam( impact_info_t( g_pGlobals->curtime, shot.shotpos, hitpos, g_pLocalPlayer, get_col( vars.visuals.beams.local.color.get<uintptr_t>() ) ) );
					continue;
				}

			}

			if ( vars.visuals.beams.local.enabled.get<bool>() && vars.visuals.beams.enabled.get<bool>() )
				beams::get().add_local_beam( impact_info_t( g_pGlobals->curtime, shot.shotpos, shot.hitpos, g_pLocalPlayer, get_col( vars.visuals.beams.local.color.get<uintptr_t>() ) ) );
			continue;
		}

		const auto hitpos = shot.hitpos;

		shot.hitpos = shot.shotpos + direction * shot.shotpos.Dist( shot.record.m_origin ) * 1.1f;

		auto player = get_entity( shot.enemy_index );
		if ( !player || player->IsDormant() || !player->get_alive() )
		{
			if ( shot.hurt )
			{
				hitmarker::get().add_hit( hitmarker_t( g_pGlobals->curtime, shot.hitinfo.victim, shot.hitinfo.damage, shot.hitinfo.hitgroup, shot.shotpos + direction * shot.shotpos.Dist( shot.record.m_origin ) ) );

				if ( vars.visuals.beams.local.enabled.get<bool>() && vars.visuals.beams.enabled.get<bool>() && !beams::get().beam_exists( g_pLocalPlayer, g_pGlobals->curtime ) )
					beams::get().add_local_beam( impact_info_t( g_pGlobals->curtime, shot.shotpos, shot.shotpos + direction * shot.shotpos.Dist( shot.record.m_origin ), g_pLocalPlayer, get_col( vars.visuals.beams.local.color.get<uintptr_t>() ) ) );
			}
			else if ( vars.visuals.beams.local.enabled.get<bool>() && vars.visuals.beams.enabled.get<bool>() )
				beams::get().add_local_beam( impact_info_t( g_pGlobals->curtime, shot.shotpos, hitpos, g_pLocalPlayer, get_col( vars.visuals.beams.local.color.get<uintptr_t>() ) ) );

			continue;
		}

		lag_record_t backup( player );
		shot.record.apply( player );

		trace_t tr;
		Ray_t ray;
		ray.Init( shot.shotpos, shot.hitpos );
		g_pTrace->ClipRayToEntity( ray, MASK_SHOT_HULL | CONTENTS_HITBOX, player, &tr );

		backup.apply( player, true );

		if ( tr.m_pEnt == player )
		{
			shot.hitpos = tr.endpos;
			shot.hit = true;
		}

		if ( vars.visuals.beams.local.enabled.get<bool>() && vars.visuals.beams.enabled.get<bool>() )
			beams::get().add_local_beam( impact_info_t( g_pGlobals->curtime, shot.shotpos, shot.hit ? shot.hitpos : hitpos, g_pLocalPlayer, get_col( vars.visuals.beams.local.color.get<uintptr_t>() ) ) );

		if ( shot.hurt )
			hitmarker::get().add_hit( hitmarker_t( g_pGlobals->curtime, shot.hitinfo.victim, shot.hitinfo.damage, shot.hitinfo.hitgroup, shot.hit ? shot.hitpos : shot.shotpos + direction * shot.shotpos.Dist( shot.record.m_origin ) ) );


		_( MISS, "[ MISS ] " );
		_( missed, "missed due to spread\n" );

		if ( player->get_player_info().fakeplayer )
		{
			if ( !shot.hurt && !shot.hit )
			{
				g_pCVar->ConsoleColorPrintf( Color( 51, 171, 249, 255 ), MISS );
				util::print_dev_console( true, Color( 255, 255, 255, 255 ), missed );
			}

			continue;
		}

		calc_missed_shots( &shot );
	}

	current_shots.clear();
	unapproved_shots.clear();
}


void resolver::calc_missed_shots( shot_t* shot )
{
	if ( !vars.aim.resolver.get<bool>() )
		return;

	const auto log = &player_log::get().get_log( shot->enemy_index );
	if ( shot->record.m_override || shot->record.m_lby_flick || shot->hurt )
		return;

	if ( shot->hit )
	{
		log->m_nShots++;
		return;
	}

	__( weapon_accuracy_nospread, "weapon_accuracy_nospread" );
	static auto nospread = g_pCVar->FindVar( weapon_accuracy_nospread );

	if ( nospread->get_bool() )
		return;

	_( MISS, "[ MISS ] " );
	_( missed, "missed due to spread\n" );
	
	g_pCVar->ConsoleColorPrintf( Color( 51, 171, 249, 255 ), MISS );
	util::print_dev_console( true, Color( 255, 255, 255, 255 ), missed );
	for (auto i = 1; i < g_pGlobals->maxClients; i++)
	{
	//	auto& log = player_log::get().get_log(i);
		auto player = get_entity(i);
		if (!player || !player->get_alive() || !player->is_enemy() || player == g_pLocalPlayer)
			continue;

		missed_shots[player->EntIndex()]++;
	}
	
}

float resolver::get_next_update( int index )
{
	return player_log::get().get_log( index ).nextBodyUpdate;
}
