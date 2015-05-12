// Double-include prevention
#if defined _mynatives_included_
	#endinput
#endif
#define _mynatives_included_

/****
	* Returns competitive match's live state.
	*
	* @return	Boolean. True if game is live, false otherwise.
****/
native bool:Competitive_IsLive();