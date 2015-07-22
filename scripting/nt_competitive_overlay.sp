#pragma semicolon 1

#include <sourcemod>
#include <smlib>
#include <neotokyo>
#include "nt_competitive/nt_competitive_natives"
#include "nt_competitive_overlay/nt_competitive_overlay_base"
#include "nt_competitive_overlay/nt_competitive_overlay_panel"
#include "nt_competitive_overlay/nt_competitive_overlay_parser"

#define PLUGIN_VERSION "0.1"

public Plugin:myinfo = {
	name		=	"Neotokyo Competitive Plugin, Overlay Module",
	description	=	"Transfer live competitive data to SQL server for overlay use",
	author		=	"Rain",
	version		=	PLUGIN_VERSION,
	url			=	""
};

public OnPluginStart()
{
	RegAdminCmd("sm_caster", Command_CasterMenu, ADMFLAG_GENERIC, "Open the casters overlay menu");
	
	hJinraiName	= FindConVar("sm_competitive_jinrai_name");
	hNSFName	= FindConVar("sm_competitive_nsf_name");
	hRoundTime	= FindConVar("neo_round_timelimit");
	hFreezeTime	= FindConVar("mp_chattime");
	
	if (hTimer_UpdateData != INVALID_HANDLE)
		SetFailState("Timer handle hTimer_UpdateData leaked");
	
	hTimer_UpdateData = CreateTimer(1.0, Timer_UpdateData, _, TIMER_REPEAT);
	
	HookEvent("game_round_start", Event_RoundStart);
}

public OnConfigsExecuted()
{
	SQL_Init();
}

public OnClientDisconnect(client)
{
	g_IsSettingCasterMsg[client] = false;
}

public Action:Command_CasterMenu(client, args)
{
	new Handle:panel = CreatePanel();
	
	SetPanelTitle(panel, "Caster Overlay");
	
	DrawPanelText(panel, " "); // Line break
	
	DrawPanelText(panel, "Current overlay message:");
	if (strlen(casterMessage) < 1)
	{
		DrawPanelText(panel, "(none)");
	}
	else
	{
		new String:castMsgBuffer[sizeof(casterMessage) + 4];
		Format(castMsgBuffer, sizeof(castMsgBuffer), "\"%s\"", casterMessage);
		DrawPanelText(panel, castMsgBuffer);
	}
	
	DrawPanelText(panel, " ");
	
	DrawPanelItem(panel, "Set overlay message");
	
	if (!g_ShowOverlayMsg)
		DrawPanelItem(panel, "Show overlay message: OFF");
	else
		DrawPanelItem(panel, "Show overlay message: ON");
	
	DrawPanelItem(panel, "Exit");
	
	SendPanelToClient(panel, client, PanelHandler_CasterMenu, MENU_TIME);
	
	CloseHandle(panel);
	
	return Plugin_Handled;
}
