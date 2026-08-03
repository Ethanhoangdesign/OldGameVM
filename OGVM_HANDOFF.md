# OGVM-UILAYOUT Handoff

## Repo
- Branch: feature/multi-edition-detector
- Build: cmake --build build -j8 && ./build/ja2

## Da xong
1. Xoa OGVM re-blit block (shift=332) - doc art[830+] OOB tren art 640px
2. Button offsets WF dung: SM_DONE_X=+46, SM_MAPSCREEN_X=+92, RADAR_WINDOW_X=+45
3. Dead strip fill code: SGPVSurface::Lock blit tu deadzone_strip.h (52x140 RGB565) vao surface[640..692]

## BUG CON LAI - Strip van hien mau TAN
### Nguyen nhan nghi ngo
Line ~914 Interface_Panels.cc:
  BltVideoObject(guiSMPanel, voSMPanel, 0, g_ui.m_teamPanelWidth - artWidth, 0);
  // destX=52, khong co clip -> ghi art[588..640] (TAN) vao surface[640..692]
Code Lock blit o line ~934 chay SAU nhung strip van TAN.

### Fix de xuat (don gian nhat)
Them clip truoc blit line 914:
  SGPRect c; c.set(0, 0, 640, INV_INTERFACE_HEIGHT);
  SGPRect old2 = SetClippingRect(c);
  BltVideoObject(guiSMPanel, voSMPanel, 0, g_ui.m_teamPanelWidth - artWidth, 0);
  SetClippingRect(old2);
Roi ColorFillVideoSurfaceArea(guiSMPanel, 640, 0, 692, INV_INTERFACE_HEIGHT, 0x1060)

## Files
- src/game/Tactical/Interface_Panels.cc (tim OGVM-UILAYOUT)
- src/game/UILayout.cc / UILayout.h
- src/game/Tactical/deadzone_strip.h (file moi, 52x140 texture)

## Constants
- WF art=640px, panel surface=692px, dead strip=52px
- TEAMPANEL_BUTTONSBOX_WIDTH_WF=194, TEAMPANEL_SLOT_WIDTH=83
- Dead zone avg color = 0x1060 (RGB 16,12,0)

## Con lai
- Text positions: A9 Omerta va Day 1 07:01 lech trai
- CLOCK_X WF offset (hien=109)
- Windows MSVC build crash
- OGVM-SHORTERR: rut gon result.reasons
