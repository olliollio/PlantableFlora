// Wild map plant -> seed / fruit, from config CfgPlantableFlora Wild (docs/maps.md 3.2). Other addons can add
// entries to that class (species packs, map packs). The longest matching model name fragment wins, so a forest
// spruce ("piceaabies_1f") beats the plain one ("piceaabies").
class PF_WildEntry
{
	string Source;	// "tree" = wild tree search, "bush" = cutting a wild bush
	string Match;	// lower-case fragment of the wild model name
	string Seed;	// seed class it gives
	string Fruit;	// fruit item the wild tree search gives instead of a seed ("" = none)
}

class PF_Species
{
	static const string ROOT = "CfgPlantableFlora Wild";

	protected static ref array<ref PF_WildEntry> s_Entries;
	protected static ref map<string, PF_WildEntry> s_Cache;	// "<source>:<shape>" -> best entry (null = none); the tree search condition runs every frame

	protected static void Load()
	{
		s_Entries = new array<ref PF_WildEntry>();
		s_Cache = new map<string, PF_WildEntry>();
		int count = g_Game.ConfigGetChildrenCount(ROOT);
		for (int i = 0; i < count; i++)
		{
			string name;
			g_Game.ConfigGetChildName(ROOT, i, name);
			string path = ROOT + " " + name;
			// reset every pass: Enforce keeps loop locals from the previous pass, and ConfigGetText leaves the
			// variable untouched when the entry has no such key (a missing fruit inherited "Plum" from the entry before)
			string source = "";
			string seed = "";
			string fruit = "";
			g_Game.ConfigGetText(path + " source", source);
			g_Game.ConfigGetText(path + " seed", seed);
			g_Game.ConfigGetText(path + " fruit", fruit);
			TStringArray matches = new TStringArray();
			g_Game.ConfigGetTextArray(path + " match", matches);
			foreach (string match : matches)
			{
				match.ToLower();
				PF_WildEntry entry = new PF_WildEntry();
				entry.Source = source;
				entry.Match = match;
				entry.Seed = seed;
				entry.Fruit = fruit;
				s_Entries.Insert(entry);
			}
		}
		PF_Log.Debug("species: " + s_Entries.Count() + " wild model matches from " + count + " entries");
	}

	static PF_WildEntry Find(string source, string shapeName)
	{
		if (!s_Entries)
			Load();
		shapeName.ToLower();
		string key = source + ":" + shapeName;
		PF_WildEntry best;
		if (s_Cache.Find(key, best))
			return best;

		int bestLength = 0;
		foreach (PF_WildEntry entry : s_Entries)
		{
			if (entry.Source == source && entry.Match.Length() > bestLength && shapeName.Contains(entry.Match))
			{
				best = entry;
				bestLength = entry.Match.Length();
			}
		}
		s_Cache.Insert(key, best);
		return best;
	}

	// wild tree model -> seed class ("" = none)
	static string TreeSeedFor(string shapeName)
	{
		PF_WildEntry entry = Find("tree", shapeName);
		if (entry)
			return entry.Seed;
		return "";
	}

	// wild fruit tree model -> fruit item class ("" = not a fruit tree)
	static string FruitFor(string shapeName)
	{
		PF_WildEntry entry = Find("tree", shapeName);
		if (entry)
			return entry.Fruit;
		return "";
	}

	// wild bush model -> seed class ("" = none)
	static string BushSeedFor(string shapeName)
	{
		PF_WildEntry entry = Find("bush", shapeName);
		if (entry)
			return entry.Seed;
		return "";
	}
}
