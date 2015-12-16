// Double-include prevention
#if defined _base_included_
	#endinput
#endif
#define _base_included_

#define MENU_TIME 20 // How long to display panel menus for, in seconds

#define MAX_STEAMID_LENGTH 44
#define MAX_ROUNDS_PLAYED 255 // This is just a random large number used for 1d int arrays because it's cheap and simple. A single comp game should never have more rounds than this to avoid weirdness.

new String:g_tag[] = "[COMP]";
new String:g_teamName[][64] = {
	"Unassigned",
	"Spectator",
	"Jinrai",
	"NSF"
};
new String:g_livePlayers[MAXPLAYERS+1][MAX_STEAMID_LENGTH]; // Array for storing SteamIDs to allow disconnected players rejoining when live
new String:g_menuSoundOk[] = "buttons/button14.wav";
new String:g_menuSoundCancel[] = "buttons/combine_button7.wav";
new String:g_soundLive[] = "buttons/button17.wav";
new String:g_kvPath[PLATFORM_MAX_PATH];
new String:g_killInfoBuffer[MAXPLAYERS+1][512];
new String:g_convarBeingEdited[128];

new g_assignedTeamWhenLive[MAXPLAYERS+1];
new g_jinraiScore[MAX_ROUNDS_PLAYED];
new g_liveTimer = 5;
new g_liveTimer_OriginalValue;
new g_loggingEnabled;
new g_nsfScore[MAX_ROUNDS_PLAYED];
new g_pauseReason;
new g_pausingTeam;
new g_roundNumber;
new g_spamTimerCount;
new g_unpauseTimer = 10;
new g_unpauseTimer_OriginalValue;
new g_usedTimeouts[sizeof(g_teamName)];
new g_playersAlive[sizeof(g_teamName)];
new g_winner;
new g_health[MAXPLAYERS+1];
new g_damageDealt[MAXPLAYERS+1][MAXPLAYERS+1];
new g_playerDeaths[MAXPLAYERS+1][MAX_ROUNDS_PLAYED];	// Players' deaths for each round
new g_playerXP[MAXPLAYERS+1][MAX_ROUNDS_PLAYED];		// Players' XP for each round
new g_targetRound;
new g_furthestPlayedRound;
new g_penalizedTeam;

new Float:g_fRoundTime;

new bool:g_isAlltalkByDefault;
new bool:g_isExpectingOverride;
new bool:g_isExpectingUnPauseTimer;
new bool:g_isLive;
new bool:g_isPaused;
new bool:g_isReady[MAXPLAYERS+1];
new bool:g_isSourceTVRecording;
new bool:g_isSpawned[MAXPLAYERS+1];
new bool:g_isTeamReadyForUnPause[sizeof(g_teamName)]; // Unassigned, spec, jinrai, nsf
new bool:g_isWantingOverride[sizeof(g_teamName)]; // Unassigned, spec, jinrai, nsf
new bool:g_shouldPause;
new bool:g_shouldShowKillInfo[MAXPLAYERS+1];
new bool:g_isLiveCountdown;
new bool:g_isEditingSetting[MAXPLAYERS+1];
new bool:g_confirmLiveEnd;
new bool:g_isGamedataOutdated;
new bool:g_isCurrentlyUnPausing;
new bool:g_confirmRoundChange;
new bool:g_isCurrentlyRestoringRound;
new bool:g_isCurrentlyEditingClients[MAXPLAYERS+1];
new bool:g_isCurrentlyGivingPenalty[MAXPLAYERS+1];
new bool:g_survivedLastRound[MAXPLAYERS+1];

new Handle:g_hAlltalk;
new Handle:g_hCommsBehaviour;
new Handle:g_hCompetitionName;
new Handle:g_hForceCamera;
new Handle:g_hLogMode;
new Handle:g_hMatchSize;
new Handle:g_hMaxPauseLength;
new Handle:g_hMaxPauseLength_Technical;
new Handle:g_hMaxTimeouts;
new Handle:g_hNeoRestartThis;
new Handle:g_hPausable;
new Handle:g_hRoundLimit;
new Handle:g_hSourceTVEnabled;
new Handle:g_hSourceTVPath;
new Handle:g_hJinraiName;
new Handle:g_hNSFName;
new Handle:g_hKillVersobity;
new Handle:g_hClientRecording;
new Handle:g_hLimitLiveTeams;
new Handle:g_hLimitTeams;
new Handle:g_hVerbosityDelay;
new Handle:g_hPauseMode;
new Handle:g_hCollectiveReady;
new Handle:g_hPreventZanshiStrats;
new Handle:g_hRoundTime;

new Handle:g_hTimer_LiveCountdown			= INVALID_HANDLE;
new Handle:g_hTimer_GoLive						= INVALID_HANDLE;
new Handle:g_hTimer_UnPause					= INVALID_HANDLE;
new Handle:g_hTimer_UnPause_Countdown	= INVALID_HANDLE;
new Handle:g_hTimer_UnPause_HalfLeft		= INVALID_HANDLE;

Menu g_clientMenu = null;

enum {
	CVAR_NONE = 0,
	CVAR_COMPETITION_NAME,
	CVAR_JINRAI_NAME,
	CVAR_NSF_NAME,
	CVAR_MAX_ROUNDS,
	CVAR_PAUSE_AMOUNT
};

enum {
	GAMESTYLE_WARMUP = 1,
	GAMESTYLE_NORMAL
};

enum {
	PAUSEMODE_FORBIDDEN = 0,
	PAUSEMODE_NORMAL,
	PAUSEMODE_STOP_CLOCK
};

enum {
	REASON_TECHNICAL = 0,
	REASON_TIMEOUT
};

void CheckIfEveryoneIsReady()
{	
	new players = GetPlayerCount();
	new playersReady = GetPlayersReady();
	
	#if DEBUG
		PrintToServer("%s Players found total: %i", g_tag, players);
		PrintToServer("%s Players ready: %i", g_tag, playersReady);
	#endif
	
	if (players > playersReady) // Everyone isn't ready, stop here
		return;
	
	if (players < playersReady)
		LogError("More players (%i) ready than there are players total (%i).", playersReady, players);
	
	if ( players < GetConVarInt(g_hMatchSize) && GetConVarBool(g_hLimitTeams) ) // Everyone connected is ready, but there are less people than expected
	{
		g_isExpectingOverride = true;
		PrintToChatAll("%s Waiting for %i more players to join.", g_tag, (GetConVarInt(g_hMatchSize) - players));
		PrintToChatAll("To start the match without %i players, both teams must !start", GetConVarInt(g_hMatchSize));
		
		return;
	}
	
	else if ( players > GetConVarInt(g_hMatchSize) && GetConVarBool(g_hLimitTeams) ) // Everyone connected is ready, but there are more people than expected
	{
		g_isExpectingOverride = true;
		PrintToChatAll("%s There are %i more players than expected.", g_tag, (players - GetConVarInt(g_hMatchSize)));
		PrintToChatAll("To force the match start with %i players, both teams must !start", players);
		
		return;
	}
	
	LiveCountDown(); // Passed all checks, ready to go live
}

public Action:Command_ReadyList(client, args)
{
	if (!Client_IsValid(client))
		return Plugin_Stop;
	
	PrintToChat(client, "%s List of readied players is printed in your console.", g_tag);
	
	PrintToConsole(client, "\n%s Players and their READY state:", g_tag);
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i))
			continue;
		
		new team = GetClientTeam(i);
		
		if (team != TEAM_JINRAI && team != TEAM_NSF)
			continue;
		
		new String:clientName[MAX_NAME_LENGTH];
		GetClientName(i, clientName, sizeof(clientName));
		
		if (g_isReady[i])
			PrintToConsole(client, "[READY]\t%s", clientName);
		
		else
			PrintToConsole(client, "[NOT READY]\t%s", clientName);
	}
	PrintToConsole(client, "\n");
	
	return Plugin_Handled;
}

public Action:Command_IgnoreTeams(client, args)
{
	if ( GetConVarBool(g_hLimitLiveTeams) )
	{
		SetConVarBool(g_hLimitLiveTeams, false);
		ReplyToCommand(client, "Now ignoring team change protection.");
	}
	
	else
	{
		SetConVarBool(g_hLimitLiveTeams, true);
		ReplyToCommand(client, "Enforcing teams.");
	}
	
	return Plugin_Handled;
}

void ToggleRecording()
{
	if ( GetConVarBool(g_hClientRecording) == false )
		return;
	
	if (g_isLive) // Start recording
	{
		for (new i = 1; i <= MaxClients; i++)
		{
			if (!Client_IsValid(i) || IsFakeClient(i))
				continue;
			
			new String:steamID[MAX_STEAMID_LENGTH];
			GetClientAuthId(i, AuthId_Steam2, steamID, sizeof(steamID));
			
			new String:timestamp[15];
			FormatTime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M");
			
			new String:competitionName[32];
			GetConVarString(g_hCompetitionName, competitionName, sizeof(competitionName));
			
			new String:replayName[sizeof(steamID)+sizeof(timestamp)+sizeof(competitionName)+2];
			
			if (strlen(competitionName) > 0)
				Format(replayName, sizeof(replayName), "%s_%s_%s", competitionName, timestamp, steamID);
			
			else
				Format(replayName, sizeof(replayName), "%s_%s", timestamp, steamID);
			
			// Clean up any non alphanumeric characters from the string
			new String:replayBuffer[sizeof(replayName) + 1];
			new bufferPos;
			
			for (new j = 0; j < sizeof(replayName); j++)
			{
				if (IsCharAlpha(replayName[j]) || IsCharNumeric(replayName[j]) || !IsCharSpace(replayName[j]))
					replayBuffer[bufferPos++] = replayName[j];
			}
			replayBuffer[bufferPos] = '\0'; // Terminate the string with 0
			
			ReplaceString(replayBuffer, sizeof(replayBuffer), ":", ""); // Remove : characters from the SteamID. To-do: proper sanitization
			strcopy(replayName, sizeof(replayName), replayBuffer);
			
			ClientCommand(i, "stop"); // Stop any previous recording, jic. This does nothing if a demo isn't running.
			ClientCommand(i, "record %s", replayName); // Start recording. Format demo filename here. Note: replays can be overwritten using this.
			
			#if DEBUG > 1
				ClientCommand(i, "say Debug message: This client is now recording to %s", replayName);
			#endif
		}
	}
	
	else // Stop recording
	{		
		for (new i = 1; i <= MaxClients; i++)
		{
			if (!Client_IsValid(i) || IsFakeClient(i))
				continue;
			
			ClientCommand(i, "stop");
			
			#if DEBUG > 1
				ClientCommand(i, "say Debug message: This client has stopped recording");
			#endif
		}
	}
}

void LiveCountDown()
{
	if (g_isLive)
	{
		LogError("Attempted LiveCountDown while game is already marked live!");
		return;
	}
	
	if (g_isLiveCountdown)
	{
		LogError("Attempted LiveCountDown while one is already happening!");
		return;
	}
	
	g_isLiveCountdown = true;
	
	g_hTimer_LiveCountdown = CreateTimer(1.0, Timer_Live_SecondsCountdown, _, TIMER_REPEAT); // This repeat timer is killed inside the callback
	g_hTimer_GoLive = CreateTimer((float(g_liveTimer) + 1.0), Timer_GoLive);
}

public Action:Timer_GoLive(Handle:timer)
{
	EmptyScore(); // Make sure everyone starts private so we avoid the no weapon spawn bug
	CreateTimer(0.1, Timer_ToggleLive); // A slight delay so the score reset above takes effect in time
}

public Action:Timer_ToggleLive(Handle:timer)
{
	ToggleLive();
}

public Action:Timer_CancelLiveEndConfirmation(Handle:timer)
{
	g_confirmLiveEnd = false;
}

void ToggleLive()
{
	g_isLive = !g_isLive;
	
	if (g_isLive)
	{
		g_isLiveCountdown = false;
		g_winner = 0; // Reset winner (overlay module queries this value)
		
		#if DEBUG
			PrintToServer("Creating SteamID array of players...");
			new foundPlayers;
		#endif
		
		// Save connected players' SteamIDs so they can be allowed to rejoin their teams if disconnected
		for (new i = 1; i <= MaxClients; i++)
		{
			if (!Client_IsValid(i) || IsFakeClient(i) ||!IsClientInGame(i))
				continue;
			
			if (!IsClientAuthorized(i))
			{
				new String:clientName[MAX_NAME_LENGTH];
				GetClientName(i, clientName, sizeof(clientName));
				PrintToChatAll("%s Player \"%s\" has no SteamID authorized yet! Stopped going live. Please try again in a moment.");
				#if DEBUG
					PrintToServer("%s Player \"%s\" has no SteamID authorized yet! Stopped going live. Please try again in a moment.");
				#endif
				
				g_isLive = false;
				
				return;
			}
			
			new String:steamID[MAX_STEAMID_LENGTH];
			GetClientAuthId(i, AuthId_Steam2, steamID, sizeof(steamID));
			PrintToServer("SteamID of %i: %s", i, steamID);
			new team = GetClientTeam(i);
			
			if (strlen(steamID) == 0)
			{
				LogError("Client %i returned empty SteamID, probably not authorized whilst attempting to save SteamID into g_livePlayers!", i);
				continue;
			}
			
			else
			{
				strcopy(g_livePlayers[i], sizeof(g_livePlayers[]), steamID);
				PrintToServer("sizeof g_livePlayers: %i", sizeof(g_livePlayers[]));
				g_assignedTeamWhenLive[i] = team;
			}
			
#if DEBUG > 1
				if (strlen(steamID) > 0)
				{
					PrintToServer("Found SteamID: %s", steamID);
					PrintToServer("Saved to array as: %s, team %s", g_livePlayers[i], g_teamName[g_assignedTeamWhenLive[i]]);
					foundPlayers++;
				}
#endif
		}
		
#if DEBUG
			PrintToServer("Completed SteamID array with %i results.", foundPlayers);
#endif
		
#if DEBUG == 0
		// This should only occur in debug test cases
		if (foundPlayers < 2)
			LogError("Less than 2 players found!");
#endif
		
		// Set convars for competitive
		SetConVarInt(g_hAlltalk, GetDesiredCommsBehaviour());
		SetConVarBool(g_hPausable, false);
		SetConVarBool(g_hForceCamera, true);
		
		NeoRestartThis();
		
		g_loggingEnabled = GetConVarBool(g_hLogMode);
		
		ToggleRecording();
		
		// Announce game start, possible tournament name, and teams
		decl String:competitionName[32];
		GetConVarString(g_hCompetitionName, competitionName, sizeof(competitionName));
		
		// If competition name hasn't been specified
		if (strlen(competitionName) < 1)
			strcopy(competitionName, sizeof(competitionName), "Competitive Neotokyo");
		
		decl String:liveTitle[sizeof(g_tag) + sizeof(competitionName) + sizeof(g_teamName[]) * 2 + 6];
		Format(liveTitle, sizeof(liveTitle), "%s %s: %s - %s", g_tag, competitionName, g_teamName[TEAM_JINRAI], g_teamName[TEAM_NSF]);
		
		PrintToChatAll(liveTitle);
		
		/* Keyvalus test, this isn't implemented at the moment.
		/
		#if DEBUG
			// Create match keyvalues (to-do: load previous game after cancel/servercrash/etc)
			// We could later use md5 (smlib) hash for id inside kv meta
			new Handle:kv = CreateKeyValues("competitive_match", "match_id", "debugID1"); // name, firstKey, firstValue
			
			KvGotoFirstSubKey(kv);
			KvSetNum(kv, "round", 1);
			
			KvSetSectionName(kv, "testing");
			KvSetNum(kv, "anotherVal", 5);
			
			decl String:timestamp[15];
			FormatTime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M");
			
			decl String:kvFilePath[PLATFORM_MAX_PATH] = "matchDebug.kv";
			Format(kvFilePath, sizeof(kvFilePath), "%s/%s_%s", g_kvPath, timestamp, kvFilePath);
			
			KeyValuesToFile(kv, kvFilePath);
			CloseHandle(kv);
		#endif
		// End kv debug
		*/
		
		SpamTimer();
	}
	
	// Stop live
	else
	{
		if (g_isPaused)
			TogglePause();
		
		ToggleRecording();
		
		SetConVarBool(g_hForceCamera, false);
		
		ResetGlobalVariables();
		
		PrintToChatAll("%s Match has ended.", g_tag);
	}
	
	ToggleSourceTV(); // This has to be called after modifying the g_isLive boolean
}

void NeoRestartThis()
{
	SetConVarBool(g_hNeoRestartThis, true);
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i))
			continue;
		
		SetPlayerRank(i, 0);
	}
}

void TogglePause()
{	
	// No pausing allowed. Todo: Do this check in !pause instead, and just error check here instead
	if ( GetConVarInt(g_hPauseMode) == PAUSEMODE_FORBIDDEN )
		return;
	
	g_isPaused = !g_isPaused; // Flip pause bool
	
	switch ( GetConVarInt(g_hPauseMode) )
	{
		// Normal Source engine pausing
		case PAUSEMODE_NORMAL:
		{
			// Pause
			if (g_isPaused)
			{
				if (!g_isLive)
				{
					LogError("Attempted to pause while match isn't live.");
					return;
				}
				
				g_isExpectingUnPauseTimer = true;
				
				// Close any previous unpause timer handles
				KillTimers_UnPause();
				
				new Float:fPauseLength;
				
				// Pause length for technical issues
				if (g_pauseReason == REASON_TECHNICAL)
				{
					fPauseLength = GetConVarFloat(g_hMaxPauseLength_Technical);
				}
				// Pause length for tactical time-out
				else
				{
					fPauseLength = GetConVarFloat(g_hMaxPauseLength);
				}
				
				PrintToChatAll( "%s Pause maximum duration: %i seconds.", g_tag, RoundToNearest(fPauseLength) );
				PrintToChatAll("%s Use !unpause when ready.", g_tag);
				
				#if DEBUG > 1
					PrintToServer( "%s Pause max duration: %i seconds.", g_tag, RoundToNearest(fPauseLength) );
				#endif
				
				g_hTimer_UnPause = CreateTimer( (1.0 + fPauseLength ), Timer_Unpause ); // Add 1 second to timer for messages to appear in correct order during countdown
						
				if ( (fPauseLength / 2) > 10.0 ) // Don't bother with "half of pause time remaining" notice when there's less than 10 secs remaining
					g_hTimer_UnPause_HalfLeft = CreateTimer( (fPauseLength / 2), Timer_UnpauseNotice_Half );
				
				g_hTimer_UnPause_Countdown = CreateTimer( (fPauseLength - float(g_unpauseTimer) ), Timer_StartTenSecondsLeft );
			}
			
			// Unpause
			else
			{
				if (!g_isExpectingUnPauseTimer)
					LogError("Unpause timer executed unexpectedly.");
				
				g_isExpectingUnPauseTimer = false;
				
				if (g_isLive) // Only announce the unpause if a match is still going
					SpamTimer();
				
				g_isTeamReadyForUnPause[TEAM_JINRAI] = false;
				g_isTeamReadyForUnPause[TEAM_NSF] = false;
			}
			
			SetConVarBool(g_hPausable, true);
	
			CreateTimer(0.5, Timer_TogglePauseLoop);
		}
		
		// Stop game's round timer instead of a regular pause. This should be less glitchy overall.
		case PAUSEMODE_STOP_CLOCK:
		{	
			// Server's Neotokyo gamedata is outdated, GameRules_SetPropFloat probably wouldn't work.
			// We have to switch pause modes to avoid running into errors.
			if (g_isGamedataOutdated)
			{
				SetConVarInt(g_hPauseMode, PAUSEMODE_NORMAL); // Use the in-engine "normal" pause instead of PAUSEMODE_STOP_CLOCK
				
				g_isPaused = !g_isPaused; // Flip the bool again, because...
				TogglePause(); // ... we're going to call ourselves to land on the right pause mode! There's probably more elegant ways to achieve this, but oh well.
				return; // Stop here
			}
			
			// Pause
			if (g_isPaused)
			{	
				g_isExpectingUnPauseTimer = true;
				
				GameRules_SetPropFloat( "m_fRoundTimeLeft", 0.0 ); // Set round time to zero. This instantly ends current round and rings the tie/win music, which works to alert players about upcoming pause.
				
				new Float:transitionTime = 5.0;
				new Float:fPauseLength;
				
				// Pause length for technical issues
				if (g_pauseReason == REASON_TECHNICAL)
				{
					fPauseLength = GetConVarFloat(g_hMaxPauseLength_Technical);
				}
				// Pause length for tactical time-out
				else
				{
					fPauseLength = GetConVarFloat(g_hMaxPauseLength);
				}
				
				// Wait slightly before initiating pause, so players have time to read chat info about what's going on
				CreateTimer(transitionTime, Timer_EnterPauseRound); 
				
				// Close any previous unpause timer handles
				KillTimers_UnPause();
				
				// Create the unpause timer
				g_hTimer_UnPause = CreateTimer( (transitionTime + fPauseLength ), Timer_Unpause );
				
				PrintToChatAll( "%s Pause maximum duration: %i seconds.", g_tag, RoundToNearest(fPauseLength) );
				PrintToChatAll("%s Use !unpause when ready.", g_tag);
			}
			
			// Unpause
			else
			{
				if (!g_isExpectingUnPauseTimer)
					LogError("Unpause timer executed unexpectedly.");
				
				g_isExpectingUnPauseTimer = false;
				g_isCurrentlyUnPausing = true;
				
				GameRules_SetPropFloat( "m_fRoundTimeLeft", 0.0 ); // Stop the pause round
				GameRules_SetProp("m_iGameState", GAMESTYLE_NORMAL); // Back to action
				
				RestoreRound(g_roundNumber); // Restore everyone's XP, deaths and ranks to the state before pausing
				
				g_isTeamReadyForUnPause[TEAM_JINRAI] = false;
				g_isTeamReadyForUnPause[TEAM_NSF] = false;
			}
		}
	}
}

public Action:Timer_NeoRestartThis(Handle:timer)
{
	ServerCommand("neo_restart_this 1");
}

void KillTimers_UnPause()
{
	if (g_hTimer_UnPause != INVALID_HANDLE)
	{
		CloseHandle(g_hTimer_UnPause);
		g_hTimer_UnPause = INVALID_HANDLE;
	}
	
	if (g_hTimer_UnPause_Countdown != INVALID_HANDLE)
	{
		CloseHandle(g_hTimer_UnPause_Countdown);
		g_hTimer_UnPause_Countdown = INVALID_HANDLE;
	}
	
	if (g_hTimer_UnPause_HalfLeft != INVALID_HANDLE)
	{
		CloseHandle(g_hTimer_UnPause_HalfLeft);
		g_hTimer_UnPause_HalfLeft = INVALID_HANDLE;
	}
}

public Action:Timer_EnterPauseRound(Handle:timer)
{
	GameRules_SetProp("m_iGameState", GAMESTYLE_WARMUP);
	
	new Float:pauseLength;
	if (g_pauseReason == REASON_TECHNICAL)
		pauseLength = GetConVarFloat(g_hMaxPauseLength_Technical);
	else
		pauseLength = GetConVarFloat(g_hMaxPauseLength);
	
	GameRules_SetPropFloat( "m_fRoundTimeLeft", pauseLength ); // Pause round uses max pause length roundtime
	
	for (new i = 0; i < 3; i++)
	{
		PrintToChatAll("GAME IS PAUSED");
	}
}

#if DEBUG
public Action:Command_UnpauseOther(client, args) // Pretend the other team made an unpause request. Debug command.
{
	if (client == 0)
	{
		ReplyToCommand(client, "%s This command can't be executed by the server.", g_tag);
		return Plugin_Stop;
	}
	
	new bool:foundSomeoneInOtherTeam;
	new team = GetClientTeam(client);
	new otherTeam = GetOtherTeam(team);
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i))
			continue;
		
		new forLoopTeam = GetClientTeam(i);
		if (forLoopTeam == otherTeam)
		{
			foundSomeoneInOtherTeam = true;
			UnPauseRequest(i); // Make someone from the other team perform an unpause request
			break;
		}
	}
	
	if (!foundSomeoneInOtherTeam)
		ReplyToCommand(client, "%s Debug: Could not find any players in the other team to fake unpause.", g_tag);
	else
		ReplyToCommand(client, "%s Debug: Faked unpause request from the other team.", g_tag);
	
	return Plugin_Handled;
}

public Action:Command_OverrideStartOther(client, args) // Pretend the other team made a force start request. Debug command.
{
	new bool:foundSomeoneInOtherTeam;
	new team = GetClientTeam(client);
	new otherTeam = GetOtherTeam(team);
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i))
			continue;
		
		new forLoopTeam = GetClientTeam(i);
		if (forLoopTeam == otherTeam)
		{
			foundSomeoneInOtherTeam = true;
			Command_OverrideStart(i, 0); // Make someone from the other team perform a force start request. Args (0) not used, but the command expects them.
			break;
		}
	}
	
	if (!foundSomeoneInOtherTeam)
		ReplyToCommand(client, "%s Debug: Could not find any players in the other team to fake force start.", g_tag);
	else
		ReplyToCommand(client, "%s Debug: Faked force start request from the other team.", g_tag);
	
	return Plugin_Handled;
}
#endif

public Action:Timer_TogglePauseLoop(Handle:timer)
{
	TogglePauseLoop();
	
	return Plugin_Stop;
}

void TogglePauseLoop()
{
	new invalidClients;
	new unspawnedClients[2];
	new bool:forcePause;
	
	for (new i = 1; i <= MaxClients; i++) // The server can't exec "pause" for some reason. So, instead, use the first valid client to execute it.
	{
		unspawnedClients[0] = unspawnedClients[1];
		if (Client_IsValid(i))
		{
			if (IsFakeClient(i))
			{
				invalidClients++;
				continue;
			}
			
			if (!g_isSpawned[i])
				unspawnedClients[1]++;
			
			if (forcePause || unspawnedClients[1] <= unspawnedClients[0])
			{				
				ClientCommand(i, "pause");
				
				#if DEBUG > 1
					if (forcePause)
						PrintToServer("Force pause detected.");
					
					new String:clientName[MAX_NAME_LENGTH];
					GetClientName(i, clientName, sizeof(clientName));
					PrintToServer("Found client to toggle pause. Client int: %i, name: %s", i, clientName);
				#endif
				
				CreateTimer(0.05, Timer_DisablePause); // Pausable cvar toggling is too fast to register the pause without a tiny delay here. 0.01 was occasionally too fast, 0.05 seems pretty safe.
				
				break;
			}
		}
		
		else
			invalidClients++;
		
		if (i == MaxClients && unspawnedClients[1] > 0) // This will keep looping in 1 second intervals for as long as there are valid unspawned clients
		{
			#if DEBUG > 1
					PrintToServer("Found %i invalid clients. Found %i valid unspawned clients.", invalidClients, unspawnedClients[1]);
			#endif
			
			if (GetGameTime() > g_fRoundTime + 13.9)
			{
				forcePause = true;
				i = 0; // Reset loop
				
				#if DEBUG > 1
					PrintToServer("New round is about to start! Force any valid client to toggle pause...");
				#endif
				
				continue;
			}
			
			CreateTimer(1.0, Timer_TogglePauseLoop);
			
			#if DEBUG > 1
				PrintToServer("No valid spawned client found. Restarting loop in TogglePauseLoop");
			#endif
		}
		
		else if (invalidClients == MaxClients)
			LogError("Attempted pause command whilst no valid clients were connected inside TogglePause. This shouldn't happen.");
	}
}

public Action:Timer_TogglePause(Handle:timer)
{
	TogglePause();
}

public Action:Timer_DisablePause(Handle:timer)
{
	SetConVarBool(g_hPausable, false);
}

public Action:Timer_SpamLive(Handle:timer)
{
	// Print the LIVE message X times, then kill the repeat timer calling this
	new spamLiveTimes = 3;
	g_spamTimerCount++;
	
	if (g_spamTimerCount == 1) // Don't spam "LIVE" during the first increment. This has to do with chat message ordering (round number msgs) during match start, in Event_RoundStart.
		return Plugin_Continue;
	
	else if (g_spamTimerCount <= (spamLiveTimes + 1)) // spamLiveTimes+1, because we won't trigger during first increment (see comment above)
		PrintToChatAll("%s LIVE", g_tag);
	
	else
	{
		g_spamTimerCount = 0;
		return Plugin_Stop;
	}
	
	return Plugin_Continue;
}

void SpamTimer()
{
	CreateTimer(0.25, Timer_SpamLive, _, TIMER_REPEAT); // This repeat timer is killed inside the callback after X repeats
}

public Action:Timer_Unpause(Handle:timer)
{
	if (g_isPaused)
	{
		PrintToChatAll("%s Maximum pause time has ended!", g_tag);
		TogglePause();
	}
	
	g_hTimer_UnPause = INVALID_HANDLE;
	
	return Plugin_Handled;
}

public Action:Timer_UnpauseNotice_Half(Handle:timer)
{
	// Show notification when half of the max pause time has elapsed
	if (g_isPaused)
	{
		PrintToChatAll("%s Pause time left: %i seconds", g_tag, (RoundToNearest(GetConVarFloat(g_hMaxPauseLength)) / 2));
		return Plugin_Continue;
	}
	
	g_hTimer_UnPause_HalfLeft = INVALID_HANDLE;
	
	return Plugin_Handled;
}

public Action:Timer_StartTenSecondsLeft(Handle:timer)
{
	if (g_isPaused)
		CreateTimer(1.0, Timer_Unpause_SecondsCountdown, _, TIMER_REPEAT); // This repeat timer is killed inside the callback
	
	g_hTimer_UnPause_Countdown = INVALID_HANDLE;
	
	return Plugin_Handled;
}

public Action:Timer_Live_SecondsCountdown(Handle:timer)
{
	if (g_liveTimer > 0)
	{
		PrintToChatAll("%s Match starting in: %i", g_tag, g_liveTimer);
		g_liveTimer--;
		
		PlayLiveBeep();
		
		return Plugin_Continue;
	}
	
	g_liveTimer = g_liveTimer_OriginalValue; // Reset the timer to its original value
	
	return Plugin_Stop;
}

public Action:Timer_Unpause_SecondsCountdown(Handle:timer)
{
	if (!g_isLive || !g_isPaused) // Pause has properly ended or the match is not live anymore (which implies the pause was properly ended already)
		return Plugin_Stop;
	
	if (g_unpauseTimer > 0)
	{
		//PlayLiveBeep(); Does not work whilst paused
		PrintToChatAll("%s Unpausing in: %i", g_tag, g_unpauseTimer);
		g_unpauseTimer--;
		
		return Plugin_Continue;
	}
	
	g_unpauseTimer = g_unpauseTimer_OriginalValue; // Reset the timer to its original value
	
	return Plugin_Stop;
}

void InitDirectory(const String:sDir[])
{
	decl String:sPieces[32][PLATFORM_MAX_PATH];
	new String:sPath[PLATFORM_MAX_PATH];
	new iNumPieces = ExplodeString(sDir, "/", sPieces, sizeof(sPieces), sizeof(sPieces[]));
	
	for(new i = 0; i < iNumPieces; i++)
	{
		Format(sPath, sizeof(sPath), "%s/%s", sPath, sPieces[i]);
		if(!DirExists(sPath))
			CreateDirectory(sPath, 509); // Note that folders must have the execute bit set on Linux. On Windows, the mode is ignored.
	}
}

// todo: check for redundant code in this
void ToggleSourceTV()
{
	if (g_isSourceTVRecording)
	{
		ServerCommand("tv_stoprecord");
		g_isSourceTVRecording = false;
		
		#if DEBUG
			PrintToServer("ToggleSourceTV: tv_stoprecord");
		#endif
	}
	
	else
	{
		if (!GetConVarBool(g_hSourceTVEnabled) || !g_isLive)
			return;
		
		if (!IsSourceTVBotConnected())
		{
			#if DEBUG
				PrintToServer("ToggleSourceTV: SourceTV bot not found.");
			#endif
			
			return;
		}
		
		decl String:demoPath[PLATFORM_MAX_PATH];
		GetConVarString(g_hSourceTVPath, demoPath, sizeof(demoPath));
		
		decl String:timestamp[16];
		FormatTime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M");
		
		decl String:mapName[32];
		GetCurrentMap(mapName, sizeof(mapName));
		
		decl String:competitionName[32];
		GetConVarString(g_hCompetitionName, competitionName, sizeof(competitionName));
		
		decl String:replayName[sizeof(mapName)+sizeof(timestamp)+sizeof(competitionName)+2];
		
		if (strlen(competitionName) > 0)
			Format(replayName, sizeof(replayName), "%s_%s_%s", competitionName, timestamp, mapName);
		
		else
			Format(replayName, sizeof(replayName), "%s_%s", timestamp, mapName);
		
		// Clean up any non alphanumeric characters from the string
		new String:replayBuffer[sizeof(replayName) + 1];
		new bufferPos;
		
		for (new j = 0; j < sizeof(replayName); j++)
		{
			if (IsCharAlpha(replayName[j]) || IsCharNumeric(replayName[j]) || !IsCharSpace(replayName[j]))
				replayBuffer[bufferPos++] = replayName[j];
		}
		replayBuffer[bufferPos] = '\0'; // Terminate the string with 0
		
		ReplaceString(replayBuffer, sizeof(replayBuffer), ":", ""); // Remove : characters from the SteamID. To-do: proper sanitization
		strcopy(replayName, sizeof(replayName), replayBuffer);
		
		ServerCommand("tv_stoprecord"); // Stop any previous server record
		ServerCommand("tv_record \"%s/%s\"", demoPath, replayName); // SourceTV bot needs to be connected for this to do anything. This is checked above with IsSourceTVBotConnected()
		g_isSourceTVRecording = true;
		
		#if DEBUG
			PrintToServer("ToggleSourceTV: tv_record \"%s/%s\"", demoPath, replayName);
		#endif
	}
}

// This won't play a sound if used during an engine pause
void PlayLiveBeep()
{
	new Float:volume	= 0.5;	// Volume between 0.0 - 1.0 (original volume is 1.0)
	new pitch				= 175;	// Pitch between 0 - 255 (original pitch is 100)
	
	PrecacheSound(g_soundLive);
	EmitSoundToAll(g_soundLive, _, _, _, _, volume, pitch);
}


bool:IsSourceTVBotConnected()
{
	// Get the SourceTV bot's name cvar, and figure out if bot is connected
	new Handle:hBotName = FindConVar("tv_name");
	
	decl String:botName[MAX_NAME_LENGTH];
	GetConVarString(hBotName, botName, sizeof(botName));
	
	CloseHandle(hBotName);
	
	#if DEBUG > 1
		PrintToServer("Looking for a SourceTV bot...");
	#endif
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i) || !IsFakeClient(i)) // We are looking for a valid fake client (bot)
			continue;
		
		decl String:clientName[MAX_NAME_LENGTH];
		GetClientName(i, clientName, sizeof(clientName));
		
		#if DEBUG > 1
			PrintToServer("Found valid fake client, index: %i, name: %s", i, clientName);
		#endif
		
		if (StrEqual(clientName, botName) || StrEqual(clientName, "SourceTV")) // Also check for default name "SourceTV", because sometimes tv_name doesn't work when bot first spawns
		{			
			#if DEBUG > 1
				PrintToServer("Fake client name %s matches tv_name cvar %s or \"SourceTV\"", clientName, botName);
			#endif
			
			return true;
		}
	}
	
	return false;
}

// Return players in Jinrai + NSF total
int GetPlayerCount()
{
	new players;
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || IsFakeClient(i) )
			continue;
		
		new team = GetClientTeam(i);
		
		if (team != TEAM_JINRAI && team != TEAM_NSF)
			continue;
		
		players++;
	}
	
	return players;
}

int GetPlayersReady()
{
	new playersReady;

	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !g_isReady[i] || IsFakeClient(i) )
			continue;
			
		new team = GetClientTeam(i);
		
		if (team != TEAM_JINRAI && team != TEAM_NSF)
			continue;
		
		playersReady++;
	}
	
	return playersReady;
}

int GetDesiredCommsBehaviour()
{
	new behaviour = GetConVarInt(g_hCommsBehaviour); // 0: disable alltalk, 1: enable alltalk, 2: use whichever initial alltalk value (tracked with g_isAlltalkByDefault)
	
	if (behaviour <= 1)
		return behaviour;
	
	else
		return g_isAlltalkByDefault;
}

int GetOtherTeam(team)
{	
	if (team == TEAM_JINRAI)
		return TEAM_NSF;
	
	else if (team == TEAM_NSF)
		return TEAM_JINRAI;
	
	else
	{
		LogError("Requested other team for invalid team (%i), returning -1 instead.");
		return -1;
	}
}

void LogCompetitive(const String:message[])
{
	if (!g_loggingEnabled)
		return;
	
	decl String:loggingPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, loggingPath, sizeof(loggingPath), "logs/competitive");
	
	new String:debugFile[] = "logfile.log"; // Static debug file for logging. We later want to use keyvalues using match IDs etc
	Format(loggingPath, sizeof(loggingPath), "%s/%s", loggingPath, debugFile);
		
	new Handle:hFile = OpenFile(loggingPath, "a"); // C++ fopen modes
	
	WriteFileLine(hFile, message);
	
	CloseHandle(hFile);
}

void PrintToConsoleAll(const String:message[])
{
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !IsClientInGame(i) || IsFakeClient(i) )
			continue;
		
		PrintToConsole(i, message);
	}
}

void UpdatePlayersAlive() // Updates g_playersAlive array
{
	new jinraiAlive;
	new nsfAlive;
	
	for (new i = 1; i <= MaxClients; i++)
	{
		#if !defined DEBUG || DEBUG == 0
			if (IsFakeClient(i))
				continue;
		#endif
		
		if (!Client_IsValid(i) || !IsClientInGame(i))
			continue;
		
		if (!IsPlayerAlive(i))
			continue;
		
		new team = GetClientTeam(i);
		
		if (team == TEAM_JINRAI)
			jinraiAlive++;
		
		else if (team == TEAM_NSF)
			nsfAlive++;
	}
	
	g_playersAlive[TEAM_JINRAI] = jinraiAlive;
	g_playersAlive[TEAM_NSF] = nsfAlive;
}

void ResetPlayersAlive() // Reset g_playersAlive array. Only call this on new round start (otherwise we have to do a spawn check to avoid marking unspawned players as dead).
{
	new jinraiPlayers;
	new nsfPlayers;
	
	for (new i = 1; i <= MaxClients; i++)
	{
		#if !defined DEBUG || DEBUG == 0
			if (IsFakeClient(i))
				continue;
		#endif
		
		if (!Client_IsValid(i))
			continue;
		
		new team = GetClientTeam(i);
		
		if (team == TEAM_JINRAI)
			jinraiPlayers++;
		
		else if (team == TEAM_NSF)
			nsfPlayers++;
	}
	
	// We only call this in round start, so it's safe to assume any team players are alive. 
	g_playersAlive[TEAM_JINRAI] = jinraiPlayers;
	g_playersAlive[TEAM_NSF] = nsfPlayers;
}

void ResetGlobalVariables() // Make sure all global variables are reset properly
{
	new i;
	
	// Reset global integers
	new correctInt = 0; // Desired value
	
	g_roundNumber = correctInt;
	g_furthestPlayedRound = correctInt;
	
	for (i = 0; i < MAX_ROUNDS_PLAYED; i++)
	{
		g_jinraiScore[i]	= correctInt;
		g_nsfScore[i]		= correctInt;
	}
	
	for (i = 0; i < sizeof(g_usedTimeouts); i++)
	{
		g_usedTimeouts[i] = correctInt;
	}
	
	correctInt = -1;
	
	for (i = 0; i < sizeof(g_assignedTeamWhenLive); i++)
	{	
		g_assignedTeamWhenLive[i] = correctInt;
	}
	
	// Reset global strings
	for (i = 0; i < sizeof(g_livePlayers); i++)
	{
		strcopy(g_livePlayers[i], sizeof(g_livePlayers[]), "");
	}
	
	for (i = 0; i <= MaxClients; i++)
	{
		strcopy(g_killInfoBuffer[i], sizeof(g_killInfoBuffer), "");
	}
	
	// Reset global bools
	new bool:correctBool = false; // Desired value
	
	g_isExpectingOverride		= correctBool;
	g_isExpectingUnPauseTimer	= correctBool;
	g_isLive					= correctBool;
	g_isPaused					= correctBool;
	g_isSourceTVRecording		= correctBool;
	g_shouldPause				= correctBool;
	g_confirmLiveEnd			= correctBool;
	g_confirmRoundChange	= correctBool;
	
	for (i = 0; i <= MaxClients; i++)
	{
		g_isReady[i]	= correctBool;
		g_isSpawned[i]	= correctBool;
		g_shouldShowKillInfo[i]	= correctBool;
	}
	
	for (i = 0; i < sizeof(g_teamName); i++)
	{
		g_isTeamReadyForUnPause[i]	= correctBool;
		g_isWantingOverride[i]		= correctBool;
	}
}

void ShowKillInfoNextRound(client, const String:message[])
{
	if ( !Client_IsValid(client) )
		return;
	
	strcopy(g_killInfoBuffer[client], sizeof(g_killInfoBuffer), message);
	g_shouldShowKillInfo[client] = true;
}

void RefereeMenu_GameInfo(client)
{
	if ( !Client_IsValid(client) )
		return;
	
	new Handle:panel = CreatePanel();
	
	SetPanelTitle(panel, "Game Information");
	DrawPanelText(panel, " ");
	
	// Show the live status
	if (g_isLiveCountdown)
		DrawPanelText(panel, "Live: COUNTDOWN...");
	else if (g_isLive)
		DrawPanelText(panel, "Live: LIVE");
	else
		DrawPanelText(panel, "Live: NOT LIVE");
	
	// Show the current competition name
	decl String:competitionName_Cvar[64];
	GetConVarString( g_hCompetitionName, competitionName_Cvar, sizeof(competitionName_Cvar) );
	
	if (strlen(competitionName_Cvar) < 1) // If no competition name set, use the generic "Competitive Neotokyo" name instead
		strcopy(competitionName_Cvar, sizeof(competitionName_Cvar), "Competitive Neotokyo");
	
	decl String:competitionName[128];
	Format( competitionName, sizeof(competitionName), "Competition name: %s", competitionName_Cvar );
	DrawPanelItem(panel, competitionName);
	
	// Show the current Jinrai name
	decl String:jinraiName_Cvar[64];
	GetConVarString( g_hJinraiName, jinraiName_Cvar, sizeof(jinraiName_Cvar) );
	
	if (strlen(jinraiName_Cvar) < 1) // Use default Jinrai name "Jinrai" if none is set
		strcopy(jinraiName_Cvar, sizeof(jinraiName_Cvar), "Jinrai");
	
	decl String:jinraiName[128];
	Format(jinraiName, sizeof(jinraiName), "Jinrai name: %s", jinraiName_Cvar);
	DrawPanelItem(panel, jinraiName);
	
	// Show the current NSF name
	decl String:nsfName_Cvar[64];
	GetConVarString( g_hNSFName, nsfName_Cvar, sizeof(nsfName_Cvar) );
	
	if (strlen(nsfName_Cvar) < 1) // Use default NSF name "NSF" if none is set
		strcopy(nsfName_Cvar, sizeof(nsfName_Cvar), "NSF");
	
	decl String:nsfName[128];
	Format(nsfName, sizeof(nsfName), "NSF name: %s", nsfName_Cvar);
	DrawPanelItem(panel, nsfName);
	
	// Show how many rounds are played
	decl String:boRounds[128];
	Format( boRounds, sizeof(boRounds), "Best of %i rounds", GetConVarInt(g_hRoundLimit) );
	DrawPanelItem(panel, boRounds);
	
	// Show how many (time-out) pauses are allowed per team per map
	decl String:pauseAmount[128];
	Format( pauseAmount, sizeof(pauseAmount), "Pausing: %i time-outs allowed per team per map.", GetConVarInt(g_hMaxTimeouts) );
	DrawPanelItem(panel, pauseAmount);
	
	// Go back to previous menu
	DrawPanelItem(panel, "Back");
	
	SendPanelToClient(panel, client, PanelHandler_RefereeMenu_GameInfo, MENU_TIME);
	
	CloseHandle(panel);
}

void RefereeMenu_ClientActions(client)
{	
	if (g_isCurrentlyEditingClients[client])
	{
		g_isCurrentlyEditingClients[client] = false;
		LogError("g_isCurrentlyEditingClients was already enabled for client %i before accessing RefereeMenu_ClientActions. This should never happen.", client);
	}
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !Client_IsAdmin(i) )
			continue;
		
		if (g_isCurrentlyEditingClients[i])
		{
			decl String:clientName[MAX_NAME_LENGTH];
			GetClientName( i, clientName, sizeof(clientName) );
			PrintToChat(client, "Admin %s is currently using the menu. Please wait.", clientName);
			
			return;
		}
	}
	
	g_isCurrentlyEditingClients[client] = true;
	
	if (g_clientMenu != INVALID_HANDLE)
	{
		delete(g_clientMenu);
		g_clientMenu = null;
	}
	
	g_clientMenu = BuildClientMenu();
	
	g_clientMenu.Display(client, MENU_TIME_FOREVER);
}

void RefereeMenu_Penalties(client)
{
	if (!g_isLive)
	{
		PrintToChat(client, "Game is not live, cannot give penalties.");
		Command_RefereeMenu(client, 1);
		return;
	}
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !Client_IsAdmin(i) )
			continue;
		
		if (g_isCurrentlyGivingPenalty[client])
		{
			g_isCurrentlyGivingPenalty[client] = false;
			LogError("g_isCurrentlyGivingPenalty was already enabled for client %i before accessing RefereeMenu_Penalties. This should never happen.", client);
		}
		
		if (g_isCurrentlyGivingPenalty[i])
		{
			decl String:clientName[MAX_NAME_LENGTH];
			GetClientName( i, clientName, sizeof(clientName) );
			PrintToChat(client, "Admin %s is currently using the penalty menu. Please wait.", clientName);
			
			return;
		}
	}
	
	g_isCurrentlyGivingPenalty[client] = true;
	
	new Handle:panel = CreatePanel();
	
	SetPanelTitle(panel, "Penalty Menu");
	
	DrawPanelItem(panel, "Jinrai");
	DrawPanelItem(panel, "NSF");
	
	SendPanelToClient(panel, client, PanelHandler_PenaltyMenu, MENU_TIME);
	
	CloseHandle(panel);
}

void RefereeMenu_ChangeRound(client)
{
	if (!g_isLive)
	{
		PrintToChat(client, "Game is not live, cannot change rounds.");
		Command_RefereeMenu(client, 1);
		return;
	}
	
	if (g_isEditingSetting[client])
	{
		g_isEditingSetting[client] = false;
		LogError("g_isEditingSetting was already enabled for client %i before accessing RefereeMenu_ChangeRound. This should never happen.", client);
	}
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !Client_IsAdmin(i) )
			continue;
		
		if (g_isEditingSetting[i])
		{
			decl String:clientName[MAX_NAME_LENGTH];
			GetClientName( i, clientName, sizeof(clientName) );
			PrintToChat(client, "Admin %s is currently editing a setting. Please wait.", clientName);
			
			return;
		}
	}
	
	g_isEditingSetting[client] = true;
	
	new Handle:panel = CreatePanel();
	
	SetPanelTitle(panel, "Change current round");
	DrawPanelText(panel, " ");
	
	decl String:currentRound[24];
	Format(currentRound, sizeof(currentRound), "Current round: %i", g_roundNumber);
	DrawPanelText(panel, currentRound);
	
	decl String:teamScores[24];
	Format(teamScores, sizeof(teamScores), "Jinrai: %i -- %i :NSF", g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber]);
	DrawPanelText(panel, teamScores);
	
	DrawPanelText(panel, " ");
	
	decl String:targetRound[24];
	Format(targetRound, sizeof(targetRound), "Target round: %i", g_targetRound);
	DrawPanelText(panel, targetRound);
	
	decl String:teamScores_target[24];
	Format(teamScores_target, sizeof(teamScores_target), "Jinrai: %i -- %i :NSF", g_jinraiScore[g_targetRound], g_nsfScore[g_targetRound]);
	DrawPanelText(panel, teamScores_target);
	
	DrawPanelText(panel, " ");
	
	DrawPanelItem(panel, "<---"); // Previous round
	DrawPanelItem(panel, "--->"); // Next round
	
	DrawPanelText(panel, " ");
	
	// Accept round change
	if (!g_confirmRoundChange)
		DrawPanelItem(panel, "Restore round");
	else
		DrawPanelItem(panel, "Restore round (confirm)");
	
	DrawPanelItem(panel, "Cancel"); // Go back
	
	SendPanelToClient(panel, client, PanelHandler_RefereeMenu_ChangeRound, MENU_TIME);
	
	CloseHandle(panel);
}

// Todo: add functionality
void RefereeMenu_LoadMatch(client)
{
	PrintToChat(client, "The goggles, they do nothing!");
	Command_RefereeMenu(client, 1);
}

void ChatCvarEdit(client, Handle:cvar)
{
	if (cvar == INVALID_HANDLE)
	{
		LogError("Attempted to edit an invalid cvar handle.");
		return;
	}
	
	if ( !Client_IsValid(client) )
		return;
	
	if (g_isEditingSetting[client])
	{
		g_isEditingSetting[client] = false;
		LogError("g_isEditingSetting was already enabled for client %i before accessing ChatCvarEdit. This should never happen.", client);
	}
	
	for (new i = 0; i <= MaxClients; i++)
	{
		if (g_isEditingSetting[i])
		{
			if ( !Client_IsValid(i) )
			{
				LogError("Invalid client with g_isEditingSetting bool as true.");
				PrintToChat(client, "%s Error editing cvar via chat. Check error logs for more info.", g_tag);
				return;
			}
			
			decl String:clientName[MAX_NAME_LENGTH];
			GetClientName(i, clientName, sizeof(clientName));
			
			PrintToChat(client, "%s \"%s\" is currently editing a game setting. Please wait.", g_tag, clientName);
			return;
		}
	}
	
	g_isEditingSetting[client] = true;
	GetConVarName( cvar, g_convarBeingEdited, sizeof(g_convarBeingEdited) );
	
	AddCommandListener(SayCallback, "say");
	AddCommandListener(SayCallback, "say_team");
	
	PrintToChat(client, "%s Please enter a new value in the chat.", g_tag);
	PrintToChat(client, "To cancel this action, type \"cancel\".");
	PrintToChat(client, "For text values, you can also type \"default\" to revert the changes.");
}

// Make sure Neotokyo gamedata file is up to date, as some of the plugin features require it.
void CheckGamedataFiles()
{
	new String:gamedataUrl[] = "https://github.com/alliedmodders/sourcemod/tree/master/gamedata"; // URL for the latest NT gamedata version
	
	decl String:filePath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, filePath, sizeof(filePath), "gamedata/sdktools.games/game.neotokyo.txt"); // Gamedata file path
	
	// Do a filesize based check.
	// This should be fairly safe as the NT gamedata is updated extremely rarely.
	new fileSize					= FileSize(filePath, false);		// Filesize of the currently running server gamedata version
	new correctFileSize		= 1933;								// Filesize of the correct gamedata version (10 Oct 2015)
	new oldFileSize_2014	= 1179;								// Old version filesize (23 Dec 2014)
	new oldFileSize_2009	= 1075;								// Old version filesize (15 Dec 2009)
	
	// File not found
	if (fileSize == -1)
	{
		LogError("Neotokyo gamedata file %s could not be found! sm_competitive_pause_mode 2 is unavailable because of this. See %s for the latest gamedata version.", filePath, gamedataUrl);
		g_isGamedataOutdated = true;
	}
	// Filesize mismatch
	else if (fileSize != correctFileSize)
	{
		// Old file version
		if (fileSize == oldFileSize_2014 || fileSize == oldFileSize_2009)
		{
			LogError("Neotokyo gamedata file %s has mismatching filesize %i bytes (expected %i bytes), it's most likely outdated. \"sm_competitive_pause_mode 2\" is unavailable because of this. See %s for the latest gamedata version.", filePath, fileSize, correctFileSize, gamedataUrl);
			g_isGamedataOutdated = true;
		}
		// Unknown file version. Notify server, but don't treat as an error.
		else
		{
			PrintToServer("%s Note: Neotokyo gamedata file %s has a nonstandard filesize %i bytes (expected %i bytes), has it been customized? \"sm_competitive_pause_mode 2\" may throw an error if the file is outdated.", filePath, fileSize, correctFileSize);
			g_isGamedataOutdated = false;
		}
	}
	// Filesize matches
	else
	{
		g_isGamedataOutdated = false;
	}
}

void PrintToAdmins(const String:message[], bool toChat = true, bool toConsole = false)
{
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !Client_IsAdmin(i) )
			continue;
		
		if (toChat)
			PrintToChat(i, message);
		
		if (toConsole)
			PrintToConsole(i, message);
	}
}

void RestoreRound(roundNumber, isAdminTriggered = false, client = 0)
{	
	if (roundNumber > g_furthestPlayedRound || roundNumber <= 0)
	{
		LogError("Attempted to restore a nonexistant round %i. Furthest played round is %i.", roundNumber, g_furthestPlayedRound);
		
		decl String:error[128];
		Format(error, sizeof(error), "Failed restoring round %i. Check error logs for details.", roundNumber);
		PrintToAdmins(error);
		
		g_confirmRoundChange = false;
		return;
	}
	
	if (isAdminTriggered)
	{
		if ( client == 0 || !Client_IsValid(client) || !Client_IsAdmin(client) )
		{
			LogError("Invalid client index %i for admin trigger", client);
			
			g_confirmRoundChange = false;
			return;
		}
		
		if (g_isPaused)
		{
			PrintToChat(client, "This command cannot be used while paused.");
			PrintToChat(client, "Please unpause the game first.");
			return;
		}
		
		if (!g_confirmRoundChange)
		{
			PrintToChat(client, "Restoring match from round %i, are you sure? Please press again to confirm.", roundNumber);
			
			g_confirmRoundChange = true;
			RefereeMenu_ChangeRound(client);
			
			CreateTimer(10.0, Timer_ConfirmRoundChange);
			return;
		}
	}
	
	g_roundNumber = roundNumber; // Change round number
	SetTeamScore(TEAM_JINRAI, g_jinraiScore[roundNumber]); // Set Jinrai score
	SetTeamScore(TEAM_NSF, g_nsfScore[roundNumber]); // Set NFS score
	
	// Set players' XP, deaths, ranks
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || IsFakeClient(i) )
			continue;
		
		SetEntProp(i, Prop_Data, "m_iDeaths", g_playerDeaths[i][roundNumber]); // Restore deaths
		SetEntProp(i, Prop_Data, "m_iFrags", g_playerXP[i][roundNumber]); // Restore XP
		
#if DEBUG > 1
		PrintToServer("Client %i has %i XP and %i deaths at round %i.", i, g_playerXP[i][roundNumber], g_playerDeaths[i][roundNumber], roundNumber);
#endif
		
		// We have to manually restore rank based on XP, so weapons selection won't glitch out
		if (g_playerXP[i][roundNumber] < 0)
			SetEntProp(i, Prop_Send, "m_iRank", 0); // Rankless dog
		else if (g_playerXP[i][roundNumber] < 4)
			SetEntProp(i, Prop_Send, "m_iRank", 1); // Private
		else if (g_playerXP[i][roundNumber] < 10)
			SetEntProp(i, Prop_Send, "m_iRank", 2); // Corporal
		else if (g_playerXP[i][roundNumber] < 20)
			SetEntProp(i, Prop_Send, "m_iRank", 3); // Sergeant
		else
			SetEntProp(i, Prop_Send, "m_iRank", 4); // Lieutenant
	}
	
	if (isAdminTriggered)
	{
		g_confirmRoundChange = false;
		Command_RefereeMenu(client, 1);
		
		GameRules_SetPropFloat( "m_fRoundTimeLeft", 0.0 ); // Stop the round
		
		new Handle:endRoundTime = FindConVar("mp_chattime");
		CreateTimer( (GetConVarFloat(endRoundTime) + 5.1), Timer_RestoreRound, roundNumber);
		CloseHandle(endRoundTime);
		
		g_isCurrentlyRestoringRound = true;
		
		PrintToChatAll("%s Round changed by admin.", g_tag);
	}
}

public Action:Timer_RestoreRound(Handle:timer, any:roundNumber)
{
	RestoreRound(roundNumber);
}

public Action:Timer_ConfirmRoundChange(Handle:timer)
{
	g_confirmRoundChange = false;
}

void EmptyScore()
{
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || IsFakeClient(i) )
			continue;
		
		SetEntProp(i, Prop_Data, "m_iDeaths", 0); // 0 deaths
		SetEntProp(i, Prop_Data, "m_iFrags", 0); // 0 kills
		SetEntProp(i, Prop_Send, "m_iRank", 1); // Private rank
	}
}

void PenalizeTeam(admin, team)
{
	if (team != TEAM_JINRAI && team != TEAM_NSF)
	{
		LogError("Invalid team %i", team);
		Command_RefereeMenu(admin, 1);
		return;
	}
	
	g_penalizedTeam = team;
	
	new Handle:panel = CreatePanel();
	
	SetPanelTitle(panel, "Give team penalty");
	DrawPanelText(panel, " ");
	
	decl String:penaltyDescription[128];
	Format( penaltyDescription, sizeof(penaltyDescription), "Penalize team %s?", g_teamName[team] );
	
	DrawPanelText(panel, penaltyDescription);
	DrawPanelText(panel, "This will give 1 point to the opposing team.");
	
	DrawPanelItem(panel, "Yes, give penalty.");
	DrawPanelItem(panel, "Cancel");
	
	SendPanelToClient(panel, admin, PanelHandler_PenalizeTeam, MENU_TIME);
	
	CloseHandle(panel);
}

void ApplyPenalty(admin, team)
{
	if (team != TEAM_JINRAI && team != TEAM_NSF)
	{
		LogError("Invalid team %i", team);
		Command_RefereeMenu(admin, 1);
		return;
	}
	
	if (team == TEAM_JINRAI)
	{
		g_nsfScore[g_roundNumber]++;
		SetTeamScore(TEAM_NSF, g_nsfScore[g_roundNumber]);
	}
	else
	{
		g_jinraiScore[g_roundNumber]++;
		SetTeamScore(TEAM_JINRAI, g_jinraiScore[g_roundNumber]);
	}
	
	PrintToChatAll("%s Penalty given to team \"%s\" by admin.", g_tag, g_teamName[team]);
	PrintToChatAll("The opposing team gains +1 point.");
	
	PlayLiveBeep(); // Sound notification about the penalty
	
	CheckMatchState();
	
	Command_RefereeMenu(admin, 1);
}

void CheckMatchState()
{
	new roundLimit = GetConVarInt(g_hRoundLimit);
	decl String:msgBuffer[256];
	
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
}