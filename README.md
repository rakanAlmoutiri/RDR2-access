# Red Dead Redemption 2 - Accessibility Mod (Rdr access)
**
           RDR2 Accessibility Mod (v1.9.2 - Beta)
**

IMPORTANT: This is an experimental build currently under active testing and development.
The entire mod is under testing. All individual features (teleportation, loot announcements, auto-aim, and bodyguards) are experimental and subject to bugs, stutters, or crashes. Please report issues and back up your save files.

**
[ INSTALLATION ]
**
1. Download Script Hook RDR2 from the official site:
   http://www.dev-c.com/rdr2/scripthookrdr2/
   
2. Open the downloaded ZIP, go to the 'bin' folder, and grab ONLY these two files:
   - ScriptHookRDR2.dll
   - dinput8.dll
   Drop them into your RDR2 main game folder (where RDR2.exe is).

3. Now, grab the mod files and paste them into your RDR2 main game folder:
   - RDR access.asi
   - nvdaControllerClient64.dll
   - SAAPI64.dll
   - tolk.dll

4. Make sure NVDA or your screen reader is running before launching the game.
5. Launch Red Dead Redemption 2.

**
[ CONTROLS & MENU NAVIGATION ]
**
- Open / Close Menu: Press [ F4 ] or [ F5 ]
- Navigate Up / Down: Press [ Numpad 8 ] and [ Numpad 2 ]
- Change Values (Left/Right): Press [ Numpad 4 ] and [ Numpad 6 ]
- Select / Enter: Press [ Numpad 5 ]
- Go Back: Press [ Numpad 0 ] or [ Backspace ]
- Speak Selected Option: Press [ Tab ]

**
[ NUMPAD QUICK MODE SYSTEM ]
**
Outside the menu, press [ Numpad 5 ] to cycle through 3 accessibility modes. Each mode configures what the Numpad keys do:

[ GLOBAL MODE ] (General Player Status)
- Numpad 0: Current Location and Compass.
- Numpad 3: Current Weapon and Ammo.
- Numpad 4: Health, Core stats, and Cash.
- Numpad 6: Game Time, Stamina/Deadeye bars, and Bounty.
- Numpad 7: Wanted level, Bounty value, and Intensity.
- Numpad 9: Player Honor level.

[ HORSE MODE ] (Mounted Status)
- Numpad 0: Horse call (whistle) and distance check.
- Numpad 1: Horse Breed, bonding level, and distance.
- Numpad 2: Horse movement state and speed.
- Numpad 3: Horse Stamina and Health bars.
- Numpad 4: Horse Health and Stamina Cores.

[ BODYGUARD MODE ] (Squad Commands)
- Numpad 0: Cycle squad formation (Column, Line, Wedge).
- Numpad 1: Check active guard status, combat, and types.
- Numpad 2: Regroup (teleport guards to player).
- Numpad 6: Toggle auto-defense (guards protect player automatically).
- Numpad 7: Command guards to follow.
- Numpad 8: Command guards to hold position.
- Numpad 9: Command guards to guard area.

**
[ SPECIAL FEATURES (UNDER ACTIVE TESTING) ]
**
- **Loot Announcer & Inventory Tracker:** Tracks your inventory in the background and automatically speaks added items and quantities (e.g. "Added 3 Yarrow", "Added 1 Gold Ring"). Monitors 100% of satchel items (all 13 fish, plants, jewelry, animal parts). Speaks "No items added. Inventory might be full" if a loot/skin animation finishes without gaining anything.
- **Auto-Aim System:** Low-vision combat aid that automatically locks onto the chest/torso of the nearest hostile target within 150 meters when holding the aim button.
- **Numpad Decimal Point Key (.):** Diagnostic key. Speaks current position and writes detailed coordinates, nearby interactive objects, and a full inventory count to "nativetrainer.log" in your Documents folder.

**
Developers:
- Built by rakanAlmoutiri.
- ScriptHookRDR2 SDK by Alexander Blade.
- Speech via Tolk.