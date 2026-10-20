// Fruit -> seeds: knife + fruit in the crafting menu ("Get seeds"). The fruit is used up, the knife loses a little health.
// 0 to MAX_SEEDS seeds (random, user 2026-10-20), the player gets a popup with the count.
class PF_RecipeFruitSeeds extends RecipeBase
{
	static const int MAX_SEEDS = 3;

	string GetFruitClass() { return ""; }
	string GetSeedClass() { return ""; }

	override void Init()
	{
		m_Name = "Get seeds";
		m_IsInstaRecipe = false;
		m_AnimationLength = 1.5;

		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = 3;
		m_MinQuantityIngredient[0] = -1;
		m_MaxQuantityIngredient[0] = -1;

		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = -1;
		m_MaxQuantityIngredient[1] = -1;

		// ingredient 1: the knife, stays but loses health
		InsertIngredient(0, "KitchenKnife");
		InsertIngredient(0, "SteakKnife");
		InsertIngredient(0, "HuntingKnife");
		InsertIngredient(0, "CombatKnife");
		InsertIngredient(0, "BoneKnife");
		InsertIngredient(0, "FangeKnife");
		InsertIngredient(0, "KukriKnife");
		InsertIngredient(0, "StoneKnife");
		InsertIngredient(0, "Cleaver");

		m_IngredientAddHealth[0] = -2;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = 0;
		m_IngredientDestroy[0] = false;

		// ingredient 2: the fruit, used up
		InsertIngredient(1, GetFruitClass());

		m_IngredientAddHealth[1] = 0;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = 0;
		m_IngredientDestroy[1] = true;

		// results: MAX_SEEDS seeds, Do() keeps a random number of them
		for (int i = 0; i < MAX_SEEDS; i++)
		{
			AddResult(GetSeedClass());
			m_ResultSetFullQuantity[i] = false;
			m_ResultSetQuantity[i] = -1;
			m_ResultSetHealth[i] = -1;
			m_ResultInheritsHealth[i] = -1;
			m_ResultInheritsColor[i] = -1;
			m_ResultToInventory[i] = -1;
			m_ResultReplacesIngredient[i] = -1;
		}
	}

	// keep 0..MAX_SEEDS of the spawned seeds and tell the player
	override void Do(ItemBase ingredients[], PlayerBase player, array<ItemBase> results, float specialty_weight)
	{
		super.Do(ingredients, player, results, specialty_weight);
		int keep = Math.RandomIntInclusive(0, MAX_SEEDS);
		for (int i = results.Count() - 1; i >= keep; i--)
		{
			if (results[i])
				g_Game.ObjectDelete(results[i]);
		}
		if (keep == 0)
			PF_Notify.Send(player, "You found no seeds");
		else if (keep == 1)
			PF_Notify.Send(player, "You found 1 seed");
		else
			PF_Notify.Send(player, "You found " + keep + " seeds");
	}

	// only the seed that grows on this map (Badlands plum vs. Chernarus plum, PF_World)
	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		return PF_World.IsNativeClass(GetSeedClass());
	}
}

class PF_RecipeAppleSeeds extends PF_RecipeFruitSeeds
{
	override string GetFruitClass() { return "Apple"; }
	override string GetSeedClass() { return "PF_Seed_Apple"; }
}
class PF_RecipePearSeeds extends PF_RecipeFruitSeeds
{
	override string GetFruitClass() { return "Pear"; }
	override string GetSeedClass() { return "PF_Seed_Pear"; }
}
class PF_RecipePlumSeeds extends PF_RecipeFruitSeeds
{
	override string GetFruitClass() { return "Plum"; }
	override string GetSeedClass() { return "PF_Seed_Plum"; }
}
class PF_RecipeBadlandsPlumSeeds extends PF_RecipeFruitSeeds
{
	override string GetFruitClass() { return "Plum"; }
	override string GetSeedClass() { return "PF_Seed_BadlandsPlum"; }
}
// Badlands food item; olive and date palm have no fruit item (their wild trees give the seed directly)
class PF_RecipeFigSeeds extends PF_RecipeFruitSeeds
{
	override string GetFruitClass() { return "Figs"; }
	override string GetSeedClass() { return "PF_Seed_Fig"; }
}

modded class PluginRecipesManagerBase
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new PF_RecipeAppleSeeds);
		RegisterRecipe(new PF_RecipePearSeeds);
		RegisterRecipe(new PF_RecipePlumSeeds);
		RegisterRecipe(new PF_RecipeBadlandsPlumSeeds);
		RegisterRecipe(new PF_RecipeFigSeeds);
	}
}
