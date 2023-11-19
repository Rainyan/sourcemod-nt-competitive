// Double-include prevention
#if defined _panel_included_
	#endinput
#endif
#define _panel_included_

#pragma semicolon 1
#pragma newdecls required

public int PanelHandler_Pause(Menu menu, MenuAction action, int client, int choice)
{
	if (action != MenuAction_Select)
		return 0;

	int team = GetClientTeam(client);

	switch (choice)
	{
		case 1: // Pause requested for timeout
		{
			// No more timeouts allowed for this team
			if (g_usedTimeouts[team] >= GetConVarInt(g_hMaxTimeouts))
			{
				EmitSoundToClient(client, g_menuSoundCancel);

				if (GetConVarInt(g_hMaxTimeouts) > 0)
					ReplyToCommand(client, "%s Your team has used all available time-outs for this match.", g_tag);

				else
					ReplyToCommand(client, "%s Tactical time-outs are not allowed for this match.", g_tag);
			}

			else
			{
				EmitSoundToClient(client, g_menuSoundOk);
				PauseRequest(client, REASON_TIMEOUT);
			}
		}

		case 2: // Pause requested for technical difficulties
		{
			EmitSoundToClient(client, g_menuSoundOk);
			PauseRequest(client, REASON_TECHNICAL);
		}

		case 3: // Exit menu
		{
			EmitSoundToClient(client, g_menuSoundCancel);
		}
	}

	return 0;
}

public int PanelHandler_UnPause(Menu menu, MenuAction action, int client, int choice)
{
	if (action != MenuAction_Select)
		return 0;

	switch (choice)
	{
		case 1: // Pause requested for technical difficulties
		{
			EmitSoundToClient(client, g_menuSoundOk);
			UnPauseRequest(client);
		}

		case 2: // Exit menu
		{
			EmitSoundToClient(client, g_menuSoundCancel);
		}
	}

	return 0;
}

public int PanelHandler_CancelPause(Menu menu, MenuAction action, int client, int choice)
{
	if (action != MenuAction_Select)
		return 0;

	switch (choice)
	{
		case 1: // Cancel pause requested
		{
			EmitSoundToClient(client, g_menuSoundOk);
			CancelPauseRequest(client);
		}

		case 2: // Exit menu
		{
			EmitSoundToClient(client, g_menuSoundCancel);
		}
	}

	return 0;
}
