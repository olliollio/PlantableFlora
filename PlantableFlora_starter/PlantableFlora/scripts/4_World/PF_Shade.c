// Tree shade. Vanilla (DayZ 1.29+) adds sun heat to the player's temperature (WorldData.m_TemperatureUnderSunModifier,
// only Badlands sets it: up to +8 °C at noon, x1.2 without a hat, less with overcast) unless
// Environment.CheckShadowPresence finds collision between the head and the sun. Tree crowns have no collision, so no
// tree ever casts shade. We add a crown test on top: the sun ray against the upper part of the bounding box of every
// tree nearby, wild or planted (same rule for both, so planted trees stay unrecognizable, §22). Server only, runs with
// the vanilla shadow check (every 30 s per player) and only while the sun heats.
class PF_Shade
{
	static const float RAY_LENGTH = 40;		// m toward the sun; longer shadows (low sun) are ignored
	static const float MIN_TREE_HEIGHT = 4;	// m, smaller plants (seedlings, saplings), stumps and fallen trunks cast no shade
	static const float CROWN_START = 0.35;	// crown = upper 65 % of the bounding box height (the bare trunk below lets the sun through)
	static const float CROWN_WIDTH = 0.8;	// crown width = this share of the bounding box width (sun gets through at the edges)

	// first tree whose crown is between head and sun, null if none
	static Object FindShadingTree(vector head, vector toSun)
	{
		vector center = head + toSun * (RAY_LENGTH * 0.5);
		array<Object> objs = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition3D(center, RAY_LENGTH * 0.5 + 10, objs, proxies);

		vector sunPoint = head + toSun * RAY_LENGTH;
		foreach (Object obj : objs)
		{
			if (!IsShadeTree(obj))
				continue;

			vector boxMin = vector.Zero;
			vector boxMax = vector.Zero;
			if (!CrownBox(obj, boxMin, boxMax))
				continue;

			if (SegmentHitsBox(obj.WorldToModel(head), obj.WorldToModel(sunPoint), boxMin, boxMax))
				return obj;
		}
		return null;
	}

	// crown box in the tree's model space: upper part of the bounding box, narrower; false for trees under MIN_TREE_HEIGHT
	static bool CrownBox(Object tree, out vector boxMin, out vector boxMax)
	{
		vector minMax[2];
		tree.ClippingInfo(minMax);
		float height = minMax[1][1] - minMax[0][1];
		if (height < MIN_TREE_HEIGHT)
			return false;

		boxMin = minMax[0];
		boxMax = minMax[1];
		boxMin[1] = minMax[0][1] + height * CROWN_START;
		for (int axis = 0; axis < 3; axis += 2)
		{
			float mid = (minMax[0][axis] + minMax[1][axis]) * 0.5;
			float half = (minMax[1][axis] - minMax[0][axis]) * 0.5 * CROWN_WIDTH;
			boxMin[axis] = mid - half;
			boxMax[axis] = mid + half;
		}
		return true;
	}

	// wild map trees and planted trees (any stage with pf_isTree; seedlings are filtered by MIN_TREE_HEIGHT), not bushes
	static bool IsShadeTree(Object obj)
	{
		if (!obj)
			return false;
		PF_Bush_Base plant = PF_Bush_Base.Cast(obj);
		if (plant)
			return !plant.IsProjection() && plant.IsTreePlant();
		return obj.IsTree();
	}

	// segment a-b against an axis-aligned box (slab test), all in the same space
	static bool SegmentHitsBox(vector a, vector b, vector boxMin, vector boxMax)
	{
		float tMin = 0;
		float tMax = 1;
		float swap = 0;
		for (int i = 0; i < 3; i++)
		{
			float d = b[i] - a[i];
			if (Math.AbsFloat(d) < 0.0001)
			{
				if (a[i] < boxMin[i] || a[i] > boxMax[i])
					return false;
				continue;
			}
			float t1 = (boxMin[i] - a[i]) / d;
			float t2 = (boxMax[i] - a[i]) / d;
			if (t1 > t2)
			{
				swap = t1;
				t1 = t2;
				t2 = swap;
			}
			tMin = Math.Max(tMin, t1);
			tMax = Math.Min(tMax, t2);
			if (tMin > tMax)
				return false;
		}
		return true;
	}
}

modded class Environment
{
	static const float PF_SHADE_DEBUG_SECONDS = 2;

	protected Object m_PF_ShadeTree;			// tree that shades the player since the last shadow check (null = none)
	protected float m_PF_ShadeDebugTimer;

	// sun heat the map adds right now (0 at night and on maps without a sun table)
	protected float PF_SunHeat()
	{
		if (!WorldDataDaytime.IsInRange(m_DayTime, m_WorldData.m_TemperatureUnderSunModifier.GetKeyArray()))
			return 0;
		return m_WorldData.GetSunEffectOnTemperature(m_DayTime);
	}

	override protected void CheckShadowPresence()
	{
		super.CheckShadowPresence();
		m_PF_ShadeTree = null;
		if (m_ShadowPresence || !PF_Config.Get().TreeShade)
			return;

		// only while the map's sun heats (Badlands by day); other maps have no sun heat, nothing to take away
		float sunHeat = PF_SunHeat();
		if (sunHeat <= 0)
			return;

		vector toSun = -g_Game.GetWorld().GetSunOrMoonDirection();
		if (toSun[1] <= 0)
			return;

		vector head = MiscGameplayFunctions.GetPlayerHeadPosition(m_Player);
		Object tree = PF_Shade.FindShadingTree(head, toSun);
		m_PF_ShadeTree = tree;
		if (tree)
			m_ShadowPresence = true;

		if (PF_Config.Get().DebugLogs)
		{
			string treeName = "none";
			if (tree)
				treeName = tree.GetType() + " " + tree.GetShapeName();
			PF_Log.Debug("shade " + m_Player.GetPosition() + " sun +" + sunHeat + " C, tree " + treeName);
		}
	}

	// admin tool (config ShadeDebug, players in AdminIds): shadow check every 2 s instead of 30 s, result sent to the
	// player and shown bottom left (PF_ShadeDebugHud)
	override void Update(float pDelta)
	{
		super.Update(pDelta);
		if (!PF_Config.Get().ShadeDebug || !m_Player || !PF_Config.Get().IsAdmin(m_Player.GetIdentity()))
			return;

		m_PF_ShadeDebugTimer += pDelta;
		if (m_PF_ShadeDebugTimer < PF_SHADE_DEBUG_SECONDS)
			return;
		m_PF_ShadeDebugTimer = 0;

		if (IsInsideBuilding())
			m_ShadowPresence = true;
		else
			CheckShadowPresence();

		// shown bottom left on the client (PF_ShadeDebugHud, Numpad 9 shows/hides it)
		float sunHeat = PF_SunHeat();
		string info;
		if (sunHeat <= 0)
		{
			info = "Shade: no sun heat now\nnight, clouds or map without sun heat\n";
		}
		else
		{
			if (m_PF_ShadeTree && PF_Bush_Base.Cast(m_PF_ShadeTree))
				info = "Shade: PLANTED tree\n";
			else if (m_PF_ShadeTree)
				info = "Shade: wild tree\n";
			else if (m_ShadowPresence)
				info = "Shade: vanilla (trunk, roof, building, terrain)\n";
			else
				info = "Shade: none, in the sun\n";

			if (m_PF_ShadeTree)
				info += m_PF_ShadeTree.GetType() + " " + m_PF_ShadeTree.GetShapeName() + "\n";
			if (m_ShadowPresence)
				info += "sun heat removed: " + sunHeat.ToString() + " C (x hat factor)\n";
			else
				info += "sun heat added: " + sunHeat.ToString() + " C (x hat factor)\n";
		}
		info += "air temperature: " + GetTemperature().ToString() + " C (updates every 3 s)";

		g_Game.RPCSingleParam(null, PF_RPC.SHADE_INFO, new Param1<string>(info), true, m_Player.GetIdentity());
	}
}
