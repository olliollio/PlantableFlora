class PF_ActionPlantSeedCB : ActiondeployObjectCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(PF_Config.Get().PlantSeconds);
	}
}

class PF_ActionPlantSeed : ActionDeployObject
{
	void PF_ActionPlantSeed()
	{
		m_CallbackClass = PF_ActionPlantSeedCB;
		m_Text = "Plant";
	}

	override bool HasProgress()
	{
		return false;
	}

	// Client: the prompt shows the player's limit of this kind for the spot. Outside their bases a full pool never blocks
	// (PF_Caps), it warns instead; inside their bases the limit blocks (server, CanBePlaced).
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		bool ok = super.ActionCondition(player, target, item);
		if (ok && !g_Game.IsDedicatedServer())
		{
			PF_Config cfg = PF_Config.Get();
			PF_Seed_Base seed = PF_Seed_Base.Cast(item);
			bool tree = seed && seed.IsTreeSeed();
			Hologram hologram = player.GetHologramLocal();
			bool inBase = hologram && PF_Territory.IsOwnBase(player, hologram.GetProjectionPosition());
			string kindName = "Bushes";
			string oneName = "bush";
			int own;
			int limit;
			if (tree)
			{
				kindName = "Trees";
				oneName = "tree";
			}
			if (inBase)
			{
				own = PF_ClientState.OwnBushesInBase;
				limit = cfg.MaxBushesPerPlayerInBase;
				if (tree)
				{
					own = PF_ClientState.OwnTreesInBase;
					limit = cfg.MaxTreesPerPlayerInBase;
				}
				m_Text = "Plant (Base: " + kindName + " " + own + "/" + limit + ")";
			}
			else
			{
				own = PF_ClientState.OwnBushes;
				limit = cfg.MaxBushesPerPlayer;
				if (tree)
				{
					own = PF_ClientState.OwnTrees;
					limit = cfg.MaxTreesPerPlayer;
				}
				if (own >= limit)
					m_Text = "Plant (" + kindName + " " + own + "/" + limit + ": your oldest " + oneName + " outside your base disappears when this one is grown)";
				else
					m_Text = "Plant (" + kindName + " " + own + "/" + limit + ")";
			}
		}
		return ok;
	}
}

// Vanilla "Choose placement" toggle; while the hologram is out the prompt also says why planting
// is not allowed here ("Choose placement (Needs a digging tool)"), the red hologram alone doesn't.
class PF_ActionTogglePlaceSeed : ActionTogglePlaceObject
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!super.ActionCondition(player, target, item))
			return false;

		if (!g_Game.IsDedicatedServer())
		{
			string text = Widget.TranslateString("#toggle_placing");
			PF_Seed_Base seed = PF_Seed_Base.Cast(item);
			Hologram hologram = player.GetHologramLocal();
			if (seed && hologram)
			{
				// runs our rules for the current spot (cached inside), even when a vanilla check already turned it red
				seed.CanBePlaced(player, hologram.GetProjectionPosition());
				string reason = seed.GetBlockReason();
				if (reason != "")
					text = text + " (" + reason + ")";
			}
			m_Text = text;
		}
		return true;
	}
}

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(PF_ActionPlantSeed);
		actions.Insert(PF_ActionTogglePlaceSeed);
		actions.Insert(PF_ActionDigPlantingMound);
		actions.Insert(PF_ActionDigMoundWithTool);
		actions.Insert(PF_ActionFertilizeMound);
		actions.Insert(PF_ActionWaterMound);
		actions.Insert(PF_ActionCutBush);
		actions.Insert(PF_ActionDigUpPlant);
		actions.Insert(PF_ActionFellTree);
		actions.Insert(PF_ActionSearchTreeSeed);
	}
}
