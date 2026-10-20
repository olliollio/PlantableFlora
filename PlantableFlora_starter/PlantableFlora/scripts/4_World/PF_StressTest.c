// DEV ONLY (remove or keep disabled for release): load test helpers.
// Config PerfLog: server FPS every 10 s. Config StressTestCount: spawn / remove test plants.

class PF_Perf
{
	protected static int s_Frames;
	protected static float s_Elapsed;
	protected static float s_WorstSlice;
	protected static int s_TickStart;
	protected static int s_MsStart;
	protected static float s_TicksPerMs = 10000;	// calibrated every window (CPU performance counter)

	static void OnFrame(float timeslice)
	{
		if (!PF_Config.Get().PerfLog)
			return;
		if (s_MsStart == 0)
		{
			s_MsStart = g_Game.GetTime();
			s_TickStart = TickCount(0);
			return;
		}
		s_Frames++;
		s_Elapsed += timeslice;
		if (timeslice > s_WorstSlice)
			s_WorstSlice = timeslice;
		if (s_Elapsed < 10)
			return;

		int ms = g_Game.GetTime() - s_MsStart;
		if (ms > 0)
			s_TicksPerMs = TickCount(s_TickStart) / ms;
		Print(string.Format("[PlantableFlora] perf: server fps %1 (worst frame %2 ms), plants %3, growing %4",
			Math.Round(s_Frames / s_Elapsed), Math.Round(s_WorstSlice * 1000), PF_Registry.CountAll(), PF_Growth.Count()));
		s_Frames = 0;
		s_Elapsed = 0;
		s_WorstSlice = 0;
		s_MsStart = g_Game.GetTime();
		s_TickStart = TickCount(0);
	}

	static float TicksToMs(int ticks)
	{
		return ticks / s_TicksPerMs;
	}
}

class PF_StressTest
{
	static const string OWNER = "stress";
	static const float SPACING = 4;
	static const int BATCH = 50;	// plants per 100 ms
	// a mix of grown trees, bushes and growing plants (the young ones go through the growth tick)
	static string ClassFor(int i)
	{
		switch (i % 10)
		{
			case 0: return "PF_Tree_Spruce";
			case 1: return "PF_Tree_Beech";
			case 2: return "PF_Tree_Oak";
			case 3: return "PF_Tree_PineForest";
			case 4: return "PF_Tree_Birch";
			case 5: return "PF_Bush_Hazel";
			case 6: return "PF_Bush_Rose";
			case 7: return "PF_Bush_Hawthorn";
			case 8: return "PF_Bush_Elder";
		}
		return "PF_Tree_Spruce_Young";
	}

	protected static int s_Index;
	protected static int s_Target;
	protected static int s_Ticks;

	static void Start()
	{
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Run, 60000, false);
	}

	static void Run()
	{
		PF_Config cfg = PF_Config.Get();
		array<PF_Bush_Base> existing = PF_Registry.GetByOwner(OWNER);
		if (cfg.StressTestCount <= 0)
		{
			if (existing.Count() == 0)
				return;
			int t = TickCount(0);
			foreach (PF_Bush_Base plant : existing)
				g_Game.ObjectDelete(plant);
			Print(string.Format("[PlantableFlora] stress: removed %1 plants in %2 ms", existing.Count(), PF_Perf.TicksToMs(TickCount(t))));
			return;
		}

		s_Index = existing.Count();
		s_Target = cfg.StressTestCount;
		s_Ticks = 0;
		Print(string.Format("[PlantableFlora] stress: %1 exist, spawning up to %2 around %3 / %4", s_Index, s_Target, cfg.StressTestX, cfg.StressTestZ));
		SpawnBatch();
	}

	static void SpawnBatch()
	{
		PF_Config cfg = PF_Config.Get();
		int side = 1;
		while (side * side < s_Target)
			side++;
		float half = (side - 1) * SPACING * 0.5;
		int t = TickCount(0);
		int end = s_Index + BATCH;
		if (end > s_Target)
			end = s_Target;
		while (s_Index < end)
		{
			int row = s_Index / side;
			int col = s_Index - row * side;
			float x = cfg.StressTestX - half + col * SPACING;
			float z = cfg.StressTestZ - half + row * SPACING;
			vector pos = Vector(x, g_Game.SurfaceY(x, z), z);
			PF_Bush_Base plant = PF_Bush_Base.Cast(g_Game.CreateObjectEx(ClassFor(s_Index), pos, ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS));
			if (plant)
			{
				plant.SetOrientation(Vector(Math.RandomFloat(0, 360), 0, 0));
				plant.SetOwnerId(OWNER);
			}
			s_Index++;
		}
		s_Ticks += TickCount(t);
		if (s_Index < s_Target)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SpawnBatch, 100, false);
			return;
		}
		Print(string.Format("[PlantableFlora] stress: spawned up to %1 plants, spawn work %2 ms total", s_Target, PF_Perf.TicksToMs(s_Ticks)));
	}
}
