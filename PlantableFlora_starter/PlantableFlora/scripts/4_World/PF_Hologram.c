// Plant projections have no solid geometry and sit on the terrain by design, so the
// box/slope checks written for solid deployables would reject them everywhere.
// Soil, tool and (later) distance rules live in PF_Seed_Base.CanBePlaced.
modded class Hologram
{
	protected bool PF_IsPlantProjection()
	{
		return m_Projection && PF_Bush_Base.Cast(m_Projection);
	}

	override bool IsCollidingBBox(ItemBase action_item = null)
	{
		if (PF_IsPlantProjection())
			return false;
		if (GardenPlotPlacing.Cast(m_Projection))
			return PF_IsGardenPlotColliding(action_item);
		return super.IsCollidingBBox(action_item);
	}

	// Garden plot: same box test as vanilla, but trees, bushes, planted flora and small rocks don't block it
	// (the vanilla box is bigger than the visible plot and rejected plots next to any vegetation)
	protected bool PF_IsGardenPlotColliding(ItemBase action_item)
	{
		if (CfgGameplayHandler.GetDisableIsCollidingBBoxCheck())
			return false;

		vector minMax[2];
		GetProjectionCollisionBox(minMax);
		vector relativeOffset;
		relativeOffset[1] = (minMax[1][1] - minMax[0][1]) * 0.5;
		vector lift = "0 0.05 0";	// like vanilla: keeps the box off floors
		vector center = m_Projection.GetPosition() + relativeOffset + lift;
		array<Object> excluded = new array<Object>();
		array<Object> hits = new array<Object>();
		excluded.Insert(m_Projection);
		excluded.Insert(m_Player);
		if (action_item)
			excluded.Insert(action_item);
		if (!g_Game.IsBoxCollidingGeometry(center, GetProjectionOrientation(), GetCollisionBoxSize(minMax), ObjIntersectFire, ObjIntersectGeom, excluded, hits))
			return false;

		foreach (Object obj : hits)
		{
			if (!PF_IsIgnoredForPlot(obj))
			{
				PF_LogPlotBlocker("blocked by " + obj.GetType() + " (" + obj.GetShapeName() + ")");
				return true;
			}
		}
		return false;
	}

	protected static string s_LastPlotLog;

	// DebugLogs: why a garden plot hologram is red (once per change)
	protected void PF_LogPlotBlocker(string reason)
	{
		if (!PF_Config.Get().DebugLogs)
			return;
		vector pos = m_Projection.GetPosition();
		string surface;
		g_Game.SurfaceGetType3D(pos[0], pos[1], pos[2], surface);
		// rounded to 1 m so moving the hologram doesn't log every frame
		string where = Math.Round(pos[0]).ToString() + "/" + Math.Round(pos[2]).ToString();
		string text = "garden plot " + reason + " at " + where + " surface " + surface + " fertile=" + g_Game.IsSurfaceFertile(surface);
		if (text == s_LastPlotLog)
			return;
		s_LastPlotLog = text;
		PF_Log.Debug(text);
	}

	protected bool PF_IsIgnoredForPlot(Object obj)
	{
		if (!obj || obj.IsTree() || obj.IsBush() || PF_Bush_Base.Cast(obj))
			return true;
		if (obj.IsRock())
		{
			vector minMax[2];
			return obj.ClippingInfo(minMax) < 1.5;
		}
		return false;
	}

	override bool IsCollidingGeometryProxy(ItemBase action_item = null)
	{
		if (PF_IsPlantProjection())
			return false;
		return super.IsCollidingGeometryProxy(action_item);
	}

	// garden plot hologram (seeds are planted onto plots): the vanilla limits (corner height 0.3 m, tilt 10 deg)
	// reject plots on ground that looks flat; config PlotSlopeTolerance / PlotMaxTiltDegrees. Set in script, not
	// in config.cpp, so there is one place for the values. Also applies to normal vegetable gardens.
	protected bool PF_IsGardenPlotProjection()
	{
		return m_Projection && GardenPlotPlacing.Cast(m_Projection);
	}

	override bool IsBaseViable()
	{
		if (PF_IsPlantProjection())
			return true;
		if (!PF_IsGardenPlotProjection())
			return super.IsBaseViable();
		m_SlopeTolerance = PF_Config.Get().PlotSlopeTolerance;
		bool viable = super.IsBaseViable();
		if (viable)
			PF_LogPlotBlocker("base ok");
		else
			PF_LogPlotBlocker("base not flat enough");
		return viable;
	}

	override bool IsCollidingAngle()
	{
		if (PF_IsPlantProjection())
			return false;
		if (PF_IsGardenPlotProjection())
		{
			float tilt = PF_Config.Get().PlotMaxTiltDegrees;
			m_YawPitchRollLimit = Vector(tilt, tilt, tilt);
		}
		return super.IsCollidingAngle();
	}

	// a flat plot may sink into a slope a bit (vanilla rejects corners more than 0.3 m in the ground)
	override bool IsInTerrain()
	{
		if (PF_IsPlantProjection() || PF_IsGardenPlotProjection())
			return false;
		return super.IsInTerrain();
	}

	// Tree projections have their origin half a model height above the ground, so the vanilla
	// height checks (origin above camera = "clipping roof", origin vs player height) must use the ground point.
	// Planting indoors is already ruled out by the fertile surface check.
	// vanilla tests the player against the whole projection box; a tree crown is metres wide,
	// so only refuse when the player stands on the planting spot itself
	override bool IsCollidingPlayer()
	{
		if (!PF_IsPlantProjection())
			return super.IsCollidingPlayer();
		vector a = m_Player.GetPosition();
		vector b = m_Projection.GetPosition();
		a[1] = 0;
		b[1] = 0;
		return vector.DistanceSq(a, b) < 0.36;
	}

	override bool IsClippingRoof()
	{
		if (PF_IsPlantProjection())
			return false;
		return super.IsClippingRoof();
	}

	override bool HeightPlacementCheck()
	{
		PF_Bush_Base plant;
		if (!Class.CastTo(plant, m_Projection))
			return super.HeightPlacementCheck();
		float groundY = m_Projection.GetPosition()[1] - plant.GetGroundOffset();
		return Math.AbsFloat(m_Player.GetPosition()[1] - groundY) <= DEFAULT_MAX_PLACEMENT_HEIGHT_DIFF;
	}

	// Vanilla spawns a copy of the projection class when it differs from the placed item
	// (seed != plant) and drops the reference, which left a ghost young plant at every spot.
	// The seed spawns the real plant itself in OnPlacementComplete.
	override EntityAI PlaceEntity(EntityAI entity_for_placing)
	{
		if (PF_IsPlantProjection())
			return entity_for_placing;
		return super.PlaceEntity(entity_for_placing);
	}

	// Hologram offsets and slope alignment sink the bush into hills; the real plant is
	// placed upright on the terrain surface, so show exactly that.
	override void SetProjectionPosition(vector position)
	{
		PF_Bush_Base plant;
		if (Class.CastTo(plant, m_Projection))
		{
			// snap onto the centre of a nearby planting mound / garden plot
			if (PF_Config.Get().RequirePreparedGround)
			{
				EntityAI ground = PF_PreparedGround.FindNear(position, PF_Config.Get().SnapDistance);
				if (ground)
				{
					position[0] = ground.GetPosition()[0];
					position[2] = ground.GetPosition()[2];
				}
			}
			position[1] = g_Game.SurfaceY(position[0], position[2]) + plant.GetGroundOffset();
		}
		super.SetProjectionPosition(position);
	}
}
