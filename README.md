sourcemod-nt-competitive
========================

SourceMod plugin for competitive Neotokyo. WIP.

---

### Recommended additional plugins for competitive
* [nt_fadefix](https://github.com/Rainyan/sourcemod-nt-fadefix) - Block any unintended un-fade user messages. Hide new round vision to block "ghosting" for opposing team's loadouts.

---

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
| cvar    | description | default |
| -------- | ------- | ------- |
| sm_competitive_round_style  | How a match win is determined. 1 = best of X rounds, 2 = first to X points. | 1 |
| sm_competitive_round_limit | If sm_competitive_round_style equals 1, this is the best of X rounds. If it equals 2, this is the score required to win. | 15 |
| sm_competitive_sudden_death | If map ends in a tied scoreline, keep play going until someone wins? | 1 |
| sm_competitive_players_total | How many players total are expected to ready up before starting a competitive match. | 10 |
| sm_competitive_max_timeouts | How many time-outs are allowed per match per team. | 1 |
| sm_competitive_max_pause_length | How long can a competitive time-out last, in seconds. | 60
| sm_competitive_max_pause_length_technical | How long can a pause last when team experiences technical difficulties. | 300 |
| sm_competitive_sourcetv_enabled | Should the competitive plugin automatically record SourceTV demos. | 1 |
| sm_competitive_sourcetv_path | Directory to save SourceTV demos into. Relative to NeotokyoSource folder. Will be created if possible. | replays_competitive |
| sm_competitive_jinrai_name | Jinrai team's name. Will use "Jinrai" if left empty. | Jinrai |
| sm_competitive_nsf_name | NSF team's name. Will use "NSF" if left empty. | NSF |
| sm_competitive_title | Name of the tournament/competition. Also used for replay filenames. 32 characters max. Use only alphanumerics and spaces. | |
| sm_competitive_comms_behaviour | Voice comms behaviour when live. 0 = no alltalk, 1 = enable alltalk, 2 = check sv_alltalk value before live state. | 1 |
| sm_competitive_log_mode | Competitive logging mode. 1 = enabled, 0 = disabled. | 1 |
| sm_competitive_killverbosity | How much info is given to players upon death. 0 = disabled, 1 = print amount of players remaining to everyone, 2 = only show the victim how much damage they dealt to their killer, 3 = only show the victim their killer's remaining health. | 1 |
| sm_competitive_killverbosity_delay | 0 = display kill info instantly, 1 = display kill info nextround. | 0 |
| sm_competitive_record_clients | Should clients automatically record when going live. | 0 |
| sm_competitive_limit_live_teams | Are players restricted from changing teams when a game is live. | 1 |
| sm_competitive_limit_teams | Are teams enforced to use set numbers (5v5 for example). | 0 |
| sm_competitive_pause_mode | Pausing mode. 0 = no pausing allowed, 1 = use Source engine pause feature, 2 = stop round timer. | 2 |
| sm_competitive_readymode_collective | Can a team collectively ready up by any one of the players. Can be useful for more organized events. | 0 |
| sm_competitive_nozanshi | Whether or not to disable timeout wins. | 1 |
| sm_competitive_jinrai_score | Competitive plugin's internal score cvar. Editing this will directly affect comp team scores. | 0 |
| sm_competitive_nsf_score | Competitive plugin's internal score cvar. Editing this will directly affect comp team scores. | 0 |
| sm_competitive_display_remaining_players_centered | How the number of remaining players is displayed to clients in a competitive game. 0 = disabled, 1 = show remaining player numbers, 2 = show team names and remaining player numbers. | 2 |
| sm_competitive_display_remaining_players_target | Who to center display remaining players to. 1 = spectators only, 2 = spectators and dead players, 3 = everyone. | 2 |
| sm_competitive_display_remaining_players_divider | What kind of divider to use between the scores (eg. 3 vs 2, 3 v 2, 3--2). | "—"

The overtime is controlled by four new cvars:
| cvar    | description | default |
| -------- | ------- | ------- |
| sm_competitive_ghost_overtime | Controls in seconds the maximum amount of overtime that can be added to the base clock. | 45 |
| sm_competitive_ghost_overtime_grace | Controls at how many seconds on the clock the overtime kicks in. | 15 |
| sm_competitive_ghost_overtime_grace_reset | Controls whether the grace time should be reset when picking up the ghost. The grace time will never be reset fully, but instead set to what the timer would have been at if the ghost had been held from when the timer showed sm_competitive_ghost_overtime_grace. I.e. a round can never be extended by more than what sm_competitive_ghost_overtime is set to. | 1 |
| sm_competitive_ghost_overtime_decay_exp | There are two modes for the timer decay. By default the decay is linear, but by setting this cvar to 1 the time will decay exponentially, moving slowly to begin with and then faster and faster as the timer reaches 0. This means that the grace period will remain long for longer which may make sense in conjunction with the setting above. | 0 |

To illustrate how the timer behaves with default settings:
Ghost overtime kicks in at 15 seconds on the clock and will add a maximum of 45 seconds to the round. This gives us a total of 15 + 45 = 60 seconds for the timer of 15 seconds to decay. 60 / 15 = 4, meaning each second on the clock during ghost overtime will last for 4 real seconds.

    * Ghost is picked up at 15 seconds and is held for 16 seconds. The timer shows 10 (15 - 16/4 = 11, rounded down to 10 for simplicity).
    * The ghost is dropped, making the timer speed up.
    * 8 seconds later the ghost is picked up again. Since grace reset is enabled, the timer jumps from 2 to 8 seconds (10 - 8/4 = 8).
