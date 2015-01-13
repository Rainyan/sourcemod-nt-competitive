#pragma semicolon 1

#define DEBUG 2

#include <sourcemod>
#include <smlib>
#include <neotokyo>
#include "nt_competitive/nt_competitive_base"
#include "nt_competitive/nt_competitive_panel"
#include "nt_competitive/nt_competitive_parser"

#define PLUGIN_VERSION "0.1"

public Plugin:myinfo = {
	name		=	"Neotokyo Competitive Plugin",
	description	=	"NT competitive setup",
	author		=	"Rain",
	version		=	PLUGIN_VERSION,
	url			=	""
};

public OnPluginStart()
{
	RegConsoleCmd("sm_ready", Command_Ready, "Mark yourself as ready for a competitive match.");
	RegConsoleCmd("sm_unready", Command_UnReady, "Mark yourself as not ready for a competitive match.");
	RegConsoleCmd("sm_start", Command_OverrideStart, "Force a competitive match start when using an unexpected setup.");
	RegConsoleCmd("sm_unstart", Command_UnOverrideStart, "Cancel !sm_start.");
	RegConsoleCmd("sm_pause", Command_Pause, "Request a pause or timeout in a competitive match.");
	RegConsoleCmd("sm_timeout", Command_Pause, "Request a pause or timeout in a competitive match.");
	
	#if DEBUG
		RegConsoleCmd("sm_forcelive", Command_ForceLive, "Force the competitive match to start. Debug command.");
	#endif

	HookEvent("game_round_start", Event_RoundStart);
	HookEvent("player_team", Event_PlayerTeam);
	
	g_hRoundLimit = CreateConVar("sm_competitive_round_limit", "13", "How many rounds are played in a competitive match.");
	AutoExecConfig(true);
	g_hMatchSize = CreateConVar("sm_competitive_match_size", "10", "How many players participate in a default sized competitive match.");
	g_hMaxTimeout = CreateConVar("sm_competitive_timeout_length", "180", "How long can a competitive time-out last, in seconds.");
	
	g_hNeoRestartThis = FindConVar("neo_restart_this");
	
	HookConVarChange(g_hNeoRestartThis, Event_Restart);
}

public OnMapStart()
{
	g_roundCount = 0;
}

public OnConfigsExecuted()
{
	g_hAlltalk = FindConVar("sv_alltalk");
	g_isAlltalkByDefault = GetConVarBool(g_hAlltalk);
}

public Action:Command_ForceLive(client, args)
{
	ToggleLive();
	
	return Plugin_Handled;
}

public Action:PauseRequest(client, reason)
{
	new team = GetClientTeam(client);
	
	switch (reason)
	{
		case REASON_TECHNICAL:
		{
			PrintToChatAll("%s Team %s wants to pause for a technical issue.", g_tag, g_teamName[team]);
		}
		
		case REASON_TIMEOUT:
		{
			PrintToChatAll("%s Team %s wants a time-out.", g_tag, g_teamName[team]);
		}
	}
	
	new Float:currentTime = GetGameTime();
	
	if (currentTime - g_fRoundTime < 15) // We are in a freezetime, it's safe to pause
	{
		TogglePause();
	}
	
	else
	{
		PrintToChatAll("Match will be paused during the next freezetime.");
		g_shouldPause = true;
	}
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
