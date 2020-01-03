#pragma once

struct shot_t
{
	shot_t( const Vector& shotpos, const int tick, const int& enemy_index, const lag_record_t* record )
	{
		this->shotpos = shotpos;
		this->enemy_index = enemy_index;
		this->tick = tick;
		if ( record )
			this->record = *record;
		else
			this->record.index = -1;

		this->hit = false;
		this->hurt = false;
	}
	Vector shotpos;
	Vector hitpos;
	bool hit;
	bool hurt;
	struct
	{
		int victim = -1;
		int damage = -1;
		int hitgroup = -1;
	}hitinfo;
	int tick;
	int enemy_index;
	lag_record_t record;
};

struct resolver_infos
{
	C_AnimationLayer animation;

	bool jitter_desync = false, high_delta = false, low_delta = false, static_desync = false, lby_changed = false;

	float last_lby = FLT_MAX, lby_delta = FLT_MAX, last_eyes = FLT_MAX, eye_delta = FLT_MAX, eye_lby_delta = FLT_MAX, eyes = FLT_MAX;

};
struct resolverInfo
{
public:
	float fakegoalfeetyaw;
	resolver_infos current_tick;
	resolver_infos previous_tick;

};

class resolver : public singleton<resolver>
{
public:
	void resolve(C_CSPlayer* player);
	static bool is_spin( player_log_t* log );
	void anim(const ClientFrameStage_t stage);
	void collect_wall_detect( const ClientFrameStage_t stage );
	void extrpolate_players( const ClientFrameStage_t stage ) const;
	
	void AnimationFix(C_CSPlayer* pEnt);
	//void resolve( C_CSPlayer* player, lag_record_t* record );
	bool wall_detect( player_log_t* log, lag_record_t* record, float& angle ) const;
	static void nospread_resolve( C_CSPlayer* player, lag_record_t* record );
	void override( C_CSPlayer* player );
	void update_missed_shots( const ClientFrameStage_t& stage );
	std::deque<shot_t>& get_shots();
	static void resolve_proxy( const C_CSPlayer* player, float* m_float );
	static void on_lby_proxy( C_CSPlayer* entity, float* LowerBodyYaw );
	static void resolve_poses( C_CSPlayer* player, player_log_t* log );
	static bool update_lby_timer( C_CSPlayer* player );
	static void calc_missed_shots( shot_t* shot );
	void add_shot( const Vector& shotpos, lag_record_t* record, const int& enemy_index );
	void hurt_listener( IGameEvent * game_event );
	shot_t* closest_shot( int tickcount );
	void record_shot( IGameEvent* game_event );
	void listener( IGameEvent * game_event );
	void approve_shots( const ClientFrameStage_t & stage );
	static float get_next_update( int index );
	void set_last_eye() { last_eye = g_pLocalPlayer->get_eye_pos(); }
	inline static resolverInfo __player[64];
	inline static resolverInfo resolverinfoo;
private:
	std::deque<shot_t> shots;
	std::deque<shot_t> current_shots;
	std::deque<shot_t> unapproved_shots;

	Vector last_eye;

	float left_damage[ 64 ];
	float right_damage[ 64 ];
	float back_damage[ 64 ];
};