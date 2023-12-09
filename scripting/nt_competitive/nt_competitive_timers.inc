// Double-include prevention
#if defined _NT_COMPETITIVE_TIMERS_INC_
	#endinput
#endif
#define _NT_COMPETITIVE_TIMERS_INC_

#if defined(FLATTEN_INCLUDE_PATHS)
#include "nt_competitive_base"
#else
// If you're compiling using Spider or other in-browser compiler,
// and these include paths are failing, un-comment the FLATTEN_INCLUDE_PATHS compile flag above.
#include "nt_competitive/nt_competitive_base"
#endif


public Action CheckGhostOvertime(Handle timer)
{
	int gameState = GameRules_GetProp("m_iGameState");
	if (gameState != GAMESTATE_ROUND_ACTIVE)
	{
		g_hTimer_GhostOvertime = INVALID_HANDLE;
		return Plugin_Stop;
	}

	float timeLeft = GameRules_GetPropFloat("m_fRoundTimeLeft");
	float graceTime = GetConVarFloat(g_hGhostOvertimeGrace);
	if (timeLeft < graceTime)
	{
		float realTimeLeft;
		float decayTime = GetConVarFloat(g_hGhostOvertimeDecay) + graceTime;
		bool graceReset = GetConVarBool(g_hGhostOvertimeGraceReset);
		if (graceReset)
		{
			float roundTime = GetConVarFloat(g_hRoundTime) * 60;
			float overtime = GetGameTime() - (g_fRoundTime + roundTime - graceTime);
			bool decayExp = GetConVarBool(g_hGhostOvertimeDecayExp);
			if (decayExp)
			{
				g_fGhostOvertime = graceTime + 1 - Pow(graceTime + 1, overtime / decayTime);
			}
			else
			{
				g_fGhostOvertime = graceTime - graceTime * overtime / decayTime;
			}
			realTimeLeft = decayTime - overtime;
		}
		else
		{
			float timePassed = g_fGhostOvertimeTick - timeLeft;
			g_fGhostOvertime -= timePassed * graceTime / decayTime;
			g_fGhostOvertimeTick = float(RoundToCeil(g_fGhostOvertime));
			realTimeLeft = g_fGhostOvertime * decayTime / graceTime;
		}
		// Round up to nearest int to prevent HUD flicker
		GameRules_SetPropFloat("m_fRoundTimeLeft", float(RoundToCeil(g_fGhostOvertime)));

		// Everything's multiplied by 2 because we want to tick every second, but the interval is 0.5
		bool printTick = g_bGhostOvertimeFirstTick
			|| (RoundToCeil(realTimeLeft * 2) % RoundToCeil(graceTime * 2) == 0) // Divisible by graceTime
			|| (realTimeLeft < graceTime && RoundToCeil(realTimeLeft * 2) % 10 == 0) // Divisible by 5
			|| (realTimeLeft < 5 && RoundToCeil(realTimeLeft * 2) % 2 == 0); // Every second for the last 5
		if (printTick) {
			PrintToChatAll("Ghost overtime engaged. %d seconds remaining.", RoundToCeil(realTimeLeft));
			g_bGhostOvertimeFirstTick = false;
		}
	}

	return Plugin_Continue;
}

public Action Timer_PostAuthXpRecovery(Handle timer, int userid)
{
	int client = GetClientOfUserId(userid);
	// If userid->client == 0, this client has disconnected again.
	if (client == 0)
	{
		return Plugin_Stop;
	}
	// Not authorized yet, keep trying
	if (!IsClientAuthorized(client))
	{
		return Plugin_Continue;
	}
	PostAuthXpRecovery(client);
	return Plugin_Stop;
}

public Action Timer_GoLive(Handle timer)
{
	EmptyScore(); // Make sure everyone starts private so we avoid the no weapon spawn bug
	CreateTimer(0.1, Timer_ToggleLive); // A slight delay so the score reset above takes effect in time

	g_hTimer_GoLive = INVALID_HANDLE;
	return Plugin_Stop;
}

public Action Timer_ToggleLive(Handle timer)
{
	ToggleLive();
	return Plugin_Stop;
}

public Action Timer_CancelLiveEndConfirmation(Handle timer)
{
	g_confirmLiveEnd = false;
	return Plugin_Stop;
}

public Action Timer_EnterPauseRound(Handle timer)
{
	SetGameState(GAMESTATE_WAITING_FOR_PLAYERS);

	float pauseLength;
	if (g_pauseReason == REASON_TECHNICAL)
		pauseLength = GetConVarFloat(g_hMaxPauseLength_Technical);
	else
		pauseLength = GetConVarFloat(g_hMaxPauseLength);

	GameRules_SetPropFloat( "m_fRoundTimeLeft", pauseLength ); // Pause round uses max pause length roundtime

	for (int i = 0; i < 3; i++)
	{
		PrintToChatAll("GAME IS PAUSED");
	}

	return Plugin_Stop;
}

public Action Timer_TogglePauseLoop(Handle timer)
{
	TogglePauseLoop();

	return Plugin_Stop;
}

public Action Timer_TogglePause(Handle timer)
{
	TogglePause();
	return Plugin_Stop;
}

public Action Timer_DisablePause(Handle timer)
{
	SetConVarBool(g_hPausable, false);
	return Plugin_Stop;
}

public Action Timer_SpamLive(Handle timer)
{
	// Print the LIVE message X times, then kill the repeat timer calling this
	int spamLiveTimes = 3;
	g_spamTimerCount++;

	if (g_spamTimerCount == 1) // Don't spam "LIVE" during the first increment. This has to do with chat message ordering (round number msgs) during match start, in Event_RoundStart.
		return Plugin_Continue;

	else if (g_spamTimerCount <= (spamLiveTimes + 1)) // spamLiveTimes+1, because we won't trigger during first increment (see comment above)
		PrintToChatAll("%s LIVE", g_tag);

	else
	{
		g_spamTimerCount = 0;
		return Plugin_Stop;
	}

	return Plugin_Continue;
}

public Action Timer_Unpause(Handle timer)
{
	if (g_isPaused)
	{
		PrintToChatAll("%s Maximum pause time has ended!", g_tag);
		PrintToChatAll("%s Unpausing...", g_tag);
		TogglePause();
	}

	g_hTimer_UnPause = INVALID_HANDLE;

	return Plugin_Handled;
}

public Action Timer_UnpauseNotice_Half(Handle timer)
{
	// Show notification when half of the max pause time has elapsed
	if (g_isPaused)
	{
		PrintToChatAll("%s Pause time left: %i seconds", g_tag, RoundToNearest(GetConVarFloat(g_hMaxPauseLength)) / 2);
		return Plugin_Continue;
	}

	g_hTimer_UnPause_HalfLeft = INVALID_HANDLE;

	return Plugin_Handled;
}

public Action Timer_StartTenSecondsLeft(Handle timer)
{
	if (g_isPaused)
		CreateTimer(1.0, Timer_Unpause_SecondsCountdown, _, TIMER_REPEAT); // This repeat timer is killed inside the callback

	g_hTimer_UnPause_Countdown = INVALID_HANDLE;

	return Plugin_Handled;
}

public Action Timer_Live_SecondsCountdown(Handle timer)
{
	if (g_liveTimer > 0)
	{
		PrintToChatAll("%s Match starting in: %i", g_tag, g_liveTimer);
		g_liveTimer--;

		PlayLiveBeep();

		return Plugin_Continue;
	}

	g_liveTimer = g_liveTimer_OriginalValue; // Reset the timer to its original value

	return Plugin_Stop;
}

public Action Timer_Unpause_SecondsCountdown(Handle timer)
{
	if (!g_isLive || !g_isPaused) // Pause has properly ended or the match is not live anymore (which implies the pause was properly ended already)
		return Plugin_Stop;

	if (g_unpauseTimer > 0)
	{
		//PlayLiveBeep(); Does not work whilst paused
		PrintToChatAll("%s Unpausing in: %i", g_tag, g_unpauseTimer);
		g_unpauseTimer--;

		return Plugin_Continue;
	}

	g_unpauseTimer = g_unpauseTimer_OriginalValue; // Reset the timer to its original value

	return Plugin_Stop;
}

public Action Timer_ClearGhostCapper(Handle timer)
{
	g_ghostCapturingTeam = TEAM_NONE;
	return Plugin_Stop;
}
