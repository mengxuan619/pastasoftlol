#include "..\AntiAim\AntiAim.h"
#include "../../Menu/Menu.h"
#include "../../Menu/TGFCfg.h"
#include "../../SDK/Vector.h"
#include "../../SDK/ISurface.h"
#include "../../Utils/Color.h"
#include "../../Utils/GlobalVars.h"
#include "../../Menu/config.h"
#include "Indicators.h"



void Indicators::test()
{
	static int iWidth, iHeight;
	g_pEngine->GetScreenSize(iWidth, iHeight);

	if (c_config::get().indicators)
	{
		if (Globals::LocalPlayer->IsAlive())
		{
			float desyncAmt = g_AntiAim.MaxDelta(Globals::LocalPlayer);
			float diffrence = (Globals::RealAngle.y - Globals::LocalPlayer->GetLowerBodyYaw());
			float Velocity = Globals::LocalPlayer->GetVelocity().Length2D();
			int offset = 40;
			Color fake = desyncAmt <= 29 ? Color(255, 0, 0) : (desyncAmt >= 55 ? Color(132, 195, 16) : Color(255 - (desyncAmt * 2.55), desyncAmt * 2.55, 0));
			std::string choke;
			auto NetChannel = g_pEngine->GetNetChannel();

			if (!NetChannel)
				return;

			choke += "choke: " + std::to_string(NetChannel->m_nChokedPackets);
			g_pSurface->DrawT(20, (iHeight - offset - 90), Color(255, 255, 255), Globals::Indicators, false, choke.c_str());


			if (diffrence > 35 && Velocity < 0.1f) // we could make multi combo box for this // no -t4zzuu
				g_pSurface->DrawT(20, (iHeight - offset - 60), Color(132, 195, 16), Globals::Indicators, false, "LBY");
			else
				g_pSurface->DrawT(20, (iHeight - offset - 60), Color(255, 0, 0), Globals::Indicators, false, "LBY");

			if (!(desyncAmt < 29) && g_Menu.Config.DesyncAngle)
				g_pSurface->DrawT(20, (iHeight - offset - 30), fake, Globals::Indicators, false, "FAKE");
		}
	}
}