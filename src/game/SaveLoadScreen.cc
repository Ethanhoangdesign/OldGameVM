#include "Directories.h"
#include "Font.h"
#include "GameLoop.h"
#include "HImage.h"
#include "Local.h"
#include "Timer_Control.h"
#include "Types.h"
#include "SaveLoadScreen.h"
#include "Video.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "Render_Dirty.h"
#include "Text_Input.h"
#include "SaveLoadGame.h"
#include "WordWrap.h"
#include "StrategicMap.h"
#include "Finances.h"
#include "Cursors.h"
#include "VObject.h"
#include "Options_Screen.h"
#include "GameVersion.h"
#include "SysUtil.h"
#include "Overhead.h"
#include "GameScreen.h"
#include "GameSettings.h"
#include "Fade_Screen.h"
#include "Game_Init.h"
#include "Sys_Globals.h"
#include "Text.h"
#include "Message.h"
#include "Map_Screen_Interface.h"
#include "GameRes.h"
#include "Campaign_Types.h"
#include "Button_System.h"
#include "Debug.h"
#include "JAScreens.h"
#include "VSurface.h"
#include "FileMan.h"
#include "Campaign_Init.h"
#include "UILayout.h"
#include "Handle_UI.h"
#include "Interface_Dialogue.h"
#include "Meanwhile.h"
#include "PreBattle_Interface.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "VObject_Blitters.h"

#include <algorithm>
#include <string_theory/format>
#include <string_theory/string>

#include <ctime>


constexpr int NUM_SAVE_GAMES = 11;


#ifdef __ANDROID__
// Fit 640×480 loadscreen chrome (no cap — stretch full). Dim underlay ~80%
// (ShadowRect ×2). Same center-scale family as Options / GIO so narrow presets
// (e.g. 934x480) expand to fill the screen instead of leaving the panel offset.
static float SlgUiScale(void)
{
	float const sx = (float)SCREEN_WIDTH  / 640.f;
	float const sy = (float)SCREEN_HEIGHT / 480.f;
	return std::min(sx, sy);
}
static INT16 SlgSX(INT32 x)
{
	float const sc = SlgUiScale();
	float const cx = (float)STD_SCREEN_X + 320.f;
	return (INT16)((float)(SCREEN_WIDTH / 2) + ((float)x - cx) * sc);
}
static INT16 SlgSY(INT32 y)
{
	float const sc = SlgUiScale();
	float const cy = (float)STD_SCREEN_Y + 240.f;
	return (INT16)((float)(SCREEN_HEIGHT / 2) + ((float)y - cy) * sc);
}
// Scale a delta; keep sign. Zero stays zero (not forced to 1).
static INT16 SlgS(INT32 v)
{
	if (v == 0) return 0;
	INT32 const s = (INT32)((float)v * SlgUiScale());
	if (v > 0) return (INT16)std::max(1, s);
	return (INT16)std::min(-1, s);
}
// ShadowRect multiplies light ≈0.48/pass → 2× ≈ 77% dark (~alpha 0.8).
#define SLG_DIM_PASSES 2
static SGPVSurface* guiSlgUnderlay = NULL;
#define SAVE_LOAD_NORMAL_FONT				FONT14ARIAL
#define SLG_BTN_FONT					FONT14HUMANIST
#else
#define SlgSX(x) ((INT16)(x))
#define SlgSY(y) ((INT16)(y))
#define SlgS(v)  ((INT16)(v))
#define SAVE_LOAD_NORMAL_FONT				FONT12ARIAL
#define SLG_BTN_FONT					OPT_BUTTON_FONT
#endif

#define SAVE_LOAD_NORMAL_COLOR				2//FONT_MCOLOR_DKWHITE//2//FONT_MCOLOR_WHITE
#define SAVE_LOAD_NORMAL_SHADOW_COLOR			118//121//118//125

#define SAVE_LOAD_EMPTYSLOT_COLOR			2//125//FONT_MCOLOR_WHITE
#define SAVE_LOAD_EMPTYSLOT_SHADOW_COLOR		121//118

#define SAVE_LOAD_HIGHLIGHTED_COLOR			FONT_MCOLOR_WHITE
#define SAVE_LOAD_HIGHLIGHTED_SHADOW_COLOR		2

#define SAVE_LOAD_SELECTED_COLOR			2//145//FONT_MCOLOR_WHITE
#define SAVE_LOAD_SELECTED_SHADOW_COLOR		130//2

#define SLG_SAVELOCATION_WIDTH				SlgS(575)
#define SLG_SAVELOCATION_HEIGHT			SlgS(30)//46
#define SLG_FIRST_SAVED_SPOT_X				SlgSX(STD_SCREEN_X + 17)
#define SLG_FIRST_SAVED_SPOT_Y				SlgSY(STD_SCREEN_Y + 49)
#define SLG_GAP_BETWEEN_LOCATIONS			SlgS(35)//47



#define SLG_DATE_OFFSET_X				SlgS(13)
#define SLG_DATE_OFFSET_Y				SlgS(11)

#define SLG_SECTOR_OFFSET_X				SlgS(95)//105//114
#define SLG_SECTOR_WIDTH				SlgS(98)

#define SLG_NUM_MERCS_OFFSET_X				SlgS(196)//190//SLG_DATE_OFFSET_X

#define SLG_BALANCE_OFFSET_X				SlgS(260)//SLG_SECTOR_OFFSET_X

#define SLG_SAVE_GAME_DESC_X				SlgS(318)//320//204
#define SLG_SAVE_GAME_DESC_Y				SLG_DATE_OFFSET_Y//SLG_DATE_OFFSET_Y + 7
#define SLG_SAVE_GAME_SKULL_X				SlgS(552)
#define SLG_SAVE_GAME_SKULL_Y				SlgS(-3)

#define SLG_TITLE_POS_X				SlgSX(STD_SCREEN_X)
#define SLG_TITLE_POS_Y				SlgSY(STD_SCREEN_Y)

#define SLG_LOAD_CANCEL_POS_X				SlgSX(329 + STD_SCREEN_X)
#define SLG_SAVE_LOAD_BTN_POS_X				SlgSX(123 + STD_SCREEN_X)
#define SLG_BTN_POS_Y					SlgSY(438 + STD_SCREEN_Y)

#define SLG_SCROLLBAR_POS_X (SLG_FIRST_SAVED_SPOT_X + SlgS(582))
#define SLG_SCROLLBAR_POS_Y (SLG_FIRST_SAVED_SPOT_Y)
#define SLG_SCROLLBAR_HEIGHT SlgS(378)
#define SLG_SCROLLBAR_WIDTH SlgS(23)
#define SLG_SCROLLBAR_BTN_HEIGHT SlgS(23)
#define SLG_SCROLLBAR_INDICATOR_HEIGHT SlgS(19)
#define SLG_SCROLLBAR_INNER_HEIGHT (SLG_SCROLLBAR_HEIGHT - 2 * SLG_SCROLLBAR_BTN_HEIGHT)
#define SLG_SCROLLBAR_INNER_POS_Y (SLG_SCROLLBAR_POS_Y + SLG_SCROLLBAR_BTN_HEIGHT)
#define SLG_SCROLLBAR_BTN_SCROLL_DOWN_POS_Y (SLG_SCROLLBAR_POS_Y + SLG_SCROLLBAR_HEIGHT - SLG_SCROLLBAR_BTN_HEIGHT)

#define SLG_SELECTED_SLOT_GRAPHICS_NUMBER		1
#define SLG_UNSELECTED_SLOT_GRAPHICS_NUMBER		0
#define SLG_SKULL_DEFAULT_GRAPHICS_NUMBER 2
#define SLG_SKULL_SELECTED_GRAPHICS_NUMBER 3
#define SLG_SKULL_HIGHLIGHTED_GRAPHICS_NUMBER 4

#define SLG_SCROLL_UP_GRAPHICS_NUMBER_UP 0
#define SLG_SCROLL_UP_GRAPHICS_NUMBER_DOWN 1
#define SLG_SCROLL_DOWN_GRAPHICS_NUMBER_UP 2
#define SLG_SCROLL_DOWN_GRAPHICS_NUMBER_DOWN 3
#define SLG_SCROLL_BAR_INNER_GRAPHICS_NUMBER 4
#define SLG_SCROLL_BAR_INDICATOR_GRAPHICS_NUMBER 5

//defines for saved game version status
enum
{
	SLS_HEADER_OK,
	SLS_SAVED_GAME_VERSION_OUT_OF_DATE,
	SLS_GAME_VERSION_OUT_OF_DATE,
	SLS_BOTH_SAVE_GAME_AND_GAME_VERSION_OUT_OF_DATE,
};

static BOOLEAN gfSaveLoadScreenEntry = TRUE;
static BOOLEAN gfSaveLoadScreenExit	= FALSE;
BOOLEAN        gfRedrawSaveLoadScreen = TRUE;

static ScreenID guiSaveLoadExitScreen = SAVE_LOAD_SCREEN;

static std::vector<SaveGameInfo> gSavedGamesList;
static int32_t gCurrentScrollTop = 0;
static INT32 gbSelectedSaveLocation = -1;
static INT32 gbHighLightedLocation  = -1;

static BOOLEAN gfDoingQuickLoad = FALSE;

//This flag is used to differentiate between loading a game and saving a game.
// gfSaveGame=TRUE		For saving a game
// gfSaveGame=FALSE		For loading a game
BOOLEAN		gfSaveGame=TRUE;

static BOOLEAN gfSaveLoadScreenButtonsCreated = FALSE;

static SGPVObject* guiSlgBackGroundImage;
static SGPVObject* guiSlgAddonsStracciatella;
static SGPVObject* guiSlgScrollbarStracciatella;

static BOOLEAN gfUserInTextInputMode = FALSE;
static UINT8   gubSaveGameNextPass   = 0;

static BOOLEAN gfStartedFadingOut = FALSE;


BOOLEAN		gfCameDirectlyFromGame = FALSE;


BOOLEAN		gfLoadedGame = FALSE;	//Used to know when a game has been loaded, the flag in gtacticalstatus might have been reset already

BOOLEAN		gfLoadGameUponEntry = FALSE;

static BOOLEAN gfHadToMakeBasementLevels = FALSE;


//
//Buttons
//
static BUTTON_PICS* guiSlgButtonImage;

// Cancel Button
static GUIButtonRef guiSlgCancelBtn;

// Save game Button
static BUTTON_PICS* guiSaveLoadImage;
static GUIButtonRef guiSlgSaveLoadBtn;
static GUIButtonRef guiSlgScrollUpBtn;
static GUIButtonRef guiSlgScrollDownBtn;

//Mouse regions for the currently selected save game
static MOUSE_REGION gSelectedSaveRegion[NUM_SAVE_GAMES];

static MOUSE_REGION gSLSEntireScreenRegion;


static void EnterSaveLoadScreen();
static void ExitSaveLoadScreen(void);
static void GetSaveLoadScreenUserInput(void);
static void RenderSaveLoadScreen(void);
static void RenderScrollBar(void);
static void SaveLoadSelectedSave();
static void SaveNewSave();


ScreenID SaveLoadScreenHandle()
{
	if( gfSaveLoadScreenEntry )
	{
		EnterSaveLoadScreen();
		gfSaveLoadScreenEntry = FALSE;
		gfSaveLoadScreenExit = FALSE;

		PauseGame();

		//save the new rect
#ifdef __ANDROID__
		// Full frame: scaled chrome can sit below y=439.
		BltVideoSurface(guiSAVEBUFFER, FRAME_BUFFER, 0, 0, NULL);
#else
		BlitBufferToBuffer(FRAME_BUFFER, guiSAVEBUFFER, 0, 0, SCREEN_WIDTH, 439);
#endif
	}

	RestoreBackgroundRects();

	//to guarentee that we do not accept input when we are fading out
	if( !gfStartedFadingOut )
	{
		GetSaveLoadScreenUserInput();
	}
	else
		gfRedrawSaveLoadScreen = FALSE;

	//if we have exited the save load screen, exit
	if( !gfSaveLoadScreenButtonsCreated )
		return( guiSaveLoadExitScreen );

	if( gfRedrawSaveLoadScreen )
	{
		RenderSaveLoadScreen();
		MarkButtonsDirty( );
		RenderButtons();

		gfRedrawSaveLoadScreen = FALSE;
	}
	RenderAllTextFields();

	if( gubSaveGameNextPass != 0 )
	{
		gubSaveGameNextPass++;

		if( gubSaveGameNextPass == 5 )
		{
			gubSaveGameNextPass = 0;
			SaveLoadSelectedSave();
		}
	}


	//If we are not exiting the screen, render the buttons
	if( !gfSaveLoadScreenExit && guiSaveLoadExitScreen == SAVE_LOAD_SCREEN )
	{
		// render buttons marked dirty
		RenderButtons( );
	}


	// ATE: Put here to save RECTS before any fast help being drawn...
	SaveBackgroundRects( );
	RenderFastHelp();

	if ( HandleFadeOutCallback( ) )
	{
		return( guiSaveLoadExitScreen );
	}

	if ( HandleBeginFadeOut( SAVE_LOAD_SCREEN ) )
	{
		return( SAVE_LOAD_SCREEN );
	}


	if( gfSaveLoadScreenExit )
	{
		ExitSaveLoadScreen();
	}

	if ( HandleFadeInCallback( ) )
	{
		// Re-render the scene!
		RenderSaveLoadScreen();
	}

	if ( HandleBeginFadeIn( SAVE_LOAD_SCREEN ) )
	{
	}

	return( guiSaveLoadExitScreen );
}


static void DestroySaveLoadTextInputBoxes(void);


static void SetSaveLoadExitScreen(ScreenID const uiScreen)
{
	if( uiScreen == GAME_SCREEN )
	{
		EnterTacticalScreen( );
	}

	gfSaveLoadScreenExit	= TRUE;

	guiSaveLoadExitScreen = uiScreen;

	SetPendingNewScreen( uiScreen );

	if( gfDoingQuickLoad )
	{
		fFirstTimeInGameScreen = TRUE;
		SetPendingNewScreen( uiScreen );
	}

	ExitSaveLoadScreen();

	DestroySaveLoadTextInputBoxes();
}


static void LeaveSaveLoadScreen()
{
	if (gfCameDirectlyFromGame)
	{
		SetSaveLoadExitScreen(guiPreviousOptionScreen);
	} else {
		switch (guiPreviousOptionScreen)
		{
			case MAINMENU_SCREEN: SetSaveLoadExitScreen(MAINMENU_SCREEN); break;
			case GAME_INIT_OPTIONS_SCREEN: SetSaveLoadExitScreen(GAME_INIT_OPTIONS_SCREEN); break;
			case INTRO_SCREEN: SetSaveLoadExitScreen(INTRO_SCREEN); break;
			default: SetSaveLoadExitScreen(OPTIONS_SCREEN);
		}
	}
}


static GUIButtonRef MakeButton(BUTTON_PICS* img, const ST::string& text, INT16 x, GUI_CALLBACK click)
{
	GUIButtonRef const btn = CreateIconAndTextButton(img, text, SLG_BTN_FONT, OPT_BUTTON_ON_COLOR, DEFAULT_SHADOW, OPT_BUTTON_OFF_COLOR, DEFAULT_SHADOW, x, SLG_BTN_POS_Y, MSYS_PRIORITY_HIGH, click);
#ifdef __ANDROID__
	// Stretch art to scaled hit (Button_System Android path when W/H > pic).
	if (ButtonDimensions const* const d = GetDimensionsOfButtonPic(img))
	{
		float const sc = SlgUiScale();
		INT32 const dw = (INT32)(d->w * sc);
		INT32 const dh = (INT32)(d->h * sc);
		btn->Area.RegionBottomRightX = btn->Area.RegionTopLeftX + dw;
		btn->Area.RegionBottomRightY = btn->Area.RegionTopLeftY + dh;
		if (btn->Area.RegionBottomRightY > (INT32)SCREEN_HEIGHT)
		{
			INT32 const shift = btn->Area.RegionBottomRightY - (INT32)SCREEN_HEIGHT;
			btn->Area.RegionTopLeftY     -= shift;
			btn->Area.RegionBottomRightY -= shift;
		}
	}
#endif
	return btn;
}

#ifdef __ANDROID__
// 8bpp STI → 16bpp temp → stretch to display rect (Options/GIO pattern).
static void SlgStretchVO(SGPVObject* vo, UINT16 sub, INT16 dx, INT16 dy, UINT16 dw, UINT16 dh)
{
	if (!vo || dw == 0 || dh == 0) return;
	ETRLEObject const& e = vo->SubregionProperties(sub);
	if (e.usWidth == 0 || e.usHeight == 0) return;
	SGPVSurface tmp(e.usWidth, e.usHeight, 16);
	SGPRect tmpClip;
	tmpClip.set(0, 0, tmp.Width(), tmp.Height());
	SGPRect const oldClip = SetClippingRect(tmpClip);
	BltVideoObject(&tmp, vo, sub, 0, 0);
	SetClippingRect(oldClip);
	SGPBox src;
	src.set(0, 0, e.usWidth, e.usHeight);
	SGPBox dst;
	dst.set((UINT16)dx, (UINT16)dy, dw, dh);
	BltStretchVideoSurface(FRAME_BUFFER, &tmp, &src, &dst);
}

static void SlgStretchVOAtNatural(SGPVObject* vo, UINT16 sub, INT32 natX, INT32 natY)
{
	if (!vo) return;
	ETRLEObject const& e = vo->SubregionProperties(sub);
	if (e.usWidth == 0 || e.usHeight == 0) return;
	float const sc = SlgUiScale();
	SlgStretchVO(vo, sub, SlgSX(natX), SlgSY(natY),
		(UINT16)std::max(1, (INT32)(e.usWidth * sc)),
		(UINT16)std::max(1, (INT32)(e.usHeight * sc)));
}

static void SlgEnlargeButtonHit(GUIButtonRef btn)
{
	if (!btn) return;
	float const sc = SlgUiScale();
	INT32 const w = btn->Area.RegionBottomRightX - btn->Area.RegionTopLeftX;
	INT32 const h = btn->Area.RegionBottomRightY - btn->Area.RegionTopLeftY;
	btn->Area.RegionBottomRightX = btn->Area.RegionTopLeftX + (INT32)(w * sc);
	btn->Area.RegionBottomRightY = btn->Area.RegionTopLeftY + (INT32)(h * sc);
}
#endif

static void BtnScrollUpCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnScrollDownCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnSlgCancelCallback(GUI_BUTTON* btn, UINT32 reason);
static void BtnSlgSaveLoadCallback(GUI_BUTTON* btn, UINT32 reason);
static void ClearSelectedSaveSlot(void);
static void InitSaveGameArray(void);
static void SelectedSLSEntireRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason);
static void SelectedSaveRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason);
static void SelectedSaveRegionMovementCallBack(MOUSE_REGION* pRegion, UINT32 reason);
static void StartFadeOutForSaveLoadScreen(void);

static void EnterSaveLoadScreen()
{
	// Display Dead Is Dead games for saving by default if we are to choose the Dead is Dead Slot
	if (guiPreviousOptionScreen == GAME_INIT_OPTIONS_SCREEN)
	{
		gfSaveGame = TRUE;
	}

	// This is a hack to get sector names, but if the underground sector is NOT loaded
	if (!gpUndergroundSectorInfoHead)
	{
		BuildUndergroundSectorInfoList();
		gfHadToMakeBasementLevels = TRUE;
	}
	else
	{
		gfHadToMakeBasementLevels = FALSE;
	}

	guiSaveLoadExitScreen = SAVE_LOAD_SCREEN;
	InitSaveGameArray();
	EmptyBackgroundRects();

	// If the user has asked to load the selected save
	if (gfLoadGameUponEntry)
	{
		// Make sure the save is valid
		INT8 const last_slot = gGameSettings.bLastSavedGameSlot;
		if (last_slot != -1 && gSavedGamesList.begin() + last_slot < gSavedGamesList.end())
		{
			gbSelectedSaveLocation = last_slot;
			StartFadeOutForSaveLoadScreen();
		}
		else
		{ // else the save is not valid, so do not load it
			gfLoadGameUponEntry = FALSE;
		}
	}

	// Load main background and add ons graphic
	guiSlgBackGroundImage = AddVideoObjectFromFile(INTERFACEDIR "/loadscreen.sti");
	guiSlgAddonsStracciatella = AddVideoObjectFromFile("sti/interface/save-load-addons.sti");
	guiSlgScrollbarStracciatella = AddVideoObjectFromFile("sti/interface/scroll-bar.sti");

#ifdef __ANDROID__
	// Capture Options (or prior screen) for dim underlay each frame.
	if (guiSlgUnderlay)
	{
		DeleteVideoSurface(guiSlgUnderlay);
		guiSlgUnderlay = NULL;
	}
	guiSlgUnderlay = AddVideoSurface(SCREEN_WIDTH, SCREEN_HEIGHT, PIXEL_DEPTH);
	BltVideoSurface(guiSlgUnderlay, FRAME_BUFFER, 0, 0, NULL);
#endif

	guiSlgScrollUpBtn = QuickCreateButtonImg("sti/interface/scroll-bar.sti", SLG_SCROLL_UP_GRAPHICS_NUMBER_UP, SLG_SCROLL_UP_GRAPHICS_NUMBER_DOWN, SLG_SCROLLBAR_POS_X, SLG_SCROLLBAR_POS_Y, MSYS_PRIORITY_HIGH, BtnScrollUpCallback);
	guiSlgScrollUpBtn->SetFastHelpText("Scroll up");
	guiSlgScrollUpBtn->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_HATCHED);
	guiSlgScrollDownBtn = QuickCreateButtonImg("sti/interface/scroll-bar.sti", SLG_SCROLL_DOWN_GRAPHICS_NUMBER_UP, SLG_SCROLL_DOWN_GRAPHICS_NUMBER_DOWN, SLG_SCROLLBAR_POS_X, SLG_SCROLLBAR_BTN_SCROLL_DOWN_POS_Y, MSYS_PRIORITY_HIGH, BtnScrollDownCallback);
	guiSlgScrollDownBtn->SetFastHelpText("Scroll down");
	guiSlgScrollDownBtn->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_HATCHED);
#ifdef __ANDROID__
	SlgEnlargeButtonHit(guiSlgScrollUpBtn);
	SlgEnlargeButtonHit(guiSlgScrollDownBtn);
#endif

	guiSlgButtonImage = LoadButtonImage(INTERFACEDIR "/loadscreenaddons.sti", 6, 9);
	guiSlgCancelBtn   = MakeButton(guiSlgButtonImage, zSaveLoadText[SLG_CANCEL], SLG_LOAD_CANCEL_POS_X, BtnSlgCancelCallback);
	// Either the save or load button
	INT32          gfx;
	ST::string text;
	if (gfSaveGame)
	{
		gfx  = 5;
		text = zSaveLoadText[SLG_SAVE_GAME];
	}
	else
	{
		gfx  = 4;
		text = zSaveLoadText[SLG_LOAD_GAME];
	}
	guiSaveLoadImage  = UseLoadedButtonImage(guiSlgButtonImage, gfx, gfx + 3);
	guiSlgSaveLoadBtn = MakeButton(guiSaveLoadImage, text, SLG_SAVE_LOAD_BTN_POS_X, BtnSlgSaveLoadCallback);
	guiSlgSaveLoadBtn->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_HATCHED);

	UINT16 const x = SLG_FIRST_SAVED_SPOT_X;
	UINT16       y = SLG_FIRST_SAVED_SPOT_Y;
	for (INT8 i = 0; i != NUM_SAVE_GAMES; ++i)
	{
		MOUSE_REGION& r = gSelectedSaveRegion[i];
		MSYS_DefineRegion(&r, x, y, x + SLG_SAVELOCATION_WIDTH, y + SLG_SAVELOCATION_HEIGHT, MSYS_PRIORITY_HIGH, CURSOR_NORMAL, SelectedSaveRegionMovementCallBack, SelectedSaveRegionCallBack);
		MSYS_SetRegionUserData(&r, 0, i);

		y += SLG_GAP_BETWEEN_LOCATIONS;
	}

	// Create the screen mask to enable ability to right click to cancel the save game
	MSYS_DefineRegion(&gSLSEntireScreenRegion, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, MSYS_PRIORITY_HIGH - 10, CURSOR_NORMAL, MSYS_NO_CALLBACK, SelectedSLSEntireRegionCallBack);

	ClearSelectedSaveSlot();

	RemoveMouseRegionForPauseOfClock();

	gbHighLightedLocation  = -1;
	// Select the first, which is the last updated, item by default
	gbSelectedSaveLocation = gSavedGamesList.size() > 0 ? 0 : -1;
	if (!gGameSettings.sCurrentSavedGameName.empty()) {
		for (auto i = gSavedGamesList.begin(); i < gSavedGamesList.end(); i++) {
			// If a current save name is used select it
			if ((*i).name() == gGameSettings.sCurrentSavedGameName) {
				gbSelectedSaveLocation = std::distance(gSavedGamesList.begin(), i);
				break;
			}
		}
	}

	EnableButton(guiSlgSaveLoadBtn, gbSelectedSaveLocation != -1);
	// Mark all buttons dirty, required for redrawing with the Tab system
	guiSlgCancelBtn->uiFlags |= BUTTON_DIRTY;

	RenderSaveLoadScreen();

	// Save load buttons are created
	gfSaveLoadScreenButtonsCreated = TRUE;

	gfDoingQuickLoad   = FALSE;
	gfStartedFadingOut = FALSE;

	DisableScrollMessages();

	gfLoadedGame = FALSE;

	if (gfLoadGameUponEntry)
	{
		guiSlgCancelBtn->uiFlags   |= BUTTON_FORCE_UNDIRTY;
		guiSlgSaveLoadBtn->uiFlags |= BUTTON_FORCE_UNDIRTY;
		FRAME_BUFFER->Fill(0);
	}

	gfGettingNameFromSaveLoadScreen = FALSE;
}


static void ExitSaveLoadScreen(void)
{
	gfLoadGameUponEntry = FALSE;

	if( !gfSaveLoadScreenButtonsCreated )
		return;

	gfSaveLoadScreenExit = FALSE;
	gfSaveLoadScreenEntry = TRUE;

	UnloadButtonImage( guiSlgButtonImage );

	RemoveButton( guiSlgScrollUpBtn );
	RemoveButton( guiSlgScrollDownBtn );
	RemoveButton( guiSlgCancelBtn );

	//Remove the save / load button
//	if( !gfSaveGame )
	{
		RemoveButton( guiSlgSaveLoadBtn );
		UnloadButtonImage( guiSaveLoadImage );
	}

	RemoveRegions(gSelectedSaveRegion);

	DeleteVideoObject(guiSlgBackGroundImage);
	RemoveVObject(MLG_LOADSAVEHEADER);
	DeleteVideoObject(guiSlgAddonsStracciatella);
	DeleteVideoObject(guiSlgScrollbarStracciatella);
#ifdef __ANDROID__
	if (guiSlgUnderlay)
	{
		DeleteVideoSurface(guiSlgUnderlay);
		guiSlgUnderlay = NULL;
	}
#endif

	//Destroy the text fields ( if created )
	DestroySaveLoadTextInputBoxes();

	MSYS_RemoveRegion( &gSLSEntireScreenRegion );

	gfSaveLoadScreenEntry = TRUE;
	gfSaveLoadScreenExit = FALSE;

	if( !gfLoadedGame )
	{
		UnLockPauseState( );
		UnPauseGame();
	}

	gfSaveLoadScreenButtonsCreated = FALSE;

	gfCameDirectlyFromGame = FALSE;

	//unload the basement sectors
	if( gfHadToMakeBasementLevels )
		TrashUndergroundSectorInfo();

	gfGettingNameFromSaveLoadScreen = FALSE;
}


static void DisplaySaveGameList(void);


static void RenderSaveLoadScreen(void)
{
	// If we are going to be instantly leaving the screen, don't draw the numbers
	if (gfLoadGameUponEntry) return;

#ifdef __ANDROID__
	// Dim prior screen ~80% (ShadowRect ×2 ≈ 0.48² remaining light).
	if (guiSlgUnderlay)
	{
		BltVideoSurface(FRAME_BUFFER, guiSlgUnderlay, 0, 0, NULL);
	}
	else
	{
		FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));
	}
	for (int i = 0; i < SLG_DIM_PASSES; ++i)
	{
		FRAME_BUFFER->ShadowRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}
	// Stretch base loadscreen + title header from natural STI.
	SlgStretchVOAtNatural(guiSlgBackGroundImage, 0, STD_SCREEN_X, STD_SCREEN_Y);
	UINT16 const gfx = gfSaveGame ? 1 : 0;
	if (SGPVObject* const header = GetVObject(MLG_LOADSAVEHEADER))
	{
		SlgStretchVOAtNatural(header, gfx, STD_SCREEN_X, STD_SCREEN_Y);
	}
#else
	BltVideoObject(FRAME_BUFFER, guiSlgBackGroundImage, 0, STD_SCREEN_X, STD_SCREEN_Y);

	// Display the Title
	UINT16 const gfx = gfSaveGame ? 1 : 0;
	BltVideoObject(FRAME_BUFFER, MLG_LOADSAVEHEADER, gfx, SLG_TITLE_POS_X, SLG_TITLE_POS_Y);
#endif

	RenderScrollBar();
	DisplaySaveGameList();
	InvalidateScreen();
}

static int32_t ScrollPositionTopMax() {
	return std::max(0, int32_t(gSavedGamesList.size()) - NUM_SAVE_GAMES);
}

static void RenderScrollBar(void) {
	SGPRect	clippingRect;
	clippingRect.set(SLG_SCROLLBAR_POS_X, SLG_SCROLLBAR_INNER_POS_Y, SLG_SCROLLBAR_POS_X + SLG_SCROLLBAR_WIDTH, SLG_SCROLLBAR_INNER_POS_Y + SLG_SCROLLBAR_INNER_HEIGHT);
	SGPRect const previousClippingRect = SetClippingRect(clippingRect);

	auto tileHeightNat = guiSlgScrollbarStracciatella->SubregionProperties(SLG_SCROLL_BAR_INNER_GRAPHICS_NUMBER).usHeight;
#ifdef __ANDROID__
	UINT16 const tileHeight = (UINT16)std::max(1, (INT32)(tileHeightNat * SlgUiScale()));
	UINT16 const barW = SLG_SCROLLBAR_WIDTH;
#else
	UINT16 const tileHeight = tileHeightNat;
	UINT16 const barW = 0; // unused
	(void)barW;
#endif
	auto repetitions = uint32_t(ceil(double(SLG_SCROLLBAR_INNER_HEIGHT) / double(tileHeight)));
	for (uint32_t i = 0; i < repetitions; i++) {
#ifdef __ANDROID__
		SlgStretchVO(guiSlgScrollbarStracciatella, SLG_SCROLL_BAR_INNER_GRAPHICS_NUMBER,
			SLG_SCROLLBAR_POS_X, SLG_SCROLLBAR_INNER_POS_Y + i * tileHeight, barW, tileHeight);
#else
		BltVideoObject(FRAME_BUFFER, guiSlgScrollbarStracciatella, SLG_SCROLL_BAR_INNER_GRAPHICS_NUMBER, SLG_SCROLLBAR_POS_X, SLG_SCROLLBAR_INNER_POS_Y + i * tileHeight);
#endif
	}
	SetClippingRect(previousClippingRect);

	auto maxTop = std::max(1, ScrollPositionTopMax());
	auto currentTop = gCurrentScrollTop;
	auto maxYPos = SLG_SCROLLBAR_INNER_HEIGHT - SLG_SCROLLBAR_INDICATOR_HEIGHT - 2;
	auto indicatorPosition = int(round(double_t(maxYPos) * double_t(currentTop) / double_t(maxTop)));
	indicatorPosition = std::clamp(indicatorPosition, 0, maxYPos);

#ifdef __ANDROID__
	{
		ETRLEObject const& ind = guiSlgScrollbarStracciatella->SubregionProperties(SLG_SCROLL_BAR_INDICATOR_GRAPHICS_NUMBER);
		UINT16 const iw = (UINT16)std::max(1, (INT32)(ind.usWidth  * SlgUiScale()));
		UINT16 const ih = (UINT16)std::max(1, (INT32)(ind.usHeight * SlgUiScale()));
		SlgStretchVO(guiSlgScrollbarStracciatella, SLG_SCROLL_BAR_INDICATOR_GRAPHICS_NUMBER,
			SLG_SCROLLBAR_POS_X + SlgS(2), SLG_SCROLLBAR_INNER_POS_Y + indicatorPosition + SlgS(1), iw, ih);
	}
#else
	BltVideoObject(FRAME_BUFFER, guiSlgScrollbarStracciatella, SLG_SCROLL_BAR_INDICATOR_GRAPHICS_NUMBER, SLG_SCROLLBAR_POS_X + 2, SLG_SCROLLBAR_INNER_POS_Y + indicatorPosition + 1);
#endif
}


static ST::string GetGameDescription()
{
	INT8 const id = GetActiveFieldID();
	if (id <= 0) return {};

	return GetStringFromField(id);
}


static BOOLEAN DisplaySaveGameEntry(const std::vector<SaveGameInfo>::iterator& entry);
static void MoveSelectionDown();
static void MoveSelectionUp();
static void ConfirmDeleteSavedGameCallBack(MessageBoxReturnValue const bExitValue);
static void InitSaveLoadScreenTextInputBoxes(void);


static void GetSaveLoadScreenUserInput(void)
{
	// If we are going to be instantly leaving the screen, dont draw the numbers
	if (gfLoadGameUponEntry) return;

	InputAtom e;
	while (DequeueSpecificEvent(&e, KEYBOARD_EVENTS))
	{
		if (HandleTextInput(&e)) continue;

		if (e.usEvent == KEY_REPEAT || e.usEvent == KEY_DOWN) {
			switch (e.usParam)
			{
				case SDLK_UP:   MoveSelectionUp();   break;
				case SDLK_DOWN: MoveSelectionDown(); break;
			}
		}
		if (e.usEvent == KEY_UP)
		{
			switch (e.usParam)
			{
				case SDLK_ESCAPE:
					if (gbSelectedSaveLocation == -1)
					{
						LeaveSaveLoadScreen();
					}
					else
					{ // Reset selected slot
						gbSelectedSaveLocation = -1;
						gfRedrawSaveLoadScreen = TRUE;
						DestroySaveLoadTextInputBoxes();
						DisableButton(guiSlgSaveLoadBtn);
					}
					break;

				case SDLK_RETURN:
					if (gfSaveGame && gbSelectedSaveLocation == 0)
					{
						if (gfUserInTextInputMode) {
							SaveNewSave();
						} else {
							InitSaveLoadScreenTextInputBoxes();
						}
					}
					else if (gbSelectedSaveLocation != -1)
					{
						SaveLoadSelectedSave();
					}
					else
					{
						gfRedrawSaveLoadScreen = TRUE;
					}
					break;

				case SDLK_DELETE:
					if (gbSelectedSaveLocation >= 0) {
						auto& save = *(gSavedGamesList.begin() + gbSelectedSaveLocation);
						auto isNewSave = save.name().empty();

						if (!isNewSave) {
							auto msg = st_format_printf(zSaveLoadText[SLG_CONFIRM_DELETE], save.header().sSavedGameDesc);
							DoSaveLoadMessageBox(msg, SAVE_LOAD_SCREEN, MSG_BOX_FLAG_YESNO, ConfirmDeleteSavedGameCallBack);
						}
					}
					break;
			}
		}
	}
}


static UINT8 CompareSaveGameVersion(INT32 bSaveGameID);
static bool AreModsEqualToEnabled(INT32 bSaveGameID);
static void ConfirmSavedGameMessageBoxCallBack(MessageBoxReturnValue);
static void LoadSavedGameWarningMessageBoxCallBack(MessageBoxReturnValue);
static void DoSaveGame(const ST::string &saveName, const ST::string &saveDescription);


static void SaveNewSave() {
	time_t now;
    time(&now);
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
	auto description = GetGameDescription();
	// Building the filename from date and description should never lead to conflicts
	// We need to sanitize the filename afterwards
	auto filename = FileMan::cleanBasename(ST::format("{}-{}", buf, description.to_lower()));

	DoSaveGame(filename, description);
}

static void SaveLoadSelectedSave()
{
	if (gbSelectedSaveLocation < 0) {
		return;
	}
	if (gfSaveGame && gbSelectedSaveLocation == 0) {
		return;
	}

	if (gfSaveGame)
	{
		auto& saveGameInfo = (*(gSavedGamesList.begin() + gbSelectedSaveLocation));
		ST::string sText = st_format_printf(zSaveLoadText[SLG_CONFIRM_SAVE], saveGameInfo.header().sSavedGameDesc);
		DoSaveLoadMessageBox(sText, SAVE_LOAD_SCREEN, MSG_BOX_FLAG_YESNO, ConfirmSavedGameMessageBoxCallBack);
	}
	else
	{
		// Check to see if the save game headers are the same
		auto versionResult = CompareSaveGameVersion(gbSelectedSaveLocation);
		auto modsEqual = AreModsEqualToEnabled(gbSelectedSaveLocation);

		// ± seems to cause proper line breaks
		auto msg = ST::format("{}±", zSaveLoadText[SLG_SAVED_GAME_ISSUE]);
		auto showMsg = false;

		if (versionResult != SLS_HEADER_OK)
		{
			showMsg = true;
			msg += ST::format("- {}±", versionResult == SLS_GAME_VERSION_OUT_OF_DATE ? zSaveLoadText[SLG_GAME_VERSION_DIF] : zSaveLoadText[SLG_SAVED_GAME_VERSION_DIF]);
		}
		if (!modsEqual) {
			showMsg = true;
			msg += ST::format("- {}±", zSaveLoadText[SLG_SAVED_GAME_MODS_DIF]);
		}
		msg += zSaveLoadText[SLG_SAVED_GAME_CONTINUE_ANYWAYS];
		if (showMsg) {
			DoSaveLoadMessageBox(msg, SAVE_LOAD_SCREEN, MSG_BOX_FLAG_YESNO, LoadSavedGameWarningMessageBoxCallBack);
		}
		else
		{
			StartFadeOutForSaveLoadScreen();
		}
	}
}

void DoSaveLoadMessageBoxWithRect(const ST::string& str, ScreenID uiExitScreen, MessageBoxFlags usFlags, MSGBOX_CALLBACK ReturnCallback, SGPBox const* centering_rect)
{
	// do message box and return
	DoMessageBox(MSG_BOX_BASIC_STYLE, str, uiExitScreen, usFlags, ReturnCallback, centering_rect);
}


void DoSaveLoadMessageBox(const ST::string& str, ScreenID uiExitScreen, MessageBoxFlags usFlags, MSGBOX_CALLBACK ReturnCallback)
{
	DoSaveLoadMessageBoxWithRect(str, uiExitScreen, usFlags, ReturnCallback, NULL);
}

bool compareSaveGames(const SaveGameInfo& i, const SaveGameInfo& j) {
	auto lastModifiedI = GCM->saveGameFiles()->getLastModifiedTime(GetSaveGamePath(i.name()));
	auto lastModifiedJ = GCM->saveGameFiles()->getLastModifiedTime(GetSaveGamePath(j.name()));
	return (lastModifiedI > lastModifiedJ);
}

std::vector<SaveGameInfo> GetValidSaveGames()
{
	auto savegameNames = GCM->saveGameFiles()->findAllFilesInDir("", false, false, true);
	std::vector<SaveGameInfo> validSaves;

	for (auto i = savegameNames.begin(); i < savegameNames.end(); i++) {
		if (!HasSaveGameExtension(*i)) {
			// Ignore non savegame files in save directory
			continue;
		}
		auto saveName = FileMan::getFileNameWithoutExt(*i);
		AutoSGPFile file(GCM->saveGameFiles()->openForReading(*i));
		try {
			validSaves.push_back(SaveGameInfo(saveName, file));
		} catch (const std::runtime_error &ex) {
			SLOGW("Could not read save game info for file `{}`: {}", *i, ex.what());
			continue;
		}
	}

	return validSaves;
}

static void InitSaveGameArray(void)
{
	auto validSaveGames = GetValidSaveGames();

	gSavedGamesList.clear();
	for (auto i = validSaveGames.begin(); i < validSaveGames.end(); i++) {
		if (gfSaveGame && (IsAutoSaveName((*i).name()) || IsQuickSaveName((*i).name()))) {
			// Dont display quick- and autosaves when saving game
			continue;
		}
		gSavedGamesList.push_back(std::move(*i));
	}

	std::sort(gSavedGamesList.begin(), gSavedGamesList.end(), compareSaveGames);
	if (gfSaveGame) {
		// Insert empty value at the beginning to create a new save
		gSavedGamesList.insert(gSavedGamesList.begin(), SaveGameInfo());
	}
}


static void DisplaySaveGameList(void)
{
	auto start = gSavedGamesList.begin() + gCurrentScrollTop;
	auto end = gSavedGamesList.begin() + std::min(size_t(gCurrentScrollTop + NUM_SAVE_GAMES), gSavedGamesList.size());
	for (auto i = start; i < end; ++i)
	{ // Display all the information from the header
		DisplaySaveGameEntry(i);
	}
}


static BOOLEAN DisplaySaveGameEntry(const std::vector<SaveGameInfo>::iterator& entry)
{
	if (entry < gSavedGamesList.begin() || entry >= gSavedGamesList.end()) return TRUE;
	// If we are going to be instantly leaving the screen, dont draw the numbers
	if (gfLoadGameUponEntry) return TRUE;
	// If we are currently fading out, leave
	if (gfStartedFadingOut) return TRUE;

	auto start = gSavedGamesList.begin() + gCurrentScrollTop;
	auto index = std::distance(gSavedGamesList.begin(), entry);
	auto indexFromScrollTop = std::distance(start, entry);

	auto isNewSave = (*entry).name().empty();
	auto isSelected = index == gbSelectedSaveLocation;

	UINT16 const bx = SLG_FIRST_SAVED_SPOT_X;
	UINT16 const by = SLG_FIRST_SAVED_SPOT_Y + SLG_GAP_BETWEEN_LOCATIONS * indexFromScrollTop;

	// Background
	UINT16 const gfx = isSelected ?
		SLG_SELECTED_SLOT_GRAPHICS_NUMBER : SLG_UNSELECTED_SLOT_GRAPHICS_NUMBER;
#ifdef __ANDROID__
	SlgStretchVO(guiSlgAddonsStracciatella, gfx, bx, by, SLG_SAVELOCATION_WIDTH, SLG_SAVELOCATION_HEIGHT);
#else
	BltVideoObject(FRAME_BUFFER, guiSlgAddonsStracciatella, gfx, bx, by);
#endif

	SGPFont  font = SAVE_LOAD_NORMAL_FONT;
	UINT8 foreground;
	UINT8 shadow;
	if (gfSaveGame && isNewSave) {
		// The new save game slot
		foreground = SAVE_LOAD_EMPTYSLOT_COLOR;
		shadow     = SAVE_LOAD_EMPTYSLOT_SHADOW_COLOR;
	}
	else if (isSelected)
	{ // The currently selected location
		foreground = SAVE_LOAD_SELECTED_COLOR;
		shadow     = SAVE_LOAD_SELECTED_SHADOW_COLOR;
	}
	else if (indexFromScrollTop == gbHighLightedLocation)
	{ // The highlighted slot
		foreground = SAVE_LOAD_HIGHLIGHTED_COLOR;
		shadow     = SAVE_LOAD_HIGHLIGHTED_SHADOW_COLOR;
	}
	else
	{ // The file exists
		foreground = SAVE_LOAD_NORMAL_COLOR;
		shadow     = SAVE_LOAD_NORMAL_SHADOW_COLOR;
	}
	SetFontShadow(shadow);

	if (isNewSave) {
		if (!gfUserInTextInputMode) {
			// If this is the new save slot
			DrawTextToScreen(pMessageStrings[MSG_NEW_SAVE], bx, by + SLG_DATE_OFFSET_Y, SLG_SAVELOCATION_WIDTH, font, foreground, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
		}
	} else {
		auto &header = (*entry).header();
		auto &mods = (*entry).mods();

		UINT16 x = bx;
		UINT16 y = by + SLG_DATE_OFFSET_Y;
		if (isSelected)
		{ // This is the currently selected location, move the text up a bit
			x++;
			y--;
		}

		MOUSE_REGION& region = gSelectedSaveRegion[indexFromScrollTop];
		if (!gfSaveGame) {
			ST::string difficulty = ST::format("{} {}", gzGIOScreenText[GIO_EASY_TEXT + header.sInitialGameOptions.ubDifficultyLevel - 1], zSaveLoadText[SLG_DIFF]);
			UINT8 gameModeText;
			switch (header.sInitialGameOptions.ubGameSaveMode)
			{
				case DIF_IRON_MAN: gameModeText = GIO_IRON_MAN_TEXT; break;
				case DIF_DEAD_IS_DEAD: gameModeText = GIO_DEAD_IS_DEAD_TEXT; break;
				default: gameModeText = GIO_SAVE_ANYWHERE_TEXT;
			}
			ST::string modsText = ST::format("{} ", zSaveLoadText[SLG_MODS]);
			if (mods.size() == 0) {
				modsText = zSaveLoadText[SLG_NO_MODS];
			} else {
				auto i = 0;
				for (auto &mod : mods) {
					modsText += ST::format("{}{} ({})", i == 0 ? "" : ", ", mod.first, mod.second);
					i++;
				}
			}
			ST::string options = ST::format("{}\n{}\n{}\n{}\n{}",
				difficulty,
				gzGIOScreenText[gameModeText],
				header.sInitialGameOptions.fGunNut      ? zSaveLoadText[SLG_ADDITIONAL_GUNS] : zSaveLoadText[SLG_NORMAL_GUNS],
				header.sInitialGameOptions.fSciFi       ? zSaveLoadText[SLG_SCIFI]           : zSaveLoadText[SLG_REALISTIC],
				modsText
			);

			region.SetFastHelpText(options);
		} else {
			region.SetFastHelpText({});
		}

		// Display the Saved game information
		// The date
		ST::string date = ST::format("{} {}, {02d}:{02d}", pMessageStrings[MSG_DAY], header.uiDay, header.ubHour, header.ubMin);
		DrawTextToScreen(date, x + SLG_DATE_OFFSET_X, y, 0, font, foreground, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		// The sector
		ST::string location;
		if (header.sSector.IsValid())
		{
			gfGettingNameFromSaveLoadScreen = TRUE;
			location = GetSectorIDString(header.sSector, FALSE);
			gfGettingNameFromSaveLoadScreen = FALSE;
		}
		else if (header.uiDay * NUM_SEC_IN_DAY + header.ubHour * NUM_SEC_IN_HOUR + header.ubMin * NUM_SEC_IN_MIN <= STARTING_TIME)
		{
			location = gpStrategicString[STR_PB_NOTAPPLICABLE_ABBREVIATION];
		}
		else
		{
			location = gzLateLocalizedString[STR_LATE_14];
		}
		location = ReduceStringLength(location, SLG_SECTOR_WIDTH, font);
		DrawTextToScreen(location, x + SLG_SECTOR_OFFSET_X, y, 0, font, foreground, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		// Number of mercs on the team
		// If only 1 merc is on the team use "merc" else "mercs"
		UINT8          const n_mercs = header.ubNumOfMercsOnPlayersTeam;
		ST::string merc = n_mercs == 1 ?
			MercAccountText[MERC_ACCOUNT_MERC] :
			pMessageStrings[MSG_MERCS];
		ST::string merc_count = ST::format("{} {}", n_mercs, merc);
		DrawTextToScreen(merc_count, x + SLG_NUM_MERCS_OFFSET_X, y, 0, font, foreground, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		// The balance
		DrawTextToScreen(SPrintMoney(header.iCurrentBalance), x + SLG_BALANCE_OFFSET_X, y, 0, font, foreground, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		if (!(gfSaveGame && gfUserInTextInputMode && isSelected))
		{
			// The saved game description
			DrawTextToScreen(header.sSavedGameDesc, x + SLG_SAVE_GAME_DESC_X, y, 0, font, foreground, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		}

		if (header.sInitialGameOptions.ubGameSaveMode == DIF_DEAD_IS_DEAD) {
				UINT16 gfx = SLG_SKULL_DEFAULT_GRAPHICS_NUMBER;
				if (indexFromScrollTop == gbHighLightedLocation) {
					gfx = SLG_SKULL_HIGHLIGHTED_GRAPHICS_NUMBER;
				}
				if (isSelected) {
					gfx = SLG_SKULL_SELECTED_GRAPHICS_NUMBER;
				}
#ifdef __ANDROID__
				{
					ETRLEObject const& sk = guiSlgAddonsStracciatella->SubregionProperties(gfx);
					UINT16 const sw = (UINT16)std::max(1, (INT32)(sk.usWidth  * SlgUiScale()));
					UINT16 const sh = (UINT16)std::max(1, (INT32)(sk.usHeight * SlgUiScale()));
					SlgStretchVO(guiSlgAddonsStracciatella, gfx, x + SLG_SAVE_GAME_SKULL_X, y + SLG_SAVE_GAME_SKULL_Y, sw, sh);
				}
#else
				BltVideoObject(FRAME_BUFFER, guiSlgAddonsStracciatella, gfx, x + SLG_SAVE_GAME_SKULL_X, y + SLG_SAVE_GAME_SKULL_Y);
#endif
		}
	}

	// Reset the shadow color
	SetFontShadow(DEFAULT_SHADOW);

	InvalidateRegion(bx, by, bx + SLG_SAVELOCATION_WIDTH, by + SLG_SAVELOCATION_HEIGHT);
	return TRUE;
}

static void ScrollUp() {
	if (gCurrentScrollTop != 0 && !gfUserInTextInputMode) {
		gCurrentScrollTop = gCurrentScrollTop - 1;
		gfRedrawSaveLoadScreen = true;
	}
}

static void ScrollDown() {
	auto nextScrollTop = std::min(gCurrentScrollTop + 1, ScrollPositionTopMax());
	if (nextScrollTop != gCurrentScrollTop && !gfUserInTextInputMode) {
		gCurrentScrollTop = nextScrollTop;
		gfRedrawSaveLoadScreen = true;
	}
}

static void HandleScrollEvent(INT32 const reason) {
	if (reason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		ScrollUp();
	}
	if (reason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		ScrollDown();
	}
}

static void BtnScrollUpCallback(GUI_BUTTON *, UINT32 reason) {
	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN || reason & MSYS_CALLBACK_REASON_POINTER_REPEAT) {
		ScrollUp();
	}
}

static void BtnScrollDownCallback(GUI_BUTTON *, UINT32 reason) {
	if (reason & MSYS_CALLBACK_REASON_POINTER_DWN || reason & MSYS_CALLBACK_REASON_POINTER_REPEAT) {
		ScrollDown();
	}
}

static void BtnSlgCancelCallback(GUI_BUTTON *, UINT32 const reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		LeaveSaveLoadScreen();
	}
	HandleScrollEvent(reason);
}


static void BtnSlgSaveLoadCallback(GUI_BUTTON *, UINT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		if (gfSaveGame && gbSelectedSaveLocation == 0 && gfUserInTextInputMode) {
			SaveNewSave();
		} else {
			SaveLoadSelectedSave();
		}
	} else {
		HandleScrollEvent(reason);
	}
}

static void DisableSelectedSlot(void);


static void SelectedSaveRegionCallBack(MOUSE_REGION* pRegion, UINT32 iReason)
{
	INT32	bSelected = gCurrentScrollTop + MSYS_GetRegionUserData( pRegion, 0 );
	if (bSelected >= INT32(gSavedGamesList.size())) {
		bSelected = -1;
	}

	if (iReason & MSYS_CALLBACK_REASON_POINTER_DOUBLECLICK) {
		if (bSelected == -1) {
			DisableButton(guiSlgSaveLoadBtn);
		} else if (gbSelectedSaveLocation == bSelected && !gfUserInTextInputMode) {
			SaveLoadSelectedSave();
		}
	}
	else if (iReason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		if(gbSelectedSaveLocation != bSelected ) {
			gbSelectedSaveLocation = bSelected;

			DestroySaveLoadTextInputBoxes();
			if (bSelected != -1) {
				EnableButton(guiSlgSaveLoadBtn);
				if (gfSaveGame && gbSelectedSaveLocation == 0) {
					// If the first entry is selected we need to input a new name
					InitSaveLoadScreenTextInputBoxes();
				}
			} else {
				DisableButton(guiSlgSaveLoadBtn);
			}

			gfRedrawSaveLoadScreen = TRUE;
		} else if (gfSaveGame && bSelected == 0 && !gfUserInTextInputMode) {
			// Clicking twice on the new save item shows input
			InitSaveLoadScreenTextInputBoxes();
			gfRedrawSaveLoadScreen = true;
		}
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
		DisableSelectedSlot();
	}
	else
	{
		HandleScrollEvent(iReason);
	}
}


static void SelectedSaveRegionMovementCallBack(MOUSE_REGION* pRegion, UINT32 reason)
{
	if( reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		gbHighLightedLocation = -1;
		gfRedrawSaveLoadScreen = true;
	}
	else if( reason & MSYS_CALLBACK_REASON_GAIN_MOUSE )
	{
		gbHighLightedLocation = (UINT8)MSYS_GetRegionUserData( pRegion, 0 );
		gfRedrawSaveLoadScreen = true;
	}
}


static void InitSaveLoadScreenTextInputBoxes(void)
{
	if (gbSelectedSaveLocation == -1)              return;
	if (!gfSaveGame)                               return;
	// If we are exiting, don't create the fields
	if (gfSaveLoadScreenExit)                      return;
	if (guiSaveLoadExitScreen != SAVE_LOAD_SCREEN) return;

	InitTextInputMode();
	SetTextInputCursor(CUROSR_IBEAM_WHITE);
	SetTextInputFont(FONT12ARIALFIXEDWIDTH);
	Set16BPPTextFieldColor(Get16BPPColor(FROMRGB(0, 0, 0)));
	SetBevelColors(Get16BPPColor(FROMRGB(136, 138, 135)), Get16BPPColor(FROMRGB(24, 61, 81)));
	SetTextInputRegularColors(FONT_WHITE, 2);
	SetTextInputHilitedColors(2, FONT_WHITE, FONT_WHITE);
	SetCursorColor(Get16BPPColor(FROMRGB(255, 255, 255)));

	AddUserInputField(NULL);

	// Game Desc Field
	INT16 const x = SLG_FIRST_SAVED_SPOT_X + SLG_SAVE_GAME_DESC_X;
	INT16 const y = SLG_FIRST_SAVED_SPOT_Y + SLG_SAVE_GAME_DESC_Y - 5 + SLG_GAP_BETWEEN_LOCATIONS * gbSelectedSaveLocation;
	AddTextInputField(x, y, SLG_SAVELOCATION_WIDTH - SLG_SAVE_GAME_DESC_X - SlgS(7), SlgS(17), MSYS_PRIORITY_HIGH + 2, {}, 46, INPUTTYPE_FULL_TEXT);
	SetActiveField(1);

	gfUserInTextInputMode = TRUE;
}


static void DestroySaveLoadTextInputBoxes(void)
{
	SetActiveField(-1);
	gfUserInTextInputMode = FALSE;
	KillAllTextInputModes();
	SetTextInputCursor( CURSOR_IBEAM );
}

static UINT8 CompareSaveGameVersion(INT32 bSaveGameID)
{
	UINT8 ubRetVal=SLS_HEADER_OK;

	auto& saveGameInfo = (*(gSavedGamesList.begin() + bSaveGameID));

	// check to see if the saved game version in the header is the same as the current version
	if (saveGameInfo.header().uiSavedGameVersion != SAVE_GAME_VERSION)
	{
		ubRetVal = SLS_SAVED_GAME_VERSION_OUT_OF_DATE;
	}

	if (strcmp(saveGameInfo.header().zGameVersionNumber, g_version_number)!= 0)
	{
		if( ubRetVal == SLS_SAVED_GAME_VERSION_OUT_OF_DATE )
			ubRetVal = SLS_BOTH_SAVE_GAME_AND_GAME_VERSION_OUT_OF_DATE;
		else
			ubRetVal = SLS_GAME_VERSION_OUT_OF_DATE;
	}

	return( ubRetVal );
}

bool AreModsEqualToEnabled(INT32 bSaveGameID) {
	auto& saveGameInfo = (*(gSavedGamesList.begin() + bSaveGameID));
	return GCM->getEnabledMods() == saveGameInfo.mods();
}


static void LoadSavedGameWarningMessageBoxCallBack(MessageBoxReturnValue const bExitValue)
{
	// yes, load the game
	if( bExitValue == MSG_BOX_RETURN_YES )
	{
		//Setup up the fade routines
		StartFadeOutForSaveLoadScreen();
	}
}


static void DoneFadeInForSaveLoadScreen(void);
static void FailedLoadingGameCallBack(MessageBoxReturnValue);


static void DoneFadeOutForSaveLoadScreen(void)
{
	// Make sure we DON'T reset the levels if we are loading a game
	gfHadToMakeBasementLevels = FALSE;

	try
	{
		auto& saveName = (*(gSavedGamesList.begin() + gbSelectedSaveLocation)).name();
		LoadSavedGame(saveName);

		gFadeInDoneCallback = DoneFadeInForSaveLoadScreen;

		ScreenID const screen = guiScreenToGotoAfterLoadingSavedGame;
		SetSaveLoadExitScreen(screen);
		if (screen == MAP_SCREEN)
		{ // We are to go to map screen after loading the game
			FadeInNextFrame();
		}
		else
		{ // We are to go to the Tactical screen after loading
			PauseTime(FALSE);
			FadeInGameScreen();
		}
	}
	catch (std::runtime_error const& e)
	{
		SLOGE("Error loading game: {}", e.what());
		ST::string msg = st_format_printf(zSaveLoadText[SLG_LOAD_GAME_ERROR], e.what());
		DoSaveLoadMessageBox(msg, SAVE_LOAD_SCREEN, MSG_BOX_FLAG_OK, FailedLoadingGameCallBack);
		NextLoopCheckForEnoughFreeHardDriveSpace();
	}
	gfStartedFadingOut = FALSE;
}


static void DoneFadeInForSaveLoadScreen(void)
{
	//Leave the screen
	//if we are supposed to stay in tactical, due nothing,
	//if we are supposed to goto mapscreen, leave tactical and go to mapscreen

	if( guiScreenToGotoAfterLoadingSavedGame == MAP_SCREEN )
	{
		if( !gfPauseDueToPlayerGamePause )
		{
			UnLockPauseState( );
			UnPauseGame( );
		}
	}

	else
	{
		//if the game is currently paused
		if( GamePaused() )
		{
			//need to call it twice
			HandlePlayerPauseUnPauseOfGame();
			HandlePlayerPauseUnPauseOfGame();
		}
	}
}


static void SelectedSLSEntireRegionCallBack(MOUSE_REGION *, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
		DisableSelectedSlot();
	} else {
		HandleScrollEvent(iReason);
	}
}


static void DisableSelectedSlot(void)
{
	//reset selected slot
	gbSelectedSaveLocation = -1;
	gfRedrawSaveLoadScreen = TRUE;
	DestroySaveLoadTextInputBoxes();

	if( !gfSaveGame )
		DisableButton( guiSlgSaveLoadBtn );

	//reset the selected graphic
	ClearSelectedSaveSlot();
}


static void ConfirmSavedGameMessageBoxCallBack(MessageBoxReturnValue const bExitValue)
{
	Assert( gbSelectedSaveLocation != -1 && gbSelectedSaveLocation != 0 );

	auto& save = *(gSavedGamesList.begin() + gbSelectedSaveLocation);

	if( bExitValue == MSG_BOX_RETURN_YES )
	{
		DoSaveGame(save.name(), save.header().sSavedGameDesc);
	}
}

static void ConfirmDeleteSavedGameCallBack(MessageBoxReturnValue const bExitValue)
{
	Assert( gbSelectedSaveLocation != -1 );

	auto& save = *(gSavedGamesList.begin() + gbSelectedSaveLocation);
	if( bExitValue == MSG_BOX_RETURN_YES )
	{
		try {
			GCM->saveGameFiles()->deleteFile(GetSaveGamePath(save.name()));
			gSavedGamesList.erase(gSavedGamesList.begin() + gbSelectedSaveLocation);
		} catch (const std::runtime_error& err) {
			SLOGE("Error deleting save game {}: {}", save.name(), err.what());
		}
		gbSelectedSaveLocation = -1;
		gfRedrawSaveLoadScreen = true;
	}
}


static void FailedLoadingGameCallBack(MessageBoxReturnValue const bExitValue)
{
	// yes
	if( bExitValue == MSG_BOX_RETURN_OK )
	{
		//if the current screen is tactical
		if( guiPreviousOptionScreen == MAP_SCREEN )
		{
			SetPendingNewScreen( MAINMENU_SCREEN );
		}
		else
		{
			LeaveTacticalScreen( MAINMENU_SCREEN );
		}

		SetSaveLoadExitScreen( MAINMENU_SCREEN );


		//We want to reinitialize the game
		ReStartingGame();
	}
}


void DoQuickSave()
{
	// Use the Dead is Dead function if we are in DiD
	if (gGameOptions.ubGameSaveMode == DIF_DEAD_IS_DEAD)
	{
		DoDeadIsDeadSave();
	} else
	{
		if (SaveGame(GetQuickSaveName(), GetQuickSaveName())) return;

		if (guiPreviousOptionScreen == MAP_SCREEN)
		{
			DoMapMessageBox(MSG_BOX_BASIC_STYLE, zSaveLoadText[SLG_SAVE_GAME_ERROR], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		} else
		{
			DoMessageBox(MSG_BOX_BASIC_STYLE, zSaveLoadText[SLG_SAVE_GAME_ERROR], GAME_SCREEN, MSG_BOX_FLAG_OK, NULL, NULL);
		}
	}
}

void DoAutoSave()
{
	// Use the Dead is Dead function if we are in DiD
	if (gGameOptions.ubGameSaveMode == DIF_DEAD_IS_DEAD)
	{
		DoDeadIsDeadSave();
	}
	else
	{
		auto saveName = GetAutoSaveName(GetNextIndexForAutoSave());
		if (SaveGame(saveName, saveName)) return;

		if (guiPreviousOptionScreen == MAP_SCREEN)
		{
			DoMapMessageBox(MSG_BOX_BASIC_STYLE, zSaveLoadText[SLG_SAVE_GAME_ERROR], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		} else
		{
			DoMessageBox(MSG_BOX_BASIC_STYLE, zSaveLoadText[SLG_SAVE_GAME_ERROR], GAME_SCREEN, MSG_BOX_FLAG_OK, NULL, NULL);
		}
	}
}

// Save function for Dead Is Dead
void DoDeadIsDeadSave()
{
	// Reload saves
	InitSaveGameArray();

	// Check if we are in a sane state! Do not save if:
	// - we are in an AI Turn
	// - we are in a Dialogue
	// - we are in Meanwhile.....
	// - we are in a locked ui
	// - we are currently in a message box - The Messagebox would be gone without selection after loading
	if (gTacticalStatus.ubCurrentTeam == OUR_TEAM && !gfInTalkPanel && !gfInMeanwhile && !gfPreBattleInterfaceActive && guiPreviousOptionScreen != MSG_BOX_SCREEN && gCurrentUIMode != LOCKUI_MODE)
	{
		// Backup old saves
		BackupSavedGame(gGameSettings.sCurrentSavedGameName);
		// Save the previous option screen State to reset it after saving
		ScreenID tmpGuiPreviousOptionScreen = guiPreviousOptionScreen;
		// We want to save the current screen we are in. Unless we are in Options, Laptop, or others
		// Make sure we are always in a sane screen.
		if (tmpGuiPreviousOptionScreen != MAP_SCREEN && tmpGuiPreviousOptionScreen != GAME_SCREEN) {
			if (guiCurrentScreen != MAP_SCREEN && guiCurrentScreen != GAME_SCREEN) {
				// If all fails, go to the map screen, this (almost) guarantees the game will start
				guiPreviousOptionScreen = MAP_SCREEN;
			} else {
				guiPreviousOptionScreen = guiCurrentScreen;
			}
		}

		BOOLEAN tmpSuccess = SaveGame(gGameSettings.sCurrentSavedGameName, gGameSettings.sCurrentSavedGameDescription);

		// Reset the previous option screen
		guiPreviousOptionScreen = tmpGuiPreviousOptionScreen;
		if (tmpSuccess) return;

		if (guiPreviousOptionScreen == MAP_SCREEN)
		{
			DoMapMessageBox(MSG_BOX_BASIC_STYLE, zSaveLoadText[SLG_SAVE_GAME_ERROR], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		} else
		{
			DoMessageBox(MSG_BOX_BASIC_STYLE, zSaveLoadText[SLG_SAVE_GAME_ERROR], GAME_SCREEN, MSG_BOX_FLAG_OK, NULL, NULL);
		}
	}
}


void DoQuickLoad()
{
	// Reload saves
	InitSaveGameArray();

	gbSelectedSaveLocation = -1;
	for (auto i = gSavedGamesList.begin(); i < gSavedGamesList.end(); i++) {
		if (IsQuickSaveName((*i).name())) {
			gbSelectedSaveLocation = std::distance(gSavedGamesList.begin(), i);
		}
	}

	if (gbSelectedSaveLocation == -1) return;

	StartFadeOutForSaveLoadScreen();
	gfDoingQuickLoad = TRUE;
}


bool AreThereAnySavedGameFiles()
{
	return GetValidSaveGames().size() > 0;
}


static void MoveSelectionDown()
{
	auto newSelectedSaveLocation = std::min((INT32)gSavedGamesList.size() - 1, gbSelectedSaveLocation + 1);
	if (newSelectedSaveLocation != gbSelectedSaveLocation) {
		gbSelectedSaveLocation = newSelectedSaveLocation;
		if (gbSelectedSaveLocation >= gCurrentScrollTop + NUM_SAVE_GAMES && gCurrentScrollTop < ScrollPositionTopMax()) {
			gCurrentScrollTop += 1;
		}
		gfRedrawSaveLoadScreen = TRUE;
	}
}


static void MoveSelectionUp()
{
	auto newSelectedSaveLocation = std::max(0, gbSelectedSaveLocation - 1);
	if (newSelectedSaveLocation != gbSelectedSaveLocation) {
		gbSelectedSaveLocation = newSelectedSaveLocation;

		if (gbSelectedSaveLocation < gCurrentScrollTop) {
			gCurrentScrollTop -= 1;
		}
		gfRedrawSaveLoadScreen = TRUE;
	}
}


static void ClearSelectedSaveSlot(void)
{
	gbSelectedSaveLocation = -1;
}


static void DoSaveGame(const ST::string &saveName, const ST::string &saveDescription)
{
	//Redraw the save load screen
	RenderSaveLoadScreen();

	//render the buttons
	MarkButtonsDirty( );
	RenderButtons();

	// If we are selecting the Dead is Dead Savegame slot, only remember the slot, do not save
	// Also set the INTRO_SCREEN as previous options screen. This is a hack to get the game started
	if (guiPreviousOptionScreen == GAME_INIT_OPTIONS_SCREEN)
	{
		guiPreviousOptionScreen = INTRO_SCREEN;
		// This is not used anymore, we use the last updated timestamp now
		gGameSettings.bLastSavedGameSlot = 0;
		gGameSettings.sCurrentSavedGameName = saveName;
		gGameSettings.sCurrentSavedGameDescription = saveDescription;
	}
	else
	{
		if( !SaveGame(saveName, saveDescription)) {
			DoSaveLoadMessageBox(zSaveLoadText[SLG_SAVE_GAME_ERROR], SAVE_LOAD_SCREEN, MSG_BOX_FLAG_OK, NULL);
			return;
		}
	}

	SetSaveLoadExitScreen( guiPreviousOptionScreen );
}


static void StartFadeOutForSaveLoadScreen(void)
{
	//if the game is paused, and we are in tactical, unpause
	if( guiPreviousOptionScreen == GAME_SCREEN )
	{
		PauseTime( FALSE );
	}

	gFadeOutDoneCallback = DoneFadeOutForSaveLoadScreen;

	FadeOutNextFrame( );
	gfStartedFadingOut = TRUE;
}
