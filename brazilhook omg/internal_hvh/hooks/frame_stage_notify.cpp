#include "../include_cheat.h"

void __stdcall hooks::frame_stage_notify( ClientFrameStage_t stage )
{
	const auto local = g_pLocalPlayer;

	if ( g_erase )
	{
		erase_fn( init::on_startup );
		g_erase = false;
	}

	chams::get().dark_mode();

	if ( g_unload )
		return orig_frame_stage_notify( stage );

	freestanding::get().update_hotkeys( stage );

	misc::remove_smoke( stage );

	misc::remove_flash( stage );

	resolver::get().approve_shots( stage );

	resolver::get().update_missed_shots( stage );

	///fake_ping::get().update( stage );

	//c_animation_system::instance()->stop( stage );

	c_animation_system::instance()->fix_local_anims( stage );

	inventorychanger::get().update_equipped( stage ); // need fix patters

	//skinchanger::get().run( stage ); // need fix patterns

	orig_frame_stage_notify( stage );

	resolver::get().extrpolate_players( stage );
	//resolver::get().anim(stage);

	resolver::get().collect_wall_detect( stage );

	player_log::get().log( stage );

	


	if (stage == FRAME_NET_UPDATE_END)
		c_animation_system::instance()->post_player_update();

	if (stage == FRAME_RENDER_START)
	{
		if (local && local->get_alive())
			local->set_abs_angles(c_animation_system::instance()->local_animation.abs_ang);

		/*for (auto i = 1; i < g_pGlobals->maxClients; i++)
		{
			auto player = get_entity(i);
			if (!player || !player->get_alive() || player->IsDormant() || !player->is_enemy() || player->get_immunity())
				continue;
			if (player == local) {
				c_animation_system::instance()->local_fix();
			}


		}*/

	}

	if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START && local && local->get_alive())
	{
		c_animation_system::instance()->server_layers = *local->get_animation_layers2();
	}


}