// Double-include prevention
#if defined _panel_included_
	#endinput
#endif
#define _panel_included_

public PanelHandler_Pause(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;

	new team = GetClientTeam(client);

	switch (choice)
	{
		// Native button sounds are generally already precached by engine, but we're using PrecacheSound here jic

		case 1: // Pause requested for timeout
		{
			// No more timeouts allowed for this team
			if (g_usedTimeouts[team] >= GetConVarInt(g_hMaxTimeouts))
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);

				if (GetConVarInt(g_hMaxTimeouts) > 0)
					ReplyToCommand(client, "%s Your team has used all available time-outs for this match.", g_tag);

				else
					ReplyToCommand(client, "%s Tactical time-outs are not allowed for this match.", g_tag);
			}

			else
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				PauseRequest(client, REASON_TIMEOUT);
			}
		}

		case 2: // Pause requested for technical difficulties
		{
			PrecacheSound(g_menuSoundOk);
			EmitSoundToClient(client, g_menuSoundOk);
			PauseRequest(client, REASON_TECHNICAL);
		}

		case 3: // Exit menu
		{
			PrecacheSound(g_menuSoundCancel);
			EmitSoundToClient(client, g_menuSoundCancel);
		}
	}
}

public PanelHandler_UnPause(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;

	switch (choice)
	{
		case 1: // Pause requested for technical difficulties
		{
			PrecacheSound(g_menuSoundOk);
			EmitSoundToClient(client, g_menuSoundOk);
			UnPauseRequest(client);
		}

		case 2: // Exit menu
		{
			PrecacheSound(g_menuSoundCancel);
			EmitSoundToClient(client, g_menuSoundCancel);
		}
	}
}

public PanelHandler_CancelPause(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;

	switch (choice)
	{
		case 1: // Cancel pause requested
		{
			PrecacheSound(g_menuSoundOk);
			EmitSoundToClient(client, g_menuSoundOk);
			CancelPauseRequest(client);
		}

		case 2: // Exit menu
		{
			PrecacheSound(g_menuSoundCancel);
			EmitSoundToClient(client, g_menuSoundCancel);
		}
	}
}
