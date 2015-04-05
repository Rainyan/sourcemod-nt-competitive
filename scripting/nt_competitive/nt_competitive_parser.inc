public Action:Event_PlayerSpawn(Handle:event, const String:name[], bool:dontBroadcast)
{
	new client = GetClientOfUserId(GetEventInt(event, "userid"));
	g_isSpawned[client] = true;
}

public Action:Event_PlayerDeath(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (g_killVerbosity == 1) // Print remaining playercount of both teams to everyone's console
	{
		UpdatePlayersAlive();
		
		new victim = GetClientOfUserId(GetEventInt(event, "userid"));
		new victimTeam = GetClientTeam(victim);
		g_playersAlive[victimTeam]--; // This hook is fired too soon for UpdatePlayersAlive() to consider the victim dead, so we manually subtract 1 here
		
		new String:msgBuffer[sizeof(g_tag) + ( sizeof(g_teamName[]) * 2 ) + 24];
		Format(msgBuffer, sizeof(msgBuffer), "%s %s (%i) -- vs -- %s (%i)", g_tag, g_teamName[TEAM_JINRAI], g_playersAlive[TEAM_JINRAI], g_teamName[TEAM_NSF], g_playersAlive[TEAM_NSF]);
		
		PrintToConsoleAll(msgBuffer);
	}
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
	
	ResetPlayersAlive(); // Reset global count of living players. Used for kill feeds etc
	
	if (g_roundCount == 1)
	{
		new String:tournamentName[128];
		GetConVarString(g_hCompetitionName, tournamentName, sizeof(tournamentName));
		
		if (strlen(tournamentName) > 0)
			LogCompetitive(tournamentName); // Log tournament name, if any
		
		new String:msgBuffer[256];
		Format(msgBuffer, sizeof(msgBuffer), "Competitive match started: %s vs %s", g_teamName[TEAM_JINRAI], g_teamName[TEAM_NSF]);
		
		LogCompetitive(msgBuffer);
	}
	
	new String:roundBuffer[64];
	Format(roundBuffer, sizeof(roundBuffer), "***** Round %i *****", g_roundCount);
	LogCompetitive(roundBuffer);
	
	if (g_shouldPause)
	{
		g_shouldPause = false;
		new Float:pauseTime = 5.0;
		CreateTimer(pauseTime, Timer_TogglePause);
		PrintToChatAll("%s Pausing the game...", g_tag);
		LogCompetitive("Game is PAUSED.");
	}
	
	new roundLimit = GetConVarInt(g_hRoundLimit);
	
	// ** Announce match state below **
	// Jinrai wins, NSF can't catch up anymore
	if (g_jinraiScore > g_nsfScore + roundLimit - (g_roundCount - 1)) // (g_roundCount - 1) because we're counting rounds by incrementing inside the event. So first round is 1, not 0.
	{
		#if DEBUG > 1
			PrintToServer("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore, g_nsfScore);
			PrintToServer("jinScore %i > nsfScore %i + roundLimit %i - roundCount %i", g_jinraiScore, g_nsfScore, roundLimit, g_roundCount);
		#endif
		
		new String:msgBuffer[256];
		Format(msgBuffer, sizeof(msgBuffer), "%s %s WINS %i - %i", g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore, g_nsfScore);
		
		PrintToChatAll(msgBuffer);
		LogCompetitive(msgBuffer);
	}
	
	// NSF wins, Jinrai can't catch up anymore
	else if (g_nsfScore > g_jinraiScore + roundLimit - (g_roundCount - 1))
	{
		#if DEBUG > 1
			PrintToServer("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_NSF], g_nsfScore, g_jinraiScore);
			PrintToServer("nsfScore %i > jinScore %i + roundLimit %i - roundCount %i", g_nsfScore, g_jinraiScore, roundLimit, g_roundCount);
		#endif
		
		new String:msgBuffer[256];
		Format(msgBuffer, sizeof(msgBuffer), "%s %s WINS %i - %i", g_tag, g_teamName[TEAM_NSF], g_nsfScore, g_jinraiScore);
		
		PrintToChatAll(msgBuffer);
		LogCompetitive(msgBuffer);
	}
	
	// Tie, we ran out of rounds to play
	else if ((g_roundCount - 1) == roundLimit)
	{
		new String:msgBuffer[256];
		Format(msgBuffer, sizeof(msgBuffer), "%s MATCH TIED %i - %i", g_tag, g_nsfScore, g_jinraiScore);
		
		PrintToChatAll(msgBuffer);
		LogCompetitive(msgBuffer);
	}
	
	// Match point, one team has almost won
	else if ((g_jinraiScore > g_nsfScore + roundLimit - (g_roundCount - 1) - 1) || (g_nsfScore > g_jinraiScore + roundLimit - g_roundCount - 1))
		PrintToChatAll("%s Round %i/%i (MATCH POINT)", g_tag, g_roundCount, roundLimit);
	
	// Just another round, simply announce round number
	else
		PrintToChatAll("%s Round %i/%i", g_tag, g_roundCount, roundLimit);
	// ** Announce match state above **
	
	
	// Check if game ended, and toggle off live mode if appropriate
	if ((g_jinraiScore > g_nsfScore + roundLimit - (g_roundCount - 1)) || (g_nsfScore > g_jinraiScore + roundLimit - (g_roundCount - 1)) || ((g_roundCount - 1) == roundLimit))
	{
		ToggleLive();
		LogCompetitive("Competitive match ended.");
	}
	
	return Plugin_Handled;
}

public Action:Command_JoinTeam(client, args)
{
	#if DEBUG
		if (g_debug_ignoreLiveTeams)
			return Plugin_Continue;
	#endif
	
	new String:clientName[MAX_NAME_LENGTH];
	GetClientName(client, clientName, sizeof(clientName));
	
	new String:steamID[MAX_STEAMID_LENGTH];
	if (!GetClientAuthId(client, AuthId_Steam2, steamID, sizeof(steamID)))
		LogError("Failed fetching auth string for %s (client %i)", clientName, client);
	
	new String:team[10];
	GetCmdArg(1, team, sizeof(team));
	new iTeam = StringToInt(team);
	
	if (!g_isLive)
		return Plugin_Continue;
	
	else if (g_assignedTeamWhenLive[client] == TEAM_UNASSIGNED) // Team not explicitly restricted for this player, let them join (substitutes after going live etc)
	{
		new String:msgBuffer[512];
		Format(msgBuffer, sizeof(msgBuffer), "Player \"%s\" <%s> joined team %s", clientName, steamID, g_teamName[g_assignedTeamWhenLive[client]]);
		LogCompetitive(msgBuffer);
		
		return Plugin_Continue;
	}
	
	else if (iTeam == g_assignedTeamWhenLive[client]) // Player attempts to join their correct team, let them
	{
		new String:msgBuffer[512];
		Format(msgBuffer, sizeof(msgBuffer), "Player \"%s\" <%s> joined team %s", clientName, steamID, g_teamName[g_assignedTeamWhenLive[client]]);
		LogCompetitive(msgBuffer);
		
		return Plugin_Continue;
	}
	
	new String:tempTeamName[13]; // We use this variable here for clarity, since teams can rename themselves
	switch (g_assignedTeamWhenLive[client])
	{
		case TEAM_JINRAI:
			strcopy(tempTeamName, sizeof(tempTeamName), "Jinrai");
		
		case TEAM_NSF:
			strcopy(tempTeamName, sizeof(tempTeamName), "NSF");
		
		case TEAM_SPECTATOR:
			strcopy(tempTeamName, sizeof(tempTeamName), "as spectator");
	}
	
	new String:msgBuffer[256];
	Format(msgBuffer, sizeof(msgBuffer), "Player \"%s\" <%s> attempted to join team %s instead of their assigned team %s. Blocked.", clientName, steamID, g_teamName[iTeam], g_teamName[g_assignedTeamWhenLive[client]]);
	LogCompetitive(msgBuffer);
	
	ReplyToCommand(client, "%s Game is live! You can only join %s.", g_tag, tempTeamName);
	
	ClientCommand(client, "jointeam %i", g_assignedTeamWhenLive[client]); // ChangeClientTeam glitches respawn, so we use client command instead
	
	return Plugin_Stop;
}

public Event_KillVerbosity(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	g_killVerbosity = StringToInt(newVal);
}

public Event_CommsBehaviour(Handle:cvar, const String:oldVal[], const String:newVal[])
{	
	if (g_isLive)
		SetConVarInt(g_hAlltalk, GetDesiredCommsBehaviour());
}

public Event_LogMode(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	// We won't toggle logging whilst live
	PrintToChatAll("Logging mode changed. Changes will become active once the game isn't live anymore.");
}

public Event_Restart(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	if (StringToInt(newVal)) // If nonzero, we have a restart. So we reset the variable.
	{
		g_roundCount = 0;
		
		if (g_isLive)
			LogCompetitive("Match restarted.");
	}
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
		
		if (g_isLive)
		{
			new String:msgBuffer[256];
			Format(msgBuffer, sizeof(msgBuffer), "Jinrai changed team name to: %s", newVal);
			
			LogCompetitive(msgBuffer);
		}
		
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
		
		if (g_isLive)
		{
			new String:msgBuffer[256];
			Format(msgBuffer, sizeof(msgBuffer), "NSF changed team name to: %s", newVal);
			
			LogCompetitive(msgBuffer);
		}
		
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
		
		PrintToChatAll("%s SourceTV bot doesn't appear to be connected. Global replay won't be recorded.", g_tag);
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
