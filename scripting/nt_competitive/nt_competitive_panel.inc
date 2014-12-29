public Action:Command_Pause(client, args)
{
	if (!g_isLive)
	{
		ReplyToCommand(client, "%s Game is not live.", g_tag);
		return Plugin_Stop;
	}
	
	new team = GetClientTeam(client);
	if (team != TEAM_JINRAI && team != TEAM_NSF) // Not in a team, ignore
		return Plugin_Stop;
	
	new Handle:panel = CreatePanel();
	SetPanelTitle(panel, "Request pause");
	
	DrawPanelText(panel, "Please select pause reason");
	
	DrawPanelItem(panel, "Technical difficulties");
	DrawPanelItem(panel, "Time-out");
	DrawPanelItem(panel, "Exit");
	
	SendPanelToClient(panel, client, PanelHandler_Pause, MENU_TIME);
	
	CloseHandle(panel);
	
	return Plugin_Handled;
}

public PanelHandler_Pause(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
	switch (choice)
	{
		case 1: // Pause requested for technical difficulties
			PauseRequest(client, REASON_TECHNICAL);
		
		case 2: // Pause requested for timeout
			PauseRequest(client, REASON_TIMEOUT);
		
		case 3: // Exit menu
			PauseRequest(client, -1); // Foo, Play menu exit sound here
	}
}
