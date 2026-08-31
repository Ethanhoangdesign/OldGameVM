#include "Auto_Resolve.h"
#include "Directories.h"
#include "Font.h"
#include "HImage.h"
#include "Handle_Items.h"
#include "Interface.h"
#include "Isometric_Utils.h"
#include "ItemModel.h"
#include "Map_Screen_Interface_Bottom.h"
#include "Map_Screen_Interface_Map_Inventory.h"
#include "MessageBoxScreen.h"
#include "Object_Cache.h"
#include "Logger.h"
#include "Timer_Control.h"
#include "VObject.h"
#include "VObject_Blitters.h"
#include "SysUtil.h"
#include "Map_Screen_Interface_Border.h"
#include "Map_Screen_Interface.h"
#include "Map_Screen_Interface_Map.h"
#include "Items.h"
#include "Interface_Items.h"
#include "Interface_Utils.h"
#include "Text.h"
#include "Font_Control.h"
#include "StrategicMap.h"
#include "World_Items.h"
#include "Tactical_Save.h"
#include "Soldier_Control.h"
#include "English.h"
#include "MapScreen.h"
#include "Radar_Screen.h"
#include "Render_Dirty.h"
#include "Interface_Panels.h"
#include "WordWrap.h"
#include "Button_System.h"
#include "ScreenIDs.h"
#include "VSurface.h"
#include "ShopKeeper_Interface.h"
#include "ArmsDealerInvInit.h"

#include "ContentManager.h"
#include "GameInstance.h"

#include <string_theory/format>
#include <string_theory/string>

#include <memory>
#include <vector>

// status bar colors
#define DESC_STATUS_BAR FROMRGB( 201, 172,  133 )
#define DESC_STATUS_BAR_SHADOW FROMRGB( 140, 136,  119 )

// delay for flash of item
#define DELAY_FOR_HIGHLIGHT_ITEM_FLASH 200

// inventory slot font
#define MAP_IVEN_FONT						SMALLCOMPFONT

// inventory pool slot positions and sizes
#define MAP_INV_SLOT_ROWS (InvLayout().rows)

/* SECTORINV-GRID: khoi bo cuc dat SAU khai bao
 * guiMapInventoryPoolBackground vi ProbeSectorInvArt() can dung no. */


// the current highlighted item
INT32 iCurrentlyHighLightedItem = -1;
BOOLEAN fFlashHighLightInventoryItemOnradarMap = FALSE;

// whether we are showing the inventory pool graphic
BOOLEAN fShowMapInventoryPool = FALSE;

// the v-object index value for the background
static cache_key_t const guiMapInventoryPoolBackground{ INTERFACEDIR "/sector_inventory.sti" };
static cache_key_t const guiMapInventoryPoolArrows{ INTERFACEDIR "/map_screen_bottom_arrows.sti" };
static cache_key_t const guiMapInventoryPoolDone{ INTERFACEDIR "/done_button.sti" };

/* SECTORINV-GRID: bo hang so bo cuc cua bang sector inventory.
 *
 * Co HAI bo art, va header STCI cua ca hai deu ghi 380x360 nen khong
 * the tin header. Kich thuoc that nam o SubregionProperties(0) vi
 * BltVideoObject() ve subregion 0:
 *
 *     vanilla  : 379 x 360  -> 5 cot x  9 hang, o  72 x 32
 *     Wildfire : 763 x 647  -> 5 cot x 10 hang, o 145 x 57
 *
 * Cac so cua ban WF do truc tiep tu art (khe giua cac o deu 145 theo
 * chieu ngang va 57 theo chieu doc, long o sang 123x48).
 */
struct SectorInvLayout
{
	SGPBox box;         // ca bang, tuong doi so voi goc bang
	SGPBox title_box;
	SGPBox slot_box;    // o dau tien + buoc luoi (w,h = buoc)
	SGPBox region_box;  // rel slot_box
	SGPBox item_box;    // rel slot_box
	/* rel slot_box. Rieng thanh tinh trang dung kieu CO DAU: o ban WF
	 * ranh lom nam NGOAI long o nen offset x am. SGPBox dung UINT16,
	 * -3 se thanh 65533 va thanh bay ra khoi man hinh. */
	struct { INT16 x, y, w, h; } bar_box;
	SGPBox name_box;    // rel slot_box
	SGPBox loc_box;
	SGPBox count_box;
	SGPBox page_box;
	INT32  rows;
	INT32  cols;
	INT16  prev_x, prev_y;
	INT16  next_x, next_y;
	INT16  done_x, done_y;
	INT16  label1_x, label1_y, label1_w;
	INT16  label2_x, label2_w;
};

static const SectorInvLayout g_inv_layout_vanilla = {
	{ 261,   0, 379, 360 },
	{ 266,   5, 370,  29 },
	{ 274,  37,  72,  32 },
	{   0,   0,  67,  31 },
	{   6,   0,  61,  24 },
	{   2,   2,   2,  20 },
	{   0,  24,  67,   7 },
	{ 326, 337,  39,  10 },
	{ 437, 337,  39,  10 },
	{ 505, 337,  50,  10 },
	9, 5,
	487, 336,   559, 336,   587, 333,
	268, 342, 53,
	369, 65,
};

/* Ban Wildfire: art 763x647. Tat ca so duoi day DO TU ART:
 *   luoi  : cot bat dau x=35 buoc 145 (5 cot), hang y=37 buoc 57 (10 hang)
 *   long o: 123 x 48  (vung sang giua hai khe)
 *   o day : tim bang flood-fill vung xanh tham, khong doan:
 *             loc   x231..281 y617..636
 *             count x473..502 y618..632
 *             page  x552..598 y619..634
 *             done  x684..721 y622..636
 *
 *   ranh lom cho thanh tinh trang: doi chieu voi ban vanilla (noi hang so
 *   goc von dung) thi ranh la vet TOI nam giua hai go SANG, khong phai
 *   vet sang:
 *       vanilla: go sang rel +1 (lum 55) | RANH rel +2..+3 (lum 13)
 *                | go sang rel +4..+5
 *       WF     : go sang rel -7 (lum 40) | RANH rel -6..-5 (lum 14)
 *                | go sang rel -3..-2 (lum 65)
 *   Ranh WF nam NGOAI long o nen bar_box.x am. Da do tren ca 50 o.
 *   Chieu doc: ranh bat dau rel y +2, cao 43.
 * Toa do duoi day deu TUONG DOI so voi goc trai-tren cua bang. */
static const SectorInvLayout g_inv_layout_wf = {
	{   0,   0, 763, 647 },
	{   6,   5, 751,  29 },
	{  35,  37, 145,  57 },
	{   0,   0, 123,  48 },
	{  11,   0, 112,  37 },
	{  -6,   2,   2,  43 },
	{   0,  37, 123,  11 },
	{ 231, 617,  51,  20 },
	{ 473, 618,  30,  15 },
	{ 552, 619,  47,  16 },
	10, 5,
	518, 617,   607, 617,   684, 617,
	150, 627, 75,
	330, 110,
};

/* Kich thuoc that cua art, doc mot lan luc dau. */
static bool  g_inv_probed = false;
static bool  g_inv_is_wf  = false;

struct SectorInvTransform
{
	INT32 x;
	INT32 y;
	INT32 w;
	INT32 h;
	INT32 native_w;
	INT32 native_h;

	bool IsScaled() const
	{
		return w != native_w || h != native_h;
	}

	INT32 X(INT32 const native_x) const
	{
		return x + native_x * w / native_w;
	}

	INT32 Y(INT32 const native_y) const
	{
		return y + native_y * h / native_h;
	}

	INT32 W(INT32 const native_x, INT32 const native_w_) const
	{
		return X(native_x + native_w_) - X(native_x);
	}

	INT32 H(INT32 const native_y, INT32 const native_h_) const
	{
		return Y(native_y + native_h_) - Y(native_y);
	}
};

static void ProbeSectorInvArt(void)
{
	if (g_inv_probed) return;
	g_inv_probed = true;
	try
	{
		SGPVObject const* const vo = GetVObject(guiMapInventoryPoolBackground);
		if (vo != NULL && vo->SubregionCount() > 0)
		{
			ETRLEObject const& e = vo->SubregionProperties(0);
			/* Art vanilla rong 379; ban WF rong 763. Lay 512 lam nguong
			 * de bat duoc ca nhung ban art che khac doi chut. */
			g_inv_is_wf = (e.usWidth >= 512);
			SLOGD("Sector inventory art: {}x{} -> {}",
				e.usWidth, e.usHeight, g_inv_is_wf ? "Wildfire" : "vanilla");
		}
	}
	catch (...)
	{
		/* khong doc duoc thi giu mac dinh vanilla */
	}
}

static SectorInvLayout const& InvLayout(void)
{
	ProbeSectorInvArt();
	return g_inv_is_wf ? g_inv_layout_wf : g_inv_layout_vanilla;
}

INT32 GetMapInventoryPoolSlotCount(void)
{
	SectorInvLayout const& L = InvLayout();
	return L.rows * L.cols;
}

static SectorInvTransform InvTransform(void)
{
	SectorInvLayout const& L = InvLayout();
	if (!g_inv_is_wf)
	{
		return { STD_SCREEN_X, STD_SCREEN_Y, L.box.w, L.box.h, L.box.w, L.box.h };
	}

	/* Center the inventory over the strategic map. The panel intentionally
	 * obscures the map; its old right-panel centering left the map exposed. */
	INT32 const map_x = g_ui.get_MAP_VIEW_START_X();
	INT32 const map_y = g_ui.get_MAP_VIEW_START_Y();
	INT32 const map_w = g_ui.get_MAP_VIEW_WIDTH();
	INT32 const map_h = g_ui.get_MAP_VIEW_HEIGHT();
	INT32 const avail_w = SCREEN_WIDTH - map_x;
	INT32 const avail_h = SCREEN_HEIGHT - 121;
	INT32 w = L.box.w;
	INT32 h = L.box.h;

	if (!g_ui.isMapFullSize() && (w > avail_w || h > avail_h))
	{
		if (L.box.w * avail_h <= avail_w * L.box.h)
		{
			h = avail_h;
			w = L.box.w * h / L.box.h;
		}
		else
		{
			w = avail_w;
			h = L.box.h * w / L.box.w;
		}
	}

	return {
		map_x + (map_w - w) / 2,
		std::max<INT32>(0, map_y + (map_h - h) / 2),
		w, h, L.box.w, L.box.h
	};
}

static INT32 InvRenderOriginX(void)
{
	SectorInvTransform const T = InvTransform();
	return T.IsScaled() ? 0 : T.x;
}

static INT32 InvRenderOriginY(void)
{
	SectorInvTransform const T = InvTransform();
	return T.IsScaled() ? 0 : T.y;
}

static SGPFont InvTextFont(void)
{
	/* Match the readable finance figures on map_screen_bottom.sti. */
	return InvTransform().IsScaled() ? COMPFONT : SMALLCOMPFONT;
}

// inventory pool list
std::vector<WORLDITEM> pInventoryPoolList;

// current page of inventory
INT32 iCurrentInventoryPoolPage = 0;
static INT32 iLastInventoryPoolPage = 0;

INT16 sObjectSourceGridNo = 0;

// the inventory slots
static MOUSE_REGION MapInventoryPoolSlots[MAP_INVENTORY_POOL_SLOT_COUNT_MAX];
static MOUSE_REGION MapInventoryPoolMask;
BOOLEAN fMapInventoryItemCompatable[ MAP_INVENTORY_POOL_SLOT_COUNT_MAX ];
static BOOLEAN      fChangedInventorySlots = FALSE;

// the unseen items list...have to save this
static std::vector<WORLDITEM> pUnSeenItems;

UINT32 guiFlashHighlightedItemBaseTime = 0;
UINT32 guiCompatibleItemBaseTime = 0;

static GUIButtonRef guiMapInvenButton[3];

static BOOLEAN gfCheckForCursorOverMapSectorInventoryItem = FALSE;


// remove background panel graphics for inventory
void RemoveInventoryPoolGraphic( void )
{
	RemoveVObject(guiMapInventoryPoolBackground);
}


static void CheckAndUnDateSlotAllocation(void);
static void DisplayCurrentSector(void);
static void DisplayPagesForMapInventoryPool(void);
static void DrawNumberOfInventoryPoolItems();
static void DrawTextOnMapInventoryBackground(void);
static void RenderItemsForCurrentPageOfInventoryPool(void);
static void UpdateHelpTextForInvnentoryStashSlots(void);
static size_t GetTotalNumberOfItemsInSectorStash(void);
static void DrawScaledInventoryText(SectorInvTransform const& T);

static void DrawScaledInventoryText(SectorInvTransform const& T)
{
	SectorInvLayout const& L = InvLayout();
	/* guiSAVEBUFFER is the visible destination of the stretched panel. */
	SetFontDestBuffer(guiSAVEBUFFER);
	SetFontAttributes(COMPFONT, FONT_WHITE);

	auto print = [&T](SGPBox const& box, ST::string const& text)
	{
		MPrint(T.X(box.x), T.Y(box.y), text,
			HCenterVCenterAlign(T.W(box.x, box.w), T.H(box.y, box.h)));
	};
	print(L.title_box, zMarksMapScreenText[11]);
	print(L.loc_box, ST::format("{}{}{}", pMapVertIndex[sSelMap.y], pMapHortIndex[sSelMap.x], pMapDepthIndex[iCurrentMapSectorZ]));
	print(L.count_box, ST::string::from_uint(GetTotalNumberOfItemsInSectorStash()));
	print(L.page_box, ST::format("{} / {}", iCurrentInventoryPoolPage + 1, iLastInventoryPoolPage + 1));

	SetFontAttributes(COMPFONT, FONT_BEIGE);
	auto print_label = [&T](INT32 const x, INT32 const w, INT32 const y, ST::string const& text)
	{
		INT32 const right = T.X(x + w);
		MPrint(std::min(T.X(x), right - StringPixLength(text, COMPFONT)), T.Y(y), text);
	};
	print_label(L.label1_x, L.label1_w, L.label1_y, pMapInventoryStrings[0]);
	print_label(L.label2_x, L.label2_w, L.label1_y, pMapInventoryStrings[1]);

	SetFontAttributes(COMPFONT, FONT_WHITE);
	for (INT32 i = 0; i < MAP_INVENTORY_POOL_SLOT_COUNT; ++i)
	{
		WORLDITEM const& item = pInventoryPoolList[iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT + i];
		if (item.o.ubNumberOfObjects == 0) continue;
		INT32 const sx = i / MAP_INV_SLOT_ROWS;
		INT32 const sy = i % MAP_INV_SLOT_ROWS;
		INT32 const x = L.slot_box.x + sx * L.slot_box.w;
		INT32 const y = L.slot_box.y + sy * L.slot_box.h;
		ST::string const name = ReduceStringLength(GCM->getItem(item.o.usItem)->getShortName(), L.name_box.w, COMPFONT);
		MPrint(T.X(x + L.name_box.x), T.Y(y + L.name_box.y), name,
			HCenterVCenterAlign(T.W(x + L.name_box.x, L.name_box.w), T.H(y + L.name_box.y, L.name_box.h)));
	}

	SetFontDestBuffer(FRAME_BUFFER);
}

namespace {
// Print text horizontally and vertically centered inside box
// x and y are added to the box's x and y.
void MPrintCenteredInBox(int x, int y, ST::string const& text, SGPBox const& box)
{
	MPrint(x + box.x, y + box.y, text, HCenterVCenterAlign(box.w, box.h));
}
}

static void DrawScaledInventoryButton(SGPVSurface* const compose, GUIButtonRef const button,
	cache_key_t const& art, UINT16 const normal, UINT16 const pressed, UINT16 const grayed,
	INT32 const native_x, INT32 const native_y)
{
	UINT16 const subregion = !button->Enabled() ? grayed : button->Clicked() ? pressed : normal;
	BltVideoObject(compose, art, subregion, native_x, native_y);
}

// blit the background panel for the inventory
void BlitInventoryPoolGraphic( void )
{
	SectorInvTransform const T = InvTransform();
	std::unique_ptr<SGPVSurface> compose;
	SGPVSurface* old_save = NULL;
	SGPVSurface* old_frame = NULL;
	SGPRect old_clip;

	if (T.IsScaled())
	{
		compose = std::make_unique<SGPVSurface>(
			static_cast<UINT16>(std::max<INT32>(SCREEN_WIDTH, T.native_w)),
			static_cast<UINT16>(std::max<INT32>(SCREEN_HEIGHT, T.native_h)), 16);
		compose->Fill(0);

		SGPRect compose_clip;
		compose_clip.set(0, 0, compose->Width(), compose->Height());
		old_clip = SetClippingRect(compose_clip);

		old_save = guiSAVEBUFFER;
		old_frame = FRAME_BUFFER;
		SaveFontSettings();
		guiSAVEBUFFER = compose.get();
		g_frame_buffer = compose.get();
		SetFontDestBuffer(compose.get());
		BltVideoObject(compose.get(), guiMapInventoryPoolBackground, 0, 0, 0);
	}
	else
	{
		BltVideoObject(guiSAVEBUFFER, guiMapInventoryPoolBackground, 0,
			T.x + InvLayout().box.x, T.y + InvLayout().box.y);
	}

	CheckAndUnDateSlotAllocation();
	RenderItemsForCurrentPageOfInventoryPool();
	UpdateHelpTextForInvnentoryStashSlots();
	if (!T.IsScaled())
	{
		DisplayPagesForMapInventoryPool();
		DrawNumberOfInventoryPoolItems();
		DisplayCurrentSector();
		DrawTextOnMapInventoryBackground();
	}
	HandleButtonStatesWhileMapInventoryActive();

	if (T.IsScaled())
	{
		SectorInvLayout const& L = InvLayout();
		DrawScaledInventoryButton(compose.get(), guiMapInvenButton[0],
			guiMapInventoryPoolArrows, 1, 3, 10, L.next_x, L.next_y);
		DrawScaledInventoryButton(compose.get(), guiMapInvenButton[1],
			guiMapInventoryPoolArrows, 0, 2, 9, L.prev_x, L.prev_y);
		DrawScaledInventoryButton(compose.get(), guiMapInvenButton[2],
			guiMapInventoryPoolDone, 0, 1, 0, L.done_x, L.done_y);

		guiSAVEBUFFER = old_save;
		g_frame_buffer = old_frame;
		RestoreFontSettings();
		SetClippingRect(old_clip);

		SGPBox const src{ 0, 0, static_cast<UINT16>(T.native_w), static_cast<UINT16>(T.native_h) };
		SGPBox const dst{ static_cast<UINT16>(T.x), static_cast<UINT16>(T.y),
			static_cast<UINT16>(T.w), static_cast<UINT16>(T.h) };
		BltStretchVideoSurface(guiSAVEBUFFER, compose.get(), &src, &dst);
		RestoreExternBackgroundRect(T.x, T.y, T.w, T.h);
		DrawScaledInventoryText(T);
	}
	else
	{
		MarkButtonsDirty();
	}
}


static BOOLEAN RenderItemInPoolSlot(INT32 iCurrentSlot, INT32 iFirstSlotOnPage);


static void RenderItemsForCurrentPageOfInventoryPool(void)
{
	INT32 iCounter = 0;

	// go through list of items on this page and place graphics to screen
	for( iCounter = 0; iCounter < MAP_INVENTORY_POOL_SLOT_COUNT ; iCounter++ )
	{
		RenderItemInPoolSlot( iCounter, ( iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT ) );
	}
}


static BOOLEAN RenderItemInPoolSlot(INT32 iCurrentSlot, INT32 iFirstSlotOnPage)
{
	// render item in this slot of the list
	const WORLDITEM& item = pInventoryPoolList[iCurrentSlot + iFirstSlotOnPage];

	// check if anything there
	if (item.o.ubNumberOfObjects == 0) return FALSE;

	const SGPBox* const slot_box = &InvLayout().slot_box;
	const INT32 dx = InvRenderOriginX() + slot_box->x + slot_box->w * (iCurrentSlot / MAP_INV_SLOT_ROWS);
	const INT32 dy = InvRenderOriginY() + slot_box->y + slot_box->h * (iCurrentSlot % MAP_INV_SLOT_ROWS);

	SetFontDestBuffer(guiSAVEBUFFER);
	const SGPBox* const item_box = &InvLayout().item_box;
	const UINT16        outline  = fMapInventoryItemCompatable[iCurrentSlot] ? Get16BPPColor(FROMRGB(255, 255, 255)) : SGP_TRANSPARENT;
	INVRenderItem(guiSAVEBUFFER, NULL, item.o, dx + item_box->x, dy + item_box->y, item_box->w, item_box->h, DIRTYLEVEL2, 0, outline);

	// draw bar for condition
	const UINT16 col0 = Get16BPPColor(DESC_STATUS_BAR);
	const UINT16 col1 = Get16BPPColor(DESC_STATUS_BAR_SHADOW);
	auto const& bar_box = InvLayout().bar_box;
	DrawItemUIBarEx(item.o, 0, dx + bar_box.x, dy + bar_box.y + bar_box.h - 1, bar_box.h, col0, col1, guiSAVEBUFFER);

	// if the item is not reachable, or if the selected merc is not in the current sector
	const SOLDIERTYPE* const s = GetSelectedInfoChar();
	if (!(item.usFlags & WORLD_ITEM_REACHABLE) ||
			s           == NULL     ||
			s->sSector.x != sSelMap.x ||
			s->sSector.y != sSelMap.y ||
			s->sSector.z != iCurrentMapSectorZ)
	{
		//Shade the item
		DrawHatchOnInventory(guiSAVEBUFFER, dx + item_box->x, dy + item_box->y, item_box->w, item_box->h);
	}

	// the name
	if (!InvTransform().IsScaled())
	{
		const SGPBox* const name_box = &InvLayout().name_box;
		SGPFont const font = InvTextFont();
		auto sString = ReduceStringLength(GCM->getItem(item.o.usItem)->getShortName(), name_box->w, font);

		SetFontAttributes(font, FONT_WHITE);
		MPrintCenteredInBox(dx, dy, sString, *name_box);
		SetFontDestBuffer(FRAME_BUFFER);
	}

	return TRUE;
}


static void UpdateHelpTextForInvnentoryStashSlots(void)
{
	ST::string pStr;
	INT32 iCounter = 0;
	INT32 iFirstSlotOnPage = ( iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT );


	// run through list of items in slots and update help text for mouse regions
	for( iCounter = 0; iCounter < MAP_INVENTORY_POOL_SLOT_COUNT; iCounter++ )
	{
		ST::string help;
		OBJECTTYPE const& o    = pInventoryPoolList[iCounter + iFirstSlotOnPage].o;
		if  (o.ubNumberOfObjects > 0)
		{
			pStr = GetHelpTextForItem(o);
			help = pStr;
		}
		MapInventoryPoolSlots[iCounter].SetFastHelpText(help);
	}
}


static void BuildStashForSelectedSector(const SGPSector& sector);
static void CreateMapInventoryButtons(void);
static void CreateMapInventoryPoolDoneButton(void);
static void CreateMapInventoryPoolSlots(void);
static void DestroyInventoryPoolDoneButton(void);
static void DestroyMapInventoryButtons(void);
static void DestroyMapInventoryPoolSlots();
static void DestroyStash(void);
static void HandleMapSectorInventory(void);
static void SaveSeenAndUnseenItems(void);


// create and remove buttons for inventory
void CreateDestroyMapInventoryPoolButtons( BOOLEAN fExitFromMapScreen )
{
	static BOOLEAN fCreated = FALSE;

/* player can leave items underground, no?
	if( iCurrentMapSectorZ )
	{
		fShowMapInventoryPool = FALSE;
	}
*/
	auto const& sector{ sSelMap };
	if (fShowMapInventoryPool && !fCreated)
	{
		if (gWorldSector == sector)
		{
			// handle all reachable before save
			HandleAllReachAbleItemsInTheSector(gWorldSector);
		}

		// destroy buttons for map border
		DeleteMapBorderButtons( );

		fCreated = TRUE;

		// also create the inventory slot
		CreateMapInventoryPoolSlots( );

		// create buttons
		CreateMapInventoryButtons( );

		// build stash
		BuildStashForSelectedSector(sector);

		CreateMapInventoryPoolDoneButton( );

		fMapPanelDirty = TRUE;
		fMapScreenBottomDirty = TRUE;
	}
	else if (!fShowMapInventoryPool && fCreated)
	{

		// check fi we are in fact leaving mapscreen
		if (!fExitFromMapScreen)
		{
			// recreate mapborder buttons
			CreateButtonsForMapBorder( );
		}
		fCreated = FALSE;

		// destroy the map inventory slots
		DestroyMapInventoryPoolSlots( );

		// destroy map inventory buttons
		DestroyMapInventoryButtons( );

		DestroyInventoryPoolDoneButton( );

		// now save results
		SaveSeenAndUnseenItems( );

		DestroyStash( );



		fMapPanelDirty = TRUE;
		fTeamPanelDirty = TRUE;
		fCharacterInfoPanelDirty = TRUE;

		//DEF: added to remove the 'item blip' from staying on the radar map
		iCurrentlyHighLightedItem = -1;

		// re render radar map
		RenderRadarScreen( );
	}

	// do our handling here
	HandleMapSectorInventory( );

}


void CancelSectorInventoryDisplayIfOn( BOOLEAN fExitFromMapScreen )
{
	if ( fShowMapInventoryPool )
	{
		// get rid of sector inventory mode & buttons
		fShowMapInventoryPool = FALSE;
		CreateDestroyMapInventoryPoolButtons( fExitFromMapScreen );
	}
}


static size_t GetTotalNumberOfItems(void);
static void ReBuildWorldItemStashForLoadedSector(const std::vector<WORLDITEM>& pSeenItemsList, const std::vector<WORLDITEM>& pUnSeenItemsList);


static void SaveSeenAndUnseenItems(void)
{
	// if there are seen items, build a temp world items list of them and save them
	std::vector<WORLDITEM> pSeenItemsList;
	for (WORLDITEM& pi : pInventoryPoolList)
	{
		if (pi.o.ubNumberOfObjects == 0) continue;

		WORLDITEM si = pi;
		if (si.sGridNo == 0)
		{
			// Use gridno of predecessor, if there is one
			if (pSeenItemsList.size() != 0)
			{
				// borrow from predecessor
				si.sGridNo = pSeenItemsList.back().sGridNo;
			}
			else
			{
				// get entry grid location
			}
		}
		si.fExists = TRUE;
		si.bVisible = TRUE;
		pSeenItemsList.push_back(si);
	}

	// if this is the loaded sector handle here
	auto const& sector{ sSelMap };
	if (gWorldSector == sector)
	{
		ReBuildWorldItemStashForLoadedSector(pSeenItemsList, pUnSeenItems);
	}
	else
	{
		// now copy over unseen and seen
		SaveWorldItemsToTempItemFile(sector, pUnSeenItems);
		AddWorldItemsToUnLoadedSector(sector, pSeenItemsList);
	}
}


static void InventoryNextPage()
{
	if (iCurrentInventoryPoolPage < iLastInventoryPoolPage)
	{
		++iCurrentInventoryPoolPage;
		fMapPanelDirty = TRUE;
	}
}


static void InventoryPrevPage()
{
	if (iCurrentInventoryPoolPage > 0)
	{
		--iCurrentInventoryPoolPage;
		fMapPanelDirty = TRUE;
	}
}


// the screen mask bttn callaback...to disable the inventory and lock out the map itself
static void MapInvenPoolScreenMaskCallbackSecondary(MOUSE_REGION* pRegion, UINT32 iReason)
{
	fShowMapInventoryPool = FALSE;
}

static void MapInvenPoolScreenMaskCallbackScroll(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		InventoryPrevPage();
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		InventoryNextPage();
	}
}


static void MapInvenPoolSlotsPrimary(MOUSE_REGION* pRegion, UINT32 iReason);
static void MapInvenPoolSlotsSecondary(MOUSE_REGION* pRegion, UINT32 iReason);
static void MapInvenPoolSlotsScroll(MOUSE_REGION* pRegion, UINT32 iReason);
static void MapInvenPoolSlotsMove(MOUSE_REGION* pRegion, UINT32 iReason);


static void CreateMapInventoryPoolSlots(void)
{
	SectorInvTransform const T = InvTransform();
	{
		SGPBox const& box = InvLayout().box;
		INT32 const x = T.X(box.x);
		INT32 const y = T.Y(box.y);
		INT32 const w = T.W(box.x, box.w);
		INT32 const h = T.H(box.y, box.h);
		MSYS_DefineRegion(&MapInventoryPoolMask, x, y, x + w - 1, y + h - 1, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MouseCallbackPrimarySecondary(MSYS_NO_CALLBACK, MapInvenPoolScreenMaskCallbackSecondary, MapInvenPoolScreenMaskCallbackScroll));
	}

	SGPBox const& slot_box = InvLayout().slot_box;
	SGPBox const& reg_box  = InvLayout().region_box;
	for (INT32 i = 0; i < MAP_INVENTORY_POOL_SLOT_COUNT; ++i)
	{
		INT32 const sx = i / MAP_INV_SLOT_ROWS;
		INT32 const sy = i % MAP_INV_SLOT_ROWS;
		INT32 const native_x = reg_box.x + slot_box.x + sx * slot_box.w;
		INT32 const native_y = reg_box.y + slot_box.y + sy * slot_box.h;
		INT32 const x = T.X(native_x);
		INT32 const y = T.Y(native_y);
		INT32 const w = T.W(native_x, reg_box.w);
		INT32 const h = T.H(native_y, reg_box.h);
		MOUSE_REGION* const r = &MapInventoryPoolSlots[i];
		MSYS_DefineRegion(r, x, y, x + w - 1, y + h - 1, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MapInvenPoolSlotsMove, MouseCallbackPrimarySecondary(MapInvenPoolSlotsPrimary, MapInvenPoolSlotsSecondary, MapInvenPoolSlotsScroll));
		MSYS_SetRegionUserData(r, 0, i);
	}
}


static void DestroyMapInventoryPoolSlots()
{
	/* SECTORINV-GRID: chi go dung so vung da tao, khong quet ca mang _MAX */
	for (INT32 i = 0; i < MAP_INVENTORY_POOL_SLOT_COUNT; ++i)
	{
		MSYS_RemoveRegion(&MapInventoryPoolSlots[i]);
	}
	MSYS_RemoveRegion(&MapInventoryPoolMask);
}


static void MapInvenPoolSlotsMove(MOUSE_REGION* pRegion, UINT32 iReason)
{
	INT32 iCounter = 0;


	iCounter = MSYS_GetRegionUserData( pRegion, 0 );

	if( iReason & MSYS_CALLBACK_REASON_GAIN_MOUSE )
	{
		iCurrentlyHighLightedItem = iCounter;
		fChangedInventorySlots = TRUE;
		gfCheckForCursorOverMapSectorInventoryItem = TRUE;
	}
	else if( iReason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		iCurrentlyHighLightedItem = -1;
		fChangedInventorySlots = TRUE;
		gfCheckForCursorOverMapSectorInventoryItem = FALSE;

		// re render radar map
		RenderRadarScreen( );
	}
}


static void BeginInventoryPoolPtr(OBJECTTYPE* pInventorySlot);
static BOOLEAN CanPlayerUseSectorInventory(void);
static BOOLEAN PlaceObjectInInventoryStash(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr);


static void MapInvenPoolSlotsPrimary(MOUSE_REGION* const pRegion, const UINT32 iReason)
{
	// check if item in cursor, if so, then swap, and no item in curor, pick up, if item in cursor but not box, put in box
	INT32      const slot_idx = MSYS_GetRegionUserData(pRegion, 0);
	WORLDITEM& slot = pInventoryPoolList[iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT + slot_idx];

	// Return if empty
	if (gpItemPointer == NULL && slot.o.usItem == NOTHING) return;

	// is this item reachable
	if (slot.o.usItem != NOTHING && !(slot.usFlags & WORLD_ITEM_REACHABLE))
	{
		// not reachable
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, gzLateLocalizedString[STR_LATE_38], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	// Valid character?
	const SOLDIERTYPE* const s = GetSelectedInfoChar();
	if (s == NULL)
	{
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, pMapInventoryErrorString[0], MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	// Check if selected merc is in this sector, if not, warn them and leave
	if (s->sSector.x != sSelMap.x           ||
			s->sSector.y != sSelMap.y           ||
			s->sSector.z != iCurrentMapSectorZ ||
			s->fBetweenSectors)
	{
		ST::string msg = (gpItemPointer == NULL ? pMapInventoryErrorString[1] : pMapInventoryErrorString[4]);
		ST::string buf = st_format_printf(msg, s->name);
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, buf, MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	// If in battle inform player they will have to do this in tactical
	if (!CanPlayerUseSectorInventory())
	{
		ST::string msg = (gpItemPointer == NULL ? pMapInventoryErrorString[2] : pMapInventoryErrorString[3]);
		DoMapMessageBox(MSG_BOX_BASIC_STYLE, msg, MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
		return;
	}

	// If we do not have an item in hand, start moving it
	if (gpItemPointer == NULL)
	{
		sObjectSourceGridNo = slot.sGridNo;
		BeginInventoryPoolPtr(&slot.o);
	}
	else
	{
		const INT32 iOldNumberOfObjects = slot.o.ubNumberOfObjects;

		// Else, try to place here
		if (PlaceObjectInInventoryStash(&slot.o, gpItemPointer))
		{
			// nothing here before, then place here
			if (iOldNumberOfObjects == 0)
			{
				slot.sGridNo                  = sObjectSourceGridNo;
				slot.ubLevel                  = s->bLevel;
				slot.usFlags                  = 0;
				slot.bRenderZHeightAboveLevel = 0;

				if (sObjectSourceGridNo == NOWHERE)
				{
					slot.usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;
				}
			}

			slot.usFlags |= WORLD_ITEM_REACHABLE;

			// Check if it's the same now!
			if (gpItemPointer->ubNumberOfObjects == 0)
			{
				MAPEndItemPointer();
			}
			else
			{
				SetMapCursorItem();
			}
		}
	}

	// dirty region, force update
	fMapPanelDirty = TRUE;
}

static void MapInvenPoolSlotsSecondary(MOUSE_REGION* const pRegion, const UINT32 iReason)
{
	if (gpItemPointer == NULL) fShowMapInventoryPool = FALSE;
}

static void MapInvenPoolSlotsScroll(MOUSE_REGION* const pRegion, const UINT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		InventoryPrevPage();
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		InventoryNextPage();
	}
}


static void MapInventoryPoolPrevBtn(GUI_BUTTON* btn, UINT32 reason);
static void MapInventoryPoolNextBtn(GUI_BUTTON* btn, UINT32 reason);


static void CreateMapInventoryButtons(void)
{
	SectorInvLayout const& L = InvLayout();
	SectorInvTransform const T = InvTransform();
	if (T.IsScaled())
	{
		ETRLEObject const& next = GetVObject(guiMapInventoryPoolArrows)->SubregionProperties(1);
		ETRLEObject const& prev = GetVObject(guiMapInventoryPoolArrows)->SubregionProperties(0);
		guiMapInvenButton[0] = CreateHotSpot(T.X(L.next_x), T.Y(L.next_y), T.W(L.next_x, next.usWidth), T.H(L.next_y, next.usHeight), MSYS_PRIORITY_HIGHEST, MapInventoryPoolNextBtn);
		guiMapInvenButton[1] = CreateHotSpot(T.X(L.prev_x), T.Y(L.prev_y), T.W(L.prev_x, prev.usWidth), T.H(L.prev_y, prev.usHeight), MSYS_PRIORITY_HIGHEST, MapInventoryPoolPrevBtn);
	}
	else
	{
		guiMapInvenButton[0] = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti", 10, 1, -1, 3, -1, T.x + L.next_x, T.y + L.next_y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolNextBtn);
		guiMapInvenButton[1] = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti",  9, 0, -1, 2, -1, T.x + L.prev_x, T.y + L.prev_y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolPrevBtn);
	}

	//reset the current inventory page to be the first page
	iCurrentInventoryPoolPage = 0;
}


static void DestroyMapInventoryButtons(void)
{
	RemoveButton( guiMapInvenButton[ 0 ] );
	RemoveButton( guiMapInvenButton[ 1 ] );
}


static void CheckGridNoOfItemsInMapScreenMapInventory(void);
static void SortSectorInventory(WORLDITEM* pInventory, size_t sizeOfArray);


static void BuildStashForSelectedSector(const SGPSector& sector)
{
	std::vector<WORLDITEM> temp;
	std::vector<WORLDITEM>* items = nullptr;
	if (sector == gWorldSector)
	{
		items = &gWorldItems;
	}
	else
	{
		temp = LoadWorldItemsFromTempItemFile(sector);
		items = &temp;
	}

	pInventoryPoolList.clear();
	pUnSeenItems.clear();

	for (const WORLDITEM& wi : *items)
	{
		if (!wi.fExists) continue;
		if (IsMapScreenWorldItemVisibleInMapInventory(wi))
		{
			pInventoryPoolList.push_back(wi);
		}
		else
		{
			pUnSeenItems.push_back(wi);
		}
	}

	size_t visible_slots = pInventoryPoolList.size();
	size_t empty_slots = MAP_INVENTORY_POOL_SLOT_COUNT - visible_slots % MAP_INVENTORY_POOL_SLOT_COUNT;
	pInventoryPoolList.resize(visible_slots + empty_slots, WORLDITEM{});
	iLastInventoryPoolPage  = static_cast<INT32>((pInventoryPoolList.size() - 1) / MAP_INVENTORY_POOL_SLOT_COUNT);

	CheckGridNoOfItemsInMapScreenMapInventory();
	SortSectorInventory(pInventoryPoolList.data(), visible_slots);
}


static void ReBuildWorldItemStashForLoadedSector(const std::vector<WORLDITEM>& pSeenItemsList, const std::vector<WORLDITEM>& pUnSeenItemsList)
{
	TrashWorldItems();

	std::vector<WORLDITEM> pTotalList;
	pTotalList.insert(pTotalList.end(), pSeenItemsList.begin(), pSeenItemsList.end());
	pTotalList.insert(pTotalList.end(), pUnSeenItemsList.begin(), pUnSeenItemsList.end());

	size_t remainder = pTotalList.size() % 10;
	if (remainder)
	{
		pTotalList.insert(pTotalList.end(), 10 - remainder, WORLDITEM{});
	}

	RefreshItemPools(pTotalList);

	//Count the total number of visible items
	UINT32 uiTotalNumberOfVisibleItems = 0;
	for (const WORLDITEM& si : pSeenItemsList)
	{
		uiTotalNumberOfVisibleItems += si.o.ubNumberOfObjects;
	}

	//reset the visible item count in the sector info struct
	SetNumberOfVisibleWorldItemsInSectorStructureForSector(gWorldSector, uiTotalNumberOfVisibleItems);
}


static void DestroyStash(void)
{
	// clear out stash
	pInventoryPoolList.clear();
	pUnSeenItems.clear();
}


static BOOLEAN GetObjFromInventoryStashSlot(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr);
static BOOLEAN RemoveObjectFromStashSlot(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr);


static void BeginInventoryPoolPtr(OBJECTTYPE* pInventorySlot)
{
	BOOLEAN fOk = FALSE;

	// If not null return
	if ( gpItemPointer != NULL )
	{
		return;
	}

	// if shift key get all

	if (_KeyDown( SHIFT ))
	{
		// Remove all from soldier's slot
		fOk = RemoveObjectFromStashSlot( pInventorySlot, &gItemPointer );
	}
	else
	{
		GetObjFromInventoryStashSlot( pInventorySlot, &gItemPointer );
		fOk = (gItemPointer.ubNumberOfObjects == 1);
	}

	if (fOk)
	{
		// Dirty interface
		fMapPanelDirty = TRUE;
		SetItemPointer(&gItemPointer, 0);
		SetMapCursorItem();

		if (fShowInventoryFlag)
		{
			SOLDIERTYPE* const s = GetSelectedInfoChar();
			if (s != NULL)
			{
				ReevaluateItemHatches(s, FALSE);
				fTeamPanelDirty = TRUE;
			}
		}
	}
}


// get this item out of the stash slot
static BOOLEAN GetObjFromInventoryStashSlot(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr)
{
	// item ptr
	if (!pItemPtr )
	{
		return( FALSE );
	}

	// if there are only one item in slot, just copy
	if (pInventorySlot->ubNumberOfObjects == 1)
	{
		*pItemPtr = *pInventorySlot;
		DeleteObj( pInventorySlot );
	}
	else
	{
		// take one item
		pItemPtr->usItem = pInventorySlot->usItem;

		// find first unempty slot
		pItemPtr->bStatus[0] = pInventorySlot->bStatus[0];
		pItemPtr->ubNumberOfObjects = 1;
		RemoveObjFrom( pInventorySlot, 0 );
	}

	return ( TRUE );
}


static BOOLEAN RemoveObjectFromStashSlot(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr)
{
	if (pInventorySlot -> ubNumberOfObjects == 0)
	{
		return( FALSE );
	}
	else
	{
		*pItemPtr = *pInventorySlot;
		DeleteObj( pInventorySlot );
		return( TRUE );
	}
}


static BOOLEAN PlaceObjectInInventoryStash(OBJECTTYPE* pInventorySlot, OBJECTTYPE* pItemPtr)
{
	UINT8 ubNumberToDrop, ubSlotLimit, ubLoop;

	// if there is something there, swap it, if they are of the same type and stackable then add to the count

	ubSlotLimit = GCM->getItem(pItemPtr -> usItem)->getPerPocket();

	if (pInventorySlot->ubNumberOfObjects == 0)
	{
		// placement in an empty slot
		ubNumberToDrop = pItemPtr->ubNumberOfObjects;

		if (ubNumberToDrop > ubSlotLimit && ubSlotLimit != 0)
		{
			// drop as many as possible into pocket
			ubNumberToDrop = ubSlotLimit;
		}

		// could be wrong type of object for slot... need to check...
		// but assuming it isn't
		*pInventorySlot = *pItemPtr;

		if (ubNumberToDrop != pItemPtr->ubNumberOfObjects)
		{
			// in the InSlot copy, zero out all the objects we didn't drop
			for (ubLoop = ubNumberToDrop; ubLoop < pItemPtr->ubNumberOfObjects; ubLoop++)
			{
				pInventorySlot->bStatus[ubLoop] = 0;
			}
		}
		pInventorySlot->ubNumberOfObjects = ubNumberToDrop;

		// remove a like number of objects from pObj
		RemoveObjs( pItemPtr, ubNumberToDrop );
	}
	else
	{
		// replacement/reloading/merging/stacking

		// placement in an empty slot
		ubNumberToDrop = pItemPtr->ubNumberOfObjects;

		if (pItemPtr->usItem == pInventorySlot->usItem)
		{
			if (pItemPtr->usItem == MONEY)
			{
				// always allow money to be combined!
				// status of money is always 100
				pInventorySlot->bMoneyStatus = 100;
				pInventorySlot->uiMoneyAmount += pItemPtr->uiMoneyAmount;

				DeleteObj( pItemPtr );
			}
			else if (ubSlotLimit < 2)
			{
				// swapping
				SwapObjs( pItemPtr, pInventorySlot );
			}
			else
			{
				// stacking
				if( ubNumberToDrop > ubSlotLimit - pInventorySlot -> ubNumberOfObjects )
				{
					ubNumberToDrop = ubSlotLimit - pInventorySlot -> ubNumberOfObjects;
				}

				StackObjs( pItemPtr, pInventorySlot, ubNumberToDrop );
			}
		}
		else
		{

				SwapObjs( pItemPtr, pInventorySlot );
		}
	}
	return( TRUE );
}


void AutoPlaceObjectInInventoryStash(OBJECTTYPE* pItemPtr)
{
	UINT8 ubNumberToDrop, ubSlotLimit, ubLoop;
	OBJECTTYPE *pInventorySlot;


	// if there is something there, swap it, if they are of the same type and stackable then add to the count
	pInventorySlot =  &( pInventoryPoolList[ pInventoryPoolList.size() ].o );// FIXME out of bounds access

	// placement in an empty slot
	ubNumberToDrop = pItemPtr->ubNumberOfObjects;

	ubSlotLimit = ItemSlotLimit( pItemPtr->usItem, BIGPOCK1POS );

	if (ubNumberToDrop > ubSlotLimit && ubSlotLimit != 0)
	{
		// drop as many as possible into pocket
		ubNumberToDrop = ubSlotLimit;
	}

	// could be wrong type of object for slot... need to check...
	// but assuming it isn't
	*pInventorySlot = *pItemPtr;

	if (ubNumberToDrop != pItemPtr->ubNumberOfObjects)
	{
		// in the InSlot copy, zero out all the objects we didn't drop
		for (ubLoop = ubNumberToDrop; ubLoop < pItemPtr->ubNumberOfObjects; ubLoop++)
		{
			pInventorySlot->bStatus[ubLoop] = 0;
		}
	}
	pInventorySlot->ubNumberOfObjects = ubNumberToDrop;

	// remove a like number of objects from pObj
	RemoveObjs( pItemPtr, ubNumberToDrop );
}


static void MapInventoryPoolNextBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		InventoryNextPage();
	}
}


static void MapInventoryPoolPrevBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		InventoryPrevPage();
	}
}


static void MapInventoryPoolDoneBtn(GUI_BUTTON* btn, UINT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
	{
		fShowMapInventoryPool = FALSE;
	}
}


static void DisplayPagesForMapInventoryPool(void)
{
	// get the current and last pages and display them
	SetFontAttributes(InvTextFont(), 183);
	SetFontDestBuffer(guiSAVEBUFFER);

	MPrintCenteredInBox(InvRenderOriginX(), InvRenderOriginY(),
		ST::format("{} / {}", iCurrentInventoryPoolPage + 1, iLastInventoryPoolPage + 1),
		InvLayout().page_box);

	SetFontDestBuffer(FRAME_BUFFER);
}


static size_t GetTotalNumberOfItemsInSectorStash(void)
{
	size_t numObjects = 0;

	// run through list of items and find out how many are there
	for (WORLDITEM& wi : pInventoryPoolList)
	{
		if (wi.o.ubNumberOfObjects > 0)
		{
			numObjects += wi.o.ubNumberOfObjects;
		}
	}

	return numObjects;
}


// get total number of items in sector
static size_t GetTotalNumberOfItems(void)
{
	size_t numSlots = 0;

	// run through list of items and find out how many are there
	for (WORLDITEM& wi : pInventoryPoolList)
	{
		if (wi.o.ubNumberOfObjects > 0)
		{
			numSlots++;
		}
	}

	return numSlots;
}


static void DrawNumberOfInventoryPoolItems()
{
	SetFontAttributes(InvTextFont(), 183);
	SetFontDestBuffer(guiSAVEBUFFER);

	MPrintCenteredInBox(InvRenderOriginX(), InvRenderOriginY(),
		ST::string::from_uint(GetTotalNumberOfItemsInSectorStash()),
		InvLayout().count_box);

	SetFontDestBuffer(FRAME_BUFFER);
}


static void CreateMapInventoryPoolDoneButton(void)
{
	// create done button
	SectorInvLayout const& L = InvLayout();
	SectorInvTransform const T = InvTransform();
	if (T.IsScaled())
	{
		ETRLEObject const& done = GetVObject(guiMapInventoryPoolDone)->SubregionProperties(0);
		guiMapInvenButton[2] = CreateHotSpot(T.X(L.done_x), T.Y(L.done_y), T.W(L.done_x, done.usWidth), T.H(L.done_y, done.usHeight), MSYS_PRIORITY_HIGHEST, MapInventoryPoolDoneBtn);
	}
	else
	{
		guiMapInvenButton[2] = QuickCreateButtonImg(INTERFACEDIR "/done_button.sti", 0, 1, T.x + L.done_x, T.y + L.done_y, MSYS_PRIORITY_HIGHEST, MapInventoryPoolDoneBtn);
	}
}


static void DestroyInventoryPoolDoneButton(void)
{
	// destroy ddone button
	RemoveButton( guiMapInvenButton[ 2 ] );
}


static void DisplayCurrentSector(void)
{
	// grab current sector being displayed
	SetFontAttributes(InvTextFont(), 183);
	SetFontDestBuffer(guiSAVEBUFFER);

	MPrintCenteredInBox(InvRenderOriginX(), InvRenderOriginY(),
		ST::format("{}{}{}", pMapVertIndex[ sSelMap.y ],
			pMapHortIndex[ sSelMap.x ], pMapDepthIndex[ iCurrentMapSectorZ ]),
		InvLayout().loc_box);

	SetFontDestBuffer(FRAME_BUFFER);
}


static void CheckAndUnDateSlotAllocation(void)
{
	// will check number of available slots, if less than half a page, allocate a new page
	size_t numTakenSlots = GetTotalNumberOfItems();

	if ((pInventoryPoolList.size() - numTakenSlots) < 2)
	{
		// not enough space
		// need to make more space
		pInventoryPoolList.insert(pInventoryPoolList.end(), MAP_INVENTORY_POOL_SLOT_COUNT, WORLDITEM{});
	}

	iLastInventoryPoolPage = ( ( static_cast<INT32>(pInventoryPoolList.size()) - 1 ) / MAP_INVENTORY_POOL_SLOT_COUNT );
}


static void DrawTextOnSectorInventory(void);


static void DrawTextOnMapInventoryBackground(void)
{
	UINT16 usStringHeight;

	SetFontDestBuffer(guiSAVEBUFFER);

	SectorInvLayout const& L = InvLayout();
	SGPFont const font = InvTextFont();
	int xPos = InvRenderOriginX() + L.label1_x;
	int yPos = InvRenderOriginY() + L.label1_y;

	//Calculate the height of the string, as it needs to be vertically centered.
	usStringHeight = DisplayWrappedString(xPos, yPos, L.label1_w, 1, font, FONT_BEIGE, pMapInventoryStrings[0], FONT_BLACK, RIGHT_JUSTIFIED | DONT_DISPLAY_TEXT);
	DisplayWrappedString(xPos, yPos - (usStringHeight / 2), L.label1_w, 1, font, FONT_BEIGE, pMapInventoryStrings[0], FONT_BLACK, RIGHT_JUSTIFIED);

	xPos = InvRenderOriginX() + L.label2_x;

	//Calculate the height of the string, as it needs to be vertically centered.
	usStringHeight = DisplayWrappedString(xPos, yPos, L.label2_w, 1, font, FONT_BEIGE, pMapInventoryStrings[1], FONT_BLACK, RIGHT_JUSTIFIED | DONT_DISPLAY_TEXT);
	DisplayWrappedString( xPos, yPos - (usStringHeight / 2), L.label2_w, 1, font, FONT_BEIGE, pMapInventoryStrings[1], FONT_BLACK, RIGHT_JUSTIFIED);

	DrawTextOnSectorInventory( );

	SetFontDestBuffer(FRAME_BUFFER);
}


void HandleButtonStatesWhileMapInventoryActive( void )
{
	// are we even showing the amp inventory pool graphic?
	if (!fShowMapInventoryPool) return;

	// first page, can't go back any
	EnableButton(guiMapInvenButton[1], iCurrentInventoryPoolPage != 0);
	// last page, go no further
	EnableButton(guiMapInvenButton[0], iCurrentInventoryPoolPage != iLastInventoryPoolPage);
	// item picked up ..disable button
	EnableButton(guiMapInvenButton[2], !fMapInventoryItem);
}


static void DrawTextOnSectorInventory(void)
{
	// Prints "Sector Inventory" in the English localization.

	SetFontDestBuffer(guiSAVEBUFFER);
	SetFontAttributes(InvTransform().IsScaled() ? COMPFONT : FONT14ARIAL, FONT_WHITE);

	MPrintCenteredInBox(InvRenderOriginX(), InvRenderOriginY(),
		zMarksMapScreenText[11], InvLayout().title_box);

	SetFontDestBuffer(FRAME_BUFFER);
}


void HandleFlashForHighLightedItem( void )
{
	UINT32 uiCurrentTime = 0;
	INT32 iDifference = 0;


	// if there is an invalid item, reset
	if( iCurrentlyHighLightedItem == -1 )
	{
		fFlashHighLightInventoryItemOnradarMap = FALSE;
		guiFlashHighlightedItemBaseTime = 0;
	}

	// get the current time
	uiCurrentTime = GetJA2Clock();

	// if there basetime is uninit
	if( guiFlashHighlightedItemBaseTime == 0 )
	{
		guiFlashHighlightedItemBaseTime = uiCurrentTime;
	}


	iDifference = uiCurrentTime - guiFlashHighlightedItemBaseTime;

	if( iDifference > DELAY_FOR_HIGHLIGHT_ITEM_FLASH )
	{
		// reset timer
		guiFlashHighlightedItemBaseTime = uiCurrentTime;

		// flip flag
		fFlashHighLightInventoryItemOnradarMap = !fFlashHighLightInventoryItemOnradarMap;

		// re render radar map
		RenderRadarScreen( );

	}
}


static void ResetMapSectorInventoryPoolHighLights();


static void HandleMouseInCompatableItemForMapSectorInventory(INT32 iCurrentSlot)
{
	SOLDIERTYPE *pSoldier = NULL;
	static BOOLEAN fItemWasHighLighted = FALSE;

	if( iCurrentSlot == -1 )
	{
		guiCompatibleItemBaseTime = 0;
	}

	if (fChangedInventorySlots)
	{
		guiCompatibleItemBaseTime = 0;
		fChangedInventorySlots = FALSE;
	}

	// reset the base time to the current game clock
	if( guiCompatibleItemBaseTime == 0 )
	{
		guiCompatibleItemBaseTime = GetJA2Clock( );

		if (fItemWasHighLighted)
		{
			fTeamPanelDirty = TRUE;
			fMapPanelDirty = TRUE;
			fItemWasHighLighted = FALSE;
		}
	}

	ResetCompatibleItemArray( );
	ResetMapSectorInventoryPoolHighLights( );

	if( iCurrentSlot == -1 )
	{
		return;
	}

	// given this slot value, check if anything in the displayed sector inventory or on the mercs inventory is compatable
	if( fShowInventoryFlag )
	{
		// check if any compatable items in the soldier inventory matches with this item
		if( gfCheckForCursorOverMapSectorInventoryItem )
		{
			const SOLDIERTYPE* const pSoldier = GetSelectedInfoChar();
			if( pSoldier )
			{
				if( HandleCompatibleAmmoUIForMapScreen( pSoldier, iCurrentSlot + ( iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT ), TRUE, FALSE ) )
				{
					if( GetJA2Clock( ) - guiCompatibleItemBaseTime > 100 )
					{
						if (!fItemWasHighLighted)
						{
							fTeamPanelDirty = TRUE;
							fItemWasHighLighted = TRUE;
						}
					}
				}
			}
		}
		else
		{
			guiCompatibleItemBaseTime = 0;
		}
	}


	// now handle for the sector inventory
	if( fShowMapInventoryPool )
	{
		// check if any compatable items in the soldier inventory matches with this item
		if( gfCheckForCursorOverMapSectorInventoryItem )
		{
			if( HandleCompatibleAmmoUIForMapInventory( pSoldier, iCurrentSlot, ( iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT ) , TRUE, FALSE ) )
			{
				if( GetJA2Clock( ) - guiCompatibleItemBaseTime > 100 )
				{
					if (!fItemWasHighLighted)
					{
						fItemWasHighLighted = TRUE;
						fMapPanelDirty = TRUE;
					}
				}
			}
		}
		else
		{
			guiCompatibleItemBaseTime = 0;
		}
	}
}


static void ResetMapSectorInventoryPoolHighLights()
{ // Reset the highlight list for the map sector inventory.
	FOR_EACH(BOOLEAN, i, fMapInventoryItemCompatable) *i = FALSE;
}


static void HandleMapSectorInventory(void)
{
	// handle mouse in compatable item map sectors inventory
	HandleMouseInCompatableItemForMapSectorInventory( iCurrentlyHighLightedItem );
}


//CJC look here to add/remove checks for the sector inventory
BOOLEAN IsMapScreenWorldItemVisibleInMapInventory(const WORLDITEM& wi)
{
	if (wi.fExists             &&
			wi.bVisible == VISIBLE &&
			wi.o.usItem != SWITCH &&
			wi.o.usItem != ACTION_ITEM &&
			wi.o.bTrap <= 0 )
	{
		return( TRUE );
	}

	return( FALSE );
}


//Check to see if any of the items in the list have a gridno of NOWHERE and the entry point flag NOT set
static void CheckGridNoOfItemsInMapScreenMapInventory(void)
{
	size_t uiNumFlagsNotSet = 0;
	size_t numTakenSlots = GetTotalNumberOfItems();


	for (size_t iCnt = 0; iCnt < numTakenSlots; iCnt++)// FIXME this only works properly when the taken slots are continuous
	{
		if( pInventoryPoolList[ iCnt ].sGridNo == NOWHERE && !( pInventoryPoolList[ iCnt ].usFlags & WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT ) )
		{
			//set the flag
			pInventoryPoolList[ iCnt ].usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;

			//count the number
			uiNumFlagsNotSet++;
		}
	}


	//loop through all the UNSEEN items
	for (size_t iCnt = 0; iCnt < pUnSeenItems.size(); iCnt++)
	{
		if( pUnSeenItems[ iCnt ].sGridNo == NOWHERE && !( pUnSeenItems[ iCnt ].usFlags & WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT ) )
		{
			//set the flag
			pUnSeenItems[ iCnt ].usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;

			//count the number
			uiNumFlagsNotSet++;
		}
	}

	if( uiNumFlagsNotSet > 0 )
	{
		SLOGD("Item with invalid gridno doesnt have flag set: {}", uiNumFlagsNotSet);
	}
}


static INT32 MapScreenSectorInventoryCompare(const void* pNum1, const void* pNum2);


static void SortSectorInventory(WORLDITEM* pInventory, size_t sizeOfArray)
{
	qsort(pInventory, sizeOfArray, sizeof(WORLDITEM), MapScreenSectorInventoryCompare);
}


static INT32 MapScreenSectorInventoryCompare(const void* pNum1, const void* pNum2)
{
	WORLDITEM *pFirst = (WORLDITEM *)pNum1;
	WORLDITEM *pSecond = (WORLDITEM *)pNum2;
	UINT16	usItem1Index;
	UINT16	usItem2Index;
	UINT8		ubItem1Quality;
	UINT8		ubItem2Quality;

	usItem1Index = pFirst->o.usItem;
	usItem2Index = pSecond->o.usItem;

	ubItem1Quality = pFirst->o.bStatus[ 0 ];
	ubItem2Quality = pSecond->o.bStatus[ 0 ];

	return( CompareItemsForSorting( usItem1Index, usItem2Index, ubItem1Quality, ubItem2Quality ) );
}


static BOOLEAN CanPlayerUseSectorInventory(void)
{
	SGPSector sector;
	return
		!GetCurrentBattleSectorXYZAndReturnTRUEIfThereIsABattle(sector) ||
		sSelMap.x           != sector.x ||
		sSelMap.y           != sector.y ||
		iCurrentMapSectorZ != sector.z;
}
