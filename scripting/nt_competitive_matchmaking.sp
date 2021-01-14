#pragma semicolon 1

#include <sourcemod>
#include <smlib>

#define PLUGIN_VERSION "0.1"

#define MAX_SQL_LENGTH 2048

#define SQL_CONFIG "nt_competitive_matchmaking"
#define SQL_TABLE_QUEUED "queued"
#define SQL_TABLE_OFFER_MATCH "match_offers"

Handle g_hMatchmaking;
Handle g_hMatchSize;

Handle g_hTimer_CheckMMStatus = INVALID_HANDLE;
Handle db = INVALID_HANDLE;

bool g_isSQLInitialized;
bool g_isServerOfferingMatch;

public Plugin myinfo = {
	name		=	"Neotokyo Competitive Plugin, Matchmaking Module",
	description	=	"Handle queue based matchmaking",
	author		=	"Rain",
	version		=	PLUGIN_VERSION,
	url			=	""
};

public void OnPluginStart()
{
	g_hMatchmaking = CreateConVar("sm_competitive_matchmaking",	"1",	"Enable matchmaking mode (automated queue system instead of manual join)", _, true, 0.0, true, 1.0);
	
	g_hMatchSize = FindConVar("sm_competitive_players_total");
	
	HookConVarChange(g_hMatchmaking, Event_Matchmaking);
}

public void OnConfigsExecuted()
{
	if (GetConVarBool(g_hMatchmaking))
	{
		InitSQL();
		
		if (g_hTimer_CheckMMStatus == INVALID_HANDLE)
			g_hTimer_CheckMMStatus = CreateTimer(60.0, Timer_CheckMMStatus, _, TIMER_REPEAT);
	}
}

int GetPlayersQueued()
{	
	char sql[MAX_SQL_LENGTH];
	
	Format(sql, sizeof(sql), "SELECT players_queued FROM %s", SQL_TABLE_QUEUED);
	
	Handle query = SQL_Query(db, sql);
	
	if (query == INVALID_HANDLE)
	{
		LogError("SQL error: query failed");
		return 0;
	}
	
	int playersQueued;
	
	while (SQL_FetchRow(query))
		playersQueued = SQL_FetchInt(query, 0);
	
	CloseHandle(query);
	
	return playersQueued;
}

void InitSQL()
{
	char sqlError[256];
	db = SQL_Connect(SQL_CONFIG, true, sqlError, sizeof(sqlError));
	
	if (db == INVALID_HANDLE)
	{
		LogError("SQL error: %s", sqlError);
		return;
	}
	
	char sql[MAX_SQL_LENGTH];
	Format(sql, sizeof(sql), "CREATE TABLE IF NOT EXISTS %s( \
id INT(5) NOT NULL AUTO_INCREMENT, \
players_queued INT(2), \
Timestamp TIMESTAMP, \
PRIMARY KEY (id)) CHARACTER SET=utf8;", SQL_TABLE_QUEUED);
	
	if (!SQL_FastQuery(db, sql))
	{
		LogError("SQL error: query error");
		return;
	}
	
	Format(sql, sizeof(sql), "CREATE TABLE IF NOT EXISTS %s( \
id INT(5) NOT NULL AUTO_INCREMENT, \
server_ip VARCHAR(16), \
server_port INT(5), \
server_password VARCHAR(32), \
server_name VARCHAR(128), \
Timestamp TIMESTAMP, \
PRIMARY KEY (id)) CHARACTER SET=utf8;", SQL_TABLE_OFFER_MATCH);
	
	if (!SQL_FastQuery(db, sql))
	{
		LogError("SQL error: query error");
		return;
	}
	
	g_isSQLInitialized = true;
	
	return;
}

public Action OfferMatch()
{
	if (!g_isSQLInitialized || g_isServerOfferingMatch)
		return Plugin_Stop;
	
	char serverIP[16];
	Server_GetIPString(serverIP, sizeof(serverIP));
	int serverPort = Server_GetPort();
	
	char sql[MAX_SQL_LENGTH];
	Format(sql, sizeof(sql), "SELECT * FROM %s WHERE server_ip=? AND server_port=?, ", SQL_TABLE_OFFER_MATCH);
	
	char sqlError[256];
	Handle stmt = SQL_PrepareQuery(db, sql, sqlError, sizeof(sqlError));
	
	if (stmt == INVALID_HANDLE)
	{
		LogError("SQL error: %s", sqlError);
		return Plugin_Stop;
	}
	
	SQL_BindParamString(stmt, 0, serverIP, false);
	SQL_BindParamInt(stmt, 1, serverPort);
	
	if (!SQL_Execute(stmt))
	{
		LogError("SQL error: %s", sqlError);
	}
	
	int entries;
	
	while (SQL_FetchRow(stmt))
	{
		entries = SQL_FetchInt(stmt, 0);
	}
	
	delete stmt;
	
	PrintToServer("Found entries: %i", entries);
	
	return Plugin_Handled;
}

public void Event_Matchmaking(Handle cvar, const char[] oldVal, const char[] newVal)
{
	if (StringToInt(newVal) == 1)
		InitSQL();
}

public Action Timer_CheckMMStatus(Handle timer)
{
	if (!GetConVarBool(g_hMatchmaking)) // We're not in "matchmaking" mode anymore, stop this timer
	{
		if (g_hTimer_CheckMMStatus != INVALID_HANDLE)
		{
			KillTimer(g_hTimer_CheckMMStatus);
			return Plugin_Stop;
		}
	}
	
	if ( GetPlayersQueued() >= GetConVarInt(g_hMatchSize) ) // There's enough people queued up to start a match
	{
		if (!g_isServerOfferingMatch)
			OfferMatch();
	}
	
	return Plugin_Continue;
}
