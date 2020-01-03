#include "Legit Aimbot.h"
#include "../Aimbot/Aimbot.h"
#include "../../Menu/Menu.h"
#include "../../Menu/config.h"
#include "../Visuals/ESP.h"
#include "..\..\SDK\ICvar.h"
#include "..\..\SDK\IVModelInfo.h"
float m_fbestfov = 20.0f;
int m_ibesttargetlegit = -1;

float deltaTime;
float curAimTime;

LegitAimbot g_LegitAimbot;

mstudiobbox_t* GetHitbox(C_BaseEntity* entity, int hitbox_index)
{
	if (entity->IsDormant() || entity->GetHealth() <= 0)
		return NULL;

	const auto pModel = entity->GetModel();
	if (!pModel)
		return NULL;

	auto pStudioHdr = g_pModelInfo->GetStudiomodel(pModel);
	if (!pStudioHdr)
		return NULL;

	auto pSet = pStudioHdr->GetHitboxSet(0);
	if (!pSet)
		return NULL;

	if (hitbox_index >= pSet->numhitboxes || hitbox_index < 0)
		return NULL;

	return pSet->GetHitbox(hitbox_index);
}

void VectorTransform(const Vector in1, const matrix3x4_t in2, Vector& out) {
	out[0] = DotProduct(in1, Vector(in2[0][0], in2[0][1], in2[0][2])) + in2[0][3];
	out[1] = DotProduct(in1, Vector(in2[1][0], in2[1][1], in2[1][2])) + in2[1][3];
	out[2] = DotProduct(in1, Vector(in2[2][0], in2[2][1], in2[2][2])) + in2[2][3];
}

Vector ClampAngle(Vector angle)
{
	while (angle.y > 180) angle.y -= 360;
	while (angle.y < -180) angle.y += 360;

	if (angle.x > 89.0f) angle.x = 89.0f;
	if (angle.x < -89.0f) angle.x = -89.0f;

	angle.z = 0.f;

	return angle;
}

void ClampAngles(Vector& angles) {
	if (angles.y > 180.0f)
		angles.y = 180.0f;
	else if (angles.y < -180.0f)
		angles.y = -180.0f;

	if (angles.x > 89.0f)
		angles.x = 89.0f;
	else if (angles.x < -89.0f)
		angles.x = -89.0f;

	angles.z = 0;
}

void NormalizeAngles(Vector& angles)
{
	for (auto i = 0; i < 3; i++) {
		while (angles[i] < -180.0f) angles[i] += 360.0f;
		while (angles[i] > 180.0f) angles[i] -= 360.0f;
	}
}

Vector NormalizeAngle(Vector angle)
{
	while (angle.x < -180.0f) angle.x += 360.0f;
	while (angle.x > 180.0f) angle.x -= 360.0f;

	while (angle.y < -180.0f) angle.y += 360.0f;
	while (angle.y > 180.0f) angle.y -= 360.0f;

	while (angle.z < -180.0f) angle.z += 360.0f;
	while (angle.z > 180.0f) angle.z -= 360.0f;

	return angle;
}

float NormalizeYaw(float yaw)
{
	if (yaw > 180)
		yaw -= (round(yaw / 360) * 360.f);
	else if (yaw < -180)
		yaw += (round(yaw / 360) * -360.f);

	return yaw;
}

bool Clamp(Vector& angles)
{
	Vector a = angles;
	NormalizeAngles(a);
	ClampAngles(a);

	if (isnan(a.x) || isinf(a.x) ||
		isnan(a.y) || isinf(a.y) ||
		isnan(a.z) || isinf(a.z)) {
		return false;
	}
	else {
		angles = a;
		return true;
	}
}

Vector CalcAngle(const Vector& vecSource, const Vector& vecDestination)
{
	Vector qAngles;
	Vector delta = Vector((vecSource[0] - vecDestination[0]), (vecSource[1] - vecDestination[1]), (vecSource[2] - vecDestination[2]));
	float hyp = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);
	qAngles[0] = (float)(atan(delta[2] / hyp) * (180.0f / M_PI));
	qAngles[1] = (float)(atan(delta[1] / delta[0]) * (180.0f / M_PI));
	qAngles[2] = 0.f;
	if (delta[0] >= 0.f)
		qAngles[1] += 180.f;

	return qAngles;
}

Vector HitBoxPos(C_BaseEntity* entity, int hitbox_id)
{
	auto pHitbox = GetHitbox(entity, hitbox_id);

	if (!pHitbox)
		return Vector(0, 0, 0);

	auto pBoneMatrix = entity->GetBoneMatrix(pHitbox->bone);

	Vector bbmin, bbmax;
	g_Math.VectorTransform(pHitbox->min, pBoneMatrix, bbmin);
	g_Math.VectorTransform(pHitbox->max, pBoneMatrix, bbmax);

	return (bbmin + bbmax) * 0.5f;
}

template<typename T>
static T CubicInterpolate(T const& p1, T const& p2, T const& p3, T const& p4, float t)
{
	return p1 * (1 - t) * (1 - t) * (1 - t) +
		p2 * 3 * t * (1 - t) * (1 - t) +
		p3 * 3 * t * t * (1 - t) +
		p4 * t * t * t;
}

float LegitAimbot::GetFov() {
	if (!Globals::LocalPlayer->GetActiveWeapon())
		return 0;
	if (Globals::LocalPlayer->IsPistol()) {
		return c_config::get().pistol_fov;
	}
	else if (Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_AK47 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_M4A1 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_M4A1_SILENCER || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_GALILAR || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_FAMAS || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_AUG || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SG556) {
		return c_config::get().rifle_fov;
	}
	else if (Globals::LocalPlayer->IsSniper()) {
		return c_config::get().sniper_fov;
	}
	else if (Globals::LocalPlayer->IsShotgun()) {
		return c_config::get().shotgun_fov;
	}
	else if (Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_MAC10 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_MP5SD || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_MP7 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_MP9 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_FAMAS || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_UMP45 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_P90 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_BIZON) {
		return c_config::get().smg_fov;
	}
	else
	{
		return c_config::get().legit_aimbot_fov;
	}
}

float LegitAimbot::GetSmooth() {
	if (!Globals::LocalPlayer->GetActiveWeapon())
		return 0;

	if (Globals::LocalPlayer->IsPistol()) {
		return c_config::get().pistol_smooth;
	}
	else if (Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_AK47 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_M4A1 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_M4A1_SILENCER || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_GALILAR || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_FAMAS || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_AUG || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SG556) {
		return c_config::get().rifle_smooth;
	}
	else if (Globals::LocalPlayer->IsSniper()) {
		return c_config::get().sniper_smooth;
	}
	else if (Globals::LocalPlayer->IsShotgun()) {
		return c_config::get().shotgun_smooth;
	}
	else if (Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_MAC10 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_MP5SD || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_MP7 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_MP9 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_FAMAS || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_UMP45 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_P90 || Globals::LocalPlayer->GetActiveWeapon()->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_BIZON) {
		return c_config::get().smg_smooth;
	}
	else
	{
		return c_config::get().legitbotSmooth;
	}
}

void LegitAimbot::OnCreateMove()
{

	if (!c_config::get().legit_aimbot_enabled)
		return;

	static C_BaseEntity* preTarget = nullptr;
	static C_BaseEntity* curTarget = nullptr;
	static float t = 0.f;

	float bestFOV = LegitAimbot::GetFov(); // fov
	Vector viewAngles, engineAngles, angles;

	g_pEngine->GetViewAngles(engineAngles);
	Vector punchAngles = Globals::LocalPlayer->GetAimPunchAngle();
	static float recoil = g_pCvar->FindVar("weapon_recoil_scale")->GetFloat();

	if (Globals::LocalPlayer->IsKnifeorNade())
		return;

	if (!LegitAimbot::GetFov())
		return;

	if (c_config::get().legitbot_onkey && !GetAsyncKeyState(c_config::get().legitbotkey))
		return;

	if (c_config::get().scope_check && ((!Globals::LocalPlayer->IsScoped()) && Globals::LocalPlayer->IsSniper()))
		return;

	{

		if (c_config::get().rcs) {
			if (Globals::pCmd->buttons & IN_ATTACK) {
				Vector final_rcsd_shit = punchAngles * recoil;
				final_rcsd_shit.x *= (c_config::get().rcs_x / 100.f);
				final_rcsd_shit.y *= (c_config::get().rcs_y / 100.f);
				angles -= final_rcsd_shit;
			}
		}

		static bool was_firing = false;

		if (c_config::get().autopistol) {

			if (Globals::LocalPlayer->IsPistol()) {
				if (Globals::pCmd->buttons & IN_ATTACK && !Globals::LocalPlayer->IsKnifeorNade()) {
					if (was_firing) {
						Globals::pCmd->buttons &= ~IN_ATTACK;
					}
				}

				was_firing = Globals::pCmd->buttons & IN_ATTACK ? true : false;
			}

		}
		for (int it = 1; it <= g_pEngine->GetMaxClients(); ++it)
		{
			C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(it);

			if (!pPlayerEntity
				|| !pPlayerEntity->IsAlive()
				|| pPlayerEntity->IsDormant()
				|| pPlayerEntity == Globals::LocalPlayer
				|| pPlayerEntity->GetTeam() == Globals::LocalPlayer->GetTeam() && !c_config::get().FriendlyFireLegit)
				continue;

			if (c_config::get().legitHitbox == 0)
				angles = ClampAngle(NormalizeAngle(CalcAngle(Globals::LocalPlayer->GetEyePosition(), HitBoxPos(pPlayerEntity, HITBOX_HEAD))));
			else if (c_config::get().legitHitbox == 1)
				angles = ClampAngle(NormalizeAngle(CalcAngle(Globals::LocalPlayer->GetEyePosition(), HitBoxPos(pPlayerEntity, HITBOX_NECK))));
			else if (c_config::get().legitHitbox == 2)
				angles = ClampAngle(NormalizeAngle(CalcAngle(Globals::LocalPlayer->GetEyePosition(), HitBoxPos(pPlayerEntity, HITBOX_CHEST))));
			else if (c_config::get().legitHitbox == 3)
				angles = ClampAngle(NormalizeAngle(CalcAngle(Globals::LocalPlayer->GetEyePosition(), HitBoxPos(pPlayerEntity, HITBOX_LEFT_HAND))));
			else if (c_config::get().legitHitbox == 4)
				angles = ClampAngle(NormalizeAngle(CalcAngle(Globals::LocalPlayer->GetEyePosition(), HitBoxPos(pPlayerEntity, HITBOX_RIGHT_HAND))));
			else if (c_config::get().legitHitbox == 5)
				angles = ClampAngle(NormalizeAngle(CalcAngle(Globals::LocalPlayer->GetEyePosition(), HitBoxPos(pPlayerEntity, HITBOX_PELVIS))));

			float fov = (engineAngles - angles).Length2D();

			if (fov < bestFOV) {
				bestFOV = fov;
				viewAngles = angles;
				curTarget = pPlayerEntity;
			}
		}

		if (preTarget != curTarget || preTarget == nullptr) {
			t = 0.0f;
			preTarget = curTarget;
		}
		else if (preTarget == curTarget) {
			t += 0.03f;
		}

		if (bestFOV != LegitAimbot::GetFov())
		{
			if (t < 1.f && bestFOV > 1.f) //not really necessary to add a curve if we are this close
			{
				Vector src = engineAngles;
				Vector dst = viewAngles;

				Vector delta = src - dst;
				ClampAngle(delta);

				float randValPt1 = 15.0f + g_Math.RandomFloat(0.0f, 15.0f);
				float finalRandValPt1 = 3.f / randValPt1;
				Vector point1 = src + (delta * finalRandValPt1);
				ClampAngle(point1);

				float randValPt2 = 40.0f + g_Math.RandomFloat(0.0f, 15.0f);
				float finalRandValPt2 = 1.0f / randValPt2;
				Vector point2 = dst * (1.0f + finalRandValPt2);
				ClampAngle(point2);

				Vector angle = CubicInterpolate(src, point1, point2, dst, t);
				ClampAngle(angle);

				Globals::pCmd->viewangles = angle;
				g_pEngine->SetViewAngles(angle);
			}
			else
			{
				Vector smoothAngle = NormalizeAngle(viewAngles - engineAngles);
				viewAngles = engineAngles + (LegitAimbot::GetSmooth() ? smoothAngle / (float)c_config::get().legitbotSmooth * 2.f : smoothAngle);
				ClampAngle(viewAngles);
				Globals::pCmd->viewangles = viewAngles;

				if (!c_config::get().LegitSilentAim)
					g_pEngine->SetViewAngles(viewAngles);
			}
		}
	}
	/*else
	{
		t = 0.f;
	}*/

	Globals::pCurrentFOV = t;
}

Vector angle_vector(Vector to_convert) { //what the fuck was going on thru my head? these all should be math classes. sorry for pulling a psilent/uber/dumb paster, fellow gamer.
	auto y_sin = sin(to_convert.y / 180.f * static_cast<float>(M_PI));
	auto y_cos = cos(to_convert.y / 180.f * static_cast<float>(M_PI));

	auto x_sin = sin(to_convert.x / 180.f * static_cast<float>(M_PI));
	auto x_cos = cos(to_convert.x / 180.f * static_cast<float>(M_PI));

	return Vector(x_cos * y_cos, x_cos * y_sin, -x_sin);
}

float distance_point_to_line(Vector point, Vector origin, Vector direction) {
	auto delta = point - origin;

	auto temp = delta.Dot(direction) / (direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
	if (temp < 0.000001f)
		return FLT_MAX;

	auto temp_meme = origin + (direction * temp);
	return (point - temp_meme).Length();
}

struct backtrack_tick {
	float simulation_time;
	Vector hitbox_position;
	Vector origin;
	Vector angles;
	matrix3x4_t bt_matrix[128];
};

matrix3x4_t bone_matrix[128];
backtrack_tick backtrack[64][14];

void LegitAimbot::CM_Backtrack() {
	int best_target = -1;
	float best_fov = 90.f;

	if (!c_config::get().legit_aimbot_backtrack || !c_config::get().legit_aimbot_enabled)
		return;

	if (!Globals::LocalPlayer)
		return;

	if (Globals::LocalPlayer->GetHealth() <= 0)
		return;

	for (int i = 0; i <= 65; i++) {
		C_BaseEntity* ent = g_pEntityList->GetClientEntity(i);

		if (!ent) continue;
		if (ent->GetHealth() < 0) continue;
		if (ent->IsDormant()) continue;
		if (ent->GetTeam() == Globals::LocalPlayer->GetTeam()) continue;

		if (ent->GetHealth() > 0) { //useless check
			float sim_time = ent->GetSimulationTime();
			ent->FixSetupBones(bone_matrix);

			backtrack[i][Globals::pCmd->command_number % 14] = backtrack_tick
			{
				sim_time,
				ent->bone_pos(8),
				ent->GetAbsOrigin(),
				ent->GetAbsAngles(),
				bone_matrix[128]
			};

			Vector view_direction = angle_vector(Globals::pCmd->viewangles);
			float fov = distance_point_to_line(ent->bone_pos(8), Globals::LocalPlayer->bone_pos(8), view_direction);

			if (best_fov > fov) {
				best_fov = fov;
				best_target = i;
			}
		}
	}
	float best_target_simulation_time = 0.f;

	if (best_target != -1) {
		float temp = FLT_MAX;
		Vector view_direction = angle_vector(Globals::pCmd->viewangles);

		for (int t = 0; t < 14; ++t) {
			float fov = distance_point_to_line(backtrack[best_target][t].hitbox_position, Globals::LocalPlayer->bone_pos(8), view_direction);
			if (temp > fov && backtrack[best_target][t].simulation_time > Globals::LocalPlayer->GetSimulationTime() - 1) {
				temp = fov;
				best_target_simulation_time = backtrack[best_target][t].simulation_time;
			}
		}

		if (Globals::pCmd->buttons & IN_ATTACK) {
			Globals::pCmd->tick_count = (int)(0.5f + (float)(best_target_simulation_time) / g_pGlobalVars->intervalPerTick);
		}
	}
}