RED DEAD REDEMPTION 2 - ACCESSIBILITY MOD (Rdr access)

Welcome to the Red Dead Redemption 2 Accessibility Mod, designed to make
the game playable for blind and low-vision players via Text-to-Speech (TTS),
audio cues, auto-navigation, and combat assistance.

INSTALLATION INSTRUCTIONS
1. Ensure you have ScriptHookRDR2 installed in your game folder (where RDR2.exe is).
2. Copy all files from this folder directly into your Red Dead Redemption 2 game directory:
   - NativeTrainer.asi
   - tolk.dll
   - nvdaControllerClient64.dll
   - SAAPI64.dll
   - tped.wav
   - tprop.wav
   - tvehicle.wav
3. Start your screen reader (NVDA ) before launching the game.
4. Launch Red Dead Redemption 2.

KEY FEATURES
* Text-To-Speech (TTS) Integration:
  - Automatically speaks menu options, stats, status updates, and wanted levels.
  - Supports NVDA(via Tolk) and SAPI (Windows System Speech).
* Auto-Announcements:
  - Cash balance changes (earned/spent cents and dollars).
  - Horse status, speed, and distance from the player.
  - Honor level adjustments.
  - Wanted levels, bounty values, and law enforcement alerts.

* Diagnostic Coordinates Tool (Numpad Decimal Key '.'):
  - Press the decimal key (.) on your numpad to log your exact coordinates and heading.
  - It also scans and writes all nearby interactive objects within 8 meters to the
    log file (nativetraner.log in your Documents folder).

EXPERIMENTAL FEATURES (UNDER ACTIVE TESTING & DEVELOPMENT)
The following features are currently under testing and are being actively
worked on for further improvements:

1. AUTO-NAVIGATION & POI BROWSER (Experimental)
   - Allows browsing and auto-walking to local town points of interest (POIs).
   - Teleporting to preset locations (Stables, Doctor, Gunsmith, Saloons, Hotels, Post Offices)
     places the player outside the doors facing the entrance.
   - Note: Navigation is still in testing and may occasionally clip or navigate incorrectly.

2. LOOT ANNOUNCER (Experimental)
   - Continuous inventory tracking that automatically detects and speaks looted items and
     quantities immediately after the looting animation concludes.
   - Note: Item registration speeds vary, and this feature is being optimized for accuracy.

3. BODYGUARDS / SQUAD SYSTEM (Experimental)
   - Allows spawning custom bodyguards, commanding them to guard an area, follow, or dismiss,
     and provides audio reports on squad status.
   - Note: Guard combat AI and spawning limits are under active refinement.

4. AUTO-AIM SYSTEM (Experimental)
   - Low-vision combat aid that automatically locks onto the nearest hostile target.
   - Note: Target selection and target retention are under active improvement.

TROUBLESHOOTING & FEEDBACK
- If speech does not work, verify that your screen reader is running before starting the game.
- If you encounter teleportation coordinate errors, use the Numpad Decimal (.) diagnostic key
  near the doorway, and share your "nativetraner.log" file (found in your Documents folder)
  with the developer to help correct the coordinate presets.