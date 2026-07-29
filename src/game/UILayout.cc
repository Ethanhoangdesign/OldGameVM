/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#include "UILayout.h"

#include "ContentManager.h"
#include "Directories.h"
#include "GameInstance.h"
#include "GamePolicy.h"
#include "JAScreens.h"
#include "MapScreen.h"
#include "ScreenIDs.h"
#include "Soldier_Control.h"
#include <stdexcept>
#include <string_theory/string>

#define MIN_INTERFACE_WIDTH       640
#define MIN_INTERFACE_HEIGHT      480

/* JA2: Wildfire ships a 1024px-wide 10-slot tactical bottom bar
 * (interface/bottom_bar.sti) whose buttons box is 194px wide, with the radar
 * window and clock further right than in the vanilla 640px art. Wildfire game
 * data is recognisable by its 16-bit strategic map interface/b_map.sti, which
 * replaces vanilla's b_map.pcx. */
static bool UsesWildfireInterfaceArt()
{
	static INT8 cached = -1;
	if (cached == -1 && GCM)
	{
		cached = GCM->doesGameResExists(INTERFACEDIR "/b_map.sti") &&
			!GCM->doesGameResExists(INTERFACEDIR "/b_map.pcx") ? 1 : 0;
	}
	return cached == 1;
}

/**
 * Default screen layout.
 * It might be changed later when the window size is known for sure. */
UILayout g_ui(MIN_INTERFACE_WIDTH, MIN_INTERFACE_HEIGHT);


/** Constructor. */
UILayout::UILayout(UINT16 screenWidth, UINT16 screenHeight)
	:m_mapScreenWidth(MIN_INTERFACE_WIDTH), m_mapScreenHeight(MIN_INTERFACE_HEIGHT),
	m_screenWidth(screenWidth), m_screenHeight(screenHeight)
{
}


void UILayout::setScreenSize(UINT16 width, UINT16 height)
{
	if (width < MIN_INTERFACE_WIDTH || height < MIN_INTERFACE_HEIGHT)
	{
		ST::string err = ST::format("Failed to set screen resolution {} x {}", width, height);
		throw std::runtime_error(err.to_std_string());
	}
	m_screenWidth = width;
	m_screenHeight = height;
}


/** Check if the screen is bigger than original 640x480. */
bool UILayout::isBigScreen() const
{
	return (m_screenWidth > 640) || (m_screenHeight > 480);
}


UINT16 UILayout::currentHeight() const             { return fInMapMode ? (get_MAP_BOTTOM_BASE_Y() + m_mapScreenHeight) : m_screenHeight; }
UINT16 UILayout::get_CLOCK_X() const               { return fInMapMode ? (isMapFullSize() ? (get_MAP_BOTTOM_BASE_X() + 668) /* CLOCKBAY */ : get_MAP_BOTTOM_BASE_X() + 554) : m_teamPanelPosition.iX + m_teamPanelSlotsTotalWidth + (getTeamPanelButtonsBoxWidth() == TEAMPANEL_BUTTONSBOX_WIDTH_WF ? 109 : 56); }
UINT16 UILayout::get_CLOCK_Y() const               { return currentHeight() - 23;                                  }
UINT16 UILayout::get_RADAR_WINDOW_X() const        { return fInMapMode ? (isMapFullSize() ? (get_MAP_BOTTOM_BASE_X() + 663) /* CLOCKBAY */ : get_MAP_BOTTOM_BASE_X() + 543) : m_teamPanelPosition.iX + m_teamPanelSlotsTotalWidth + (getTeamPanelButtonsBoxWidth() == TEAMPANEL_BUTTONSBOX_WIDTH_WF ? 98 : 45); }
UINT16 UILayout::get_RADAR_WINDOW_TM_Y() const     { return currentHeight() - 107;                                 }
UINT16 UILayout::get_INV_INTERFACE_START_Y() const { return m_screenHeight - INV_INTERFACE_HEIGHT;                                  }


void UILayout::recalculatePositions()
{
	m_teamPanelSlotsTotalWidth = getTeamPanelNumSlots() * TEAMPANEL_SLOT_WIDTH;
	UINT16 tpXOffset = (m_screenWidth - m_teamPanelSlotsTotalWidth - getTeamPanelButtonsBoxWidth()) / 2;
	UINT16 tpYOffset = m_screenHeight - TEAMPANEL_HEIGHT;
	m_teamPanelPosition.set(tpXOffset, tpYOffset);
	m_teamPanelWidth = m_teamPanelSlotsTotalWidth + getTeamPanelButtonsBoxWidth();

	UINT16 startInvY = get_INV_INTERFACE_START_Y();
	UINT16 startX    = INTERFACE_START_X;

	m_stdScreenOffsetX            = (m_screenWidth - MIN_INTERFACE_WIDTH) / 2;
	m_stdScreenOffsetY            = (m_screenHeight - MIN_INTERFACE_HEIGHT) / 2;

	// tactical screen inventory position
	m_invSlotPositionTac[HELMETPOS           ].set(startX + 344, startInvY +   6);
	m_invSlotPositionTac[VESTPOS             ].set(startX + 344, startInvY +  35);
	m_invSlotPositionTac[LEGPOS              ].set(startX + 344, startInvY +  95);
	m_invSlotPositionTac[HEAD1POS            ].set(startX + 226, startInvY +   6);
	m_invSlotPositionTac[HEAD2POS            ].set(startX + 226, startInvY +  30);
	m_invSlotPositionTac[HANDPOS             ].set(startX + 226, startInvY +  84);
	m_invSlotPositionTac[SECONDHANDPOS       ].set(startX + 226, startInvY + 108);
	m_invSlotPositionTac[BIGPOCK1POS         ].set(startX + 468, startInvY +   5);
	m_invSlotPositionTac[BIGPOCK2POS         ].set(startX + 468, startInvY +  29);
	m_invSlotPositionTac[BIGPOCK3POS         ].set(startX + 468, startInvY +  53);
	m_invSlotPositionTac[BIGPOCK4POS         ].set(startX + 468, startInvY +  77);
	m_invSlotPositionTac[SMALLPOCK1POS       ].set(startX + 396, startInvY +   5);
	m_invSlotPositionTac[SMALLPOCK2POS       ].set(startX + 396, startInvY +  29);
	m_invSlotPositionTac[SMALLPOCK3POS       ].set(startX + 396, startInvY +  53);
	m_invSlotPositionTac[SMALLPOCK4POS       ].set(startX + 396, startInvY +  77);
	m_invSlotPositionTac[SMALLPOCK5POS       ].set(startX + 432, startInvY +   5);
	m_invSlotPositionTac[SMALLPOCK6POS       ].set(startX + 432, startInvY +  29);
	m_invSlotPositionTac[SMALLPOCK7POS       ].set(startX + 432, startInvY +  53);
	m_invSlotPositionTac[SMALLPOCK8POS       ].set(startX + 432, startInvY +  77);

	// map screen inventory position
	/* Map screen inventory slots follow the left column origin. */
	UINT16 const mapInvX = get_MAP_LEFT_COL_X();
	UINT16 const mapInvY = get_MAP_LEFT_COL_Y();
	m_invSlotPositionMap[HELMETPOS           ].set(mapInvX + 204, mapInvY + 116);
	m_invSlotPositionMap[VESTPOS             ].set(mapInvX + 204, mapInvY + 145);
	m_invSlotPositionMap[LEGPOS              ].set(mapInvX + 204, mapInvY + 205);
	m_invSlotPositionMap[HEAD1POS            ].set(mapInvX +  21, mapInvY + 116);
	m_invSlotPositionMap[HEAD2POS            ].set(mapInvX +  21, mapInvY + 140);
	m_invSlotPositionMap[HANDPOS             ].set(mapInvX +  21, mapInvY + 194);
	m_invSlotPositionMap[SECONDHANDPOS       ].set(mapInvX +  21, mapInvY + 218);
	m_invSlotPositionMap[BIGPOCK1POS         ].set(mapInvX +  98, mapInvY + 251);
	m_invSlotPositionMap[BIGPOCK2POS         ].set(mapInvX +  98, mapInvY + 275);
	m_invSlotPositionMap[BIGPOCK3POS         ].set(mapInvX +  98, mapInvY + 299);
	m_invSlotPositionMap[BIGPOCK4POS         ].set(mapInvX +  98, mapInvY + 323);
	m_invSlotPositionMap[SMALLPOCK1POS       ].set(mapInvX +  22, mapInvY + 251);
	m_invSlotPositionMap[SMALLPOCK2POS       ].set(mapInvX +  22, mapInvY + 275);
	m_invSlotPositionMap[SMALLPOCK3POS       ].set(mapInvX +  22, mapInvY + 299);
	m_invSlotPositionMap[SMALLPOCK4POS       ].set(mapInvX +  22, mapInvY + 323);
	m_invSlotPositionMap[SMALLPOCK5POS       ].set(mapInvX +  60, mapInvY + 251);
	m_invSlotPositionMap[SMALLPOCK6POS       ].set(mapInvX +  60, mapInvY + 275);
	m_invSlotPositionMap[SMALLPOCK7POS       ].set(mapInvX +  60, mapInvY + 299);
	m_invSlotPositionMap[SMALLPOCK8POS       ].set(mapInvX +  60, mapInvY + 323);

	m_invCamoRegion.set(SM_BODYINV_X, SM_BODYINV_Y);

	m_progress_bar_box.set(STD_SCREEN_X + 5, 2, MIN_INTERFACE_WIDTH - 10, 12);
	m_moneyButtonLoc.set(startX + 343, startInvY + 11);
	m_MoneyButtonLocMap.set(mapInvX + 174, mapInvY + 115);

	m_VIEWPORT_START_X            = 0;
	m_VIEWPORT_START_Y            = 0;
	m_VIEWPORT_WINDOW_START_Y     = 0;
	m_VIEWPORT_END_X              = m_screenWidth;
	m_VIEWPORT_END_Y              = m_screenHeight - 120;
	m_VIEWPORT_WINDOW_END_Y       = m_screenHeight - 120;
	m_tacticalMapCenterX          = (m_VIEWPORT_END_X - m_VIEWPORT_START_X) / 2;
	m_tacticalMapCenterY          = (m_VIEWPORT_END_Y - m_VIEWPORT_START_Y) / 2;

	m_worldClippingRect.set(0, 0, m_screenWidth, m_screenHeight - 120);

	/* Map screen assignment popup menus anchor to the left column (which the
	 * full-size Wildfire layout pins to the top-left corner). */
	UINT16 const mapLeftX = get_MAP_LEFT_COL_X();
	UINT16 const mapLeftY = get_MAP_LEFT_COL_Y();
	m_contractPosition.set(       mapLeftX + 120, mapLeftY +  50);
	m_attributePosition.set(      mapLeftX + 220, mapLeftY + 150);
	m_trainPosition.set(          mapLeftX + 160, mapLeftY + 150);
	m_vehiclePosition.set(        mapLeftX + 160, mapLeftY + 150);
	m_repairPosition.set(         mapLeftX + 160, mapLeftY + 150);
	m_assignmentPosition.set(     mapLeftX + 120, mapLeftY + 150);
	m_squadPosition.set(          mapLeftX + 160, mapLeftY + 150);
	m_versionPosition.set(        10, m_screenHeight - 15);
}

/** Get X position of tactical textbox. */
UINT16 UILayout::getTacticalTextBoxX() const
{

	if ( guiCurrentScreen == MAP_SCREEN )
	{
		return STD_SCREEN_X + 110;
	}
	else
	{
		return 110;
	}
}

/** Get Y position of tactical textbox. */
UINT16 UILayout::getTacticalTextBoxY() const
{
	if ( guiCurrentScreen == MAP_SCREEN )
	{
		return DEFAULT_EXTERN_PANEL_Y_POS;
	}
	else
	{
		return 20;
	}
}

UINT16 UILayout::getTeamPanelNumSlots() const
{
	if (!GCM || !GCM->getGamePolicy())
	{
		throw std::runtime_error("ContentManager is not initialized yet. Unable to determine num of team slots");
	}

	int numSlots = std::min({(int)gamepolicy(squad_size), (m_screenWidth - getTeamPanelButtonsBoxWidth()) / TEAMPANEL_SLOT_WIDTH, 12});
	numSlots = std::max((int)numSlots, 6);
	return numSlots;
}

bool UILayout::isMapFullSize() const
{
	/* Full-size strategic map (docs/KE-HOACH-mapscreen-fullsize.md, M2):
	 * editions with full-resolution map art (Wildfire's 714x612 b_map.sti)
	 * draw it at full scale when the 1024x768 Wildfire layout fits. */
	return UsesWildfireInterfaceArt() &&
		m_screenWidth >= 1024 && m_screenHeight >= 768;
}

/* Strategic-map screen grid metrics. Vanilla renders the strategic map at
 * half scale: 21x18 cells in a 336x298 view at STD_SCREEN + (270,10). The
 * full-size mode doubles the grid (42x36, 672x596 view) and anchors the view
 * to the Wildfire 1024x768 map screen layout: a 261px left column and a 763px
 * right region whose 714x612 map art starts at x=285 (terrain at art (41,35),
 * so cell (1,1) lands at view start + one grid step, like vanilla). */
/* MAPZOOM: chi man hinh that su lon (>=1600x1000) moi phong ban do len 1.5 lan.
 * O 1024x768 va 1366x768 ham nay tra ve 2, moi bieu thuc thanh x2/2, tuc giu
 * nguyen tuyet doi gia tri cu. 42*3/2=63 va 36*3/2=54 deu la so nguyen, nen
 * buoc luoi khong sinh so thap phan lam lech o khi bam chuot. */
static inline int MapZoomNum(UINT16 w, UINT16 h) { return (w >= 1600 && h >= 1000) ? 3 : 2; }

UINT16 UILayout::get_MAP_GRID_X() const       { return isMapFullSize() ? (UINT16)(42 * MapZoomNum(m_screenWidth, m_screenHeight) / 2) : 21; }
UINT16 UILayout::get_MAP_GRID_Y() const       { return isMapFullSize() ? (UINT16)(36 * MapZoomNum(m_screenWidth, m_screenHeight) / 2) : 18; }
UINT16 UILayout::get_MAP_VIEW_START_X() const { /* LEVELSLOT-H: centre the fixed 672px map in the right-hand region so a
	   wider screen only grows the wooden background, never the map. */
	/* LEVELSLOT-H: centre the fixed 672px map in the right-hand region so a
	   wider screen only grows the wooden background, never the map. The map
	   art is a fixed 714px picture, so pinning it to the right edge would
	   only move the empty wood from one side to the other. */
	return isMapFullSize() ? (UINT16)(261 + (m_screenWidth - 261 - 714 * MapZoomNum(m_screenWidth, m_screenHeight) / 2) / 2) : m_stdScreenOffsetX + 270; }
/* Full-size: the map hugs the top of the screen (grid bottom lands at y 612),
 * leaving the 613..648 band free for the toggle strip, matching Wildfire's own
 * Map_Bord.sti border art (wood bar at 609..648). */
UINT16 UILayout::get_MAP_VIEW_START_Y() const { return isMapFullSize()
		? (UINT16)((m_screenHeight - 121) > (612 * MapZoomNum(m_screenWidth, m_screenHeight) / 2 + 35) ? ((m_screenHeight - 121) - (612 * MapZoomNum(m_screenWidth, m_screenHeight) / 2 + 35)) / 2 : 0)
		: m_stdScreenOffsetY + 10; }
UINT16 UILayout::get_MAP_VIEW_WIDTH() const   { return isMapFullSize() ? (UINT16)(672 * MapZoomNum(m_screenWidth, m_screenHeight) / 2) : 336; }
UINT16 UILayout::get_MAP_VIEW_HEIGHT() const  { return isMapFullSize() ? (UINT16)(596 * MapZoomNum(m_screenWidth, m_screenHeight) / 2) : 298; }

/* The map screen bottom panel keeps its vanilla-relative offsets (+0..640,
 * +359..480); in the full-size Wildfire layout its 763px art sits at
 * (261, 647), i.e. base (261, 288). */
/* PINRIGHT: the 763px bottom panel art is pinned to the right edge, so the
 * left column grows with the screen and the history log fills it. */
UINT16 UILayout::get_MAP_BOTTOM_BASE_X() const { return isMapFullSize() ? (UINT16)(m_screenWidth > 1024 ? m_screenWidth - 763 : 261) : m_stdScreenOffsetX; }
UINT16 UILayout::get_MAP_BOTTOM_BASE_Y() const { return isMapFullSize() ? (UINT16)(m_screenHeight - 480) : m_stdScreenOffsetY; }

UINT16 UILayout::get_MAP_LEFT_COL_X() const { return isMapFullSize() ? 0 : m_stdScreenOffsetX; }
UINT16 UILayout::get_MAP_LEFT_COL_Y() const { return isMapFullSize() ? 0 : m_stdScreenOffsetY; }

UINT16 UILayout::getTeamPanelButtonsBoxWidth() const
{
	/* The wider Wildfire buttons box is only used when the screen can hold the
	 * minimum six team slots next to it (i.e. from 800x600 up, Wildfire's own
	 * minimum resolution); below that keep the vanilla layout. */
	if (UsesWildfireInterfaceArt() &&
		m_screenWidth >= 6 * TEAMPANEL_SLOT_WIDTH + TEAMPANEL_BUTTONSBOX_WIDTH_WF)
	{
		return TEAMPANEL_BUTTONSBOX_WIDTH_WF;
	}
	return TEAMPANEL_BUTTONSBOX_WIDTH;
}
