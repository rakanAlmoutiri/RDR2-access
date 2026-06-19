RED DEAD REDEMPTION 2 - ACCESSIBILITY MOD (Rdr access)
**

IMPORTANT: This is an experimental build currently under active testing and development.
The entire mod is under testing. All individual features (teleportation, loot announcements, auto-aim, and bodyguards) are experimental and subject to bugs, stutters, or crashes. Please report issues and back up your save files.

**
INSTALLATION INSTRUCTIONS
**
1. Ensure you have ScriptHookRDR2 installed in your game folder (where RDR2.exe is).
2. Copy all files from this folder directly into your Red Dead Redemption 2 game directory:
   - RDR access.asi
   - tolk.dll
   - nvdaControllerClient64.dll
   - SAAPI64.dll
3. Start your screen reader (NVDA or JAWS) before launching the game.
4. Launch Red Dead Redemption 2.

**
GENERAL CONTROLS & MENU NAVIGATION
**
* F4: Open or Close the Trainer Menu.
* Numpad 8 / 2: Navigate Up or Down in the menu options.
* Numpad 4 / 6: Navigate Left or Right (or decrease/increase values).
* Numpad 5: Select / Accept menu option.
* Numpad 0: Back / Close submenu.
* Tab Key: Re-announce / Speak the currently selected menu option.

**
NUMPAD MODES SYSTEM
**
Outside of the Trainer Menu, Numpad 5 is used to cycle through 3 accessibility modes:
Global Mode -> Horse Mode -> Bodyguard Mode.
Ensure NumLock is ON.

1. GLOBAL MODE (General status checks)
* Numpad 0: Speak current location (zone, area, and compass heading).
* Numpad 3: Speak current weapon and available ammunition count.
* Numpad 4: Speak player cores (Health, Stamina, DeadEye percentages) and Cash balance.
* Numpad 6: Speak game time (period, hour, minute), Stamina bar, DeadEye bar, and Bounty.
* Numpad 7: Speak wanted status (wanted level, bounty value, and wanted intensity).
* Numpad 9: Speak player honor level.

2. HORSE MODE (Horse status & whistling)
* Numpad 0: Whistle and check horse distance (speaks distance and status: approaching, out of range, etc.).
* Numpad 1: Speak horse breed, bonding level (1-4), and distance.
* Numpad 2: Speak horse movement state and speed (idle, walking, trotting, cantering, galloping).
* Numpad 3: Speak horse stamina bar and health bar.
* Numpad 4: Speak horse cores (Health and Stamina cores).

3. BODYGUARD / SQUAD MODE (Command bodyguards)
* Numpad 0: Cycle squad formation (Column, Line, Wedge).
* Numpad 1: Speak squad status (alive count, combat status, and type breakdown: humans, wolves, cougars, etc.).
* Numpad 2: Regroup (teleport all bodyguards back to you in formation).
* Numpad 6: Toggle auto-defense (bodyguards automatically protect you when attacked).
* Numpad 7: Command guards to follow player.
* Numpad 8: Command guards to hold position (stay still).
* Numpad 9: Command guards to guard this area.
* Note: Bodyguards and animals must be spawned using the main Trainer Menu (F4).

**
SPECIAL FEATURES (UNDER ACTIVE TESTING)
**

1. LOOT ANNOUNCER & INVENTORY TRACKER (Automatic)
* Tracks your inventory in the background and instantly speaks added items and quantities (e.g. "Added 3 Yarrow", "Added 1 Gold Ring").
* Monitors 100% of satchel items (all 13 fish types, fish meats, plants, exotics, animal parts, feathers, and jewelry).
* Warning System: Speaks "No items added. Inventory might be full." if a looting, skinning, or picking animation concludes but no items or cash were gained (meaning inventory was already full or target was empty).
* Numpad Decimal Point Key (.): Diagnostic key. Speaks current position and writes detailed coordinates, nearby interactive objects, and a full inventory count to "nativetrainer.log" in your Documents folder.

2. AUTO-AIM SYSTEM (Combat Aid)
* Toggle "AUTO AIM" inside the Accessibility sub-menu of the trainer (F4).
* Hold the aim button (Right-Click on mouse or Left Trigger on controller) to automatically rotate the camera and lock onto the chest/torso of the nearest hostile NPC or animal within 150 meters. (Requires clear line of sight).

**
TROUBLESHOOTING & CONTACT
**
- If speech does not work, make sure NVDA or JAWS is running before starting the game.
- If keys do not work, make sure NumLock is ON.
- If you face bugs, check the "nativetrainer.log" file in your Documents folder for diagnostic data.

**
CREDITS
**
- ScriptHookRDR2 SDK by Alexander Blade.
- Speech via Tolk.
- Mod Developer: rakanAlmoutiri.
