// Frozen ground on winter maps (docs/ideas.md "Thaw frozen ground with a fire", user 2026-10-23): planting holes and
// seeds on frozen ground only where a fire has thawed it. A fireplace / fire barrel burning hot (the vanilla melt
// patch under it mostly grown: temperature >= HOT_SHARE of its max) on frozen ground for ThawBurnMinutes thaws ThawRadius
// around it; the spot stays thawed ThawMinutes after the fire is out or gone. Spots live in memory only (a restart
// refreezes the ground; dug holes keep their own state). The server syncs the spot positions to every client
// (PF_RPC.SYNC_THAW), the dig/plant checks run on both sides.
//
// Which surfaces are frozen: config FrozenSurfaces, else the map's list (CfgPlantableFlora Frozen <world> surfaces[]),
// else every surface with the vanilla snow melt patch (CfgSurfaces underObjectDecal), e.g. Winter Chernarus.
class PF_Frozen
{
	static const string MELT_DECAL = "StaticObj_fireplace_spot_snow";
	static const int TICK_MS = 15000;
	static const float HOT_SHARE = 0.4;	// share of the fire's max temperature that counts as hot (500 of 1000 seen with a normal fire: vanilla melt patch ~83 % size)

	protected static ref array<string> s_List;			// explicit list (config or map), null = melt patch rule
	protected static bool s_ListRead;
	protected static ref map<string, bool> s_Cache = new map<string, bool>();	// surface -> frozen

	// thawed spots on the server, with expiry (clients: PF_ClientState.ThawSpots, synced)
	protected static ref array<vector> s_Spots = new array<vector>();
	protected static ref array<int> s_Expiry = new array<int>();	// server, g_Game.GetTime() ms
	protected static ref array<FireplaceBase> s_Fires = new array<FireplaceBase>();
	protected static bool s_Ticking;

	static bool IsFrozenSurface(string surface)
	{
		if (!PF_Config.Get().FrozenGround || surface == "")
			return false;
		bool frozen;
		if (s_Cache.Find(surface, frozen))
			return frozen;
		ReadList();
		if (s_List)
			frozen = s_List.Find(surface) != -1;
		else
		{
			string decal = "";
			g_Game.ConfigGetText("CfgSurfaces " + surface + " underObjectDecal", decal);
			frozen = decal == MELT_DECAL;
		}
		s_Cache.Insert(surface, frozen);
		return frozen;
	}

	protected static void ReadList()
	{
		if (s_ListRead)
			return;
		s_ListRead = true;
		PF_Config cfg = PF_Config.Get();
		if (cfg.FrozenSurfaces && cfg.FrozenSurfaces.Count() > 0)
		{
			s_List = new array<string>();
			s_List.Copy(cfg.FrozenSurfaces);
			return;
		}
		string path = "CfgPlantableFlora Frozen " + PF_World.Name() + " surfaces";
		if (g_Game.ConfigIsExisting(path))
		{
			s_List = new array<string>();
			g_Game.ConfigGetTextArray(path, s_List);
		}
	}

	static bool IsThawed(vector pos)
	{
		float r2 = PF_Config.Get().ThawRadius * PF_Config.Get().ThawRadius;
		array<vector> spots = s_Spots;
		if (!g_Game.IsServer())
			spots = PF_ClientState.ThawSpots;
		foreach (vector spot : spots)
		{
			float dx = spot[0] - pos[0];
			float dz = spot[2] - pos[2];
			if (dx * dx + dz * dz <= r2)
				return true;
		}
		return false;
	}

	// terrain at pos is frozen and no fire has thawed it
	static bool IsFrozenUnthawed(vector pos)
	{
		string surface = "";
		g_Game.SurfaceGetType(pos[0], pos[2], surface);
		return IsFrozenSurface(surface) && !IsThawed(pos);
	}

	// "" = the ground takes a planting hole / seed, else the player-facing reason.
	// Frozen ground: only thawed, then also when not fertile (snow surfaces aren't); other ground: must be fertile
	static string GroundBlock(vector pos, string surface)
	{
		if (IsFrozenSurface(surface))
		{
			if (IsThawed(pos))
				return "";
			return "Frozen ground: make a fire here first";
		}
		if (!g_Game.IsSurfaceFertile(surface))
			return "Ground not fertile";
		return "";
	}

	// ---- server ----

	static void RegisterFire(FireplaceBase fire)
	{
		if (!g_Game.IsServer() || !PF_Config.Get().FrozenGround || s_Fires.Find(fire) != -1)
			return;
		s_Fires.Insert(fire);
		PF_Log.Debug("thaw: fire registered " + fire.GetType() + " at " + fire.GetPosition().ToString());
		if (!s_Ticking)
		{
			s_Ticking = true;
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Tick, TICK_MS, true);
		}
	}

	static void UnregisterFire(FireplaceBase fire)
	{
		s_Fires.RemoveItem(fire);
	}

	static void Tick()
	{
		PF_Config cfg = PF_Config.Get();
		int now = g_Game.GetTime();
		bool changed;
		for (int i = s_Fires.Count() - 1; i >= 0; i--)
		{
			FireplaceBase fire = s_Fires[i];
			if (!fire)
			{
				s_Fires.Remove(i);
				continue;
			}
			if (!IsHotOnFrozenGround(fire))
			{
				fire.m_PF_HotSeconds = 0;
				continue;
			}
			fire.m_PF_HotSeconds += TICK_MS / 1000;
			if (fire.m_PF_HotSeconds >= cfg.ThawBurnMinutes * 60)
				changed = Thaw(fire.GetPosition(), now + cfg.ThawMinutes * 60000) || changed;
		}
		for (int j = s_Spots.Count() - 1; j >= 0; j--)
		{
			if (s_Expiry[j] <= now)
			{
				PF_Log.Debug("thawed ground froze again at " + s_Spots[j].ToString());
				s_Spots.Remove(j);
				s_Expiry.Remove(j);
				changed = true;
			}
		}
		if (changed)
			SendTo(null);
	}

	protected static bool IsHotOnFrozenGround(FireplaceBase fire)
	{
		vector pos = fire.GetPosition();
		string surface = "";
		g_Game.SurfaceGetType(pos[0], pos[2], surface);
		if (PF_Config.Get().DebugLogs)
		{
			string info = "thaw: " + fire.GetType() + " at " + pos.ToString();
			info += " burning " + fire.IsBurning();
			info += " temp " + fire.GetTemperature() + " / max " + fire.GetTemperatureMax();
			info += " surface " + surface + " frozen " + IsFrozenSurface(surface);
			info += " hot " + fire.m_PF_HotSeconds + " s";
			PF_Log.Debug(info);
		}
		if (!fire.IsBurning() || fire.GetHierarchyParent())
			return false;
		if (fire.GetTemperature() < fire.GetTemperatureMax() * HOT_SHARE)
			return false;
		return IsFrozenSurface(surface);
	}

	// new spot or a longer life for the one at this fire; true = clients need the new list
	protected static bool Thaw(vector pos, int expiry)
	{
		for (int i = 0; i < s_Spots.Count(); i++)
		{
			if (vector.DistanceSq(s_Spots[i], pos) < 1)
			{
				s_Expiry[i] = expiry;
				return false;
			}
		}
		s_Spots.Insert(pos);
		s_Expiry.Insert(expiry);
		PF_Log.Debug("ground thawed at " + pos.ToString() + " (radius " + PF_Config.Get().ThawRadius + " m)");
		return true;
	}

	// identity null = every client
	static void SendTo(PlayerIdentity identity)
	{
		g_Game.RPCSingleParam(null, PF_RPC.SYNC_THAW, new Param1<ref array<vector>>(s_Spots), true, identity);
	}
}

// fires register themselves on the server, so the thaw tick only looks at fireplaces (no world search)
modded class FireplaceBase
{
	int m_PF_HotSeconds;	// how long it has burned hot on frozen ground (PF_Frozen)

	override void EEInit()
	{
		super.EEInit();
		PF_Frozen.RegisterFire(this);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		PF_Frozen.UnregisterFire(this);
	}
}
