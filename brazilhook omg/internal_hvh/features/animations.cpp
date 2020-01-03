#include "../include_cheat.h"
#include <functional>
struct ClientAnimating
{
	C_BaseAnimating* entity;
	uint32_t flags;
};


void for_each_player(const std::function<void(C_CSPlayer*)> fn)
{
	for (auto i = 0; i <= static_cast<int32_t>(g_pEngine->GetMaxClients()); ++i) {
		const auto entity = g_pEntitiyList->GetClientEntity(i);
		if (entity && entity->is_player())
			fn(reinterpret_cast<C_CSPlayer*>(entity));
	}
}

constexpr auto rad_pi = 57.295779513082f;
typedef CUtlVector< ClientAnimating >ClientSideAnimationList;


static constexpr auto pitch_bounds = 178.f;
static constexpr auto yaw_bounds = 360.f;
static constexpr auto roll_bounds = 100.f;

static void normalize(QAngle& angle)
{
	if (!angle.IsValid())
	{
		angle = { 0.f, 0.f, 0.f };
		return;
	}

	angle.x = std::remainderf(angle.x, pitch_bounds);
	angle.y = std::remainderf(angle.y, yaw_bounds);
	angle.z = std::remainderf(angle.z, roll_bounds);
}

static float normalize_yaw(float angle)
{
	if (!std::isfinite(angle))
		angle = 0.f;

	return std::remainderf(angle, yaw_bounds);
}

static QAngle calc_angle2(const Vector from, const Vector to) {
	static const auto ang_zero = QAngle(0.0f, 0.0f, 0.0f);

	const auto delta = from - to;
	if (delta.Length() <= 0.0f)
		return ang_zero;

	if (delta.z == 0.0f && delta.Length() == 0.0f)
		return ang_zero;

	if (delta.y == 0.0f && delta.x == 0.0f)
		return ang_zero;

	QAngle angles;
	angles.x = asinf(delta.z / delta.Length()) * rad_pi;
	angles.y = atanf(delta.y / delta.x) * rad_pi;
	angles.z = 0.0f;

	if (delta.x >= 0.0f)
		angles.y += 180.0f;

	normalize(angles);
	return angles;
}




static float interpolate(const float from, const float to, const float percent)
{
	return to * percent + from * (1.f - percent);
}

static Vector interpolate(const Vector from, const Vector to, const float percent)
{
	return to * percent + from * (1.f - percent);
}

static QAngle interpolate(const QAngle from, const QAngle to, const float percent)
{
	return to * percent + from * (1.f - percent);
}
struct AnimationBackup_t
{
	Vector m_origin;
	Vector m_abs_origin;
	Vector m_velocity;
	Vector m_abs_velocity;
	int m_flags;
	int m_eflags;
	float m_duck;
	float m_body;
	std::array<C_AnimationLayer, 15> m_layers;
};

/*void animations::stop( const ClientFrameStage_t stage )
{
	if ( stage != FRAME_RENDER_START )
		return;

	static auto clientSideAnimationList = *reinterpret_cast< ClientSideAnimationList** >( sig( "client_panorama.dll", "A1 ? ? ? ? F6 44" ) + 1 );

	const auto list = clientSideAnimationList;

	for ( auto i = 0; i < list->Count(); i++ )
	{
		auto& animating = list->Element( i );

		if ( animating.entity && animating.entity->GetClientClass()->m_ClassID == ClassId_CCSPlayer)
		{
			
			if ( animating.entity->is_enemy() && animating.flags & 1 )
			{
				animating.flags &= ~1;
			}
		}
	}
}

void animations::update_local_animations()
{
	static auto wasonground = true;

	const auto player_flags = g_pLocalPlayer->get_flags();

	if ( !wasonground || !( g_pLocalPlayer->get_flags() & FL_ONGROUND ) )
	{
		g_pLocalPlayer->get_flags() &= ~FL_ONGROUND;
	}

	if ( !g_pClientState->m_nChokedCommands )
	{
		wasonground = player_flags & FL_ONGROUND;
	}

	const auto inlandanim = static_cast< bool >( g_pLocalPlayer->get_anim_state()->m_bInHitGroundAnimation );
	const bool onground = g_pLocalPlayer->get_flags() & FL_ONGROUND;

	if ( inlandanim && onground && wasonground )
	{
		thirdperson::get().get_tp_angle().x = -10.f;
		thirdperson::get().get_tp_angle().y = g_pLocalPlayer->get_anim_state()->m_flGoalFeetYaw;
	}

	g_pLocalPlayer->get_flags() = player_flags;
	g_pLocalPlayer->get_visual_angles() = thirdperson::get().get_tp_angle();
}

void animations::fix_local_anims( const ClientFrameStage_t stage )
{
	if ( stage != FRAME_RENDER_START )
		return;

	const auto reset = ( !g_pLocalPlayer || !g_pLocalPlayer->get_alive() );

	if ( !g_pClientState->m_nChokedCommands || reset )
		update_custom_anims( g_pLocalPlayer );

	if ( reset )
		return;

	const auto animstate = g_pLocalPlayer->get_anim_state();

	if ( !g_pInput->m_fCameraInThirdPerson )
	{
		animstate->m_bInHitGroundAnimation = false;
		return;
	}

	update_local_animations();
}

struct AnimationBackup_t
{
	Vector m_origin;
	Vector m_abs_origin;
	Vector m_velocity;
	Vector m_abs_velocity;
	int m_flags;
	int m_eflags;
	float m_duck;
	float m_body;
	std::array<C_AnimationLayer, 15> m_layers;
};

void ResetAnimationState( CCSGOPlayerAnimState* state )
{
	using ResetAnimState_t = void( __thiscall* )( CCSGOPlayerAnimState* );

	static auto ResetAnimState = reinterpret_cast< ResetAnimState_t >( sig( "client_panorama.dll", "56 6A 01 68 ? ? ? ? 8B F1" ) );
	if ( !ResetAnimState )
		return;

	ResetAnimState( state );
}

void animations::update_player_animations( lag_record_t* record, C_CSPlayer* m_player )
{
	auto log = &player_log::get().get_log( m_player->EntIndex() );

	auto state = m_player->get_anim_state();
	if ( !state )
		return;

	// player respawned.
	if ( m_player->get_spawn_time() != log->m_flSpawntime )
	{
		// reset animation state.
		ResetAnimationState( state );

		// note new spawn time.
		log->m_flSpawntime = m_player->get_spawn_time();
	}

	// backup curtime.
	float curtime = g_pGlobals->curtime;
	float frametime = g_pGlobals->frametime;

	// set curtime to animtime.
	// set frametime to ipt just like on the server during simulation.
	g_pGlobals->curtime = record->m_anim_time;
	g_pGlobals->frametime = g_pGlobals->interval_per_tick;

	// backup stuff that we do not want to fuck with.
	AnimationBackup_t backup;

	backup.m_origin = m_player->get_origin();
	backup.m_abs_origin = m_player->get_abs_origin();
	backup.m_velocity = m_player->get_velocity();
	backup.m_abs_velocity = m_player->get_abs_velocity();
	backup.m_flags = m_player->get_flags();
	backup.m_eflags = m_player->get_eflags();
	backup.m_duck = m_player->get_duck_amt();
	backup.m_body = m_player->get_lby();
	backup.m_layers = m_player->get_anim_layers();

	// is player a bot?
	bool bot = m_player->get_player_info().fakeplayer;

	// reset fakewalk state.
	record->m_fake_walk = false;
	//record->m_mode = Resolver::Modes::RESOLVE_NONE;

	// fix velocity.
	// https://github.com/VSES/SourceEngine2007/blob/master/se2007/game/client/c_baseplayer.cpp#L659
	if ( record->m_lag > 0 && record->m_lag < 16 && log->record.size() >= 2 )
	{
		// get pointer to previous record.
		auto previous = &log->record[ 1 ];

		if ( previous && !previous->m_dormant )
			record->m_velocity = ( record->m_origin - previous->m_origin ) * ( 1.f / ticks_to_time( record->m_lag ) );
	}

	// set this fucker, it will get overriden.
	record->m_anim_velocity = record->m_velocity;

	// fix various issues with the game
	// these issues can only occur when a player is choking data.
	if ( record->m_lag > 1 && !bot )
	{
		// detect fakewalk.
		float speed = record->m_velocity.Length();

		if ( record->m_flags & FL_ONGROUND && record->m_layers[ 6 ].m_flWeight == 0.f && speed > 0.1f && speed < 100.f )
			record->m_fake_walk = true;

		if ( record->m_fake_walk )
			record->m_anim_velocity = record->m_velocity = { 0.f, 0.f, 0.f };

		// we need atleast 2 updates/records
		// to fix these issues.
		if ( log->record.size() >= 2 )
		{
			// get pointer to previous record.
			auto previous = &log->record[ 1 ];

			if ( previous && !previous->m_dormant )
			{
				// set previous flags.
				m_player->get_flags() = previous->m_flags;

				// strip the on ground flag.
				m_player->get_flags() &= ~FL_ONGROUND;

				// been onground for 2 consecutive ticks? fuck yeah.
				if ( record->m_flags & FL_ONGROUND && previous->m_flags & FL_ONGROUND )
					m_player->get_flags() |= FL_ONGROUND;

				// fix jump_fall.
				else
				{
					if ( record->m_layers[ 4 ].m_flWeight != 1.f && previous->m_layers[ 4 ].m_flWeight == 1.f && record->m_layers[ 5 ].m_flWeight != 0.f )
						m_player->get_flags() |= FL_ONGROUND;

					if ( record->m_flags & FL_ONGROUND && !( previous->m_flags & FL_ONGROUND ) )
						m_player->get_flags() &= ~FL_ONGROUND;
				}

				// calc fraction between both records.
				float frac = ( record->m_anim_time - record->m_sim_time ) / ( previous->m_sim_time - record->m_sim_time );

				m_player->get_duck_amt() = math::lerp( record->m_duck, previous->m_duck, frac );

				if ( !record->m_fake_walk )
					record->m_anim_velocity = math::lerp( record->m_velocity, previous->m_velocity, frac );
			}
		}
	}

	// and not bot, player uses fake angles.
	bool fake = vars.aim.resolver.get<bool>() && !bot && ( m_player->is_enemy() || vars.aim.resolve_team.get<bool>() );

	// if using fake angles, correct angles.
	if ( fake )
	{
		resolver::get().resolve( m_player, record );
		resolver::get().override( m_player, record );
	}
	else
		log->m_iMode = RMODE_MOVING;

	log->m_vecLastNonDormantOrig = record->m_origin;

	// set stuff before animating.
	m_player->get_origin() = record->m_origin;
	m_player->set_abs_origin( record->m_origin );

	m_player->get_velocity() = m_player->get_abs_velocity() = record->m_anim_velocity;
	m_player->get_lby() = record->m_body;

	// EFL_DIRTY_ABSVELOCITY
	// skip call to C_BaseEntity::CalcAbsoluteVelocity
	m_player->get_eflags() &= ~0x1000;

	// run body prediction.
	if ( resolver::get().update_lby_timer( m_player ) )
		record->m_lby_flick = true;

	const auto backup_eye = m_player->get_eye_angles();

	if ( record->m_lby_flick )
	{
		const auto lby = math::normalize_float( m_player->get_lby() );
		record->m_eye_angles.y = lby;
		m_player->set_abs_angles( QAngle( 0.f, lby, 0.f ) );
	}

	// write potentially resolved angles.
	m_player->get_eye_angles() = record->m_eye_angles;

	// fix animating in same frame.
	if ( state->m_iLastClientSideAnimationUpdateFramecount == g_pGlobals->framecount )
		state->m_iLastClientSideAnimationUpdateFramecount -= 1;

	// 'm_animating' returns true if being called from SetupVelocity, passes raw velocity to animstate.
	g_setup_vel_fix = true;
	m_player->get_clientside_animation() = true;
	m_player->update_clientside_anim();
	m_player->get_clientside_animation() = false;
	g_setup_vel_fix = false;

	m_player->get_eye_angles() = backup_eye;

	// store updated/animated poses and rotation in lagrecord.
	record->m_poses = m_player->get_pose_params();
	record->m_abs_ang = m_player->get_abs_angles();

	// correct poses
	//resolver::resolve_poses( m_player, log );

	// restore backup data.
	m_player->get_origin() = backup.m_origin;
	m_player->get_velocity() = backup.m_velocity;
	m_player->get_abs_velocity() = backup.m_abs_velocity;
	m_player->get_flags() = backup.m_flags;
	m_player->get_eflags() = backup.m_eflags;
	m_player->get_duck_amt() = backup.m_duck;
	m_player->get_lby() = backup.m_body;
	m_player->set_abs_origin( backup.m_abs_origin );
	m_player->set_anim_layers( backup.m_layers );

	// IMPORTANT: do not restore poses here, since we want to preserve them for rendering.
	// also dont restore the render angles which indicate the model rotation.

	// restore globals.
	g_pGlobals->curtime = curtime;
	g_pGlobals->frametime = frametime;
}

void animations::update_custom_anims( C_CSPlayer * player )
{
	for ( auto i = 1; i <= g_pGlobals->maxClients; i++ )
	{
		const auto ent = get_entity( i );
		auto& info = anim_info[ i ];
		if ( ( !ent || ent != g_pLocalPlayer ) && info.animstate )
		{
			g_pMemAlloc->Free( info.animstate );
			info.animstate = nullptr;
		}
	}

	if ( !player )
		return;

	auto& info = anim_info[ player->EntIndex() ];

	if ( player->get_spawn_time() != info.spawn_time || player->GetRefEHandle() != info.handle || !player->get_alive() )
	{
		info.spawn_time = player->get_spawn_time();
		info.handle = player->GetRefEHandle();

		if ( info.animstate )
		{
			g_pMemAlloc->Free( info.animstate );
			info.animstate = nullptr;
		}

		if ( !player->get_alive() )
			return;
	}
	if ( !info.animstate )
	{
		info.animstate = reinterpret_cast< CCSGOPlayerAnimState* > ( g_pMemAlloc->Alloc( 860u ) );
		player->create_anim_state( info.animstate );
	}

	// backup curtime.
	const auto curtime = g_pGlobals->curtime;
	const auto frametime = g_pGlobals->frametime;

	// set curtime to animtime.
	// set frametime to ipt just like on the server during simulation.
	g_pGlobals->curtime = player->get_oldsimtime() + g_pGlobals->interval_per_tick;
	g_pGlobals->frametime = g_pGlobals->interval_per_tick;

	// backup stuff that we do not want to fuck with.
	const auto backup_origin = player->get_origin();
	const auto backup_velocity = player->get_velocity();
	const auto backup_abs_velocity = player->get_abs_velocity();
	const auto backup_flags = player->get_flags();
	const auto backup_eflags = player->get_eflags();
	const auto backup_duck_amt = player->get_duck_amt();
	const auto backup_lby = player->get_lby();
	auto backup_layers = player->get_anim_layers();
	auto backup_poses = player->get_pose_params();

	if ( info.animstate->m_iLastClientSideAnimationUpdateFramecount == g_pGlobals->framecount )
		info.animstate->m_iLastClientSideAnimationUpdateFramecount -= 1;

	info.animstate->update( player->get_eye_angles() );
	info.animvel = info.animstate->m_velocity;

	player->get_origin() = backup_origin;
	player->get_velocity() = backup_velocity;
	player->get_abs_velocity() = backup_abs_velocity;
	player->get_flags() = backup_flags;
	player->get_eflags() = backup_eflags;
	player->get_duck_amt() = backup_duck_amt;
	player->get_lby() = backup_lby;
	player->set_anim_layers( backup_layers );
	player->set_pose_params( backup_poses );

	g_pGlobals->curtime = curtime;
	g_pGlobals->frametime = frametime;
}
*/
void ResetAnimationState(CCSGOPlayerAnimState* state)
{
	using ResetAnimState_t = void(__thiscall*)(CCSGOPlayerAnimState*);

	static auto ResetAnimState = reinterpret_cast<ResetAnimState_t>(sig("client_panorama.dll", "56 6A 01 68 ? ? ? ? 8B F1"));
	if (!ResetAnimState)
		return;

	ResetAnimState(state);
}

void c_animation_system::update_player_animations(lag_record_t* record, C_CSPlayer* m_player)
{
	auto log = &player_log::get().get_log(m_player->EntIndex());

	auto state = m_player->get_anim_state();
	if (!state)
		return;

	// player respawned.
	if (m_player->get_spawn_time() != log->m_flSpawntime)
	{
		// reset animation state.
		ResetAnimationState(state);

		// note new spawn time.
		log->m_flSpawntime = m_player->get_spawn_time();
	}

	// backup curtime.
	float curtime = g_pGlobals->curtime;
	float frametime = g_pGlobals->frametime;

	// set curtime to animtime.
	// set frametime to ipt just like on the server during simulation.
	g_pGlobals->curtime = record->m_anim_time;
	g_pGlobals->frametime = g_pGlobals->interval_per_tick;

	// backup stuff that we do not want to fuck with.
	AnimationBackup_t backup;

	backup.m_origin = m_player->get_origin();
	backup.m_abs_origin = m_player->get_abs_origin();
	backup.m_velocity = m_player->get_velocity();
	backup.m_abs_velocity = m_player->get_abs_velocity();
	backup.m_flags = m_player->get_flags();
	backup.m_eflags = m_player->get_eflags();
	backup.m_duck = m_player->get_duck_amt();
	backup.m_body = m_player->get_lby();
	backup.m_layers = m_player->get_anim_layers();

	// is player a bot?
	bool bot = m_player->get_player_info().fakeplayer;

	// reset fakewalk state.
	record->m_fake_walk = false;
	//record->m_mode = Resolver::Modes::RESOLVE_NONE;

	// fix velocity.
	// https://github.com/VSES/SourceEngine2007/blob/master/se2007/game/client/c_baseplayer.cpp#L659
	if (record->m_lag > 0 && record->m_lag < 16 && log->record.size() >= 2)
	{
		// get pointer to previous record.
		auto previous = &log->record[1];

		if (previous && !previous->m_dormant)
			record->m_velocity = (record->m_origin - previous->m_origin) * (1.f / ticks_to_time(record->m_lag));
	}

	// set this fucker, it will get overriden.
	record->m_anim_velocity = record->m_velocity;

	// fix various issues with the game
	// these issues can only occur when a player is choking data.
	if (record->m_lag > 1 && !bot)
	{
		// detect fakewalk.
		float speed = record->m_velocity.Length();

		if (record->m_flags & FL_ONGROUND && record->m_layers[6].m_flWeight == 0.f && speed > 0.1f && speed < 100.f)
			record->m_fake_walk = true;

		if (record->m_fake_walk)
			record->m_anim_velocity = record->m_velocity = { 0.f, 0.f, 0.f };

		// we need atleast 2 updates/records
		// to fix these issues.
		if (log->record.size() >= 2)
		{
			// get pointer to previous record.
			auto previous = &log->record[1];

			if (previous && !previous->m_dormant)
			{
				// set previous flags.
				m_player->get_flags() = previous->m_flags;

				// strip the on ground flag.
				m_player->get_flags() &= ~FL_ONGROUND;

				// been onground for 2 consecutive ticks? fuck yeah.
				if (record->m_flags & FL_ONGROUND && previous->m_flags & FL_ONGROUND)
					m_player->get_flags() |= FL_ONGROUND;

				// fix jump_fall.
				else
				{
					if (record->m_layers[4].m_flWeight != 1.f && previous->m_layers[4].m_flWeight == 1.f && record->m_layers[5].m_flWeight != 0.f)
						m_player->get_flags() |= FL_ONGROUND;

					if (record->m_flags & FL_ONGROUND && !(previous->m_flags & FL_ONGROUND))
						m_player->get_flags() &= ~FL_ONGROUND;
				}

				// calc fraction between both records.
				float frac = (record->m_anim_time - record->m_sim_time) / (previous->m_sim_time - record->m_sim_time);

				m_player->get_duck_amt() = math::lerp(record->m_duck, previous->m_duck, frac);

				if (!record->m_fake_walk)
					record->m_anim_velocity = math::lerp(record->m_velocity, previous->m_velocity, frac);
			}
		}
	}

	// and not bot, player uses fake angles.
	bool fake = vars.aim.resolver.get<bool>() && !bot && (m_player->is_enemy() || vars.aim.resolve_team.get<bool>());

	// if using fake angles, correct angles.
	if (fake)
	{
		resolver::get().resolve(m_player);
		resolver::get().override(m_player);
	}
	else
		log->m_iMode = RMODE_MOVING;

	log->m_vecLastNonDormantOrig = record->m_origin;

	// set stuff before animating.
	m_player->get_origin() = record->m_origin;
	m_player->set_abs_origin(record->m_origin);

	m_player->get_velocity() = m_player->get_abs_velocity() = record->m_anim_velocity;
	m_player->get_lby() = record->m_body;

	// EFL_DIRTY_ABSVELOCITY
	// skip call to C_BaseEntity::CalcAbsoluteVelocity
	m_player->get_eflags() &= ~0x1000;

	// run body prediction.
	if (resolver::get().update_lby_timer(m_player))
		record->m_lby_flick = true;

	const auto backup_eye = m_player->get_eye_angles();

	if (record->m_lby_flick)
	{
		const auto lby = math::normalize_float(m_player->get_lby());
		record->m_eye_angles.y = lby;
		m_player->set_abs_angles(QAngle(0.f, lby, 0.f));
	}

	// write potentially resolved angles.
	m_player->get_eye_angles() = record->m_eye_angles;

	// fix animating in same frame.
	if (state->m_iLastClientSideAnimationUpdateFramecount == g_pGlobals->framecount)
		state->m_iLastClientSideAnimationUpdateFramecount -= 1;

	// 'm_animating' returns true if being called from SetupVelocity, passes raw velocity to animstate.
	g_setup_vel_fix = true;
	m_player->get_clientside_animation() = true;
	m_player->update_clientside_anim();
	m_player->get_clientside_animation() = false;
	g_setup_vel_fix = false;

	m_player->get_eye_angles() = backup_eye;

	// store updated/animated poses and rotation in lagrecord.
	record->m_poses = m_player->get_pose_params();
	record->m_abs_ang = m_player->get_abs_angles();

	// correct poses
	//resolver::resolve_poses( m_player, log );

	// restore backup data.
	m_player->get_origin() = backup.m_origin;
	m_player->get_velocity() = backup.m_velocity;
	m_player->get_abs_velocity() = backup.m_abs_velocity;
	m_player->get_flags() = backup.m_flags;
	m_player->get_eflags() = backup.m_eflags;
	m_player->get_duck_amt() = backup.m_duck;
	m_player->get_lby() = backup.m_body;
	m_player->set_abs_origin(backup.m_abs_origin);
	m_player->set_anim_layers(backup.m_layers);

	// IMPORTANT: do not restore poses here, since we want to preserve them for rendering.
	// also dont restore the render angles which indicate the model rotation.

	// restore globals.
	g_pGlobals->curtime = curtime;
	g_pGlobals->frametime = frametime;
}


void c_animation_system::update_custom_anims(C_CSPlayer* player)
{
	for (auto i = 1; i <= g_pGlobals->maxClients; i++)
	{
		const auto ent = get_entity(i);
		auto& info = anim_info[i];
		if ((!ent || ent != g_pLocalPlayer) && info.animstate)
		{
			g_pMemAlloc->Free(info.animstate);
			info.animstate = nullptr;
		}
	}

	if (!player)
		return;

	auto& info = anim_info[player->EntIndex()];

	if (player->get_spawn_time() != info.spawn_time || player->GetRefEHandle() != info.handle || !player->get_alive())
	{
		info.spawn_time = player->get_spawn_time();
		info.handle = player->GetRefEHandle();

		if (info.animstate)
		{
			g_pMemAlloc->Free(info.animstate);
			info.animstate = nullptr;
		}

		if (!player->get_alive())
			return;
	}
	if (!info.animstate)
	{
		info.animstate = reinterpret_cast<CCSGOPlayerAnimState*> (g_pMemAlloc->Alloc(860u));
		player->create_anim_state(info.animstate);
	}

	// backup curtime.
	const auto curtime = g_pGlobals->curtime;
	const auto frametime = g_pGlobals->frametime;

	// set curtime to animtime.
	// set frametime to ipt just like on the server during simulation.
	g_pGlobals->curtime = player->get_oldsimtime() + g_pGlobals->interval_per_tick;
	g_pGlobals->frametime = g_pGlobals->interval_per_tick;

	// backup stuff that we do not want to fuck with.
	const auto backup_origin = player->get_origin();
	const auto backup_velocity = player->get_velocity();
	const auto backup_abs_velocity = player->get_abs_velocity();
	const auto backup_flags = player->get_flags();
	const auto backup_eflags = player->get_eflags();
	const auto backup_duck_amt = player->get_duck_amt();
	const auto backup_lby = player->get_lby();
	auto backup_layers = player->get_anim_layers();
	auto backup_poses = player->get_pose_params();

	if (info.animstate->m_iLastClientSideAnimationUpdateFramecount == g_pGlobals->framecount)
		info.animstate->m_iLastClientSideAnimationUpdateFramecount -= 1;

	info.animstate->update(player->get_eye_angles());
	info.animvel = info.animstate->m_velocity;

	player->get_origin() = backup_origin;
	player->get_velocity() = backup_velocity;
	player->get_abs_velocity() = backup_abs_velocity;
	player->get_flags() = backup_flags;
	player->get_eflags() = backup_eflags;
	player->get_duck_amt() = backup_duck_amt;
	player->get_lby() = backup_lby;
	player->set_anim_layers(backup_layers);
	player->set_pose_params(backup_poses);

	g_pGlobals->curtime = curtime;
	g_pGlobals->frametime = frametime;
}


void c_animation_system::stop(const ClientFrameStage_t stage)
{
	if (stage != FRAME_RENDER_START)
		return;

	static auto clientSideAnimationList = *reinterpret_cast<ClientSideAnimationList**>(sig("client_panorama.dll", "A1 ? ? ? ? F6 44") + 1);

	const auto list = clientSideAnimationList;

	for (auto i = 0; i < list->Count(); i++)
	{
		auto& animating = list->Element(i);

		if (animating.entity && animating.entity->GetClientClass()->m_ClassID == ClassId_CCSPlayer)
		{

			if (animating.entity->is_enemy() && animating.flags & 1)
			{
				animating.flags &= ~1;
			}
		}
	}
}
void c_animation_system::update_local_animations()
{
	static auto wasonground = true;

	const auto player_flags = g_pLocalPlayer->get_flags();

	if (!wasonground || !(g_pLocalPlayer->get_flags() & FL_ONGROUND))
	{
		g_pLocalPlayer->get_flags() &= ~FL_ONGROUND;
	}

	if (!g_pClientState->m_nChokedCommands)
	{
		wasonground = player_flags & FL_ONGROUND;
	}

	const auto inlandanim = static_cast<bool>(g_pLocalPlayer->get_anim_state()->m_bInHitGroundAnimation);
	const bool onground = g_pLocalPlayer->get_flags() & FL_ONGROUND;

	if (inlandanim && onground && wasonground)
	{
		thirdperson::get().get_tp_angle().x = -10.f;
		thirdperson::get().get_tp_angle().y = g_pLocalPlayer->get_anim_state()->m_flGoalFeetYaw;
	}

	g_pLocalPlayer->get_flags() = player_flags;
	g_pLocalPlayer->get_visual_angles() = thirdperson::get().get_tp_angle();
}

void c_animation_system::fix_local_anims(const ClientFrameStage_t stage)
{
	if (stage != FRAME_RENDER_START)
		return;

	const auto reset = (!g_pLocalPlayer || !g_pLocalPlayer->get_alive());

	if (!g_pClientState->m_nChokedCommands || reset)
		update_custom_anims(g_pLocalPlayer);

	if (reset)
		return;

	const auto animstate = g_pLocalPlayer->get_anim_state();

	if (!g_pInput->m_fCameraInThirdPerson)
	{
		animstate->m_bInHitGroundAnimation = false;
		return;
	}

	update_local_animations();
}

c_animation_system::animation::animation(C_CSPlayer* player)
{
	/*const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(player->get_current_weapon_handle()));*/

	const auto weapon = get_weapon(player->get_active_weapon());


	this->player = player;
	index = player->EntIndex();
	dormant = player->IsDormant();
	velocity = player->get_velocity();
	origin = player->get_origin();
	abs_origin = player->get_abs_origin();
	obb_mins = player->get_mins();
	obb_maxs = player->get_maxs();
	layers = *player->get_animation_layers2();
	poses = player->get_pose_parameter2();
	if ((has_anim_state = player->get_anim_state()))
		anim_state = *player->get_anim_state();
	anim_time = player->get_oldsimtime() + g_pGlobals->interval_per_tick;
	sim_time = player->get_simtime();
	interp_time = 0.f;
	last_shot_time = weapon ? weapon->get_last_shot_time() : 0.f;
	duck = player->get_duck_amt();
	lby = player->get_lby();
	eye_angles = player->get_eye_angles();
	abs_ang = player->get_abs_angles();
	flags = player->get_flags();
	eflags = player->get_eflags();
	effects = player->get_effects();
	lag = time_to_ticks(player->get_simtime() - player->get_oldsimtime());

	// animations are off when we enter pvs, we do not want to shoot yet.
	valid = lag >= 0 && lag <= 17;

	// clamp it so we don't interpolate too far : )
	lag = std::clamp(lag, 0, 17);
}

c_animation_system::animation::animation(C_CSPlayer* player, QAngle last_reliable_angle) : animation(player)
{
	this->last_reliable_angle = last_reliable_angle;
}
bool c_animation_system::animation::is_valid(float sim_time, bool this_valid, float range)
{

	const auto net_channel = g_pEngine->GetNetChannelInfo();

	if (!net_channel || !this_valid)
		return false;

	const auto local = g_pLocalPlayer;

	float max_unlag = 0.2f;

	static auto sv_maxunlag = g_pCVar->FindVar("sv_maxunlag");

	if (sv_maxunlag)
		max_unlag = sv_maxunlag->get_float();

	const auto correct = std::clamp(net_channel->GetLatency(FLOW_INCOMING)
		+ net_channel->GetLatency(FLOW_OUTGOING)
		+ calculate_lerp(), 0.f, max_unlag);

	return fabsf(correct - (g_pGlobals->curtime - sim_time)) <= range;
}


void c_animation_system::animation::restore(C_CSPlayer* player) const
{
	player->get_velocity() = velocity;
	player->get_flags() = flags;
	player->get_eflags() = eflags;
	player->get_duck_amt() = duck;
	*player->get_animation_layers2() = layers;
	player->get_lby() = lby;
	player->get_origin() = origin;
	player->set_abs_origin(abs_origin);
}

void c_animation_system::animation::apply(C_CSPlayer* player) const
{



	player->get_pose_parameter2() = poses;
	player->get_eye_angles() = eye_angles;
	player->get_velocity() = player->get_abs_velocity() = velocity;
	player->get_lby() = lby;
	player->get_duck_amt() = duck;
	player->get_flags() = flags;
	player->get_origin() = origin;
	player->set_abs_origin(origin);
	if (player->get_anim_state() && has_anim_state)
		*player->get_anim_state() = anim_state;
}

void c_animation_system::animation::build_server_bones(C_CSPlayer* player)
{
	auto local_player = g_pLocalPlayer;

	bool is_local_player = player == local_player;//check

	// keep track of old occlusion values
	const auto backup_occlusion_flags = player->get_occlusion_flags();
	const auto backup_occlusion_framecount = player->get_occlusion_framecount();

	// skip occlusion checks in c_cs_player::setup_bones
	if (!is_local_player)
	{
		player->get_occlusion_flags() = 0;
		player->get_occlusion_framecount() = 0;
	}

	// clear bone accessor
	player->get_readable_bones() = player->get_writable_bones() = 0;

	// invalidate bone cache
	player->get_most_recent_model_bone_counter() = 0;
	player->get_last_bone_setup_time() = -FLT_MAX;

	// stop interpolation
	player->get_effects() |= C_BaseEntity::ef_nointerp;

	// change bone accessor
	const auto backup_bone_array = player->get_bone_array_for_write();

	if (!is_local_player)
		player->get_bone_array_for_write() = bones;

	// build bones

	player->SetupBones(nullptr, -1, BONE_USED_BY_ANYTHING, g_pGlobals->curtime);

//	player->setup_bones(g_pGlobals->curtime, nullptr, BONE_USED_BY_ANYTHING);
//	player->setup_bones(*current, BONE_USED_BY_ANYTHING, current->matrix);
	// restore bone accessor
	player->get_bone_array_for_write() = backup_bone_array;

	// restore original occlusion
	if (!is_local_player)
	{
		player->get_occlusion_flags() = backup_occlusion_flags;
		player->get_occlusion_framecount() = backup_occlusion_framecount;
	}

	// start interpolation again
	player->get_effects() &= ~C_BaseEntity::ef_nointerp;
}
void c_animation_system::animation_info::update_animations(animation* record, animation* from)
{
	if (!from)
	{
		// set velocity and layers.
		record->velocity = player->get_velocity();

		// fix feet spin.
		record->anim_state.m_flFeetYawRate = 0.f;

		// resolve player.
	/*	switch (config.rage.resolver) {
		case 1: c_resolver::myframework(record);
			break;
		case 2: c_resolver_beta::resolve(record);
			break;
		}*/

		resolver::get().resolve(record->player);
		resolver::get().override(record->player);

		// apply record.
		record->apply(player);

		// run update.
		return instance()->update_player(player);
	}

	const auto new_velocity = player->get_velocity();

	// restore old record.
	*player->get_animation_layers2() = from->layers;
	player->set_abs_origin(record->origin);
	player->set_abs_angles(from->abs_ang);
	player->get_velocity() = from->velocity;




	// setup velocity.
	record->velocity = new_velocity;

	// did the player shoot?
	const auto shot = record->last_shot_time > from->sim_time&& record->last_shot_time <= record->sim_time;

	if (shot)
		record->didshot = true;
	else {
		const auto deltaAngle = fabsf(normalize_yaw(calc_angle2(record->origin, g_pLocalPlayer->get_eye_pos()).y - record->abs_ang.y));

		record->sideways = deltaAngle >= 90 - 15 && deltaAngle <= 90 + 15;

		record->upPitch = record->abs_ang.x < -50;//temp
	}

	// setup extrapolation parameters.
	auto old_origin = from->origin;
	auto old_flags = from->flags;

	for (auto i = 0; i < record->lag; i++)
	{
		// move time forward.
		const auto time = from->sim_time + ticks_to_time(i + 1);
		const auto lerp = 1.f - (record->sim_time - time) / (record->sim_time - from->sim_time);

		// lerp eye angles.
		auto eye_angles = interpolate(from->eye_angles, record->eye_angles, lerp);
		normalize(eye_angles);
		player->get_eye_angles() = eye_angles;

		// lerp duck amount.
		player->get_duck_amt() = interpolate(from->duck, record->duck, lerp);

		// resolve player.
		if (record->lag - 1 == i)
		{
			player->get_velocity() = new_velocity;
			player->get_flags() = record->flags;

			if (record->lag > 1)
			{


				resolver::get().resolve(record->player);
				resolver::get().override(record->player);

				/*switch (config.rage.resolver) {
				case 1: c_resolver::myframework(record);
					break;
				case 2: c_resolver_beta::resolve(record);
					break;
				}*/
			}
		}
		else // compute velocity and flags.
		{
		//	c_trace_system::extrapolate(player, old_origin, player->get_velocity(), player->get_flags(), old_flags & c_base_player::on_ground);
			old_flags = player->get_flags();
		}



		// correct shot desync.
		if (shot)
		{
			player->get_eye_angles() = record->last_reliable_angle;

			if (record->last_shot_time <= time)
				player->get_eye_angles() = record->eye_angles;
		}
		else
			player->get_eye_angles() = record->last_reliable_angle;


		// instant approach.
		if (player->get_velocity().Length2D() < .1f && fabsf(player->get_velocity().z) < 100.f && record->lag > 1)
			c_animation_system::instance()->last_process_state = player->get_anim_state();

		// fix feet spin.
		player->get_anim_state()->m_flFeetYawRate = 0.f;

		// backup simtime.
		const auto backup_simtime = player->get_simtime();

		// set new simtime.
		player->get_simtime() = time;

		// run update.
		instance()->update_player(player);

		// restore old simtime.
		player->get_simtime() = backup_simtime;
	}
}


void c_animation_system::post_player_update()
{
	if (!g_pEngine->IsInGame())
		return;

	const auto local = g_pLocalPlayer;

	// erase outdated entries
	for (auto it = animation_infos.begin(); it != animation_infos.end();) {
		auto player = reinterpret_cast<C_CSPlayer*>(g_pEntitiyList->GetClientEntityFromHandle(it->first));

		if (!player || player != it->second.player || !player->get_alive() || !player->is_enemy()
			|| !local || !local->get_alive())
		{
			if (player)
				player->get_clientside_animation() = true;
			it = animation_infos.erase(it);
		}
		else
			it = next(it);
	}

	if (!local || !local->get_alive())
	{
		for_each_player([](C_CSPlayer* player) -> void
			{
				player->get_clientside_animation() = true;
			});
		return;
	}

	// create new entries and reset old ones
	for_each_player([&](C_CSPlayer* player) -> void {
		bool is_local_player = player == local;//check
		if (!player->is_enemy() && !is_local_player)
			player->get_clientside_animation() = true;

		if (!player->get_alive() || player->IsDormant() || is_local_player || !player->is_enemy())
			return;

		if (animation_infos.find(player->GetRefEHandle()) == animation_infos.end())
			animation_infos.insert_or_assign(player->GetRefEHandle(), animation_info(player, {}));
		});

	// run post update
	for (auto& info : animation_infos)
	{
		auto& animation = info.second;
		const auto player = animation.player;

		// erase frames out-of-range
		for (auto it = animation.frames.rbegin(); it != animation.frames.rend();) {
			if (g_pGlobals->curtime - it->sim_time > 1.2f)
				it = decltype(it) { info.second.frames.erase(next(it).base()) };
			else
				it = next(it);
		}

		// have we already seen this update?
		if (player->get_simtime() == player->get_oldsimtime())
			continue;

		// reset animstate
		if (animation.last_spawn_time != player->get_spawn_time())
		{
			const auto state = player->get_anim_state();
			if (state)
				state->reset();

			animation.last_spawn_time = player->get_spawn_time();
		}

		// grab weapon
		const auto weapon = get_weapon(player->get_active_weapon());

		if (!weapon)
			return;

		// make a full backup of the player
		auto backup = c_animation_system::animation(player);
		backup.apply(player);

		// grab previous
		c_animation_system::animation* previous = nullptr;

		if (!animation.frames.empty() && !animation.frames.front().dormant && time_to_ticks(player->get_simtime() - animation.frames.front().sim_time) <= 17)
			previous = &animation.frames.front();

		// update shot info
		const auto shot = weapon && previous && weapon->get_last_shot_time() > previous->sim_time&& weapon->get_last_shot_time() <= player->get_simtime();

		if (!shot)
			info.second.last_reliable_angle = player->get_eye_angles23();

		// store server record
		auto& record = animation.frames.emplace_front(player, info.second.last_reliable_angle);

		// run full update
		animation.update_animations(&record, previous);

		// restore server layers
		*player->get_animation_layers2() = backup.layers;

		// use uninterpolated data to generate our bone matrix
		record.build_server_bones(player);

		// restore correctly synced values
		backup.restore(player);

		// set record to latest animation
		animation.latest_animation = record;
	}
}

void c_animation_system::update_player(C_CSPlayer* player)
{
	const auto local = g_pLocalPlayer;
	bool is_local_player = player == local;//check

	const auto state = player->get_anim_state();

/*	static auto& enable_bone_cache_invalidation = **reinterpret_cast<bool**>(
		reinterpret_cast<uintptr_t>(sig("client_panorama.dll", "C6 05 ? ? ? ? ? 89 47 70")) + 2);*/


//	static auto& enable_bone_cache_invalidation = *reinterpret_cast<uintptr_t*>(sig("client_panorama.dll", "C6 05 ? ? ? ? ? 89 47 70") + 0x2);


	static auto& enable_bone_cache_invalidation2 = player->get_enable_invalidate_bone_cache();

	

	// make a backup of globals
	const auto backup_frametime = g_pGlobals->frametime;
	const auto backup_curtime = g_pGlobals->curtime;

	// get player anim state
//	const auto state = player->get_anim_state();

	// fixes for networked players
	if (!is_local_player)
	{
		g_pGlobals->frametime = g_pGlobals->interval_per_tick;
		g_pGlobals->curtime = player->get_simtime();
		player->get_eflags() &= ~C_BaseEntity::efl_dirty_absvelocity;
	}

	// allow reanimating in the same frame
	if (state->m_iLastClientSideAnimationUpdateFramecount == g_pGlobals->framecount)
		state->m_iLastClientSideAnimationUpdateFramecount -= 1;

	// make sure we keep track of the original invalidation state
	const auto old_invalidation = enable_bone_cache_invalidation2;

	// notify the other hooks to instruct animations and pvs fix
	instance()->enable_bones = player->get_clientside_animation() = true;
	player->update_clientside_anim();
	instance()->enable_bones = false;

	if (!is_local_player)
		player->get_clientside_animation() = false;

	instance()->last_process_state = nullptr;

	// invalidate physics.
	if (!is_local_player)
		player->invalidate_physics_recursive(C_BaseEntity::angles_changed
			| C_BaseEntity::animation_changed
			| C_BaseEntity::sequence_changed);

	// we don't want to enable cache invalidation by accident
	enable_bone_cache_invalidation2 = old_invalidation;

	// restore globals
	g_pGlobals->frametime = backup_frametime;
	g_pGlobals->curtime = backup_curtime;
}

void c_animation_system::update_simple_local_player(C_CSPlayer* player, CUserCmd* cmd)
{

	/* perfect for me*/
	if (player && !player->get_alive())
		return;

	player->get_pose_parameter2() = instance()->local_animation.poses;
	*player->get_animation_layers2() = instance()->server_layers;
	instance()->local_animation.eye_angles = cmd->viewangles;
	normalize(instance()->local_animation.eye_angles);

	player->get_anim_state()->m_flFeetYawRate = 0.f;
	instance()->update_player(player);
	instance()->local_animation.abs_ang.y = player->get_anim_state()->m_flGoalFeetYaw;
	instance()->local_animation.layers = *player->get_animation_layers2();
	instance()->local_animation.poses = player->get_pose_parameter2();
}



void c_animation_system::local_fix()
{
	auto local_player = g_pLocalPlayer;

	if (!local_player)
		return;

	static float sim_time;
	if (sim_time != local_player->get_simtime())
	{
		auto state = local_player->get_anim_state(); if (!state) return;

		const float curtime = g_pGlobals->curtime;
		const float frametime = g_pGlobals->frametime;
		const float realtime = g_pGlobals->realtime;
		const float absoluteframetime = g_pGlobals->absoluteframetime;
		const float absoluteframestarttimestddev = g_pGlobals->absoluteframestarttimestddev;
		const float interpolation_amount = g_pGlobals->interpolation_at;
		const float framecount = g_pGlobals->framecount;
		const float tickcount = g_pGlobals->tickcount;

		static auto host_timescale = g_pCVar->FindVar(("host_timescale"));

		g_pGlobals->curtime = local_player->get_simtime();
		g_pGlobals->realtime = local_player->get_simtime();
		g_pGlobals->frametime = g_pGlobals->interval_per_tick * host_timescale->get_float();
		g_pGlobals->absoluteframetime = g_pGlobals->interval_per_tick * host_timescale->get_float();
		g_pGlobals->absoluteframestarttimestddev = local_player->get_simtime() - g_pGlobals->interval_per_tick * host_timescale->get_float();
		g_pGlobals->interpolation_at = 0;
		g_pGlobals->framecount = ticks_to_time(local_player->get_simtime());
		g_pGlobals->tickcount = ticks_to_time(local_player->get_simtime());
		int backup_flags = local_player->get_flags();

	    C_AnimationLayer backup_layers[15];
		std::memcpy(backup_layers, local_player->AnimOverlays3(), (sizeof(C_AnimationLayer) * 15));

		if (state->m_iLastClientSideAnimationUpdateFramecount == g_pGlobals->framecount)
			state->m_iLastClientSideAnimationUpdateFramecount = g_pGlobals->framecount - 1;

		std::memcpy(local_player->AnimOverlays3(), backup_layers, (sizeof(C_AnimationLayer) * 15));


		g_pGlobals->curtime = curtime;
		g_pGlobals->realtime = realtime;
		g_pGlobals->frametime = frametime;
		g_pGlobals->absoluteframetime = absoluteframetime;
		g_pGlobals->absoluteframestarttimestddev = absoluteframestarttimestddev;
		g_pGlobals->interpolation_at = interpolation_amount;
		g_pGlobals->framecount = framecount;
		g_pGlobals->tickcount = tickcount;
		sim_time = local_player->get_simtime();
	}
	//local_player->invalidate_bone_cache();
	local_player->SetupBones(nullptr, -1, 0x7FF00, g_pGlobals->curtime);
}
