#define MENU_TIME 20 // How long to display panel menus for, in seconds

#define REASON_TECHNICAL 0
#define REASON_TIMEOUT 1

new String:g_tag[] = "[COMP]";
new String:g_teamName[][64] = {
	"Unassigned",
	"Spectator",
	"Jinrai",
	"NSF"
};
new String:g_livePlayers[44][MAXPLAYERS+1]; // Array for storing SteamIDs to allow disconnected players rejoining when live
new String:g_menuSoundOk[21] = "buttons/button14.wav";
new String:g_menuSoundCancel[27] = "buttons/combine_button7.wav";

new g_assignedTeamWhenLive[MAXPLAYERS+1] = -1;
new g_jinraiScore;
new g_nsfScore;
new g_pausingReason;
new g_pausingTeam;
new g_roundCount;
new g_spamTimerCount;
new g_unpauseTimer = 10;
new g_usedTimeouts[4];

new Float:g_fRoundTime;

new bool:g_isAlltalkByDefault;
//new bool:g_isCompetitor[MAXPLAYERS+1];
new bool:g_isExpectingOverride;
new bool:g_isLive;
new bool:g_isPaused;
new bool:g_isReady[MAXPLAYERS+1];
new bool:g_isSourceTVRecording;
new bool:g_isSpawned[MAXPLAYERS+1];
new bool:g_isTeamReadyForUnPause[4]; // Unassigned, spec, jinrai, nsf
new bool:g_isWantingOverride[4]; // Unassigned, spec, jinrai, nsf
new bool:g_shouldPause;

#if DEBUG
	new bool:g_debug_ignoreLiveTeams;
#endif

new Handle:g_hAlltalk;
new Handle:g_hCompetitionName;
new Handle:g_hForceCamera;
new Handle:g_hMatchSize;
new Handle:g_hMaxPauseLength;
new Handle:g_hMaxTimeouts;
new Handle:g_hNeoRestartThis;
new Handle:g_hPausable;
new Handle:g_hRoundLimit;
new Handle:g_hSourceTVEnabled;
new Handle:g_hSourceTVPath;
new Handle:g_hJinraiName;
new Handle:g_hNSFName;
//new Handle:g_hServerPassword;

new Handle:g_hTimerSpamLive = INVALID_HANDLE;
new Handle:g_hTimerTenSeconds = INVALID_HANDLE;
new Handle:g_hTimerTogglePause = INVALID_HANDLE;

public Action:CheckIfEveryoneIsReady()
{
	new players;
	
	#if DEBUG // count bots for debug purposes. start override will not work if bots or sourcetv present!
		players = Client_GetCount(true, true); // countInGameOnly, countFakeClients
	#else
		players = Client_GetCount(true, false); // countInGameOnly, countFakeClients
	#endif
	
	new playersReady = GetPlayersReady();
	
	#if DEBUG
		PrintToServer("%s Players found total: %i", g_tag, players);
		PrintToServer("%s Players ready: %i", g_tag, playersReady);
	#endif
	
	if (players > playersReady) // Everyone isn't ready, stop here
		return Plugin_Stop;
	
	if (players < playersReady)
		LogError("More players ready than there are players total.");
	
	if (players < GetConVarInt(g_hMatchSize)) // Everyone connected is ready, but there are less people than expected
	{
		g_isExpectingOverride = true;
		PrintToChatAll("%s Waiting for %i more players to join.", g_tag, (GetConVarInt(g_hMatchSize) - players));
		PrintToChatAll("To start the match without %i players, both teams must !start", GetConVarInt(g_hMatchSize));
		
		return Plugin_Stop;
	}
	
	else if (players > GetConVarInt(g_hMatchSize)) // Everyone connected is ready, but there are more people than expected
	{
		g_isExpectingOverride = true;
		PrintToChatAll("%s There are %i more players than expected.", g_tag, (players - GetConVarInt(g_hMatchSize)));
		PrintToChatAll("To force the match start, both teams must !start");
		
		return Plugin_Stop;
	}
	
	ToggleLive(); // Passed all checks, ready to go live
	
	return Plugin_Handled;
}

public Action:Command_IgnoreTeams(client, args)
{
	if (!g_debug_ignoreLiveTeams)
	{
		g_debug_ignoreLiveTeams = true;
		ReplyToCommand(client, "Now ignoring team change protection.");
	}
	
	else
	{
		g_debug_ignoreLiveTeams = false;
		ReplyToCommand(client, "Enforcing teams.");
	}
	
	return Plugin_Handled;
}

public Action:ToggleRecording()
{
	if (g_isLive) // Start recording
	{
		for (new i = 1; i <= MaxClients; i++)
		{
			if (!Client_IsValid(i) || IsFakeClient(i))
				continue;
			
			new String:steamID[44];
			GetClientAuthString(i, steamID, sizeof(steamID));
			
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

public Action:ToggleLive()
{
	if (!g_isLive)
	{
		g_isLive = true;
		
		SetConVarBool(g_hAlltalk, false);
		SetConVarBool(g_hPausable, false);
		SetConVarBool(g_hNeoRestartThis, true);
		SetConVarBool(g_hForceCamera, true);
		
		#if DEBUG
			PrintToServer("Creating SteamID array of players...");
			new foundPlayers;
		#endif
		
		for (new i = 1; i <= MaxClients; i++) // Save connected players' SteamIDs so they can be allowed to rejoin their teams if disconnected
		{
			if (!Client_IsValid(i) || IsFakeClient(i))
				continue;
			
			new String:steamID[44];
			GetClientAuthString(i, steamID, sizeof(steamID));
			
			if (strlen(steamID) == 0)
				LogError("Client %i returned empty SteamID, probably not authorized whilst attempting to save SteamID into g_livePlayers!", i);
			
			else
			{
				strcopy(g_livePlayers[i], sizeof(g_livePlayers[]), steamID);
				g_assignedTeamWhenLive[i] = GetClientTeam(i);
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
		
		ToggleRecording();
		
		// Announce game start, possible tournament name, and teams
		new String:competitionName[32];
		GetConVarString(g_hCompetitionName, competitionName, sizeof(competitionName));
		
		if (strlen(competitionName) < 1)
			strcopy(competitionName, sizeof(competitionName), "Competitive Neotokyo");
		
		new String:liveTitle[sizeof(g_tag) + sizeof(competitionName) + sizeof(g_teamName[]) * 2 + 6];
		
		Format(liveTitle, sizeof(liveTitle), "%s %s: %s - %s", g_tag, competitionName, g_teamName[TEAM_JINRAI], g_teamName[TEAM_NSF]);
		
		PrintToChatAll(liveTitle);
		
		SpamTimer();
	}
	
	else
	{
		g_isLive = false;
		
		if (g_isPaused)
			TogglePause();
		
		ToggleRecording();
		
		g_roundCount = 0;
		g_usedTimeouts[TEAM_JINRAI] = 0;
		g_usedTimeouts[TEAM_NSF] = 0;
		
		SetConVarBool(g_hForceCamera, false);
		
		if (g_isAlltalkByDefault)
			SetConVarBool(g_hAlltalk, true);
		
		for (new i = 1; i <= MaxClients; i++)
		{
			strcopy(g_livePlayers[i], sizeof(g_livePlayers[]), ""); // Empty live players array
			g_assignedTeamWhenLive[i] = TEAM_UNASSIGNED;
		}
		
		PrintToChatAll("%s Match has ended.", g_tag);
	}
	
	ToggleSourceTV(); // This has to be called after modifying the g_isLive boolean
}

public Action:TogglePause()
{
	if (!g_isPaused)
	{
		if (!g_isLive)
		{
			LogError("Attempted to pause while match isn't live.");
			return Plugin_Handled;
		}
		
		if (g_pausingReason == REASON_TIMEOUT)
			g_usedTimeouts[g_pausingTeam]++;
		
		g_isPaused = true;
		PrintToChatAll("%s Pause duration: %i seconds.", g_tag, GetConVarInt(g_hMaxPauseLength));
		PrintToChatAll("%s Use !pause when ready.", g_tag);
		
		#if DEBUG > 1
			PrintToServer("%s Pause duration: %i seconds.", g_tag, GetConVarInt(g_hMaxPauseLength));
		#endif
		
		CreateTimer(GetConVarFloat(g_hMaxPauseLength), Timer_Unpause);
		
		if ((GetConVarFloat(g_hMaxPauseLength) / 2) > 10.0) // Don't bother with "half of pause time remaining" notice there's less than 10 secs remaining
			CreateTimer((GetConVarFloat(g_hMaxPauseLength) / 2), Timer_UnpauseNotice_Half);
		
		CreateTimer((GetConVarFloat(g_hMaxPauseLength) - 10.0), Timer_StartTenSecondsLeft);
	}
	
	else
	{	
		g_isPaused = false;
		
		if (g_isLive) // Only announce the unpause if a match is still going
		{
			PrintToChatAll("%s UNPAUSED", g_tag);
			SpamTimer();
		}
	}
	
	SetConVarBool(g_hPausable, true);
	
	CreateTimer(0.5, Timer_TogglePauseLoop);
	
	return Plugin_Handled;
}

public Action:Timer_TogglePauseLoop(Handle:timer)
{
	TogglePauseLoop();
}

public Action:TogglePauseLoop()
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
				if (g_hTimerTogglePause != INVALID_HANDLE)
				{
					KillTimer(g_hTimerTogglePause);
					g_hTimerTogglePause = INVALID_HANDLE;
				}
				
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
			
			if (g_hTimerTogglePause != INVALID_HANDLE)
			{
				KillTimer(g_hTimerTogglePause);
				g_hTimerTogglePause = INVALID_HANDLE;
			}
			
			if (GetGameTime() > g_fRoundTime + 13.9)
			{
				forcePause = true;
				i = 0; // Reset loop
				
				#if DEBUG > 1
					PrintToServer("New round is about to start! Force any valid client to toggle pause...");
				#endif
				
				continue;
			}
			
			g_hTimerTogglePause = CreateTimer(1.0, Timer_TogglePauseLoop);
			
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
	g_spamTimerCount++;
	
	if (g_spamTimerCount <= 3)
		PrintToChatAll("%s LIVE", g_tag);
	
	else
	{
		if (g_hTimerSpamLive != INVALID_HANDLE)
		{
			KillTimer(g_hTimerSpamLive);
			g_hTimerSpamLive = INVALID_HANDLE;
		}
		
		g_spamTimerCount = 0;
	}
}

public Action:SpamTimer()
{
	g_hTimerSpamLive = CreateTimer(0.75, Timer_SpamLive, _, TIMER_REPEAT); // This repeat timer is killed inside the callback after X repeats
}

public Action:Timer_Unpause(Handle:timer)
{
	if (g_isPaused)
	{
		PrintToChatAll("%s Maximum pause time has ended!", g_tag);
		TogglePause();
	}
}

public Action:Timer_UnpauseNotice_Half(Handle:timer)
{
	// Show notification when half of the max pause time has elapsed
	if (g_isPaused)
		PrintToChatAll("%s Pause time left: %i seconds", g_tag, (RoundToNearest(GetConVarFloat(g_hMaxPauseLength)) / 2));
}

public Action:Timer_StartTenSecondsLeft(Handle:timer)
{
	g_hTimerTenSeconds = CreateTimer(1.0, Timer_TenSecondsCountdown, _, TIMER_REPEAT); // This repeat timer is killed inside the callback
}

public Action:Timer_TenSecondsCountdown(Handle:timer)
{
	if (g_isPaused && g_unpauseTimer => 0)
	{
		PrintToChatAll("%s Unpausing in: %i", g_tag, g_unpauseTimer);
		g_unpauseTimer--;
	}
	
	else
	{
		if (g_hTimerTenSeconds != INVALID_HANDLE)
		{
			KillTimer(g_hTimerTenSeconds);
			g_hTimerTenSeconds = INVALID_HANDLE;
		}
		
		g_unpauseTimer = 10;
	}
}

public InitDirectory(const String:sDir[])
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

public Action:ToggleSourceTV()
{
	if (g_isSourceTVRecording)
	{
		ServerCommand("tv_stoprecord");
		g_isSourceTVRecording = false;
		
		#if DEBUG
			PrintToServer("tv_stoprecord");
		#endif
	}
	
	else
	{
		if (!GetConVarBool(g_hSourceTVEnabled) || !g_isLive)
			return Plugin_Stop;
		
		if (!IsSourceTVBotConnected())
		{
			#if DEBUG
				PrintToServer("ToggleSourceTV: SourceTV bot not found.");
			#endif
			
			return Plugin_Stop;
		}
		
		if (g_isSourceTVRecording)
			ServerCommand("tv_stoprecord");
		
		new String:demoPath[PLATFORM_MAX_PATH];
		new String:timestamp[16];
		new String:mapName[32];
		
		GetConVarString(g_hSourceTVPath, demoPath, sizeof(demoPath));
		GetCurrentMap(mapName, sizeof(mapName));
		
		FormatTime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M");
		
		new String:competitionName[32];
		GetConVarString(g_hCompetitionName, competitionName, sizeof(competitionName));
		
		new String:replayName[sizeof(mapName)+sizeof(timestamp)+sizeof(competitionName)+2];
		
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
		
		ReplaceString(replayBuffer, sizeof(replayBuffer), ":", ""); // Remove : characters from the SteamID. To-do: proper sanitazion
		strcopy(replayName, sizeof(replayName), replayBuffer);
		
		ServerCommand("tv_record \"%s/%s\"", demoPath, replayName); // SourceTV bot needs to be connected for this to do anything. This is checked above with IsSourceTVBotConnected()
		g_isSourceTVRecording = true;
		
		#if DEBUG
			PrintToServer("tv_record \"%s/%s\"", demoPath, replayName);
		#endif
	}
	
	return Plugin_Handled;
}

bool:IsSourceTVBotConnected()
{
	// Get the SourceTV bot's name cvar, and figure out if bot is connected
	new Handle:hBotName = FindConVar("tv_name");
	new String:botName[MAX_NAME_LENGTH];
	GetConVarString(hBotName, botName, sizeof(botName));
	CloseHandle(hBotName);
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i) || !IsFakeClient(i)) // We are looking for a valid fake client (bot)
			continue;
		
		new String:clientName[MAX_NAME_LENGTH];
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

stock GetPlayersReady()
{
	new playersReady;

	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i) || !g_isReady[i]) // Fake clients are never ready, so we don't need to explicitly check for them
			continue;
		
		playersReady++;
	}
	
	return playersReady;
}
