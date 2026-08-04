# OGVM — Handoff: Sector Inventory (04/08/2026)

Repo: `Ethanhoangdesign/OldGameVM`
Branch: `feature/multi-edition-detector`
Local: `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella`
Build: `cmake --build build -j8 && ./build/ja2`
Cau hinh dang test: `res 1024x768`, `fullscreen false`, `scaling NEAR_PERFECT`
Game dang load: `/Users/ethan/Documents/GOG/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)`

---

## 0. TRANG THAI

| Viec | Trang thai |
|---|---|
| Patch 1 — deadzone strip o SM panel | DONE, da push |
| Patch 2 — lech 52px giua SM va TEAM panel | DONE, da push |
| Patch 3 — sector inventory khong hien ra | DONE, user xac nhan |
| Patch 4 — nen go quanh bang inventory | **DA GO** (thua sau patch 5) |
| Patch 5 — ke do bi zoom to + canh le | DONE, user xac nhan bang mat |
| Windows MSVC build crash | Chua lam |
| OGVM-SHORTERR (rut gon `result.reasons`) | Chua lam |

Man hinh sector inventory o che do Wildfire full-size hien da dung: bang phu
kin vung ban do, luoi 5x10 = 50 o, chu va nut nam dung khung, thanh tinh
trang lot dung ranh lom.

---

## 1. PHAT HIEN QUAN TRONG NHAT: HEADER STCI KHONG PHAI KICH THUOC VE

Day la goc cua toan bo bug "ke do bi zoom to", va la thu da lam 4 lan chan
doan truoc that bai.

**`BltVideoObject(vo, 0, x, y)` ve SUBREGION 0. Kich thuoc that nam o
`SubregionProperties(0)`, KHONG phai o header STCI.**

Do bang header se ra so DANH NGHIA, vo nghia:

| Bo art | header STCI | **sub[0] = kich thuoc VE** |
|---|---|---|
| vanilla GOG | 380 x 360 | **379 x 360** |
| Wildfire 6.08 | 380 x 360 | **763 x 647** |

Hai bo art khac nhau GAP DOI nhung header ghi Y HET NHAU. Bang chung phu
trong cung kho: `INVENTOR.sti` cung "bao" 640x480 nhung chi nang 2KB.

Dau hieu de nhan ra som: **file 499KB thi khong the la anh 380x360.** Neu
kich thuoc doc duoc khong hop ly voi kich thuoc file, la dang doc nham cho.

Cong cu dung: `tools/check_sector_inv_layout.py` (doc sub[0], khong doc header).

---

## 2. PATCH 5 — SECTORINV-GRID (viec chinh cua phien nay)

### Trieu chung

Bang inventory ve to gap ~2 lan, tran qua mep phai va xuong duoi. Nhung
popup huong dan trong CUNG cua so lai co chu nho binh thuong.

Chi tiet doi chieu do da loai tru duoc mot nhanh gia thuyet ma khong can
build lai: neu ca cua so bi scale thi popup cung phai to theo. Popup binh
thuong ma luoi o to => **rieng art cua bang bi ve to**, khong phai ca man
hinh bi scale.

### Nguyen nhan

Art Wildfire that su la 763x647 (xem muc 1), trong khi moi hang so trong
`Map_Screen_Interface_Map_Inventory.cc` gan cung theo art vanilla 379x360.

Con so khop hoan hao:

| | |
|---|---|
| Art WF that | 763 x 647 |
| Vung ban do full-size | 763 x 648 |
| Khung code gia dinh | 379 x 360 |
| Ti le | **2.01x** — dung "to gap 2 lan" |

Art WF duoc thiet ke de PHU KIN vung ban do.

### Cach sua

Doc kich thuoc that luc runtime roi chon bo layout, thay vi hardcode. Code
phai chay dung voi CA HAI bo art nen khong duoc gan cung so cua WF.

Trong `Map_Screen_Interface_Map_Inventory.cc`:

- `ProbeSectorInvArt()` — doc `SubregionProperties(0)` MOT LAN, nguong 512px
  de phan biet (du sac bat cac ban art che lech doi chut)
- `g_inv_layout_vanilla` / `g_inv_layout_wf` — hai bo hang so
- `InvLayout()` — tra ve bo dang dung
- `InvOrigin()` — goc trai-tren cua bang tren man hinh:
  - vanilla: giu nguyen neo cu `STD_SCREEN_X/Y` (khong doi hanh vi da co)
  - WF: neo vao giua vung ban do
- `GetMapInventoryPoolSlotCount()` — so o runtime (45 hoac 50)

Trong `.h`:

```c
#define MAP_INVENTORY_POOL_SLOT_COUNT_MAX 50        // cap phat mang tinh
INT32 GetMapInventoryPoolSlotCount(void);
#define MAP_INVENTORY_POOL_SLOT_COUNT (GetMapInventoryPoolSlotCount())
```

Macro van giu TEN CU nen 6 file dung no khong phai sua gi. Mang tinh
(`MapInventoryPoolSlots[]`, `fMapInventoryItemCompatable[]`) cap theo `_MAX`.

CANH BAO da kiem tra: `BuildStashForSelectedSector()` dung
`empty_slots = COUNT - visible % COUNT` — tu co gian theo count runtime,
khong can sua. `CheckAndUnDateSlotAllocation()` cung vay.

### Hang so WF — TAT CA DEU DO TU ART, khong suy tu ti le

Loat so DAU TIEN toi lam la nhan ti le tu vanilla len. **Sai het.** Phai do
tung cai tu art.

```
luoi   : cot bat dau x=35 buoc 145 (5 cot), hang y=37 buoc 57 (10 hang)
long o : 123 x 48
o day  : flood-fill vung xanh tham (khong doan cua so quet):
           loc   x231..281 y617..636
           count x473..502 y618..632
           page  x552..598 y619..634
           done  x684..721 y622..636
```

### RANH LOM CHO THANH TINH TRANG — cho de sai nhat

Thanh trang o mep trai moi o the hien **do ben cua item**, kha quan trong,
phai lot dung vao ranh lom cua art.

Toi dat sai HAI lan lien tiep vi tim ranh bang truc giac. Cach dung la
**doi chieu voi ban vanilla** — noi hang so goc von da dung:

| | vanilla (moc dung) | WF |
|---|---|---|
| go sang trai | rel +1 (lum 55) | rel −7 (lum 40) |
| **RANH TOI ← thanh vao day** | **rel +2..+3 (lum 13)** | **rel −6..−5 (lum 14)** |
| go sang phai | rel +4..+5 | rel −3..−2 (lum 65) |

**Ranh la vet TOI nam giua hai go SANG, khong phai vet sang.** Lan dau toi
dat vao vet sang (rel −3), lan hai van sai.

Ranh WF nam NGOAI long o (offset x AM) — do la ly do lan dau tim khong ra:
chi quet ben trong o nen khong thay gi.

```c
bar_box: { -6, 2, 2, 43 }
```

Cai bay kieu du lieu: `SGPBox` dung `UINT16`, `-6` se thanh **65530** va
thanh bay khoi man hinh. Rieng `bar_box` phai dung struct co dau `INT16`.

Luu y: `DrawItemUIBarEx()` LUON ve 2px (`LineDraw` tai `x` va `x+1`), bo qua
tham so `w`. Truong `bar_box.w` chi mang tinh tai lieu.

### Patch 4 da bi go

`SECTORINV-BG` lap nen go quanh bang. Sau patch 5 thi thua: art WF tu phu
kin vung ban do, khong con mep ho nao de lap.

---

## 3. PATCH 3 — SECTORINV-FIX (van con hieu luc)

Loi goi ve bang **nam BEN TRONG mot ham cua man hinh khac**.
`Map_Screen_Interface_Border.cc:173`:

```cpp
void RenderMapBorder( void )
{
	if( fShowMapInventoryPool )
	{
		BlitInventoryPoolGraphic( );   // <-- loi goi DUY NHAT trong ca repo
		return;
	}
```

Ma `MapScreen.cc` bo qua ham nay o che do full-size vi khung vien vanilla
khong vua. Tat khung vien => tat luon bang inventory.

| Trieu chung | Nguyen nhan |
|---|---|
| Khong hien bang | Ham ve khong bao gio chay |
| Nhu treo | `MapInventoryPoolMask` voi primary callback `MSYS_NO_CALLBACK` nuot het click trai |
| Chi chuot phai thoat duoc | `MapInvenPoolScreenMaskCallbackSecondary()` la loi thoat duy nhat |

Cach sua: tach loi goi ra khoi `RenderMapBorder()`, dua len
`RenderMapRegionBackground()` de chay cho ca hai che do.

---

## 4. NHUNG LOI CHAN DOAN DA MAC — DUNG LAP LAI

Bug nay bi doan sai **5 lan**. Ghi lai de nguoi sau khong di lai:

1. **"Toa do vanilla roi ra ngoai vung, bi DrawMap() ve de len".** Sai.
   Kiem tra bang dai so thi vi tri vanilla nam chinh giua vung full-size.

2. **"Loi goi `BlitInventoryPoolGraphic()` da bi xoa".** Sai. No nam trong
   `RenderMapBorder()`. Ket luan vay vi chi doc `RenderMapRegionBackground()`
   — suy dien tu mot file khong chua loi goi.

3. **"Art WF lon hon 379x360".** Bi gach bo vi do ra 380x360.
   **Thuc ra DUNG** — nhung do bang header nen ra so danh nghia (muc 1).

4. **"Da chet gia thuyet art lon hon".** Toi tuyen bo vay sau khi do lai
   header lan hai. Van sai vi van do nham cho.

5. **Thanh tinh trang: dat vao vet SANG thay vi ranh TOI.** Sai hai lan
   lien tiep vi tim ranh bang truc giac thay vi doi chieu ban vanilla.

### Bai hoc

1. **`grep -rn` toan `src/` TRUOC khi ket luan.** Dung suy dien tu file dang
   mo. Lan sai 2 ra tu loi nay.
2. **Do dung CHO.** Header STCI khong phai kich thuoc ve. Lan sai 3 va 4 deu
   ra tu day. Neu kich thuoc doc duoc khong hop ly voi kich thuoc FILE, la
   dang doc nham cho.
3. **Do dung THU MUC ma game THAT SU load** — doc `game_dir` trong
   `~/.ja2/ja2.json` truoc.
4. **Kiem tra parser bang mot gia tri da biet.** `b_map.sti` phai ra 714x612.
5. **Doi chieu voi ban da dung.** Khi khong chac cau truc art, do ban vanilla
   — noi hang so goc von dung — roi ap cung tieu chi len ban WF. Day la cach
   duy nhat tim ra ranh lom sau hai lan sai.
6. **Dung suy hang so bang ti le.** Loat so WF dau tien nhan ti le tu vanilla,
   sai het. Phai do tung cai.
7. **Dung doc toa do bang mat tren screenshot.** Tinh tu ma nguon / do tu art
   roi doi chieu, dung lam nguoc lai.
8. **Khong chon mot loi goi ve cua man hinh nay trong ham render cua man hinh
   khac.** Goc cua patch 3.
9. Trong screenshot, tim chi tiet DOI CHIEU trong cung khung hinh. Popup chu
   nho + luoi o to => khong phai ca cua so bi scale.
10. **Khi cua so quet cho truoc, ket qua chi phan chieu chinh no.** Do o day
    bang flood-fill (tu tim cum) thay vi cho truoc vung bao.

---

## 5. CONG CU

`tools/check_sector_inv_layout.py` — do lai art va doi chieu voi hang so
trong `.cc`. Chay duoc, hien pass ca hai bo art:

```bash
python3 tools/check_sector_inv_layout.py \
  "/Users/ethan/Documents/GOG/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)" \
  "/Users/ethan/Documents/GOG/setup_jagged_alliance_2_26614298_gog_v4_(80537)"
```

Chay lai script nay sau moi lan doi hang so layout. No doc sub[0] (dung cho),
tu do luoi, va bao SAI neu lech.

Luu y: `probe-sti.py` cu trong `~/Downloads` bi macOS chan quyen truy cap
(`Operation not permitted`) — dung script trong `tools/` thay the.

---

## 6. QUY UOC SCRIPT PATCH

- Regex voi `\s*` de chiu duoc khac biet tab/space.
- Dau moc rieng (`SECTORINV-FIX`, `SECTORINV-GRID`) de chay lan hai tu nhan
  ra va khong lam gi.
- Luu ban goc `*.ogvm-bak` (da co trong `.gitignore`).
- **Khong ghi file nao** neu bat ky buoc nao that bai.
- Bao loi ro rang khi khong tim thay mau.

### Kiem tra can bang ngoac — SUA LAI SO VOI HANDOFF CU

Handoff cu bao delta phai bang 0. **Sai voi file lon.** `MapScreen.cc` GOC da
co delta = -2 (do `#ifdef` / chuoi co ngoac). Phai so delta TRUOC va SAU:

```python
before = brace_delta(path.read_text())
after  = brace_delta(new_text)
if before != after:   # patch lam lech ngoac
    fail(...)
```

---

## 7. SO LIEU THAM CHIEU

### Tai 1024x768

| | Gia tri |
|---|---|
| `STD_SCREEN_X` | `(1024-640)/2` = 192 |
| `STD_SCREEN_Y` | `(768-480)/2` = 144 |
| `isMapFullSize()` | TRUE |
| Vung ban do full-size | x 261→1024, y 0→~648 |
| Bang inventory WF | 763 x 647, neo giua vung ban do |

### `UILayout.cc` — getter ban do

```cpp
get_MAP_VIEW_START_X() : isMapFullSize() ? 261 + (m_screenWidth - 261 - 714*MapZoomNum()/2)/2
                                        : m_stdScreenOffsetX + 270
get_MAP_VIEW_START_Y() : full-size hug top : m_stdScreenOffsetY + 10
isMapFullSize()        : UsesWildfireInterfaceArt() && w >= 1024 && h >= 768
```

`#define JA2_MAPZOOM_ALLOW_LARGE 0` → `MapZoomNum()` khoa o 2 (MAPZOOM-LOCK).
`UsesWildfireInterfaceArt()` = cache cua `b_map.sti ton tai && !b_map.pcx ton tai`.

### Hang so `UILayout.h`

`INV_INTERFACE_HEIGHT` 140, `TEAMPANEL_HEIGHT` 120,
`TEAMPANEL_SLOT_WIDTH` 83, `TEAMPANEL_BUTTONSBOX_WIDTH` 142,
`TEAMPANEL_BUTTONSBOX_WIDTH_WF` 194, `SM_INVINTERFACE_WIDTH` 532,
`MIN_INTERFACE_WIDTH` 640, `MIN_INTERFACE_HEIGHT` 480,
`NUM_INVENTORY_SLOTS` 19.

### Bug upstream chua sua (khong phai do OGVM)

`AutoPlaceObjectInInventoryStash()` truy cap
`pInventoryPoolList[pInventoryPoolList.size()]` — out of bounds, co san
comment `// FIXME` trong upstream. Xem
`Map_Screen_Interface_Map_Inventory.cc:1039`.

---

## 8. GIT — BAI HOC

Su co cu: `git add -A` quet ca `build-win/` vao commit, GitHub tu choi file
197MB.

`.gitignore` hien da che `/build/`, `/build-win/`, `*.o`, `*.a`, `*.ogvm-bak`.

Truoc moi lan push, luon chay — phai RONG:

```bash
git diff --cached --name-only | grep -E '^(build|build-win)/'
```

**GitHub PAT hien tai READ-ONLY** — moi thao tac ghi file qua API tra ve
`403 Resource not accessible by personal access token`. Patch phai chay
script tai may.

**zsh:** `#` khong phai comment trong zsh tuong tac, va `?` bi glob. Duong dan
game co dau `(` `)` nen LUON boc nhay kep.

---

## 9. VIEC TIEP THEO

1. Windows MSVC build crash.
2. OGVM-SHORTERR: rut gon `result.reasons`.
3. Kiem tra sector inventory tren do phan giai khac (1366x768, 1920x1080) —
   `InvOrigin()` tinh theo `SCREEN_WIDTH/HEIGHT` nen ve ly thuyet tu can
   giua, nhung chua do thuc te.
4. Chua test man hinh nay voi bo art VANILLA o che do full-size (may hien
   dang load ban WF). Nhanh vanilla giu nguyen hang so cu nen khong ky vong
   thay doi, nhung chua chay qua.
