// Admin tool (config ShadeDebug), client: shade debug text from the server (PF_ClientState.ShadeInfo) bottom left.
// The server only sends it to players in AdminIds, so other players never see the box and Numpad 9 does nothing for
// them. Numpad 9 shows/hides it (visible by default)
class PF_ShadeDebugHud
{
	static const string LAYOUT = "PlantableFlora/gui/pf_shade_debug.layout";

	protected Widget m_Root;
	protected MultilineTextWidget m_Text;
	protected bool m_Visible = true;
	protected bool m_KeyDown;

	void ~PF_ShadeDebugHud()
	{
		if (m_Root)
			m_Root.Unlink();
	}

	void Update()
	{
		if (!PF_Config.Get().ShadeDebug || PF_ClientState.ShadeInfo == "")
		{
			if (m_Root)
				m_Root.Show(false);
			return;
		}

		// toggle on key press (not while a menu like the chat is open)
		bool keyDown = KeyState(KeyCode.KC_NUMPAD9) != 0;
		if (keyDown && !m_KeyDown && !g_Game.GetUIManager().GetMenu())
			m_Visible = !m_Visible;
		m_KeyDown = keyDown;

		if (!m_Root)
		{
			m_Root = g_Game.GetWorkspace().CreateWidgets(LAYOUT);
			if (!m_Root)
				return;
			m_Text = MultilineTextWidget.Cast(m_Root.FindAnyWidget("PF_ShadeDebugText"));
		}

		m_Root.Show(m_Visible);
		if (m_Visible && m_Text)
			m_Text.SetText(PF_ClientState.ShadeInfo + "\n(Numpad 9: hide)");
	}
}

modded class MissionGameplay
{
	protected ref PF_ShadeDebugHud m_PF_ShadeDebugHud;

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!m_PF_ShadeDebugHud)
			m_PF_ShadeDebugHud = new PF_ShadeDebugHud();
		m_PF_ShadeDebugHud.Update();
	}
}
