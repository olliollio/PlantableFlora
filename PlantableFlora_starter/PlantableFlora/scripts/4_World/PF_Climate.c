// Snowy / frozen look of planted plants (decided 2026-10-22, docs/maps.md 4.6). The ground surface doesn't tell
// (Deer Isle's snowy north has plain forest ground under winter spruces), the wild plants do: one snowy (_winter,
// Deer Isle _snow_) or frozen (_frozen) wild tree or bush within RADIUS is enough (user, 2026-10-22; a majority vote
// failed: green reeds, nettles and bushes grow under the snowy spruces). The seed then plants its
// pf_plantClassWinter / pf_plantClassFrozen class (frozen when frozen models outnumber snowy ones); a seed without
// one is "Too cold here". Map objects never move, so the answer is cached per grid cell for the whole session
// (CanBePlaced asks every frame while placing).
class PF_Climate
{
	static const string GREEN = "";
	static const string WINTER = "Winter";
	static const string FROZEN = "Frozen";
	static const float RADIUS = 30;		// m around the spot
	static const float CELL = 10;		// m, cache grid

	protected static ref map<string, string> s_Cache;

	// GREEN, WINTER or FROZEN at pos
	static string At(vector pos)
	{
		if (!s_Cache)
			s_Cache = new map<string, string>();
		string key = Math.Floor(pos[0] / CELL).ToString() + "_" + Math.Floor(pos[2] / CELL).ToString();
		string climate;
		if (s_Cache.Find(key, climate))
			return climate;

		int winter, frozen;
		string example = "";
		array<Object> objs = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(pos, RADIUS, objs, proxies);
		foreach (Object obj : objs)
		{
			if ((!obj.IsTree() && !obj.IsBush()) || PF_Bush_Base.Cast(obj))
				continue;	// planted flora doesn't count, only the map's own plants
			string shape = obj.GetShapeName();
			shape.ToLower();
			if (shape.Contains("_frozen"))
				frozen++;
			else if (shape.Contains("_winter") || shape.Contains("_snow_"))
				winter++;
			else
				continue;
			example = shape;
		}

		climate = GREEN;
		if (frozen > winter)
			climate = FROZEN;
		else if (winter > 0)
			climate = WINTER;
		s_Cache.Insert(key, climate);
		PF_Log.Debug("climate at " + pos[0] + "/" + pos[2] + ": '" + climate + "' (winter " + winter + ", frozen " + frozen + ", e.g. " + example + ")");
		return climate;
	}

	// the class a seed plants at pos: pf_plantClass, or its climate variant; "" = no variant (too cold)
	static string PlantClassFor(string seedType, vector pos)
	{
		string plantClass = "";
		string climate = At(pos);
		g_Game.ConfigGetText("CfgVehicles " + seedType + " pf_plantClass" + climate, plantClass);
		return plantClass;
	}
}
