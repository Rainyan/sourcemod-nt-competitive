public Action:Event_RoundStart(Handle:event, const String:name[], bool:dontBroadcast)
{	
	if (!g_isLive)
		return Plugin_Continue;
	
	if (g_shouldPause)
		TogglePause();
	
	g_fRoundTime = GetGameTime();
	
	g_roundCount++;
	
	g_jinraiScore	=	GetTeamScore(TEAM_JINRAI);
	g_nsfScore		=	GetTeamScore(TEAM_NSF);
	
	new roundLimit = GetConVarInt(g_hRoundLimit);
	
	if (g_jinraiScore > g_nsfScore + roundLimit - g_roundCount) // Jinrai wins, NSF can't catch up anymore
		PrintToChatAll("%s JINRAI WINS %i - %i", g_tag, g_jinraiScore, g_nsfScore);
	
	else if (g_jinraiScore > g_nsfScore + roundLimit - g_roundCount - 1) // Jinrai has almost won, match point
		PrintToChatAll("%s MATCH POINT", g_tag);
	
	else if (g_nsfScore > g_jinraiScore + roundLimit - g_roundCount) // NSF wins, Jinrai can't catch up anymore
		PrintToChatAll("%s NSF WINS %i - %i", g_tag, g_nsfScore, g_jinraiScore);
	
	else if (g_nsfScore > g_jinraiScore + roundLimit - g_roundCount - 1) // NSF has almost won, match point
		PrintToChatAll("%s MATCH POINT", g_tag);
	
	else if (g_roundCount == roundLimit) // Tie, we ran out of rounds to play
		ToggleLive();
	
	else
		PrintToChatAll("%s MR%i round %i (%i rounds left)", g_tag, roundLimit, g_roundCount, roundLimit - g_roundCount);
	
	return Plugin_Handled;
}

public Action:Event_PlayerTeam(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (!g_isLive || GetEventBool(event, "disconnect"))
		return Plugin_Continue;
	
	new client = GetClientOfUserId(GetEventInt(event, "userid"));
	//new team = GetEventInt(event, "team");
	
	PrintToChat(client, "%s Game is live, you cannot change your team right now!", g_tag);
	
	return Plugin_Stop;
}

public Event_Restart(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	if (StringToInt(newVal) != 0)
		g_roundCount = 0;
}
