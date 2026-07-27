# OldGameVM

This is a MODIFIED version. You are not receiving the original software.

## What this is

OldGameVM is a non-commercial fork of JA2 Stracciatella, which is
itself built on the Jagged Alliance 2 source code released by
Strategy First Inc. All original copyright notices are retained.

## License

Distributed under the same terms as the original: see
"SFI Source Code license agreement.txt" in this directory, included
verbatim. Non-commercial use only. No broader rights are granted.

## Changes made in this fork

Changed by the OldGameVM maintainer, July 2026:

- Full-size strategic map screen layout for 1366x768 and wider
- Support for loading Wildfire-edition 16-bit map artwork
- Game edition detection and resource variant resolution
- Import Game Data from a retail installer, from the launcher
- Windows-1252 punctuation mapping in text rendering
- HiDPI fullscreen fix on macOS
- Corrected Wildfire inventory artwork mapping
- Procedural ground cross-section art for map level slots
- Renamed to OldGameVM; launcher offers 1366x768 by default

## Modified files

Relative to upstream JA2 Stracciatella, these files were changed
in July 2026:

- .claude/settings.local.json
- assets/externalized/translation_tables/translation-table-eng.json
- assets/externalized/translation_tables/translation-table-fr.json
- assets/externalized/translation_tables/translation-table-rus-gold.json
- assets/externalized/translation_tables/translation-table-rus.json
- src/externalized/DefaultContentManager.cc
- src/externalized/ItemModel.cc
- src/externalized/ItemModel.h
- src/externalized/ResourceVariants.cc
- src/externalized/ResourceVariants.h
- src/externalized/editiondetector/EditionDetector.cc
- src/externalized/editiondetector/EditionDetector.h
- src/externalized/editiondetector/EditionDetector_manualtest.cc
- src/externalized/strings/EncryptedString.cc
- src/game/Intro.cc
- src/game/Laptop/IMP_Confirm.cc
- src/game/MainMenuScreen.cc
- src/game/SaveLoadGame.cc
- src/game/Strategic/MapScreen.cc
- src/game/Strategic/MapScreen.h
- src/game/Strategic/Map_Screen_Interface.cc
- src/game/Strategic/Map_Screen_Interface.h
- src/game/Strategic/Map_Screen_Interface_Border.cc
- src/game/Strategic/Map_Screen_Interface_Border.h
- src/game/Strategic/Map_Screen_Interface_Bottom.cc
- src/game/Strategic/Map_Screen_Interface_Bottom.h
- src/game/Strategic/Map_Screen_Interface_Map.cc
- src/game/Strategic/Map_Screen_Interface_Map.h
- src/game/Tactical/Interface.cc
- src/game/Tactical/Interface_Panels.cc
- src/game/Tactical/Interface_Panels.h
- src/game/UILayout.cc
- src/game/UILayout.h
- src/game/Utils/Message.cc
- src/launcher/Launcher.cc
- src/launcher/Launcher.h
- src/launcher/StracciatellaLauncher.cc
- src/launcher/StracciatellaLauncher.fl
- src/launcher/StracciatellaLauncher.h
- src/sgp/VObject_Blitters.cc
- src/sgp/VObject_Blitters.h
- src/sgp/VSurface.cc
- src/sgp/Video.cc

