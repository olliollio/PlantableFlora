// Server: every planted flora object (any stage), for the plant caps.
class PF_Registry
{
	protected static ref array<PF_Bush_Base> s_All = new array<PF_Bush_Base>();

	protected static bool s_LifetimeLogRunning;

	static void Register(PF_Bush_Base plant)
	{
		if (s_All.Find(plant) == -1)
			s_All.Insert(plant);

		if (!s_LifetimeLogRunning && PF_Config.Get().DebugLogs)
		{
			s_LifetimeLogRunning = true;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(LogLifetimes, 15000, true);
		}
	}

	// debug (lifetime test): CE lifetime of every plant + whether an active flag refresher covers it
	static void LogLifetimes()
	{
		array<vector> refreshers = g_Game.GetMission().GetActiveRefresherLocations();
		PF_Log.Debug("lifetimes: " + s_All.Count() + " plants, " + refreshers.Count() + " active flags");
		foreach (PF_Bush_Base plant : s_All)
		{
			if (!plant)
				continue;
			bool inFlag = false;
			foreach (vector flagPos : refreshers)
			{
				if (vector.Distance(flagPos, plant.GetPosition()) < GameConstants.REFRESHER_RADIUS)
					inFlag = true;
			}
			PF_Log.Debug(string.Format("lifetime %1 at %2: %3 s (max %4) inFlag=%5", plant.GetType(), plant.GetPosition().ToString(), plant.GetLifetime(), plant.GetLifetimeMax(), inFlag));
		}
	}

	static void Unregister(PF_Bush_Base plant)
	{
		s_All.RemoveItem(plant);
		NotifyOwner(plant.GetOwnerId());
	}

	// tell the owning client how many trees and bushes they have outside / inside their bases (dig/plant prompts)
	static void NotifyOwner(string ownerId)
	{
		if (ownerId == "")
			return;

		array<Man> players = new array<Man>();
		g_Game.GetPlayers(players);
		foreach (Man man : players)
		{
			PlayerIdentity identity = man.GetIdentity();
			if (identity && identity.GetId() == ownerId)
			{
				Param4<int, int, int, int> counts = new Param4<int, int, int, int>(PF_Caps.CountOwnerKind(ownerId, PF_Caps.KIND_TREE, false), PF_Caps.CountOwnerKind(ownerId, PF_Caps.KIND_BUSH, false),
					PF_Caps.CountOwnerKind(ownerId, PF_Caps.KIND_TREE, true), PF_Caps.CountOwnerKind(ownerId, PF_Caps.KIND_BUSH, true));
				g_Game.RPCSingleParam(null, PF_RPC.SYNC_COUNT, counts, true, identity);
				return;
			}
		}
	}

	static int CountAll()
	{
		return s_All.Count();
	}

	// any planted tree (young or mature) within radius of pos
	static bool HasTreeWithin(vector pos, float radius)
	{
		float radiusSq = radius * radius;
		foreach (PF_Bush_Base plant : s_All)
		{
			if (plant && plant.IsTreePlant() && vector.DistanceSq(plant.GetPosition(), pos) < radiusSq)
				return true;
		}
		return false;
	}

	// copy (deleting plants changes s_All)
	static array<PF_Bush_Base> GetAll()
	{
		array<PF_Bush_Base> result = new array<PF_Bush_Base>();
		foreach (PF_Bush_Base plant : s_All)
		{
			if (plant)
				result.Insert(plant);
		}
		return result;
	}

	static array<PF_Bush_Base> GetByOwner(string ownerId)
	{
		array<PF_Bush_Base> result = new array<PF_Bush_Base>();
		foreach (PF_Bush_Base plant : s_All)
		{
			if (plant && plant.GetOwnerId() == ownerId)
				result.Insert(plant);
		}
		return result;
	}

}
