// Placement rules for planting (client for the hologram/prompt, server as the authority).
class PF_Placement
{
	static const float STASH_CLEARANCE = 2;	// m, no trees this close to an underground stash (bushes may sit on top)
	static const string SILENT = "silent:";	// reasons starting with this are only logged, never shown to the player

	static const float UNDERGROUND_DEPTH = 1.5;	// m below the terrain surface that counts as inside a cave / bunker

	// caves (Badlands, user 2026-10-20: not enough light to grow): the spot or the player is clearly below the
	// terrain surface (the mountain above). Vanilla's underground state (m_UndergroundPresence) is client only.
	static bool IsUnderground(Man player, vector pos)
	{
		if (pos[1] < g_Game.SurfaceY(pos[0], pos[2]) - UNDERGROUND_DEPTH)
			return true;
		if (!player)
			return false;
		vector p = player.GetPosition();
		return p[1] < g_Game.SurfaceY(p[0], p[2]) - UNDERGROUND_DEPTH;
	}

	// "" = allowed, otherwise the reason
	static string Check(Man player, vector pos, bool isTree = false)
	{
		PF_Config cfg = PF_Config.Get();

		if (IsUnderground(player, pos))
			return "Not enough light to grow here";

		if (IsInNoPlantZone(pos, cfg))
			return "No planting in this area";

		// strict mode: only inside your own base (not silent, it only tells the player about their own flags)
		if (cfg.RequireOwnFlag && !PF_Territory.IsOwnBase(player, pos))
			return "Needs your own territory flag nearby";

		if (IsNearRoad(pos, cfg))
			return "Too close to a road";

		// both silent: a hint would let players probe for hidden flags/stashes with a seed hologram
		if (PF_Territory.IsForeignBase(player, pos))
			return SILENT + "foreign base";

		// a trunk on top of a buried stash would block digging it out
		if (isTree && IsNearStash(pos))
			return SILENT + "stash";

		string blocker = FindBlocker(player, pos, cfg.BuildingClearance);
		if (blocker != "")
			return "Too close to " + blocker;

		return "";
	}

	// admin zones (config NoPlantZones: traders, safe zones, events); horizontal distance, Y is ignored
	static bool IsInNoPlantZone(vector pos, PF_Config cfg)
	{
		if (!cfg.NoPlantZones)
			return false;
		foreach (PF_NoPlantZone zone : cfg.NoPlantZones)
		{
			float dx = pos[0] - zone.X;
			float dz = pos[2] - zone.Z;
			if (dx * dx + dz * dz <= zone.Radius * zone.Radius)
				return true;
		}
		return false;
	}

	static bool IsNearStash(vector pos)
	{
		array<Object> objs = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(pos, STASH_CLEARANCE, objs, proxies);
		foreach (Object obj : objs)
		{
			if (UndergroundStash.Cast(obj))
				return true;
		}
		return false;
	}

	// client-side stand-in for PF_Registry.HasTreeWithin (the registry only exists on the server)
	static bool HasTreeNear(vector pos, float radius)
	{
		array<Object> objs = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(pos, radius, objs, proxies);
		foreach (Object obj : objs)
		{
			PF_Bush_Base plant = PF_Bush_Base.Cast(obj);
			if (plant && !plant.IsProjection() && plant.IsTreePlant())
				return true;
		}
		return false;
	}

	// road: a road-network piece within RoadClearance (IsNearRoadPiece), or a painted road surface from
	// cfg.RoadSurfaces right at the spot
	static bool IsNearRoad(vector pos, PF_Config cfg)
	{
		if (cfg.RoadClearance <= 0)
			return false;

		if (IsNearRoadPiece(pos, cfg.RoadClearance))
			return true;

		if (!cfg.RoadSurfaces || cfg.RoadSurfaces.Count() == 0)
			return false;

		// painted surfaces only count right at the spot: cp_dirt/cp_gravel are also bare yard and village patches,
		// a ring around the spot blocked whole gardens (the real roads are the road pieces above)
		return IsRoadAt(pos[0], pos[2], cfg);
	}

	protected static vector s_LastLogPos;

	// debug: terrain surface at a spot, at most once per metre moved. Roads of the map's road network are not
	// detectable (all engine queries return the terrain under them, e.g. 'cp_grass'), only painted ones are.
	static void LogSurfaceAt(float x, float z)
	{
		vector pos = Vector(x, 0, z);
		if (vector.DistanceSq(pos, s_LastLogPos) < 1)
			return;
		s_LastLogPos = pos;
		string surface;
		g_Game.SurfaceGetType(x, z, surface);
		PF_Log.Debug(string.Format("surface at %1/%2: '%3'", x, z, surface));
	}

	protected static vector s_RoadCachePos;
	protected static int s_RoadCacheTime;
	protected static bool s_RoadCacheResult;

	// Roads of the map's road network are objects (dz\structures\roads\parts\*.p3d, e.g. asf3_6) without collision
	// geometry and invisible to every surface query (they read as the terrain under them, e.g. cp_grass).
	// Found with GetObjectsAtPosition; the point is tested in the piece's own space against its bounding box
	// grown by the clearance, so long pieces (25 m) only count near their actual strip. Cached: called every frame.
	static bool IsNearRoadPiece(vector pos, float clearance)
	{
		int now = g_Game.GetTime();
		if (vector.DistanceSq(pos, s_RoadCachePos) < 0.25 && now - s_RoadCacheTime < 500)
			return s_RoadCacheResult;
		s_RoadCachePos = pos;
		s_RoadCacheTime = now;
		s_RoadCacheResult = false;

		vector ground = pos;
		ground[1] = g_Game.SurfaceY(pos[0], pos[2]);
		array<Object> objs = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(ground, clearance + 25, objs, proxies);
		foreach (Object obj : objs)
		{
			string shape = obj.GetShapeName();
			shape.ToLower();
			if (shape.IndexOf("roads\\parts\\") == -1)
				continue;
			vector inPiece = obj.WorldToModel(ground);
			vector minMax[2];
			obj.ClippingInfo(minMax);
			if (inPiece[0] >= minMax[0][0] - clearance && inPiece[0] <= minMax[1][0] + clearance && inPiece[2] >= minMax[0][2] - clearance && inPiece[2] <= minMax[1][2] + clearance)
			{
				s_RoadCacheResult = true;
				PF_Log.Debug("near road piece " + obj.GetShapeName());
				break;
			}
		}
		return s_RoadCacheResult;
	}

	protected static bool IsRoadAt(float x, float z, PF_Config cfg)
	{
		string surface;
		g_Game.SurfaceGetType(x, z, surface);
		return cfg.RoadSurfaces.Find(surface) != -1;
	}

	// solid things a plant must keep clear of: buildings, base objects, tents, vehicles, characters.
	// Wild flora (trees, bushes, rocks) and other planted flora don't count.
	static string FindBlocker(Man player, vector pos, float clearance)
	{
		if (clearance <= 0)
			return "";

		vector center = pos + Vector(0, 1.2, 0);
		vector size = Vector(clearance * 2, 2.4, clearance * 2);
		array<Object> excluded = new array<Object>();
		excluded.Insert(player);
		array<Object> hits = new array<Object>();
		g_Game.IsBoxCollidingGeometry(center, vector.Zero, size, ObjIntersectFire, ObjIntersectGeom, excluded, hits);

		foreach (Object obj : hits)
		{
			if (IsBlocker(obj))
				return BlockerName(obj);
		}
		return "";
	}

	// player-facing name for the placement hint
	protected static string BlockerName(Object obj)
	{
		if (obj.IsMan())
			return "a player";
		if (obj.IsBuilding())
			return "a building";
		string name = obj.GetDisplayName();
		if (name == "")
			return "a building";
		return name;
	}

	protected static bool IsBlocker(Object obj)
	{
		if (!obj || obj.IsTree() || obj.IsBush() || obj.IsRock())
			return false;
		if (PF_Bush_Base.Cast(obj))
			return false;

		return obj.IsBuilding() || obj.IsTransport() || obj.IsMan() || obj.IsInherited(BaseBuildingBase) || obj.IsInherited(TentBase) || obj.IsInherited(DeployableContainer_Base);
	}
}
