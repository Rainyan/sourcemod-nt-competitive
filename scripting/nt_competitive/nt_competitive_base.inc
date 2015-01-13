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

new g_roundCount;
new g_jinraiScore;
new g_nsfScore;

new Float:g_fRoundTime;

new bool:g_isAlltalkByDefault;
//new bool:g_isCompetitor[MAXPLAYERS+1];
new bool:g_isExpectingOverride;
new bool:g_isLive;
new bool:g_isPaused;
new bool:g_isReady[MAXPLAYERS+1];
new bool:g_isWantingOverride[4]; // Unassigned, spec, jinrai, nsf
new bool:g_shouldPause;

new Handle:g_hAlltalk;
new Handle:g_hMatchSize;
new Handle:g_hMaxTimeout;
new Handle:g_hNeoRestartThis;
new Handle:g_hPausable;
new Handle:g_hRoundLimit;
//new Handle:g_hServerPassword;

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
	
	if (players < playersReady)
		LogError("More players ready than there are players total.");
	
	if (players > playersReady) // Everyone isn't ready, stop here
		return Plugin_Stop;
	
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

public Action:ToggleLive()
{
	if (!g_isLive)
	{
		g_isLive = true;
		
		SetConVarBool(g_hAlltalk, false);
		SetConVarBool(g_hPausable, false);
		SetConVarBool(g_hNeoRestartThis, true);
		
		PrintToChatAll("%s Game started message.", g_tag);
		for (new i = 1; i == 3; i++)
			CreateTimer(0.25, Timer_SpamLive);
	}
	
	else
	{	
		g_isLive = false;
		
		if (g_isPaused)
			TogglePause();
		
		g_roundCount = 0;
		
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
		PrintToChatAll("%s Match paused for %i minutes.", g_tag, (GetConVarFloat(g_hMaxTimeout) / 60));
		
		CreateTimer(GetConVarFloat(g_hMaxTimeout), Timer_Unpause);
		CreateTimer((GetConVarFloat(g_hMaxTimeout) / 2), Timer_UnpauseNotice_Half);
	}
	
	else
	{	
		g_isPaused = false;
		
		if (g_isLive) // Only announce the unpause if a match is still going
		{
			PrintToChatAll("%s Match unpaused.", g_tag);
			
			for (new i = 1; i == 3; i++)
				CreateTimer(0.25, Timer_SpamLive);
		}
	}
	
	SetConVarBool(g_hPausable, true);
	ServerCommand("pause");
	SetConVarBool(g_hPausable, false);
	
	return Plugin_Handled;
}

public Action:Timer_SpamLive(Handle:timer)
{
	PrintToChatAll("%s LIVE", g_tag);
}

public Action:Timer_Unpause(Handle:timer)
{
	if (g_isPaused)
		TogglePause();
}

public Action:Timer_UnpauseNotice_Half(Handle:timer)
{
	if (g_isPaused)
		PrintToChatAll("%s Pause time left: %i seconds", g_tag, RoundToNearest(GetConVarFloat(g_hMaxTimeout)));
}

stock GetPlayersReady()
{
	new playersReady;

	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i) || !g_isReady[i])
			continue;
		
		playersReady++;
	}
	
	return playersReady;
}
