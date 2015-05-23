/*
	GPLv3
		- Fade-to-black function borrowed from Agiel's nt_fadetoblack plugin.
		- SourceTV recording functions borrowed from Stevo.TVR's Auto Recorder plugin: http://forums.alliedmods.net/showthread.php?t=92072
*/

#pragma semicolon 1

//#define DEBUG 0 // Release
//#define DEBUG 1 // Basic debug
#define DEBUG 2 // Extended debug

#include <sourcemod>
#include <sdktools>
#include <smlib>
#include <neotokyo>
#include "nt_competitive/nt_competitive_base"
#include "nt_competitive/nt_competitive_panel"
#include "nt_competitive/nt_competitive_parser"

#define PLUGIN_VERSION "0.3.3"

public Plugin:myinfo = {
	name		=	"Neotokyo Competitive Plugin",
	description	=	"Count score, announce winner, perform other competitive tasks",
	author		=	"Rain",
	version		=	PLUGIN_VERSION,
	url			=	"https://github.com/Rainyan/sourcemod-nt-competitive"
};

public APLRes:AskPluginLoad2(Handle:myself, bool:late, String:error[], err_max)
{
	CreateNative("Competitive_IsLive", Competitive_IsLive);
	return APLRes_Success;
}

public OnPluginStart()
{
	RegConsoleCmd("sm_ready",		Command_Ready,				"Mark yourself as ready for a competitive match.");
	
	RegConsoleCmd("sm_unready",		Command_UnReady,			"Mark yourself as not ready for a competitive match.");
	RegConsoleCmd("sm_notready",	Command_UnReady,			"Mark yourself as not ready for a competitive match.");
	
	RegConsoleCmd("sm_start",		Command_OverrideStart,		"Force a competitive match start when using an unexpected setup.");
	RegConsoleCmd("sm_unstart",		Command_UnOverrideStart,	"Cancel sm_start.");
	
	RegConsoleCmd("sm_pause",		Command_Pause,				"Request a pause or timeout in a competitive match.");
	RegConsoleCmd("sm_timeout",		Command_Pause,				"Request a pause or timeout in a competitive match.");
	
	RegConsoleCmd("jointeam",		Command_JoinTeam); // There's no pick team event for NT, so we do this instead
	
	#if DEBUG
		RegAdminCmd("sm_forcelive",			Command_ForceLive,			ADMFLAG_GENERIC,	"Force the competitive match to start. Debug command.");
		RegAdminCmd("sm_ignoreteams",		Command_IgnoreTeams,		ADMFLAG_GENERIC,	"Ignore team limitations when a match is live. Debug command.");
		RegAdminCmd("sm_pause_resetbool",	Command_ResetPauseBool,		ADMFLAG_GENERIC,	"Reset g_isPaused to FALSE. Debug command.");
		RegAdminCmd("sm_logtest",			Command_LoggingTest,		ADMFLAG_GENERIC,	"Test competitive file logging. Logs the cmd argument. Debug command.");
		RegAdminCmd("sm_unpause_other",		Command_UnpauseOther,		ADMFLAG_GENERIC,	"Pretend the other team requested unpause. Debug command.");
		RegAdminCmd("sm_start_other",		Command_OverrideStartOther,	ADMFLAG_GENERIC,	"Pretend the other team requested force start. Debug command.");
	#endif
	
	HookEvent("game_round_start",	Event_RoundStart);
	HookEvent("player_death",		Event_PlayerDeath);
	HookEvent("player_spawn",		Event_PlayerSpawn);
	
	CreateConVar("sm_competitive_version", PLUGIN_VERSION, "Competitive plugin version.", FCVAR_PLUGIN|FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY);
	
	g_hRoundLimit						= CreateConVar("sm_competitive_round_limit",						"15",					"How many rounds are played in a competitive match.", _, true, 1.0);
	g_hMatchSize						= CreateConVar("sm_competitive_players_total",						"10",					"How many players total are expected to ready up before starting a competitive match.");
	g_hMaxTimeouts						= CreateConVar("sm_competitive_max_timeouts",						"1",					"How many time-outs are allowed per match per team.", _, true, 0.0);
	g_hMaxPauseLength					= CreateConVar("sm_competitive_max_pause_length",					"180",					"How long can a competitive time-out last, in seconds.", _, true, 0.0);
	g_hSourceTVEnabled					= CreateConVar("sm_competitive_sourcetv_enabled",					"1",					"Should the competitive plugin automatically record SourceTV demos.", _, true, 0.0, true, 1.0);
	g_hSourceTVPath						= CreateConVar("sm_competitive_sourcetv_path",						"replays_competitive",	"Directory to save SourceTV demos into. Relative to NeotokyoSource folder. Will be created if possible.");
	g_hJinraiName						= CreateConVar("sm_competitive_jinrai_name",						"Jinrai",				"Jinrai team's name. Will use \"Jinrai\" if left empty.");
	g_hNSFName							= CreateConVar("sm_competitive_nsf_name",							"NSF",					"NSF team's name. Will use \"NSF\" if left empty.");
	g_hCompetitionName					= CreateConVar("sm_competitive_title",								"",						"Name of the tournament/competition. Also used for replay filenames. 32 characters max. Use only alphanumerics and spaces.");
	g_hCommsBehaviour					= CreateConVar("sm_competitive_comms_behaviour",					"0",					"Voice comms behaviour when live. 0 = no alltalk, 1 = enable alltalk, 2 = check sv_alltalk value before live state.", _, true, 0.0, true, 2.0);
	g_hLogMode							= CreateConVar("sm_competitive_log_mode",							"1",					"Competitive logging mode. 1 = enabled, 0 = disabled.", _, true, 0.0, true, 1.0);
	g_hKillVersobity					= CreateConVar("sm_competitive_killverbosity",						"1",					"Display the players still alive in console after each kill.", _, true, 0.0, true, 1.0);
	g_hClientRecording					= CreateConVar("sm_competitive_record_clients",						"0",					"Should clients automatically record when going live.", _, true, 0.0, true, 1.0);
	g_hTimeAllowedForUnpauseRejoiner	= CreateConVar("sm_competitive_max_unpause_during_pause_rejoin",	"5",					"How many seconds are we allowed to unpause during a team's own pause, if one of their players has dropped from the server and needs to reconnect. If connect time exceeds this, the player will have to wait for actual unpause to rejoin the game. If zero, nobody is allowed to rejoin until the pause has completely ended.", _, true, 0.0);
	
	g_hAlltalk			= FindConVar("sv_alltalk");
	g_hForceCamera		= FindConVar("mp_forcecamera");
	g_hNeoRestartThis	= FindConVar("neo_restart_this");
	g_hPausable			= FindConVar("sv_pausable");
	
	HookConVarChange(g_hNeoRestartThis,					Event_Restart);
	HookConVarChange(g_hSourceTVEnabled,				Event_SourceTVEnabled);
	HookConVarChange(g_hSourceTVPath,					Event_SourceTVPath);
	HookConVarChange(g_hJinraiName,						Event_TeamNameJinrai);
	HookConVarChange(g_hNSFName,						Event_TeamNameNSF);
	HookConVarChange(g_hCommsBehaviour,					Event_CommsBehaviour);
	HookConVarChange(g_hLogMode,						Event_LogMode);
	HookConVarChange(g_hKillVersobity,					Event_KillVerbosity);
	HookConVarChange(g_hClientRecording,				Event_ClientRecording);
	HookConVarChange(g_hTimeAllowedForUnpauseRejoiner,	Event_TimeAllowedForUnpauseRejoiner);
	
	HookUserMessage(GetUserMessageId("Fade"), Hook_Fade, true); // Hook fade to black (on death)
	
	// Initialize SourceTV path
	new String:sourceTVPath[PLATFORM_MAX_PATH];
	GetConVarString(g_hSourceTVPath, sourceTVPath, sizeof(sourceTVPath));
	if (!DirExists(sourceTVPath))
		InitDirectory(sourceTVPath);
	
	// Initialize logs path
	new String:loggingPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, loggingPath, sizeof(loggingPath), "logs/competitive");
	if (!DirExists(loggingPath))
		InitDirectory(loggingPath);
	
	// Initialize keyvalues path
	BuildPath(Path_SM, g_kvPath, sizeof(g_kvPath), "data/competitive");
	if (!DirExists(g_kvPath))
		InitDirectory(g_kvPath);
	
	BuildPath(Path_SM, g_kvPath, sizeof(g_kvPath), "data/competitive/matches");
	if (!DirExists(g_kvPath))
		InitDirectory(g_kvPath);
	
	AutoExecConfig();
}

public OnMapStart()
{
	g_roundCount = 0;
}

public OnConfigsExecuted()
{
	g_isAlltalkByDefault					= GetConVarBool(g_hAlltalk);
	g_shouldClientsRecord					= GetConVarBool(g_hClientRecording);
	g_killVerbosity							= GetConVarInt(g_hKillVersobity);
	g_ftimeAllowedForRejoinerDuringUnpause	= GetConVarFloat(g_hTimeAllowedForUnpauseRejoiner);
}

public OnClientAuthorized(client, const String:authID[])
{
	if (g_isLive)
	{
		// ** Check for competitor status below **
		new bool:isPlayerCompeting;
		new earlierUserid;
		
		for (new i = 0; i < sizeof(g_livePlayers); i++)
		{
			#if DEBUG > 1
				PrintToServer("Checking array index %i, array size %i", i, sizeof(g_livePlayers));
				PrintToServer("Contents: %s", g_livePlayers[i]);
			#endif
			
			if (StrEqual(authID, g_livePlayers[i]))
			{
				isPlayerCompeting = true;
				earlierUserid = i;
				break;
			}
		}
		
		if (!isPlayerCompeting)
			g_assignedTeamWhenLive[client] = TEAM_SPECTATOR;
		
		else
			g_assignedTeamWhenLive[client] = g_assignedTeamWhenLive[earlierUserid];
		
		#if DEBUG
			PrintToServer("Client connected when live. Assigned to team %s", g_teamName[g_assignedTeamWhenLive[client]]);
		#endif
		
		// ** Check for competitor rejoining during a pause below **
		if (g_isPaused && isPlayerCompeting && g_assignedTeamWhenLive[client] == g_pausingTeam)
		{
			UnPauseForClientRejoin(client);
		}
	}
}

public Action:Command_ResetPauseBool(client, args)
{
	g_isPaused = false;
	ReplyToCommand(client, "g_isPaused reset to FALSE");
	
	return Plugin_Handled;
}

public Action:Command_ForceLive(client, args)
{
	LiveCountDown();
	
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
	
	if (!g_isPaused && g_shouldPause)
	{
		if (team != g_pausingTeam)
		{
			ReplyToCommand(client, "%s The other team has already requested a pause for the next freezetime.", g_tag);
			return Plugin_Stop;
		}
		
		else
		{
			new Handle:panel = CreatePanel();
			SetPanelTitle(panel, "Cancel pause request?");
			
			DrawPanelItem(panel, "Yes, cancel");
			DrawPanelItem(panel, "Exit");
			
			SendPanelToClient(panel, client, PanelHandler_CancelPause, MENU_TIME);
			
			CloseHandle(panel);
			
			return Plugin_Handled;
		}
	}
	
	else if (g_isPaused)
	{
		new otherTeam = GetOtherTeam(team);
		
		if (!g_isTeamReadyForUnPause[g_pausingTeam] && team != g_pausingTeam)
		{
			ReplyToCommand(client, "%s Cannot unpause − the pause was initiated by %s", g_tag, g_teamName[otherTeam]);
			return Plugin_Stop;
		}
		
		if (!g_isTeamReadyForUnPause[team])
		{
			new Handle:panel = CreatePanel();
			SetPanelTitle(panel, "Unpause?");
			
			DrawPanelItem(panel, "Team is ready, request unpause");
			DrawPanelItem(panel, "Exit");
			
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
	
	if (!g_isPaused && !g_shouldPause)
	{
		// Check if this team has tactical time-outs available
		if (g_usedTimeouts[team] >= GetConVarInt(g_hMaxTimeouts))
		{
			if (GetConVarInt(g_hMaxTimeouts) == 0)
				DrawPanelItem(panel, "Time-outs are not allowed.");
			
			else if (GetConVarInt(g_hMaxTimeouts) == 1)
				DrawPanelItem(panel, "Team has already used their timeout.");
			
			else if (GetConVarInt(g_hMaxTimeouts) > 1)
				DrawPanelItem(panel, "Team has already used all their %i timeouts.", GetConVarInt(g_hMaxTimeouts));
			
			else
			{
				new String:cvarValue[128];
				GetConVarString(g_hMaxTimeouts, cvarValue, sizeof(cvarValue));
				LogError("sm_competitive_max_timeouts has invalid value: %s", cvarValue);
			}
		}
		
		else // Team is allowed to call a time-out
		{
			DrawPanelItem(panel, "Time-out");
		}
	}
	
	DrawPanelItem(panel, "Technical difficulties"); // Team is always allowed to call a pause for technical issues
	DrawPanelItem(panel, "Exit");
	
	SendPanelToClient(panel, client, PanelHandler_Pause, MENU_TIME);
	
	CloseHandle(panel);
	
	return Plugin_Handled;
}

public Action:PauseRequest(client, reason)
{
	new team = GetClientTeam(client);
	g_pausingTeam = team;
	g_pausingReason = reason;
	
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

public Action:CancelPauseRequest(client)
{
	g_shouldPause = false;
	
	new team = GetClientTeam(client);
	PrintToChatAll("%s %s has cancelled their pause request for the next freezetime.", g_tag, g_teamName[team]);
}

public Action:UnPauseRequest(client)
{
	new team = GetClientTeam(client);
	new otherTeam = GetOtherTeam(team); // We check for non playable teams already in Command_Pause before calling this
	
	g_isTeamReadyForUnPause[team] = true;
	PrintToChatAll("%s %s is ready, and wants to unpause.", g_tag, g_teamName[team]);
	
	if (g_isTeamReadyForUnPause[TEAM_JINRAI] && g_isTeamReadyForUnPause[TEAM_NSF])
		TogglePause();
	
	else
		PrintToChatAll("Waiting for %s to confirm unpause.", g_teamName[otherTeam]);
}

public Action:Command_OverrideStart(client, args)
{
	new team = GetClientTeam(client);
	
	if (team != TEAM_JINRAI && team != TEAM_NSF) // Spectator or unassigned, ignore
		return Plugin_Stop;
	
	if (!g_isExpectingOverride)
	{
		ReplyToCommand(client, "%s Not expecting any !start override currently.", g_tag);
		return Plugin_Stop;
	}
	
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
	
	if (playersInTeam < playersInTeamReady)
	{
		LogError("There are more players marked ready than there are players in the team!");
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
	
	g_isWantingOverride[team] = true;
	PrintToChatAll("%s Team %s wishes to start the match with current players.", g_tag, g_teamName[team]);
	
	if (bothTeamsWantOverride)
	{
		g_isExpectingOverride = false;
		
		// Cancel both teams' override preference
		g_isWantingOverride[TEAM_JINRAI] = false;
		g_isWantingOverride[TEAM_NSF] = false;
		
		LiveCountDown();
	}
	
	return Plugin_Handled;
}

public Action:Command_UnOverrideStart(client, args)
{
	new team = GetClientTeam(client);
	
	if (team != TEAM_JINRAI && team != TEAM_NSF) // Spectator or unassigned, ignore
		return Plugin_Stop;
	
	if (!g_isExpectingOverride)
	{
		ReplyToCommand(client, "%s Not expecting any !start override currently.", g_tag);
		return Plugin_Stop;
	}
	
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
			PrintToChatAll("Cancelled %s's force start vote.", g_teamName[team]);
		}
	}
	
	return Plugin_Handled;
}

public Action:Command_LoggingTest(client, args)
{
	if (args != 1)
	{
		ReplyToCommand(client, "Expected 1 argument.");
		
		return Plugin_Stop;
	}
	
	new String:message[128];
	GetCmdArg(1, message, sizeof(message));
	LogCompetitive(message);
	
	ReplyToCommand(client, "Debug log message sent.");
	
	return Plugin_Handled;
}

public Competitive_IsLive(Handle:plugin, numParams)
{
	if (g_isLive)
		return true;
	
	return false;
}