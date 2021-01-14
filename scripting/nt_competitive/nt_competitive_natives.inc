// Double-include prevention
#if defined _natives_included_
	#endinput
#endif
#define _natives_included_

/****
	* Returns competitive match's live state.
	*
	* @return	int		1 if game is live, 0 otherwise.
****/
native int Competitive_IsLive();

/****
	* Returns competitive match's pause state.
	*
	* @return	int		1 if game is paused, 0 otherwise.
****/
native int Competitive_IsPaused();

/****
	* Returns competitive match's team score.
	*
	* @param	int		Team index.
	* @return	int		Team score. Returns -1 if error occurs.
****/
native int Competitive_GetTeamScore(int team);

/****
	* Returns competitive match's winner.
	*
	* @return	int		Winner team index. Returns 0 if there is no winner.
****/
native int Competitive_GetWinner();
