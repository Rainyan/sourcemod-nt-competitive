# sourcemod-nt-competitive

SourceMod plugin for competitive Neotokyo. WIP.

* * *

### Recommended additional plugins for competitive
* [nt_fadefix](https://github.com/Rainyan/sourcemod-nt-fadefix) - Block any unintended un-fade user messages. Hide new round vision to block "ghosting" for opposing team's loadouts.

* * *

## Features
- Count rounds, announce winner
- Automatic player demo and SourceTV recording
- Handle dis/reconnecting players and teams
- Pausing and ready-up systems
- Option to disable timeout round wins

## Build requirements
- <a target="_blank" href="http://www.sourcemod.net/downloads.php?branch=stable">SourceMod</a>, version 1.7.3 or newer
- <a target="_blank" href="https://github.com/softashell/sourcemod-nt-include">neotokyo.inc include</a>, version 1.2 or newer

Some optional features also require:
- <a target="_blank" href="https://github.com/softashell/nt-sourcemod-plugins">Ghostcap event plugin</a>, version 1.9.0 or newer

## Cvars
### nt_competitive.sp
* sm_competitive_live
  * Default value: `0`
  * Description: `Whether the match is live`
  * Bit flags: `FCVAR_NOTIFY`
  * Min: `float(false)`
  * Max: `float(true)`
* sm_competitive_jinrai_score
  * Default value: `0`
  * Description: `Team Jinrai score for a competitive match`
  * Min: `float(0)`
  * Max: `float(99)`
* sm_competitive_nsf_score
  * Default value: `0`
  * Description: `Team NSF score for a competitive match`
  * Min: `float(0)`
  * Max: `float(99)`
* sm_competitive_win_condition
  * Default value: `0`
  * Description: `Win condition enumeration for competitive play`
  * Bit flags: `FCVAR_NOTIFY`
  * Min: `float(0)`
  * Max: `float(WinCondition_EnumCount) - 1`
* sm_competitive_limit
  * Default value: `15`
  * Description: `Default score limit for competitive play. The meaning depends  on \"sm_competitive_win_condition\" value.`
  * Bit flags: `FCVAR_NOTIFY`
  * Min: `float(1)`
  * Max: `float(99)`
* sm_competitive_jinrai_name
  * Default value: `Jinrai`
  * Description: `Default Jinrai team name`
  * Bit flags: `FCVAR_NOTIFY`
* sm_competitive_nsf_name
  * Default value: `NSF`
  * Description: `Default NSF team name`
  * Bit flags: `FCVAR_NOTIFY`
* sm_competitive_log_lvl
  * Default value: `1`
  * Description: `Logging level`
  * Min: `float(0)`


## Ghost overtime
To illustrate how the timer behaves with default settings:
Ghost overtime kicks in at 15 seconds on the clock and will add a maximum of 45 seconds to the round. This gives us a total of 15 + 45 = 60 seconds for the timer of 15 seconds to decay. 60 / 15 = 4, meaning each second on the clock during ghost overtime will last for 4 real seconds.

    * Ghost is picked up at 15 seconds and is held for 16 seconds. The timer shows 10 (15 - 16/4 = 11, rounded down to 10 for simplicity).
    * The ghost is dropped, making the timer speed up.
    * 8 seconds later the ghost is picked up again. Since grace reset is enabled, the timer jumps from 2 to 8 seconds (10 - 8/4 = 8).

## Player score manipulation
The nt_competitive plugin modifies player score/deaths values under some conditions. If you're a plugin developer and would like to override this behaviour, a global forward is exposed for this:
```sp
function Action Competitive_OnPlayerScoreChange(PlayerScoreChangeReason reason, int client);
```

The shared header is available at [scripting/nt_competitive/nt_competitive_shared.inc](./scripting/nt_competitive/nt_competitive_shared.inc).

Example of overriding the score change behaviour from your plugin:
```sp
#include <sourcemod>

// needed for the PlayerScoreChangeReason enum definition
#include "nt_competitive/nt_competitive_shared"

// optional: handle cases where the forward doesn't exist,
// if it's critical to your plugin
public void OnAllPluginsLoaded()
{
    if (!LibraryExists("CompetitiveFwds"))
    {
        PrintToServer("Competitive forwards don't exist!");

        if (null != FindConVar("sm_competitive_version"))
        {
            // nt_competitive exists, but it doesn't have the forward.
            // Is the nt_competitive plugin out of date?
        }
        else
        {
            // nt_competitive plugin does not exist
        }
    }
}

// listener for the global forward from nt_competitive
public Action Competitive_OnPlayerScoreChange(PlayerScoreChangeReason reason, int client)
{
    bool allow = true;

    if ( true /* replace with your custom condition! */ )
    {
        allow = false; // Block the nt_competitive score change!
    }

    // return Plugin_Continue to allow the nt_competitive score change.
    // return Plugin_Handled to block the nt_competitive score change.
    return allow ? Plugin_Continue : Plugin_Handled;
}

```
