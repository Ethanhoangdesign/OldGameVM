#!/usr/bin/env python3
"""Kiem tra hang so bo cuc sector inventory KHOP voi art that.

Chay:
    python3 tools/check_sector_inv_layout.py <thu-muc-game> [...]

Ly do co file nay: header STCI cua ca hai bo art deu ghi 380x360, nhung
kich thuoc THAT nam o subimage[0] va khac nhau gap doi:

    vanilla  : 379 x 360  -> 5 cot x  9 hang
    Wildfire : 763 x 647  -> 5 cot x 10 hang

Doc nham header la nguyen nhan bug "ke do bi zoom to". Script nay do lai
luoi tu art roi doi chieu voi g_inv_layout_* trong
src/game/Strategic/Map_Screen_Interface_Map_Inventory.cc.
"""
import os
import struct
import sys

SLF_HEADER = 532
STCI_HDR = 64

# Phai khop g_inv_layout_vanilla / g_inv_layout_wf trong file .cc
# probe_x/probe_y: diem quet phai nam GIUA long o dau tien (vung sang),
# khong duoc roi vao khe toi giua cac o.
EXPECTED = {
    (379, 360): dict(name="vanilla", cols=5, rows=9, step_x=72, step_y=32,
                     probe_x=100, probe_y=46),
    (763, 647): dict(name="Wildfire", cols=5, rows=10, step_x=145, step_y=57,
                     probe_x=100, probe_y=60),
}


def slf_find(path, want):
    size = os.path.getsize(path)
    with open(path, "rb") as f:
        n = struct.unpack_from("<i", f.read(SLF_HEADER), 512)[0]
        for esz in (280, 276):
            start = size - n * esz
            if start <= SLF_HEADER:
                continue
            f.seek(start)
            data = f.read(n * esz)
            hit, ok = None, True
            for i in range(n):
                rec = data[i * esz:(i + 1) * esz]
                nm = rec[:256].split(b"\0")[0].decode("latin-1")
                off, ln = struct.unpack_from("<II", rec, 256)
                if off + ln > size:
                    ok = False
                    break
                if nm.replace("\\", "/").split("/")[-1].lower() == want:
                    hit = (off, ln)
            if ok and hit:
                return hit
    return None


def etrle(data, w, h):
    out = bytearray(w * h)
    p = 0
    for y in range(h):
        x = 0
        while True:
            if p >= len(data):
                return out
            c = data[p]
            p += 1
            if c == 0:
                break
            if c & 0x80:
                x += c & 0x7F
            else:
                for _ in range(c):
                    if p < len(data) and x < w:
                        out[y * w + x] = data[p]
                    p += 1
                    x += 1
    return out


def bright_runs(vals, thr, minlen):
    runs, cur = [], None
    for i, v in enumerate(vals):
        if v >= thr:
            cur = i if cur is None else cur
        elif cur is not None:
            if i - 1 - cur >= minlen:
                runs.append((cur, i - 1))
            cur = None
    if cur is not None and len(vals) - 1 - cur >= minlen:
        runs.append((cur, len(vals) - 1))
    return runs


def check(slf):
    hit = slf_find(slf, "sector_inventory.sti")
    if not hit:
        return None
    off, ln = hit
    with open(slf, "rb") as f:
        f.seek(off)
        blob = f.read(ln)

    nsub = struct.unpack_from("<H", blob, 28)[0]
    pal = blob[STCI_HDR:STCI_HDR + 768]
    sb = STCI_HDR + 768
    _, dlen, _, _, sh, sw = struct.unpack_from("<IIhhHH", blob, sb)
    pix = etrle(blob[sb + nsub * 16: sb + nsub * 16 + dlen], sw, sh)

    lut = [(pal[i * 3] * 299 + pal[i * 3 + 1] * 587 + pal[i * 3 + 2] * 114) // 1000
           for i in range(256)]

    exp = EXPECTED.get((sw, sh))
    print(f"\n{slf}")
    print(f"  art that (subimage[0]) : {sw} x {sh}")
    if not exp:
        print("  [?] kich thuoc la, chua co bo hang so tuong ung -> BO QUA")
        return None

    print(f"  nhan dang               : {exp['name']}")

    # Do luoi: quet mot hang giua o dau tien, va mot cot giua o dau tien
    cols = bright_runs([lut[pix[exp["probe_y"] * sw + x]] for x in range(sw)], 55, 8)
    rows = bright_runs([lut[pix[y * sw + exp["probe_x"]]] for y in range(sh)], 55, 8)

    ok = True

    def cmp(label, got, want):
        nonlocal ok
        mark = "OK " if got == want else "SAI"
        if got != want:
            ok = False
        print(f"  {mark} {label:<12} do duoc={got}  mong doi={want}")

    cmp("so cot", len(cols), exp["cols"])
    cmp("so hang", len(rows), exp["rows"])
    if len(cols) > 1:
        cmp("buoc cot", cols[1][0] - cols[0][0], exp["step_x"])
    if len(rows) > 1:
        cmp("buoc hang", rows[1][0] - rows[0][0], exp["step_y"])
    return ok


def main():
    dirs = sys.argv[1:]
    if not dirs:
        print(__doc__)
        sys.exit(2)

    results = []
    for d in dirs:
        for dirpath, _, files in os.walk(d):
            for fn in files:
                if fn.lower() == "interface.slf":
                    results.append(check(os.path.join(dirpath, fn)))

    checked = [r for r in results if r is not None]
    if not checked:
        print("\nKhong do duoc bo art nao (khong thay Interface.slf?).")
        sys.exit(2)
    if all(checked):
        print(f"\nTAT CA KHOP ({len(checked)} bo art).")
    else:
        print("\nCO SAI LECH - hang so trong .cc khong con khop art.")
        sys.exit(1)


main()
