#pragma once
#include <deque>
#include <utility>
#include <mutex>
#include <optional>
#include "xmmintrin.h"

template<typename t>
class c_singleton {
public:
	template<typename... Args>
	static t* instance(Args&&... args);
protected:
	c_singleton() = default;
	~c_singleton() = default;
};

template<typename t>
template<typename... Args>
t* c_singleton<t>::instance(Args&&... args)
{
	static t instance(std::forward<Args>(args)...);
	return &instance;
}




template< typename t = float >
t c_maxes(const t& a, const t& b) {
	// check type.
	static_assert(std::is_arithmetic< t >::value, "math::max only supports integral types.");
	return (t)_mm_cvtss_f32(
		_mm_max_ss(_mm_set_ss((float)a),
			_mm_set_ss((float)b))
	);
}

enum backtrack_mode {
	backtrack_high,
	backtrack_last_record
};

enum resolver_state /*WHEN STANDING/SLOW WALKING USE THIS FOR NOW*/
{
	resolver_start,
	resolver_inverse,
	resolver_no_desync,
	resolver_jitter
};

enum resolver_state_moving { /* LETS ATTEMPT A DIFFERENT METHOD OF MOVING RESOLVER*/
	resolver_moving_start,
	resolver_moving_start_inverse,
	resolver_moving_inverse,
	resolver_moving_no_desync,
	resolver_moving_jitter,
};

enum resolver_slow_walk_state {
	resolver_slow_walk_begin,
	resolver_slow_walk_inverse
};

__forceinline float calculate_lerp()
{
	static auto cl_ud_rate = g_pCVar->FindVar("cl_updaterate");
	static auto min_ud_rate = g_pCVar->FindVar("sv_minupdaterate");
	static auto max_ud_rate = g_pCVar->FindVar("sv_maxupdaterate");

	int ud_rate = 64;

	if (cl_ud_rate)
		ud_rate = cl_ud_rate->get_int();

	if (min_ud_rate && max_ud_rate)
		ud_rate = max_ud_rate->get_int();

	float ratio = 1.f;
	static auto cl_interp_ratio = g_pCVar->FindVar("cl_interp_ratio");

	if (cl_interp_ratio)
		ratio = cl_interp_ratio->get_float();

	static auto cl_interp = g_pCVar->FindVar("cl_interp");
	static auto c_min_ratio = g_pCVar->FindVar("sv_client_min_interp_ratio");
	static auto c_max_ratio = g_pCVar->FindVar("sv_client_max_interp_ratio");

	float lerp = g_pGlobals->interval_per_tick;

	if (cl_interp)
		lerp = cl_interp->get_float();

	if (c_min_ratio && c_max_ratio && c_min_ratio->get_float() != 1)
		ratio = std::clamp(ratio, c_min_ratio->get_float(), c_max_ratio->get_float());

	return c_maxes(lerp, ratio / ud_rate);
}
struct anim_state_info_t
{
	CBaseHandle	handle;
	float spawn_time;
	CCSGOPlayerAnimState* animstate;
	float animvel;
	std::array<float, 24> poses;
};
class c_animation_system : public c_singleton<c_animation_system>
{
public:
	struct animation
	{
		animation() = default;

		explicit animation(C_CSPlayer* player);
		explicit animation(C_CSPlayer* player, QAngle last_reliable_angle);

		void restore(C_CSPlayer* player) const;
		void apply(C_CSPlayer* player) const;
		void build_server_bones(C_CSPlayer* player);

		bool is_valid(float sim_time, bool this_valid, const float range = .2f);

		C_CSPlayer* player{};
		int32_t index{};

		bool valid{}, has_anim_state{};
		alignas(16) matrix3x4_t bones[128]{};

		bool dormant{};

		Vector velocity;
		Vector origin;
		Vector abs_origin;
		Vector obb_mins;
		Vector obb_maxs;

		animation_layers layers{};
		CCSGOAnimStatePoses poses{};

		CCSGOPlayerAnimState anim_state{};

		float anim_time{};
		float sim_time{};
		float interp_time{};
		float duck{};
		float lby{};
		float last_shot_time{};

		QAngle last_reliable_angle{};
		QAngle eye_angles;
		QAngle abs_ang;

		int flags{};
		int eflags{};
		int effects{};
		int lag{};
		int didshot{};
		int sideways{};
		int upPitch{};
		int record_priority{};
	};
private:
	struct animation_info {
		animation_info(C_CSPlayer* player, std::deque<animation> animations)
			: player(player), frames(std::move(animations)), last_spawn_time(0) { }

		void update_animations(animation* to, animation* from);

		C_CSPlayer* player{};
		std::deque<animation> frames{};

		// latest animation (might be invalid)
		animation latest_animation{};

		// last time this player spawned
		float last_spawn_time;

		// counter of how many shots we missed
		int32_t missed_due_to_spread{};
		int32_t missed_due_to_resolver{};

		// resolver data
		resolver_state brute_state{};
		resolver_state_moving brute_moving_state{};
		resolver_slow_walk_state brute_slowwalk_state{};

		float brute_yaw{};
		float moving_brute_yaw{};
		float slowwalk_brute_yaw{};

		Vector last_reliable_angle{};
	};

	std::unordered_map<CBaseHandle, animation_info> animation_infos;
	anim_state_info_t anim_info[65];

public:
	void update_player(C_CSPlayer* player);
	void update_simple_local_player(C_CSPlayer* player, CUserCmd* cmd);
	void local_fix();
	void update_player_animations(lag_record_t* record, C_CSPlayer* m_player);
	void update_custom_anims(C_CSPlayer* player);
	void stop(const ClientFrameStage_t stage);
	void update_local_animations();
	void fix_local_anims(const ClientFrameStage_t stage);
	void post_player_update();

	animation_info* get_animation_info(C_CSPlayer* player);

	//
	std::optional<animation*> get_latest_animation(C_CSPlayer* player);
	std::optional<animation*> get_oldest_animation(C_CSPlayer* player);
	std::optional<animation*> get_uncrouched_animation(C_CSPlayer* player);

	std::optional<animation*> get_latest_firing_animation(C_CSPlayer* player);
	std::optional<animation*> get_latest_upPitch_animation(C_CSPlayer* player);
	std::optional<animation*> get_latest_sideways_animation(C_CSPlayer* player);
	std::optional<animation*> get_oldest_firing_animation(C_CSPlayer* player);
	std::optional<animation*> get_firing_uncrouched_animation(C_CSPlayer* player);
	//

	std::optional<std::pair<animation*, animation*>> get_intermediate_animations(C_CSPlayer* player, float range = 1.f);
	std::optional<animation*> get_lastest_animation_unsafe(C_CSPlayer* player);

	std::vector<animation*> get_valid_animations(C_CSPlayer* player, float range = 1.f);

	animation local_animation;
	animation_layers server_layers{};
	bool in_jump{}, enable_bones{};
	CCSGOPlayerAnimState* last_process_state{};
};
