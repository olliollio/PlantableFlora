// Which species grow on the current map (docs/maps.md 3.1). A class with config pf_world (e.g. "nasdara") only
// grows on that world. A world listed in CfgPlantableFlora exclusiveWorlds only grows its own classes, so a
// Chernarus seed (admin spawn, old loot entry) won't grow on Badlands and a Badlands seed won't grow anywhere else.
class PF_World
{
	protected static string s_Name;
	protected static ref TStringArray s_Exclusive;

	static string Name()
	{
		if (s_Name == "")
		{
			g_Game.GetWorldName(s_Name);
			s_Name.ToLower();
		}
		return s_Name;
	}

	protected static bool IsExclusive(string world)
	{
		if (!s_Exclusive)
		{
			s_Exclusive = new TStringArray();
			g_Game.ConfigGetTextArray("CfgPlantableFlora exclusiveWorlds", s_Exclusive);
		}
		return s_Exclusive.Find(world) != -1;
	}

	protected static bool s_SoilRead;
	protected static string s_SoilTexture;
	protected static string s_LimedTexture;

	protected static void ReadSoil()
	{
		if (s_SoilRead)
			return;
		s_SoilRead = true;
		g_Game.ConfigGetText("CfgPlantableFlora Soil " + Name() + " texture", s_SoilTexture);
		g_Game.ConfigGetText("CfgPlantableFlora Soil " + Name() + " limedTexture", s_LimedTexture);
	}

	// texture of plain soil on planting holes / seedling heaps on this map ("" = vanilla garden soil)
	static string SoilTexture()
	{
		ReadSoil();
		return s_SoilTexture;
	}

	// same with garden lime ("" = vanilla limed soil)
	static string LimedSoilTexture()
	{
		ReadSoil();
		return s_LimedTexture;
	}

	// a seed or plant class (CfgVehicles) that is native to the current map
	static bool IsNativeClass(string className)
	{
		string classWorld = "";
		g_Game.ConfigGetText("CfgVehicles " + className + " pf_world", classWorld);
		classWorld.ToLower();
		if (classWorld != "")
			return classWorld == Name();
		return !IsExclusive(Name());
	}
}
