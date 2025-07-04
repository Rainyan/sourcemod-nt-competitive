// Double-include prevention
#if defined _NT_COMPETITIVE_HOOKS_INC_
	#endinput
#endif
#define _NT_COMPETITIVE_HOOKS_INC_

#pragma semicolon 1
#pragma newdecls required

public void Event_PlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
	int client = GetClientOfUserId(GetEventInt(event, "userid"));

	if ( DidPlayerReallySpawn(client) )
	{
		g_isSpawned[client] = true;
		g_survivedLastRound[client] = true; // This will be checked on next round event, thus the variable name
	}
}

public Action Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast)
{
	int victim = GetClientOfUserId(GetEventInt(event, "userid"));

	// Did the death happen before round time ran out?
	int gameState = GameRules_GetProp("m_iGameState");
	if (gameState == GAMESTATE_ROUND_ACTIVE)
		g_survivedLastRound[victim] = false;

	if ( GetConVarInt(g_hKillVersobity) == 0 ) // Do nothing
		return Plugin_Continue;

	int victimTeam = GetClientTeam(victim);

	int attacker = GetClientOfUserId(GetEventInt(event, "attacker"));

	switch ( GetConVarInt(g_hKillVersobity) )
	{
		case 1: // Print remaining playercount of both teams to everyone's console
		{
			UpdatePlayersAlive();

			// This hook is fired too soon for UpdatePlayersAlive() to consider the victim dead, so we manually subtract 1 here
			g_playersAlive[victimTeam]--;

			PrintToConsoleAll("%s %s (%i) -- vs -- %s (%i)", g_tag, g_teamName[TEAM_JINRAI], g_playersAlive[TEAM_JINRAI], g_teamName[TEAM_NSF], g_playersAlive[TEAM_NSF]);
		}

		case 2: // Only show the victim how much damage they dealt to their killer (CSGO style)
		{
			char msgBuffer[sizeof(g_tag) + MAX_NAME_LENGTH + 53];
			Format(msgBuffer, sizeof(msgBuffer), "%s Player \"%N\" killed you. Damage dealt: %i", g_tag, attacker, g_damageDealt[victim][attacker]);

			if ( GetConVarBool(g_hVerbosityDelay) )
				ShowKillInfoNextRound(victim, msgBuffer);
			else
				PrintToConsole(victim, msgBuffer);
		}

		case 3: // Only show the victim their killer's remaining health
		{
			char msgBuffer[sizeof(g_tag) + MAX_NAME_LENGTH + 53];
			Format(msgBuffer, sizeof(msgBuffer), "%s Player \"%N\" killed you with %i health remaining.", g_tag, attacker, g_health[attacker]);

			if ( GetConVarBool(g_hVerbosityDelay) )
				ShowKillInfoNextRound(victim, msgBuffer);
			else
				PrintToConsole(victim, msgBuffer);
		}
	}

	CenterText_DisplayRemaining();

	return Plugin_Handled;
}

public Action Event_PlayerHurt(Event event, const char[] name, bool dontBroadcast)
{
	if ( GetConVarInt(g_hKillVersobity) < 2 ) // Kill verbosity cvar doesn't require any health info - do nothing
		return Plugin_Continue;

	int attacker = GetClientOfUserId(GetEventInt(event, "attacker"));
	int victim = GetClientOfUserId(GetEventInt(event, "userid"));

	int damage, health;

	if ( !IsPlayerAlive(victim) ) // todo: is this called too fast for IsPlayerAlive to be reliable?
	{
		health = 0;
	}
	else
	{
		health = GetClientOfUserId(GetEventInt(event, "health"));

		if (!g_survivedLastRound[victim])
			LogError("g_survivedLastRound has player %i incorrectly marked as dead (health remaining: %i)", victim, health);
	}

	damage = g_health[victim] - health;

	g_health[victim] = health;
	g_damageDealt[attacker][victim] = damage;

	return Plugin_Handled;
}

public Action Event_RoundStart(Event event, const char[] name, bool dontBroadcast)
{
	for (int i = 1; i <= MaxClients; i++)
	{
		g_isSpawned[i] = false;
		g_health[i] = 100;

		for (int j = 1; j <= MaxClients; j++)
		{
			g_damageDealt[i][j] = 0;
		}
		PrintKillInfoBuffer(i);
	}

	CreateTimer(5.0, Timer_ClearGhostCapper);

	// Game is not live. Stop here.
	if (!g_isLive)
		return Plugin_Continue;

	// Game is paused. Stop here.
	if (g_isPaused)
		return Plugin_Continue;

	g_fRoundTime = GetGameTime();
	g_fGhostOvertime = g_fGhostOvertimeTick = GetConVarFloat(g_hGhostOvertimeGrace);

	// Game is currently being unpaused (points being recovered etc). Stop here.
	if (g_isCurrentlyUnPausing)
	{
		RestoreRound(g_roundNumber, false);
		PrintToChatAll( "%s Round %i/%i", g_tag, g_roundNumber, GetConVarInt(g_hRoundLimit) );
		SpamTimer();

		g_isCurrentlyUnPausing = false; // We have finished restoring score and announcing match live again

		return Plugin_Continue;
	}

	// Increment round number, unless we just came back from a pause
	else if (!g_isCurrentlyRestoringRound)
	{
		g_roundNumber++;
		g_furthestPlayedRound	= g_roundNumber;
	}

	// Round is being edited, stop here.
	else if (g_isCurrentlyRestoringRound)
	{
		g_isCurrentlyRestoringRound = false;

		PrintToChatAll( "%s Round %i/%i", g_tag, g_roundNumber, GetConVarInt(g_hRoundLimit) );

		return Plugin_Continue;
	}

	// Should we disallow wins by timeout?
	if ( g_roundNumber > 1 && GetConVarBool(g_hPreventZanshiStrats) )
	{
		// Nobody captured the ghost last round
		if (g_ghostCapturingTeam != TEAM_JINRAI && g_ghostCapturingTeam != TEAM_NSF)
		{
			int survivors[sizeof(g_teamName)];

			// Get amount of survivors on both teams
			for (int i = 1; i <= MaxClients; i++)
			{
				if ( !IsClientInGame(i) )
					continue;

				int team = GetClientTeam(i);
				if (team != TEAM_JINRAI && team != TEAM_NSF)
					continue;

				if (g_survivedLastRound[i])
					survivors[team]++;
			}

			// Both teams had players still alive when the time ran out
			if (survivors[TEAM_JINRAI] > 0 && survivors[TEAM_NSF] > 0)
			{
				// Roll back team scores to previous round's state
				RestoreRound(g_roundNumber - 1, true);
				PrintToChatAll("%s Round timed out. No team point awarded.", g_tag);
			}
		}
	}

	if (g_roundNumber > MAX_ROUNDS_PLAYED)
	{
		LogError("Current round number %i is larger than MAX_ROUNDS_PLAYED %i – round info array is too small to store current round. This should never happen.", g_roundNumber, MAX_ROUNDS_PLAYED);
	}
	else
	{
		g_jinraiScore[g_roundNumber]	= GetTeamScore(TEAM_JINRAI);
		g_nsfScore[g_roundNumber]	= GetTeamScore(TEAM_NSF);

		for (int i = 1; i <= MaxClients; i++)
		{
			if ( !IsClientInGame(i) || IsFakeClient(i) )
				continue;

			g_playerDeaths[i][g_roundNumber]	= GetPlayerDeaths(i);	// Save everyone's death count for this round
			g_playerXP[i][g_roundNumber]		= GetPlayerXP(i);	// Save everyone's XP for this round
		}
	}

	ResetPlayersAlive(); // Reset global count of living players. Used for kill feeds etc

	if (g_roundNumber == 1)
	{
		char zone[10];
		FormatTime(zone, sizeof(zone), "%z");
		char time[64];
		FormatTime(time, sizeof(time), "%F %R", GetTime());
		
		char mapName[32];
		GetCurrentMap(mapName, sizeof(mapName));
		
		char tournamentName[128];
		GetConVarString(g_hCompetitionName, tournamentName, sizeof(tournamentName));
		
		LogCompetitive("\n%s\nCompetitive match started:\n%s vs %s\n%s (%s)\n%s",
			tournamentName,
			g_teamName[TEAM_JINRAI], g_teamName[TEAM_NSF],
			time, zone, mapName);
	}

	//LogCompetitive("***** Round %i *****", g_roundNumber);
	if (g_shouldPause)
	{
		g_shouldPause = false;

		float pauseTime = 5.0;
		CreateTimer(pauseTime, Timer_TogglePause);
		PrintToChatAll("%s Pausing the game...", g_tag);
		LogCompetitive("Game is PAUSED.");
	}

	CheckMatchState();

	return Plugin_Handled;
}

public void Event_JinraiScore(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (g_isLive)
	{
		if (g_roundNumber > 0)
		{
			g_jinraiScore[g_roundNumber] = StringToInt(newValue);
			SetTeamScore(TEAM_JINRAI, g_jinraiScore[g_roundNumber]);
		}
		else
		{
			LogError("Tried to edit unexpected round number %i, expected round 1 or larger", g_roundNumber);
		}
	}
}

public void Event_NSFScore(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (g_isLive)
	{
		if (g_roundNumber > 0)
		{
			g_nsfScore[g_roundNumber] = StringToInt(newValue);
			SetTeamScore(TEAM_NSF, g_nsfScore[g_roundNumber]);
		}
		else
		{
			LogError("Tried to edit unexpected round number %i, expected round 1 or larger", g_roundNumber);
		}
	}
}

public void Event_CommsBehaviour(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (g_isLive)
		SetConVarInt(g_hAlltalk, GetDesiredCommsBehaviour());
}

public void Event_LogMode(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (g_isLive)
	{
		// We won't toggle logging whilst live
		PrintToChatAll("Logging mode changed. Changes will become active once the game isn't live anymore.");
	}
}

public void Event_Restart(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (StringToInt(newValue)) // If nonzero, we have a restart
	{
		g_roundNumber = 0; // Reset round count
		GameRules_SetProp("m_iRoundNumber", g_roundNumber);

		g_ghostCapturingTeam = TEAM_NONE; // Reset last round capper

		if (g_isLive)
			LogCompetitive("Match restarted.");
	}
}

public void Event_ZanshiStrats(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (StringToInt(newValue))
		CheckGhostcapPlugin();
}

public void Event_TeamNameJinrai(ConVar convar, const char[] oldValue, const char[] newValue)
{
	char nsfName[sizeof(g_teamName[])];
	GetConVarString(g_hNSFName, nsfName, sizeof(nsfName));

	if (StrEqual(newValue, "NSF", false)) // Make sure we don't use the other team's default name
	{
		SetConVarString(g_hJinraiName, oldValue);
	}

	else if (strlen(newValue) == 0 || StrEqual(newValue, oldValue)) // No cvar value, use default name "Jinrai"
	{
		strcopy(g_teamName[TEAM_JINRAI], sizeof(g_teamName[]), "Jinrai");
	}

	else if (StrEqual(newValue, nsfName)) // Jinrai tried using NSF's current name
	{
		SetConVarString(g_hJinraiName, oldValue);
	}

	else // Name change ok
	{
		strcopy(g_teamName[TEAM_JINRAI], sizeof(g_teamName[]), newValue);

		if (g_isLive)
		{
			LogCompetitive("Jinrai changed team name to: %s", newValue);
		}
	}
}

public void Event_TeamNameNSF(ConVar convar, const char[] oldValue, const char[] newValue)
{
	char jinraiName[sizeof(g_teamName[])];
	GetConVarString(g_hJinraiName, jinraiName, sizeof(jinraiName));

	if (StrEqual(newValue, "Jinrai", false)) // Make sure we don't use the other team's default name
	{
		SetConVarString(g_hJinraiName, oldValue);
	}

	else if (strlen(newValue) == 0 || StrEqual(newValue, oldValue)) // No cvar value, use default name "NSF"
	{
		strcopy(g_teamName[TEAM_NSF], sizeof(g_teamName[]), "NSF");
	}

	else if (StrEqual(newValue, jinraiName)) // NSF tried using Jinrai's current name
	{
		SetConVarString(g_hNSFName, oldValue);
	}

	else // Name change ok
	{
		strcopy(g_teamName[TEAM_NSF], sizeof(g_teamName[]), newValue);

		if (g_isLive)
		{
			LogCompetitive("NSF changed team name to: %s", newValue);
		}
	}
}

public void Event_SourceTVEnabled(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (StringToInt(newValue) && !IsSourceTVBotConnected())
	{
		PrintToChatAll("%s SourceTV bot doesn't appear to be connected. Global replay won't be recorded.", g_tag);
	}

	else if (!StringToInt(newValue) && g_isSourceTVRecording) // SourceTV disabled while recording a demo
		ToggleSourceTV();
}

public void Event_SourceTVPath(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (!DirExists(newValue))
		InitDirectory(newValue);
}
