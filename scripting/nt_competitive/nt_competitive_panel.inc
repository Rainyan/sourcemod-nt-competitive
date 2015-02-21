public PanelHandler_Pause(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
	switch (choice)
	{
		case 1: // Pause requested for technical difficulties
		{
			PrecacheSound(g_menuSoundOk); // Button sounds are generally already precached, but jic
			EmitSoundToClient(client, g_menuSoundOk);
			PauseRequest(client, REASON_TECHNICAL);
		}
		
		case 2: // Pause requested for timeout
		{
			PrecacheSound(g_menuSoundOk);
			EmitSoundToClient(client, g_menuSoundOk);
			PauseRequest(client, REASON_TIMEOUT);
		}
		
		case 3: // Exit menu
		{
			PrecacheSound(g_menuSoundOk);
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
			PrecacheSound(g_menuSoundOk);
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
		
		case 2:
		{
			PrecacheSound(g_menuSoundOk);
			EmitSoundToClient(client, g_menuSoundCancel);
		}
	}
}
