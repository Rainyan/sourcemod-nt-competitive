#define MENU_TIME 20 // How long to display panel menus for, in seconds

#define REASON_TECHNICAL 0
#define REASON_TIMEOUT 1

new String:g_tag[] = "[COMP]";
new String:g_teamName[][] = {
	"Unassigned",
	"Spectator",
	"Jinrai",
	"NSF"
};

new g_jinraiScore;
new g_nsfScore;
new g_pausingTeam;
new g_roundCount;
new g_spamTimerCount;

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

new Handle:g_hAlltalk;
new Handle:g_hForceCamera;
new Handle:g_hMatchSize;
new Handle:g_hMaxTimeout;
new Handle:g_hNeoRestartThis;
new Handle:g_hPausable;
new Handle:g_hRoundLimit;
new Handle:g_hSourceTVEnabled;
new Handle:g_hSourceTVPath;
//new Handle:g_hServerPassword;

new Handle:g_hTimerSpamLive = INVALID_HANDLE;
new Handle:g_hTimerTogglePause = INVALID_HANDLE;

public Action:CheckIfEveryoneIsReady()
{
	new players;
	
	#if DEBUG // count bots for debug purposes
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

public Action:ToggleRecording()
{
	if (g_isLive) // Start recording
	{
		for (new i = 1; i <= MaxClients; i++)
		{
			if (!Client_IsValid(i) || IsFakeClient(i))
				continue;
			
			ClientCommand(i, "stop"); // Stop any previous recording, jic. This does nothing if a demo isn't running.
			ClientCommand(i, "record foobar"); // Start recording. Format demo filename here. Note: replays can be overwritten using this.
			
			#if DEBUG > 1
				ClientCommand(i, "say Test: Start demo.");
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
				ClientCommand(i, "say Test: Stop demo.");
			#endif
		}
	}
}

public Action:ToggleLive()
{
	ToggleSourceTV();
	
	if (!g_isLive)
	{
		g_isLive = true;
		
		SetConVarBool(g_hAlltalk, false);
		SetConVarBool(g_hPausable, false);
		SetConVarBool(g_hNeoRestartThis, true);
		SetConVarBool(g_hForceCamera, true);
		
		ToggleRecording();
		
		PrintToChatAll("%s Game started message.", g_tag);
		SpamTimer();
	}
	
	else
	{
		g_isLive = false;
		
		if (g_isPaused)
			TogglePause();
		
		ToggleRecording();
		
		g_roundCount = 0;
		
		SetConVarBool(g_hForceCamera, false);
		
		if (g_isAlltalkByDefault)
			SetConVarBool(g_hAlltalk, true);
		
		PrintToChatAll("%s Game ended message.", g_tag);
	}
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
	
		g_isPaused = true;
		PrintToChatAll("%s Pause duration: %i seconds.", g_tag, GetConVarInt(g_hMaxTimeout));
		PrintToChatAll("%s Use !pause when ready.", g_tag);
		
		#if DEBUG > 1
			PrintToServer("%s Pause duration: for %.1f minutes.", g_tag, (GetConVarFloat(g_hMaxTimeout) / 60));
			PrintToServer("Max timeout: %f", GetConVarFloat(g_hMaxTimeout));
		#endif
		
		CreateTimer(GetConVarFloat(g_hMaxTimeout), Timer_Unpause);
		CreateTimer((GetConVarFloat(g_hMaxTimeout) / 2), Timer_UnpauseNotice_Half);
	}
	
	else
	{	
		g_isPaused = false;
		
		if (g_isLive) // Only announce the unpause if a match is still going
		{
			PrintToChatAll("%s Match unpaused.", g_tag);
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
	g_hTimerSpamLive = CreateTimer(0.33, Timer_SpamLive, _, TIMER_REPEAT); // This repeat timer is killed inside the callback after X repeats
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
		PrintToChatAll("%s Pause time left: %i seconds", g_tag, (RoundToNearest(GetConVarFloat(g_hMaxTimeout)) / 2));
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
			CreateDirectory(sPath, 509); // Note that folders must have the execute bit set on Linux.  On Windows, the mode is ignored.
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
		
		if (g_isSourceTVRecording)
			ServerCommand("tv_stoprecord");
		
		new String:demoPath[PLATFORM_MAX_PATH];
		new String:timestamp[16];
		new String:mapName[32];
		
		GetConVarString(g_hSourceTVPath, demoPath, sizeof(demoPath));
		FormatTime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", GetTime());
		GetCurrentMap(mapName, sizeof(mapName));
		
		ServerCommand("tv_record \"%s/sourcetv-%s-%s\"", demoPath, timestamp, mapName);
		g_isSourceTVRecording = true;
		
		#if DEBUG
			PrintToServer("tv_record \"%s/sourcetv-%s-%s\"", demoPath, timestamp, mapName);
		#endif
	}
	
	return Plugin_Handled;
}

stock GetPlayersReady()
{
	new playersReady;

	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i) || !g_isReady[i]) // Fake clients are never ready, so we don't need to check for them
			continue;
		
		playersReady++;
	}
	
	return playersReady;
}
