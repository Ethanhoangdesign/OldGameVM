/* OldGameVM modification notice
 * This file was changed for OldGameVM in July 2026.
 * It is not the original file. See NOTICE.md.
 */
#ifndef _UI_LAYOUT_H_
#define _UI_LAYOUT_H_

#include "Types.h"

/////////////////////////////////////////////////////////////
// defines
/////////////////////////////////////////////////////////////

#define NUM_INVENTORY_SLOTS     (19)

/* Following defines allow us to not change the old code too much.
 * It will help to preserve original Stracciatella codebase. */

#define SCREEN_HEIGHT                   (g_ui.m_screenHeight)
#define SCREEN_WIDTH                    (g_ui.m_screenWidth)
#define INV_INTERFACE_START_Y           (g_ui.get_INV_INTERFACE_START_Y())
#define INV_INTERFACE_HEIGHT            (140)                                 // height of the bottom bar single-merc inventory panel
#define INTERFACE_START_X               (g_ui.m_teamPanelPosition.iX)
#define INTERFACE_START_Y               (g_ui.m_teamPanelPosition.iY)
#define gsVIEWPORT_START_X              (g_ui.m_VIEWPORT_START_X)
#define gsVIEWPORT_START_Y              (g_ui.m_VIEWPORT_START_Y)
#define gsVIEWPORT_WINDOW_START_Y       (g_ui.m_VIEWPORT_WINDOW_START_Y)
#define gsVIEWPORT_END_X                (g_ui.m_VIEWPORT_END_X)
#define gsVIEWPORT_END_Y                (g_ui.m_VIEWPORT_END_Y)
#define gsVIEWPORT_WINDOW_END_Y         (g_ui.m_VIEWPORT_WINDOW_END_Y)
#define STD_SCREEN_X                    (g_ui.m_stdScreenOffsetX)
#define STD_SCREEN_Y                    (g_ui.m_stdScreenOffsetY)
#define MAP_SCREEN_WIDTH                (g_ui.m_mapScreenWidth)
#define MAP_SCREEN_HEIGHT               (g_ui.m_mapScreenHeight)

#define SM_BODYINV_X                    (INTERFACE_START_X + 244)
#define SM_BODYINV_Y                    (INV_INTERFACE_START_Y + 6)
#define SM_INVINTERFACE_WIDTH           (532)    // width of the single-merc inventory panel excluding the right-side buttons and minimap

#define EDITOR_TASKBAR_HEIGHT           (120)
#define EDITOR_TASKBAR_POS_Y            (UINT16)(SCREEN_HEIGHT - EDITOR_TASKBAR_HEIGHT)

#define DEFAULT_EXTERN_PANEL_X_POS      (STD_SCREEN_X + 320)
#define DEFAULT_EXTERN_PANEL_Y_POS      (STD_SCREEN_Y + 40)

#define TEAMPANEL_SLOT_WIDTH            (83)     // width of one slot in the bottom team panel
#define TEAMPANEL_BUTTONSBOX_WIDTH      (142)    // width of the container of the buttons on the right of team panel
#define TEAMPANEL_BUTTONSBOX_WIDTH_WF   (194)    // width of the same container in the JA2: Wildfire bottom bar art
#define TEAMPANEL_HEIGHT                (120)    // height of the bottom bar team panel


/////////////////////////////////////////////////////////////
// type definitions
/////////////////////////////////////////////////////////////

// USED TO SETUP REGION POSITIONS, ETC
struct INV_REGION_DESC
{
	UINT16     uX;
	UINT16     uY;

	void set(UINT16 x, UINT16 y)
	{
		uX = x;
		uY = y;
	}
};


struct MoneyLoc
{
	UINT16 x;
	UINT16 y;

	void set(UINT16 _x, UINT16 _y)
	{
		x = _x;
		y = _y;
	}
};


/** User Interface layout definition. */
struct UILayout
{
public:
	UINT16                m_mapScreenWidth;
	UINT16                m_mapScreenHeight;
	UINT16                m_screenWidth;
	UINT16                m_screenHeight;
	INV_REGION_DESC       m_invSlotPositionMap[NUM_INVENTORY_SLOTS];      /**< Map screen inventory slots positions  */
	INV_REGION_DESC       m_invSlotPositionTac[NUM_INVENTORY_SLOTS];      /**< Tactical screen Inventory slots positions */
	INV_REGION_DESC       m_invCamoRegion;                                /**< Camo (body) region in the inventory. */

	SGPBox                m_progress_bar_box;
	MoneyLoc              m_moneyButtonLoc;
	MoneyLoc              m_MoneyButtonLocMap;

	/** Viewport coordiantes.
	 * Viewport is the area of the screen where tactical map is displayed.
	 * For 640x480 it is (320, 180) */
	UINT16                m_VIEWPORT_START_X;
	UINT16                m_VIEWPORT_START_Y;
	UINT16                m_VIEWPORT_WINDOW_START_Y;

	UINT16                m_VIEWPORT_END_X;
	UINT16                m_VIEWPORT_END_Y;
	UINT16                m_tacticalMapCenterX;                           /**< Center of the tactical map (for 640x480 it is (320, 180)). */
	UINT16                m_tacticalMapCenterY;                           /**< Center of the tactical map (for 640x480 it is (320, 180)). */

	UINT16                m_VIEWPORT_WINDOW_END_Y;

	SGPRect               m_worldClippingRect;

	// Map screen interface
	SGPPoint              m_versionPosition;
	SGPPoint              m_contractPosition;
	SGPPoint              m_attributePosition;
	SGPPoint              m_trainPosition;
	SGPPoint              m_vehiclePosition;
	SGPPoint              m_repairPosition;
	SGPPoint              m_assignmentPosition ;
	SGPPoint              m_squadPosition ;

	// Tactical screen bottom bar
	// It can be in the "team" (TEAM) or the "single merc inventory" (SM or INV_) mode. Both modes have the same
	// width, but the single-merc mode is slightly taller.
	SGPPoint              m_teamPanelPosition;              // offset position of the bottom bar
	UINT16                m_teamPanelSlotsTotalWidth;       // total width of all team slots in the bottom team panel
	UINT16                m_teamPanelWidth;                 // width of the entire team panel including slots and buttons

	UINT16                m_stdScreenOffsetX;             /** Offset of the standard (640x480) window */
	UINT16                m_stdScreenOffsetY;             /** Offset of the standard (640x480) window */

	/** Constructor.
	 * @param screenWidth Screen width
	 * @param screenHeight Screen height */
	UILayout(UINT16 screenWidth, UINT16 screenHeight);

	/** Set new screen size. Element positions should be recalculated after setting this. @see UILayout::recalculatePositions */
	void setScreenSize(UINT16 width, UINT16 height);

	/** Check if the screen is bigger than original 640x480. */
	bool isBigScreen() const;

	/** True when using widescreen layouts that span full width (bottom panel from x=0 to x=width)
	 * but not full-size map art. Used for screens like 934x480, 1024x600, etc. where height < 720
	 * but we want full-width bottom panels. */
	bool isWidescreenLayout() const;

	/** True when map-screen widgets use the wide bottom-panel artwork. */
	bool isWidePanel() const;

	UINT16 currentHeight() const;
	UINT16 get_CLOCK_X() const;
	UINT16 get_CLOCK_Y() const;
	UINT16 get_INV_INTERFACE_START_Y() const;
	UINT16 get_RADAR_WINDOW_X() const;
	UINT16 get_RADAR_WINDOW_TM_Y() const;

	/** Get X position of tactical textbox. */
	UINT16 getTacticalTextBoxX() const;

	/** Get Y position of tactical textbox. */
	UINT16 getTacticalTextBoxY() const;

	/** Number of displayable slots in the team panel, based on the game policy and screen width. */
	UINT16 getTeamPanelNumSlots() const;

	/** Width of the buttons box on the right of the team panel, depending on the loaded interface art edition. */
	UINT16 getTeamPanelButtonsBoxWidth() const;

	/** Horizontal shift of the widgets that live inside the buttons box
	 *  (radar window, clock, town name) for the panel that is currently
	 *  on screen.
	 *
	 *  The SM (single-merc inventory) panel is always composed from the
	 *  vanilla inventory_bottom_panel.sti, whose buttons box is
	 *  TEAMPANEL_BUTTONSBOX_WIDTH (142) wide, while the TEAM panel may be
	 *  composed from Wildfire's bottom_bar.sti with a 194px box. Both
	 *  boxes are flush right, so their left edge -- the point every offset
	 *  below is measured from -- differs by (boxWidth - 142) px.
	 *
	 *  Anything positioned inside the buttons box MUST add this. Do not
	 *  branch on getTeamPanelButtonsBoxWidth() directly: that describes
	 *  the art edition, not the panel currently being drawn, and picking
	 *  either constant then leaves the other panel 52px out. */
	UINT16 activeButtonsBoxShift() const;

	/** True when the strategic map is drawn at full size (Wildfire 714x612 map
	 *  art on a big enough screen) instead of the vanilla half scale.
	 *  See docs/KE-HOACH-mapscreen-fullsize.md. */
	bool isMapFullSize() const;

	/** Strategic-map screen grid metrics (see Map_Screen_Interface_Map.h). */
	UINT16 get_MAP_GRID_X() const;
	UINT16 get_MAP_GRID_Y() const;
	UINT16 get_MAP_VIEW_START_X() const;
	UINT16 get_MAP_VIEW_START_Y() const;
	UINT16 get_MAP_VIEW_WIDTH() const;
	UINT16 get_MAP_VIEW_HEIGHT() const;

	/** Base offset of the map screen bottom panel (vanilla: STD_SCREEN with
	 *  the panel at +0,+359; full-size Wildfire layout: at 261,647; widescreen right-anchored panel leaves the history strip at the left). */
	UINT16 get_MAP_BOTTOM_BASE_X() const;
	UINT16 get_MAP_BOTTOM_BASE_Y() const;

	/** Origin of the map screen left column (character info + roster).
	 *  Vanilla: STD_SCREEN; full-size Wildfire layout: the top-left corner. */
	UINT16 get_MAP_LEFT_COL_X() const;
	UINT16 get_MAP_LEFT_COL_Y() const;

	/** Recalculate UI elements' positions after changing screen size.
	 *  This method requires the game data to be loaded, but it should be called before most other the application initialization is done.
	 */
	void recalculatePositions();
};

/////////////////////////////////////////////////////////////
// external declarations
/////////////////////////////////////////////////////////////

extern UILayout g_ui;

/////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////

#endif
