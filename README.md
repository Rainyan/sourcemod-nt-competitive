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

Foo

## Cvars

## Ghost overtime
To illustrate how the timer behaves with default settings:
Ghost overtime kicks in at 15 seconds on the clock and will add a maximum of 45 seconds to the round. This gives us a total of 15 + 45 = 60 seconds for the timer of 15 seconds to decay. 60 / 15 = 4, meaning each second on the clock during ghost overtime will last for 4 real seconds.

    * Ghost is picked up at 15 seconds and is held for 16 seconds. The timer shows 10 (15 - 16/4 = 11, rounded down to 10 for simplicity).
    * The ghost is dropped, making the timer speed up.
    * 8 seconds later the ghost is picked up again. Since grace reset is enabled, the timer jumps from 2 to 8 seconds (10 - 8/4 = 8).
