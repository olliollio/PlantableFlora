// Admin overview: $profile:PlantableFlora/stats.txt (rewritten every StatsMinutes) + one summary line in the
// server script log. Counts per kind and per owner (PlayerIdentity.GetId, the BI id admin tools show) with positions.
class PF_StatsCount
{
	int Trees;
	int Bushes;
	int Stumps;
	int Growing;
	ref array<string> Lines = new array<string>();

	int Total()
	{
		return Trees + Bushes + Stumps;
	}
}

class PF_Stats
{
	static const string PATH = "$profile:PlantableFlora/stats.txt";

	static void Start()
	{
		int minutes = PF_Config.Get().StatsMinutes;
		if (minutes <= 0)
			return;
		// first write once the save is loaded
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Write, 60000, false);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Write, minutes * 60000, true);
	}

	static void Add(PF_StatsCount count, PF_Bush_Base plant)
	{
		string type = plant.GetType();
		if (type.Contains("_Stump"))
			count.Stumps++;
		else if (plant.IsTreePlant())
			count.Trees++;
		else
			count.Bushes++;
		if (plant.GetNextStageConfig() != "" && !type.Contains("_Cut"))
			count.Growing++;
	}

	static string Summary(PF_StatsCount c)
	{
		return string.Format("%1 total: %2 trees, %3 bushes, %4 stumps (%5 still growing)", c.Total(), c.Trees, c.Bushes, c.Stumps, c.Growing);
	}

	static void Write()
	{
		PF_StatsCount all = new PF_StatsCount();
		map<string, ref PF_StatsCount> byOwner = new map<string, ref PF_StatsCount>();

		array<PF_Bush_Base> plants = PF_Registry.GetAll();
		foreach (PF_Bush_Base plant : plants)
		{
			string owner = plant.GetOwnerId();
			if (owner == "")
				owner = "(none: spawned by admin/script)";
			PF_StatsCount own = byOwner.Get(owner);
			if (!own)
			{
				own = new PF_StatsCount();
				byOwner.Insert(owner, own);
			}
			Add(all, plant);
			Add(own, plant);
			vector pos = plant.GetPosition();
			own.Lines.Insert(string.Format("    %1  X %2  Z %3", plant.GetType(), Math.Round(pos[0]), Math.Round(pos[2])));
		}

		int mounds = PF_PlantingMound.s_All.Count();
		PF_Config cfg = PF_Config.Get();
		Print("[PlantableFlora] stats: " + Summary(all) + ", " + mounds + " planting mounds, " + byOwner.Count() + " owners (details in " + PATH + ")");

		FileHandle file = OpenFile(PATH, FileMode.WRITE);
		if (file == 0)
			return;
		int year, month, day, hour, minute, second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);
		FPrintln(file, string.Format("PlantableFlora stats, %1-%2-%3 %4:%5 UTC (rewritten every %6 min)", year, month.ToStringLen(2), day.ToStringLen(2), hour.ToStringLen(2), minute.ToStringLen(2), cfg.StatsMinutes));
		FPrintln(file, "Server: " + Summary(all) + ", limit " + cfg.MaxPlantsPerServer);
		FPrintln(file, "Planting mounds (unused): " + mounds);
		FPrintln(file, "");
		foreach (string ownerId, PF_StatsCount c : byOwner)
		{
			FPrintln(file, "Owner " + ownerId + ": " + Summary(c));
			foreach (string line : c.Lines)
				FPrintln(file, line);
			FPrintln(file, "");
		}
		CloseFile(file);
	}
}
