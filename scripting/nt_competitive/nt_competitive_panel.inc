// Double-include prevention
#if defined _panel_included_
	#endinput
#endif
#define _panel_included_

public PanelHandler_Pause(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
	new team = GetClientTeam(client);
	
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

public PanelHandler_UnPause(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
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

public PanelHandler_CancelPause(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
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

public PanelHandler_RefereeMenu_Main(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
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
		
		case 5: // Load previous match
		{
			PrecacheSound(g_menuSoundOk);
			EmitSoundToClient(client, g_menuSoundOk);
			RefereeMenu_LoadMatch(client);
		}
		
		case 6: // Exit menu
		{
			PrecacheSound(g_menuSoundCancel);
			EmitSoundToClient(client, g_menuSoundCancel);
		}
	}
}

public PanelHandler_RefereeMenu_ChangeRound(Handle:menu, MenuAction:action, client, choice)
{
	g_isEditingSetting[client] = false; // We're done editing the setting when this gets called
	
	if (action != MenuAction_Select)
		return;
	
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

public PanelHandler_PenaltyMenu(Handle:menu, MenuAction:action, client, choice)
{
	g_isCurrentlyGivingPenalty[client] = false;
	
	if (action != MenuAction_Select)
		return;
	
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

public PanelHandler_PenalizeTeam(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
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

public PanelHandler_RefereeMenu_GameInfo(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
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

Menu BuildClientMenu()
{
	Menu menu = new Menu(Menu_Clients);
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if ( !Client_IsValid(i) || IsFakeClient(i) )
			continue;
		
		new team = GetClientTeam(i);
		if (team != TEAM_JINRAI && team != TEAM_NSF)
			continue;
		
		decl String:clientName[MAX_NAME_LENGTH];
		decl String:clientStr[3];
		IntToString(i, clientStr, sizeof(clientStr));
		GetClientName( i, clientName, sizeof(clientName) );
		
		menu.AddItem(clientStr, clientName);
	}
	
	menu.SetTitle("Client menu");
	
	return menu;
}

public int Menu_Clients(Menu menu, MenuAction action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
	decl String:targetString[32];
	menu.GetItem( choice, targetString, sizeof(targetString) );
	
	new target = StringToInt(targetString);
	
	if ( !Client_IsValid(target) )
	{
		PrintToChat(client, "This player has left the server.");
		return;
	}
	
	RefereeMenu_ClientActions(client);
}