RDR2 Accessibility Trainer (Experimental)
=========================================

IMPORTANT: Experimental build – expect bugs and instability.
- All features are experimental. Crashes or odd behavior can happen.
- Use at your own risk. Make backups of your save games.

Install
-------
1) Install ScriptHookRDR2 (runtime) from: http://www.dev-c.com/rdr2/scripthookrdr2/
2) Copy these files into your Red Dead Redemption 2 game folder (where RDR2.exe is):
How it works (Modes and Numpad)
first, we have the normal NativeTrainer, f5 opening it menu and 0 to back or clost it. also backspace key should works too. 

-------------------------------
outside the NativeTrainer, click these keys: 
- Press NumPad 5 to cycle through modes: Global -> Horse -> Wolves -> Bodyguard.
- In each mode, primary shortcuts are NumPad 1..0. Some extras use NumPad . (decimal).

Global Mode
-----------
- N1: Player health and context (on foot/mount/vehicle, speed, cores)
- N2: What you are riding/driving + speed
- N3: Current weapon name
- N4: Location (zone/place)
- N5: Cycle mode
- N6: Money total
- N7: Honor (broken)  wanted intensity 
- N8: Heading (compass)
- N.: Heading (quick)


Horse Mode
----------
- N1: Horse status (health percent + stamina)
- N2: Location (zone)
- N3: Brush horse (clean)
- N4: Feed horse (small heal + stamina)
- N5: Cycle mode

Wolves Mode (companion + pack)
------------------------------
- N1: Wolf status – main wolf health and percent.
- N2: Gather – main wolf and helpers move to you and follow behind.
- N3: Call/Regroup – plays a player call cue and schedules wolf howls. If your desired pack size > current helpers, howls will try to summon more wolves up to the desired size. Everyone regroups and follows.
- N4: Toggle main wolf – spawn if none; dismiss if present (also clears helper pack and resets desired size).
- N5: Cycle mode.
- N6: Attack – command companion to attack the target you are aiming at (or melee target). Falls back to bodyguard if wolf unavailable.
- N7: Increase desired pack – raises desired pack size (by 2 each press) up to the maximum; triggers howls to summon helpers.
- N8: Decrease desired pack – lowers desired size (by 1). If you have more helpers than target, dismisses one immediately.
- N9: Speak pack status – announces how many helper wolves you have (not counting the main wolf).
- N0: Clear pack – dismisses all helper wolves but keeps the main wolf.

Notes about Wolves
------------------
- The main wolf is friendly and follows you. Dismissing the main wolf also clears the pack and resets the desired size.
- Sound cues are conservative to avoid crashes; sometimes audio may not play depending on game state.
- Rapid key presses can cause short cooldowns for stability.

Bodyguard Mode
--------------
- N1: Bodyguard status (health percent)
- N2: Teleport bodyguard to you
- N3: Follow close
- N4: Toggle bodyguard (spawn/dismiss)
- N5: Cycle mode

Troubleshooting
---------------
- No speech? Ensure NVDA (or JAWS) is running and tolk.dll is next to NativeTrainer.asi.
and nvdaControllerClient64.dll is available newr rdr2
- Keys not working? Ensure NumLock is ON. 
- Crashes/odd behavior: experimental build; reduce rapid presses and avoid stacking many actions.

Credits
-------
- ScriptHookRDR2 SDK by Alexander Blade , and it's Native Trainer.
- Speech via Tolk. 
