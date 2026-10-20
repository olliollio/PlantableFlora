// Foreign-base protection (Claude.md §21): no planting near a territory flag the player is not a member of.
// Vanilla flags have no owner, so membership is tracked here: building any part of the flag pole, raising the
// flag, or building any base part within FlagPlantRadius makes a player a member (covers clans without lists).
// Stored in $profile:PlantableFlora/territory.json, not in the vanilla save. Flags never move, so a flag is
// identified by its rounded position.

class PF_FlagEntry
{
	string Key;
	ref array<string> Members = new array<string>();
}

class PF_TerritoryData
{
	ref array<ref PF_FlagEntry> Flags = new array<ref PF_FlagEntry>();
}

class PF_Territory
{
	static const string PATH = "$profile:PlantableFlora/territory.json";
	protected static ref PF_TerritoryData s_Data;

	static string KeyOf(Object flag)
	{
		vector p = flag.GetPosition();
		return Math.Round(p[0]).ToString() + "_" + Math.Round(p[2]).ToString();
	}

	protected static PF_TerritoryData Data()
	{
		if (!s_Data)
		{
			s_Data = new PF_TerritoryData();
			string error;
			if (FileExist(PATH) && !JsonFileLoader<PF_TerritoryData>.LoadFile(PATH, s_Data, error))
			{
				ErrorEx("[PlantableFlora] territory.json could not be read: " + error);
				s_Data = new PF_TerritoryData();
			}
		}
		return s_Data;
	}

	protected static PF_FlagEntry Find(string key)
	{
		foreach (PF_FlagEntry entry : Data().Flags)
		{
			if (entry.Key == key)
				return entry;
		}
		return null;
	}

	// server
	static bool IsMember(string key, string playerId)
	{
		PF_FlagEntry entry = Find(key);
		return entry && entry.Members.Find(playerId) != -1;
	}

	// server: player built a part of this flag pole, raised its flag or built near it
	static void AddMember(Object flag, Man player)
	{
		if (!g_Game.IsServer() || !flag || !player || !player.GetIdentity())
			return;
		string id = player.GetIdentity().GetId();
		string key = KeyOf(flag);
		PF_FlagEntry entry = Find(key);
		if (!entry)
		{
			entry = new PF_FlagEntry();
			entry.Key = key;
			Data().Flags.Insert(entry);
		}
		if (entry.Members.Find(id) != -1)
			return;
		entry.Members.Insert(id);
		PF_Log.Debug("territory: " + id + " joined flag " + key);
		string error;
		if (!JsonFileLoader<PF_TerritoryData>.SaveFile(PATH, Data(), error))
			ErrorEx("[PlantableFlora] territory.json could not be written: " + error);
		SendTo(player.GetIdentity());
	}

	// server: a base part was built at pos -> member of every flag whose radius covers it
	static void AddMemberNear(vector pos, Man player)
	{
		array<TerritoryFlag> flags = FlagsAround(pos);
		foreach (TerritoryFlag flag : flags)
			AddMember(flag, player);
	}

	// server -> client: the flags this player is a member of (for the placement prompt)
	static void SendTo(PlayerIdentity identity)
	{
		if (!identity)
			return;
		string id = identity.GetId();
		array<string> keys = new array<string>();
		foreach (PF_FlagEntry entry : Data().Flags)
		{
			if (entry.Members.Find(id) != -1)
				keys.Insert(entry.Key);
		}
		g_Game.RPCSingleParam(null, PF_RPC.SYNC_FLAGS, new Param1<ref array<string>>(keys), true, identity);
	}

	static array<TerritoryFlag> FlagsAround(vector pos)
	{
		array<TerritoryFlag> flags = new array<TerritoryFlag>();
		array<Object> objs = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(pos, PF_Config.Get().FlagPlantRadius, objs, proxies);
		foreach (Object obj : objs)
		{
			TerritoryFlag flag = TerritoryFlag.Cast(obj);
			if (flag)
				flags.Insert(flag);
		}
		return flags;
	}

	// server: every flag pole in the world (registered in TerritoryFlag.EEInit). Vanilla's
	// GetActiveRefresherLocations() is empty after a restart until a flag changes state, so it can't be used.
	protected static ref array<TerritoryFlag> s_Flags = new array<TerritoryFlag>();

	static void RegisterFlag(TerritoryFlag flag)
	{
		if (s_Flags.Find(flag) == -1)
			s_Flags.Insert(flag);
	}

	static void UnregisterFlag(TerritoryFlag flag)
	{
		s_Flags.RemoveItem(flag);
	}

	static array<TerritoryFlag> AllFlags()
	{
		return s_Flags;
	}

	// server: inside the radius of a flag that currently refreshes lifetimes (raised, not run out)
	static bool IsInActiveBase(vector pos)
	{
		float radius = PF_Config.Get().FlagPlantRadius;
		float radiusSq = radius * radius;
		foreach (TerritoryFlag flag : s_Flags)
		{
			if (flag && flag.m_RefresherActive && vector.DistanceSq(flag.GetPosition(), pos) <= radiusSq)
				return true;
		}
		return false;
	}

	// server: is pos inside the radius of a flag this player id is a member of? (per-player base limits, PF_Caps;
	// works for offline owners too)
	static bool IsOwnerBase(string ownerId, vector pos)
	{
		if (ownerId == "")
			return false;
		float radius = PF_Config.Get().FlagPlantRadius;
		float radiusSq = radius * radius;
		foreach (TerritoryFlag flag : s_Flags)
		{
			if (flag && vector.DistanceSq(flag.GetPosition(), pos) <= radiusSq && IsMember(KeyOf(flag), ownerId))
				return true;
		}
		return false;
	}

	// server + client: is pos inside the radius of a flag this player is not a member of?
	static bool IsForeignBase(Man player, vector pos)
	{
		if (!PF_Config.Get().ProtectForeignBases)
			return false;
		array<TerritoryFlag> flags = FlagsAround(pos);
		foreach (TerritoryFlag flag : flags)
		{
			if (!PlayerIsMember(player, flag))
				return true;
		}
		return false;
	}

	// server + client: is pos inside the radius of a flag this player is a member of? (strict mode RequireOwnFlag)
	static bool IsOwnBase(Man player, vector pos)
	{
		array<TerritoryFlag> flags = FlagsAround(pos);
		foreach (TerritoryFlag flag : flags)
		{
			if (PlayerIsMember(player, flag))
				return true;
		}
		return false;
	}

	// server: membership from territory.json; client: the flag keys the server synced (SYNC_FLAGS)
	protected static bool PlayerIsMember(Man player, TerritoryFlag flag)
	{
		string key = KeyOf(flag);
		if (!g_Game.IsServer())
			return PF_ClientState.MyFlags.Find(key) != -1;
		if (!player || !player.GetIdentity())
			return false;
		return IsMember(key, player.GetIdentity().GetId());
	}
}

modded class BaseBuildingBase
{
	override void OnPartBuiltServer(notnull Man player, string part_name, int action_id)
	{
		super.OnPartBuiltServer(player, part_name, action_id);
		if (TerritoryFlag.Cast(this))
			PF_Territory.AddMember(this, player);
		else
			PF_Territory.AddMemberNear(GetPosition(), player);
	}
}

modded class TerritoryFlag
{
	override void EEInit()
	{
		super.EEInit();
		if (g_Game.IsServer())
			PF_Territory.RegisterFlag(this);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (g_Game.IsServer())
			PF_Territory.UnregisterFlag(this);
	}

	override void AnimateFlagEx(float delta, PlayerBase player = null)
	{
		super.AnimateFlagEx(delta, player);
		if (player && g_Game.IsServer())
			PF_Territory.AddMember(this, player);
	}
}
