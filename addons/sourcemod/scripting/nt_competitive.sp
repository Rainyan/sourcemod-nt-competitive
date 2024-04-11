#include <sourcemod>

#include <neotokyo>

#pragma semicolon 1
#pragma newdecls required

#define PLUGIN_VERSION "4.0.0"

#define MAX_CUSTOM_TEAM_NAME_LEN 64
char _plugin_tag[] = "[COMP]";

#define TIMER_SPAMLIVE 0.25

static int _prev_winner = TEAM_NONE;

public Plugin myinfo = {
	name = "Neotokyo Competitive Plugin",
	description = "SourceMod plugin for competitive Neotokyo.",
	author = "Rain",
	version = PLUGIN_VERSION,
	url = "https://github.com/Rainyan/sourcemod-nt-competitive"
};

enum {
	WinCondition_BestOfX = 0,
	WinCondition_FirstToX,

	WinCondition_EnumCount
};

enum struct Rules {
	int win_condition;
	int score_limit;
}

enum struct Team {
	int index;
	ConVar score;
}

ConVar sm_competitive_live,
	sm_competitive_jinrai_score,
	sm_competitive_nsf_score,
	sm_competitive_limit,
	sm_competitive_win_condition,
	sm_competitive_jinrai_name,
	sm_competitive_nsf_name,
	sm_competitive_log_lvl,
	neo_restart_this;

GlobalForward _fwd_match_conclusion = null;


public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	CreateNative("Competitive_IsLive", Competitive_IsLive);
	CreateNative("Competitive_IsMatchPoint", Competitive_IsMatchPoint);
	CreateNative("Competitive_GetTeamScore", Competitive_GetTeamScore);
	CreateNative("Competitive_GetWinner", Competitive_GetWinner);
	return APLRes_Success;
}

public void OnPluginStart()
{
	_fwd_match_conclusion = new GlobalForward("Competitive_OnMatchConclusion",
		ET_Hook, Param_CellByRef);
	if (_fwd_match_conclusion == null)
	{
		SetFailState("Failed to create global forward");
	}

	if (!HookEventEx("game_round_start", OnRoundStart, EventHookMode_PostNoCopy))
	{
		SetFailState("Failed to hook event");
	}

	neo_restart_this = FindConVar("neo_restart_this");
	if (neo_restart_this == null)
	{
		SetFailState("Failed to find convar");
	}
	neo_restart_this.AddChangeHook(OnNeoRestartThis);

	sm_competitive_live = CreateConVar("sm_competitive_live",
		"0", "Whether the match is live",
		FCVAR_NOTIFY, true, float(false), true, float(true));
	sm_competitive_jinrai_score = CreateConVar("sm_competitive_jinrai_score",
		"0", "Team Jinrai score for a competitive match",
		_, true, float(0), true, float(99));
	sm_competitive_nsf_score = CreateConVar("sm_competitive_nsf_score",
		"0", "Team NSF score for a competitive match",
		_, true, float(0), true, float(99));
	sm_competitive_win_condition = CreateConVar("sm_competitive_win_condition",
		"0", "Win condition enumeration for competitive play",
		FCVAR_NOTIFY, true, float(0), true, float(WinCondition_EnumCount) - 1);
	sm_competitive_limit = CreateConVar("sm_competitive_limit",
		"15", "Default score limit for competitive play. The meaning depends \
on \"sm_competitive_win_condition\" value.",
		FCVAR_NOTIFY, true, float(1), true, float(99));
	sm_competitive_jinrai_name = CreateConVar("sm_competitive_jinrai_name",
		"Jinrai", "Default Jinrai team name",
		FCVAR_NOTIFY);
	sm_competitive_nsf_name = CreateConVar("sm_competitive_nsf_name",
		"NSF", "Default NSF team name",
		FCVAR_NOTIFY);
	sm_competitive_log_lvl = CreateConVar("sm_competitive_log_lvl",
		"1", "Logging level",
		_, true, float(0));
}

void OnNeoRestartThis(ConVar convar, const char[] oldValue, const char[] newValue)
{
	bool went_live = StringToBool(newValue) && !StringToBool(oldValue);
	if (went_live)
	{
		GameRules_SetProp("m_iRoundNumber", 1);
	}
}

bool StringToBool(const char[] s)
{
	return StringToInt(s) != 0;
}

public void OnRoundStart(Event event, const char[] name, bool dontBroadcast)
{
	if (!sm_competitive_live.BoolValue)
	{
		return;
	}

	int round_number = GameRules_GetProp("m_iRoundNumber");

	if (round_number == 1) // one-indexed
	{
		CreateTimer(TIMER_SPAMLIVE, Timer_SpamLive, 0, TIMER_FLAG_NO_MAPCHANGE);
	}

	Rules rules;
	GetRules(rules);

	switch (rules.win_condition)
	{
		case WinCondition_BestOfX:
		{
			Notify("Round %d/%d%s",
				round_number,
				sm_competitive_limit.IntValue,
				(round_number >= sm_competitive_limit.IntValue) ? " (SUDDEN DEATH)" : ""
			);
		}
		case WinCondition_FirstToX:
		{
			Notify("Round %d",
				round_number
			);
		}
	}

	if (IsMatchPoint())
	{
		Notify("Match point!");
	}
}

public Action Timer_SpamLive(Handle timer, int n)
{
	int n_timer_loops = 3;
	if (n++ < n_timer_loops)
	{
		Notify("LIVE");
		CreateTimer(TIMER_SPAMLIVE, Timer_SpamLive, n, TIMER_FLAG_NO_MAPCHANGE);
	}
	return Plugin_Stop;
}

public void OnRoundConcluded(const int winner)
{
	if (!sm_competitive_live.BoolValue)
	{
		return;
	}

	Rules rules;
	GetRules(rules);

	Team jinrai = { TEAM_JINRAI };
	jinrai.score = sm_competitive_jinrai_score;
	Team nsf = { TEAM_NSF };
	nsf.score = sm_competitive_nsf_score;
	if (winner != TEAM_NONE)
	{
		((winner == TEAM_NSF) ? nsf : jinrai).score.IntValue += 1;
	}

	int round_number = GameRules_GetProp("m_iRoundNumber");
	int comp_winner = GetWinner(jinrai, nsf, rules, round_number);

	if (comp_winner != TEAM_NONE)
	{
		ConcludeMatch(comp_winner);
		return;
	}

	if (winner == TEAM_NONE)
	{
		Notify("Round has tied; no team point awarded");
		return;
	}

	Notify("Team point awarded to: %s", winner == TEAM_NSF ? "NSF" : "Jinrai");
}

bool Forward_ConcludeMatch(int& winner)
{
	Call_StartForward(_fwd_match_conclusion);
	Call_PushCellRef(winner);
	Action res = Plugin_Continue;
	if (Call_Finish(res) != SP_ERROR_NONE)
	{
		ThrowError("Global forward call failed: res was %d", res);
	}
	return (res == Plugin_Continue || res == Plugin_Changed);
}

stock void ConcludeMatch(int winner)
{
	// Opportunity for third party plugins to block, or mutate winner.
	if (!Forward_ConcludeMatch(winner))
	{
		return;
	}

	int jin_score = sm_competitive_jinrai_score.IntValue;
	int nsf_score = sm_competitive_nsf_score.IntValue;

	// Reset up front so we can fail gracefully
	ResetMatchState();

	if (winner == TEAM_NONE)
	{
		Notify("MATCH TIED %d – %d", nsf_score, jin_score);
	}
	else
	{
		if (winner != TEAM_JINRAI && winner != TEAM_NSF)
		{
			// This can happen if we're passed an invalid winner index
			// by a third party plugin via the global forward.
			Notify("ERROR: indeterminate winner. Please see error logs.");
			ThrowError("Got invalid winning team index: %d", winner);
		}

		char winner_team_name[MAX_CUSTOM_TEAM_NAME_LEN];
		GetCompetitiveTeamName(winner, winner_team_name, sizeof(winner_team_name));

		Notify("%s WINS %d – %d",
			winner_team_name,
			GetConVarInt((winner == TEAM_NSF)
				? sm_competitive_nsf_score : sm_competitive_jinrai_score),
			GetConVarInt((winner == TEAM_NSF)
				? sm_competitive_jinrai_score : sm_competitive_nsf_score)
		);
	}

	_prev_winner = winner;
}

void ResetMatchState()
{
	_prev_winner = TEAM_NONE;

	sm_competitive_live.BoolValue = false;
	sm_competitive_jinrai_score.IntValue = 0;
	sm_competitive_nsf_score.IntValue = 0;
}

void GetCompetitiveTeamName(const int team, char[] out_name, const int max_len)
{
	if (team != TEAM_JINRAI && team != TEAM_NSF)
	{
		ThrowError("Unexpected team index: %d", team);
	}

	GetConVarString(
		(team == TEAM_NSF) ?
			sm_competitive_nsf_name : sm_competitive_jinrai_name,
		out_name, max_len
	);

	if (strlen(out_name) == 0)
	{
		strcopy(out_name, max_len, (team == TEAM_JINRAI) ? "NSF" : "Jinrai");
	}
}

void Notify(const char[] message, any ...)
{
	char format_msg[256];
	VFormat(format_msg, sizeof(format_msg), message, 2);

	PrintToChatAndConsoleAll("%s %s", _plugin_tag, format_msg);
	LogCompetitive("%s", format_msg);
}

void LogCompetitive(const char[] message, any ...)
{
	if (!sm_competitive_log_lvl.BoolValue)
	{
		return;
	}

	char format_msg[256];
	VFormat(format_msg, sizeof(format_msg), message, 2);

	char logging_path[PLATFORM_MAX_PATH];
	if (0 == BuildPath(Path_SM, logging_path, sizeof(logging_path),
		"logs/competitive"))
	{
		ThrowError("Failed to build path");
	}
	char log_file[] = "competitive.log";
	Format(logging_path, sizeof(logging_path), "%s/%s", logging_path, log_file);

	LogToFileEx(logging_path, "%s", format_msg);
}

int GetWinner(const Team team1, const Team team2, const Rules rules,
	const int num_rounds_played)
{
	if (team1.score.IntValue == team2.score.IntValue)
	{
		PrintToServer("NONE!!!");
		return TEAM_NONE;
	}

	PrintToServer("alt: %d %d",
		team1.score.IntValue,
		team2.score.IntValue
	);

	int num_rounds_remaining = rules.score_limit - num_rounds_played;
	switch (rules.win_condition)
	{
		case WinCondition_BestOfX:
		{
			// Sudden death
			if (num_rounds_remaining <= 0)
			{
				if (team1.score.IntValue > team2.score.IntValue)
				{
					return team1.index;
				}
				if (team2.score.IntValue > team1.score.IntValue)
				{
					return team2.index;
				}
				return TEAM_NONE;
			}

			if  (rules.score_limit > team1.score.IntValue + num_rounds_remaining &&
				 rules.score_limit > team2.score.IntValue + num_rounds_remaining)
			{
				return TEAM_NONE;
			}
			if (team1.score.IntValue >= rules.score_limit)
			{
				return team1.index;
			}
			if (team2.score.IntValue >= rules.score_limit)
			{
				return team2.index;
			}
		}
		case WinCondition_FirstToX:
		{
			if (team1.score.IntValue > rules.score_limit)
			{
				return team1.index;
			}
			if (team2.score.IntValue > rules.score_limit)
			{
				return team2.index;
			}
		}
	}
	return TEAM_NONE;
}

bool IsMatchPoint()
{
	if (!sm_competitive_live.BoolValue)
	{
		return false;
	}

	// TODO: unimplemented for other modes
	if (sm_competitive_win_condition.IntValue != WinCondition_BestOfX)
	{
		return false;
	}

	int round_number = GameRules_GetProp("m_iRoundNumber");
	// Subtract 1 because rounds are one-indexed
	int rounds_remaining = sm_competitive_limit.IntValue - (round_number - 1);

	// Can't be match point because the match is over.
	// As a special case, we don't consider sudden death mode as "match point" here.
	if (rounds_remaining <= 0)
	{
		return false;
	}

	int jin_score = sm_competitive_jinrai_score.IntValue;
	int nsf_score = sm_competitive_nsf_score.IntValue;

	// If a team's potential max score cannot catch up to the
	// score of the other team winning the current round,
	// consider it a match point.
	if ((jin_score + rounds_remaining) - (nsf_score + 1) <= 0 ||
		(nsf_score + rounds_remaining) - (jin_score + 1) <= 0)
	{
		return true;
	}
	return false;
}

public int Competitive_IsLive(Handle plugin, int num_params)
{
	return sm_competitive_live.BoolValue;
}

public int Competitive_IsMatchPoint(Handle plugin, int num_params)
{
	return IsMatchPoint();
}

public int Competitive_GetTeamScore(Handle plugin, int num_params)
{
	if (num_params == 1)
	{
		int team = GetNativeCell(1);
		if (team == TEAM_JINRAI)
		{
			return sm_competitive_jinrai_score.IntValue;
		}
		if (team == TEAM_NSF)
		{
			return sm_competitive_nsf_score.IntValue;
		}
	}
	return -1;
}

public int Competitive_GetWinner(Handle plugin, int num_params)
{
	return _prev_winner;
}

stock void PrintToChatAndConsoleAll(const char[] format, any ...)
{
	char buffer[254];
	for (int i = 1; i <= MaxClients; ++i)
	{
		if (IsClientInGame(i))
		{
			SetGlobalTransTarget(i);
			VFormat(buffer, sizeof(buffer), format, 2);
			PrintToChat(i, "%s", buffer);
			PrintToConsole(i, "%s", buffer);
		}
	}
}

void GetRules(Rules rules)
{
	rules.win_condition = sm_competitive_win_condition.IntValue;
	rules.score_limit = sm_competitive_limit.IntValue;
}