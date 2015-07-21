// Double-include prevention
#if defined _mynatives_included_
	#endinput
#endif
#define _mynatives_included_

/****
	* Returns competitive match's live state.
	*
	* @return	bool	True if game is live, false otherwise.
****/
native bool:Competitive_IsLive();

/****
	* Returns competitive match's pause state.
	*
	* @return	bool	True if game is paused, false otherwise.
****/
native bool:Competitive_IsPaused();

/****
	* Returns competitive match's team score.
	*
	* @param	int		Team index.
	* @return	int		Team score. Returns -1 if error occurs.
****/
native Competitive_GetTeamScore(team);