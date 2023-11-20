// Double-include prevention
#if defined _base_included_
	#endinput
#endif
#define _base_included_

#pragma semicolon 1
#pragma newdecls required

#define PLUGIN_VERSION "1.0.0"

#define MAX_STEAMID_LENGTH 44
#define MAX_ROUNDS_PLAYED 255 // This is just a random large number used for 1d int arrays because it's cheap and simple. A single comp game should never have more rounds than this to avoid weirdness.

char g_tag[] = "[COMP]";
char g_teamName[][64] = {
	"Unassigned",
	"Spectator",
	"Jinrai",
	"NSF"
};
char g_livePlayers[NEO_MAXPLAYERS+1][MAX_STEAMID_LENGTH]; // Array for storing SteamIDs to allow disconnected players rejoining when live
char g_menuSoundOk[] = "buttons/button14.wav";
char g_menuSoundCancel[] = "buttons/combine_button7.wav";
char g_soundLive[] = "buttons/button17.wav";
char g_killInfoBuffer[NEO_MAXPLAYERS+1][512];

int g_assignedTeamWhenLive[NEO_MAXPLAYERS+1];
int g_jinraiScore[MAX_ROUNDS_PLAYED];
int g_liveTimer = 5;
int g_liveTimer_OriginalValue;
int g_loggingEnabled;
int g_nsfScore[MAX_ROUNDS_PLAYED];
int g_pauseReason;
int g_pausingTeam;
int g_roundNumber;
int g_spamTimerCount;
int g_unpauseTimer = 10;
int g_unpauseTimer_OriginalValue;
int g_usedTimeouts[sizeof(g_teamName)];
int g_playersAlive[sizeof(g_teamName)];
int g_winner;
int g_health[NEO_MAXPLAYERS+1];
int g_damageDealt[NEO_MAXPLAYERS+1][NEO_MAXPLAYERS+1];
int g_playerDeaths[NEO_MAXPLAYERS+1][MAX_ROUNDS_PLAYED];	// Players' deaths for each round
int g_playerXP[NEO_MAXPLAYERS+1][MAX_ROUNDS_PLAYED];		// Players' XP for each round
int g_playerSteamID[NEO_MAXPLAYERS+1];						// Players' SteamID as int32
int g_furthestPlayedRound;
int g_ghostCapturingTeam;
int g_epoch;

float g_fRoundTime;
float g_fGhostOvertime;
float g_fGhostOvertimeTick;
bool g_bGhostOvertimeFirstTick;

bool g_isAlltalkByDefault;
bool g_isExpectingOverride;
bool g_isExpectingUnPauseTimer;
bool g_isLive;
bool g_isPaused;
bool g_isReady[NEO_MAXPLAYERS+1];
bool g_isSourceTVRecording;
bool g_isSpawned[NEO_MAXPLAYERS+1];
bool g_isTeamReadyForUnPause[sizeof(g_teamName)]; // Unassigned, spec, jinrai, nsf
bool g_isWantingOverride[sizeof(g_teamName)]; // Unassigned, spec, jinrai, nsf
bool g_shouldPause;
bool g_shouldShowKillInfo[NEO_MAXPLAYERS+1];
bool g_isLiveCountdown;
bool g_confirmLiveEnd;
bool g_isCurrentlyUnPausing;
bool g_isCurrentlyRestoringRound;
bool g_survivedLastRound[NEO_MAXPLAYERS+1];
bool g_isSuddenDeathRound;
bool g_isRecording[NEO_MAXPLAYERS+1];

// ConVars
Handle g_hAlltalk, g_hCommsBehaviour, g_hCompetitionName, g_hForceCamera,
	g_hLogMode, g_hMatchSize, g_hMaxPauseLength, g_hMaxPauseLength_Technical,
	g_hMaxTimeouts, g_hNeoScoreLimit, g_hNeoRestartThis, g_hPausable,
	g_hRoundLimit, g_hRoundStyle, g_hSourceTVEnabled, g_hSourceTVPath,
	g_hJinraiName, g_hNSFName, g_hKillVersobity, g_hClientRecording,
	g_hLimitLiveTeams, g_hLimitTeams, g_hVerbosityDelay, g_hPauseMode,
	g_hCollectiveReady, g_hPreventZanshiStrats, g_hRoundTime, g_hJinraiScore,
	g_hNSFScore, g_hSuddenDeath, g_hCenteredDisplayRemaining,
	g_hCenteredDisplayTarget, g_hGhostOvertimeDecay, g_hGhostOvertimeGrace,
	g_hGhostOvertimeDecayExp, g_hGhostOvertimeGraceReset,
	g_hCenteredDisplayDivider;

// Timers
Handle g_hTimer_LiveCountdown = INVALID_HANDLE,
	g_hTimer_GoLive = INVALID_HANDLE,
	g_hTimer_UnPause = INVALID_HANDLE,
	g_hTimer_UnPause_Countdown = INVALID_HANDLE,
	g_hTimer_UnPause_HalfLeft = INVALID_HANDLE,
	g_hTimer_GhostOvertime = INVALID_HANDLE;

enum {
	CVAR_NONE = 0,
	CVAR_COMPETITION_NAME,
	CVAR_JINRAI_NAME,
	CVAR_NSF_NAME,
	CVAR_MAX_ROUNDS,
	CVAR_PAUSE_AMOUNT
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

	int players = GetPlayerCount();
	int playersReady = GetPlayersReady();

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

public Action Command_ReadyList(int client, int argc)
{
	ReplyToCommand(client, "%s List of readied players is printed in your console.", g_tag);

	PrintToConsole(client, "\n%s Players and their READY state:", g_tag);
	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i))
			continue;

		int team = GetClientTeam(i);

		if (team != TEAM_JINRAI && team != TEAM_NSF)
			continue;

		PrintToConsole(client, "[%s]\t%N",
			g_isReady[i] ? "READY" : "NOT READY",
			i
		);
	}
	PrintToConsole(client, "\n");

	return Plugin_Handled;
}

// Assumes valid client
void PlayerRecord(int client)
{
	char steamID[MAX_STEAMID_LENGTH];
	GetClientAuthId(client, AuthId_Steam2, steamID, sizeof(steamID));

	char timestamp[15];
	FormatTime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M");

	char competitionName[32];
	GetConVarString(g_hCompetitionName, competitionName, sizeof(competitionName));

	char replayName[sizeof(steamID)+sizeof(timestamp)+sizeof(competitionName)+2];

	if (strlen(competitionName) > 0)
		Format(replayName, sizeof(replayName), "%s_%s_%s", competitionName, timestamp, steamID);
	else
		Format(replayName, sizeof(replayName), "%s_%s", timestamp, steamID);

	// Clean up any non alphanumeric characters from the string
	char replayBuffer[sizeof(replayName) + 1];
	int bufferPos;

	for (int j = 0; j < sizeof(replayName); j++)
	{
		if (IsCharAlpha(replayName[j]) || IsCharNumeric(replayName[j]) || !IsCharSpace(replayName[j]))
			replayBuffer[bufferPos++] = replayName[j];
	}
	replayBuffer[bufferPos] = '\0'; // Terminate the string with 0

	ReplaceString(replayBuffer, sizeof(replayBuffer), ":", ""); // Remove : characters from the SteamID. To-do: proper sanitization
	strcopy(replayName, sizeof(replayName), replayBuffer);

	ClientCommand(client, "stop"); // Stop any previous recording, jic. This does nothing if a demo isn't running.
	ClientCommand(client, "record %s", replayName); // Start recording. Format demo filename here. Note: replays can be overwritten using this.
	g_isRecording[client] = true;
}

void ToggleRecording()
{
	if ( GetConVarBool(g_hClientRecording) == false )
		return;

	if (g_isLive) // Start recording
	{
		for (int i = 1; i <= MaxClients; i++)
		{
			if (!IsClientInGame(i) || IsFakeClient(i))
				continue;

			PlayerRecord(i);
		}
	}
	else // Stop recording
	{
		for (int i = 1; i <= MaxClients; i++)
		{
			if (!IsClientInGame(i) || IsFakeClient(i))
				continue;

			ClientCommand(i, "stop");
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

public Action Timer_GoLive(Handle timer)
{
	EmptyScore(); // Make sure everyone starts private so we avoid the no weapon spawn bug
	CreateTimer(0.1, Timer_ToggleLive); // A slight delay so the score reset above takes effect in time

	g_hTimer_GoLive = INVALID_HANDLE;
	return Plugin_Stop;
}

public Action Timer_ToggleLive(Handle timer)
{
	ToggleLive();
	return Plugin_Stop;
}

public Action Timer_CancelLiveEndConfirmation(Handle timer)
{
	g_confirmLiveEnd = false;
	return Plugin_Stop;
}

void ToggleLive()
{
	g_isLive = !g_isLive;

	if (g_isLive)
	{
		SetGameState(GAMESTATE_ROUND_ACTIVE);
		GenerateEpoch();

		g_isWantingOverride[TEAM_JINRAI] = false;
		g_isWantingOverride[TEAM_NSF] = false;
		g_isExpectingOverride = false;
		g_isLiveCountdown = false;
		g_winner = 0;

		// Save connected players' SteamIDs so they can be allowed to rejoin their teams if disconnected
		for (int i = 1; i <= MaxClients; i++)
		{
			if (!IsClientInGame(i) || IsFakeClient(i))
				continue;

			if (!IsClientAuthorized(i))
			{
				PrintToChatAll("%s Player \"%N\" has no SteamID authorized yet! Stopped going live. Please try again in a moment.",
					g_tag, i
				);
				g_isLive = false;
				return;
			}

			char steamID[MAX_STEAMID_LENGTH];
			GetClientAuthId(i, AuthId_Steam2, steamID, sizeof(steamID));
			PrintToServer("SteamID of %i: %s", i, steamID);
			int team = GetClientTeam(i);

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

				int acc_id = GetSteamAccountID(i);
				if (acc_id == 0)
				{
					LogError("acc_id was zero for %N (%d), this should never happen", i, i);
					continue;
				}
				g_playerSteamID[i] = acc_id;
			}
		}

		// Set convars for competitive
		SetConVarInt(g_hAlltalk, GetDesiredCommsBehaviour());
		SetConVarBool(g_hPausable, false);
		SetConVarBool( g_hForceCamera, true );
		SetConVarInt(g_hNeoScoreLimit, 99); // Make sure Neotokyo's own max round count is maxed out so it won't interfere

		NeoRestartThis();

		g_loggingEnabled = GetConVarBool(g_hLogMode);

		ToggleRecording();

		// Announce game start, possible tournament name, and teams
		char competitionName[32];
		GetConVarString(g_hCompetitionName, competitionName, sizeof(competitionName));

		// If competition name hasn't been specified
		if (strlen(competitionName) < 1)
			strcopy(competitionName, sizeof(competitionName), "Competitive Neotokyo");

		char liveTitle[sizeof(g_tag) + sizeof(competitionName) + sizeof(g_teamName[]) * 2 + 6];
		Format(liveTitle, sizeof(liveTitle), "%s %s: %s - %s", g_tag, competitionName, g_teamName[TEAM_JINRAI], g_teamName[TEAM_NSF]);

		PrintToChatAll(liveTitle);

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

		SetGameState(GAMESTATE_WAITING_FOR_PLAYERS);
	}

	ToggleSourceTV(); // This has to be called after modifying the g_isLive boolean
}

void NeoRestartThis()
{
	SetConVarBool(g_hNeoRestartThis, true);

	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i))
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
					return;
				}

				g_isExpectingUnPauseTimer = true;

				// Close any previous unpause timer handles
				KillTimers_UnPause();

				float fPauseLength;

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

				PrintToChatAll("%s Pause maximum duration: %i seconds.", g_tag, RoundToNearest(fPauseLength));
				PrintToChatAll("%s Use !unpause when ready.", g_tag);

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
			// Pause
			if (g_isPaused)
			{
				g_isExpectingUnPauseTimer = true;

				GameRules_SetPropFloat( "m_fRoundTimeLeft", 0.0 ); // Set round time to zero. This instantly ends current round and rings the tie/win music, which works to alert players about upcoming pause.

				float transitionTime = 5.0;
				float fPauseLength;

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
				GameRules_SetProp("m_iRoundNumber", g_roundNumber - 1);
				SetGameState(GAMESTATE_ROUND_ACTIVE);

				RestoreRound(g_roundNumber, false); // Restore everyone's XP, deaths and ranks to the state before pausing

				g_isTeamReadyForUnPause[TEAM_JINRAI] = false;
				g_isTeamReadyForUnPause[TEAM_NSF] = false;
			}
		}
	}
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

public Action Timer_EnterPauseRound(Handle timer)
{
	SetGameState(GAMESTATE_WAITING_FOR_PLAYERS);

	float pauseLength;
	if (g_pauseReason == REASON_TECHNICAL)
		pauseLength = GetConVarFloat(g_hMaxPauseLength_Technical);
	else
		pauseLength = GetConVarFloat(g_hMaxPauseLength);

	GameRules_SetPropFloat( "m_fRoundTimeLeft", pauseLength ); // Pause round uses max pause length roundtime

	for (int i = 0; i < 3; i++)
	{
		PrintToChatAll("GAME IS PAUSED");
	}

	return Plugin_Stop;
}

public Action Timer_TogglePauseLoop(Handle timer)
{
	TogglePauseLoop();

	return Plugin_Stop;
}

void TogglePauseLoop()
{
	int invalidClients;
	int unspawnedClients[2];
	bool forcePause;

	for (int i = 1; i <= MaxClients; i++) // The server can't exec "pause" for some reason. So, instead, use the first valid client to execute it.
	{
		unspawnedClients[0] = unspawnedClients[1];
		if (IsClientInGame(i))
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
				CreateTimer(0.05, Timer_DisablePause); // Pausable cvar toggling is too fast to register the pause without a tiny delay here. 0.01 was occasionally too fast, 0.05 seems pretty safe.

				break;
			}
		}

		else
			invalidClients++;

		if (i == MaxClients && unspawnedClients[1] > 0) // This will keep looping in 1 second intervals for as long as there are valid unspawned clients
		{
			if (GetGameTime() > g_fRoundTime + 13.9)
			{
				forcePause = true;
				i = 0; // Reset loop
				continue;
			}

			CreateTimer(1.0, Timer_TogglePauseLoop);
		}
		else if (invalidClients == MaxClients)
			LogError("Attempted pause command whilst no valid clients were connected inside TogglePause. This shouldn't happen.");
	}
}

public Action Timer_TogglePause(Handle timer)
{
	TogglePause();
	return Plugin_Stop;
}

public Action Timer_DisablePause(Handle timer)
{
	SetConVarBool(g_hPausable, false);
	return Plugin_Stop;
}

public Action Timer_SpamLive(Handle timer)
{
	// Print the LIVE message X times, then kill the repeat timer calling this
	int spamLiveTimes = 3;
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

public Action Timer_Unpause(Handle timer)
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

public Action Timer_UnpauseNotice_Half(Handle timer)
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

public Action Timer_StartTenSecondsLeft(Handle timer)
{
	if (g_isPaused)
		CreateTimer(1.0, Timer_Unpause_SecondsCountdown, _, TIMER_REPEAT); // This repeat timer is killed inside the callback

	g_hTimer_UnPause_Countdown = INVALID_HANDLE;

	return Plugin_Handled;
}

public Action Timer_Live_SecondsCountdown(Handle timer)
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

public Action Timer_Unpause_SecondsCountdown(Handle timer)
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

void InitDirectory(const char[] sDir)
{
	char sPieces[32][PLATFORM_MAX_PATH];
	char sPath[PLATFORM_MAX_PATH];
	int iNumPieces = ExplodeString(sDir, "/", sPieces, sizeof(sPieces), sizeof(sPieces[]));

	for(int i = 0; i < iNumPieces; i++)
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
	}

	else
	{
		if (!GetConVarBool(g_hSourceTVEnabled) || !g_isLive)
			return;

		if (!IsSourceTVBotConnected())
		{
			return;
		}

		char demoPath[PLATFORM_MAX_PATH];
		GetConVarString(g_hSourceTVPath, demoPath, sizeof(demoPath));

		char timestamp[16];
		FormatTime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M");

		char mapName[32];
		GetCurrentMap(mapName, sizeof(mapName));

		char competitionName[32];
		GetConVarString(g_hCompetitionName, competitionName, sizeof(competitionName));

		char replayName[sizeof(mapName)+sizeof(timestamp)+sizeof(competitionName)+2];

		if (strlen(competitionName) > 0)
			Format(replayName, sizeof(replayName), "%s_%s_%s_", competitionName, timestamp, mapName);

		else
			Format(replayName, sizeof(replayName), "%s_%s_", timestamp, mapName);

		// Concatenate epoch to the end of filename. This can be helpful to connect a replay to related KeyValues entry
		char epochBuffer[16];
		IntToString( g_epoch, epochBuffer, sizeof(epochBuffer) );
		StrCat(replayName, sizeof(replayName), epochBuffer);

		// Clean up any non alphanumeric characters from the string
		char replayBuffer[sizeof(replayName) + 1];
		int bufferPos;

		for (int j = 0; j < sizeof(replayName); j++)
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
	}
}

// This won't play a sound if used during an engine pause
void PlayLiveBeep()
{
	float volume = 0.5;	// Volume between 0.0 - 1.0 (original volume is 1.0)
	int pitch = 175;	// Pitch between 0 - 255 (original pitch is 100)
	EmitSoundToAll(g_soundLive, _, _, _, _, volume, pitch);
}

bool IsSourceTVBotConnected()
{
	// Get the SourceTV bot's name cvar, and figure out if bot is connected
	ConVar tvname = FindConVar("tv_name");
	char botName[MAX_NAME_LENGTH];
	tvname.GetString(botName, sizeof(botName));

	char clientName[MAX_NAME_LENGTH];
	for (int i = 1; i <= MaxClients; i++)
	{
		// We are looking for a valid fake client (bot)
		if (!IsClientInGame(i) || !IsFakeClient(i))
			continue;

		GetClientName(i, clientName, sizeof(clientName));

		if (StrEqual(clientName, botName) || StrEqual(clientName, "SourceTV")) // Also check for default name "SourceTV", because sometimes tv_name doesn't work when bot first spawns
		{
			return true;
		}
	}

	return false;
}

// Return players in Jinrai + NSF total
int GetPlayerCount()
{
	int players;

	for (int i = 1; i <= MaxClients; i++)
	{
		if ( !IsClientInGame(i) || IsFakeClient(i) )
			continue;

		int team = GetClientTeam(i);

		if (team != TEAM_JINRAI && team != TEAM_NSF)
			continue;

		players++;
	}

	return players;
}

int GetPlayersReady()
{
	int playersReady;

	for (int i = 1; i <= MaxClients; i++)
	{
		if ( !IsClientInGame(i) || !g_isReady[i] || IsFakeClient(i) )
			continue;

		int team = GetClientTeam(i);

		if (team != TEAM_JINRAI && team != TEAM_NSF)
			continue;

		playersReady++;
	}

	return playersReady;
}

int GetDesiredCommsBehaviour()
{
	int behaviour = GetConVarInt(g_hCommsBehaviour); // 0: disable alltalk, 1: enable alltalk, 2: use whichever initial alltalk value (tracked with g_isAlltalkByDefault)

	if (behaviour <= 1)
		return behaviour;

	else
		return g_isAlltalkByDefault;
}

int GetOtherTeam(int team)
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

void LogCompetitive(const char[] message, any ...)
{
	if (!g_loggingEnabled)
		return;

	char formatMsg[256];
	VFormat(formatMsg, sizeof(formatMsg), message, 2);

	char loggingPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, loggingPath, sizeof(loggingPath), "logs/competitive");

	char debugFile[] = "logfile.log";
	Format(loggingPath, sizeof(loggingPath), "%s/%s", loggingPath, debugFile);

	Handle hFile = OpenFile(loggingPath, "a");

	WriteFileLine(hFile, formatMsg);

	CloseHandle(hFile);
}

#if SOURCEMOD_V_MAJOR <= 1 && SOURCEMOD_V_MINOR < 9
void PrintToConsoleAll(const char[] message, any ...)
{
	char formatMsg[256];
	VFormat(formatMsg, sizeof(formatMsg), message, 2);

	for (int i = 1; i <= MaxClients; i++)
	{
		if ( !IsClientInGame(i) || IsFakeClient(i) )
			continue;

		PrintToConsole(i, formatMsg);
	}
}
#endif

void UpdatePlayersAlive() // Updates g_playersAlive array
{
	int jinraiAlive, nsfAlive;

	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i))
			continue;

		if (!IsPlayerAlive(i))
			continue;

		int team = GetClientTeam(i);

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
	int jinraiPlayers;
	int nsfPlayers;

	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i))
			continue;

		int team = GetClientTeam(i);

		if (team == TEAM_JINRAI)
			jinraiPlayers++;

		else if (team == TEAM_NSF)
			nsfPlayers++;
	}

	// We only call this in round start, so it's safe to assume any team players are alive.
	g_playersAlive[TEAM_JINRAI] = jinraiPlayers;
	g_playersAlive[TEAM_NSF] = nsfPlayers;
}

// Copies the value "val" into each of the first "size" indices.
void MemSetAny(any[] arr, int size, any val)
{
	for (int i = 0; i < size; ++i)
	{
		arr[i] = val;
	}
}

// For a 2D array "arr", for its first "size_i" indices,
// copies the value "val" into each of the first "size_j" indices.
void MemSetAny2D(any[][] arr, int size_i, int size_j, any val)
{
	for (int i = 0; i < size_i; ++i)
	{
		MemSetAny(arr[i], size_j, val);
	}
}

// Copies the first "size_j" chars of string "val" into each of the first
// "size_i" indices.
void MemSetChar2D(char[][] arr, int size_i, int size_j, const char[] val)
{
	for (int i = 0; i < size_i; ++i)
	{
		strcopy(arr[i], size_j, val);
	}
}

void ResetGlobalVariables() // Make sure all global variables are reset properly
{
	g_roundNumber = 0;
	g_furthestPlayedRound = 0;

	SetConVarInt(g_hJinraiScore, 0);
	SetConVarInt(g_hNSFScore, 0);

	MemSetAny(g_playerSteamID, sizeof(g_playerSteamID), 0);
	MemSetAny2D(g_playerXP, sizeof(g_playerXP), sizeof(g_playerXP[]), 0);
	MemSetAny(g_jinraiScore, sizeof(g_jinraiScore), 0);
	MemSetAny(g_nsfScore, sizeof(g_nsfScore), 0);
	MemSetAny(g_usedTimeouts, sizeof(g_usedTimeouts), 0);

	MemSetAny(g_assignedTeamWhenLive, sizeof(g_assignedTeamWhenLive), -1);

	MemSetChar2D(g_livePlayers, sizeof(g_livePlayers), sizeof(g_livePlayers[]), "");
	MemSetChar2D(g_killInfoBuffer, sizeof(g_killInfoBuffer), sizeof(g_killInfoBuffer[]), "");

	g_isExpectingOverride		= false;
	g_isExpectingUnPauseTimer	= false;
	g_isLive					= false;
	g_isPaused					= false;
	g_isSourceTVRecording		= false;
	g_shouldPause				= false;
	g_confirmLiveEnd			= false;
	g_isSuddenDeathRound		= false;

	MemSetAny(g_isReady, sizeof(g_isReady), false);
	MemSetAny(g_isSpawned, sizeof(g_isSpawned), false);
	MemSetAny(g_shouldShowKillInfo, sizeof(g_shouldShowKillInfo), false);
	MemSetAny(g_isTeamReadyForUnPause, sizeof(g_isTeamReadyForUnPause), false);
	MemSetAny(g_isWantingOverride, sizeof(g_isWantingOverride), false);

	if (g_hTimer_GhostOvertime != INVALID_HANDLE)
	{
		CloseHandle(g_hTimer_GhostOvertime);
		g_hTimer_GhostOvertime = INVALID_HANDLE;
	}
}

void ShowKillInfoNextRound(int client, const char[] message)
{
	strcopy(g_killInfoBuffer[client], sizeof(g_killInfoBuffer), message);
	g_shouldShowKillInfo[client] = true;
}

// Returns the number of admins printed to.
int PrintToAdmins(bool toChat = true, bool toConsole = false,
	const char[] message, any ...)
{
	char formatMsg[256];
	VFormat(formatMsg, sizeof(formatMsg), message, 4);

	int n;
	for (int i = 1; i <= MaxClients; i++)
	{
		if ( !IsClientInGame(i) || !IsAdmin(i) )
			continue;

		if (toChat)
			PrintToChat(i, formatMsg);

		if (toConsole)
			PrintToConsole(i, formatMsg);

		if (toChat || toConsole)
		{
			++n;
		}
	}
	return n;
}

void RestoreRound(int roundNumber, bool isZanshiRestore)
{
	if (roundNumber > g_furthestPlayedRound || roundNumber <= 0)
	{
		LogError("Attempted to restore a nonexistant round %i. Furthest played round is %i.", roundNumber, g_furthestPlayedRound);

		int n_admins = PrintToAdmins(_, _,
			"Failed restoring round %i. Check SM error logs for details.",
			roundNumber
		);

		if (n_admins == 0)
			PrintToChatAll("Round restore failed. Contact server admins for help.");
		return;
	}

	if (isZanshiRestore == false)
	{
		// Only edit round number if restoring previous match state (unpause etc)
		g_roundNumber = roundNumber;

		// Only recover previous XP/ranks if restoring previous match state (unpause etc)
		for (int i = 1; i <= MaxClients; i++)
		{
			if ( !IsClientInGame(i) || IsFakeClient(i) )
				continue;

			SetEntProp(i, Prop_Data, "m_iDeaths", g_playerDeaths[i][roundNumber]); // Restore deaths
			SetPlayerXP(i, g_playerXP[i][roundNumber]); // Restore XP
			UpdatePlayerRank(i);
		}
	}

	SetTeamScore(TEAM_JINRAI, g_jinraiScore[roundNumber]); // Set Jinrai score
	SetTeamScore(TEAM_NSF, g_nsfScore[roundNumber]); // Set NFS score
}

public Action Timer_ClearGhostCapper(Handle timer)
{
	g_ghostCapturingTeam = TEAM_NONE;
	return Plugin_Stop;
}

void EmptyScore()
{
	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i))
		{
			continue;
		}
		SetPlayerDeaths(i, 0);
		SetPlayerXP(i, 0);
	}
}

static void Notify(const char[] message, any ...)
{
	char formatMsg[256];
	VFormat(formatMsg, sizeof(formatMsg), message, 2);

	PrintToChatAll(formatMsg);
	PrintToConsoleAll(formatMsg);
	LogCompetitive(formatMsg);
}

void CheckMatchState()
{
	int roundLimit = GetConVarInt(g_hRoundLimit);

	// Win condition: Best of X rounds
	if (GetConVarInt(g_hRoundStyle) == 1)
	{
		// ** Announce match state below **
		// Jinrai wins, NSF can't catch up anymore
		if ( !g_isSuddenDeathRound && g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber] + roundLimit - (g_roundNumber - 1) ) // (g_roundNumber - 1) because we're counting rounds by incrementing inside the event. So first round is 1, not 0.
		{
			Notify("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber]);

			g_winner = TEAM_JINRAI;
		}

		// NSF wins, Jinrai can't catch up anymore
		else if ( !g_isSuddenDeathRound && g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber] + roundLimit - (g_roundNumber - 1))
		{
			Notify("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_NSF], g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);

			g_winner = TEAM_NSF;
			g_isLive = false;
		}

		else if (g_isSuddenDeathRound)
		{
			if (g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber])
			{
				Notify("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber]);
				g_winner = TEAM_JINRAI;
			}
			else if (g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber])
			{
				Notify("%s %s WINS %i - %i", g_tag, g_teamName[TEAM_NSF], g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);
				g_winner = TEAM_NSF;
			}
			else
			{
				Notify("%s SUDDEN DEATH. Next team to score wins.", g_tag);
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
					Notify("%s SUDDEN DEATH. Next team to score wins.", g_tag);
					return;
				}
			}

			// No sudden death. End match in a tie.
			else
			{
				Notify("%s MATCH TIED %i - %i", g_tag, g_nsfScore[g_roundNumber], g_jinraiScore[g_roundNumber]);
			}
		}

		// Check if game ended, and toggle off live mode if appropriate
		if ((g_jinraiScore[g_roundNumber] > g_nsfScore[g_roundNumber] + roundLimit - (g_roundNumber - 1)) || (g_nsfScore[g_roundNumber] > g_jinraiScore[g_roundNumber] + roundLimit - (g_roundNumber - 1)) || ((g_roundNumber - 1) == roundLimit))
		{
			ToggleLive();
			LogCompetitive("Competitive match ended.");
		}

		// Match point, one team has almost won
		else if (IsMatchPoint())
		{
			Notify("%s Round %i/%i (MATCH POINT)", g_tag, g_roundNumber, roundLimit);
		}

		// Just another round, simply announce round number
		else
		{
			// Don't announce match number during the initial "LIVE LIVE LIVE" chat spam
			if (g_spamTimerCount <= 1)
			{
				Notify("%s Round %i/%i (MATCH POINT)", g_tag, g_roundNumber, roundLimit);
			}
		}
	}
	// Win condition: First to X rounds
	else
	{
		// Jinrai reached score required to win
		if (g_jinraiScore[g_roundNumber] == roundLimit)
		{
			Notify("%s %s WINS %i - %i",
				g_tag, g_teamName[TEAM_JINRAI], g_jinraiScore[g_roundNumber],
				g_nsfScore[g_roundNumber]);

			g_winner = TEAM_JINRAI;

			ToggleLive();
			LogCompetitive("Competitive match ended.");
		}
		// NSF reached score required to win
		else if (g_nsfScore[g_roundNumber] == roundLimit)
		{
			Notify("%s %s WINS %i - %i",
				g_tag, g_teamName[TEAM_NSF], g_nsfScore[g_roundNumber],
				g_jinraiScore[g_roundNumber]);

			g_winner = TEAM_NSF;

			ToggleLive();
			LogCompetitive("Competitive match ended.");
		}
		// This should never happen
		else if (g_nsfScore[g_roundNumber] > roundLimit ||
			g_jinraiScore[g_roundNumber] > roundLimit)
		{
			ThrowError("Team has reached score higher than required to win a match. \
Jinrai %i - NSF %i, round %i in a first to %i points game.",
			g_jinraiScore[g_roundNumber], g_nsfScore[g_roundNumber], g_roundNumber,
			roundLimit);
		}
		else if (g_roundNumber > 1)
		{
			Notify("%s Round %i", g_tag, g_roundNumber);
		}
	}
}

void CheckGhostcapPlugin()
{
	// Anti-timeout cvar is disabled, no need to check for ghostcap plugin
	if ( !GetConVarBool(g_hPreventZanshiStrats) )
		return;

	Handle ghostcapPlugin = FindConVar("sm_ntghostcap_version");

	// Look for ghost cap plugin's version variable
	if (ghostcapPlugin == null)
	{
		char ghostcapUrl[] = "https://github.com/softashell/nt-sourcemod-plugins";
		LogError("Using sm_competitive_nozanshi 1 setting requires Soft as HELL's Ghost cap event plugin, version 1.5.4 or newer: %s", ghostcapUrl);
		SetConVarBool(g_hPreventZanshiStrats, false);
	}
}

bool DidPlayerReallySpawn(int client)
{
	if (!IsClientInGame(client))
	{
		return false;
	}

	int team = GetClientTeam(client);
	if (team != TEAM_JINRAI && team != TEAM_NSF)
	{
		return false;
	}

	float currentTime = GetGameTime();
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

	char message[24];
	char message_nsf[24];

	char sDivider[10];
	GetConVarString(g_hCenteredDisplayDivider, sDivider, sizeof(sDivider));

	// Just display numbers remaining
	if (GetConVarInt(g_hCenteredDisplayRemaining) == 1)
	{
		Format(message, sizeof(message), "%i %s %i",
			g_playersAlive[TEAM_JINRAI], sDivider, g_playersAlive[TEAM_NSF]);
		Format(message_nsf, sizeof(message_nsf), "%i %s %i",
			g_playersAlive[TEAM_NSF], sDivider, g_playersAlive[TEAM_JINRAI]);
	}
	// Display team name + numbers remaining
	else
	{
		Format(message, sizeof(message), "JIN %i %s %i NSF",
			g_playersAlive[TEAM_JINRAI], sDivider, g_playersAlive[TEAM_NSF]);
		Format(message_nsf, sizeof(message_nsf), "NSF %i %s %i JIN",
			g_playersAlive[TEAM_NSF], sDivider, g_playersAlive[TEAM_JINRAI]);
	}

	bool specOnly = (GetConVarInt(g_hCenteredDisplayTarget) == 1);
	bool deadOnly = (GetConVarInt(g_hCenteredDisplayTarget) == 2);

	for (int i = 1; i <= MaxClients; i++)
	{
		if ( !IsClientInGame(i) )
			continue;

		int team = GetClientTeam(i);

		if (specOnly && team != TEAM_SPECTATOR)
			continue;

		if (deadOnly && team != TEAM_SPECTATOR && IsPlayerAlive(i))
			continue;

		if (team == TEAM_NSF)
			PrintCenterText(i, message_nsf);
		else
			PrintCenterText(i, message);
	}
}

// Purpose: Store current epoch globally so multiple functions can use it for identifying themselves
void GenerateEpoch()
{
	ConVar hTimeAdjustment = FindConVar("sm_time_adjustment");
	g_epoch = GetTime() + hTimeAdjustment.IntValue;
}

void SetGameState(int state)
{
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

bool IsAdmin(int client)
{
	if (!IsClientInGame(client) || !IsClientAuthorized(client))
	{
		LogError("IsAdmin: Client %i is not valid. This shouldn't happen.", client);
		return false;
	}
	AdminId adminId = GetUserAdmin(client);
	if (adminId == INVALID_ADMIN_ID)
	{
		return false;
	}
	return GetAdminFlag(adminId, Admin_Generic);
}

bool IsMatchPoint()
{
	if (!g_isLive)
	{
		return false;
	}

	// TODO: unimplemented for other modes
	if (GetConVarInt(g_hRoundStyle) != 1)
	{
		return false;
	}

	if (g_isSuddenDeathRound)
	{
		return true;
	}

	// Subtract 1 because rounds are one-indexed
	int rounds_remaining = GetConVarInt(g_hRoundLimit) - (g_roundNumber - 1);

	// Should never have less than 0 rounds remaining, but do a sanity check just in case.
	if (rounds_remaining < 0)
	{
		LogError("Had %d rounds remaining, but expected 0 or more (%d - %d = %d).",
			rounds_remaining,
			GetConVarInt(g_hRoundLimit),
			g_roundNumber,
			rounds_remaining);
		return false;
	}
	// Can't be match point because the match is over.
	else if (rounds_remaining == 0)
	{
		return false;
	}

	int jin_score = g_jinraiScore[g_roundNumber];
	int nsf_score = g_nsfScore[g_roundNumber];

	// If a team's potential max score cannot catch up to the
	// score of the other team winning the current round,
	// consider it a match point.
	if ((jin_score + rounds_remaining) - (nsf_score + 1) <= 0 ||
		(nsf_score + rounds_remaining) - (jin_score + 1) <= 0)
	{
		return true;
	}

	return false;
}
