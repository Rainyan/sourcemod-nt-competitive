// Double-include prevention
#if defined _parser_included_
	#endinput
#endif
#define _parser_included_

public Action:Event_PlayerSpawn(Handle:event, const String:name[], bool:dontBroadcast)
{
	new client = GetClientOfUserId(GetEventInt(event, "userid"));
	g_isSpawned[client] = true;
}

public Action:Event_PlayerDeath(Handle:event, const String:name[], bool:dontBroadcast)
{
	if ( GetConVarInt(g_hKillVersobity) == 0 ) // Do nothing
		return Plugin_Continue;
	
	new attacker = GetClientOfUserId(GetEventInt(event, "attacker"));
	decl String:attackerName[MAX_NAME_LENGTH];
	GetClientName( attacker, attackerName, sizeof(attackerName) );
	
	new victim = GetClientOfUserId(GetEventInt(event, "userid"));
	new victimTeam = GetClientTeam(victim);
	
	switch ( GetConVarInt(g_hKillVersobity) )
	{
		case 1: // Print remaining playercount of both teams to everyone's console
		{
			UpdatePlayersAlive();
			
			g_playersAlive[victimTeam]--; // This hook is fired too soon for UpdatePlayersAlive() to consider the victim dead, so we manually subtract 1 here
			
			decl String:msgBuffer[sizeof(g_tag) + ( sizeof(g_teamName[]) * 2 ) + 27];
			Format(msgBuffer, sizeof(msgBuffer), "%s %s (%i) -- vs -- %s (%i)", g_tag, g_teamName[TEAM_JINRAI], g_playersAlive[TEAM_JINRAI], g_teamName[TEAM_NSF], g_playersAlive[TEAM_NSF]);
			
			PrintToConsoleAll(msgBuffer);
		}
		
		case 2: // Only show the victim how much damage they dealt to their killer (CSGO style)
		{
			decl String:msgBuffer[sizeof(g_tag) + MAX_NAME_LENGTH + 53];
			Format(msgBuffer, sizeof(msgBuffer), "%s Player \"%s\" killed you. Damage dealt: %i", g_tag, attackerName, g_damageDealt[victim][attacker]);
			
			if ( GetConVarBool(g_hVerbosityDelay) )
				ShowKillInfoNextRound(victim, msgBuffer);
			else
				PrintToConsole(victim, msgBuffer);
		}
		
		case 3: // Only show the victim their killer's remaining health
		{
			decl String:msgBuffer[sizeof(g_tag) + MAX_NAME_LENGTH + 53];
			Format(msgBuffer, sizeof(msgBuffer), "%s Player \"%s\" killed you with %i health remaining.", g_tag, attackerName, g_health[attacker]);
			
			if ( GetConVarBool(g_hVerbosityDelay) )
				ShowKillInfoNextRound(victim, msgBuffer);
			else
				PrintToConsole(victim, msgBuffer);
		}
	}
	
	return Plugin_Handled;
}

public Action:Event_PlayerHurt(Handle:event, const String:name[], bool:dontBroadcast)
{
	if ( GetConVarInt(g_hKillVersobity) < 2 ) // Kill verbosity cvar doesn't require any health info - do nothing
		return Plugin_Continue;
	
	new attacker = GetClientOfUserId(GetEventInt(event, "attacker"));
	new victim = GetClientOfUserId(GetEventInt(event, "userid"));
	
	new damage;
	new health;
	
	if ( !IsPlayerAlive(victim) ) // todo: is this called too fast for IsPlayerAlive to be reliable?
		health = 0;
	else
		health = GetClientOfUserId(GetEventInt(event, "health"));
	
	damage = g_health[victim] - health;
	
	g_health[victim] = health;
	g_damageDealt[attacker][victim] = damage;
	
	return Plugin_Handled;
}

public Action:Event_RoundStart(Handle:event, const String:name[], bool:dontBroadcast)
{
	new i;
	for (i = 1; i <= MaxClients; i++)
	{
		g_isSpawned[i] = false;
		g_health[i] = 100;
		
		for (new j = 1; j <= MaxClients; j++)
		{
			g_damageDealt[i][j] = 0;
		}
		
		if ( GetConVarBool(g_hVerbosityDelay) && g_shouldShowKillInfo[i] ) // We want to display last round's kill info
			PrintToConsole(i, g_killInfoBuffer[i]);
		
		g_shouldShowKillInfo[i] = false;
	}
	
	// Game is not live. Stop here.
	if (!g_isLive || g_isPaused)
		return Plugin_Continue;
	
	// Game is paused. Stop here.
	if (g_isPaused)
		return Plugin_Continue;
	
	// Game is currently being unpaused (points being recovered etc). Stop here.
	if (g_isCurrentlyUnPausing)
		return Plugin_Continue;
	
	g_fRoundTime	= GetGameTime();
	g_roundNumber++;
	
	g_jinraiScore[g_roundNumber]	= GetTeamScore(TEAM_JINRAI);
	g_nsfScore[g_roundNumber]	= GetTeamScore(TEAM_NSF);
	
	for (i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || IsFakeClient(i) )
			continue;
		
		g_playerDeaths[i][g_roundNumber]	= GetEntProp(i, Prop_Data, "m_iDeaths");	// Save everyone's death count for this round
		g_playerXP[i][g_roundNumber]		= GetEntProp(i, Prop_Data, "m_iFrags");	// Save everyone's XP for this round
	}
	
	ResetPlayersAlive(); // Reset global count of living players. Used for kill feeds etc
	
	decl String:msgBuffer[256];
	
	if (g_roundNumber == 1)
	{
		decl String:tournamentName[128];
		GetConVarString(g_hCompetitionName, tournamentName, sizeof(tournamentName));
		
		if (strlen(tournamentName) > 0)
			LogCompetitive(tournamentName); // Log tournament name, if any
		
		Format(msgBuffer, sizeof(msgBuffer), "Competitive match started: %s vs %s", g_teamName[TEAM_JINRAI], g_teamName[TEAM_NSF]);
		
		LogCompetitive(msgBuffer);
	}
	
	decl String:roundBuffer[64];
	Format(roundBuffer, sizeof(roundBuffer), "***** Round %i *****", g_roundNumber);
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
	if (g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber] + roundLimit - (g_roundNumber - 1)) // (g_roundNumber - 1) because we're counting rounds by incrementing inside the event. So first round is 1, not 0.
	{
		#if DEBUG > 1
			PrintToServer("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber]);
			PrintToServer("jinScore %i > nsfScore %i + roundLimit %i - roundCount %i", g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber], roundLimit, g_roundNumber);
		#endif
		
		Format(msgBuffer, sizeof(msgBuffer), "%s %s WINS %i - %i", g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore, g_nsfScore);
		
		PrintToChatAll(msgBuffer);
		LogCompetitive(msgBuffer);
		
		g_winner = TEAM_JINRAI;
	}
	
	// NSF wins, Jinrai can't catch up anymore
	else if (g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber] + roundLimit - (g_roundNumber - 1))
	{
		#if DEBUG > 1
			PrintToServer("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_NSF], g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);
			PrintToServer("nsfScore %i > jinScore %i + roundLimit %i - roundCount %i", g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber], roundLimit, g_roundNumber);
		#endif
		
		Format(msgBuffer, sizeof(msgBuffer), "%s %s WINS %i - %i", g_tag, g_teamName[TEAM_NSF], g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);
		
		PrintToChatAll(msgBuffer);
		LogCompetitive(msgBuffer);
		
		g_winner = TEAM_NSF;
	}
	
	// Tie, we ran out of rounds to play
	else if ( (g_roundNumber - 1) == roundLimit )
	{
		Format(msgBuffer, sizeof(msgBuffer), "%s MATCH TIED %i - %i", g_tag, g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);
		
		PrintToChatAll(msgBuffer);
		LogCompetitive(msgBuffer);
	}
	
	// Match point, one team has almost won
	else if ((g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber] + roundLimit - (g_roundNumber - 1) - 1) || (g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber] + roundLimit - g_roundNumber - 1))
		PrintToChatAll("%s Round %i/%i (MATCH POINT)", g_tag, g_roundNumber, roundLimit);
	
	// Just another round, simply announce round number
	else
	{
		// Don't announce match number during the initial "LIVE LIVE LIVE" chat spam
		if (g_spamTimerCount <= 1)
			PrintToChatAll("%s Round %i/%i", g_tag, g_roundNumber, roundLimit);
	}
	// ** Announce match state above **
	
	
	// Check if game ended, and toggle off live mode if appropriate
	if ((g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber] + roundLimit - (g_roundNumber - 1)) || (g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber] + roundLimit - (g_roundNumber - 1)) || ((g_roundNumber - 1) == roundLimit))
	{
		ToggleLive();
		LogCompetitive("Competitive match ended.");
	}
	
	return Plugin_Handled;
}

public Action:Command_JoinTeam(client, args)
{
	decl String:clientName[MAX_NAME_LENGTH];
	GetClientName(client, clientName, sizeof(clientName));

	if (!g_isLive || !GetConVarBool(g_hLimitTeams) )
	{
		if (g_isReady[client])
		{
			g_isReady[client] = false;
			PrintToChatAll("%s Player %s is NOT READY.", g_tag, clientName);
		}
		
		return Plugin_Continue;
	}
	
	new String:steamID[MAX_STEAMID_LENGTH];
	if (!GetClientAuthId(client, AuthId_Steam2, steamID, sizeof(steamID)))
		LogError("Failed fetching auth string for %s (client %i)", clientName, client);
	
	decl String:team[10];
	GetCmdArg(1, team, sizeof(team));
	new iTeam = StringToInt(team);
	
	decl String:msgBuffer[512];
	
	if (g_assignedTeamWhenLive[client] == TEAM_UNASSIGNED) // Team not explicitly restricted for this player, let them join (substitutes after going live etc)
	{
		Format(msgBuffer, sizeof(msgBuffer), "Player \"%s\" <%s> joined team %s", clientName, steamID, g_teamName[g_assignedTeamWhenLive[client]]);
		LogCompetitive(msgBuffer);
		
		return Plugin_Continue;
	}
	
	else if (iTeam == g_assignedTeamWhenLive[client]) // Player attempts to join their correct team, let them
	{
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
	
	Format(msgBuffer, sizeof(msgBuffer), "Player \"%s\" <%s> attempted to join team %s instead of their assigned team %s. Blocked.", clientName, steamID, g_teamName[iTeam], g_teamName[g_assignedTeamWhenLive[client]]);
	LogCompetitive(msgBuffer);
	
	PrintToChat(client, "%s Game is live! You can only join %s.", g_tag, tempTeamName);
	PrintToConsole(client, "%s Game is live! You can only join %s.", g_tag, tempTeamName);
	
	ClientCommand(client, "jointeam %i", g_assignedTeamWhenLive[client]); // ChangeClientTeam glitches respawn, so we use client command instead
	
	return Plugin_Stop;
}

public Event_CommsBehaviour(Handle:cvar, const String:oldVal[], const String:newVal[])
{	
	if (g_isLive)
		SetConVarInt(g_hAlltalk, GetDesiredCommsBehaviour());
}

public Event_LogMode(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	if (g_isLive)
	{
		// We won't toggle logging whilst live
		PrintToChatAll("Logging mode changed. Changes will become active once the game isn't live anymore.");
	}
}

public Event_Restart(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	if (StringToInt(newVal)) // If nonzero, we have a restart
	{
		g_roundNumber = 0; // Reset round count
		
		if (g_isLive)
			LogCompetitive("Match restarted.");
	}
}

public Event_TeamNameJinrai(Handle:cvar, const String:oldVal[], const String:newVal[])
{
	decl String:nsfName[sizeof(g_teamName[])];
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
			decl String:msgBuffer[256];
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
	decl String:jinraiName[sizeof(g_teamName[])];
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
			decl String:msgBuffer[256];
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

public Action:SayCallback(client, const String:command[], argc)
{
	// This player is not editing a cvar via chat, let the chat message through normally.
	if (!g_isEditingCvar[client])
		return Plugin_Continue;
	
	new Handle:cvar = FindConVar(g_convarBeingEdited);
	if (cvar == INVALID_HANDLE)
	{
		LogError("Attempted to edit an invalid cvar handle.");
		PrintToChat(client, "%s Error editing the value. See error logs.", g_tag);
		
		RemoveCommandListener(SayCallback, "say");
		RemoveCommandListener(SayCallback, "say_team");
		
		g_isEditingCvar[client] = false;
		
		RefereeMenu_GameInfo(client);
		
		return Plugin_Stop;
	}
	
	decl String:message[256];
	GetCmdArgString( message, sizeof(message) );
	
	StripQuotes(message);
	TrimString(message);
	
	// Player wishes to cancel editing cvar via chat hook
	if ( StrEqual(message, "cancel") )
	{
		PrintToChat(client, "%s Cancelled editing the value.", g_tag);
	}
	// Player wishes to empty a text var
	// Todo: Run a default values config instead, so this works for all types of cvars without fancy tricks
	else if ( StrEqual(message, "default") || strlen(message) < 1 || StrEqual(message, " ") )
	{	
		SetConVarString(cvar, "");
		PrintToChat(client, "%s Reset the value back to defaults.", g_tag);
	}
	// Player entered a valid chat message, changing cvar value accordingly
	else
	{
		SetConVarString(cvar, message);
		PrintToChat(client, "%s Set the value of \"%s\" to \"%s\"", g_tag, g_convarBeingEdited, message);
	}
	
	RemoveCommandListener(SayCallback, "say");
	RemoveCommandListener(SayCallback, "say_team");
	
	CloseHandle(cvar);
	
	g_isEditingCvar[client] = false;
	
	RefereeMenu_GameInfo(client);
	
	return Plugin_Handled;
}