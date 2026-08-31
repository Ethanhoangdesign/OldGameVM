#ifndef _MAP_INTERFACE_MAP_INVEN_H
#define _MAP_INTERFACE_MAP_INVEN_H

#include "Types.h"
#include "World_Items.h"

#include <vector>

// number of inventory slots
/* SECTORINV-GRID: so o moi trang phu thuoc bo art dang dung.
 * Art vanilla (379x360) co 5x9 = 45 o; art Wildfire (763x647)
 * co 5x10 = 50 o. Mang tinh cap phat theo so TOI DA, con vong
 * lap va phep chia trang dung gia tri runtime ben duoi. */
#define MAP_INVENTORY_POOL_SLOT_COUNT_MAX 50

// so o thuc te cua bo art dang dung (45 hoac 50)
INT32 GetMapInventoryPoolSlotCount(void);

#define MAP_INVENTORY_POOL_SLOT_COUNT (GetMapInventoryPoolSlotCount())

// whether we are showing the inventory pool graphic
extern BOOLEAN fShowMapInventoryPool;

// remove inventory pool graphic
void RemoveInventoryPoolGraphic( void );

// blit the inventory graphic
void BlitInventoryPoolGraphic( void );

// which buttons in map invneotyr panel?
void HandleButtonStatesWhileMapInventoryActive( void );

// handle creation and destruction of map inventory pool buttons
void CreateDestroyMapInventoryPoolButtons( BOOLEAN fExitFromMapScreen );

// bail out of sector inventory mode if it is on
void CancelSectorInventoryDisplayIfOn( BOOLEAN fExitFromMapScreen );

// handle flash of inventory items
void HandleFlashForHighLightedItem( void );

// the list for the inventory
extern std::vector<WORLDITEM> pInventoryPoolList;

// autoplace down object
void AutoPlaceObjectInInventoryStash(OBJECTTYPE* pItemPtr);

// the current inventory item
extern INT32 iCurrentlyHighLightedItem;
extern BOOLEAN fFlashHighLightInventoryItemOnradarMap;
extern INT16 sObjectSourceGridNo;
extern INT32 iCurrentInventoryPoolPage;
extern BOOLEAN fMapInventoryItemCompatable[ ];

BOOLEAN IsMapScreenWorldItemVisibleInMapInventory(const WORLDITEM& wi);

#endif
