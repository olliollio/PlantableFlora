// Admin wipe: config WipeAllPlants true -> 30 s after the server start (the save is loaded by then) every planted
// plant (all stages, stumps, cut bushes) and every planting mound is deleted, then the switch goes back to false.
class PF_Wipe
{
	static void Start()
	{
		if (PF_Config.Get().WipeAllPlants)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Run, 30000, false);
	}

	static void Run()
	{
		array<PF_Bush_Base> plants = PF_Registry.GetAll();
		foreach (PF_Bush_Base plant : plants)
			g_Game.ObjectDelete(plant);

		array<PF_PlantingMound> mounds = new array<PF_PlantingMound>();
		mounds.Copy(PF_PlantingMound.s_All);
		foreach (PF_PlantingMound mound : mounds)
		{
			if (mound)
				g_Game.ObjectDelete(mound);
		}

		Print(string.Format("[PlantableFlora] WipeAllPlants: deleted %1 plants and %2 planting mounds", plants.Count(), mounds.Count()));
		PF_Config.Get().WipeAllPlants = false;
		PF_Config.Save();
	}
}
