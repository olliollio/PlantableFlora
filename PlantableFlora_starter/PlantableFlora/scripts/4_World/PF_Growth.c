// One shared timer for all planted flora (no per-plant timers).
class PF_Growth
{
	protected static ref array<PF_Bush_Base> s_Plants = new array<PF_Bush_Base>();
	protected static bool s_Running;

	static void Register(PF_Bush_Base plant)
	{
		if (s_Plants.Find(plant) == -1)
			s_Plants.Insert(plant);

		if (!s_Running)
		{
			s_Running = true;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Tick, PF_Config.Get().GrowthTickSeconds * 1000, true);
		}
	}

	static void Unregister(PF_Bush_Base plant)
	{
		s_Plants.RemoveItem(plant);
	}

	static int Count()
	{
		return s_Plants.Count();
	}

	static void Tick()
	{
		// DEV: plants keep their stage (height dial-in with TreeLineup); they catch up once it is off again
		if (PF_Config.Get().FreezeGrowth)
			return;
		int ticks = TickCount(0);
		int now = PF_Time.Now();
		array<PF_Bush_Base> due = new array<PF_Bush_Base>();
		foreach (PF_Bush_Base plant : s_Plants)
		{
			if (plant && plant.IsGrown(now))
				due.Insert(plant);
		}
		foreach (PF_Bush_Base grownPlant : due)
			grownPlant.Grow();
		if (PF_Config.Get().PerfLog)
			Print(string.Format("[PlantableFlora] perf: growth tick %1 plants, %2 grew, %3 ms", s_Plants.Count(), due.Count(), PF_Perf.TicksToMs(TickCount(ticks))));
	}
}
