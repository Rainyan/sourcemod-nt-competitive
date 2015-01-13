public Action:Event_RoundStart(Handle:event, const String:name[], bool:dontBroadcast)
{	
	if (!g_isLive)
		return Plugin_Continue;
	
	if (g_shouldPause)
		TogglePause();
	
	g_fRoundTime = GetGameTime();
	
	g_roundCount++;
	
	g_jinraiScore	= GetTeamScore(TEAM_JINRAI);
	g_nsfScore		= GetTeamScore(TEAM_NSF);
	
	new roundLimit = GetConVarInt(g_hRoundLimit);
	
	// Announce match state below
	if (g_jinraiScore > g_nsfScore + roundLimit - (g_roundCount - 1)) // Jinrai wins, NSF can't catch up anymore
	{
		#if DEBUG > 1
			PrintToServer("%s JINRAI WINS %i - %i", g_tag, g_jinraiScore, g_nsfScore);
			PrintToServer("jinScore %i > nsfScore %i + roundLimit %i - roundCount %i", g_jinraiScore, g_nsfScore, roundLimit, g_roundCount);
		#endif
			
		PrintToChatAll("%s JINRAI WINS %i - %i", g_tag, g_jinraiScore, g_nsfScore);
	}
	
	else if (g_nsfScore > g_jinraiScore + roundLimit - (g_roundCount - 1)) // NSF wins, Jinrai can't catch up anymore
	{
		#if DEBUG > 1
			PrintToServer("%s NSF WINS %i - %i", g_tag, g_nsfScore, g_jinraiScore);
			PrintToServer("nsfScore %i > jinScore %i + roundLimit %i - roundCount %i", g_nsfScore, g_jinraiScore, roundLimit, g_roundCount);
		#endif
		
		PrintToChatAll("%s NSF WINS %i - %i", g_tag, g_nsfScore, g_jinraiScore);
	}
	
	else if ((g_roundCount - 1) == roundLimit) // Tie, we ran out of rounds to play
		PrintToChatAll("%s MATCH TIED %i - %i", g_tag, g_nsfScore, g_jinraiScore);
	
	else if ((g_jinraiScore > g_nsfScore + roundLimit - (g_roundCount - 1) - 1) || (g_nsfScore > g_jinraiScore + roundLimit - g_roundCount - 1)) // Team has almost won, match point
		PrintToChatAll("%s Round %i/%i (MATCH POINT)", g_tag, g_roundCount, roundLimit);
	
	else
		PrintToChatAll("%s Round %i/%i", g_tag, g_roundCount, roundLimit);
	
	
	// Check if game ended, and toggle off live mode if appropriate
	if ((g_jinraiScore > g_nsfScore + roundLimit - (g_roundCount - 1)) || (g_nsfScore > g_jinraiScore + roundLimit - (g_roundCount - 1)) || ((g_roundCount - 1) == roundLimit))
		ToggleLive();
	
	return Plugin_Handled;
}

public Action:Event_PlayerTeam(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (!g_isLive || GetEventBool(event, "disconnect")) // Do nothing if not live or if event triggered by player disconnect
		return Plugin_Continue;
	
	new client = GetClientOfUserId(GetEventInt(event, "userid"));
	//new team = GetEventInt(event, "team");
	
	PrintToChat(client, "%s Game is live, you cannot change your team right now!", g_tag);
	
	return Plugin_Stop;
}

public Event_Restart(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	if (StringToInt(newVal)) // If nonzero, we have a restart. So we reset the variable.
		g_roundCount = 0;
}
