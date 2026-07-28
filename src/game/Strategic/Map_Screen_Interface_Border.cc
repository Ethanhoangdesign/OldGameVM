/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#include "Map_Screen_Interface_Border.h"
#include "Assignments.h"
#include "Campaign_Types.h"
#include "Debug.h"
#include "Directories.h"
#include "Interface.h"
#include "Map_Screen_Helicopter.h"
#include "Map_Screen_Interface.h"
#include "Map_Screen_Interface_Map.h"
#include "Map_Screen_Interface_Map_Inventory.h"
#include "MapScreen.h"
#include "MouseSystem.h"
#include "Object_Cache.h"
#include "SysUtil.h"
#include "Text.h"
#include "UILayout.h"
#include "Video.h"
#include "HImage.h"
#include "Line.h"
#include "VObject.h"
#include "VSurface.h"
#include <string_theory/string>

struct BUTTON_PICS;

#define MAP_BORDER_FILE INTERFACEDIR "/mbs.sti"

/* Full-size layout: the toggle row floats right above the bottom band on
 * the right, like the reference layout. */
/* Full-size layout: the Wildfire toggle-button art is 50x44 (frames 1-5, 8 of
 * map_border_buttons.sti), so space the buttons on a 64px pitch (50 + 14 gap)
 * and centre them in the 56px strip band (y 592..648). */
#define BTN_ROW_Y       (g_ui.isMapFullSize() ? (g_ui.get_MAP_VIEW_START_Y() + 601) : STD_SCREEN_Y + 323)
/* BTNBLOCK: the six map toggle buttons move off the wooden strip under
 * the map and into the free band to the right of the 763px bottom panel art,
 * laid out as three columns by two rows of 50x44 buttons. When the screen is
 * too narrow for that band the old single row under the map is kept. */
#define BTN_BLOCK_FITS  (g_ui.isMapFullSize() && \
	(INT32)(g_ui.get_MAP_BOTTOM_BASE_X() + 763) <= (INT32)SCREEN_WIDTH)
/* BTNNUDGE: chinh tay cum sau nut bieu tuong.
 *   X: am = sang trai, duong = sang phai
 *   Y: am = len tren,  duong = xuong duoi
 * Sua so o hai dong duoi day roi dung lai la thay ngay. */
#define BTN_NUDGE_X (-5)
#define BTN_NUDGE_Y (2)
#define BTN_BLOCK_X(c)  (g_ui.get_MAP_BOTTOM_BASE_X() + 13 + BTN_NUDGE_X + 58 * (c))  /* BTNSLOTS */
#define BTN_BLOCK_Y(r)  (g_ui.get_MAP_BOTTOM_BASE_Y() + 368 + BTN_NUDGE_Y + 57 * (r))  /* BTNSLOTS */
#define BTN_TOWN_X      (BTN_BLOCK_FITS ? BTN_BLOCK_X(0) : \
	(g_ui.isMapFullSize() ? (g_ui.get_MAP_VIEW_START_X() + 0) : STD_SCREEN_X + 299))
#define BTN_TOWN_Y      (BTN_BLOCK_FITS ? BTN_BLOCK_Y(0) : BTN_ROW_Y)
#define BTN_MINE_X      (BTN_BLOCK_FITS ? BTN_BLOCK_X(1) : \
	(g_ui.isMapFullSize() ? (g_ui.get_MAP_VIEW_START_X() + 50) : STD_SCREEN_X + 342))
#define BTN_MINE_Y      (BTN_BLOCK_FITS ? BTN_BLOCK_Y(0) : BTN_ROW_Y)
#define BTN_TEAMS_X      (BTN_BLOCK_FITS ? BTN_BLOCK_X(2) : \
	(g_ui.isMapFullSize() ? (g_ui.get_MAP_VIEW_START_X() + 100) : STD_SCREEN_X + 385))
#define BTN_TEAMS_Y      (BTN_BLOCK_FITS ? BTN_BLOCK_Y(0) : BTN_ROW_Y)
#define BTN_MILITIA_X      (BTN_BLOCK_FITS ? BTN_BLOCK_X(0) : \
	(g_ui.isMapFullSize() ? (g_ui.get_MAP_VIEW_START_X() + 150) : STD_SCREEN_X + 428))
#define BTN_MILITIA_Y      (BTN_BLOCK_FITS ? BTN_BLOCK_Y(1) : BTN_ROW_Y)
#define BTN_AIR_X      (BTN_BLOCK_FITS ? BTN_BLOCK_X(1) : \
	(g_ui.isMapFullSize() ? (g_ui.get_MAP_VIEW_START_X() + 200) : STD_SCREEN_X + 471))
#define BTN_AIR_Y      (BTN_BLOCK_FITS ? BTN_BLOCK_Y(1) : BTN_ROW_Y)
#define BTN_ITEM_X      (BTN_BLOCK_FITS ? BTN_BLOCK_X(2) : \
	(g_ui.isMapFullSize() ? (g_ui.get_MAP_VIEW_START_X() + 250) : STD_SCREEN_X + 514))
#define BTN_ITEM_Y      (BTN_BLOCK_FITS ? BTN_BLOCK_Y(1) : BTN_ROW_Y)

/* Full-size layout: the Wildfire level-marker art (greenarr.sti) is 151x23,
 * so the selector rows match its width and sit at the right end of the strip
 * with room to spare (861 + 151 = 1012 < 1024). */
/* LEVELSLOT-V: the map art is 714px wide, so the wooden margin beside it is
 * only 24px at 1024x768 but grows with the screen. When the right margin can
 * hold the selector we stack the four levels vertically (23px rows, like
 * Wildfire's 151x23 GreenArr art); otherwise we fall back to four 37x31
 * cells side by side inside the wooden strip under the map. */
/* ANCHOR169: the map controls hang off the bottom edge of the map art
 * (art bottom = MAP_VIEW_START_Y + 613), so they follow the map when it is
 * centred vertically on a 16:9 screen instead of drifting to the very
 * bottom of the display. */
#define MAP_LEVEL_RIGHT_MARGIN (SCREEN_WIDTH - (g_ui.get_MAP_VIEW_START_X() + 715))
#define MAP_LEVEL_VERTICAL     (g_ui.isMapFullSize() && MAP_LEVEL_RIGHT_MARGIN >= 48)
#define ONMAP_MAP_LEVEL_MARKER_X    (!g_ui.isMapFullSize() ? (STD_SCREEN_X + 565) : \
	(MAP_LEVEL_VERTICAL ? (g_ui.get_MAP_VIEW_START_X() + 719) \
	                    : (g_ui.get_MAP_VIEW_START_X() + 672 - 148)))
#define ONMAP_MAP_LEVEL_MARKER_Y    (!g_ui.isMapFullSize() ? (STD_SCREEN_Y + 323) : \
	(MAP_LEVEL_VERTICAL ? (g_ui.get_MAP_VIEW_START_Y() + 521) : (g_ui.get_MAP_VIEW_START_Y() + 613)))
#define MAP_LEVEL_MARKER_DELTA   8
#define MAP_LEVEL_MARKER_WIDTH  (g_ui.isMapFullSize() ? 151 : 55)
/* LEVELSLOT-H: at full size the four level slots sit side by side inside the
 * wooden strip (37x31 each) because a vertical 4x23 stack does not fit the
 * 35px band left under the map. Vanilla keeps its vertical 55x8 rows. */
#define ONMAP_MAP_LEVEL_SLOT_W    (!g_ui.isMapFullSize() ? MAP_LEVEL_MARKER_WIDTH : \
	(MAP_LEVEL_VERTICAL ? (MAP_LEVEL_RIGHT_MARGIN - 8 > 151 ? 151 : MAP_LEVEL_RIGHT_MARGIN - 8) : 37))
#define ONMAP_MAP_LEVEL_SLOT_H    (!g_ui.isMapFullSize() ? MAP_LEVEL_MARKER_DELTA : \
	(MAP_LEVEL_VERTICAL ? 23 : 31))
#define MAP_LEVEL_SLOT_X(i) (MAP_LEVEL_MARKER_X + \
	((g_ui.isMapFullSize() && !LEVEL_BAR_FITS && !MAP_LEVEL_VERTICAL) ? MAP_LEVEL_SLOT_W * (i) : 0))
#define MAP_LEVEL_SLOT_Y(i) (MAP_LEVEL_MARKER_Y + \
	(!g_ui.isMapFullSize() ? MAP_LEVEL_MARKER_DELTA * (i) : \
	 ((LEVEL_BAR_FITS || MAP_LEVEL_VERTICAL) ? MAP_LEVEL_SLOT_H * (i) : 0)))



#define MAP_BORDER_X (STD_SCREEN_X + 261)
#define MAP_BORDER_Y (STD_SCREEN_Y + 0)


// mouse levels
/* LEVELBAR: when the band to the right of the 763px bottom panel art is
 * wide enough to hold the six-button block AND the level selector, the
 * selector moves down there next to the buttons; otherwise it stays in the
 * wooden margin beside the map. Four rows of 22px fit the 121px panel. */
#define LEVEL_BAR_FITS  (BTN_BLOCK_FITS && \
	(INT32)(g_ui.get_MAP_BOTTOM_BASE_X() + 763) <= (INT32)SCREEN_WIDTH)
#define LEVEL_BAR_X     (g_ui.get_MAP_BOTTOM_BASE_X() + 200)
#define LEVEL_BAR_Y     (g_ui.get_MAP_BOTTOM_BASE_Y() + 374)
#define LEVEL_BAR_W     (149)
#define MAP_LEVEL_MARKER_X  (LEVEL_BAR_FITS ? (UINT16)LEVEL_BAR_X : ONMAP_MAP_LEVEL_MARKER_X)
#define MAP_LEVEL_MARKER_Y  (LEVEL_BAR_FITS ? (UINT16)LEVEL_BAR_Y : ONMAP_MAP_LEVEL_MARKER_Y)
#define MAP_LEVEL_SLOT_W    (LEVEL_BAR_FITS ? (INT32)LEVEL_BAR_W : ONMAP_MAP_LEVEL_SLOT_W)
#define MAP_LEVEL_SLOT_H    (LEVEL_BAR_FITS ? 23 : ONMAP_MAP_LEVEL_SLOT_H)

static MOUSE_REGION LevelMouseRegions[4];

// graphics
namespace {
// the white rectangle highlighting the current level on the map border
cache_key_t const guiLEVELMARKER{ INTERFACEDIR "/greenarr.sti" };
cache_key_t const guiMapBorder{ MAP_BORDER_FILE };
 // the map border eta pop up
cache_key_t const guiMapBorderEtaPopUp{ INTERFACEDIR "/eta_pop_up.sti" };
}

// scroll direction
INT32 giScrollButtonState = -1;

// flags
BOOLEAN fShowTownFlag = FALSE;
BOOLEAN fShowMineFlag = FALSE;
BOOLEAN fShowTeamFlag = FALSE;
BOOLEAN fShowMilitia = FALSE;
BOOLEAN fShowAircraftFlag = FALSE;
BOOLEAN fShowItemsFlag = FALSE;


// buttons & button images
GUIButtonRef giMapBorderButtons[6];
static BUTTON_PICS* giMapBorderButtonsImage[6];

extern void CancelMapUIMessage( void );


void DeleteMapBorderGraphics( void )
{
	// procedure will delete graphics loaded for map border
	RemoveVObject(guiLEVELMARKER);
	RemoveVObject(guiMapBorder);
	RemoveVObject(guiMapBorderEtaPopUp);
}


static void DisplayCurrentLevelMarker(void);


void RenderMapBorder( void )
{
	if( fShowMapInventoryPool )
	{
		// render background, then leave
		BlitInventoryPoolGraphic( );
		return;
	}

	BltVideoObject(guiSAVEBUFFER, guiMapBorder, 0, MAP_BORDER_X, MAP_BORDER_Y);

	// show the level marker
	DisplayCurrentLevelMarker( );
}

void RenderMapBorderEtaPopUp( void )
{
	if( fShowMapInventoryPool )
	{
		return;
	}

	if (fPlotForHelicopter)
	{
		DisplayDistancesForHelicopter( );
		return;
	}

	BltVideoObject(FRAME_BUFFER, guiMapBorderEtaPopUp, 0, MAP_BORDER_X + 215, STD_SCREEN_Y + 291);

	InvalidateRegion( MAP_BORDER_X + 215, (STD_SCREEN_Y + 291), MAP_BORDER_X + 215 + 100 , (STD_SCREEN_Y + 310));
}


/* LEVELMARK-FS: at full size the 151x23 GreenArr art does not fit the 4x8 px
 * level rows, so draw a thin highlight frame instead of blitting the art. */
static void DrawCurrentLevelMarker(void)
{
	INT32 const i = iCurrentMapSectorZ;
	if (g_ui.isMapFullSize())
	{
		SGPVSurface::Lock l(guiSAVEBUFFER);
		UINT16 const white = Get16BPPColor(FROMRGB(220, 226, 200));
		RectangleDraw(TRUE, MAP_LEVEL_SLOT_X(i) + 1, MAP_LEVEL_SLOT_Y(i) + 1,
			MAP_LEVEL_SLOT_X(i) + MAP_LEVEL_SLOT_W - 1, MAP_LEVEL_SLOT_Y(i) + MAP_LEVEL_SLOT_H - 1,
			white, l.Buffer<UINT16>());
	}
	else
	{
		BltVideoObject(guiSAVEBUFFER, guiLEVELMARKER, 0, MAP_LEVEL_MARKER_X, MAP_LEVEL_MARKER_Y + MAP_LEVEL_MARKER_DELTA * i);
	}
}


/* Full-size layout: RenderMapBorder() (which normally draws the level strip
 * art and marker) is skipped, so draw a small self-made level selector at the
 * end of the toggle strip: four sunken rows plus the white marker. The mouse
 * regions already sit at MAP_LEVEL_MARKER_X/Y. */
/* LEVELSTRATA: cheap hash used as noise, so the rock has grain. */
static UINT32 StrataNoise(INT32 const x, INT32 const y, INT32 const salt)
{
	UINT32 h = (UINT32)(x * 374761393) + (UINT32)(y * 668265263) + (UINT32)(salt * 1442695041);
	h = (h ^ (h >> 13)) * 1274126177u;
	return h ^ (h >> 16);
}


/* LEVELSTRATA: one slot of the level strip, drawn as a slice of ground.
 * Slot 0 is the surface (sky over grass); slots 1..3 go progressively darker
 * and rockier, with the odd fleck of ore. No artwork required. */
static void DrawLevelSlotStrata(INT32 const level)
{
	INT32 const x0 = MAP_LEVEL_SLOT_X(level) + 1;
	INT32 const y0 = MAP_LEVEL_SLOT_Y(level) + 1;
	INT32 const w  = MAP_LEVEL_SLOT_W - 2;
	INT32 const h  = MAP_LEVEL_SLOT_H - 2;
	if (w <= 0 || h <= 0) return;

	SGPVSurface::Lock l(guiSAVEBUFFER);
	UINT16* const buf    = l.Buffer<UINT16>();
	UINT32  const stride = l.Pitch() / 2;

	for (INT32 py = 0; py < h; ++py)
	{
		for (INT32 px = 0; px < w; ++px)
		{
			UINT32 const n     = StrataNoise(px, py, level);
			INT32  const grain = (INT32)(n % 25) - 12;
			INT32 r, g, b;

			if (level == 0)
			{
				INT32 const horizon = (h * 2) / 5;
				if (py < horizon)
				{
					/* sky, brightening towards the horizon */
					INT32 const t = (py * 70) / (horizon > 0 ? horizon : 1);
					r = 84 + t;
					g = 116 + t;
					b = 158 + t / 3;
				}
				else
				{
					/* grass, darkening with depth */
					INT32 const d = py - horizon;
					r = 58 - d + grain;
					g = 104 - d * 2 + grain;
					b = 42 - d + grain;
				}
			}
			else
			{
				/* rock: each level down loses brightness */
				INT32 const base = 78 - (level - 1) * 24;
				INT32 const seam = ((py + (INT32)(StrataNoise(px / 9, level, 7) % 4)) % 6 == 0) ? 12 : 0;
				r = base + 16 + grain + seam;
				g = base + 7  + grain + seam;
				b = base - 9  + grain + seam;
				if (n % 233 == 0)
				{
					/* fleck of ore */
					r += 70;
					g += 52;
					b += 12;
				}
			}

			if (r < 0)   r = 0;
			if (g < 0)   g = 0;
			if (b < 0)   b = 0;
			if (r > 255) r = 255;
			if (g > 255) g = 255;
			if (b > 255) b = 255;

			buf[(UINT32)(y0 + py) * stride + (UINT32)(x0 + px)] =
				Get16BPPColor(FROMRGB(r, g, b));
		}
	}
}


void RenderMapLevelSelectorFullSize(void)
{
	/* LEVELSTRATA: paint the four slots as a cross section of the ground,
	 * from sky and grass down to deep rock, so the strip reads as depth. */
	for (INT32 i = 0; i < 4; ++i)
	{
		DrawLevelSlotStrata(i);
	}
	{
		SGPVSurface::Lock l(guiSAVEBUFFER);
		UINT16 const dark  = Get16BPPColor(FROMRGB(20, 24, 18));
		UINT16 const light = Get16BPPColor(FROMRGB(96, 104, 84));
		for (INT32 i = 0; i < 4; ++i)
		{
			RectangleDraw(TRUE, MAP_LEVEL_SLOT_X(i), MAP_LEVEL_SLOT_Y(i),
				MAP_LEVEL_SLOT_X(i) + MAP_LEVEL_SLOT_W, MAP_LEVEL_SLOT_Y(i) + MAP_LEVEL_SLOT_H,
				dark, l.Buffer<UINT16>());
		}
		RectangleDraw(TRUE, MAP_LEVEL_MARKER_X - 2, MAP_LEVEL_MARKER_Y - 2,
			MAP_LEVEL_SLOT_X(3) + MAP_LEVEL_SLOT_W + 2, MAP_LEVEL_SLOT_Y(3) + MAP_LEVEL_SLOT_H + 2,
			light, l.Buffer<UINT16>());
	}
	DrawCurrentLevelMarker();
}


static void MakeButton(UINT idx, UINT gfx, INT16 x, INT16 y, GUI_CALLBACK click, const ST::string& help)
{
	BUTTON_PICS* const img = LoadButtonImage(INTERFACEDIR "/map_border_buttons.sti", gfx, gfx + 9);
	giMapBorderButtonsImage[idx] = img;
	GUIButtonRef const btn = QuickCreateButtonNoMove(img, x, y, MSYS_PRIORITY_HIGH, click);
	giMapBorderButtons[idx] = btn;
	btn->SetFastHelpText(help);
	btn->SetCursor(MSYS_NO_CURSOR);
}


static void BtnAircraftCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnItemCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnMilitiaCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnMineCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnTeamCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnTownCallback(GUI_BUTTON* btn, UINT32 reason);
static void InitializeMapBorderButtonStates(void);


void CreateButtonsForMapBorder(void)
{
	// will create the buttons needed for the map screen border region

	MakeButton(MAP_BORDER_TOWN_BTN,     5, BTN_TOWN_X, BTN_TOWN_Y,    BtnTownCallback,     pMapScreenBorderButtonHelpText[0]); // towns
	MakeButton(MAP_BORDER_MINE_BTN,     4, BTN_MINE_X, BTN_MINE_Y,    BtnMineCallback,     pMapScreenBorderButtonHelpText[1]); // mines
	MakeButton(MAP_BORDER_TEAMS_BTN,    3, BTN_TEAMS_X, BTN_TEAMS_Y,   BtnTeamCallback,     pMapScreenBorderButtonHelpText[2]); // people
	MakeButton(MAP_BORDER_MILITIA_BTN,  8, BTN_MILITIA_X, BTN_MILITIA_Y, BtnMilitiaCallback,  pMapScreenBorderButtonHelpText[5]); // militia
	MakeButton(MAP_BORDER_AIRSPACE_BTN, 2, BTN_AIR_X, BTN_AIR_Y,     BtnAircraftCallback, pMapScreenBorderButtonHelpText[3]); // airspace
	MakeButton(MAP_BORDER_ITEM_BTN,     1, BTN_ITEM_X, BTN_ITEM_Y,    BtnItemCallback,     pMapScreenBorderButtonHelpText[4]); // items

	InitializeMapBorderButtonStates( );
}


void DeleteMapBorderButtons( void )
{
	static_assert(std::size(giMapBorderButtonsImage) == std::size(giMapBorderButtons));

	for (UINT8 ubCnt = 0; ubCnt < std::size(giMapBorderButtons); ++ubCnt)
	{
		RemoveButton(giMapBorderButtons[ubCnt]);
		UnloadButtonImage(giMapBorderButtonsImage[ubCnt]);
	}
}


// callbacks

static void CommonBtnCallbackBtnDownChecks(void);


static void BtnMilitiaCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
		ToggleShowMilitiaMode( );
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnTeamCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
		ToggleShowTeamsMode();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnTownCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
		ToggleShowTownsMode();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnMineCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
		ToggleShowMinesMode();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnAircraftCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();

		ToggleAirspaceMode();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}


static void BtnItemCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_DWN )
	{
		CommonBtnCallbackBtnDownChecks();

		ToggleItemsFilter();
	}
	else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
	{
		CommonBtnCallbackBtnDownChecks();
	}
}

static void MapBorderButtonOff(UINT8 ubBorderButtonIndex);
static void MapBorderButtonOn(UINT8 ubBorderButtonIndex);


void ToggleShowTownsMode( void )
{
	if (fShowTownFlag)
	{
		fShowTownFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
	}
	else
	{
		fShowTownFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_TOWN_BTN );

		if (fShowMineFlag)
		{
			fShowMineFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_MINE_BTN );
		}

		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}

		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}
	}

	fMapPanelDirty = TRUE;
}


void ToggleShowMinesMode( void )
{
	if (fShowMineFlag)
	{
		fShowMineFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_MINE_BTN );
	}
	else
	{
		fShowMineFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_MINE_BTN );

		if (fShowTownFlag)
		{
			fShowTownFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
		}

		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}

		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}
	}

	fMapPanelDirty = TRUE;
}


static bool DoesPlayerHaveAnyMilitia();


void ToggleShowMilitiaMode( void )
{
	if (fShowMilitia)
	{
		fShowMilitia = FALSE;
		MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
	}
	else
	{
		// toggle militia ON
		fShowMilitia = TRUE;
		MapBorderButtonOn( MAP_BORDER_MILITIA_BTN );

		// if Team is ON, turn it OFF
		if (fShowTeamFlag)
		{
			fShowTeamFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );
		}

/*
		// if Airspace is ON, turn it OFF
		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}
*/

		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}


		// check if player has any militia
		if (!DoesPlayerHaveAnyMilitia())
		{
			ST::string pwString;

			// no - so put up a message explaining how it works

			// if he's already training some
			if( IsAnyOneOnPlayersTeamOnThisAssignment( TRAIN_TOWN ) )
			{
				// say they'll show up when training is completed
				pwString = pMapErrorString[ 28 ];
			}
			else
			{
				// say you need to train them first
				pwString = zMarksMapScreenText[ 1 ];
			}

			BeginMapUIMessage(0, pwString);
		}
	}

	fMapPanelDirty = TRUE;
}


void ToggleShowTeamsMode( void )
{
	if (fShowTeamFlag)
	{
		// turn show teams OFF
		fShowTeamFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
	else
	{	// turn show teams ON
		TurnOnShowTeamsMode();
	}
}


void ToggleAirspaceMode( void )
{
	if (fShowAircraftFlag)
	{
		// turn airspace OFF
		fShowAircraftFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );

		if (fPlotForHelicopter)
		{
			AbortMovementPlottingMode( );
		}

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
	else
	{	// turn airspace ON
		TurnOnAirSpaceMode();
	}
}


static void TurnOnItemFilterMode(void);


void ToggleItemsFilter( void )
{
	if (fShowItemsFlag)
	{
		// turn items OFF
		fShowItemsFlag = FALSE;
		MapBorderButtonOff( MAP_BORDER_ITEM_BTN );

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
	else
	{
		// turn items ON
		TurnOnItemFilterMode();
	}
}

static void DisplayCurrentLevelMarker(void)
{
	// display the current level marker on the map border
/*
	if( fDisabledMapBorder )
	{
		return;
	}
*/

	// it's actually a white rectangle, not a green arrow!
	DrawCurrentLevelMarker();
}


static void LevelMarkerBtnCallback(MOUSE_REGION* pRegion, UINT32 iReason);


void CreateMouseRegionsForLevelMarkers(void)
{
	for (UINT sCounter = 0; sCounter < 4 ; ++sCounter)
	{
		MOUSE_REGION* const r = &LevelMouseRegions[sCounter];
		const UINT16        x = MAP_LEVEL_SLOT_X(sCounter);
		const UINT16        y = MAP_LEVEL_SLOT_Y(sCounter);
		const UINT16        w = MAP_LEVEL_SLOT_W;
		const UINT16        h = MAP_LEVEL_SLOT_H;
		MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, LevelMarkerBtnCallback);

		MSYS_SetRegionUserData(r, 0, sCounter);

		ST::string sString = ST::format("{} {}", zMarksMapScreenText[0], sCounter + 1);
		r->SetFastHelpText(sString);
	}
}


void DeleteMouseRegionsForLevelMarkers()
{
	FOR_EACH(MOUSE_REGION, i, LevelMouseRegions) MSYS_RemoveRegion(&*i);
}


static void LevelMarkerBtnCallback(MOUSE_REGION* pRegion, UINT32 iReason)
{
	// btn callback handler for assignment screen mask region
	INT32 iCounter = 0;

	iCounter = MSYS_GetRegionUserData( pRegion, 0 );

	if( ( iReason & MSYS_CALLBACK_REASON_POINTER_UP ) )
	{
		JumpToLevel( iCounter );
	}
}


/*
void DisableMapBorderRegion( void )
{
	// will shutdown map border region

	if( fDisabledMapBorder )
	{
		// checked, failed
		return;
	}

	// get rid of graphics and mouse regions
	DeleteMapBorderGraphics( );


	fDisabledMapBorder = TRUE;
}

void EnableMapBorderRegion( void )
{
	// will re-enable mapborder region

	if (!fDisabledMapBorder)
	{
		// checked, failed
		return;
	}

	// re load graphics and buttons
	LoadMapBorderGraphics( );

	fDisabledMapBorder = FALSE;

}
*/


void TurnOnShowTeamsMode( void )
{
	// if mode already on, leave, else set and redraw

	if (!fShowTeamFlag)
	{
		fShowTeamFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_TEAMS_BTN );

		if (fShowMilitia)
		{
			fShowMilitia = FALSE;
			MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
		}

/*
		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}
*/

		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
}



void TurnOnAirSpaceMode( void )
{
	// if mode already on, leave, else set and redraw

	if (!fShowAircraftFlag)
	{
		fShowAircraftFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_AIRSPACE_BTN );


		// Turn off towns & mines (mostly because town/mine names overlap SAM site names)
		if (fShowTownFlag)
		{
			fShowTownFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
		}

		if (fShowMineFlag)
		{
			fShowMineFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_MINE_BTN );
		}

/*
		// Turn off teams and militia
		if (fShowTeamFlag)
		{
			fShowTeamFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );
		}

		if (fShowMilitia)
		{
			fShowMilitia = FALSE;
			MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
		}
*/

		// Turn off items
		if (fShowItemsFlag)
		{
			fShowItemsFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
		}

		if ( bSelectedDestChar != -1 )
		{
			AbortMovementPlottingMode( );
		}


		// if showing underground
		if ( iCurrentMapSectorZ != 0 )
		{
			// switch to the surface
			JumpToLevel( 0 );
		}

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
}


static void TurnOnItemFilterMode(void)
{
	// if mode already on, leave, else set and redraw

	if (!fShowItemsFlag)
	{
		fShowItemsFlag = TRUE;
		MapBorderButtonOn( MAP_BORDER_ITEM_BTN );


		// Turn off towns, mines, teams, militia & airspace if any are on
		if (fShowTownFlag)
		{
			fShowTownFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
		}

		if (fShowMineFlag)
		{
			fShowMineFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_MINE_BTN );
		}

		if (fShowTeamFlag)
		{
			fShowTeamFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );
		}

		if (fShowMilitia)
		{
			fShowMilitia = FALSE;
			MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
		}

		if (fShowAircraftFlag)
		{
			fShowAircraftFlag = FALSE;
			MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
		}

		if (bSelectedDestChar != -1 || fPlotForHelicopter)
		{
			AbortMovementPlottingMode( );
		}

		// dirty regions
		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;
	}
}


// set button states to match map flags
static void InitializeMapBorderButtonStates(void)
{
	if( fShowItemsFlag )
	{
		MapBorderButtonOn( MAP_BORDER_ITEM_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_ITEM_BTN );
	}

	if( fShowTownFlag )
	{
		MapBorderButtonOn( MAP_BORDER_TOWN_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_TOWN_BTN );
	}

	if( fShowMineFlag )
	{
		MapBorderButtonOn( MAP_BORDER_MINE_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_MINE_BTN );
	}

	if( fShowTeamFlag )
	{
		MapBorderButtonOn( MAP_BORDER_TEAMS_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );
	}

	if( fShowAircraftFlag )
	{
		MapBorderButtonOn( MAP_BORDER_AIRSPACE_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
	}

	if( fShowMilitia )
	{
		MapBorderButtonOn( MAP_BORDER_MILITIA_BTN );
	}
	else
	{
		MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
	}
}


static bool DoesPlayerHaveAnyMilitia()
{
	FOR_EACH(SECTORINFO const, i, SectorInfo)
	{
		UINT8 const (&n)[MAX_MILITIA_LEVELS] = i->ubNumberOfCivsAtLevel;
		if (n[GREEN_MILITIA] + n[REGULAR_MILITIA] + n[ELITE_MILITIA] != 0) return true;
	}
	return false;
}


static void CommonBtnCallbackBtnDownChecks(void)
{
	// any click cancels MAP UI messages, unless we're in confirm map move mode
	if (g_ui_message_overlay != NULL && !gfInConfirmMapMoveMode)
	{
		CancelMapUIMessage( );
	}
}



void InitMapScreenFlags( void )
{
	fShowTownFlag = TRUE;
	fShowMineFlag = FALSE;

	fShowTeamFlag = TRUE;
	fShowMilitia = FALSE;

	fShowAircraftFlag = FALSE;
	fShowItemsFlag = FALSE;
}


static void MapBorderButtonOff(UINT8 ubBorderButtonIndex)
{
	Assert( ubBorderButtonIndex < 6 );

	if( fShowMapInventoryPool )
	{
		return;
	}

	// if button doesn't exist, return
	GUIButtonRef const b = giMapBorderButtons[ubBorderButtonIndex];
	if (b) b->uiFlags &= ~BUTTON_CLICKED_ON;
}


static void MapBorderButtonOn(UINT8 ubBorderButtonIndex)
{
	Assert( ubBorderButtonIndex < 6 );

	if( fShowMapInventoryPool )
	{
		return;
	}

	GUIButtonRef const b = giMapBorderButtons[ubBorderButtonIndex];
	if (b) b->uiFlags |= BUTTON_CLICKED_ON;
}
