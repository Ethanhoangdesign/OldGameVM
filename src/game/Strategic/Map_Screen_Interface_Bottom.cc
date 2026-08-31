/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#include "Directories.h"
#include "Logger.h"
#include "Map_Screen_Interface_Border.h"
#include "Font.h"
#include "Interface.h"
#include "HImage.h"
#include "Interface_Panels.h"
#include "Line.h"
#include "Map_Screen_Interface_Bottom.h"
#include "MessageBoxScreen.h"
#include "Object_Cache.h"
#include "Timer_Control.h"
#include "Types.h"
#include "VSurface.h"
#include "MouseSystem.h"
#include "Button_System.h"
#include "Message.h"
#include "MapScreen.h"
#include "StrategicMap.h"
#include "Font_Control.h"
#include "Radar_Screen.h"
#include "Game_Clock.h"
#include "SysUtil.h"
#include "Render_Dirty.h"
#include "Map_Screen_Interface.h"
#include "Map_Screen_Interface_Map.h"
#include "Text.h"
#include "Overhead.h"
#include "PreBattle_Interface.h"
#include "Options_Screen.h"
#include "GameLoop.h"
#include "Tactical_Save.h"
#include "Campaign_Types.h"
#include "Finances.h"
#include "LaptopSave.h"
#include "Interface_Items.h"
#include "WordWrap.h"
#include "Dialogue_Control.h"
#include "Meanwhile.h"
#include "Map_Screen_Helicopter.h"
#include "Map_Screen_Interface_TownMine_Info.h"
#include "Merc_Contract.h"
#include "Map_Screen_Interface_Map_Inventory.h"
#include "Explosion_Control.h"
#include "Creature_Spreading.h"
#include "Soldier_Macros.h"
#include "GameSettings.h"
#include "GameRes.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "SaveLoadScreen.h"
#include "Soldier_Profile.h"
#include "Debug.h"
#include "JAScreens.h"
#include "ScreenIDs.h"
#include "UILayout.h"

#include <string_theory/string>


/* Base offset of the bottom panel: vanilla anchors to STD_SCREEN, the
 * full-size Wildfire layout anchors to (261, 647-359=288). */
#define MAP_BOTTOM_BASE_X (g_ui.get_MAP_BOTTOM_BASE_X())
#define MAP_BOTTOM_BASE_Y (g_ui.get_MAP_BOTTOM_BASE_Y())

#define MAP_BOTTOM_X (MAP_BOTTOM_BASE_X + 0)
#define MAP_BOTTOM_Y (MAP_BOTTOM_BASE_Y + 359)

/* Wide panels reserve the left strip for history; the 763px art begins at
 * MAP_BOTTOM_BASE_X and must not cover it. */
#define MESSAGE_BOX_X (g_ui.isWidePanel() ? 8 : MAP_BOTTOM_BASE_X + 17)
#define MESSAGE_BOX_Y (g_ui.isWidePanel() ? (MapScreenLogTop() + 13) : MAP_BOTTOM_BASE_Y + 377)
#define MESSAGE_BOX_W (g_ui.isMapFullSize() ? (MAP_BOTTOM_BASE_X - 40) : g_ui.isWidescreenLayout() ? (MAP_BOTTOM_BASE_X - 8) : 301)
#define MESSAGE_BOX_H (g_ui.isWidePanel() ? 100 : 86)

/* SCROLLIN: nut van de dua cum cuon vao trong long khung BEVEL3.
 * Vien khung day 13 pixel nen long khung ket thuc som hon truoc.
 * So am dich sang trai, so duong dich sang phai. */
#define SCROLL_NUDGE_X (-6)
#define SCROLL_NUDGE_Y (2)
#define MESSAGE_SCROLL_AREA_START_X (g_ui.isWidescreenLayout() ? (MAP_BOTTOM_BASE_X - (MAP_BOTTOM_BASE_X * 23 / 261)) : g_ui.isWidePanel() ? (MAP_BOTTOM_BASE_X - 23 + SCROLL_NUDGE_X) : MAP_BOTTOM_BASE_X + 330)   /* SCROLLBAY */
#define MESSAGE_SCROLL_AREA_WIDTH    15

#define MESSAGE_SCROLL_AREA_START_Y (g_ui.isWidescreenLayout() ? (27 * SCREEN_HEIGHT / 409) : g_ui.isWidePanel() ? (MapScreenLogTop() + 32 + SCROLL_NUDGE_Y) : MAP_BOTTOM_BASE_Y + 390)
#define MESSAGE_SCROLL_AREA_HEIGHT  (g_ui.isWidescreenLayout() ? ((381 - 27) * SCREEN_HEIGHT / 409) : g_ui.isWidePanel() ?  53 :  59)

#define SLIDER_HEIGHT		11
#define SLIDER_WIDTH		11

#define SLIDER_BAR_RANGE			( MESSAGE_SCROLL_AREA_HEIGHT - SLIDER_HEIGHT )



#define MESSAGE_BTN_SCROLL_TIME 100

// delay for paused flash
#define PAUSE_GAME_TIMER 500

#define MAP_BOTTOM_FONT_COLOR ( 32 * 4 - 9 )

// button enums
enum{
	MAP_SCROLL_MESSAGE_UP =0,
	MAP_SCROLL_MESSAGE_DOWN,
};

enum{
	MAP_TIME_COMPRESS_MORE = 0,
	MAP_TIME_COMPRESS_LESS,
};


/* Top edge of the full-size history-log box, now inside the bottom band
 * (wide, 9 lines) per the reference layout; the left column below the
 * roster stays free for a future taller roster. */
/* LOGFRAME2: dan khung nhat ky that cua Wildfire thay cho mang to den.
 * Tranh goc map_screen_log.sti la 261x409, do duoc:
 *   cot   0..12  mep trai (go noi o cot 10..12)
 *   cot  13..228 long khung, den phang tuyet doi
 *   cot 229..260 tron cum thanh cuon (go, ranh, con truot)
 *   hang   0..13 mep tren
 *   hang 391..408 mep duoi
 * Khung cua ta rong hon tranh nen bon goc va cum cuon giu nguyen,
 * chi lap lai dai giua cho du be ngang. */

static void RenderMapScreenLogFrame(INT32 bx, INT32 by, INT32 bw, INT32 bh)
{
	/* Stretch the original Wildfire log frame to the available left column. */
	auto const source = CreateVideoSurfaceFromObjectFile(INTERFACEDIR "/map_screen_log.sti", 0);
	SGPBox const src{ 0, 0, (UINT16)source->Width(), (UINT16)source->Height() };
	SGPBox const dst{ (UINT16)bx, (UINT16)by, (UINT16)bw, (UINT16)bh };
	BltStretchVideoSurface(guiSAVEBUFFER, source.get(), &src, &dst);
}

static void RenderMapScreenLogFrameFallback(INT32 bx, INT32 by, INT32 bw, INT32 bh)
{
	/* BEVEL3 fallback when the edition does not ship the Wildfire frame asset.
	   Dai do sang lay tu anh ban goc: 2 diem sang ngoai, ranh toi 3 diem,
	   dai xam 4 diem, 3 diem sang trong, roi vao long khung. */
	static UINT8 const prof[13][3] =
	{
		{  48,  52,  42 }, {  96, 104,  84 }, {  64,  70,  56 },
		{  12,  16,  10 }, {   8,  12,   8 }, {  12,  16,  10 },
		{  28,  32,  24 }, {  28,  32,  24 }, {  28,  32,  24 },
		{  24,  28,  20 },
		{  76,  84,  68 }, {  88,  96,  78 }, {  64,  70,  56 }
	};
	INT32 const n = 13;

	UINT16 const fill = Get16BPPColor(FROMRGB(0, 12, 0));
	ColorFillVideoSurfaceArea(guiSAVEBUFFER, bx, by, bx + bw, by + bh, fill);

	if (bw < 2 * n + 8 || bh < 2 * n + 8) return;

	for (INT32 i = 0; i < n; ++i)
	{
		INT32 const j = n - 1 - i;
		UINT16 const ca = Get16BPPColor(FROMRGB(prof[i][0], prof[i][1], prof[i][2]));
		UINT16 const cb = Get16BPPColor(FROMRGB(prof[j][0], prof[j][1], prof[j][2]));

		/* canh tren va canh trai - huong sang */
		ColorFillVideoSurfaceArea(guiSAVEBUFFER,
			bx + i, by + i, bx + bw - i, by + i + 1, ca);
		ColorFillVideoSurfaceArea(guiSAVEBUFFER,
			bx + i, by + i, bx + i + 1, by + bh - i, ca);

		/* canh duoi va canh phai - dao chieu cho ra khoi chim */
		ColorFillVideoSurfaceArea(guiSAVEBUFFER,
			bx + i, by + bh - i - 1, bx + bw - i, by + bh - i, cb);
		ColorFillVideoSurfaceArea(guiSAVEBUFFER,
			bx + bw - i - 1, by + i, bx + bw - i, by + bh - i, cb);
	}
}

INT16 MapScreenLogTop()
{
	/* Widescreen frame has a 13px inner border; keep the text origin aligned
	 * with Message.cc while leaving the frame itself at the screen edge. */
	if (g_ui.isWidescreenLayout()) return 13 + 4;
	return (INT16)(SCREEN_HEIGHT - 118);   /* VFIT */
}

BOOLEAN fMapScreenBottomDirty = TRUE;

static BOOLEAN fMapBottomDirtied = FALSE;

//Used to flag the transition animation from mapscreen to laptop.
BOOLEAN gfStartMapScreenToLaptopTransition = FALSE;

// leaving map screen
BOOLEAN fLeavingMapScreen = FALSE;

// don't start transition from laptop to tactical stuff
BOOLEAN gfDontStartTransitionFromLaptop = FALSE;

// exiting to laptop?
BOOLEAN fLapTop = FALSE;

static BOOLEAN gfOneFramePauseOnExit = FALSE;

// exit states
static ExitToWhere gbExitingMapScreenToWhere = MAP_EXIT_TO_INVALID;

static UINT8 gubFirstMapscreenMessageIndex = 0;

UINT32 guiCompressionStringBaseTime = 0;

// graphics
namespace {
cache_key_t const guiMAPBOTTOMPANEL{ INTERFACEDIR "/map_screen_bottom.sti" };
cache_key_t const guiSliderBar{ INTERFACEDIR "/map_screen_bottom_arrows.sti" };
}

// buttons
GUIButtonRef        guiMapBottomExitButtons[3];
static GUIButtonRef guiMapBottomTimeButtons[2];
static GUIButtonRef guiMapMessageScrollButtons[2];

// mouse regions
static MOUSE_REGION gMapMessageScrollBarRegion;
static MOUSE_REGION gMapPauseRegion;

static MOUSE_REGION gTimeCompressionMask[3];


static void BtnLaptopCallback(GUI_BUTTON *btn, UINT32 reason);
static void BtnTacticalCallback(GUI_BUTTON *btn, UINT32 reason);
static void BtnOptionsFromMapScreenCallback(GUI_BUTTON *btn, UINT32 reason);

static void BtnTimeCompressMoreMapScreenCallback(GUI_BUTTON *btn, UINT32 reason);
static void BtnTimeCompressLessMapScreenCallback(GUI_BUTTON *btn, UINT32 reason);

static void BtnMessageDownMapScreenCallback(GUI_BUTTON *btn, UINT32 reason);
static void BtnMessageUpMapScreenCallback(GUI_BUTTON *btn, UINT32 reason);

static void CreateButtonsForMapScreenInterfaceBottom(void);
static void CreateCompressModePause(void);
static void CreateMapScreenBottomMessageScrollBarRegion(void);


void LoadMapScreenInterfaceBottom(void)
{
	CreateButtonsForMapScreenInterfaceBottom();
	CreateMapScreenBottomMessageScrollBarRegion( );

	// create pause region
	CreateCompressModePause( );
}


void DeleteMapBottomGraphics( void )
{
	RemoveVObject(guiMAPBOTTOMPANEL);
	RemoveVObject(guiSliderBar);
}


static void DeleteMapScreenBottomMessageScrollRegion(void);
static void DestroyButtonsForMapScreenInterfaceBottom();
static void RemoveCompressModePause(void);


void DeleteMapScreenInterfaceBottom( void )
{
	// will delete graphics loaded for the mapscreen interface bottom

	DestroyButtonsForMapScreenInterfaceBottom( );
	DeleteMapScreenBottomMessageScrollRegion( );

	// remove comrpess mode pause
	RemoveCompressModePause( );
}


static void DisplayCompressMode(void);
static void DisplayCurrentBalanceForMapBottom(void);
static void DisplayCurrentBalanceTitleForMapBottom(void);
static void DisplayProjectedDailyMineIncome(void);
static void DisplayProjectedDailyExpenses(void);
static void DisplayScrollBarSlider(void);
static void DrawNameOfLoadedSector();
static void EnableDisableBottomButtonsAndRegions(void);
static void EnableDisableMessageScrollButtonsAndRegions(void);

// will render the map screen bottom interface
void RenderMapScreenInterfaceBottom( void )
{
	// render whole panel
	if (fMapScreenBottomDirty)
	{
		if (!g_ui.isMapFullSize())
		{
			/* Fill the widescreen extension first, then preserve the complete panel art on top. */
			if (g_ui.isWidescreenLayout())
			{
				SGPBox const band = {(UINT16)MAP_BOTTOM_X, (UINT16)MAP_BOTTOM_Y,
				                     (UINT16)(SCREEN_WIDTH - MAP_BOTTOM_X), (UINT16)(SCREEN_HEIGHT - MAP_BOTTOM_Y)};
				DrawFillerOnSurface(guiSAVEBUFFER, band);
			}

			BltVideoObject(guiSAVEBUFFER, guiMAPBOTTOMPANEL, 0, MAP_BOTTOM_X, MAP_BOTTOM_Y);
				/* Use the original Wildfire frame across the tall history column. */
			if (g_ui.isWidescreenLayout())
			{
				if (GCM->doesGameResExists(INTERFACEDIR "/map_screen_log.sti"))
				{
					RenderMapScreenLogFrame(0, 0,
						(INT32)g_ui.get_MAP_BOTTOM_BASE_X(), SCREEN_HEIGHT);
				}
				else
				{
					RenderMapScreenLogFrameFallback(0, 0,
						(INT32)g_ui.get_MAP_BOTTOM_BASE_X(), SCREEN_HEIGHT);
				}
			}
		}
		if (g_ui.isMapFullSize())
		{
			/* Left column below the roster: plain filler (kept free for a
			 * future taller roster). Bottom band: filler over the corner and
			 * the art's old log frame, then the wide log box on top. */
						/* VFILL2: neo dai go cot trai vao DAY BANG DANH SACH LINH bang chinh
			 * cong thuc engine dung de ve danh sach, thay vi neo vao day ban do
			 * (hai thu nay khong lien quan nhau nen neo vao nhau la sai). Nho vay
			 * no tu dung o moi co man va moi co phong chu. */
			/* LISTLONG: khung danh sach gio da chay cham dinh khung nhat ky, nen dai go
			 * cot trai chi con can neo vao dung dinh do; neu neo cao hon no se ve
			 * de len khung danh sach. */
			INT32 const lcListBot = (INT32)SCREEN_HEIGHT - 118;
			INT32 const lcSafeTop = (INT32)SCREEN_HEIGHT - 125;
			/* kep hai dau: chi dung moc tinh duoc khi no nam trong khoang hop ly,
			 * neu khong thi tro ve cach cu de khong bao gio sinh ra hop am. */
			INT32 const lcRaw = (lcListBot > 8 && lcListBot < lcSafeTop) ? lcListBot : lcSafeTop;
			UINT16 const lcTop = (UINT16)lcRaw;
			UINT16 const lcH   = (UINT16)((INT32)SCREEN_HEIGHT > lcRaw ? (INT32)SCREEN_HEIGHT - lcRaw : 125);
			/* Fill the complete strip left of the map, not just the roster width;
			 * this removes the wide-screen wood gap behind the history log. */
			SGPBox const leftColumn = {0, lcTop, g_ui.get_MAP_VIEW_START_X(), lcH};
			DrawFillerOnSurface(guiSAVEBUFFER, leftColumn);
			SGPBox const band = {(UINT16)MAP_BOTTOM_X, (UINT16)MAP_BOTTOM_Y, (UINT16)(SCREEN_WIDTH - MAP_BOTTOM_X), 121};
			DrawFillerOnSurface(guiSAVEBUFFER, band);
			/* REALPANEL: tam tranh nen goc cua Wildfire rong dung 763 px,
			 * bang dung khoang tu MAP_BOTTOM_X den mep phai man hinh, nen
			 * ve thang no len thay vi tu ve khung chim bang tay. */
			BltVideoObject(guiSAVEBUFFER, guiMAPBOTTOMPANEL, 0, MAP_BOTTOM_X, MAP_BOTTOM_Y);
			/* The Wildfire art already contains the finance, radar, clock, and
			 * control recesses. Keep only the custom history-log frame; drawing
			 * synthetic section borders over the art makes those recesses appear
			 * shifted from their widgets. */
			RenderMapScreenLogFrameFallback(0, MapScreenLogTop(),
				(INT32)g_ui.get_MAP_BOTTOM_BASE_X(),
				(INT32)SCREEN_HEIGHT - MapScreenLogTop());
		}
		auto const& sMap{ sSelMap };

		if (GetSectorFlagStatus(sMap, SF_ALREADY_VISITED))
		{
			LoadRadarScreenBitmap(GetMapFileName(sMap, TRUE));
		}
		else
		{
			ClearOutRadarMapImage();
		}

		fInterfacePanelDirty = DIRTYLEVEL2;

		// display title
		DisplayCurrentBalanceTitleForMapBottom( );

		// dirty buttons
		MarkButtonsDirty( );

		// invalidate region (in full-size mode include the log column on the left)
		INT16 const restoreX = (g_ui.isMapFullSize() || g_ui.isWidescreenLayout()) ? 0 : MAP_BOTTOM_X;
		INT16 const restoreY = g_ui.isWidescreenLayout() ? 0 : g_ui.isMapFullSize() ? 354 : MAP_BOTTOM_Y;
		if (g_ui.isWidePanel())
		{
			/* LEVELRENDER: the panel drawn above wipes the current-level marker,
			 * so draw it again before the band reaches the screen. */
			RenderMapLevelSelectorFullSize();
		}
		RestoreExternBackgroundRect(restoreX, restoreY, SCREEN_WIDTH - restoreX, SCREEN_HEIGHT - restoreY);

		// re render radar map
		RenderRadarScreen( );

		// reset dirty flag
		fMapScreenBottomDirty = FALSE;
		fMapBottomDirtied = TRUE;
	}

	DisplayCompressMode( );

	DisplayCurrentBalanceForMapBottom( );
	DisplayProjectedDailyMineIncome( );
	if (g_ui.isMapFullSize()) DisplayProjectedDailyExpenses();

	// draw the name of the loaded sector
	DrawNameOfLoadedSector( );

	// display slider on the scroll bar
	DisplayScrollBarSlider( );

	// display messages that can be scrolled through
	DisplayStringsInMapScreenMessageList( );

	EnableDisableMessageScrollButtonsAndRegions( );

	EnableDisableBottomButtonsAndRegions( );

	fMapBottomDirtied = FALSE;
}


static GUIButtonRef MakeExitButton(INT32 off, INT32 on, INT16 x, INT16 y, GUI_CALLBACK click, const ST::string& help)
{
	GUIButtonRef const btn = QuickCreateButtonImg(INTERFACEDIR "/map_border_buttons.sti", off, on, x, y, MSYS_PRIORITY_HIGHEST - 1, click);
	btn->SetFastHelpText(help);
	btn->SetCursor(MSYS_NO_CURSOR);
	return btn;
}


static GUIButtonRef MakeArrowButton(INT32 grayed, INT32 off, INT32 on, INT16 x, INT16 y, GUI_CALLBACK click, const ST::string& help)
{
	GUIButtonRef const btn = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti", grayed, off, -1, on, -1, x, y, MSYS_PRIORITY_HIGHEST - 2, click);
	btn->SetFastHelpText(help);
	btn->SetCursor(MSYS_NO_CURSOR);
	return btn;
}


static void CreateButtonsForMapScreenInterfaceBottom(void)
{
	bool const fs = g_ui.isWidePanel();
	/* Real frame sizes (measured from map_border_buttons.sti): options disc
	 * 94x27, laptop/tactical 43x32. */
	guiMapBottomExitButtons[MAP_EXIT_TO_LAPTOP]   = MakeExitButton( 6, 15, fs ? (MAP_BOTTOM_BASE_X + 554) : MAP_BOTTOM_BASE_X + 456, fs ? (MAP_BOTTOM_BASE_Y + 410) : MAP_BOTTOM_BASE_Y + 410, BtnLaptopCallback,               pMapScreenBottomFastHelp[0]);
	guiMapBottomExitButtons[MAP_EXIT_TO_TACTICAL] = MakeExitButton( 7, 16, fs ? (MAP_BOTTOM_BASE_X + 607) : MAP_BOTTOM_BASE_X + 496, fs ? (MAP_BOTTOM_BASE_Y + 410) : MAP_BOTTOM_BASE_Y + 410, BtnTacticalCallback,             pMapScreenBottomFastHelp[1]);
	guiMapBottomExitButtons[MAP_EXIT_TO_OPTIONS]  = MakeExitButton(18, 19, fs ? (MAP_BOTTOM_BASE_X + 554) : MAP_BOTTOM_BASE_X + 458, fs ? (MAP_BOTTOM_BASE_Y + 372) : MAP_BOTTOM_BASE_Y + 372, BtnOptionsFromMapScreenCallback, pMapScreenBottomFastHelp[2]);

	// time compression buttons
	guiMapBottomTimeButtons[MAP_TIME_COMPRESS_MORE] = MakeArrowButton(10, 1, 3, fs ? (MAP_BOTTOM_BASE_X + 639) : MAP_BOTTOM_BASE_X + 528, fs ? (MAP_BOTTOM_BASE_Y + 456) : MAP_BOTTOM_BASE_Y + 456, BtnTimeCompressMoreMapScreenCallback, pMapScreenBottomFastHelp[3]);
	guiMapBottomTimeButtons[MAP_TIME_COMPRESS_LESS] = MakeArrowButton( 9, 0, 2, fs ? (MAP_BOTTOM_BASE_X + 558) : MAP_BOTTOM_BASE_X + 466, fs ? (MAP_BOTTOM_BASE_Y + 456) : MAP_BOTTOM_BASE_Y + 456, BtnTimeCompressLessMapScreenCallback, pMapScreenBottomFastHelp[4]);

	// scroll buttons
	INT16 const msgUpX   = g_ui.isWidescreenLayout() ? (MAP_BOTTOM_BASE_X - (MAP_BOTTOM_BASE_X * 23 / 261)) : g_ui.isWidePanel() ? (MAP_BOTTOM_BASE_X - 23 + SCROLL_NUDGE_X) : MAP_BOTTOM_BASE_X + 331;   /* SCROLLBAY */
	INT16 const msgUpY   = g_ui.isWidescreenLayout() ? (12 * SCREEN_HEIGHT / 409) : g_ui.isWidePanel() ? (MapScreenLogTop() + 14 + SCROLL_NUDGE_Y) : MAP_BOTTOM_BASE_Y + 371;
	INT16 const msgDownY = g_ui.isWidescreenLayout() ? (381 * SCREEN_HEIGHT / 409) : g_ui.isWidePanel() ? (MapScreenLogTop() + 85 + SCROLL_NUDGE_Y) : MAP_BOTTOM_BASE_Y + 452;
	guiMapMessageScrollButtons[MAP_SCROLL_MESSAGE_UP]   = MakeArrowButton(11, 4, 6, msgUpX, msgUpY,   BtnMessageUpMapScreenCallback,   pMapScreenBottomFastHelp[5]);
	guiMapMessageScrollButtons[MAP_SCROLL_MESSAGE_DOWN] = MakeArrowButton(12, 5, 7, msgUpX, msgDownY, BtnMessageDownMapScreenCallback, pMapScreenBottomFastHelp[6]);
}


static void DestroyButtonsForMapScreenInterfaceBottom()
{
	FOR_EACH(GUIButtonRef, i, guiMapBottomExitButtons)    RemoveButton(*i);
	FOR_EACH(GUIButtonRef, i, guiMapBottomTimeButtons)    RemoveButton(*i);
	FOR_EACH(GUIButtonRef, i, guiMapMessageScrollButtons) RemoveButton(*i);
	fMapScreenBottomDirty = TRUE;
}


static void BtnLaptopCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		RequestTriggerExitFromMapscreen(MAP_EXIT_TO_LAPTOP);
	}
}


static void BtnTacticalCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		RequestTriggerExitFromMapscreen(MAP_EXIT_TO_TACTICAL);
	}
}


static void BtnOptionsFromMapScreenCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		RequestTriggerExitFromMapscreen(MAP_EXIT_TO_OPTIONS);
	}
}


static void DrawNameOfLoadedSector()
{
	SetFontDestBuffer(FRAME_BUFFER);
	SGPFont const font = COMPFONT;
	SetFontAttributes(font, 183);

	ST::string buf = GetSectorIDString(sSelMap, TRUE);
	buf = ReduceStringLength(buf, g_ui.isWidePanel() ? 92 : 80, font);

	if (g_ui.isWidePanel())
	{
		MPrint(MAP_BOTTOM_BASE_X + 663, MAP_BOTTOM_BASE_Y + 423, buf, HCenterVCenterAlign(94, 23));   /* RIGHTBAY */
	}
	else
	{
		MPrint(MAP_BOTTOM_BASE_X + 548, MAP_BOTTOM_BASE_Y + 426, buf, HCenterVCenterAlign(80, 16));
	}
}


static void CompressModeClickCallback(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if( iReason & MSYS_CALLBACK_REASON_ANY_BUTTON_UP )
	{
		if (CommonTimeCompressionChecks()) return;

		RequestToggleTimeCompression();
	}
}


static void BtnTimeCompressMoreMapScreenCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN)
	{
		if (CommonTimeCompressionChecks()) return;
	}
	else if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		fMapScreenBottomDirty = TRUE;
		RequestIncreaseInTimeCompression();
	}
	else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		CommonTimeCompressionChecks();
	}
}


static void BtnTimeCompressLessMapScreenCallback(GUI_BUTTON *btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN)
	{
		if (CommonTimeCompressionChecks()) return;
	}
	else if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		fMapScreenBottomDirty = TRUE;
		RequestDecreaseInTimeCompression();
	}
	else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		CommonTimeCompressionChecks();
	}
}


static void BtnMessageDownMapScreenCallback(GUI_BUTTON *btn, UINT32 reason)
{
	static UINT32 uiLastRepeatScrollTime = 0;

	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN)
	{
		uiLastRepeatScrollTime = 0;
	}
	else if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		MapScreenMsgScrollDown(1);
	}
	else if (reason & (MSYS_CALLBACK_REASON_POINTER_REPEAT))
	{
		if (GetJA2Clock() - uiLastRepeatScrollTime >= MESSAGE_BTN_SCROLL_TIME)
		{
			MapScreenMsgScrollDown(1);
			uiLastRepeatScrollTime = GetJA2Clock();
		}
	}
	else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		uiLastRepeatScrollTime = 0;
	}
	else if (reason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
		MapScreenMsgScrollDown(MAX_MESSAGES_ON_MAP_BOTTOM);
	}
	else if (reason & MSYS_CALLBACK_REASON_RBUTTON_REPEAT)
	{
		if (GetJA2Clock() - uiLastRepeatScrollTime >= MESSAGE_BTN_SCROLL_TIME)
		{
			MapScreenMsgScrollDown(MAX_MESSAGES_ON_MAP_BOTTOM);
			uiLastRepeatScrollTime = GetJA2Clock();
		}
	}
}


static void BtnMessageUpMapScreenCallback(GUI_BUTTON *btn, UINT32 reason)
{
	static UINT32 uiLastRepeatScrollTime = 0;

	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN)
	{
		uiLastRepeatScrollTime = 0;
	}
	else if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		MapScreenMsgScrollUp(1);
	}
	else if (reason & MSYS_CALLBACK_REASON_POINTER_REPEAT)
	{
		if (GetJA2Clock() - uiLastRepeatScrollTime >= MESSAGE_BTN_SCROLL_TIME)
		{
			MapScreenMsgScrollUp(1);
			uiLastRepeatScrollTime = GetJA2Clock();
		}
	}
	else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		uiLastRepeatScrollTime = 0;
	}
	else if (reason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
		MapScreenMsgScrollUp(MAX_MESSAGES_ON_MAP_BOTTOM);
	}
	else if (reason & MSYS_CALLBACK_REASON_RBUTTON_REPEAT)
	{
		if (GetJA2Clock() - uiLastRepeatScrollTime >= MESSAGE_BTN_SCROLL_TIME)
		{
			MapScreenMsgScrollUp(MAX_MESSAGES_ON_MAP_BOTTOM);
			uiLastRepeatScrollTime = GetJA2Clock();
		}
	}
}


static void EnableDisableMessageScrollButtonsAndRegions(void)
{
	UINT8 ubNumMessages;

	ubNumMessages = GetRangeOfMapScreenMessages();

	// if no scrolling required, or already showing the topmost message
	if( ( ubNumMessages <= MAX_MESSAGES_ON_MAP_BOTTOM ) || ( gubFirstMapscreenMessageIndex == 0 ) )
	{
		DisableButton( guiMapMessageScrollButtons[ MAP_SCROLL_MESSAGE_UP ] );
		guiMapMessageScrollButtons[MAP_SCROLL_MESSAGE_UP]->uiFlags &= ~BUTTON_CLICKED_ON;
	}
	else
	{
		EnableButton( guiMapMessageScrollButtons[ MAP_SCROLL_MESSAGE_UP ] );
	}

	// if no scrolling required, or already showing the last message
	if( ( ubNumMessages <= MAX_MESSAGES_ON_MAP_BOTTOM ) ||
			( ( gubFirstMapscreenMessageIndex + MAX_MESSAGES_ON_MAP_BOTTOM ) >= ubNumMessages ) )
	{
		DisableButton( guiMapMessageScrollButtons[ MAP_SCROLL_MESSAGE_DOWN ] );
		guiMapMessageScrollButtons[MAP_SCROLL_MESSAGE_DOWN]->uiFlags &= ~BUTTON_CLICKED_ON;
	}
	else
	{
		EnableButton( guiMapMessageScrollButtons[ MAP_SCROLL_MESSAGE_DOWN ] );
	}

	if( ubNumMessages <= MAX_MESSAGES_ON_MAP_BOTTOM )
	{
		gMapMessageScrollBarRegion.Disable();
	}
	else
	{
		gMapMessageScrollBarRegion.Enable();
	}
}


static void DisplayCompressMode(void)
{
	static UINT8 usColor = FONT_LTGREEN;

	// get compress speed
	ST::string Time;
	if( giTimeCompressMode != NOT_USING_TIME_COMPRESSION )
	{
		Time = sTimeStrings[IsTimeBeingCompressed() ? giTimeCompressMode : 0];
	}

	INT16 const cmX = g_ui.isWidePanel() ? (MAP_BOTTOM_BASE_X + 573) : MAP_BOTTOM_BASE_X + 489;
	INT16 const cmY = g_ui.isWidePanel() ? (MAP_BOTTOM_BASE_Y + 456) : MAP_BOTTOM_BASE_Y + 457;
	/* RIGHTBAY: khoang trong giua hai mui ten rong 66, cao 13. */
	INT16 const cmW = g_ui.isWidePanel() ? 66 : (522 - 489);
	INT16 const cmH = g_ui.isWidePanel() ? 13 : (467 - 454);
	RestoreExternBackgroundRect( cmX, cmY, cmW, cmH );
	SetFontDestBuffer(FRAME_BUFFER);

	if( GetJA2Clock() - guiCompressionStringBaseTime >= PAUSE_GAME_TIMER )
	{
		if( usColor == FONT_LTGREEN )
		{
			usColor = FONT_WHITE;
		}
		else
		{
			usColor = FONT_LTGREEN;
		}

		guiCompressionStringBaseTime = GetJA2Clock();
	}

	if (giTimeCompressMode != 0 && !GamePaused())
	{
		usColor = FONT_LTGREEN;
	}

	SetFontAttributes(COMPFONT, usColor);
	MPrint(cmX, cmY, Time, HCenterVCenterAlign(cmW, cmH));
}


static void CreateCompressModePause(void)
{
	INT16 const pzX = g_ui.isWidePanel() ? (MAP_BOTTOM_BASE_X + 573) : MAP_BOTTOM_BASE_X + 487;
	INT16 const pzY = g_ui.isWidePanel() ? (MAP_BOTTOM_BASE_Y + 456) : MAP_BOTTOM_BASE_Y + 456;
	MSYS_DefineRegion( &gMapPauseRegion, pzX, pzY, pzX + (g_ui.isWidePanel() ? 66 : 35), pzY + (g_ui.isWidePanel() ? 13 : 11), MSYS_PRIORITY_HIGH,
							MSYS_NO_CURSOR, MSYS_NO_CALLBACK, CompressModeClickCallback );
	gMapPauseRegion.SetFastHelpText(pMapScreenBottomFastHelp[7]);
}


static void RemoveCompressModePause(void)
{
	MSYS_RemoveRegion( &gMapPauseRegion );
}


static void MapScreenMessageBoxCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		MapScreenMsgScrollUp(3);
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		MapScreenMsgScrollDown(3);
	}
}


static MOUSE_REGION MapMessageBoxRegion;


static void MapScreenMessageScrollBarCallBack(MOUSE_REGION* pRegion, UINT32 iReason);


static void CreateMapScreenBottomMessageScrollBarRegion(void)
{
	const INT8 prio = MSYS_PRIORITY_NORMAL;
	{
		const UINT16 x = MESSAGE_SCROLL_AREA_START_X;
		const UINT16 y = MESSAGE_SCROLL_AREA_START_Y;
		const UINT16 w = MESSAGE_SCROLL_AREA_WIDTH;
		const UINT16 h = MESSAGE_SCROLL_AREA_HEIGHT;
		MSYS_DefineRegion(&gMapMessageScrollBarRegion, x, y, x + w, y + h, prio, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MapScreenMessageScrollBarCallBack);
	}
	{
		const UINT16 x = MESSAGE_BOX_X;
		const UINT16 y = MESSAGE_BOX_Y;
		const UINT16 w = MESSAGE_BOX_W;
		const UINT16 h = MESSAGE_BOX_H;
		MSYS_DefineRegion(&MapMessageBoxRegion, x, y, x + w, y + h, prio, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MapScreenMessageBoxCallBack);
	}
}


static void DeleteMapScreenBottomMessageScrollRegion(void)
{
	MSYS_RemoveRegion( &gMapMessageScrollBarRegion );
	MSYS_RemoveRegion(&MapMessageBoxRegion);
}


static void MapScreenMessageScrollBarCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	UINT8	ubDesiredSliderOffset;
	UINT8 ubDesiredMessageIndex;
	UINT8 ubNumMessages;

	if ( iReason & ( MSYS_CALLBACK_REASON_POINTER_DWN | MSYS_CALLBACK_REASON_POINTER_REPEAT ) )
	{
		// how many messages are there?
		ubNumMessages = GetRangeOfMapScreenMessages();

		// region is supposed to be disabled if there aren't enough messages to scroll.  Formulas assume this
		if ( ubNumMessages > MAX_MESSAGES_ON_MAP_BOTTOM )
		{
			const UINT8 ubMouseYOffset = pRegion->RelativeYPos;

			// if clicking in the top 5 pixels of the slider bar
			if ( ubMouseYOffset < ( SLIDER_HEIGHT / 2 ) )
			{
				// scroll all the way to the top
				ubDesiredMessageIndex = 0;
			}
			// if clicking in the bottom 6 pixels of the slider bar
			else if ( ubMouseYOffset >= ( MESSAGE_SCROLL_AREA_HEIGHT - ( SLIDER_HEIGHT / 2 ) ) )
			{
				// scroll all the way to the bottom
				ubDesiredMessageIndex = ubNumMessages - MAX_MESSAGES_ON_MAP_BOTTOM;
			}
			else
			{
				// somewhere in between
				ubDesiredSliderOffset = ubMouseYOffset - ( SLIDER_HEIGHT / 2 );

				Assert( ubDesiredSliderOffset <= SLIDER_BAR_RANGE );

				// calculate what the index should be to place the slider at this offset (round fractions of .5+ up)
				ubDesiredMessageIndex = ( ( ubDesiredSliderOffset * ( ubNumMessages - MAX_MESSAGES_ON_MAP_BOTTOM ) ) + ( SLIDER_BAR_RANGE / 2 ) ) / SLIDER_BAR_RANGE;
			}

			// if it's a change
			if ( ubDesiredMessageIndex != gubFirstMapscreenMessageIndex )
			{
				ChangeCurrentMapscreenMessageIndex( ubDesiredMessageIndex );
			}
		}
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		MapScreenMsgScrollUp(3);
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		MapScreenMsgScrollDown(3);
	}
}


static void DisplayScrollBarSlider(void)
{
	// will display the scroll bar icon
	UINT8 ubNumMessages;
	UINT8 ubSliderOffset;

	ubNumMessages = GetRangeOfMapScreenMessages();

	// only show the slider if there are more messages than will fit on screen
	if ( ubNumMessages > MAX_MESSAGES_ON_MAP_BOTTOM )
	{
		// calculate where slider should be positioned
		ubSliderOffset = ( SLIDER_BAR_RANGE * gubFirstMapscreenMessageIndex ) / ( ubNumMessages - MAX_MESSAGES_ON_MAP_BOTTOM );

		BltVideoObject(FRAME_BUFFER, guiSliderBar, 8, MESSAGE_SCROLL_AREA_START_X + 2, MESSAGE_SCROLL_AREA_START_Y + ubSliderOffset);
	}
}


static void EnableDisableTimeCompressButtons(void);


static void EnableDisableBottomButtonsAndRegions(void)
{
	// this enables and disables the buttons MAP_EXIT_TO_LAPTOP, MAP_EXIT_TO_TACTICAL, and MAP_EXIT_TO_OPTIONS
	for (ExitToWhere iExitButtonIndex = MAP_EXIT_TO_LAPTOP; iExitButtonIndex <= MAP_EXIT_TO_OPTIONS; ++iExitButtonIndex)
	{
		EnableButton(guiMapBottomExitButtons[iExitButtonIndex], AllowedToExitFromMapscreenTo(iExitButtonIndex));
	}

	// enable/disable time compress buttons and region masks
	EnableDisableTimeCompressButtons( );
	CreateDestroyMouseRegionMasksForTimeCompressionButtons( );


	// Enable/Disable map inventory panel buttons

	// if in merc inventory panel
	if( fShowInventoryFlag )
	{
		// and an item is in the cursor
		EnableButton(giMapInvDoneButton, !fMapInventoryItem && !InKeyRingPopup() && !InItemStackPopup());

		if( fShowDescriptionFlag )
		{
			ForceButtonUnDirty( giMapInvDoneButton );
		}
	}
}


static void EnableDisableTimeCompressButtons(void)
{
	if (!AllowedToTimeCompress())
	{
		DisableButton( guiMapBottomTimeButtons[ MAP_TIME_COMPRESS_MORE ] );
		DisableButton( guiMapBottomTimeButtons[ MAP_TIME_COMPRESS_LESS ] );
	}
	else
	{
		// disable LESS if time compression is at minimum or OFF
		EnableButton(guiMapBottomTimeButtons[MAP_TIME_COMPRESS_LESS], IsTimeCompressionOn() && giTimeCompressMode != TIME_COMPRESS_X0);

		// disable MORE if we're not paused and time compression is at maximum
		// only disable MORE if we're not paused and time compression is at maximum
		EnableButton(guiMapBottomTimeButtons[MAP_TIME_COMPRESS_MORE], !IsTimeCompressionOn() || giTimeCompressMode != TIME_COMPRESS_60MINS);
	}
}


void EnableDisAbleMapScreenOptionsButton( BOOLEAN fEnable )
{
	EnableButton(guiMapBottomExitButtons[MAP_EXIT_TO_OPTIONS], fEnable);
}


BOOLEAN AllowedToTimeCompress( void )
{
	// if already leaving, disallow any other attempts to exit
	if ( fLeavingMapScreen )
	{
		return( FALSE );
	}

	// if already going someplace
	if (gbExitingMapScreenToWhere != MAP_EXIT_TO_INVALID) return FALSE;

	// if we're locked into paused time compression by some event that enforces that
	if ( PauseStateLocked() )
	{
		return( FALSE );
	}

	// meanwhile coming up
	if ( gfMeanwhileTryingToStart )
	{
		return( FALSE );
	}

	// someone has something to say
	if ( !DialogueQueueIsEmpty() )
	{
		return( FALSE );
	}

	// moving / confirming movement
	if( ( bSelectedDestChar != -1 ) || fPlotForHelicopter || gfInConfirmMapMoveMode || fShowMapScreenMovementList )
	{
		return( FALSE );
	}

	if (fShowAssignmentMenu || fShowTrainingMenu || fShowAttributeMenu || fShowSquadMenu || fShowContractMenu)
	{
		return( FALSE );
	}

	if( fShowUpdateBox || fShowTownInfo || ( sSelectedMilitiaTown != 0 ) )
	{
		return( FALSE );
	}

	// renewing contracts
	if ( gfContractRenewalSquenceOn )
	{
		return( FALSE );
	}

	// disabled due to battle?
	if( ( fDisableMapInterfaceDueToBattle ) || ( fDisableDueToBattleRoster ) )
	{
		return( FALSE );
	}

	// if holding an inventory item
	if ( fMapInventoryItem )
	{
		return( FALSE );
	}

	// show the inventory pool?
	if( fShowMapInventoryPool )
	{
		// prevent time compress (items get stolen over time, etc.)
		return( FALSE );
	}

	// no mercs have ever been hired
	if (!gfAtLeastOneMercWasHired) return FALSE;


	// no usable mercs on team!
	if ( !AnyUsableRealMercenariesOnTeam() )
	{
		return( FALSE );
	}

		// must wait till bombs go off
	if ( ActiveTimedBombExists() )
	{
		return( FALSE );
	}

	// hostile sector / in battle
	if( (gTacticalStatus.uiFlags & INCOMBAT ) || ( gTacticalStatus.fEnemyInSector ) )
	{
		return( FALSE );
	}

	if( PlayerGroupIsInACreatureInfestedMine() )
	{
		return FALSE;
	}

	// bloodcat ambush?
	if (gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE && HostileBloodcatsPresent())
	{
		return FALSE;
	}

	return( TRUE );
}


static void DisplayCurrentBalanceTitleForMapBottom(void)
{
	SetFontDestBuffer(guiSAVEBUFFER);
	SetFontAttributes(COMPFONT, MAP_BOTTOM_FONT_COLOR);
	HCenterVCenterAlign const alignment{ 437 - 359, 10 };

	if (g_ui.isWidePanel())
	{
		/* MONEYPLATE: the panel art recesses only two money plaques, at panel
		   (372,27)-(528,50) and (372,85)-(528,108). Each label sits on the
		   wood just above its plaque; the figure goes inside the plaque. */
		HCenterVCenterAlign const a{ 157, 24 };
		MPrint(MAP_BOTTOM_BASE_X + 372, MAP_BOTTOM_BASE_Y + 362, pMapScreenBottomText,   a);
		MPrint(MAP_BOTTOM_BASE_X + 372, MAP_BOTTOM_BASE_Y + 416, zMarksMapScreenText[2], a);
	}
	else
	{
		MPrint(MAP_BOTTOM_BASE_X + 359, MAP_BOTTOM_BASE_Y + 387 - 14, pMapScreenBottomText, alignment);
		MPrint(MAP_BOTTOM_BASE_X + 359, MAP_BOTTOM_BASE_Y + 433 - 14, zMarksMapScreenText[2], alignment);
	}

	SetFontDestBuffer(FRAME_BUFFER);
}


static void DisplayCurrentBalanceForMapBottom(void)
{
	// show the current balance for the player on the map panel bottom
	SetFontDestBuffer(FRAME_BUFFER);
	SetFontAttributes(COMPFONT, 183);
	if (g_ui.isWidePanel())
	{
		MPrint(MAP_BOTTOM_BASE_X + 372, MAP_BOTTOM_BASE_Y + 386, SPrintMoney(LaptopSaveInfo.iCurrentBalance), HCenterVCenterAlign(157, 24));
	}
	else
	{
		MPrint(MAP_BOTTOM_BASE_X + 359, MAP_BOTTOM_BASE_Y + 387 + 2,
			SPrintMoney(LaptopSaveInfo.iCurrentBalance),
			HCenterVCenterAlign(437 - 359, 10));
	}
}


static void CompressMaskClickCallback(MOUSE_REGION* pRegion, UINT32 iReason);


void CreateDestroyMouseRegionMasksForTimeCompressionButtons()
{
	static bool created = false;

	// Disable buttons, if not allowed to compress time.
	bool const disabled = fInMapMode && !AllowedToTimeCompress();
	if (disabled && !created)
	{
		// Mask over compress more, compress less and paus game buttons.
		bool const fsm = g_ui.isWidePanel();
		INT16 const mMoreX = fsm ? (MAP_BOTTOM_BASE_X + 639) : MAP_BOTTOM_BASE_X + 528, mLessX = fsm ? (MAP_BOTTOM_BASE_X + 558) : MAP_BOTTOM_BASE_X + 466;
		INT16 const mPzX   = fsm ? (MAP_BOTTOM_BASE_X + 573) : MAP_BOTTOM_BASE_X + 487, mY    = fsm ? (MAP_BOTTOM_BASE_Y + 456) : MAP_BOTTOM_BASE_Y + 457;
		MSYS_DefineRegion(&gTimeCompressionMask[0], mMoreX, mY, mMoreX + 13, mY + 14, MSYS_PRIORITY_HIGHEST - 1, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, CompressMaskClickCallback);
		MSYS_DefineRegion(&gTimeCompressionMask[1], mLessX, mY, mLessX + 13, mY + 14, MSYS_PRIORITY_HIGHEST - 1, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, CompressMaskClickCallback);
		MSYS_DefineRegion(&gTimeCompressionMask[2], mPzX,   mY, mPzX + (fsm ? 66 : 35), mY + (fsm ? 13 : 11), MSYS_PRIORITY_HIGHEST - 1, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, CompressMaskClickCallback);
		created = true;
	}
	else if (!disabled && created)
	{
		FOR_EACH(MOUSE_REGION, i, gTimeCompressionMask) MSYS_RemoveRegion(&*i);
		created = false;
	}
}


static void CompressMaskClickCallback(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if( iReason & MSYS_CALLBACK_REASON_POINTER_UP )
	{
		TellPlayerWhyHeCantCompressTime( );
	}
}


static void DisplayProjectedDailyMineIncome(void)
{
	INT32 iRate = 0;
	static INT32 iOldRate = -1;

	// grab the rate from the financial system
	iRate = GetProjectedTotalDailyIncome( );

	if( iRate != iOldRate )
	{
		iOldRate = iRate;
		fMapScreenBottomDirty = TRUE;

		// if screen was not dirtied, leave
		if (!fMapBottomDirtied) return;
	}

	SetFontDestBuffer(FRAME_BUFFER);
	SetFontAttributes(COMPFONT, 183);
	if (g_ui.isWidePanel())
	{
		MPrint(MAP_BOTTOM_BASE_X + 372, MAP_BOTTOM_BASE_Y + 444, SPrintMoney(iRate), HCenterVCenterAlign(157, 24));
	}
	else
	{
		MPrint(MAP_BOTTOM_BASE_X + 359, MAP_BOTTOM_BASE_Y + 433 + 2,
			SPrintMoney(iRate), HCenterVCenterAlign(437 - 359, 10));
	}
}


/* Projected daily expenses: the summed daily salary of every hired merc.
 * Shown only in the full-size layout (third finance box). */
static INT32 GetProjectedTotalDailyExpenses(void)
{
	INT32 iTotal = 0;
	CFOR_EACH_IN_TEAM(s, OUR_TEAM)
	{
		if (s->bLife <= 0) continue;
		iTotal += GetProfile(s->ubProfile).sSalary;
	}
	return iTotal;
}


static void DisplayProjectedDailyExpenses(void)
{
	static INT32 iOldExpenses = -1;
	INT32 const iExpenses = GetProjectedTotalDailyExpenses();

	if (iExpenses != iOldExpenses)
	{
		iOldExpenses = iExpenses;
		fMapScreenBottomDirty = TRUE;
		if (!fMapBottomDirtied) return;
	}

	/* MONEYPLATE: the Wildfire panel art has room for two money plaques
	   only, so the daily expenses figure is no longer drawn. The tally is
	   kept because it still marks the panel dirty when the payroll moves. */
	(void)iExpenses;
}


BOOLEAN CommonTimeCompressionChecks( void )
{
	if (bSelectedDestChar != -1 || fPlotForHelicopter)
	{
		// abort plotting movement
		AbortMovementPlottingMode( );
		return( TRUE );
	}

	return( FALSE );
}


bool AnyUsableRealMercenariesOnTeam()
{
	/* Check whether there is a merc on team, who is not a vehicle, robot, POW or
		* EPC. */
	CFOR_EACH_IN_TEAM(i, OUR_TEAM)
	{
		SOLDIERTYPE const& s = *i;
		if (s.bLife <= 0)                            continue;
		if (IsMechanical(s))                         continue;
		if (s.bAssignment == ASSIGNMENT_POW)         continue;
		if (s.bAssignment == ASSIGNMENT_DEAD)        continue;
		if (s.ubWhatKindOfMercAmI == MERC_TYPE__EPC) continue;
		return true;
	}
	return false;
}



void RequestTriggerExitFromMapscreen(ExitToWhere const bExitToWhere)
{
	Assert( ( bExitToWhere >= MAP_EXIT_TO_LAPTOP ) && ( bExitToWhere <= MAP_EXIT_TO_SAVE ) );

	// if allowed to do so
	if ( AllowedToExitFromMapscreenTo( bExitToWhere ) )
	{
		//if the screen to exit to is the SAVE screen
		if( bExitToWhere == MAP_EXIT_TO_SAVE )
		{
			//if the game CAN NOT be saved
			if( !CanGameBeSaved() )
			{
				//Display a message saying the player cant save now
				DoMapMessageBox( MSG_BOX_BASIC_STYLE, zNewTacticalMessages[ TCTL_MSG__IRON_MAN_CANT_SAVE_NOW ], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL );
				return;
			}
			else if ( gGameOptions.ubGameSaveMode == DIF_DEAD_IS_DEAD )
			{
				//Display DiD message saying the player cant save now
				DoMapMessageBox( MSG_BOX_BASIC_STYLE, zNewTacticalMessages[ TCTL_MSG__DEAD_IS_DEAD_CANT_SAVE_NOW ], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL );
				return;
			}
		}

		// permit it, and get the ball rolling
		gbExitingMapScreenToWhere = bExitToWhere;

		// delay until mapscreen has had a chance to render at least one full frame
		gfOneFramePauseOnExit = TRUE;
	}
}


BOOLEAN AllowedToExitFromMapscreenTo(ExitToWhere const bExitToWhere)
{
	Assert( ( bExitToWhere >= MAP_EXIT_TO_LAPTOP ) && ( bExitToWhere <= MAP_EXIT_TO_SAVE ) );

	// if already leaving, disallow any other attempts to exit
	if ( fLeavingMapScreen )
	{
		return( FALSE );
	}

	// if already going someplace else
	if (gbExitingMapScreenToWhere != MAP_EXIT_TO_INVALID &&
			gbExitingMapScreenToWhere != bExitToWhere)
	{
		return( FALSE );
	}

	// someone has something to say
	if ( !DialogueQueueIsEmpty() )
	{
		return( FALSE );
	}

	// meanwhile coming up
	if ( gfMeanwhileTryingToStart )
	{
		return( FALSE );
	}

	// if we're locked into paused time compression by some event that enforces that
	if ( PauseStateLocked() )
	{
		return( FALSE );
	}

	// if holding an inventory item
	if (fMapInventoryItem) return FALSE;

	if( fShowUpdateBox || fShowTownInfo || ( sSelectedMilitiaTown != 0 ) )
	{
		return( FALSE );
	}

	// renewing contracts
	if( gfContractRenewalSquenceOn )
	{
		return( FALSE );
	}

	// battle about to occur?
	if( ( fDisableDueToBattleRoster ) || ( fDisableMapInterfaceDueToBattle ) )
	{
		return( FALSE );
	}

	// the following tests apply to going tactical screen only
	if ( bExitToWhere == MAP_EXIT_TO_TACTICAL )
	{
		// if in battle or bloodcat ambush, the ONLY sector we can go tactical in is the one that's loaded
		auto const& sector{ sSelMap };
		BOOLEAN fBattleGoingOn = gTacticalStatus.uiFlags & INCOMBAT || gTacticalStatus.fEnemyInSector || (gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE && HostileBloodcatsPresent());
		BOOLEAN fCurrentSectorSelected = sector == gWorldSector;
		if (fBattleGoingOn && !fCurrentSectorSelected)
		{
			return( FALSE );
		}

		// must have some mercs there
		if (!CanGoToTacticalInSector(sector))
		{
			return( FALSE );
		}
	}

	//if we are map screen sector inventory
	if( fShowMapInventoryPool )
	{
		//dont allow it
		return( FALSE );
	}

	// OK to go there, passed all the checks
	return( TRUE );
}


void HandleExitsFromMapScreen( void )
{
	// if going somewhere
	if (gbExitingMapScreenToWhere == MAP_EXIT_TO_INVALID) return;

	// delay all exits by one frame...
	if (gfOneFramePauseOnExit)
	{
		gfOneFramePauseOnExit = FALSE;
		return;
	}

	// make sure it's still legal to do this!
	if ( AllowedToExitFromMapscreenTo( gbExitingMapScreenToWhere ) )
	{
		// see where we're trying to go
		switch ( gbExitingMapScreenToWhere )
		{
			case MAP_EXIT_TO_LAPTOP:
				fLapTop = TRUE;
				SetPendingNewScreen(LAPTOP_SCREEN);

				BltVideoSurface(guiEXTRABUFFER, FRAME_BUFFER, 0, 0, NULL);
				gfStartMapScreenToLaptopTransition = TRUE;
				break;

			case MAP_EXIT_TO_TACTICAL:
				SetCurrentWorldSector(sSelMap);
				break;

			case MAP_EXIT_TO_OPTIONS:
				guiPreviousOptionScreen = guiCurrentScreen;
				SetPendingNewScreen( OPTIONS_SCREEN );
				break;

			case MAP_EXIT_TO_SAVE:
			case MAP_EXIT_TO_LOAD:
				gfCameDirectlyFromGame = TRUE;
				guiPreviousOptionScreen = guiCurrentScreen;
				SetPendingNewScreen( SAVE_LOAD_SCREEN );
				break;

			default:
				// invalid exit type
				Assert( FALSE );
		}

		// time compression during mapscreen exit doesn't seem to cause any problems, but turn it off as early as we can
		StopTimeCompression();

		// now leaving mapscreen
		fLeavingMapScreen = TRUE;
	}

	// cancel exit, either we're on our way, or we're not allowed to go
	gbExitingMapScreenToWhere = MAP_EXIT_TO_INVALID;
}



void MapScreenMsgScrollDown( UINT8 ubLinesDown )
{
	UINT8 ubNumMessages;

	ubNumMessages = GetRangeOfMapScreenMessages();

	// check if we can go that far, only go as far as we can
	if ( ( gubFirstMapscreenMessageIndex + MAX_MESSAGES_ON_MAP_BOTTOM + ubLinesDown ) > ubNumMessages )
	{
		ubLinesDown = ubNumMessages - gubFirstMapscreenMessageIndex - std::min(int(ubNumMessages), MAX_MESSAGES_ON_MAP_BOTTOM);
	}

	if ( ubLinesDown > 0 )
	{
		ChangeCurrentMapscreenMessageIndex( ( UINT8 ) ( gubFirstMapscreenMessageIndex + ubLinesDown ) );
	}
}


void MapScreenMsgScrollUp( UINT8 ubLinesUp )
{
	// check if we can go that far, only go as far as we can
	if ( gubFirstMapscreenMessageIndex < ubLinesUp )
	{
		ubLinesUp = gubFirstMapscreenMessageIndex;
	}

	if ( ubLinesUp > 0 )
	{
		ChangeCurrentMapscreenMessageIndex( ( UINT8 ) ( gubFirstMapscreenMessageIndex - ubLinesUp ) );
	}
}



void MoveToEndOfMapScreenMessageList( void )
{
	UINT8 ubDesiredMessageIndex;
	UINT8 ubNumMessages;

	ubNumMessages = GetRangeOfMapScreenMessages();

	ubDesiredMessageIndex = ubNumMessages - std::min(int(ubNumMessages), MAX_MESSAGES_ON_MAP_BOTTOM);
	ChangeCurrentMapscreenMessageIndex( ubDesiredMessageIndex );
}



void ChangeCurrentMapscreenMessageIndex( UINT8 ubNewMessageIndex )
{
	Assert(ubNewMessageIndex + MAX_MESSAGES_ON_MAP_BOTTOM <= std::max(MAX_MESSAGES_ON_MAP_BOTTOM, int(GetRangeOfMapScreenMessages())));

	gubFirstMapscreenMessageIndex = ubNewMessageIndex;
	gubCurrentMapMessageString = ( gubStartOfMapScreenMessageList + gubFirstMapscreenMessageIndex ) % 256;

	// refresh screen
	fMapScreenBottomDirty = TRUE;
}
