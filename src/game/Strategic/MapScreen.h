/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#ifndef __MAPSCREEN_H
#define __MAPSCREEN_H

#include "Button_System.h"
#include "MessageBoxScreen.h"
#include "ScreenIDs.h"
#include "JA2Types.h"
#include <string_theory/string>


// Sector name identifiers
enum Towns
{
	BLANK_SECTOR=0,
	OMERTA,
	DRASSEN,
	ALMA,
	GRUMM,
	TIXA,
	CAMBRIA,
	SAN_MONA,
	ESTONI,
	ORTA,
	BALIME,
	MEDUNA,
	CHITZENA,
	NUM_TOWNS
};

#define FIRST_TOWN	OMERTA


extern BOOLEAN fCharacterInfoPanelDirty;
extern BOOLEAN fTeamPanelDirty;
extern BOOLEAN fMapPanelDirty;

extern BOOLEAN fMapInventoryItem;
extern BOOLEAN gfInConfirmMapMoveMode;
extern BOOLEAN gfInChangeArrivalSectorMode;

extern BOOLEAN gfSkyriderEmptyHelpGiven;


void SetInfoChar(SOLDIERTYPE const*);
void EndMapScreen( BOOLEAN fDuringFade );
void ReBuildCharactersList( void );


void HandlePreloadOfMapGraphics(void);
void HandleRemovalOfPreLoadedMapGraphics( void );

void ChangeSelectedMapSector(const SGPSector& sector);

BOOLEAN CanExtendContractForSoldier(const SOLDIERTYPE* s);

void TellPlayerWhyHeCantCompressTime( void );

// the info character
extern INT8 bSelectedInfoChar;

SOLDIERTYPE* GetSelectedInfoChar(void);
void ChangeSelectedInfoChar( INT8 bCharNumber, BOOLEAN fResetSelectedList );

void MAPEndItemPointer(void);

void CopyPathToAllSelectedCharacters(PathSt* pPath);
void CancelPathsOfAllSelectedCharacters(void);

INT32 GetPathTravelTimeDuringPlotting(PathSt* pPath);

void AbortMovementPlottingMode( void );

BOOLEAN CanChangeSleepStatusForSoldier(const SOLDIERTYPE* s);

bool MapCharacterHasAccessibleInventory(SOLDIERTYPE const&);

ST::string GetMapscreenMercAssignmentString(SOLDIERTYPE const& s);
ST::string GetMapscreenMercLocationString(SOLDIERTYPE const& s);
ST::string GetMapscreenMercDestinationString(SOLDIERTYPE const& s);
ST::string GetMapscreenMercDepartureString(SOLDIERTYPE const& s, UINT8* text_colour);

// mapscreen wrapper to init the item description box
void MAPInternalInitItemDescriptionBox(OBJECTTYPE* pObject, UINT8 ubStatusIndex, SOLDIERTYPE* pSoldier);

// rebuild contract box this character
void RebuildContractBoxForMerc(const SOLDIERTYPE* s);

void    InternalMAPBeginItemPointer(SOLDIERTYPE* pSoldier);
BOOLEAN ContinueDialogue(SOLDIERTYPE* pSoldier, BOOLEAN fDone);
void    EndConfirmMapMoveMode(void);
BOOLEAN CanDrawSectorCursor(void);
void    RememberPreviousPathForAllSelectedChars(void);
void    MapScreenDefaultOkBoxCallback(MessageBoxReturnValue);
void    SetUpCursorForStrategicMap(void);
void    DrawFace(void);
void DrawStringRight(const ST::string& str, UINT16 x, UINT16 y, UINT16 w, UINT16 h, SGPFont font);

extern GUIButtonRef giMapInvDoneButton;
extern BOOLEAN      fInMapMode;
extern BOOLEAN      fReDrawFace;
extern BOOLEAN      fShowInventoryFlag;
extern BOOLEAN      fShowDescriptionFlag;
extern GUIButtonRef giMapContractButton;
extern GUIButtonRef giCharInfoButton[2];
extern BOOLEAN      fDrawCharacterList;
extern SGPSector    gsHighlightSector;

// create/destroy inventory button as needed
void CreateDestroyMapInvButton(void);

void     MapScreenInit(void);
ScreenID MapScreenHandle(void);
void     MapScreenShutdown(void);

void LockMapScreenInterface(bool lock);
void MakeDialogueEventEnterMapScreen();

void SetMapCursorItem();

/* The character list and contract box belong to the map screen left column;
 * the ETA clock belongs to the bottom panel (see UILayout). */
#define NAME_X                (g_ui.get_MAP_LEFT_COL_X() + 11)
#define NAME_WIDTH            (g_ui.get_MAP_LEFT_COL_X() + 62 - NAME_X)
#define ASSIGN_X              (g_ui.get_MAP_LEFT_COL_X() + 67)
#define ASSIGN_WIDTH          (g_ui.get_MAP_LEFT_COL_X() + 118 - ASSIGN_X)
#define SLEEP_X               (g_ui.get_MAP_LEFT_COL_X() + 123)
#define SLEEP_WIDTH           (g_ui.get_MAP_LEFT_COL_X() + 142 - SLEEP_X)
#define LOC_X                 (g_ui.get_MAP_LEFT_COL_X() + 147)
#define LOC_WIDTH             (g_ui.get_MAP_LEFT_COL_X() + 179 - LOC_X)
#define DEST_ETA_X            (g_ui.get_MAP_LEFT_COL_X() + 184)
#define DEST_ETA_WIDTH        (g_ui.get_MAP_LEFT_COL_X() + 217 - DEST_ETA_X)
#define TIME_REMAINING_X      (g_ui.get_MAP_LEFT_COL_X() + 222)
#define TIME_REMAINING_WIDTH  (g_ui.get_MAP_LEFT_COL_X() + 250 - TIME_REMAINING_X)
#define CLOCK_Y_START         (g_ui.get_MAP_BOTTOM_BASE_Y() + (g_ui.isMapFullSize() ? 456 : 298))
#define CLOCK_ETA_X           (g_ui.get_MAP_BOTTOM_BASE_X() + 463 - 15 + 6 + 30)
#define CLOCK_HOUR_X_START    (g_ui.get_MAP_BOTTOM_BASE_X() + (g_ui.isMapFullSize() ? 677 : (463 + 25 + 30)))
#define CLOCK_MIN_X_START     (g_ui.get_MAP_BOTTOM_BASE_X() + (g_ui.isMapFullSize() ? 725 : (463 + 45 + 30)))

// contract
#define CONTRACT_X            (g_ui.get_MAP_LEFT_COL_X() + 185)
#define CONTRACT_Y            (g_ui.get_MAP_LEFT_COL_Y() + 50)

// trash can
#define TRASH_CAN_X           (g_ui.get_MAP_LEFT_COL_X() + 176)
#define TRASH_CAN_Y           (211 + PLAYER_INFO_Y)
#define TRASH_CAN_WIDTH       193 - 165
#define TRASH_CAN_HEIGHT      239 - 217

#endif
