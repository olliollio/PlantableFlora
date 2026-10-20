// DEV ONLY (remove before release, copy kept in tools/dev): survey of a map's trees, bushes and terrain surfaces for
// multi-map support (docs/maps.md step 1). Runs only when $profile:PlantableFlora/survey_request.txt exists (deleted
// when done), server side, no client needed. Walks the map in square cells (each object counted once) and writes
// $profile:PlantableFlora/survey_<world>.txt:
//   model <shape> <tree|bush|other> <count> <model height> <median origin height above terrain> <samples> <x> <z>
//   surface <name> <samples>   (terrain surface every SURFACE_STEP m)
class PF_MapSurvey
{
	static const string REQUEST = "$profile:PlantableFlora/survey_request.txt";
	static const float STEP = 500;			// m, cell size
	static const float SURFACE_STEP = 50;	// m between surface samples
	static const int MAX_SAMPLES = 15;		// height samples per model

	protected static ref map<string, int> s_Count;
	protected static ref map<string, string> s_Kind;
	protected static ref map<string, float> s_Height;
	protected static ref map<string, ref array<float>> s_Above;
	protected static ref map<string, vector> s_FirstPos;
	protected static ref map<string, int> s_Surfaces;
	protected static float s_X;
	protected static float s_Z;
	protected static int s_Cells;
	protected static int s_StartTime;

	static void StartIfRequested()
	{
		if (!FileExist(REQUEST))
			return;
		s_Count = new map<string, int>();
		s_Kind = new map<string, string>();
		s_Height = new map<string, float>();
		s_Above = new map<string, ref array<float>>();
		s_FirstPos = new map<string, vector>();
		s_Surfaces = new map<string, int>();
		s_X = 0;
		s_Z = 0;
		s_StartTime = g_Game.GetTime();
		Print("[PlantableFlora] map survey start, world size " + g_Game.GetWorld().GetWorldSize());
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Next, 5000, false);
	}

	static void Next()
	{
		float worldSize = g_Game.GetWorld().GetWorldSize();
		if (s_Z >= worldSize)
		{
			Finish();
			return;
		}

		float x0 = s_X;
		float z0 = s_Z;
		vector center = Vector(x0 + STEP * 0.5, 0, z0 + STEP * 0.5);
		center[1] = g_Game.SurfaceY(center[0], center[2]);
		array<Object> objs = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(center, STEP * 0.7072 + 1, objs, proxies);
		foreach (Object obj : objs)
		{
			vector pos = obj.GetPosition();
			if (pos[0] < x0 || pos[0] >= x0 + STEP || pos[2] < z0 || pos[2] >= z0 + STEP)
				continue;	// belongs to a neighbour cell
			string shape = obj.GetShapeName();
			shape.ToLower();
			string kind = "other";
			if (obj.IsTree())
				kind = "tree";
			else if (obj.IsBush())
				kind = "bush";
			else if (shape.IndexOf("plants") == -1)
				continue;	// buildings, rocks, roads, items
			if (PF_Bush_Base.Cast(obj))
				continue;	// planted flora

			if (!s_Count.Contains(shape))
			{
				s_Count.Insert(shape, 0);
				s_Kind.Insert(shape, kind);
				vector minMax[2];
				obj.ClippingInfo(minMax);
				s_Height.Insert(shape, minMax[1][1] - minMax[0][1]);
				s_Above.Insert(shape, new array<float>());
				s_FirstPos.Insert(shape, pos);
			}
			s_Count.Set(shape, s_Count.Get(shape) + 1);
			array<float> above = s_Above.Get(shape);
			if (above.Count() < MAX_SAMPLES)
				above.Insert(pos[1] - g_Game.SurfaceY(pos[0], pos[2]));
		}

		for (float sx = x0 + SURFACE_STEP * 0.5; sx < x0 + STEP; sx += SURFACE_STEP)
		{
			for (float sz = z0 + SURFACE_STEP * 0.5; sz < z0 + STEP; sz += SURFACE_STEP)
			{
				string surface;
				g_Game.SurfaceGetType(sx, sz, surface);
				surface.ToLower();
				if (s_Surfaces.Contains(surface))
					s_Surfaces.Set(surface, s_Surfaces.Get(surface) + 1);
				else
					s_Surfaces.Insert(surface, 1);
			}
		}

		s_Cells++;
		if (s_Cells % 50 == 0)
			Print("[PlantableFlora] map survey: " + s_Cells + " cells, " + s_Count.Count() + " models");

		s_X = s_X + STEP;
		if (s_X >= worldSize)
		{
			s_X = 0;
			s_Z = s_Z + STEP;
		}
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Next, 100, false);
	}

	static void Finish()
	{
		string world;
		g_Game.GetWorldName(world);
		world.ToLower();
		string path = "$profile:PlantableFlora/survey_" + world + ".txt";
		FileHandle file = OpenFile(path, FileMode.WRITE);
		if (file == 0)
		{
			ErrorEx("[PlantableFlora] map survey: cannot write " + path);
			return;
		}
		FPrintln(file, "# PF_MapSurvey world=" + world + " size=" + g_Game.GetWorld().GetWorldSize() + " cells=" + s_Cells + " seconds=" + ((g_Game.GetTime() - s_StartTime) / 1000));
		FPrintln(file, "# model <shape> <kind> <count> <height> <above median> <samples> <x> <z>");
		foreach (string shape, int count : s_Count)
		{
			array<float> above = s_Above.Get(shape);
			above.Sort();
			float median = above[above.Count() / 2];
			vector first = s_FirstPos.Get(shape);
			FPrintln(file, string.Format("model %1 %2 %3 %4 %5 %6 %7 %8", shape, s_Kind.Get(shape), count, s_Height.Get(shape), median, above.Count(), Math.Round(first[0]), Math.Round(first[2])));
		}
		foreach (string surface, int n : s_Surfaces)
		{
			FPrintln(file, "surface " + surface + " " + n);
		}
		CloseFile(file);
		DeleteFile(REQUEST);
		Print("[PlantableFlora] map survey done: " + s_Count.Count() + " models, written to " + path);
	}
}
