// Double-include prevention
#if defined _natives_included_
	#endinput
#endif
#define _natives_included_

/****
	* Returns competitive match's live state.
	*
	* @return	bool	True if game is live, false otherwise.
****/
native bool Competitive_IsLive();

/****
	* Returns whether the current match round is match point.
	*
	* @return	bool	True if match point, false otherwise.
****/
native bool Competitive_IsMatchPoint();

/****
	* Returns competitive match's pause state.
	*
	* @return	bool	True if game is paused, false otherwise.
****/
native bool Competitive_IsPaused();

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
