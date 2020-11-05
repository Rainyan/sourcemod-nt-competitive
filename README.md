sourcemod-nt-competitive
========================

SourceMod plugin for competitive Neotokyo. WIP.

The main module should already work.<br>
The overlay and matchmaking modules are experimental and may not work properly.

Features:
  - Automatic demo recording
  - Automatic SourceTV recording
  - Count rounds, announce winner
  - Fade to black
  - Handle dis/reconnecting players and teams
  - Pausing system
  - Ready up system
  - Panel menus for admins
  - Ability to fall back to previous rounds
  - Option to disable timeout round wins

To-do:
  - Panel menus for players
  - Panel menus for casters
  - Log competitive matches (wins, kills, weapons used, etc)

Compile dependencies:
  - <a target="_blank" href="https://github.com/softashell/sourcemod-nt-include">SourceMod NT include</a>

Server requirements:
  - <a target="_blank" href="http://www.sourcemod.net/downloads.php?branch=stable">SourceMod</a> 1.7 or later

Some optional features also require:
  - Up-to-date <a target="_blank" href="https://github.com/alliedmodders/sourcemod/tree/master/gamedata">Neotokyo gamedata</a>
  - <a target="_blank" href="https://github.com/softashell/nt-sourcemod-plugins">Ghostcap event plugin</a> 1.5.4 or later

Plugin cvars:
```
sm_competitive_round_style                              - How a match win is determined. 1 = best of X rounds, 2 = first to X points. Default: 1
sm_competitive_round_limit								- If sm_competitive_round_style equals 1, this is the best of X rounds. If it equals 2, this is the score required to win. Default: 15
sm_competitive_sudden_death								- If map ends in a tied scoreline, keep play going until someone wins? Default: 1
sm_competitive_players_total							- How many players total are expected to ready up before starting a competitive match. Default: 10
sm_competitive_max_timeouts								- How many time-outs are allowed per match per team. Default: 1
sm_competitive_max_pause_length							- How long can a competitive time-out last, in seconds. Default: 60
sm_competitive_max_pause_length_technical				- How long can a pause last when team experiences technical difficulties. Default: 300
sm_competitive_sourcetv_enabled							- Should the competitive plugin automatically record SourceTV demos. Default: 1
sm_competitive_sourcetv_path							- Directory to save SourceTV demos into. Relative to NeotokyoSource folder. Will be created if possible. Default: replays_competitive
sm_competitive_jinrai_name								- Jinrai team's name. Will use "Jinrai" if left empty. Default: Jinrai
sm_competitive_nsf_name									- NSF team's name. Will use "NSF" if left empty. Default: NSF
sm_competitive_title									- Name of the tournament/competition. Also used for replay filenames. 32 characters max. Use only alphanumerics and spaces. Default:
sm_competitive_comms_behaviour							- Voice comms behaviour when live. 0 = no alltalk, 1 = enable alltalk, 2 = check sv_alltalk value before live state. Default: 0
sm_competitive_log_mode									- Competitive logging mode. 1 = enabled, 0 = disabled. Default: 1
sm_competitive_killverbosity							- How much info is given to players upon death. 0 = disabled, 1 = print amount of players remaining to everyone, 2 = only show the victim how much damage they dealt to their killer, 3 = only show the victim their killer's remaining health. Default: 1
sm_competitive_killverbosity_delay						- 0 = display kill info instantly, 1 = display kill info nextround. Default: 0
sm_competitive_record_clients							- Should clients automatically record when going live. Default: 0
sm_competitive_limit_live_teams							- Are players restricted from changing teams when a game is live. Default: 1
sm_competitive_limit_teams								- Are teams enforced to use set numbers (5v5 for example). Default: 0
sm_competitive_pause_mode								- Pausing mode. 0 = no pausing allowed, 1 = use Source engine pause feature, 2 = stop round timer.  Default: 2
sm_competitive_readymode_collective						- Can a team collectively ready up by any one of the players. Can be useful for more organized events. Default: 0
sm_competitive_nozanshi									- Whether or not to disable timeout wins. Default: 1
sm_competitive_jinrai_score								- Competitive plugin's internal score cvar. Editing this will directly affect comp team scores. Default: 0
sm_competitive_nsf_score								- Competitive plugin's internal score cvar. Editing this will directly affect comp team scores. Default: 0
sm_competitive_display_remaining_players_centered		- How the number of remaining players is displayed to clients in a competitive game. 0 = disabled, 1 = show remaining player numbers, 2 = show team names and remaining player numbers. Default: 2
sm_competitive_display_remaining_players_target  		- Who to center display remaining players to. 1 = spectators only, 2 = spectators and dead players, 3 = everyone. Default: 2
sm_competitive_keyvalues_test							- Store match data into KeyValues file. Debug cvar. Default: 1
sm_competitive_force_camera				    			- Should fade to black be forced on death when live. Can be useful to disable on pugs etc. Default: 1
```
