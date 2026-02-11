# Light-Year-Wars-C
A replica of the Adobe Flash game "Light Year Wars" by Nico Tuason.

Written in C with OpenGL. Requires various Windows and OpenGL header files.

Run `make -B` to compile both the server and client executables.

I used GCC from [WinLibs](https://winlibs.com/) when writing the code, 
so it's probably the best place to look if your version of GCC is erroring when
trying to run make.
The specific GCC version as printed when I run `gcc --version` is
```
gcc.exe (MinGW-W64 i686-msvcrt-posix-dwarf, built by Brecht Sanders, r5) 15.2.0
```

# Controls

## Login menu
Input the IP address and port of the server, and provide a user name. The default port is 22311.
Left click on the input field to select it, then type. Press tab to move to the next lower field,
or shift tab to move to the next upper field. Press enter to attempt to connect.

## Lobby
In the lobby, all settings can be changed by the server. Simply click on the field which you wish to edit, 
and either type in the desired values if it is a numerical or text field, or select from a dropdown if it's
a dropdown field. For the color picker specifically, adjust the red, green, and blue values by clicking the color
box to the right of a player's slot, then click off to save the color.

Field rundown:
- Planet Count: total number of planets to generate.
- Faction Count: number of player slots (2 - 48). Must be greater than 2 and less than Planet Count.
- Min Fleet Capacity: minimum size of planets in terms of fleet capacity (how many ships the planet will spawn before it stops spawning any more).
- Max Fleet Capacity: maximum size of planets in terms of fleet capacity (how many ships the planet will spawn before it stops spawning any more).
- Level Width: world width in game units.
- Level Height: world height in game units.
- Random Seed: use 0 for a random seed, or enter a specific seed for repeatable layouts.

Slot rundown:
- Team: team number for alliances; matching teams are allied.
- Shared Control (archon): matching values allow allies to issue orders from each other's planets. Both the team and archon numbers must match if you want to share control with someone (or the AI!).
- AI: pick an AI personality when the slot is unoccupied.
- Color: edit the faction color using the RGB picker.
- Preview Level: opens/closes the level preview panel.

Press tab to move to the next lower field,
or shift tab to move to the next upper field. Press enter to attempt to start the game.

The client may only edit lobby setting pertaining directly to their slot (so their own team and archon numbers 
and their own color.) The client may not set themselves to be played as an AI, and neither can the server until
the player is not longer occupying the slot.

## In-game

Selection and orders:
- Left click a planet to select it.
- Drag with left click to box-select planets.
- Hold Shift to add to selection; without Shift, a new selection replaces the old one.
- Right click a destination planet to send ships from all selected planets.

Camera:
- Pan with WASD or arrow keys.
- Pan by moving the mouse to the screen edges.
- Zoom with the mouse wheel.

Control groups:
- Number keys 0-9 recall control groups.
- Ctrl + number overwrites a control group with the current selection.
- Shift + number adds the current selection to that control group.
- F2 selects all planets owned (or shared-controlled) by your faction.

