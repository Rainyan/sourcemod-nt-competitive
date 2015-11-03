// Double-include prevention
#if defined _overlay_base_included_
	#endinput
#endif
#define _overlay_base_included_

/*
	0: Jinrai name,
	1: NSF name,
	2: Jinrai score,
	3: NSF score,
	4: caster comments,
	5: live state,
	6: pause state
*/
#define ARR_DATA_ID_AMOUNT 7
#define MAX_DATA_LENGTH 256
#define MAX_SQL_LENGTH 2048
#define MENU_TIME 30

#define SQL_CONFIG "nt_competitive_overlay"
#define SQL_TABLE_NAME "competitive_overlay"

new Handle:hTimer_UpdateData = INVALID_HANDLE;
new Handle:db = INVALID_HANDLE;

new Handle:hJinraiName;
new Handle:hNSFName;
new Handle:hRoundTime;
new Handle:hFreezeTime;

new bool:IsSQLInitialized;
new bool:g_ShowOverlayMsg;
new bool:g_IsSettingCasterMsg[MAXPLAYERS+1];

new Float:g_fGameTime_LastRound;
 
new String:casterMessage[MAX_DATA_LENGTH];
new String:g_winner[12];

SQL_Init()
{
	new String:error[MAX_DATA_LENGTH];
	db = SQL_Connect(SQL_CONFIG, true, error, sizeof(error));
	
	if (db == INVALID_HANDLE)
		LogError("SQL connection failed: %s", error);
	
	if ( CheckTableExistsInSQL() )
		IsSQLInitialized = true;
}

bool:CheckTableExistsInSQL()
{
	new String:sqlError[MAX_DATA_LENGTH]; // Buffer for SQL related errors
	
	// Make sure the table exists
	new String:sql[MAX_SQL_LENGTH];
	Format(sql, sizeof(sql), "CREATE TABLE IF NOT EXISTS %s( \
		id INT(5) NOT NULL AUTO_INCREMENT, \
		Score_Jinrai INT(2), \
		Score_NSF INT(2), \
		Teamname_Jinrai VARCHAR(256), \
		Teamname_NSF VARCHAR(256), \
		Caster_Status VARCHAR(256), \
		Caster_ShowStatus BOOL NOT NULL DEFAULT '0', \
		Pause_Status BOOL NOT NULL DEFAULT '0', \
		Live_Status BOOL NOT NULL DEFAULT '0', \
		Map_Name VARCHAR(64), \
		Gametime_LastSync FLOAT(24), \
		Gametime_LastRound FLOAT(24), \
		Gametime_RoundLength FLOAT(6), \
		Gametime_FreezeLength FLOAT(6), \
		Winner VARCHAR(12), \
		Timestamp TIMESTAMP, \
		PRIMARY KEY (id)) CHARACTER SET=utf8;", SQL_TABLE_NAME);
	
	if (!SQL_FastQuery(db, sql))
	{
		SQL_GetError(db, sqlError, sizeof(sqlError));
		SetFailState("Failed SQL query: %s", sqlError);
	}
	
	Format(sql, sizeof(sql), "SELECT COUNT(*) FROM %s", SQL_TABLE_NAME);
	
	new Handle:query = SQL_Query(db, sql);
	if (query == INVALID_HANDLE)
	{
		SQL_GetError(db, sqlError, sizeof(sqlError));
		SetFailState("SQL error: %s", sqlError);
	}
	
	if (!SQL_HasResultSet(query))
	{
		SetFailState("SQL error: no result set returned");
	}
	
	new rows = SQL_GetRowCount(query);
	
	if (rows != 1)
	{
		SetFailState("SQL error: error returning result");
	}
	
	new String:result[MAX_DATA_LENGTH];
	SQL_FetchRow(query);
	SQL_FetchString(query, 0, result, sizeof(result));
	
	CloseHandle(query);
	
	new iResult = StringToInt(result);
	
	if (iResult > 1)
		SetFailState("SQL error: multiple data rows found, expected only 1");
	
	if (iResult < 0)
		SetFailState("SQL error: row count returned negative");
	
	if (iResult == 0)
	{	
		Format(sql, sizeof(sql), "INSERT INTO %s \
									(Score_Jinrai, Score_NSF, Teamname_Jinrai, Teamname_NSF, Caster_Status, Pause_Status, Live_Status, Map_Name, Caster_ShowStatus, Gametime_LastSync, Gametime_LastRound, Gametime_RoundLength, Gametime_FreezeLength, Winner) \
									VALUES \
									(0, 0, '', '', '', 0, 0, '', 0, 0.0, 0.0, 0.0, 0.0, '')", SQL_TABLE_NAME);
		
		if (!SQL_FastQuery(db, sql))
		{
			SQL_GetError(db, sqlError, sizeof(sqlError));
			SetFailState("Failed SQL query: %s", sqlError);
		}
	}
	
	return true;
}

public Action:SetCasterMessage(client)
{
	if (!Client_IsValid(client))
		return Plugin_Stop;
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if (g_IsSettingCasterMsg[i] == true)
		{
			new String:otherClientName[MAX_NAME_LENGTH];
			GetClientName(i, otherClientName, sizeof(otherClientName));
			PrintToChat(client, "User \"%s\" is currently editing the caster message. Please wait.", otherClientName);
			return Plugin_Stop;
		}
	}
	
	g_IsSettingCasterMsg[client] = true;
	
	PrintToChat(client, "Please type your overlay message in the chat.");
	PrintToChat(client, "Max length: %i. Type \"cancel\" to cancel this.", MAX_DATA_LENGTH);
	
	AddCommandListener(SayCallback_CasterMsg, "say");
	AddCommandListener(SayCallback_CasterMsg, "say_team");
	
	return Plugin_Handled;
}

public Action:ToggleCasterMessage(client)
{
	if (!Client_IsValid(client))
		return Plugin_Stop;
	
	if (!g_ShowOverlayMsg)
		g_ShowOverlayMsg = true;
	else
		g_ShowOverlayMsg = false;
	
	Command_CasterMenu(client, 1);
	
	return Plugin_Handled;
}