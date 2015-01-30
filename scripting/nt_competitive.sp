#pragma semicolon 1

//#define DEBUG 0 // Release
//#define DEBUG 1 // Basic debug
#define DEBUG 2 // Extended debug

#include <sourcemod>
#include <smlib>
#include <neotokyo>
#include "nt_competitive/nt_competitive_base"
#include "nt_competitive/nt_competitive_panel"
#include "nt_competitive/nt_competitive_parser"

#define PLUGIN_VERSION "0.2"

public Plugin:myinfo = {
	name		=	"Neotokyo Competitive Plugin",
	description	=	"NT competitive setup",
	author		=	"Rain",
	version		=	PLUGIN_VERSION,
	url			=	""
};

public OnPluginStart()
{
	RegConsoleCmd("sm_ready",	Command_Ready,				"Mark yourself as ready for a competitive match.");
	RegConsoleCmd("sm_unready",	Command_UnReady,			"Mark yourself as not ready for a competitive match.");
	RegConsoleCmd("sm_start",	Command_OverrideStart,		"Force a competitive match start when using an unexpected setup.");
	RegConsoleCmd("sm_unstart",	Command_UnOverrideStart,	"Cancel !sm_start.");
	RegConsoleCmd("sm_pause",	Command_Pause,				"Request a pause or timeout in a competitive match.");
	RegConsoleCmd("sm_timeout",	Command_Pause,				"Request a pause or timeout in a competitive match.");
	RegConsoleCmd("jointeam",	Command_JoinTeam);
	
	#if DEBUG
		RegAdminCmd("sm_forcelive", Command_ForceLive, ADMFLAG_GENERIC, "Force the competitive match to start. Debug command.");
	#endif
	
	HookEvent("game_round_start",	Event_RoundStart);
	HookEvent("player_spawn",		Event_PlayerSpawn);
	
	g_hRoundLimit		= CreateConVar("sm_competitive_round_limit", "13", "How many rounds are played in a competitive match.");
	g_hMatchSize		= CreateConVar("sm_competitive_players_total", "10", "How many players participate in a default sized competitive match.");
	g_hMaxTimeout		= CreateConVar("sm_competitive_max_pause_length", "180", "How long can a competitive time-out last, in seconds.");
	
	g_hAlltalk			= FindConVar("sv_alltalk");
	g_hNeoRestartThis	= FindConVar("neo_restart_this");
	g_hPausable			= FindConVar("sv_pausable");
	
	HookConVarChange(g_hNeoRestartThis, Event_Restart);
	
	AutoExecConfig();
}

public OnMapStart()
{
	g_roundCount = 0;
}

public OnConfigsExecuted()
{
	g_isAlltalkByDefault = GetConVarBool(g_hAlltalk);
}

public Action:Command_ForceLive(client, args)
{
	ToggleLive();
	
	return Plugin_Handled;
}

public Action:Command_Pause(client, args)
{
	if (!g_isLive)
	{
		ReplyToCommand(client, "%s Game is not live.", g_tag);
		return Plugin_Stop;
	}
	
	new team = GetClientTeam(client);
	
	if (team != TEAM_JINRAI && team != TEAM_NSF) // Not in a team, ignore
		return Plugin_Stop;
	
	if (g_isPaused)
	{
		new otherTeam;
		
		if (team == TEAM_JINRAI)
			otherTeam = TEAM_NSF;
		else
			otherTeam = TEAM_JINRAI;
		
		if (!g_isTeamReadyForUnPause[g_pausingTeam] && team != g_pausingTeam)
		{
			PrintToChat(client, "%s Cannot unpause − the pause was initiated by %s", g_teamName[otherTeam]);
			return Plugin_Stop;
		}
		
		if (!g_isTeamReadyForUnPause[team])
		{
			new Handle:panel = CreatePanel();
			SetPanelTitle(panel, "Unpause?");
			
			DrawPanelItem(panel, "Team is ready, request unpause");
			DrawPanelItem(panel, "Cancel");
			
			SendPanelToClient(panel, client, PanelHandler_UnPause, MENU_TIME);
			
			CloseHandle(panel);
			
			return Plugin_Handled;
		}
		
		g_isTeamReadyForUnPause[team] = false;
		PrintToChatAll("%s Team %s cancelled being ready for unpause", g_tag, g_teamName[team]);
		
		return Plugin_Handled;
	}
	
	new Handle:panel = CreatePanel();
	SetPanelTitle(panel, "Request pause");
	
	DrawPanelText(panel, "Please select pause reason");
	
	DrawPanelItem(panel, "Technical difficulties");
	DrawPanelItem(panel, "Time-out");
	DrawPanelItem(panel, "Exit");
	
	SendPanelToClient(panel, client, PanelHandler_Pause, MENU_TIME);
	
	CloseHandle(panel);
	
	return Plugin_Handled;
}

public Action:PauseRequest(client, reason)
{
	new team = GetClientTeam(client);
	g_pausingTeam = team;
	
	switch (reason)
	{
		case REASON_TECHNICAL:
			PrintToChatAll("%s Team %s wants to pause for a technical issue.", g_tag, g_teamName[team]);
		
		case REASON_TIMEOUT:
			PrintToChatAll("%s Team %s wants a time-out.", g_tag, g_teamName[team]);
	}
	
	new Float:currentTime = GetGameTime();
	
	if (currentTime - g_fRoundTime < 15) // We are in a freezetime, it's safe to pause
		TogglePause();
	
	else
	{
		PrintToChatAll("Match will be paused during the next freezetime.");
		g_shouldPause = true;
	}
}

public Action:UnPauseRequest(client)
{
	new team = GetClientTeam(client);
	new otherTeam;
	
	// We check for non playable teams already in Command_Pause before calling this
	if (team == TEAM_JINRAI)
		otherTeam = TEAM_NSF;
	else
		otherTeam = TEAM_JINRAI;
	
	g_isTeamReadyForUnPause[team] = true;
	PrintToChatAll("%s Team %s is ready, and wants to unpause.", g_tag, g_teamName[team]);
	
	if (g_isTeamReadyForUnPause[TEAM_JINRAI] && g_isTeamReadyForUnPause[TEAM_NSF])
		TogglePause();
	
	else
		PrintToChatAll("Waiting for %s to confirm unpause.", g_teamName[otherTeam]);
}

public Action:Command_OverrideStart(client, args)
{
	if (!g_isExpectingOverride)
	{
		ReplyToCommand(client, "%s Not expecting any !start override currently.", g_tag);
		return Plugin_Stop;
	}
	
	new team = GetClientTeam(client);
	new bool:bothTeamsWantOverride;
	
	// Check if everyone in the team is still ready
	new playersInTeam;
	new playersInTeamReady;
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!Client_IsValid(i))
			continue;
		
		if (GetClientTeam(i) == team)
			playersInTeam++;
		
		if (GetClientTeam(i) == team && g_isReady[i])
			playersInTeamReady++;
	}
	
	if (playersInTeam != playersInTeamReady)
	{
		ReplyToCommand(client, "%s Only %i of %i players in your team are marked !ready.", g_tag, playersInTeamReady, playersInTeam);
		ReplyToCommand(client, "Everyone in your team needs to be ready to force a start.");
		
		return Plugin_Stop;
	}
	
	if (g_isWantingOverride[team])
	{
		ReplyToCommand(client, "%s Your team already wants to start. If you want to revert this, use !unstart.", g_tag);
		return Plugin_Stop;
	}
	
	if (team == TEAM_JINRAI)
		bothTeamsWantOverride = g_isWantingOverride[TEAM_NSF];
	
	else if (team == TEAM_NSF)
		bothTeamsWantOverride = g_isWantingOverride[TEAM_JINRAI];
	
	else // Spectator or unassigned, ignore
		return Plugin_Stop;
	
	g_isWantingOverride[team] = true;
	PrintToChatAll("%s Team %s wishes to start the match with current players.", g_tag, g_teamName[team]);
	
	if (bothTeamsWantOverride)
	{
		g_isExpectingOverride = false;
		
		for (new i = TEAM_SPECTATOR + 1; i == 2; i++) // Cancel both teams' override preference
			g_isWantingOverride[i] = false;
		
		ToggleLive();
	}
	
	return Plugin_Handled;
}

public Action:Command_UnOverrideStart(client, args)
{
	if (!g_isExpectingOverride)
	{
		ReplyToCommand(client, "%s Not expecting any !start override currently.", g_tag);
		return Plugin_Stop;
	}
	
	new team = GetClientTeam(client);
	
	if (team != TEAM_JINRAI || team != TEAM_NSF) // Spectator or unassigned, ignore
		return Plugin_Stop;
	
	if (!g_isWantingOverride[team])
	{
		ReplyToCommand(client, "%s Your team already does not wish to force a start!", g_tag);
		return Plugin_Stop;
	}
	
	g_isWantingOverride[team] = false;
	PrintToChatAll("%s Team %s has cancelled wanting to force start the match.", g_tag, g_teamName[team]);
	
	return Plugin_Handled;
}

public Action:Command_Ready(client, args)
{
	if (g_isLive)
	{
		ReplyToCommand(client, "%s Game is live, cannot change ready state!", g_tag);
		return Plugin_Continue;
	}
	
	if (g_isReady[client])
	{
		ReplyToCommand(client, "%s You are already marked as ready. Use !unready to revert this.", g_tag);
		return Plugin_Continue;
	}
	
	g_isReady[client] = true;
	
	new String:clientName[MAX_NAME_LENGTH];
	GetClientName(client, clientName, sizeof(clientName));
	PrintToChatAll("%s Player %s is READY.", g_tag, clientName);
	
	CheckIfEveryoneIsReady();
	
	return Plugin_Handled;
}

public Action:Command_UnReady(client, args)
{
	if (g_isLive)
	{
		ReplyToCommand(client, "%s Game is live, cannot change ready state!", g_tag);
		return Plugin_Continue;
	}
	
	if (!g_isReady[client])
	{
		ReplyToCommand(client, "%s You are already marked not ready. Use !ready when ready.", g_tag);
		return Plugin_Continue;
	}
	
	g_isReady[client] = false;
	
	new String:clientName[MAX_NAME_LENGTH];
	GetClientName(client, clientName, sizeof(clientName));
	PrintToChatAll("%s Player %s is NOT READY.", g_tag, clientName);
	
	if (g_isExpectingOverride)
	{
		new team = GetClientTeam(client);
		
		if (g_isWantingOverride[team])
		{
			g_isWantingOverride[team] = false;
			PrintToChatAll("Cancelled %s's team's force start vote.", clientName);
		}
	}
	
	return Plugin_Handled;
}
