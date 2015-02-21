public Action:Event_PlayerSpawn(Handle:event, const String:name[], bool:dontBroadcast)
{
	new client = GetClientOfUserId(GetEventInt(event, "userid"));
	g_isSpawned[client] = true;
}

public Action:Event_RoundStart(Handle:event, const String:name[], bool:dontBroadcast)
{
	for (new i = 1; i <= MaxClients; i++)
		g_isSpawned[i] = false;

	if (!g_isLive)
		return Plugin_Continue;
	
	g_fRoundTime	= GetGameTime();
	
	g_roundCount++;
	
	g_jinraiScore	= GetTeamScore(TEAM_JINRAI);
	g_nsfScore		= GetTeamScore(TEAM_NSF);
	
	if (g_shouldPause)
	{
		g_shouldPause = false;
		new Float:pauseTime = 5.0;
		CreateTimer(pauseTime, Timer_TogglePause);
		PrintToChatAll("%s Pausing the game...", g_tag);
	}
	
	new roundLimit = GetConVarInt(g_hRoundLimit);
	
	// ** Announce match state below **
	// Jinrai wins, NSF can't catch up anymore
	if (g_jinraiScore > g_nsfScore + roundLimit - (g_roundCount - 1)) // (g_roundCount - 1) because we're counting rounds by incrementing inside the event. So first round is 1, not 0.
	{
		#if DEBUG > 1
			PrintToServer("%s JINRAI WINS %i - %i", g_tag, g_jinraiScore, g_nsfScore);
			PrintToServer("jinScore %i > nsfScore %i + roundLimit %i - roundCount %i", g_jinraiScore, g_nsfScore, roundLimit, g_roundCount);
		#endif
			
		PrintToChatAll("%s JINRAI WINS %i - %i", g_tag, g_jinraiScore, g_nsfScore);
	}
	
	// NSF wins, Jinrai can't catch up anymore
	else if (g_nsfScore > g_jinraiScore + roundLimit - (g_roundCount - 1))
	{
		#if DEBUG > 1
			PrintToServer("%s NSF WINS %i - %i", g_tag, g_nsfScore, g_jinraiScore);
			PrintToServer("nsfScore %i > jinScore %i + roundLimit %i - roundCount %i", g_nsfScore, g_jinraiScore, roundLimit, g_roundCount);
		#endif
		
		PrintToChatAll("%s NSF WINS %i - %i", g_tag, g_nsfScore, g_jinraiScore);
	}
	
	// Tie, we ran out of rounds to play
	else if ((g_roundCount - 1) == roundLimit)
		PrintToChatAll("%s MATCH TIED %i - %i", g_tag, g_nsfScore, g_jinraiScore);
	
	// Match point, one team has almost won
	else if ((g_jinraiScore > g_nsfScore + roundLimit - (g_roundCount - 1) - 1) || (g_nsfScore > g_jinraiScore + roundLimit - g_roundCount - 1))
		PrintToChatAll("%s Round %i/%i (MATCH POINT)", g_tag, g_roundCount, roundLimit);
	
	// Just another round, simply announce round number
	else
		PrintToChatAll("%s Round %i/%i", g_tag, g_roundCount, roundLimit);
	// ** Announce match state above **
	
	
	// Check if game ended, and toggle off live mode if appropriate
	if ((g_jinraiScore > g_nsfScore + roundLimit - (g_roundCount - 1)) || (g_nsfScore > g_jinraiScore + roundLimit - (g_roundCount - 1)) || ((g_roundCount - 1) == roundLimit))
		ToggleLive();
	
	return Plugin_Handled;
}

public Action:Command_JoinTeam(client, args)
{
	// To-do: Allow team rejoining if player disconnects whilst live

	if (!g_isLive)
		return Plugin_Continue;
	
	PrintToChat(client, "%s Game is live, you cannot change your team right now!", g_tag);
	
	return Plugin_Stop;
}

public Event_Restart(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	if (StringToInt(newVal)) // If nonzero, we have a restart. So we reset the variable.
		g_roundCount = 0;
}

public Event_TeamNameJinrai(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	new String:nsfName[sizeof(g_teamName[])];
	GetConVarString(g_hNSFName, nsfName, sizeof(nsfName));
	
	if (StrEqual(newVal, "NSF", false)) // Make sure we don't use the other team's default name
	{
		SetConVarString(g_hJinraiName, oldVal);
		
		#if DEBUG
			PrintToServer("Jinrai tried using \"NSF\" as their name. This is blocked because it's reserved as NSF's fallback default name.");
		#endif
	}
	
	else if (strlen(newVal) == 0 || StrEqual(newVal, oldVal)) // No cvar value, use default name "Jinrai"
	{
		strcopy(g_teamName[TEAM_JINRAI], sizeof(g_teamName[]), "Jinrai");
		
		#if DEBUG
			PrintToServer("Jinrai to default name.");
		#endif
	}
	
	else if (StrEqual(newVal, nsfName)) // Jinrai tried using NSF's current name
	{
		SetConVarString(g_hJinraiName, oldVal);
		
		#if DEBUG
			PrintToServer("Jinrai tried using NSF's current name, blocked.");
		#endif
	}
	
	else // Name change ok
	{
		strcopy(g_teamName[TEAM_JINRAI], sizeof(g_teamName[]), newVal);
		
		#if DEBUG
			PrintToServer("Jinrai name: %s", newVal);
		#endif
	}
}

public Event_TeamNameNSF(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	new String:jinraiName[sizeof(g_teamName[])];
	GetConVarString(g_hJinraiName, jinraiName, sizeof(jinraiName));
	
	if (StrEqual(newVal, "Jinrai", false)) // Make sure we don't use the other team's default name
	{
		SetConVarString(g_hJinraiName, oldVal);
		
		#if DEBUG
			PrintToServer("NSF tried using \"Jinrai\" as their name. This is blocked because it's reserved as Jinrai's fallback default name.");
		#endif
	}
	
	else if (strlen(newVal) == 0 || StrEqual(newVal, oldVal)) // No cvar value, use default name "NSF"
	{
		strcopy(g_teamName[TEAM_NSF], sizeof(g_teamName[]), "NSF");
		
		#if DEBUG
			PrintToServer("NSF to default name.");
		#endif
	}
	
	else if (StrEqual(newVal, jinraiName)) // NSF tried using Jinrai's current name
	{
		SetConVarString(g_hNSFName, oldVal);
		
		#if DEBUG
			PrintToServer("NSF tried using Jinrai's current name, blocked.");
		#endif
	}
	
	else // Name change ok
	{
		strcopy(g_teamName[TEAM_NSF], sizeof(g_teamName[]), newVal);
		
		#if DEBUG
			PrintToServer("NSF name: %s", newVal);
		#endif
	}
}

public Event_SourceTVEnabled(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	if (StringToInt(newVal) && !IsSourceTVBotConnected())
	{
		#if DEBUG
			PrintToServer("Event_SourceTVEnabled: No SourceTV bot found.");
		#endif
		
		PrintToChatAll("%s SourceTV bot doesn't appear to be connected. Global replay won't be recorded.");
	}
	
	else if (!StringToInt(newVal) && g_isSourceTVRecording) // SourceTV disabled while recording a demo
		ToggleSourceTV();
}

public Event_SourceTVPath(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	if (!DirExists(newVal))
		InitDirectory(newVal);
}

public Action:Hook_Fade(UserMsg:msg_id, Handle:bf, const players[], playersNum, bool:reliable, bool:init)
{
	if (GetConVarInt(g_hForceCamera))
	{
		if (BfReadShort(bf) == 640 && !IsPlayerAlive(players[0]))
			return Plugin_Stop;
	}
	
	return Plugin_Continue;
}
