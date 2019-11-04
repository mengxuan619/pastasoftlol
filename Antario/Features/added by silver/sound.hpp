#pragma once
#include <windows.h>
#include <iostream>
#include <cstdint>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <array>
#include <fstream>
#include <istream>
#include <unordered_map>
#include <intrin.h>

#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")


#include "../../hooks.h"
#include "../../Menu/config.h"
#include "../../Menu/TGFCfg.h"
#include "../../SDK/singleton.h"
#include "..\..\SDK\IVRenderBeams.h"
#include "../../Utils/Math.h"
#include "color.hpp"
#include "../../Utils/GlobalVars.h"

class c_sound_info {
public:
	c_sound_info(vec3_t positions, float times, int userids) {
		this->position = positions;
		this->time = times;
		this->userid = userids;
	}

	vec3_t position;
	float time;
	int userid;
};

class c_sound_esp : public singleton<c_sound_esp> {
public:
	void draw();
	void draw_circle(color colors, vec3_t position);
	void event_player_footstep(IGameEvent* event);
	void event_player_hurt(IGameEvent* event);
};

extern c_sound_esp sound_esp;