// Double-include prevention
#if defined _overlay_panel_included_
	#endinput
#endif
#define _overlay_panel_included_

public PanelHandler_CasterMenu(Handle:menu, MenuAction:action, client, choice)
{
	if (action != MenuAction_Select)
		return;
	
	switch (choice)
	{
		case 1:
			SetCasterMessage(client);
		
		case 2:
			ToggleCasterMessage(client);
	}
}