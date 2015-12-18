sourcemod-nt-competitive
========================

SourceMod plugin for competitive Neotokyo. WIP.

The main module should already work.<br>
The overlay and matchmaking modules are experimental and may not work properly.

Features:
  - Automatic demo recording.
  - Automatic SourceTV recording
  - Count rounds, announce winner.
  - Fade to black
  - Handle dis/reconnecting players and teams
  - Pausing system.
  - Ready up system.
  - Panel menus for admins
  - Ability to fall back to previous rounds
  - Option to disable timeout round wins

To-do:
  - Panel menus for players
  - Panel menus for casters
  - Log competitive matches (wins, kills, weapons used, etc)

Compile dependencies:
  - <a target="_blank" href="https://github.com/bcserv/smlib/">SMLIB</a>
  - <a target="_blank" href="https://github.com/softashell/sourcemod-nt-include">SourceMod NT include</a>

Server requirements:
  - <a target="_blank" href="http://www.sourcemod.net/downloads.php?branch=stable">SourceMod</a> 1.7.0 or later

Some optional features also require:
  - Up-to-date <a target="_blank" href="https://github.com/alliedmodders/sourcemod/tree/master/gamedata">Neotokyo gamedata</a>
  - <a target="_blank" href="https://github.com/softashell/nt-sourcemod-plugins">Ghostcap event plugin</a> 1.5.1 or later

