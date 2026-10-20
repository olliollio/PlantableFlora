// Player messages as vanilla notification popups (white on dark, top right); MessageStatus lines were blue and hard to read.
// Server side only.
class PF_Notify
{
	static const float SHOW_SECONDS = 5;

	static void Send(Man player, string title, string detail = "")
	{
		if (player && player.GetIdentity())
			NotificationSystem.SendNotificationToPlayerIdentityExtended(player.GetIdentity(), SHOW_SECONDS, title, detail);
	}

	// the owner of a plant, if online (PlayerIdentity.GetId)
	static void SendToOwner(string ownerId, string title, string detail = "")
	{
		if (ownerId == "")
			return;
		array<Man> players = new array<Man>();
		g_Game.GetPlayers(players);
		foreach (Man player : players)
		{
			if (player && player.GetIdentity() && player.GetIdentity().GetId() == ownerId)
			{
				Send(player, title, detail);
				return;
			}
		}
	}
}
