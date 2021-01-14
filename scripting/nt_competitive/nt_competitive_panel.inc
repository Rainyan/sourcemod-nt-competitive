// Double-include prevention
#if defined _panel_included_
	#endinput
#endif
#define _panel_included_

public int PanelHandler_Pause(Handle menu, MenuAction action, int client, int choice)
{
	if (action == MenuAction_Select) {
		int team = GetClientTeam(client);

		switch (choice)
		{
			// Native button sounds are generally already precached by engine, but we're using PrecacheSound here jic

			case 1: // Pause requested for timeout
			{
				// No more timeouts allowed for this team
				if (g_usedTimeouts[team] >= GetConVarInt(g_hMaxTimeouts))
				{
					PrecacheSound(g_menuSoundCancel);
					EmitSoundToClient(client, g_menuSoundCancel);

					if (GetConVarInt(g_hMaxTimeouts) > 0)
						ReplyToCommand(client, "%s Your team has used all available time-outs for this match.", g_tag);

					else
						ReplyToCommand(client, "%s Tactical time-outs are not allowed for this match.", g_tag);
				}

				else
				{
					PrecacheSound(g_menuSoundOk);
					EmitSoundToClient(client, g_menuSoundOk);
					PauseRequest(client, REASON_TIMEOUT);
				}
			}

			case 2: // Pause requested for technical difficulties
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				PauseRequest(client, REASON_TECHNICAL);
			}

			case 3: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
			}
		}
	}
}

public int PanelHandler_UnPause(Handle menu, MenuAction action, int client, int choice)
{
	if (action == MenuAction_Select) {
		switch (choice)
		{
			case 1: // Pause requested for technical difficulties
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				UnPauseRequest(client);
			}

			case 2: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
			}
		}
	}
}

public int PanelHandler_CancelPause(Handle menu, MenuAction action, int client, int choice)
{
	if (action == MenuAction_Select) {
		switch (choice)
		{
			case 1: // Cancel pause requested
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				CancelPauseRequest(client);
			}

			case 2: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
			}
		}
	}
}

public int PanelHandler_RefereeMenu_Main(Handle menu, MenuAction action, int client, int choice)
{
	if (action == MenuAction_Select) {
		switch (choice)
		{
			case 1: // Game information
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				RefereeMenu_GameInfo(client);
			}

			case 2: // Penalties
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				RefereeMenu_Penalties(client);
			}

			case 3: // Rollback / change round
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				RefereeMenu_ChangeRound(client);
			}

			case 4: // Force toggle live
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				Command_ForceLive(client, 1); // Toggle live
				Command_RefereeMenu(client, 1); // Return to the referee menu
			}

			case 5: // Manually edit team score
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				RefereeMenu_ManualEdit_TeamScore(client);
			}

			case 6: // Manually edit player score
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				RefereeMenu_ManualEdit_PlayerScore(client);
			}

			case 7: // Load previous match
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				RefereeMenu_LoadMatch(client);
			}

			case 8: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
			}
		}
	}
}

public int PanelHandler_ManualEdit_TeamScore(Handle menu, MenuAction action, int client, int choice)
{
	if (action == MenuAction_Select) {
		switch (choice)
		{
			case 1: // Edit Jinrai score
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				ChatCvarEdit(client, g_hJinraiScore);
			}

			case 2: // Edit NSF score
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				ChatCvarEdit(client, g_hNSFScore);
			}

			case 3: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
				Command_RefereeMenu(client, 1);
			}
		}
	}
}

public int PanelHandler_RefereeMenu_ChangeRound(Handle menu, MenuAction action, int client, int choice)
{
	g_isEditingSetting[client] = false; // We're done editing the setting when this gets called

	if (action == MenuAction_Select) {
		switch (choice)
		{
			case 1: // Previous round
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);

				if (g_targetRound > 1 && g_targetRound >= g_furthestPlayedRound)
					g_targetRound--;

				RefereeMenu_ChangeRound(client);
			}

			case 2: // Next round
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);

				if (g_targetRound < g_furthestPlayedRound)
					g_targetRound++;

				RefereeMenu_ChangeRound(client);
			}

			case 3: // Accept changes
			{
				RestoreRound(g_targetRound, true, client); // round, isAdminTriggered, client
			}

			case 4: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
				Command_RefereeMenu(client, 1);
			}
		}
	}
}

public int PanelHandler_PenaltyMenu(Handle menu, MenuAction action, int client, int choice)
{
	g_isCurrentlyGivingPenalty[client] = false;

	if (action == MenuAction_Select) {
		switch (choice)
		{
			case 1: // Penalize Jinrai
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				PenalizeTeam(client, TEAM_JINRAI);
			}

			case 2: // Penalize NSF
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				PenalizeTeam(client, TEAM_NSF);
			}

			case 3: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
				Command_RefereeMenu(client, 1);
			}
		}
	}
}

public int PanelHandler_PenalizeTeam(Handle menu, MenuAction action, int client, int choice)
{
	if (action == MenuAction_Select) {
		switch (choice)
		{
			case 1: // Apply team penalty
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				ApplyPenalty(client, g_penalizedTeam);
			}

			case 2: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
				Command_RefereeMenu(client, 1);
			}
		}
	}
}

public int PanelHandler_RefereeMenu_GameInfo(Handle menu, MenuAction action, int client, int choice)
{
	if (action == MenuAction_Select) {
		switch (choice)
		{
			case 1: // Edit competition name
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				ChatCvarEdit(client, g_hCompetitionName);
			}

			case 2: // Edit Jinrai name
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				ChatCvarEdit(client, g_hJinraiName);
			}

			case 3: // Edit NSF name
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				ChatCvarEdit(client, g_hNSFName);
			}

			case 4: // Edit max round count
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				ChatCvarEdit(client, g_hRoundLimit);
			}

			case 5: // Edit max time-out pause count for teams
			{
				PrecacheSound(g_menuSoundOk);
				EmitSoundToClient(client, g_menuSoundOk);
				ChatCvarEdit(client, g_hMaxTimeouts);
			}

			case 6: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
				Command_RefereeMenu(client, 1);
			}
		}
	}
}

public int PanelHandler_RefereeMenu_LoadMatch_ViewMeta(Handle menu, MenuAction action, int client, int choice)
{
#if defined KV_DEBUG
	if (action == MenuAction_Select) {
		switch (choice)
		{
			case 1: // Flip "go live after kv restore" bool
			{
				g_goLiveAfterKvRestore[client] = !g_goLiveAfterKvRestore[client];
				Menu_KvDetails(client, g_kvName_Temp[client]);
			}
			case 2: // Load match
			{
				Kv_RestoreFromFile(g_kvName_Temp[client], g_goLiveAfterKvRestore[client]);
			}
			case 3: // Next
			{
				Kv_Menu_Details_GetParallelFile(client, g_kvName_Temp[client], false);
			}
			case 4: // Previous
			{
				Kv_Menu_Details_GetParallelFile(client, g_kvName_Temp[client], true);
			}
			case 5: // Back
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
				RefereeMenu_LoadMatch(client);
			}
			case 6: // Exit menu
			{
				PrecacheSound(g_menuSoundCancel);
				EmitSoundToClient(client, g_menuSoundCancel);
				Command_RefereeMenu(client, 1);
			}
		}
	}
#else
	PrecacheSound(g_menuSoundCancel);
	EmitSoundToClient(client, g_menuSoundCancel);
	Command_RefereeMenu(client, 1);
#endif
}

Menu BuildClientMenu()
{
	Menu menu = new Menu(Menu_Clients);

	for (int i = 1; i <= MaxClients; ++i)
	{
		if ( !IsValidClient(i) || IsFakeClient(i) )
			continue;

		int team = GetClientTeam(i);
		if (team != TEAM_JINRAI && team != TEAM_NSF)
			continue;

		char clientName[MAX_NAME_LENGTH];
		char clientStr[3];
		IntToString(i, clientStr, sizeof(clientStr));
		GetClientName( i, clientName, sizeof(clientName) );

		menu.AddItem(clientStr, clientName);
	}

	menu.SetTitle("Client menu");

	return menu;
}

public int Menu_Clients(Menu menu, MenuAction action, int client, int choice)
{
	if (action == MenuAction_Select) {
		char targetString[32];
		menu.GetItem( choice, targetString, sizeof(targetString) );

		int target = StringToInt(targetString);

		if ( !IsValidClient(target) )
		{
			PrintToChat(client, "This player has left the server.");
			return;
		}

		RefereeMenu_ClientActions(client);
	}
}

#if defined KV_DEBUG
void Menu_KvDetails(int client, const char[] kvFileName, bool outOfBounds = false)
{
	if ( !IsValidClient(client) || !IsClientInGame(client) )
		return;

	if (strlen(kvFileName) < 1)
	{
		LogError("Attempted to restore KeyValues from zero length filename");
		return;
	}

	if ( !Kv_IsValidFile(kvFileName) )
	{
		LogError("fileName doesn't contain .kv extension. This shouldn't happen.");
		return;
	}

	char fullPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, fullPath, sizeof(fullPath), "%s/%s", g_kvPath, kvFileName);

	// Import KeyValues from file
	KeyValues kv = new KeyValues("");

	if ( !kv.ImportFromFile(fullPath) )
	{
		LogError("Failed importing KeyValues from path: %s", fullPath);
		return;
	}

	// Prepare to fetch metadata
	kv.JumpToKey("meta", false);

	// Get this match's date/time
	char readableTime[64];
	FormatTime( readableTime, sizeof(readableTime), "%Y%m%d-%H%M", kv.GetNum("timestamp") );

	// Get match title, if any
	char matchTitle[MAX_CVAR_LENGTH];
	kv.GetString( g_kvKey_competitionName, matchTitle, sizeof(matchTitle) );
	if (strlen(matchTitle) < 1)
		strcopy(matchTitle, sizeof(matchTitle), "untitled match");

	// Get team names, if any
	char jinraiName[MAX_CVAR_LENGTH];
	kv.GetString( g_kvKey_jinraiName, jinraiName, sizeof(jinraiName) );
	char nsfName[MAX_CVAR_LENGTH];
	kv.GetString( g_kvKey_nsfName, nsfName, sizeof(nsfName) );

	kv.GoBack();

	char roundLabel[16] = "round";
	int roundCount = 1;
	char labelBuffer[16];
	Format(labelBuffer, sizeof(labelBuffer), "%s%i", roundLabel, roundCount);

	while ( kv.JumpToKey(labelBuffer, false) )
	{
		roundCount++;
		Format(labelBuffer, sizeof(labelBuffer), "%s%i", roundLabel, roundCount);

		kv.GoBack();
	}
	roundCount--;

	Format(labelBuffer, sizeof(labelBuffer), "%s%i", roundLabel, roundCount);
	kv.JumpToKey(labelBuffer, false);

	int jinraiScore = kv.GetNum(g_kvKey_jinraiScore);
	int nsfScore = kv.GetNum(g_kvKey_nsfScore);
	int roundLimit = kv.GetNum(g_kvKey_roundLimit);

	// Done reading from kv
	delete kv;

	// Create panel
	Handle panel = CreatePanel();
	// Store selected kv filename for use in callbacks
	strcopy(g_kvName_Temp[client], sizeof(g_kvName_Temp), kvFileName);

	SetPanelTitle(panel, "Match information");

	if (outOfBounds)
		DrawPanelText(panel, "(Last file reached.)");

	DrawPanelText(panel, readableTime);
	DrawPanelText(panel, matchTitle);

	char infoBuffer[128];
	Format(infoBuffer, sizeof(infoBuffer), "Jinrai %i vs %i NSF", jinraiScore, nsfScore);
	DrawPanelText(panel, infoBuffer);

	Format(infoBuffer, sizeof(infoBuffer), "Round limit: %i", roundLimit);
	DrawPanelText(panel, infoBuffer);

	char liveFromRestore[27] = "Go live after restore: ";
	if (g_goLiveAfterKvRestore[client])
		StrCat(liveFromRestore, sizeof(liveFromRestore), "yes");
	else
		StrCat(liveFromRestore, sizeof(liveFromRestore), "no");

	DrawPanelItem(panel, liveFromRestore);
	DrawPanelItem(panel, "Load this match");
	DrawPanelItem(panel, "--->");
	DrawPanelItem(panel, "<---");
	DrawPanelItem(panel, "Back");
	DrawPanelItem(panel, "Exit");

	SendPanelToClient(panel, client, PanelHandler_RefereeMenu_LoadMatch_ViewMeta, MENU_TIME_FOREVER);

	delete panel;
}

public int MenuHandler_RefereeMenu_LoadMatch(Menu menu, MenuAction action, int client, int choice)
{
	if (action == MenuAction_Select) {
		char kvFileName[PLATFORM_MAX_PATH];
		menu.GetItem( choice, kvFileName, sizeof(kvFileName) );

		Menu_KvDetails(client, kvFileName);

		//Kv_RestoreFromFile(kvFileName);

		//	PrintToServer("File chosen: %s", kvFileName);
	}
}
#endif
