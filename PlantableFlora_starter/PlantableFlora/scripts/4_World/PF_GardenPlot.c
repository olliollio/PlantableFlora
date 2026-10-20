// Vanilla garden plot as prepared ground (Claude.md §15, see PF_PreparedGround): the player digs the plot, may
// fertilize and water its slots with the vanilla actions, then plants a seed; the plot is removed when planting.
class PF_GardenPlot
{
	// outdoor vanilla garden plot (not polytunnel/greenhouse, not the placement hologram)
	static bool IsOutdoorPlot(Object obj)
	{
		return GardenPlot.Cast(obj) && !GardenPlotPolytunnel.Cast(obj) && !GardenPlotGreenhouse.Cast(obj) && !GardenPlotPlacing.Cast(obj);
	}

	// no vanilla seed or plant in any slot (planting removes the plot, so it must not destroy someone's crops)
	static bool IsEmpty(GardenPlot plot)
	{
		for (int i = 0; i < plot.GetGardenSlotsCount(); i++)
		{
			Slot slot = plot.GetSlotByIndex(i);
			if (slot && (slot.GetPlant() || slot.HasSeed()))
				return false;
		}
		return true;
	}

	// any fertilized slot / any watered slot counts for the whole plot; lime beats compost
	static void GetCare(GardenPlot plot, out int fertilizer, out bool watered)
	{
		fertilizer = PF_PreparedGround.FERT_NONE;
		watered = false;
		for (int i = 0; i < plot.GetGardenSlotsCount(); i++)
		{
			Slot slot = plot.GetSlotByIndex(i);
			if (!slot)
				continue;
			if (slot.GetFertilityState() != eFertlityState.NONE)
			{
				if (slot.GetFertilityType() == "GardenLime")
					fertilizer = PF_PreparedGround.FERT_LIME;
				else if (fertilizer == PF_PreparedGround.FERT_NONE)
					fertilizer = PF_PreparedGround.FERT_COMPOST;
			}
			if (slot.GetWateredState() == eWateredState.WET)
				watered = true;
		}
	}

	// growth time multiplier: any fertilized slot and any watered slot each give their bonus
	static float GrowFactor(GardenPlot plot)
	{
		int fertilizer;
		bool watered;
		GetCare(plot, fertilizer, watered);
		PF_Log.Debug("garden plot: fertilizer=" + fertilizer + " watered=" + watered);
		return PF_PreparedGround.Factor(fertilizer, watered);
	}
}
