class PF_Bush_Base extends ItemBase
{
	// Item physics lets the model rest slightly above the ground; snap it to the terrain.
	// Negative = sink into the ground (tune by eye).
	static const float GROUND_OFFSET = -0.03;
	// the next stage / stump spawns this long after the old entity is deleted; with 300 ms a client once kept
	// the new tree invisible (same-tick swaps always do)
	static const int SWAP_DELAY_MS = 1000;

	// growth: only plants with a pf_nextStage in config (young stages) use these
	static const int STORAGE_VERSION = 4;	// first int written by OnStoreSave; 2 added m_GrowFactor, 3 m_FirstPlantedAt, 4 soil look (1-3 still load)
	protected string m_OwnerId;			// PlayerIdentity.GetId() of the planter, saved
	protected int m_PlantedAt;			// PF_Time.Now() when this stage started, saved with growing stages
	protected int m_FirstPlantedAt;		// PF_Time.Now() when the seed went into the ground, kept through all stages, saved; 0 = from an older save (counts as oldest)
	protected string m_NextStage;
	protected int m_GrowSeconds;
	protected float m_GrowFactor = 1;	// growth time multiplier from the planting hole (fertilizer/water), kept through all stages, saved
	protected bool m_Loaded;			// came from the save (OnStoreLoad), not newly planted/grown
	// seedling stage (config pf_soilLook): its soil heap keeps the look of the planting hole it was planted in, saved
	protected int m_SoilFertilizer;		// PF_PreparedGround.FERT_*
	protected bool m_SoilWatered;
	protected Object m_ClutterCutter;	// config pf_clutterCutter: keeps the grass off the soil heap (not saved, made again on load)

	void PF_Bush_Base()
	{
		RegisterNetSyncVariableInt("m_SoilFertilizer", 0, 2);
		RegisterNetSyncVariableBool("m_SoilWatered");
	}

	bool HasSoilLook()
	{
		return g_Game.ConfigGetInt("CfgVehicles " + GetType() + " pf_soilLook") == 1;
	}

	void SetSoil(int fertilizer, bool watered)
	{
		m_SoilFertilizer = fertilizer;
		m_SoilWatered = watered;
		SetSynchDirty();
	}

	void RefreshSoilLook()
	{
		PF_PlantingMound.ApplySoilLook(this, m_SoilFertilizer, m_SoilWatered);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (!g_Game.IsDedicatedServer() && HasSoilLook())
			PF_PlantingMound.ApplySoilLook(this, m_SoilFertilizer, m_SoilWatered);
	}

	string GetOwnerId()
	{
		return m_OwnerId;
	}

	void SetOwnerId(string ownerId)
	{
		m_OwnerId = ownerId;
		if (g_Game.IsServer() && !IsProjection())
			PF_Registry.NotifyOwner(ownerId);
	}

	int GetFirstPlantedAt()
	{
		return m_FirstPlantedAt;
	}

	void SetFirstPlantedAt(int time)
	{
		m_FirstPlantedAt = time;
	}

	protected int m_IsProjection = -1;	// cached config pf_isProjection (-1 = not read yet)

	// the hologram projection shown while placing a seed must not be snapped/frozen (config pf_isProjection = 1 on the
	// *_Placing classes, so species need no script class of their own)
	bool IsProjection()
	{
		if (m_IsProjection == -1)
			m_IsProjection = g_Game.ConfigGetInt("CfgVehicles " + GetType() + " pf_isProjection");
		return m_IsProjection == 1;
	}

	// stumps of felled trees: not in the player pools or base caps (PF_Caps)
	bool IsStump()
	{
		return GetType().Contains("_Stump");
	}

	override void EEInit()
	{
		super.EEInit();
		// client: seedling soil heap (also the placement preview) gets the map soil look without waiting for a change;
		// set in EEInit itself it was lost (test 2026-10-20), so a moment later
		if (!g_Game.IsDedicatedServer() && HasSoilLook())
			g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(RefreshSoilLook, 200, false);
		if (g_Game.IsServer() && !IsProjection())
		{
			// new planting; stage swaps pass the original time on, loaded plants get it in OnStoreLoad (runs after EEInit)
			if (m_FirstPlantedAt == 0)
				m_FirstPlantedAt = PF_Time.Now();
			SnapToGround();
			RefreshLifetime();
			// the CE restores the stored lifetime after loading, so set it again a moment later
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(RefreshLifetime, 10000, false);
			// spawners (VPP) move/drop the object after creation, so snap again a few times
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SnapToGround, 500, false);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SnapToGround, 2000, false);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SnapToGround, 5000, false);
			InitGrowth();
			PF_Registry.Register(this);
			string cutter;
			g_Game.ConfigGetText("CfgVehicles " + GetType() + " pf_clutterCutter", cutter);
			if (cutter != "" && !m_ClutterCutter)
				m_ClutterCutter = g_Game.CreateObjectEx(cutter, GetPosition(), ECE_PLACE_ON_SURFACE);
		}
		PF_Log.Debug("EEInit " + GetType() + " at " + GetPosition().ToString() + " server=" + g_Game.IsServer());
	}

	// Planted flora is not in types.xml, so the Central Economy would give it CleanupLifetimeDefault (45 s)
	// and delete it as soon as no player is within CleanupAvoidance (100 m).
	// The max is what a territory flag refresh resets the lifetime to (tested 2026-10-18), so set it on every load.
	// The remaining lifetime itself is saved by the CE and only set for new entities: plants outside an active
	// flag radius decay after PlantLifetimeDays, like base building.
	void RefreshLifetime()
	{
		float lifetime = PF_Config.Get().PlantLifetimeDays * 86400;
		float before = GetLifetime();
		// SetLifetimeMax also resets the lifetime to the max, so put the saved one back for loaded plants
		SetLifetimeMax(lifetime);
		if (m_Loaded && before > 0)
			SetLifetime(before);
		else
			SetLifetime(lifetime);
		PF_Log.Debug("lifetime " + GetType() + " " + before + " -> " + GetLifetime());
	}

	string GetNextStageConfig()
	{
		string next;
		g_Game.ConfigGetText("CfgVehicles " + GetType() + " pf_nextStage", next);
		return next;
	}

	void InitGrowth()
	{
		m_NextStage = GetNextStageConfig();
		if (m_NextStage == "")
			return;

		string kind;
		g_Game.ConfigGetText("CfgVehicles " + GetType() + " pf_kind", kind);
		m_GrowSeconds = PF_Config.Get().GrowSecondsFor(kind);

		// new planting; a loaded object gets its saved time in OnStoreLoad
		if (m_PlantedAt == 0)
			m_PlantedAt = PF_Time.Now();

		PF_Growth.Register(this);
	}

	string GetCutClass()
	{
		string cutClass;
		g_Game.ConfigGetText("CfgVehicles " + GetType() + " pf_cutClass", cutClass);
		return cutClass;
	}

	// only mature bushes (config pf_cutClass) can be cut; the projection never
	bool CanBeCut()
	{
		return !IsProjection() && GetCutClass() != "";
	}

	// Cut: sticks drop, the bush is replaced by an invisible placeholder class that regrows
	// into the mature class (same growth system, kind "bushRegrow")
	void Cut()
	{
		string cutClass = GetCutClass();
		if (cutClass == "")
			return;

		vector pos = GetPosition();
		vector ori = GetOrientation();
		for (int i = 0; i < 2; i++)
			g_Game.CreateObjectEx("LongWoodenStick", pos, ECE_PLACE_ON_SURFACE);

		PF_Bush_Base stub = PF_Bush_Base.Cast(g_Game.CreateObjectEx(cutClass, pos, ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS));
		if (stub)
		{
			stub.SetOrientation(ori);
			stub.SetFirstPlantedAt(m_FirstPlantedAt);
			stub.SetOwnerId(m_OwnerId);
		}
		PF_Log.Debug("" + GetType() + " cut -> " + cutClass + " at " + pos.ToString());
		g_Game.ObjectDelete(this);
	}

	string GetFellClass()
	{
		string fellClass;
		g_Game.ConfigGetText("CfgVehicles " + GetType() + " pf_fellClass", fellClass);
		return fellClass;
	}

	// counts for the tree spacing rule (config pf_isTree, any stage except the stump)
	bool IsTreePlant()
	{
		int isTree = g_Game.ConfigGetInt("CfgVehicles " + GetType() + " pf_isTree");
		return isTree == 1;
	}

	// only mature trees (config pf_fellClass) can be felled
	bool CanBeFelled()
	{
		return !IsProjection() && GetFellClass() != "";
	}

	// mature trees must be felled first; only young plants, bushes and stumps can be dug up
	bool CanBeDugUp()
	{
		return !IsProjection() && GetFellClass() == "";
	}

	// Fell: logs and sticks drop, the tree is replaced by a permanent stump (no regrow)
	void Fell()
	{
		string fellClass = GetFellClass();
		if (fellClass == "")
			return;

		vector pos = GetTrunkPosition();
		vector ori = GetOrientation();
		for (int i = 0; i < 3; i++)
			g_Game.CreateObjectEx("WoodenLog", pos + Vector(Math.RandomFloat(-1.5, 1.5), 0.3, Math.RandomFloat(-1.5, 1.5)), ECE_PLACE_ON_SURFACE);
		for (int j = 0; j < 4; j++)
			g_Game.CreateObjectEx("LongWoodenStick", pos + Vector(Math.RandomFloat(-1.5, 1.5), 0.3, Math.RandomFloat(-1.5, 1.5)), ECE_PLACE_ON_SURFACE);

		// the stump spawns a moment after the tree is gone, otherwise clients never show it
		PF_Log.Debug("" + GetType() + " felled -> " + fellClass + " at " + pos.ToString());
		if (fellClass != "none")
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PF_Bush_Base.SpawnStump, SWAP_DELAY_MS, false, fellClass, pos, ori, m_OwnerId, 1.0, m_FirstPlantedAt, false);
		g_Game.ObjectDelete(this);
	}

	// spawns the next stage or the stump; matured = it is the final stage reached by growing (player pool check)
	static void SpawnStump(string stumpClass, vector pos, vector ori, string ownerId, float growFactor, int firstPlantedAt, bool matured)
	{
		PF_Bush_Base stump = PF_Bush_Base.Cast(g_Game.CreateObjectEx(stumpClass, pos, ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS));
		if (stump)
		{
			stump.SetOrientation(ori);
			// pos is the trunk of the previous stage: put this model's trunk there
			stump.SetPosition(stump.GetPosition() - stump.GetTrunkOffset());
			stump.SetFirstPlantedAt(firstPlantedAt);
			stump.SetOwnerId(ownerId);
			stump.SetGrowFactor(growFactor);
			if (matured)
				PF_Caps.OnMatured(stump);
		}
	}

	void SetGrowFactor(float factor)
	{
		m_GrowFactor = factor;
	}

	bool IsGrown(int now)
	{
		return m_NextStage != "" && now - m_PlantedAt >= m_GrowSeconds * m_GrowFactor;
	}

	// not every vanilla tree model has its trunk at the model origin (forest oak t_quercusrobur_2fc: 2-3 m off,
	// so the grown tree stood away from the seedling). Config pf_trunk[] = {x, z}: trunk in model space, measured by
	// a ray survey (tools/trunk_survey.txt, written by gen_species.py). Missing = trunk at the origin.
	vector GetTrunkOffset()
	{
		array<float> trunk = new array<float>();
		g_Game.ConfigGetFloatArray("CfgVehicles " + GetType() + " pf_trunk", trunk);
		if (trunk.Count() < 2)
			return vector.Zero;
		vector offset = ModelToWorld(Vector(trunk[0], 0, trunk[1])) - GetPosition();
		offset[1] = 0;
		return offset;
	}

	vector GetTrunkPosition()
	{
		return GetPosition() + GetTrunkOffset();
	}

	// no SetModel in script: the next stage is a new entity of another class at the same spot
	void Grow()
	{
		vector pos = GetTrunkPosition();
		vector ori = GetOrientation();
		PF_Log.Debug("" + GetType() + " grew into " + m_NextStage + " at " + pos.ToString());
		// final stage reached by growing (not a cut bush growing back): the owner's pool may be over the limit
		string kind;
		g_Game.ConfigGetText("CfgVehicles " + GetType() + " pf_kind", kind);
		string afterNext;
		g_Game.ConfigGetText("CfgVehicles " + m_NextStage + " pf_nextStage", afterNext);
		bool matured = kind != "bushRegrow" && afterNext == "";
		// the next stage spawns a moment after this one is gone, otherwise clients can keep drawing both
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PF_Bush_Base.SpawnStump, SWAP_DELAY_MS, false, m_NextStage, pos, ori, m_OwnerId, m_GrowFactor, m_FirstPlantedAt, matured);
		g_Game.ObjectDelete(this);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		PF_Log.Debug("EEDelete " + GetType() + " at " + GetPosition().ToString() + " server=" + g_Game.IsServer());
		if (g_Game.IsServer())
		{
			PF_Growth.Unregister(this);
			PF_Registry.Unregister(this);
		}
		if (m_ClutterCutter && g_Game)
		{
			g_Game.ObjectDelete(m_ClutterCutter);
			m_ClutterCutter = null;
		}
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(STORAGE_VERSION);
		ctx.Write(m_OwnerId);
		ctx.Write(m_GrowFactor);
		ctx.Write(m_FirstPlantedAt);
		ctx.Write(m_SoilFertilizer);
		ctx.Write(m_SoilWatered);
		if (GetNextStageConfig() != "")
			ctx.Write(m_PlantedAt);
		PF_Log.Debug("OnStoreSave " + GetPosition().ToString());
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		// ghost projections saved by older builds (vanilla Hologram.PlaceEntity copy): drop them
		if (IsProjection())
			return false;
		if (!super.OnStoreLoad(ctx, version))
			return false;
		int storedVersion;
		if (!ctx.Read(storedVersion) || storedVersion < 1 || storedVersion > STORAGE_VERSION)
			return false;
		if (!ctx.Read(m_OwnerId))
			return false;
		if (storedVersion >= 2 && !ctx.Read(m_GrowFactor))
			return false;
		// older saves have no planting time: 0 = oldest (EEInit already set "now", undo that)
		m_FirstPlantedAt = 0;
		if (storedVersion >= 3 && !ctx.Read(m_FirstPlantedAt))
			return false;
		if (storedVersion >= 4)
		{
			if (!ctx.Read(m_SoilFertilizer) || !ctx.Read(m_SoilWatered))
				return false;
			SetSynchDirty();
		}
		if (GetNextStageConfig() != "")
		{
			if (!ctx.Read(m_PlantedAt))
				return false;
		}
		m_Loaded = true;
		PF_Log.Debug("OnStoreLoad " + GetPosition().ToString());
		return true;
	}

	// height of the model origin above the terrain. Tree models have their origin in the
	// middle of the model, not at the trunk base, so lift them by the bottom of their bounding box,
	// minus a root sink (the box includes the root tips; vanilla map trees sit ~3-10 % of their height deeper).
	// Preferred: config pf_groundOffset = how high the vanilla map sets this model (tools/tree_survey.txt).
	float GetGroundOffset()
	{
		PF_Config cfg = PF_Config.Get();
		float treeTune = cfg.OffsetOverrideFor(GetType());
		if (IsTreePlant())
			treeTune += cfg.TreeGroundOffset;

		string path = "CfgVehicles " + GetType() + " pf_groundOffset";
		if (g_Game.ConfigIsExisting(path))
			return g_Game.ConfigGetFloat(path) + treeTune;

		if (!IsTreePlant())
			return GROUND_OFFSET + treeTune;
		vector minMax[2];
		ClippingInfo(minMax);
		float bottom = -minMax[0][1];
		float height = minMax[1][1] - minMax[0][1];
		return bottom - height * cfg.TreeRootSinkPercent / 100 + treeTune;
	}

	void SnapToGround()
	{
		// seedling (config pf_alignToTerrain): the soil heap tilts with the slope like the planting hole
		if (g_Game.ConfigGetInt("CfgVehicles " + GetType() + " pf_alignToTerrain") == 1)
		{
			PF_PlantingMound.AlignObjectToTerrain(this, GetGroundOffset());
			dBodyActive(this, ActiveState.INACTIVE);
			dBodyDynamic(this, false);
			return;
		}
		vector pos = GetPosition();
		// terrain under the trunk, not under the model origin (GetTrunkOffset)
		vector trunk = pos + GetTrunkOffset();
		float terrainY = g_Game.SurfaceY(trunk[0], trunk[2]);
		// trees on a slope: the lowest terrain around the trunk, so the downhill roots don't hang in the air
		// (test 2026-10-20: birch on a steep slope floated); the uphill roots go into the slope
		float radius = PF_Config.Get().TreeSlopeSampleRadius;
		if (radius > 0 && IsTreePlant())
		{
			for (int i = 0; i < 8; i++)
			{
				float angle = i * Math.PI2 / 8;
				terrainY = Math.Min(terrainY, g_Game.SurfaceY(trunk[0] + Math.Cos(angle) * radius, trunk[2] + Math.Sin(angle) * radius));
			}
		}
		float groundY = terrainY + GetGroundOffset();
		if (Math.AbsFloat(pos[1] - groundY) > 0.005)
		{
			pos[1] = groundY;
			SetPosition(pos);
		}
		SetOrientation(Vector(GetOrientation()[0], 0, 0));
		dBodyActive(this, ActiveState.INACTIVE);
		dBodyDynamic(this, false);
	}

	// planted flora is scenery, never carried; not listed in the vicinity inventory (would give planted plants away)
	override bool IsInventoryVisible() { return false; }

	// wild trees/bushes have no name in the look-at panel (the panel hides on an empty name);
	// "Planted Beech" would give planted ones away. Shown with DebugLogs for development.
	override string GetDisplayName()
	{
		if (PF_Config.Get().DebugLogs)
			return super.GetDisplayName();
		return "";
	}
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
	override bool CanReceiveItemIntoCargo(EntityAI item) { return false; }
}

// Species classes (PF_Bush_*, PF_Tree_*, PF_Seed_*) are config-only: they use the script class of the nearest
// scripted config parent (PF_Bush_Base / PF_Seed_Base).
