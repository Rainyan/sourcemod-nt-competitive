// Double-include prevention
#if defined _overlay_parser_included_
	#endinput
#endif
#define _overlay_parser_included_

public Action Event_RoundStart(Handle event, const char[] name, bool dontBroadcast)
{
	int winner = Competitive_GetWinner();
	
	if (winner == TEAM_JINRAI)
		strcopy(g_winner, sizeof(g_winner), "Jinrai");
	else if (winner == TEAM_NSF)
		strcopy(g_winner, sizeof(g_winner), "NSF");
	else
		strcopy(g_winner, sizeof(g_winner), "");
	
	g_fGameTime_LastRound = GetGameTime();
}

public Action SayCallback_CasterMsg(int client, const char[] commandName, int arcg)
{
	if ( !Client_IsValid(client) || !g_IsSettingCasterMsg[client] || !Client_IsAdmin(client) )
		return Plugin_Continue;
	
	char message[MAX_DATA_LENGTH];
	GetCmdArgString(message, sizeof(message));
	StripQuotes(message);
	
	strcopy(casterMessage, sizeof(casterMessage), message);
	
	PrintToChat(client, "Overlay message updated.");
	
	RemoveCommandListener(SayCallback_CasterMsg, "say");
	RemoveCommandListener(SayCallback_CasterMsg, "say_team");
	
	g_IsSettingCasterMsg[client] = false;
	
	Command_CasterMenu(client, 1);
	
	return Plugin_Handled;
}

public Action Timer_UpdateData(Handle timer)
{	
	if (!IsSQLInitialized)
	{
		PrintToServer("!IsSQLInitialized");
		return Plugin_Continue;
	}
	
	// Variables to store the data in
	char teamName_Jinrai[MAX_DATA_LENGTH];
	char teamName_NSF[MAX_DATA_LENGTH];
	int teamScore_Jinrai;
	int teamScore_NSF;
	bool isLive;
	bool isPaused;
	char mapName[64];
	
	GetCurrentMap(mapName, sizeof(mapName));
	
	// Get data
	isLive = Competitive_IsLive();
	isPaused = Competitive_IsPaused();
	teamScore_Jinrai = GetTeamScore(TEAM_JINRAI);
	teamScore_NSF = GetTeamScore(TEAM_NSF);
	
	GetConVarString(hJinraiName, teamName_Jinrai, sizeof(teamName_Jinrai));
	GetConVarString(hNSFName, teamName_NSF, sizeof(teamName_NSF));
	// Use default team names if custom names aren't set
	if (strlen(teamName_Jinrai) < 1)
		strcopy(teamName_Jinrai, sizeof(teamName_Jinrai), "Jinrai");
	if (strlen(teamName_NSF) < 1)
		strcopy(teamName_NSF, sizeof(teamName_NSF), "NSF");
	
	char sql[MAX_SQL_LENGTH];
	
	Format(sql, sizeof(sql), "UPDATE competitive_overlay SET \
Score_Jinrai=?, \
Score_NSF=?, \
Teamname_Jinrai=?, \
Teamname_NSF=?, \
Caster_Status=?, \
Pause_Status=?, \
Live_Status=?, \
Map_Name=?, \
Caster_ShowStatus=?, \
Gametime_LastSync=?, \
Gametime_LastRound=?, \
Gametime_RoundLength=?, \
Gametime_FreezeLength=?, \
Winner=?");
	
	char sqlError[MAX_DATA_LENGTH];
	Handle hSQL_Stmt = SQL_PrepareQuery(db, sql, sqlError, sizeof(sqlError));
	
	if (hSQL_Stmt == INVALID_HANDLE)
	{
		LogError("SQL error: %s", sqlError);
		return Plugin_Continue;
	}
	
	SQL_BindParamInt	(hSQL_Stmt,	0,	teamScore_Jinrai,	false	); // (int)		Score_Jinrai
	SQL_BindParamInt	(hSQL_Stmt,	1,	teamScore_NSF,		false	); // (int)		Score_NSF
	SQL_BindParamString	(hSQL_Stmt,	2,	teamName_Jinrai,	false	); // (str)		Teamname_Jinrai
	SQL_BindParamString	(hSQL_Stmt,	3,	teamName_NSF,		false	); // (str)		Teamname_NSF
	SQL_BindParamString	(hSQL_Stmt,	4,	casterMessage,		false	); // (str)		Caster_Status
	SQL_BindParamInt	(hSQL_Stmt,	5,	isPaused,			false	); // (int)		Pause_Status
	SQL_BindParamInt	(hSQL_Stmt,	6,	isLive,				false	); // (int)		Live_Status
	SQL_BindParamString	(hSQL_Stmt, 7,	mapName,			false	); // (str)		Map_Name
	SQL_BindParamInt	(hSQL_Stmt, 8,	g_ShowOverlayMsg,	false	); // (bool)	Caster_ShowStatus
	SQL_BindParamFloat	(hSQL_Stmt, 9,	GetGameTime()				); // (float)	Gametime_LastSync
	SQL_BindParamFloat	(hSQL_Stmt, 10,	g_fGameTime_LastRound		); // (float)	Gametime_LastRound
	SQL_BindParamFloat	(hSQL_Stmt, 11,	GetConVarFloat(hRoundTime)	); // (float)	Gametime_RoundLength
	SQL_BindParamFloat	(hSQL_Stmt, 12,	GetConVarFloat(hFreezeTime)	); // (float)	Gametime_FreezeLength
	SQL_BindParamString	(hSQL_Stmt, 13, g_winner,			false	); // (str)		Winner
	
	if (!SQL_Execute(hSQL_Stmt))
	{
		LogError("SQL error: %s", sqlError);
		delete hSQL_Stmt;
		return Plugin_Continue;
	}
	
	delete hSQL_Stmt;
	return Plugin_Continue;
}
