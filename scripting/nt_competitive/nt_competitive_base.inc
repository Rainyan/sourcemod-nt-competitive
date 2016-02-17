// Double-include prevention
#if defined _base_included_
	#endinput
#endif
#define _base_included_

#define PLUGIN_VERSION "0.3.9.12"

#define MAX_STEAMID_LENGTH 44
#define MAX_ROUNDS_PLAYED 255 // This is just a random large number used for 1d int arrays because it's cheap and simple. A single comp game should never have more rounds than this to avoid weirdness.
#define MAX_CVAR_LENGTH 64

#define KV_IDENTIFIER "compdata"

new const String:g_tag[] = "[COMP]";
new String:g_teamName[][64] = {
	"Unassigned",
	"Spectator",
	"Jinrai",
	"NSF"
};
new String:g_livePlayers[MAXPLAYERS+1][MAX_STEAMID_LENGTH]; // Array for storing SteamIDs to allow disconnected players rejoining when live
new const String:g_menuSoundOk[] = "buttons/button14.wav";
new const String:g_menuSoundCancel[] = "buttons/combine_button7.wav";
new const String:g_soundLive[] = "buttons/button17.wav";
new String:g_killInfoBuffer[MAXPLAYERS+1][512];
new String:g_convarBeingEdited[128];
#if DEBUG
new String:g_path_logDebug[PLATFORM_MAX_PATH] = "logs/competitive";
new String:g_kvPath[PLATFORM_MAX_PATH] = "data/competitive/matches";
new String:g_kvName[128];
new String:g_kvName_Temp[MAXPLAYERS+1][sizeof(g_kvName)];
#endif

new const String:g_kvKey_jinraiName[] = "jinrai_name";
new const String:g_kvKey_nsfName[] = "nsf_name";
new const String:g_kvKey_competitionName[] = "competition_name";
new const String:g_kvKey_sourceTVPath[] = "sourcetv_path";
new const String:g_kvKey_roundLimit[] = "round_limit";
new const String:g_kvKey_matchSize[] = "match_size";
new const String:g_kvKey_maxTimeouts[] = "max_timeouts";
new const String:g_kvKey_maxPauseLength[] = "max_pause_length";
new const String:g_kvKey_maxPauseLengthTechnical[] = "max_pause_length_technical";
new const String:g_kvKey_commsBehaviour[] = "comms_behaviour";
new const String:g_kvKey_logMode[] = "log_mode";
new const String:g_kvKey_killVerbosity[] = "kill_verbosity";
new const String:g_kvKey_killVerbosityDelay[] = "kill_verbosity_delay";
new const String:g_kvKey_clientRecording[] = "client_recording";
new const String:g_kvKey_limitLiveTeams[] = "limit_live_teams";
new const String:g_kvKey_limitTeams[] = "limit_teams";
new const String:g_kvKey_pauseMode[] = "pause_mode";
new const String:g_kvKey_collectiveReady[] = "collective_ready";
new const String:g_kvKey_noZanshi[] = "no_zanshi";
new const String:g_kvKey_jinraiScore[] = "jinrai_score";
new const String:g_kvKey_nsfScore[] = "nfs_score";
new const String:g_kvKey_suddenDeath[] = "sudden_death";
new const String:g_kvKey_centered_displayRemaining[] = "centered_display_remaining";
new const String:g_kvKey_centered_displayTarget[] = "centered_display_target";
new const String:g_kvKey_natives_alltalk[] = "natives_alltalk";
new const String:g_kvKey_natives_forceCamera[] = "natives_forcecamera";
new const String:g_kvKey_natives_pausable[] = "natives_pausable";
new const String:g_kvKey_natives_roundTime[] = "natives_round_time";
new const String:g_kvKey_natives_scoreLimit[] = "natives_scorelimit";
new const String:g_kvKey_jinraiPauses[] = "jinrai_pauses_used";
new const String:g_kvKey_nsfPauses[] = "nsf_pauses_used";

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
new g_ghostCapturingTeam;
new g_epoch;

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
new bool:g_isSuddenDeathRound;
new bool:g_goLiveAfterKvRestore[MAXPLAYERS+1] = true;

new Handle:g_hAlltalk;
new Handle:g_hCommsBehaviour;
new Handle:g_hCompetitionName;
new Handle:g_hForceCamera;
new Handle:g_hLogMode;
new Handle:g_hMatchSize;
new Handle:g_hMaxPauseLength;
new Handle:g_hMaxPauseLength_Technical;
new Handle:g_hMaxTimeouts;
new Handle:g_hNeoScoreLimit;
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
new Handle:g_hJinraiScore;
new Handle:g_hNSFScore;
new Handle:g_hSuddenDeath;
new Handle:g_hCenteredDisplayRemaining;
new Handle:g_hCenteredDisplayTarget;
#if DEBUG
new Handle:g_hDebugKeyValues;
#endif

new Handle:g_hTimer_LiveCountdown			= INVALID_HANDLE;
new Handle:g_hTimer_GoLive						= INVALID_HANDLE;
new Handle:g_hTimer_UnPause					= INVALID_HANDLE;
new Handle:g_hTimer_UnPause_Countdown	= INVALID_HANDLE;
new Handle:g_hTimer_UnPause_HalfLeft		= INVALID_HANDLE;

KeyValues g_kv;

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
	GAMESTATE_WARMUP = 1,
	GAMESTATE_NORMAL
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
	if (g_isLiveCountdown || g_isLive)
		return;
	
	new players = GetPlayerCount();
	new playersReady = GetPlayersReady();
	
#if DEBUG
	LogDebug("%s Players found total: %i", g_tag, players);
	LogDebug("%s Players ready: %i", g_tag, playersReady);
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
	
	ReplyToCommand(client, "%s List of readied players is printed in your console.", g_tag);
	
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
			LogDebug("Client %i is now recording to: %s", i, replayName);
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
			LogDebug("Client %i has stopped recording.", i);
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

void ToggleLive(bool:isKvRestore = false)
{
	g_isLive = !g_isLive;
	
	if (g_isLive)
	{
		SetGameState(GAMESTATE_NORMAL);
		GenerateEpoch();
		
		// Cancel both teams' override preference
		g_isWantingOverride[TEAM_JINRAI] = false;
		g_isWantingOverride[TEAM_NSF] = false;
		g_isExpectingOverride = false;
#if DEBUG
		if ( GetConVarBool(g_hDebugKeyValues) )
		{
			Kv_Generate();
		}
#endif
		g_isLiveCountdown = false;
		g_winner = 0; // Reset winner (overlay module queries this value)
		
#if DEBUG
		LogDebug("Creating SteamID array of players...");
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
				LogDebug("%s Player \"%s\" has no SteamID authorized yet! Stopped going live.");
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
#if DEBUG
				LogDebug("Found SteamID: %s", steamID);
				LogDebug("Saved to array as: %s, team %s", g_livePlayers[i], g_teamName[g_assignedTeamWhenLive[i]]);
				foundPlayers++;
#endif
			}
		}
#if DEBUG
		LogDebug("Completed SteamID array with %i results.", foundPlayers);
#endif
		
		// Set convars for competitive
		SetConVarInt(g_hAlltalk, GetDesiredCommsBehaviour());
		SetConVarBool(g_hPausable, false);
		SetConVarBool(g_hForceCamera, true);
		SetConVarInt(g_hNeoScoreLimit, 99); // Make sure Neotokyo's own max round count is maxed out so it won't interfere
		
		// Only reset team/player scores if we're not restoring a previous game from KeyValues
		if (!isKvRestore)
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
		
		SpamTimer();
	}
	
	// Stop live
	else
	{
#if DEBUG
		if ( GetConVarBool(g_hDebugKeyValues) )
		{
			if ( Kv_IsValid(g_kv) )
			{
				Kv_Store(g_kv, g_kvName);
				CloseHandle(g_kv);
				g_kv = null;
			}
		}
#endif
		
		if (g_isPaused)
			TogglePause();
		
		ToggleRecording();
		SetConVarBool(g_hForceCamera, false);
		ResetGlobalVariables();
		
		PrintToChatAll("%s Match has ended.", g_tag);
		
		SetGameState(GAMESTATE_WARMUP);
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
				
#if DEBUG
				LogDebug( "%s Pausing! Pause max duration: %i seconds.", g_tag, RoundToNearest(fPauseLength) );
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
				SetGameState(GAMESTATE_NORMAL);
				
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
	SetGameState(GAMESTATE_WARMUP);
	
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
						LogDebug("Force pause detected.");
					
					new String:clientName[MAX_NAME_LENGTH];
					GetClientName(i, clientName, sizeof(clientName));
					LogDebug("Found client to toggle pause. Client int: %i, name: %s", i, clientName);
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
			LogDebug("Found %i invalid clients. Found %i valid unspawned clients.", invalidClients, unspawnedClients[1]);
#endif
			
			if (GetGameTime() > g_fRoundTime + 13.9)
			{
				forcePause = true;
				i = 0; // Reset loop
				
#if DEBUG > 1
				LogDebug("New round is about to start! Force any valid client to toggle pause...");
#endif
				continue;
			}
			
			CreateTimer(1.0, Timer_TogglePauseLoop);
			
#if DEBUG > 1
			LogDebug("No valid spawned client found. Restarting loop in TogglePauseLoop");
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
		PrintToChatAll("%s Unpausing...", g_tag);
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
		LogDebug("ToggleSourceTV: tv_stoprecord");
#endif
	}
	
	else
	{
		if (!GetConVarBool(g_hSourceTVEnabled) || !g_isLive)
			return;
		
		if (!IsSourceTVBotConnected())
		{
#if DEBUG
			LogDebug("ToggleSourceTV: SourceTV bot not found.");
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
			Format(replayName, sizeof(replayName), "%s_%s_%s_", competitionName, timestamp, mapName);
		
		else
			Format(replayName, sizeof(replayName), "%s_%s_", timestamp, mapName);
		
		// Concatenate epoch to the end of filename. This can be helpful to connect a replay to related KeyValues entry
		decl String:epochBuffer[16];
		IntToString( g_epoch, epochBuffer, sizeof(epochBuffer) );
		StrCat(replayName, sizeof(replayName), epochBuffer);
		
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
		LogDebug("ToggleSourceTV: tv_record \"%s/%s\"", demoPath, replayName);
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
	LogDebug("Looking for a SourceTV bot...");
#endif
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i) || !IsFakeClient(i)) // We are looking for a valid fake client (bot)
			continue;
		
		decl String:clientName[MAX_NAME_LENGTH];
		GetClientName(i, clientName, sizeof(clientName));
		
#if DEBUG > 1
		LogDebug("Found valid fake client, index: %i, name: %s", i, clientName);
#endif
		
		if (StrEqual(clientName, botName) || StrEqual(clientName, "SourceTV")) // Also check for default name "SourceTV", because sometimes tv_name doesn't work when bot first spawns
		{			
#if DEBUG > 1
			LogDebug("Fake client name %s matches tv_name cvar %s or \"SourceTV\"", clientName, botName);
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
		if ( !Client_IsValid(i) || !IsClientInGame(i) || IsFakeClient(i) )
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

void PrintToConsoleAll(const String:message[], any ...)
{
	decl String:formatMsg[512];
	VFormat(formatMsg, sizeof(formatMsg), message, 2);
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !IsClientInGame(i) || IsFakeClient(i) )
			continue;
		
		PrintToConsole(i, formatMsg);
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
	
	SetConVarInt(g_hJinraiScore, correctInt);
	SetConVarInt(g_hNSFScore, correctInt);
	
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
	g_isSuddenDeathRound	= correctBool;
	
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
	
	SendPanelToClient(panel, client, PanelHandler_RefereeMenu_GameInfo, MENU_TIME_FOREVER);
	
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
	
	SendPanelToClient(panel, client, PanelHandler_PenaltyMenu, MENU_TIME_FOREVER);
	
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
	
	SendPanelToClient(panel, client, PanelHandler_RefereeMenu_ChangeRound, MENU_TIME_FOREVER);
	
	CloseHandle(panel);
}

void RefereeMenu_ManualEdit_TeamScore(client)
{
	if (!g_isLive)
	{
		PrintToChat(client, "Game is not live, can't edit score!");
		Command_RefereeMenu(client, 1);
		return;
	}
	
	if (g_isEditingSetting[client])
	{
		g_isEditingSetting[client] = false;
		LogError("g_isEditingSetting was already enabled for client %i before accessing RefereeMenu_ManualEdit_TeamScore. This should never happen.", client);
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
	
	new Handle:panel = CreatePanel();
	
	SetPanelTitle(panel, "Manually edit team score");
	DrawPanelText(panel, " ");
	
	decl String:teamScores[24];
	Format(teamScores, sizeof(teamScores), "Jinrai: %i -- %i :NSF", g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber]);
	DrawPanelText(panel, teamScores);
	
	DrawPanelText(panel, " ");
	
	DrawPanelItem(panel, "Edit Jinrai score");
	DrawPanelItem(panel, "Edit NSF score");
	DrawPanelItem(panel, "Cancel");
	
	SendPanelToClient(panel, client, PanelHandler_ManualEdit_TeamScore, MENU_TIME_FOREVER);
	
	CloseHandle(panel);
}

// Todo: add functionality
void RefereeMenu_ManualEdit_PlayerScore(client)
{
	PrintToChat(client, "The goggles, they do nothing!");
	Command_RefereeMenu(client, 1);
}

void RefereeMenu_LoadMatch(client)
{
#if DEBUG
	Menu menu = CreateMenu(MenuHandler_RefereeMenu_LoadMatch);
	
	menu.SetTitle("Load previous match state");
	
	decl String:fullPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, fullPath, sizeof(fullPath), g_kvPath);
	
	if ( !DirExists(fullPath) )
	{
		LogError("Failed to open KeyValues directory path: %s", fullPath);
		return;
	}
	
	new Handle:dir = OpenDirectory(fullPath);
	if (dir == null)
	{
		LogError("Failed to open KeyValues directory path: %s", fullPath);
		return;
	}
	
	new results;
	new max_files = 128;
	decl String:buffer[PLATFORM_MAX_PATH];
	FileType type;
	while ( ReadDirEntry(dir, buffer, sizeof(buffer), type) )
	{
		results++;
		
		if (results > max_files)
		{
			//fixme: unoptimised
			LogError("Warning: Over %i file results found. Breaking out of the loop to avoid unresponsiveness.", max_files);
			break;
		}
		
		// Not a file
		if (type != FileType_File)
			continue;
		
		// Not a .kv file
		if ( !Kv_IsValidFile(buffer) )
			continue;
		
//		PrintToServer("Found file: %s", buffer);
		menu.AddItem(buffer, buffer);
	}
	
	CloseHandle(dir);
	
	menu.Display(client, MENU_TIME_FOREVER);
#else
	Command_RefereeMenu(client, 1);
#endif
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
	new correctFileSize		= 1933;								// Filesize of the correct gamedata version (30 Dec 2015 or 10 Oct 2015)
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
			PrintToServer("%s Note: Neotokyo gamedata file %s has a nonstandard filesize %i bytes (expected %i bytes), has it been customized? \"sm_competitive_pause_mode 2\" may throw an error if the file is outdated.", g_tag, filePath, fileSize, correctFileSize);
			g_isGamedataOutdated = false;
		}
	}
	// Filesize matches
	else
	{
		g_isGamedataOutdated = false;
	}
}

void PrintToAdmins(bool toChat = true, bool toConsole = false, const String:message[], any ...)
{
	decl String:formatMsg[512];
	VFormat(formatMsg, sizeof(formatMsg), message, 4);
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !Client_IsAdmin(i) )
			continue;
		
		if (toChat)
			PrintToChat(i, formatMsg);
		
		if (toConsole)
			PrintToConsole(i, formatMsg);
	}
}

void RestoreRound(roundNumber, bool:isAdminTriggered = false, client = 0, bool:isZanshiRestore = false)
{	
	if (roundNumber > g_furthestPlayedRound || roundNumber <= 0)
	{
		LogError("Attempted to restore a nonexistant round %i. Furthest played round is %i.", roundNumber, g_furthestPlayedRound);
		
		new bool:adminsPresent;
		for (new i = 1; i <= MaxClients; i++)
		{
			if ( !Client_IsValid(i) || !Client_IsAdmin(i) )
				continue;
			
			adminsPresent = true;
			break;
		}
		
		if (adminsPresent)
			PrintToAdmins(_, _, "Failed restoring round %i. Check SM error logs for details.", roundNumber);
		else
			PrintToChatAll("Round restore failed. Contact server admins for help.");
		
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
	
	if (isZanshiRestore == false)
	{
		// Only edit round number if restoring previous match state (unpause etc)
		g_roundNumber = roundNumber;
		
		// Only recover previous XP/ranks if restoring previous match state (unpause etc)
		for (new i = 1; i <= MaxClients; i++)
		{
			if ( !Client_IsValid(i) || IsFakeClient(i) )
				continue;
			
			SetEntProp(i, Prop_Data, "m_iDeaths", g_playerDeaths[i][roundNumber]); // Restore deaths
			SetPlayerXP(i, g_playerXP[i][roundNumber]); // Restore XP
#if DEBUG > 1
			LogDebug("Client %i has %i XP and %i deaths at round %i.", i, g_playerXP[i][roundNumber], g_playerDeaths[i][roundNumber], roundNumber);
#endif
			UpdatePlayerRank(i);
		}
	}
	
	SetTeamScore(TEAM_JINRAI, g_jinraiScore[roundNumber]); // Set Jinrai score
	SetTeamScore(TEAM_NSF, g_nsfScore[roundNumber]); // Set NFS score
	
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

public Action:Timer_ClearGhostCapper(Handle:timer)
{
	g_ghostCapturingTeam = TEAM_NONE;
}

void EmptyScore()
{
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !IsClientInGame(i) || IsFakeClient(i) )
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
	
	SendPanelToClient(panel, admin, PanelHandler_PenalizeTeam, MENU_TIME_FOREVER);
	
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
#if DEBUG
	if ( GetConVarBool(g_hDebugKeyValues) )
	{
		Kv_LogRound(g_kv);
		Kv_Store(g_kv, g_kvName);
	}
#endif
	new roundLimit = GetConVarInt(g_hRoundLimit);
	new roundsLeft = roundLimit - g_roundNumber;
	decl String:msgBuffer[256];
	
	// ** Announce match state below **
	// Jinrai wins, NSF can't catch up anymore
	if ( !g_isSuddenDeathRound && g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber] + roundLimit - (g_roundNumber - 1) ) // (g_roundNumber - 1) because we're counting rounds by incrementing inside the event. So first round is 1, not 0.
	{
#if DEBUG
		LogDebug("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber]);
		LogDebug("jinScore %i > nsfScore %i + roundLimit %i - roundCount %i", g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber], roundLimit, g_roundNumber);
#endif
		
		Format(msgBuffer, sizeof(msgBuffer), "%s %s WINS %i - %i", g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber]);
		
		PrintToChatAll(msgBuffer);
		LogCompetitive(msgBuffer);
		
		g_winner = TEAM_JINRAI;
	}
	
	// NSF wins, Jinrai can't catch up anymore
	else if ( !g_isSuddenDeathRound && g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber] + roundLimit - (g_roundNumber - 1))
	{
#if DEBUG
		LogDebug("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_NSF], g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);
		LogDebug("nsfScore %i > jinScore %i + roundLimit %i - roundCount %i", g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber], roundLimit, g_roundNumber);
#endif
		
		Format(msgBuffer, sizeof(msgBuffer), "%s %s WINS %i - %i", g_tag, g_teamName[TEAM_NSF], g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);
		
		PrintToChatAll(msgBuffer);
		LogCompetitive(msgBuffer);
		
		g_winner = TEAM_NSF;
	}
	
	else if (g_isSuddenDeathRound)
	{
		if (g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber])
		{
			Format(msgBuffer, sizeof(msgBuffer), "%s %s WINS %i - %i", g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber]);
			
			PrintToChatAll(msgBuffer);
			LogCompetitive(msgBuffer);
			
			g_winner = TEAM_JINRAI;
		}
		else if (g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber])
		{
			Format(msgBuffer, sizeof(msgBuffer), "%s %s WINS %i - %i", g_tag, g_teamName[TEAM_NSF], g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);
			
			PrintToChatAll(msgBuffer);
			LogCompetitive(msgBuffer);
			
			g_winner = TEAM_NSF;
		}
		else
		{
			Format(msgBuffer, sizeof(msgBuffer), "%s SUDDEN DEATH. Next team to score wins.", g_tag);
			PrintToChatAll(msgBuffer);
			LogCompetitive(msgBuffer);
			
			return;
		}
	}
	
	// Tie, we ran out of rounds to play
	else if ( (g_roundNumber - 1) == roundLimit )
	{
		// Sudden death mode
		if ( GetConVarBool(g_hSuddenDeath) )
		{
			if (!g_isSuddenDeathRound)
			{
				g_isSuddenDeathRound = true;
				Format(msgBuffer, sizeof(msgBuffer), "%s SUDDEN DEATH. Next team to score wins.", g_tag);
				PrintToChatAll(msgBuffer);
				LogCompetitive(msgBuffer);
				
				return;
			}
		}
		
		// No sudden death. End match in a tie.
		else
		{
			Format(msgBuffer, sizeof(msgBuffer), "%s MATCH TIED %i - %i", g_tag, g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);
			PrintToChatAll(msgBuffer);
		}
		
		LogCompetitive(msgBuffer);
	}
	
	// Check if game ended, and toggle off live mode if appropriate
	if ((g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber] + roundLimit - (g_roundNumber - 1)) || (g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber] + roundLimit - (g_roundNumber - 1)) || ((g_roundNumber - 1) == roundLimit))
	{
		ToggleLive();
		LogCompetitive("Competitive match ended.");
	}
	
	// Match point, one team has almost won
	else if (
				// Jinrai is winning, and NSF can catch up exactly in the amount of rounds remaining
				(g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber] && g_nsfScore[g_roundNumber] + roundsLeft == g_jinraiScore[g_roundNumber]) ||
				// NSF is winning, and Jinrai can catch up exactly in the amount of rounds remaining
				(g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber] && g_jinraiScore[g_roundNumber] + roundsLeft == g_nsfScore[g_roundNumber]) ||
				// It is the last round
				(g_roundNumber == roundLimit)
			)
			{
				PrintToChatAll("%s Round %i/%i (MATCH POINT)", g_tag, g_roundNumber, roundLimit);
			}
	
	// Just another round, simply announce round number
	else
	{
		// Don't announce match number during the initial "LIVE LIVE LIVE" chat spam
		if (g_spamTimerCount <= 1)
			PrintToChatAll("%s Round %i/%i", g_tag, g_roundNumber, roundLimit);
	}
}

void CheckGhostcapPlugin()
{
	// Anti-timeout cvar is disabled, no need to check for ghostcap plugin
	if ( !GetConVarBool(g_hPreventZanshiStrats) )
		return;
	
	new Handle:ghostcapPlugin = FindConVar("sm_ntghostcap_version");
	
	// Look for ghost cap plugin's version variable
	if (ghostcapPlugin == null)
	{
		new String:ghostcapUrl[] = "https://github.com/softashell/nt-sourcemod-plugins";
		LogError("Using sm_competitive_nozanshi 1 setting requires Soft as HELL's Ghost cap event plugin, version 1.5.4 or newer: %s", ghostcapUrl);
		SetConVarBool(g_hPreventZanshiStrats, false);
	}
}

#if DEBUG
void PrepareDebugLogFolder()
{
	decl String:path[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, path, sizeof(path), g_path_logDebug);
	if ( !DirExists(path) )
		CreateDirectory(path, 509);
}

void LogDebug(const String:message[], any ...)
{
	decl String:path[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, path, sizeof(path), g_path_logDebug);
	StrCat(path, sizeof(path), "/competitive_debug.log");
	
	// Format according to SM format rules
	decl String:formatMsg[512];
	VFormat(formatMsg, sizeof(formatMsg), message, 2);
	
	new Handle:file = OpenFile(path, "a"); // fopen
	WriteFileLine(file, formatMsg);
	CloseHandle(file);
}
#endif

bool DidPlayerReallySpawn(client)
{
	if ( !Client_IsValid(client) || !IsClientInGame(client) )
	{
//		PrintToServer("Client is not valid %i", client);
		return false;
	}
	
	new team = GetClientTeam(client);
	if (team != TEAM_JINRAI && team != TEAM_NSF)
	{
//		PrintToServer("Team is not valid %i", team);
		return false;
	}
	
	new Float:currentTime = GetGameTime();
	if (currentTime - g_fRoundTime > 30 + 1) // Spawn event triggered after round spawning is finished. Player cannot have spawned.
	{
//		PrintToServer("# # # NEW SPAWN Game Time: %f", currentTime);
//		PrintToServer("Time is not valid %f - %f = %f > 15+1", currentTime, g_fRoundTime, (currentTime-g_fRoundTime) );
		return false;
	}
	
	return true;
}

void CenterText_DisplayRemaining()
{
	if (GetConVarInt(g_hCenteredDisplayRemaining) == 0)
		return;
	
	decl String:message[24];
	
	// Just display numbers remaining
	if (GetConVarInt(g_hCenteredDisplayRemaining) == 1)
		Format(message, sizeof(message), "%i -- %i", g_playersAlive[TEAM_JINRAI], g_playersAlive[TEAM_NSF]);
	// Display team name + numbers remaining
	else
		Format(message, sizeof(message), "JIN %i -- %i NSF", g_playersAlive[TEAM_JINRAI], g_playersAlive[TEAM_NSF]);
	
	new bool:specOnly;
	if ( GetConVarInt(g_hCenteredDisplayTarget) == 1)
		specOnly = true;
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !IsClientInGame(i) )
			continue;
		
		new team = GetClientTeam(i);
		
		if (specOnly && team != TEAM_SPECTATOR)
			continue;
		
		else if ( team != TEAM_SPECTATOR || IsPlayerAlive(i) )
			continue;
		
		PrintCenterText(i, message);
	}
}

#if DEBUG
// Purpose: Make sure handle holds valid KeyValues data, using the comp plugin's format.
// This will also reset the entire traversal stack. Intended use: kv initialisation, import, export.
bool Kv_IsValid(KeyValues kv)
{
	if (kv == INVALID_HANDLE)
		return false;
	
	kv.Rewind();
	
	new identifier_length = strlen(KV_IDENTIFIER) +1; // strlen + terminator
	decl String:sectionName[identifier_length];
	kv.GetSectionName(sectionName, identifier_length);
	
	if ( !StrEqual(sectionName, KV_IDENTIFIER) )
	{
		LogError("KeyValues handle points to a KeyValues that is not standard for the competitive plugin. This is most likely an error.");
		LogError("Section name: %s Expected: %s", sectionName, KV_IDENTIFIER);
		return false;
	}
	
	return true;
}

bool Kv_IsValidFile(const String:fileName[])
{
	new const String:extension[] = ".kv";
	if ( StrContains(fileName, extension) != strlen(fileName) - strlen(extension) )
		return false;
	
	return true;
}

// Purpose: Logs current match status into round keyvalues entry
void Kv_LogRound(KeyValues kv)
{
	if ( !Kv_IsValid(kv) )
	{
		LogError("Failed logging round to KeyValues");
		return;
	}
	
	kv.Rewind();
	
	// Create or jump to round key
	decl String:roundKey[9];
	Format(roundKey, sizeof(roundKey), "round%i", g_roundNumber);
	kv.JumpToKey(roundKey, true);
	
	// Set round specific values
	kv.SetNum(g_kvKey_jinraiScore, g_jinraiScore[g_roundNumber]);
	kv.SetNum(g_kvKey_nsfScore, g_nsfScore[g_roundNumber]);
	kv.SetNum( g_kvKey_roundLimit, GetConVarInt(g_hRoundLimit) );
	kv.SetNum(g_kvKey_jinraiPauses, g_usedTimeouts[TEAM_JINRAI]);
	kv.SetNum(g_kvKey_nsfPauses, g_usedTimeouts[TEAM_NSF]);
	
	// Set player values for round
	kv.JumpToKey("client_values", true);
	decl String:steamID[MAX_STEAMID_LENGTH];
	new team;
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !IsClientConnected(i) || !IsClientAuthorized(i) || IsFakeClient(i) )
			continue;
		
		team = GetClientTeam(i);
		if (0 > team > 3)
			continue;
		
		GetClientAuthId( i, AuthId_Steam2, steamID, sizeof(steamID) );
		kv.JumpToKey(steamID, true);
		
		// Store current round values
		kv.SetNum("team", team);
		kv.SetNum( "xp", GetPlayerXP(i) );
		kv.SetNum( "deaths", GetPlayerDeaths(i) );
		
		// Insert client into meta client list
		kv.Rewind();
		kv.JumpToKey("meta");
		kv.JumpToKey("clients");
		kv.SetNum(steamID, team);
		
		// Return cursor to client_values
		kv.Rewind();
		kv.JumpToKey(roundKey);
		kv.JumpToKey("client_values");
	}
	
	g_kv = kv;
}

// Purpose: Generates new KeyValues structure
void Kv_Generate()
{
	Format(g_kvName, sizeof(g_kvName), "comp_%i", g_epoch);
	
	// Concatenate competition title (if any) to the end of filename
	decl String:compTitle[MAX_CVAR_LENGTH];
	GetConVarString( g_hCompetitionName, compTitle, sizeof(compTitle) );
	if (strlen(compTitle) > 0)
	{
		StrCat(g_kvName, sizeof(g_kvName), "_");
		
		// Clean up any non alphanumeric characters from the string
		new String:compBuffer[sizeof(compTitle)];
		new bufferPos;
		for (new j = 0; j < sizeof(compTitle); j++)
		{
			if (IsCharAlpha(compTitle[j]) || IsCharNumeric(compTitle[j]) || !IsCharSpace(compTitle[j]))
				compBuffer[bufferPos++] = compTitle[j];
		}
		compBuffer[bufferPos] = '\0'; // Terminate the string with 0
		StrCat(g_kvName, sizeof(g_kvName), compBuffer);
	}
	
	StrCat(g_kvName, sizeof(g_kvName), ".kv");
	
	g_kv = new KeyValues(KV_IDENTIFIER);
	Kv_PopulateMeta(g_kv);
}

// Purpose: Populates metadata for competitive plugin's KeyValues
void Kv_PopulateMeta(KeyValues kv)
{
	if ( !Kv_IsValid(kv) )
	{
		LogError("Failed populating KeyValues");
		return;
	}
	
	kv.JumpToKey("meta", true);
	
	kv.SetNum("timestamp", g_epoch);
	kv.SetString("plugin_version", PLUGIN_VERSION);
	
	// Store string cvars
	new String:jinraiName[MAX_CVAR_LENGTH];
	new String:nsfName[MAX_CVAR_LENGTH];
	new String:competitionName[MAX_CVAR_LENGTH];
	new String:sourceTVPath[MAX_CVAR_LENGTH];
	
	GetConVarString( g_hJinraiName, jinraiName, sizeof(jinraiName) );
	GetConVarString( g_hNSFName, nsfName, sizeof(nsfName) );
	GetConVarString( g_hCompetitionName, competitionName, sizeof(competitionName) );
	GetConVarString( g_hSourceTVPath, sourceTVPath, sizeof(sourceTVPath) );
	
	kv.SetString(g_kvKey_jinraiName, jinraiName);
	kv.SetString(g_kvKey_nsfName, nsfName);
	kv.SetString(g_kvKey_competitionName, competitionName);
	kv.SetString(g_kvKey_sourceTVPath, sourceTVPath);
	
	// Store integer/boolean cvars
	// Floats need SetFloat, but we're not expecting any as of now
	kv.SetNum( g_kvKey_roundLimit, GetConVarInt(g_hRoundLimit) );
	kv.SetNum( g_kvKey_matchSize, GetConVarInt(g_hMatchSize) );
	kv.SetNum( g_kvKey_maxTimeouts, GetConVarInt(g_hMaxTimeouts) );
	kv.SetNum( g_kvKey_maxPauseLength, GetConVarInt(g_hMaxPauseLength) );
	kv.SetNum( g_kvKey_maxPauseLengthTechnical, GetConVarInt(g_hMaxPauseLength_Technical) );
	kv.SetNum( g_kvKey_commsBehaviour, GetConVarInt(g_hCommsBehaviour) );
	kv.SetNum( g_kvKey_logMode, GetConVarInt(g_hLogMode) );
	kv.SetNum( g_kvKey_killVerbosity, GetConVarInt(g_hKillVersobity) );
	kv.SetNum( g_kvKey_killVerbosityDelay, GetConVarInt(g_hVerbosityDelay) );
	kv.SetNum( g_kvKey_clientRecording, GetConVarInt(g_hClientRecording) );
	kv.SetNum( g_kvKey_limitLiveTeams, GetConVarInt(g_hLimitLiveTeams) );
	kv.SetNum( g_kvKey_limitTeams, GetConVarInt(g_hLimitTeams) );
	kv.SetNum( g_kvKey_pauseMode, GetConVarInt(g_hPauseMode) );
	kv.SetNum( g_kvKey_collectiveReady, GetConVarInt(g_hCollectiveReady) );
	kv.SetNum( g_kvKey_noZanshi, GetConVarInt(g_hPreventZanshiStrats) );
	kv.SetNum( g_kvKey_jinraiScore, GetConVarInt(g_hJinraiScore) );
	kv.SetNum( g_kvKey_nsfScore, GetConVarInt(g_hNSFScore) );
	kv.SetNum( g_kvKey_suddenDeath, GetConVarInt(g_hSuddenDeath) );
	kv.SetNum( g_kvKey_centered_displayRemaining, GetConVarInt(g_hCenteredDisplayRemaining) );
	kv.SetNum( g_kvKey_centered_displayTarget, GetConVarInt(g_hCenteredDisplayTarget) );
	kv.SetNum( g_kvKey_natives_alltalk, GetConVarInt(g_hAlltalk) );
	kv.SetNum( g_kvKey_natives_forceCamera, GetConVarInt(g_hForceCamera) );
	kv.SetNum( g_kvKey_natives_pausable, GetConVarInt(g_hPausable) );
	kv.SetNum( g_kvKey_natives_roundTime, GetConVarInt(g_hRoundTime) );
	kv.SetNum( g_kvKey_natives_scoreLimit, GetConVarInt(g_hNeoScoreLimit) );
	
	// List clients and their teams
	kv.JumpToKey("clients", true);
	
	decl String:steamID[MAX_STEAMID_LENGTH];
	new team;
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || !IsClientConnected(i) || !IsClientAuthorized(i) || IsFakeClient(i) )
			continue;
		
		team = GetClientTeam(i);
		// Valid teams are in range 0-3
		if ( 0 > team > 3 )
			continue;
		
		GetClientAuthId( i, AuthId_Steam2, steamID, sizeof(steamID) );
		kv.SetNum(steamID, team);
	}
	
	Kv_Store(kv, g_kvName);
}

// Purpose: Stores KeyValues data globally and exports latest changes to file
void Kv_Store(KeyValues kv, const String:exportName[] = "comp_debug.kv")
{
	if ( !Kv_IsValid(kv) )
	{
		LogError("Failed exporting KeyValues");
		return;
	}
	
	g_kv = kv;
	
	if ( !Kv_IsValidFile(exportName) )
	{
		LogError("exportName doesn't use a .kv extension. This shouldn't happen.");
		return;
	}
	
	decl String:fullPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, fullPath, sizeof(fullPath), "%s/%s", g_kvPath, exportName);
	
	kv.ExportToFile(fullPath);
}

void Kv_RestoreFromFile(const String:fileName[], bool:goLive = false)
{
	if (strlen(fileName) < 1)
	{
		LogError("Attempted to restore KeyValues from zero length filename");
		return;
	}
	
	if ( !Kv_IsValidFile(fileName) )
	{
		LogError("fileName doesn't use a .kv extension. This shouldn't happen.");
		return;
	}
	
	decl String:fullPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, fullPath, sizeof(fullPath), "%s/%s", g_kvPath, fileName);
	
	KeyValues kv = new KeyValues("");
	
	if ( !kv.ImportFromFile(fullPath) )
	{
		LogError("Failed importing KeyValues from path: %s", fullPath);
		return;
	}
	
	kv.JumpToKey("meta", false);
	
	/*
	// TODO: Do KeyValues compatibility check here if/when breaking compatibility
	new String:pluginVersion[32];
	kv.GetString( "plugin_version", pluginVersion, sizeof(pluginVersion) );
	if ( !Kv_IsCompatible(pluginVersion) )
	{
		delete kv;
		return;
	}
	*/
	
	// Restore string variables
	new String:jinraiName[MAX_CVAR_LENGTH];
	new String:nsfName[MAX_CVAR_LENGTH];
	new String:competitionName[MAX_CVAR_LENGTH];
	new String:sourceTVPath[MAX_CVAR_LENGTH];
	
	kv.GetString( g_kvKey_jinraiName, jinraiName, sizeof(jinraiName) );
	kv.GetString( g_kvKey_nsfName, nsfName, sizeof(nsfName) );
	kv.GetString( g_kvKey_competitionName, competitionName, sizeof(competitionName) );
	kv.GetString( g_kvKey_sourceTVPath, sourceTVPath, sizeof(sourceTVPath) );
	
	SetConVarString(g_hJinraiName, jinraiName);
	SetConVarString(g_hNSFName, nsfName);
	SetConVarString(g_hCompetitionName, competitionName);
	SetConVarString(g_hSourceTVPath, sourceTVPath);
	
	// Restore numerical values
	SetConVarInt( g_hMatchSize, kv.GetNum(g_kvKey_matchSize) );
	SetConVarInt( g_hMaxTimeouts, kv.GetNum(g_kvKey_maxTimeouts) );
	SetConVarInt( g_hMaxPauseLength, kv.GetNum(g_kvKey_maxPauseLength) );
	SetConVarInt( g_hMaxPauseLength_Technical, kv.GetNum(g_kvKey_maxPauseLengthTechnical) );
	SetConVarInt( g_hCommsBehaviour, kv.GetNum(g_kvKey_commsBehaviour) );
	SetConVarInt( g_hLogMode, kv.GetNum(g_kvKey_logMode) );
	SetConVarInt( g_hKillVersobity, kv.GetNum(g_kvKey_killVerbosity) );
	SetConVarInt( g_hVerbosityDelay, kv.GetNum(g_kvKey_killVerbosityDelay) );
	SetConVarInt( g_hClientRecording, kv.GetNum(g_kvKey_clientRecording) );
	SetConVarInt( g_hLimitLiveTeams, kv.GetNum(g_kvKey_limitLiveTeams) );
	SetConVarInt( g_hLimitTeams, kv.GetNum(g_kvKey_limitTeams) );
	SetConVarInt( g_hPauseMode, kv.GetNum(g_kvKey_pauseMode) );
	SetConVarInt( g_hCollectiveReady, kv.GetNum(g_kvKey_collectiveReady) );
	SetConVarInt( g_hPreventZanshiStrats, kv.GetNum(g_kvKey_noZanshi) );
	SetConVarInt( g_hSuddenDeath, kv.GetNum(g_kvKey_suddenDeath) );
	SetConVarInt( g_hCenteredDisplayRemaining, kv.GetNum(g_kvKey_centered_displayRemaining) );
	SetConVarInt( g_hCenteredDisplayTarget, kv.GetNum(g_kvKey_centered_displayTarget) );
	SetConVarInt( g_hAlltalk, kv.GetNum(g_kvKey_natives_alltalk) );
	SetConVarInt( g_hForceCamera, kv.GetNum(g_kvKey_natives_forceCamera) );
	SetConVarInt( g_hPausable, kv.GetNum(g_kvKey_natives_pausable) );
	SetConVarInt( g_hRoundLimit, kv.GetNum(g_kvKey_roundLimit) );
	SetConVarInt( g_hNeoScoreLimit, kv.GetNum(g_kvKey_natives_scoreLimit) );
	
	kv.GoBack();
	
	decl String:roundLabel[16] = "round";
	new round = 1;
	new i;
	decl String:labelBuffer[16];
	Format(labelBuffer, sizeof(labelBuffer), "%s%i", roundLabel, round);
	
	// Determine the highest played round
	// When recovering from server crash, this is most likely what is desired
	// TODO: Ability to specify where to load in
	while ( kv.JumpToKey(labelBuffer, false) )
	{
//		PrintToServer("Found round %i", round);
		g_roundNumber = round;
		g_jinraiScore[g_roundNumber] = kv.GetNum("jinrai_score");
		g_nsfScore[g_roundNumber] = kv.GetNum("nsf_score");
		
		// Restore player scores etc. for specific round
		kv.JumpToKey("client_values", false);
		decl String:steamID[MAX_STEAMID_LENGTH];
		new xp;
		new deaths;
		for (i = 1; i <= MaxClients; i++)
		{
			if ( !Client_IsValid(i) || !IsClientConnected(i) || !IsClientAuthorized(i) || IsFakeClient(i) )
				continue;
			
			GetClientAuthId( i, AuthId_Steam2, steamID, sizeof(steamID) );
			
			ClientCommand(i, "kill");
			xp = 0;
			deaths = 0;
			
			// Player has xp/deaths stored in KeyValues for this round
			if ( kv.JumpToKey(steamID, false) )
			{
				ClientCommand( i, "jointeam %i", kv.GetNum("team") );
				xp = kv.GetNum("xp");
				deaths = kv.GetNum("deaths");
				
				g_playerXP[i][g_roundNumber] = xp;
				g_playerDeaths[i][g_roundNumber] = deaths;
				
				kv.GoBack();
			}
			
			DataPack pack = new DataPack();
			CreateDataTimer(1.0, Timer_RestoreClientFromKeyValues, pack);
			pack.WriteCell(i);
			pack.WriteCell(xp);
			pack.WriteCell(deaths);
		}
		kv.GoBack();
		
		round++;
		Format(labelBuffer, sizeof(labelBuffer), "%s%i", roundLabel, round);
		
		kv.GoBack();
	}
	round--;
//	PrintToServer("Rounds found total: %i", round);
	
	Format(labelBuffer, sizeof(labelBuffer), "%s%i", roundLabel, round);
	if (! kv.JumpToKey(labelBuffer, false) )
	{
		LogError("Failed restoring round %i labelled \"%s\" from KeyValues", round, labelBuffer);
		delete kv;
		return;
	}
	
	// Restore round specific match settings
	SetConVarInt( g_hRoundLimit, kv.GetNum(g_kvKey_roundLimit) );
	g_usedTimeouts[TEAM_JINRAI] = kv.GetNum(g_kvKey_jinraiPauses);
	g_usedTimeouts[TEAM_NSF] = kv.GetNum(g_kvKey_nsfPauses);
	
	DataPack pack = new DataPack();
	CreateDataTimer(1.0, Timer_RestoreTeamScores, pack);
	pack.WriteCell( kv.GetNum(g_kvKey_jinraiScore) );
	pack.WriteCell( kv.GetNum(g_kvKey_nsfScore) );
	
	g_kv = kv;
	delete kv;
	strcopy(g_kvName, sizeof(g_kvName), fileName);
	
	if (strlen(competitionName) > 0)
		PrintToChatAll("%s Match loaded: %s", g_tag, competitionName);
	else
		PrintToChatAll("%s Match loaded.", g_tag);
	
	// Start restored match right away
	if (goLive)
	{
		if (!g_isLive)
		{
			ToggleLive(true);
		}
		// Already live, subtract current round from count.
		// Everyone was slayed earlier so newround is guaranteed.
		else
		{
			g_roundNumber--;
		}
	}
	// Just announce successful restore, but don't go live
	else
	{
		PrintToChatAll("%s vs %s", jinraiName, nsfName);
		PrintToChatAll("Loaded round: %i", g_roundNumber);
	}
}

/*
bool Kv_IsCompatible(const String:pluginVersion[])
{
	// Todo: actual version check
	if ( !StrEqual(pluginVersion, pluginVersion) )
		return false;
	
	return true;
}
*/

public Action:Timer_RestoreClientFromKeyValues(Handle:timer, DataPack:pack)
{
	pack.Reset();
	new client = pack.ReadCell();
	new xp = pack.ReadCell();
	new deaths = pack.ReadCell();
	
	SetPlayerXP(client, xp);
	SetPlayerDeaths(client, deaths);
	UpdatePlayerRank(client);
}

public Action:Timer_RestoreTeamScores(Handle:timer, DataPack:pack)
{
	pack.Reset();
	new jinraiScore = pack.ReadCell();
	new nsfScore = pack.ReadCell();
	
	SetConVarInt(g_hJinraiScore, jinraiScore);
	SetConVarInt(g_hNSFScore, nsfScore);
	
	g_jinraiScore[g_roundNumber] = jinraiScore;
	g_nsfScore[g_roundNumber] = nsfScore;
	
	SetTeamScore(TEAM_JINRAI, jinraiScore);
	SetTeamScore(TEAM_NSF, nsfScore);
}

void Kv_Menu_Details_GetParallelFile(client, const String:kvFileName[], bool:getPrevious = false)
{
	if ( !Client_IsValid(client) || !IsClientInGame(client) )
		return;
	
	if (strlen(kvFileName) < 1)
	{
		LogError("Attempted to restore KeyValues from zero length filename");
		return;
	}
	
	if ( !Kv_IsValidFile(kvFileName) )
	{
		LogError("KeyValues filename doesn't contain .kv extension. This shouldn't happen.");
		return;
	}
	
	decl String:fullPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, fullPath, sizeof(fullPath), "%s", g_kvPath);
	
	new Handle:dir = OpenDirectory(fullPath);
	if (!DirExists(fullPath) || dir == null)
	{
		LogError("Failed to open KeyValues directory path: %s", fullPath);
		return;
	}
	
	decl String:filePath[PLATFORM_MAX_PATH];
	Format(filePath, sizeof(filePath), "%s/%s", fullPath, kvFileName);
	if ( !FileExists(filePath) )
	{
		LogError("KeyValues file %s does not exist.", fullPath);
		return;
	}
	
	new i;
	new locatedPos;
	new results;
	new max_files = 128;
	decl String:buffer[max_files][PLATFORM_MAX_PATH]; // fixme: this is expensive :-( dynamic array instead?
	new bool:locatedFile;
	new bool:servedFile;
	FileType type;
	while ( ReadDirEntry(dir, buffer[i], PLATFORM_MAX_PATH, type) )
	{
		results++;
		
//		PrintToServer("Testing %s against %s", buffer[i], kvFileName);
		
		if (results > max_files)
		{
			LogError("Warning: Over %i file results found. Breaking out of the loop to avoid unresponsiveness.", max_files);
			break;
		}
		
		// Not a file
		if (type != FileType_File)
			continue;
		
		// Not a .kv file
		if ( !Kv_IsValidFile(buffer[i]) )
			continue;
		
		if (locatedFile)
		{
			if (getPrevious)
			{
				new prevIndex = locatedPos - 1;
				// We're at the first key, can't return previous.
				// Return current key and notify via boolean
				if (prevIndex < 0)
				{
					Menu_KvDetails(client, buffer[locatedPos], true);
				}
				else
				{
					Menu_KvDetails(client, buffer[prevIndex]);
				}
			}
			else
			{
				Menu_KvDetails(client, buffer[i]);
			}
			
			servedFile = true;
			break;
		}
		else if ( StrEqual(buffer[i], kvFileName) )
		{
//			PrintToServer("Found it!");
			locatedPos = i;
			locatedFile = true;
		}
		
		i++;
	}
	
	// Failed to serve any next file after a match. Probably reached end of folder.
	if (locatedFile && !servedFile)
	{
		if (getPrevious)
		{
			new prevIndex = locatedPos - 1;
			// We're at the first key, can't return previous.
			// Return current key and notify via boolean
			if (prevIndex < 0)
			{
				Menu_KvDetails(client, buffer[i], true);
			}
			else
			{
//				PrintToServer("Going to previous index %i: %s", prevIndex, buffer[prevIndex]);
				Menu_KvDetails(client, buffer[prevIndex]);
			}
			
			servedFile = true;
		}
		else
		{
			Menu_KvDetails(client, buffer[locatedPos], true);
		}
	}
	
	CloseHandle(dir);
}
#endif

// Purpose: Store current epoch globally so multiple functions can use it for identifying themselves
void GenerateEpoch()
{
	new Handle:hTimeAdjustment = FindConVar("sm_time_adjustment");
	g_epoch = GetTime() + GetConVarInt(hTimeAdjustment);
	CloseHandle(hTimeAdjustment);
}

void SetGameState(state)
{
	if (state != GAMESTATE_NORMAL && state != GAMESTATE_WARMUP)
	{
		LogError("Invalid gamestate %i", state);
		return;
	}
	
	GameRules_SetProp("m_iGameState", state);
}

void CancelLiveCountdown()
{
	// Kill the live countdown timer
	if (g_hTimer_LiveCountdown != INVALID_HANDLE)
	{
		KillTimer(g_hTimer_LiveCountdown);
		g_hTimer_LiveCountdown = INVALID_HANDLE;
		
		if (g_hTimer_LiveCountdown != INVALID_HANDLE)
			LogError("Failed killing timer g_hTimer_LiveCountdown");
	}
	
	// Kill the actual live toggle timer
	if (g_hTimer_GoLive != INVALID_HANDLE)
	{
		KillTimer(g_hTimer_GoLive);
		g_hTimer_GoLive = INVALID_HANDLE;
		
		if (g_hTimer_GoLive != INVALID_HANDLE)
			LogError("Failed killing timer g_hTimer_GoLive");
	}
	
	g_isLiveCountdown = false; // We are no longer in a live countdown
	g_liveTimer = g_liveTimer_OriginalValue; // Reset live countdown timer to its original value
	
	PrintToChatAll("%s Live countdown cancelled.", g_tag);
}