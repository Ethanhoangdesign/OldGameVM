# OGVM-UILAYOUT Handoff

Cap nhat: 2026-08-03
Branch: `feature/multi-edition-detector`
Trang thai: **DA FIX XONG va verify bang mat tren ca HAI panel, 1366x768, JA2 Wildfire.**

---

## 1. Tom tat

DA XONG:

- SM panel (inventory 1 linh): het dai ho 52px mau tan o rim phai.
- TEAM panel (doi hinh): minimap, ten khu vuc, dong ho nam dung trong buttons box.
- Chuyen qua lai giua hai panel: moi thu nhay dung vi tri.
- `deadzone_strip.h` da bi xoa (hack cu, khong con can).
- Build macOS OK.

CON LAI (chua dung toi):

- Windows MSVC build crash.
- OGVM-SHORTERR: rut gon `result.reasons`.

---

## 2. DOC PHAN NAY TRUOC KHI SUA BAT KY OFFSET NAO O BOTTOM BAR

Bug nay ngon 3 ngay va bi revert di revert lai. Ly do khong phai vi kho, ma vi
**no trong giong mot bug nhung thuc ra la hai**, va sua cai nay thi hong cai kia.

### 2.1 Hai panel, hai bo art, hai buttons box

Thanh bottom bar co hai che do, dung hai file art KHAC NHAU:

| Panel | Art | Buttons box |
|---|---|---|
| SM panel (inventory 1 linh) | `interface/inventory_bottom_panel.sti` (**vanilla**) | **142px** |
| TEAM panel (doi hinh) | `interface/bottom_bar.sti` (**Wildfire**) | **194px** |

SM panel dung art vanilla vi entry tuong ung trong `InterFace.slf` cua Wildfire co
`len = 0` (thieu file). Commit `22c7cff` da import art vanilla tu ban JA2 Mac DMG
de vao cho do. Vi vay:

> **`getTeamPanelButtonsBoxWidth()` (tra ve 194 voi WF) mo ta TEAM panel.
> No KHONG mo ta SM panel. SM panel luon luon la 142.**

Day la cau quan trong nhat trong file nay.

### 2.2 Con so 52 o dau ra

```
194 (box WF)  -  142 (box vanilla)  =  52
```

Moi lan ban thay con so 52 hoac mot cho lech 52px o bottom bar, no la con so nay.
Khong phai trung hop.

@1366x768, `squad_size` 6:

```
getTeamPanelNumSlots()        = 6
m_teamPanelSlotsTotalWidth    = 6 * 83 = 498
getTeamPanelButtonsBoxWidth() = 194        (WF)
m_teamPanelWidth              = 498 + 194 = 692   <- TEAM panel
SM panel dung                 = 498 + 142 = 640   <- bang dung artWidth
```

### 2.3 Cai bay: hai he toa do cho cung mot widget

`get_RADAR_WINDOW_X()`, `get_CLOCK_X()` va `RenderTownIDString()` duoc **dung chung
cho ca hai panel**. Chung do tu `m_teamPanelSlotsTotalWidth`, tuc tu **mep TRAI cua
buttons box**. Nhung buttons box cua ca hai panel deu can **sat mep PHAI** cua panel,
nen mep trai cua chung lech nhau dung 52px.

He qua: **khong co gia tri co dinh nao dung cho ca hai panel.**

| Offset | SM panel | TEAM panel |
|---|---|---|
| radar 45 / town 50 / clock 56 | DUNG | lech trai 52px |
| radar 97 / town 102 / clock 108 | dai ho 52px | DUNG |

AI truoc do lap di lap lai vong nay: sua thanh 98, thay TEAM panel dep, commit
(`0100d0e`); sau do mo inventory thay hong, revert ve 45 (`69aed3f`); sau do mo
doi hinh thay hong, lai doi... Moi lan chi mo MOT panel de kiem tra nen khong bao
gio nhin thay ca hai cung luc.

### 2.4 Loi giai

Dieu kien phan nhanh phai la **panel nao dang hien thi**, khong phai **art edition
nao dang load**:

```cpp
UINT16 UILayout::activeButtonsBoxShift() const
{
	if (gsCurInterfacePanel == SM_PANEL) return 0;
	return getTeamPanelButtonsBoxWidth() - TEAMPANEL_BUTTONSBOX_WIDTH;
}
```

`gsCurInterfacePanel` khai bao trong `src/game/Tactical/Interface.h`
(`enum InterfacePanelKind { SM_PANEL, TEAM_PANEL, NUM_UI_PANELS }`).

Moi thu nam BEN TRONG buttons box phai cong them ham nay.
Moi thu neo vao MEP TRAI buttons box (vi du cac nut TEAM: `TM_ENDTURN_X` =
`slots + 9`) thi KHONG duoc cong -- chung von da dung.

---

## 3. Nhung gi da thay doi

### 3.1 `src/game/UILayout.h`

- Them khai bao `UINT16 activeButtonsBoxShift() const;` kem comment day du.

### 3.2 `src/game/UILayout.cc`

- Them `#include "Interface.h"` (de lay `gsCurInterfacePanel`, `SM_PANEL`).
- Them dinh nghia `UILayout::activeButtonsBoxShift()`.
- `get_CLOCK_X()` -> nhanh tactical thanh `slots + 56 + activeButtonsBoxShift()`.
  (truoc do la `... ? 109 : 56`)
- `get_RADAR_WINDOW_X()` -> `slots + 45 + activeButtonsBoxShift()`.
  (truoc do la nhanh chet `... ? 45 : 45`, tan tich cua lan revert)

### 3.3 `src/game/Tactical/Interface_Panels.cc`

**a) Xoa dau `}` thua trong `InitializeSMPanel()`.**

Commit `e2f159f` (+522/-4) de lot mot dau ngoac nhon thua. **File khong compile duoc.**
Day la ly do that su khien mot ngay bi mat: nguoi test dang chay binary CU, nen moi
sua deu "khong co tac dung", va handoff cu ket luan nham rang "code Lock blit chay SAU
nhung strip van TAN".

Cach kiem chung nhanh, khong can compile:

```bash
python3 - <<'PY'
import re
s = open('src/game/Tactical/Interface_Panels.cc').read()
s = re.sub(r'/\*.*?\*/', '', s, flags=re.S)
s = re.sub(r'//[^\n]*', '', s)
s = re.sub(r'"(\\.|[^"\\])*"', '""', s)
s = re.sub(r"'(\\.|[^'\\])*'", "''", s)
print('delta', s.count('{') - s.count('}'))   # phai bang 0
PY
```

Truoc khi sua: `delta -1`. Sau khi sua: `delta 0`.

**b) Them `GetSMPanelWidth()`** -- nguon su that duy nhat cho be rong SM panel:

```cpp
static UINT16 GetSMPanelWidth()
{
	return g_ui.m_teamPanelSlotsTotalWidth + TEAMPANEL_BUTTONSBOX_WIDTH;  // 142, KHONG phai 194
}
```

Truoc day `InitializeSMPanel()` cap phat surface theo `m_teamPanelWidth` (692) trong
khi art chi rong 640 -> chua bao gio co gi ve len dai 52px cuoi -> dai ho.

- 6 slot: 498 + 142 = 640 = `artWidth` -> mot blit, khong ho, `sFillerWidth = 0`.
- 8 slot: 664 + 142 = 806 > 640 -> double blit can phai + `DrawFillerOnSurface()`
  lap seam o giua (thuat toan vanilla goc, van dung).

**c) Bo hardcode 640 + them clip.** `sFillerWidth` gio tinh tu `artWidth` that.
Them `SetClippingRect` quanh double blit vi art WF co ban rong 1024px, blit khong
clip se ghi tran surface (bus error).

**d) Xoa toan bo khoi `SGPVSurface::Lock` + `kDeadZoneStrip`** va
`#include "deadzone_strip.h"`. File `src/game/Tactical/deadzone_strip.h` (459 dong)
da bi xoa khoi repo.

**e) `FillEmptySpaceAtBottom(UINT16 panelWidth)`** -- gio nhan tham so.
SM mode truyen 640, TEAM mode truyen `m_teamPanelWidth` (692).

**f) `SM_DONE_X` / `SM_MAPSCREEN_X`** -- bo nhanh `m_teamPanelWidth >= 748`.
Dieu kien do chua bao gio dung (692 < 748) nen hanh vi runtime khong doi; chi la bo
mot cai bay. Gio la `slots + 46` va `slots + 92`, dung o moi so slot vi buttons box
luon can phai.

**g) `RenderTownIDString()`** -> `slots + 50 + g_ui.activeButtonsBoxShift()`.

**h) Them log chan doan** dau `InitializeSMPanel()`:

```
OGVM-UILAYOUT SM panel: artWidth=640 smPanelWidth=640 slotsTotal=498 teamPanelWidth=692 buttonsBox=194
```

Doc bon so nay TRUOC khi chinh bat ky offset nao.

---

## 4. Bang tra offset (sau khi fix)

Tat ca do tu `INTERFACE_START_X + m_teamPanelSlotsTotalWidth`.

| Widget | Offset | Cong shift? | Panel |
|---|---|---|---|
| `get_RADAR_WINDOW_X()` | +45 | CO | ca hai |
| `RenderTownIDString()` | +50 | CO | ca hai |
| `get_CLOCK_X()` | +56 | CO | ca hai |
| `SM_DONE_X` | +46 | khong | chi SM |
| `SM_MAPSCREEN_X` | +92 | khong | chi SM |
| `TM_ENDTURN_X` | +9 | khong | chi TEAM |
| `TM_ROSTERMODE_X` | +9 | khong | chi TEAM |
| `TM_DISK_X` | +9 | khong | chi TEAM |

`shift` = 0 o SM panel, = 52 o TEAM panel voi art WF.

---

## 5. Quy trinh bat buoc cho lan sau

1. **Compile truoc khi ket luan bat cu dieu gi tu anh chup man hinh.**
   Neu build fail thi ban dang nhin binary cu. Kiem tra can ngoac (muc 3.3a) neu
   nghi ngo.
2. **Luon test CA HAI panel.** Bam mo inventory 1 linh, roi bam ve doi hinh.
   Mot panel dep khong co nghia la da xong.
3. **Khong hardcode be rong panel.** Doc tu `artWidth`
   (`SubregionProperties(0).usWidth`) hoac tu `GetSMPanelWidth()`.
4. **Khong branch tren `getTeamPanelButtonsBoxWidth()` de dinh vi widget.**
   No mo ta art edition, khong mo ta panel dang ve. Dung `activeButtonsBoxShift()`.
5. **Thay so 52 thi dung doan.** Do la `194 - 142`. Tim xem cho nao dang tron hai
   he toa do.
6. Test them o `squad_size` khac 6 (vi du 8) de chac nhanh double-blit con dung.

---

## 6. Lich su commit lien quan

| Commit | Noi dung |
|---|---|
| `81960562` | OGVM-WIDEBG: background 1366/1024 |
| `46a6583e` | OGVM-FLUENT2-DARK theme |
| `0100d0ea` | fix WF inventory SM panel layout (dat he toa do B: DONE +97, MAPSCREEN +143) |
| `a832f025` | fix WF tactical panel layout (radar, map screen) |
| `22c7cff4` | import art vanilla cho SM panel tu JA2 Mac DMG (`Interface.slf`, ENTRY_SIZE=280) |
| `e2f159f3` | WF SM panel fixes + handoff -- **chua dau `}` thua, file khong compile** |
| `69aed3f5` | revert `get_RADAR_WINDOW_X()` 98 -> 45 ("caused segfault") |

Ghi chu: cai segfault o `69aed3f` la trieu chung cua viec cap phat surface theo 692
trong khi art chi 640, khong phai do con so 98. Revert con so do chi giau bug di.
