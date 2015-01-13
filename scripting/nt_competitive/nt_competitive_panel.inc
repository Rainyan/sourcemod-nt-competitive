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

public PanelHandler_UnPause(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
	switch (choice)
	{
		case 1: // Pause requested for technical difficulties
			UnPauseRequest(client);
		
		case 2: // Exit menu
			UnPauseRequest(client); // Foo, Play menu exit sound here
	}
}
