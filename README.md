# sourcemod-nt-competitive

SourceMod plugin for competitive Neotokyo. WIP.

* * *

### Recommended additional plugins for competitive
* [nt_fadefix](https://github.com/Rainyan/sourcemod-nt-fadefix) - Block any unintended un-fade user messages. Hide new round vision to block "ghosting" for opposing team's loadouts.

* * *

Features:
- Automatic demo recording
- Automatic SourceTV recording
- Count rounds, announce winner
- Handle dis/reconnecting players and teams
- Pausing system
- Ready up system
- ~~Panel menus for admins~~
- ~~Ability to fall back to previous rounds~~
- Option to disable timeout round wins

To-do:
- Panel menus for players
- Panel menus for casters
- Log competitive matches (wins, kills, weapons used, etc)

Compile dependencies:
- <a target="_blank" href="https://github.com/softashell/sourcemod-nt-include">SourceMod NT include</a>, version 1.0 or newer

Server requirements:
- <a target="_blank" href="http://www.sourcemod.net/downloads.php?branch=stable">SourceMod</a> 1.7 or later

Some optional features also require:
- Up-to-date <a target="_blank" href="https://github.com/alliedmodders/sourcemod/tree/master/gamedata">Neotokyo gamedata</a>
- <a target="_blank" href="https://github.com/softashell/nt-sourcemod-plugins">Ghostcap event plugin</a> 1.9.0 or later

#### Cvars
### nt_competitive.sp
* sm_competitive_version
  * Default value: `PLUGIN_VERSION`
  * Description: `Competitive plugin version.`
  * Bit flags: `FCVAR_DONTRECORD`
* sm_competitive_round_style
  * Default value: `1`
  * Description: `How a match win is determined. 1 = best of X rounds, 2 = first to X points`
  * Min: `1.0`
  * Max: `2.0`
* sm_competitive_round_limit
  * Default value: `15`
  * Description: `If sm_competitive_round_style equals 1, this is the best of X rounds. If it equals 2, this is the score required to win.`
  * Min: `1.0`
* sm_competitive_players_total
  * Default value: `10`
  * Description: `How many players total are expected to ready up before starting a competitive match.`
* sm_competitive_max_timeouts
  * Default value: `1`
  * Description: `How many time-outs are allowed per match per team.`
  * Min: `0.0`
* sm_competitive_max_pause_length
  * Default value: `60`
  * Description: `How long can a competitive time-out last, in seconds.`
  * Min: `0.0`
* sm_competitive_max_pause_length_technical
  * Default value: `300`
  * Description: `How long can a pause last when team experiences technical difficulties.`
  * Min: `0.0`
* sm_competitive_sourcetv_enabled
  * Default value: `1`
  * Description: `Should the competitive plugin automatically record SourceTV demos.`
  * Min: `0.0`
  * Max: `1.0`
* sm_competitive_sourcetv_path
  * Default value: `replays_competitive`
  * Description: `Directory to save SourceTV demos into. Relative to NeotokyoSource folder. Will be created if possible.`
* sm_competitive_jinrai_name
  * Default value: `Jinrai`
  * Description: `Jinrai team's name. Will use \"Jinrai\" if left empty.`
* sm_competitive_nsf_name
  * Default value: `NSF`
  * Description: `NSF team's name. Will use \"NSF\" if left empty.`
* sm_competitive_title
  * Default value: ``
  * Description: `Name of the tournament/competition. Also used for replay filenames. 32 characters max. Use only alphanumerics and spaces.`
* sm_competitive_comms_behaviour
  * Default value: `0`
  * Description: `Voice comms behaviour when live. 0 = no alltalk, 1 = enable alltalk, 2 = check sv_alltalk value before live state`
  * Min: `0.0`
  * Max: `2.0`
* sm_competitive_log_mode
  * Default value: `1`
  * Description: `Competitive logging mode. 1 = enabled, 0 = disabled.`
  * Min: `0.0`
  * Max: `1.0`
* sm_competitive_killverbosity
  * Default value: `1`
  * Description: `How much info is given to players upon death. 0 = disabled, 1 = print amount of players remaining to everyone, 2 = only show the victim how much damage they dealt to their killer, 3 = only show the victim their killer's remaining health`
  * Min: `0.0`
  * Max: `3.0`
* sm_competitive_killverbosity_delay
  * Default value: `0`
  * Description: `0 = display kill info instantly, 1 = display kill info nextround`
  * Min: `0.0`
  * Max: `1.0`
* sm_competitive_record_clients
  * Default value: `0`
  * Description: `Should clients automatically record when going live.`
  * Min: `0.0`
  * Max: `1.0`
* sm_competitive_limit_live_teams
  * Default value: `0`
  * Description: `Team restrictions when game is live. 0 = players can join any team, 1 = only players present when going live can play in their teams 2 = new players can join teams midgame`
  * Min: `0.0`
  * Max: `2.0`
* sm_competitive_limit_teams
  * Default value: `1`
  * Description: `Are teams enforced to use set numbers (5v5 for example).`
  * Min: `0.0`
  * Max: `1.0`
* sm_competitive_pause_mode
  * Default value: `2`
  * Description: `Pausing mode. 0 = no pausing allowed, 1 = use Source engine pause feature, 2 = stop round timer`
  * Min: `0.0`
  * Max: `2.0`
* sm_competitive_readymode_collective
  * Default value: `0`
  * Description: `Can a team collectively ready up by anyone of the players. Can be useful for more organized events.`
  * Min: `0.0`
  * Max: `1.0`
* sm_competitive_nozanshi
  * Default value: `1`
  * Description: `Whether or not to disable timeout wins.`
  * Min: `0.0`
  * Max: `1.0`
* sm_competitive_jinrai_score
  * Default value: `0`
  * Description: `Competitive plugin's internal score cvar. Editing this will directly affect comp team scores.`
  * Min: `0.0`
* sm_competitive_nsf_score
  * Default value: `0`
  * Description: `Competitive plugin's internal score cvar. Editing this will directly affect comp team scores.`
  * Min: `0.0`
* sm_competitive_sudden_death
  * Default value: `1`
  * Description: `Whether or not to allow match to end in a tie. Otherwise, game proceeds to sudden death until one team scores a point.`
  * Min: `0.0`
  * Max: `1.0`
* sm_competitive_display_remaining_players_centered
  * Default value: `2`
  * Description: `How the number of remaining players is displayed to clients in a competitive game. 0 = disabled, 1 = show remaining player numbers, 2 = show team names and remaining player numbers`
  * Min: `0.0`
  * Max: `2.0`
* sm_competitive_display_remaining_players_target
  * Default value: `2`
  * Description: `Who to center display remaining players to. 1 = spectators only, 2 = spectators and dead players, 3 = everyone`
  * Min: `1.0`
  * Max: `3.0`
* sm_competitive_display_remaining_players_divider
  * Default value: `—`
  * Description: `What kind of divider to use between the scores (eg. 3 vs 2, 3 v 2, 3--2)`
* sm_competitive_ghost_overtime
  * Default value: `45`
  * Description: `Add up to this many seconds to the round time while the ghost is held.`
  * Min: `0.0`
  * Max: `120.0`
* sm_competitive_ghost_overtime_grace
  * Default value: `15`
  * Description: `Freeze the round timer at this many seconds while the ghost is held. Will decay and end at 0 when the overtime runs out. 0 = disabled`
  * Min: `0.0`
  * Max: `30.0`
* sm_competitive_ghost_overtime_decay_exp
  * Default value: `0`
  * Description: `Whether ghost overtime decay should be exponential or linear. Exponential requires grace reset to be enabled. 0 = linear, 1 = exponential`
  * Min: `0.0`
  * Max: `1.0`
* sm_competitive_ghost_overtime_grace_reset
  * Default value: `1`
  * Description: `When the ghost is picked up, reset the timer to where it would be on the decay curve if the ghost was never dropped. This means the full overtime can be used even when juggling.`
  * Min: `0.0`
  * Max: `1.0`





#### Ghost overtime
To illustrate how the timer behaves with default settings:
Ghost overtime kicks in at 15 seconds on the clock and will add a maximum of 45 seconds to the round. This gives us a total of 15 + 45 = 60 seconds for the timer of 15 seconds to decay. 60 / 15 = 4, meaning each second on the clock during ghost overtime will last for 4 real seconds.

    * Ghost is picked up at 15 seconds and is held for 16 seconds. The timer shows 10 (15 - 16/4 = 11, rounded down to 10 for simplicity).
    * The ghost is dropped, making the timer speed up.
    * 8 seconds later the ghost is picked up again. Since grace reset is enabled, the timer jumps from 2 to 8 seconds (10 - 8/4 = 8).
