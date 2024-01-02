// Double-include prevention
#if defined _NT_COMPETITIVE_SHARED_INC_
    #endinput
#endif
#define _NT_COMPETITIVE_SHARED_INC_


// Whenever nt_competitive changes player score, it will emit the global forward
// "function Action Competitive_OnPlayerScoreChange(PlayerScoreChangeReason reason, int client)".
// If you want to override this forward, you can check this reason to make a
// decision on what should happen.
enum PlayerScoreChangeReason {
	// As part of round restore, typically during unpause.
	PLRSCORECHANGE_REASON_ROUND_RESTORE = 0,
	// This player just rejoined the server.
	PLRSCORECHANGE_REASON_PLAYER_REJOIN,

	PLRSCORECHANGE_REASON_OTHER
};
