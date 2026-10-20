// DEV tools for dialing in tree heights (Claude.md §18):
// - TreeLineup: when the first player joins, one stage (TreeLineupStage) of every tree species is planted in a grid in front of
//   them (6 per row, 12 m apart, starting 15 m ahead); the list goes to $profile:PlantableFlora/lineup.txt.
// - With DebugLogs on, config.json is re-read every 10 s: TreeLineup and FreezeGrowth apply live; when the overrides changed, every
//   plant is snapped to its new height (no restart).
class PF_DevTune
{
	static const string LINEUP_PATH = "$profile:PlantableFlora/lineup.txt";
	static const int COLUMNS = 6;
	static const float SPACING = 12;
	static const float AHEAD = 15;

	protected static bool s_LineupDone;
	protected static string s_LastOverrides;

	static void Start()
	{
		if (PF_Config.Get().DebugLogs)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ReloadOverrides, 10000, true);
	}

	static string OverridesText(PF_Config cfg)
	{
		string text;
		foreach (PF_OffsetOverride entry : cfg.GroundOffsetOverrides)
			text += entry.PlantClass + "=" + entry.Offset + ";";
		return text;
	}

	static void ReloadOverrides()
	{
		PF_Config fresh = new PF_Config();
		string error;
		if (!JsonFileLoader<PF_Config>.LoadFile(PF_Config.PATH, fresh, error) || !fresh.GroundOffsetOverrides)
			return;
		if (fresh.FreezeGrowth != PF_Config.Get().FreezeGrowth)
		{
			PF_Config.Get().FreezeGrowth = fresh.FreezeGrowth;
			Print("[PlantableFlora] dev: FreezeGrowth " + fresh.FreezeGrowth);
		}
		// TreeLineup switched on while the server runs: plant it in front of the first player (teleport to a flat spot first)
		if (fresh.TreeLineup)
		{
			array<Man> players = new array<Man>();
			g_Game.GetPlayers(players);
			if (players.Count() > 0)
			{
				PF_Config.Get().TreeLineupStage = fresh.TreeLineupStage;
				SpawnLineup(PlayerBase.Cast(players[0]));
			}
		}
		string text = OverridesText(fresh);
		if (s_LastOverrides == "")
			s_LastOverrides = OverridesText(PF_Config.Get());
		if (text == s_LastOverrides)
			return;
		s_LastOverrides = text;
		PF_Config.Get().GroundOffsetOverrides = fresh.GroundOffsetOverrides;
		array<PF_Bush_Base> plants = PF_Registry.GetAll();
		foreach (PF_Bush_Base plant : plants)
			plant.SnapToGround();
		Print("[PlantableFlora] dev: ground offset overrides reloaded (" + text + "), " + plants.Count() + " plants snapped");
	}

	static void OnPlayerConnected(PlayerBase player)
	{
		if (s_LineupDone || !PF_Config.Get().TreeLineup)
			return;
		s_LineupDone = true;
		// the character is placed a moment after connecting
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SpawnLineup, 15000, false, player);
	}

	static void SpawnLineup(PlayerBase player)
	{
		if (!player)
			return;

		// config TreeLineupStage ("Medium" or a list like "Medium,Mature") of every tree species: generated classes
		// PF_Tree_<Name>_Young / _Medium / PF_Tree_<Name>; each stage starts a new block of rows
		array<string> stages = new array<string>();
		PF_Config.Get().TreeLineupStage.Split(",", stages);
		array<string> classes = new array<string>();
		int count = g_Game.ConfigGetChildrenCount("CfgVehicles");
		foreach (string stageEntry : stages)
		{
			string stage = stageEntry.Trim();
			while (classes.Count() % COLUMNS != 0)
				classes.Insert("");	// empty cells: the next stage starts on a new row
			for (int i = 0; i < count; i++)
			{
				string name = "";
				g_Game.ConfigGetChildName("CfgVehicles", i, name);
				if (name.IndexOf("PF_Tree_") != 0 || !PF_World.IsNativeClass(name))
					continue;
				string rest = name.Substring(8, name.Length() - 8);
				bool mature = rest.IndexOf("_") == -1;	// PF_Tree_<Name> has no further "_"
				if ((stage == "Mature" && mature) || (stage != "Mature" && rest.Contains("_" + stage) && !rest.Contains("Placing")))
					classes.Insert(name);
			}
		}

		vector origin = player.GetPosition();
		vector forward = player.GetDirection();
		forward[1] = 0;
		forward.Normalize();
		vector right = Vector(forward[2], 0, -forward[0]);

		FileHandle file = OpenFile(LINEUP_PATH, FileMode.WRITE);
		if (file != 0)
			FPrintln(file, "Tree lineup: row 1 is closest to the player's start, column 1 is on the left (looking the same way as the player)");
		for (int n = 0; n < classes.Count(); n++)
		{
			if (classes[n] == "")
				continue;
			int row = n / COLUMNS;
			int col = n - row * COLUMNS;
			// floats on purpose: int * float can stay int in Enforce
			float ahead = row;
			ahead = AHEAD + ahead * SPACING;
			float side = col;
			side = (side - (COLUMNS - 1) * 0.5) * SPACING;
			vector pos = origin + forward * ahead + right * side;
			pos[1] = g_Game.SurfaceY(pos[0], pos[2]);
			Object obj = g_Game.CreateObjectEx(classes[n], pos, ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS);
			if (obj)
				obj.SetOrientation(Vector(player.GetOrientation()[0], 0, 0));
			string line = string.Format("row %1 col %2: %3  X %4  Z %5", row + 1, col + 1, classes[n], Math.Round(pos[0]), Math.Round(pos[2]));
			Print("[PlantableFlora] lineup " + line);
			if (file != 0)
				FPrintln(file, line);
		}
		if (file != 0)
			CloseFile(file);

		PF_Config.Get().TreeLineup = false;
		PF_Config.Save();
	}
}
