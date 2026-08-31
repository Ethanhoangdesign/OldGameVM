#!/usr/bin/env python3
"""Inventory Wildfire item names and archive metadata without copying assets."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

STCI_HEADER_SIZE = 64
STCI_PALETTE_SIZE = 768
STCI_SUBIMAGE_HEADER_SIZE = 16

ENTRY_SIZE = 280
ITEM_RECORD_SIZE = 800
ITEM_COUNT = 351
ARCHIVES = ("BinaryData.slf", "InterFace.slf", "BigItems.slf")
EXTERNALIZED = ("weapons", "magazines", "items", "armours", "explosives")
ART_ARCHIVE_BY_PREFIX = {
    "interface/": "InterFace.slf",
    "bigitems/": "BigItems.slf",
}
NUMBERED_ART_RE = re.compile(r"(?:^|/)(gun|p[123]item)(\d+)\.sti$", re.IGNORECASE)

# Artwork provenance only. Handoffs and behavior tests do not prove a native
# item-to-frame association; unlisted rows remain unknown.
KNOWN_EVIDENCE = {
    54: {
        "classification": "confirmed",
        "evidence": "Wildfire labeled contact sheet identifies Machete as slot 47",
    },
    66: {
        "classification": "unknown",
        "evidence": "VSS native small/big artwork has not been revalidated",
    },
    77: {
        "classification": "confirmed",
        "evidence": "Wildfire labeled guns contact sheet identifies 4.6mm Magazine as gun slot 73",
    },
    259: {
        "classification": "confirmed",
        "evidence": "Baseline owner of mdguns frame 42 and gun42.sti",
    },
}

KNOWN_BEHAVIOR_EVIDENCE = {
    265: "Wildfire-only .45 silencer compatibility confirmed",
    269: "Wildfire-only P90 silencer compatibility confirmed",
    305: "Wildfire-only MP7 silencer compatibility confirmed",
    306: "Wildfire-only Commando silencer compatibility confirmed",
}


def archive_entries(data: bytes):
    count = struct.unpack_from("<i", data, 512)[0]
    start = len(data) - count * ENTRY_SIZE
    for index in range(count):
        offset = start + index * ENTRY_SIZE
        name = data[offset : offset + 256].split(b"\0", 1)[0].decode("latin1")
        file_offset, length = struct.unpack_from("<II", data, offset + 256)
        state = data[offset + 264]
        if state == 0:
            yield name, file_offset, length


def validate_archive(path: Path, data: bytes) -> list[tuple[str, int, int]]:
    if len(data) < 516:
        raise SystemExit(f"archive too small: {path}")
    count = struct.unpack_from("<i", data, 512)[0]
    if count < 0:
        raise SystemExit(f"invalid archive directory count: {path}")
    start = len(data) - count * ENTRY_SIZE
    if start < 516:
        raise SystemExit(f"invalid archive directory offset: {path}")
    entries = list(archive_entries(data))
    for name, offset, length in entries:
        if offset > len(data) or length > len(data) - offset:
            raise SystemExit(f"invalid archive member bounds: {path}: {name}")
    return entries


def read_member(data: bytes, suffix: str) -> bytes | None:
    suffix = suffix.lower()
    for name, offset, length in archive_entries(data):
        if name.lower().replace("\\", "/").endswith(suffix):
            return data[offset : offset + length]
    return None


def decode_etrle(data: bytes, width: int, height: int) -> tuple[bytearray, bool]:
    pixels = bytearray(width * height)
    position = 0
    for y in range(height):
        x = 0
        while True:
            if position >= len(data):
                return pixels, False
            count = data[position]
            position += 1
            if count == 0:
                break
            if count & 0x80:
                x += count & 0x7F
                if x > width:
                    return pixels, False
                continue
            if x + count > width or position + count > len(data):
                return pixels, False
            start = y * width + x
            pixels[start : start + count] = data[position : position + count]
            position += count
            x += count
    return pixels, True


def write_png(path: Path, rgb: bytes, width: int, height: int) -> None:
    import zlib

    def chunk(kind: bytes, payload: bytes) -> bytes:
        return (struct.pack(">I", len(payload)) + kind + payload +
                struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))

    scanlines = b"".join(b"\0" + rgb[y * width * 3:(y + 1) * width * 3]
                         for y in range(height))
    payload = (b"\x89PNG\r\n\x1a\n" +
               chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
               chunk(b"IDAT", zlib.compress(scanlines, 9)) + chunk(b"IEND", b""))
    path.write_bytes(payload)


LABEL_GLYPHS = {
    "0": ("111", "101", "101", "101", "111"),
    "1": ("010", "110", "010", "010", "111"),
    "2": ("111", "001", "111", "100", "111"),
    "3": ("111", "001", "111", "001", "111"),
    "4": ("101", "101", "111", "001", "001"),
    "5": ("111", "100", "111", "001", "111"),
    "6": ("111", "100", "111", "101", "111"),
    "7": ("111", "001", "010", "010", "010"),
    "8": ("111", "101", "111", "101", "111"),
    "9": ("111", "101", "111", "001", "111"),
    ":": ("000", "010", "000", "010", "000"),
}


def draw_label(rgb: bytearray, image_width: int, x0: int, y0: int, text: str) -> None:
    scale = 2
    for character in text:
        glyph = LABEL_GLYPHS[character]
        for y, row in enumerate(glyph):
            for x, bit in enumerate(row):
                if bit == "0":
                    continue
                for dy in range(scale):
                    for dx in range(scale):
                        offset = ((y0 + y * scale + dy) * image_width + x0 + x * scale + dx) * 3
                        rgb[offset:offset + 3] = b"\xff\xff\xff"
        x0 += 4 * scale


def decode_stci_pixels(blob: bytes, frame_index: int) -> tuple[bytearray, int, int, bytes]:
    metadata = decode_stci_metadata(blob)
    if frame_index < 0 or frame_index >= metadata["subImageCount"]:
        raise ValueError(f"frame index out of range: {frame_index}")
    frame = metadata["frames"][frame_index]
    palette = blob[STCI_HEADER_SIZE:STCI_HEADER_SIZE + STCI_PALETTE_SIZE]
    data_start = STCI_HEADER_SIZE + STCI_PALETTE_SIZE + metadata["subImageCount"] * STCI_SUBIMAGE_HEADER_SIZE
    start = data_start + frame["dataOffset"]
    pixels, valid = decode_etrle(blob[start:start + frame["dataLength"]], frame["width"], frame["height"])
    if not valid:
        raise ValueError(f"invalid ETRLE frame {frame_index}")
    rgb = bytearray(frame["width"] * frame["height"] * 3)
    for index, palette_index in enumerate(pixels):
        rgb[index * 3:index * 3 + 3] = palette[palette_index * 3:palette_index * 3 + 3]
    return rgb, frame["width"], frame["height"], bytes(pixels)


def is_art_candidate(archive_name: str, normalized: str, family: str) -> bool:
    magazine = (
        archive_name == "InterFace.slf" and normalized.endswith("mdp1items.sti")
    ) or (
        archive_name == "BigItems.slf" and
        re.search(r"(?:^|/)p1item\d+\.sti$", normalized) is not None
    )
    guns = (
        archive_name == "InterFace.slf" and normalized.endswith("mdguns.sti")
    ) or (
        archive_name == "BigItems.slf" and
        re.search(r"(?:^|/)gun\d+\.sti$", normalized) is not None
    )
    return (
        (family == "all" and (magazine or guns)) or
        (family == "magazines" and magazine) or
        (family == "guns" and guns)
    )


def art_candidates(archives: dict[str, bytes], family: str = "all") -> list[dict]:
    candidates = []
    for archive_name, archive in archives.items():
        for name, offset, length in archive_entries(archive):
            normalized = name.lower().replace("\\", "/")
            if not is_art_candidate(archive_name, normalized, family):
                continue
            blob = archive[offset:offset + length]
            metadata = decode_stci_metadata(blob)
            frame_indices = range(metadata["subImageCount"]) if archive_name == "InterFace.slf" else (0,)
            for frame_index in frame_indices:
                candidates.append({
                    "archive": archive_name,
                    "member": name,
                    "frame": frame_index,
                    "label": f"{name} frame {frame_index}",
                    "blob": blob,
                })
    return candidates


def write_contact_sheet(archives: dict[str, bytes], output: Path, family: str = "all") -> Path:
    # The JSON sidecar is authoritative: it maps every PNG cell to an exact member/frame.
    candidates = art_candidates(archives, family)
    if not candidates:
        raise ValueError(f"no {family} art candidates found")
    scale = 2
    cell_width, cell_height = 180, 140
    columns = 8
    rows = (len(candidates) + columns - 1) // columns
    sheet_width, sheet_height = cell_width * columns, cell_height * rows
    sheet = bytearray(sheet_width * sheet_height * 3)
    mapping = []
    for slot, candidate in enumerate(candidates):
        pixels, width, height, pixel_indices = decode_stci_pixels(candidate["blob"], candidate["frame"])
        cell_x, cell_y = slot % columns, slot // columns
        x0 = cell_x * cell_width + max(0, (cell_width - width * scale) // 2)
        y0 = cell_y * cell_height + max(0, (cell_height - height * scale) // 2)
        for y in range(height):
            for x in range(width):
                if pixel_indices[y * width + x] == 0:
                    continue
                source = (y * width + x) * 3
                for dy in range(scale):
                    for dx in range(scale):
                        dest_x, dest_y = x0 + x * scale + dx, y0 + y * scale + dy
                        if 0 <= dest_x < sheet_width and 0 <= dest_y < sheet_height:
                            dest = (dest_y * sheet_width + dest_x) * 3
                            sheet[dest:dest + 3] = pixels[source:source + 3]
        draw_label(sheet, sheet_width, cell_x * cell_width + 4, cell_y * cell_height + 4,
                   f"{slot}:{candidate['frame']}")
        mapping.append({
            "slot": slot,
            "cell": {"column": cell_x, "row": cell_y, "x": cell_x * cell_width,
                     "y": cell_y * cell_height, "width": cell_width, "height": cell_height},
            "archive": candidate["archive"],
            "member": candidate["member"],
            "frame": candidate["frame"],
            "frameWidth": width,
            "frameHeight": height,
            "pixelSha256": hashlib.sha256(pixel_indices).hexdigest(),
        })
    write_png(output, bytes(sheet), sheet_width, sheet_height)
    sidecar = output.with_suffix(".json")
    sidecar.write_text(json.dumps({
        "image": str(output),
        "family": family,
        "columns": columns,
        "cellWidth": cell_width,
        "cellHeight": cell_height,
        "candidates": mapping,
    }, indent=2), encoding="utf-8")
    print(f"contact sheet: {output}", file=sys.stderr)
    print(f"contact sheet map: {sidecar}", file=sys.stderr)
    return sidecar

def decode_stci_metadata(blob: bytes, decode_pixels: bool = False) -> dict:
    if len(blob) < STCI_HEADER_SIZE:
        raise ValueError("STCI header truncated")
    if blob[:4] != b"STCI":
        raise ValueError("not an STCI file")
    flags = struct.unpack_from("<I", blob, 16)[0]
    height, width = struct.unpack_from("<HH", blob, 20)
    palette_colors = struct.unpack_from("<I", blob, 24)[0]
    subimages = struct.unpack_from("<H", blob, 28)[0]
    if flags & 0x0008 == 0 or flags & 0x0020 == 0:
        raise ValueError("STCI is not ETRLE-indexed")
    if blob[30:33] != b"\x08\x08\x08":
        raise ValueError("STCI is not RGB888-indexed")
    if palette_colors != 256:
        raise ValueError("STCI does not have a 256-color palette")
    if subimages == 0:
        raise ValueError("STCI has no subimages")
    app_data_size = struct.unpack_from("<I", blob, 48)[0]
    if app_data_size and app_data_size < subimages * 16:
        raise ValueError("STCI app-data section truncated")
    if app_data_size and app_data_size % (subimages * 16) != 0:
        raise ValueError("STCI app-data size is not per-frame")

    if flags & 0x0004:
        raise ValueError("STCI has RGB flag set")

    palette_start = STCI_HEADER_SIZE
    subimage_start = palette_start + STCI_PALETTE_SIZE
    if subimage_start + subimages * STCI_SUBIMAGE_HEADER_SIZE > len(blob):
        raise ValueError("STCI palette/subimage table truncated")
    frames = []
    data_start = subimage_start + subimages * STCI_SUBIMAGE_HEADER_SIZE
    if data_start > len(blob):
        raise ValueError("STCI data section truncated")
    for index in range(subimages):
        header = subimage_start + index * STCI_SUBIMAGE_HEADER_SIZE
        data_offset, data_length, offset_x, offset_y, frame_height, frame_width = struct.unpack_from(
            "<IIhhHH", blob, header
        )
        if frame_width <= 0 or frame_height <= 0:
            raise ValueError(f"invalid frame dimensions at index {index}")
        start = data_start + data_offset
        end = start + data_length
        if start < data_start or end > len(blob):
            raise ValueError(f"frame {index} data out of bounds")
        frame = {
            "index": index,
            "width": frame_width,
            "height": frame_height,
            "offsetX": offset_x,
            "offsetY": offset_y,
            "dataOffset": data_offset,
            "dataLength": data_length,
            "sha256": hashlib.sha256(blob[start:end]).hexdigest(),
        }
        if decode_pixels:
            pixels, valid = decode_etrle(blob[start:end], frame_width, frame_height)
            frame["etrleValid"] = valid
            frame["pixelSha256"] = hashlib.sha256(pixels).hexdigest()
        frames.append(frame)
    return {
        "flags": flags,
        "headerWidth": width,
        "headerHeight": height,
        "subImageCount": subimages,
        "frames": frames,
    }


def art_member_metadata(archive: bytes, name: str, offset: int, length: int,
                        decode_pixels: bool = False) -> dict:
    blob = archive[offset : offset + length]
    result = {
        "name": name,
        "offset": offset,
        "length": length,
        "sha256": hashlib.sha256(blob).hexdigest(),
    }
    try:
        result["stci"] = decode_stci_metadata(blob, decode_pixels)
    except ValueError as error:
        result["stciError"] = str(error)
    return result


def art_metadata(archives: dict[str, bytes], decode_pixels: bool = False) -> dict:
    result = {}
    for archive_name, archive in archives.items():
        wanted = []
        for name, offset, length in archive_entries(archive):
            normalized = name.lower().replace("\\", "/")
            if is_art_candidate(archive_name, normalized, "all"):
                wanted.append(art_member_metadata(archive, name, offset, length, decode_pixels))
        result[archive_name] = wanted
    return result


def decode_item_name(record: bytes) -> str:
    text = record.decode("utf-16le", errors="ignore")
    text = "".join(chr(ord(char) - 1) if char != "\0" else "\0" for char in text)
    return text.split("\0", 1)[0].strip().replace("\x1f", " ")


def normalized_name(value: str) -> str:
    return "".join(char for char in value.upper() if char.isalnum())


def load_externalized_items(repo_root: Path) -> tuple[dict[int, list[dict]], list[str]]:
    result: dict[int, list[dict]] = {}
    loaded: list[str] = []
    for source_name in EXTERNALIZED:
        path = repo_root / "assets" / "externalized" / f"{source_name}.json"
        if not path.is_file():
            continue
        loaded.append(source_name)
        for item in json.loads(path.read_text(encoding="utf-8")):
            item_index = item.get("itemIndex")
            if item_index is None:
                continue
            result.setdefault(item_index, []).append({
                "source": source_name,
                "internalName": item.get("internalName"),
                "inventoryGraphics": item.get("inventoryGraphics"),
                "tileGraphic": item.get("tileGraphic"),
                "itemClass": item.get("usItemClass"),
                "internalType": item.get("internalType"),
                "calibre": item.get("calibre"),
                "capacity": item.get("capacity", item.get("ubMagSize")),
                "ammoType": item.get("ammoType"),
            })
    return result, loaded


def compare_item(index: int, name: str, candidates: list[dict]) -> dict:
    normalized = normalized_name(name)
    exact = [
        item for item in candidates
        if item.get("internalName") and normalized_name(item["internalName"]) == normalized
    ]
    known = KNOWN_EVIDENCE.get(index, {
        "classification": "unknown",
        "evidence": "ITEMDESC.edt identity only; artwork association unproven",
    })
    return {
        "wildfireName": name,
        "externalized": candidates,
        "internalNameMatch": "exact" if exact else "none" if not candidates else "different",
        "classification": known["classification"],
        "evidence": [known["evidence"]],
        "behaviorEvidence": KNOWN_BEHAVIOR_EVIDENCE.get(index),
    }


def graphic_key(graphic: dict | None) -> tuple[str, int] | None:
    if not isinstance(graphic, dict) or not graphic.get("path"):
        return None
    return graphic["path"].lower().replace("\\", "/"), graphic.get("subImageIndex", 0)


def tile_key(tile: dict | None) -> tuple[str, int] | None:
    if not isinstance(tile, dict):
        return None
    tile_type = tile.get("type", tile.get("tileType"))
    sub_index = tile.get("subIndex", tile.get("subImageIndex"))
    return (str(tile_type), sub_index) if tile_type is not None and sub_index is not None else None


def item_graphics(item: dict) -> tuple[tuple[str, int] | None, tuple[str, int] | None, tuple[str, int] | None]:
    graphics = item.get("inventoryGraphics") or {}
    return graphic_key(graphics.get("small")), graphic_key(graphics.get("big")), tile_key(item.get("tileGraphic"))


def archive_for_art_path(path: str) -> str | None:
    normalized = path.lower().replace("\\", "/")
    for prefix, archive_name in ART_ARCHIVE_BY_PREFIX.items():
        if normalized.startswith(prefix):
            return archive_name
    return None


def numbered_art_key(path: str | None, frame: int | None = None) -> str | None:
    if not path:
        return None
    name = Path(path.lower().replace("\\", "/")).name
    if name == "mdguns.sti" and frame is not None:
        return f"gun{frame:02d}"
    sheet = re.fullmatch(r"md(p[123])items\.sti", name)
    if sheet and frame is not None:
        return f"{sheet.group(1)}item{frame:02d}"
    match = NUMBERED_ART_RE.search(name)
    if match is None:
        return None
    family, number = match.groups()
    return f"{family.lower()}{int(number):02d}"


def numbered_art_owners(items: list[dict]) -> dict[str, list[dict]]:
    owners: dict[str, list[dict]] = {}
    for item in items:
        for source in item["externalized"]:
            small, big, _ = item_graphics(source)
            for surface, graphic in (("small", small), ("big", big)):
                if graphic is None:
                    continue
                key = numbered_art_key(graphic[0], graphic[1] if surface == "small" else None)
                if key is None:
                    continue
                owners.setdefault(key, []).append({
                    "itemIndex": item["itemIndex"],
                    "wildfireName": item["wildfireName"],
                    "source": source["source"],
                    "internalName": source.get("internalName"),
                    "surface": surface,
                    "graphic": list(graphic),
                })
    return owners


def missing_art_references(items: list[dict], archive_members: dict[str, set[str]]) -> list[dict]:
    missing = []
    for item in items:
        for source in item["externalized"]:
            small, big, _ = item_graphics(source)
            for surface, graphic in (("small", small), ("big", big)):
                if graphic is None:
                    continue
                archive_name = archive_for_art_path(graphic[0])
                if archive_name is None:
                    continue
                normalized = graphic[0].lower().replace("\\", "/")
                if normalized not in archive_members.get(archive_name, set()):
                    missing.append({
                        "itemIndex": item["itemIndex"],
                        "wildfireName": item["wildfireName"],
                        "source": source["source"],
                        "internalName": source.get("internalName"),
                        "surface": surface,
                        "archive": archive_name,
                        "graphic": list(graphic),
                    })
    return missing


def art_family(path: str | None) -> str | None:
    if not path:
        return None
    name = Path(path).name.lower()
    if name.startswith("mdguns") or re.fullmatch(r"gun\d+\.sti", name):
        return "guns"
    if name.startswith("mdp1items") or re.fullmatch(r"p1item\d+\.sti", name):
        return "p1items"
    if name.startswith("mdp2items") or re.fullmatch(r"p2item\d+\.sti", name):
        return "p2items"
    return "other"


def collision_audit(items: list[dict], archives: dict[str, bytes] | None = None) -> dict:
    surfaces: dict[str, dict[tuple, list[dict]]] = {"small": {}, "big": {}, "tile": {}}
    family_mismatches = []
    numbered_mismatches = []
    blade_gun_art = []
    for item in items:
        for source in item["externalized"]:
            small, big, tile = item_graphics(source)
            owner = {
                "itemIndex": item["itemIndex"],
                "wildfireName": item["wildfireName"],
                "source": source["source"],
                "internalName": source.get("internalName"),
            }
            for surface, key in (("small", small), ("big", big), ("tile", tile)):
                if key is not None:
                    surfaces[surface].setdefault(key, []).append(owner)
            small_family = art_family(small[0] if small else None)
            big_family = art_family(big[0] if big else None)
            if small_family and big_family and small_family != big_family:
                family_mismatches.append({**owner, "small": small, "big": big})
            small_number = numbered_art_key(small[0], small[1]) if small else None
            big_number = numbered_art_key(big[0]) if big else None
            if small_number and big_number and small_number != big_number:
                numbered_mismatches.append({**owner, "small": small, "big": big, "smallArt": small_number, "bigArt": big_number})
            if source.get("internalType") in ("BLADE", "THROWINGBLADE") and (
                small_family == "guns" or big_family == "guns"
            ):
                blade_gun_art.append({**owner, "small": small, "big": big})

    def collisions(surface: str) -> list[dict]:
        return [
            {"graphic": list(key), "owners": owners}
            for key, owners in surfaces[surface].items()
            if len({owner["itemIndex"] for owner in owners}) > 1
        ]

    numbered_owners = numbered_art_owners(items)
    archive_members = {
        archive_name: {name.lower().replace("\\", "/") for name, _, _ in archive_entries(data)}
        for archive_name, data in (archives or {}).items()
    }
    return {
        "smallGraphicCollisions": collisions("small"),
        "bigGraphicCollisions": collisions("big"),
        "tileGraphicCollisions": collisions("tile"),
        "numberedArtCollisions": [
            {"art": key, "owners": owners}
            for key, owners in numbered_owners.items()
            if len({owner["itemIndex"] for owner in owners}) > 1
        ],
        "missingArchiveArtReferences": missing_art_references(items, archive_members) if archives else [],
        "smallBigFamilyMismatches": family_mismatches,
        "smallBigNumberMismatches": numbered_mismatches,
        "bladeUsingGunFamily": blade_gun_art,
    }


def resource_index(archives: dict[str, bytes]) -> dict[str, list[str]]:
    wanted = ("itemdesc", "mdp1items", "mdp2items", "mdguns", "p1item", "p2item", "gun")
    return {
        archive_name: [
            name for name, _, _ in archive_entries(data)
            if any(token in name.lower().replace("\\", "/") for token in wanted)
        ]
        for archive_name, data in archives.items()
    }


def duplicate_ids(externalized: dict[int, list[dict]]) -> dict[str, list[str]]:
    return {
        str(index): [item["source"] for item in entries]
        for index, entries in externalized.items()
        if len(entries) > 1
    }


def audit_summary(items: list[dict], externalized: dict[int, list[dict]]) -> dict:
    return {
        "wildfireItemCount": ITEM_COUNT,
        "externalizedItemCount": len(externalized),
        "exactInternalNameMatches": sum(item["internalNameMatch"] == "exact" for item in items),
        "differentInternalNames": sum(item["internalNameMatch"] == "different" for item in items),
        "unmatchedWildfireIds": [item["itemIndex"] for item in items if not item["externalized"]],
        "duplicateExternalizedIds": duplicate_ids(externalized),
        "classificationCounts": {
            classification: sum(item["classification"] == classification for item in items)
            for classification in ("confirmed", "fallback", "unknown", "disproved")
        },
    }


def run_self_check() -> None:
    assert normalized_name("5.7mm Magazine, HP") == "57MMMAGAZINEHP"
    assert normalized_name("CLIP57_50_HP") == "CLIP5750HP"
    assert KNOWN_EVIDENCE[54]["classification"] == "confirmed"
    assert KNOWN_EVIDENCE[77]["classification"] == "confirmed"
    assert KNOWN_EVIDENCE[259]["classification"] == "confirmed"
    assert is_art_candidate("InterFace.slf", "interface/mdguns.sti", "guns")
    assert is_art_candidate("BigItems.slf", "bigitems/gun42.sti", "guns")
    assert not is_art_candidate("BigItems.slf", "bigitems/p1item20.sti", "guns")
    assert art_family("interface/mdguns.sti") == "guns"
    assert art_family("bigitems/p1item20.sti") == "p1items"
    assert numbered_art_key("interface/mdguns.sti", 42) == "gun42"
    assert numbered_art_key("bigitems/gun42.sti") == "gun42"
    assert numbered_art_key("interface/mdp1items.sti", 24) == "p1item24"
    assert numbered_art_key("bigitems/p1item24.sti") == "p1item24"
    sample_items = [
        {
            "itemIndex": 54,
            "wildfireName": "Machete",
            "externalized": [{"source": "weapons", "internalName": "MACHETE", "inventoryGraphics": {"small": {"path": "interface/mdguns.sti", "subImageIndex": 42}}}],
        },
        {
            "itemIndex": 259,
            "wildfireName": "Remote Ctrl",
            "externalized": [{"source": "items", "internalName": "ROBOT_REMOTE_CONTROL", "inventoryGraphics": {"big": {"path": "bigitems/gun42.sti"}}}],
        },
    ]
    assert collision_audit(sample_items)["numberedArtCollisions"][0]["art"] == "gun42"
    sample_items[0]["externalized"][0]["inventoryGraphics"]["big"] = {"path": "bigitems/gun41.sti"}
    assert collision_audit(sample_items)["smallBigNumberMismatches"][0]["smallArt"] == "gun42"
    assert missing_art_references(sample_items, {"InterFace.slf": {"interface/mdguns.sti"}, "BigItems.slf": {"bigitems/gun41.sti", "bigitems/gun42.sti"}}) == []
    sample_items[0]["externalized"][0]["inventoryGraphics"]["big"] = {"path": "bigitems/gun42.sti"}
    assert missing_art_references(sample_items, {"InterFace.slf": {"interface/mdguns.sti"}, "BigItems.slf": {"bigitems/gun41.sti"}})[0]["graphic"] == ["bigitems/gun42.sti", 0]
    assert decode_etrle(b"\x01\x01\x00", 1, 1) == (bytearray([1]), True)
    assert not decode_etrle(b"\x01", 1, 1)[1]
    blob = bytearray(64 + 768 + 16 + 3)
    blob[:4] = b"STCI"
    struct.pack_into("<I", blob, 16, 0x28)
    struct.pack_into("<HH", blob, 20, 1, 1)
    struct.pack_into("<I", blob, 24, 256)
    struct.pack_into("<H", blob, 28, 1)
    blob[30:33] = b"\x08\x08\x08"
    struct.pack_into("<IIhhHH", blob, 64 + 768, 0, 3, 0, 0, 1, 1)
    assert decode_stci_metadata(bytes(blob), True)["frames"][0]["etrleValid"]
    try:
        decode_stci_metadata(bytes(blob[:-1]))
    except ValueError:
        pass
    else:
        raise AssertionError("truncated STCI accepted")
    assert decode_stci_metadata(bytes(blob), True)["subImageCount"] == 1
    assert decode_stci_pixels(bytes(blob), 0)[1:3] == (1, 1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("data_dir", type=Path, nargs="?", help="Wildfire Data directory")
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="ja2-stracciatella repository root",
    )
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--art-audit", action="store_true", help="include decoded STI metadata")
    parser.add_argument("--decode-art", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--contact-sheet", type=Path, help="write decoded art PNG and JSON map")
    parser.add_argument(
        "--art-family", choices=("all", "magazines", "guns"), default="all",
        help="limit contact sheet candidates",
    )
    args = parser.parse_args()
    if args.self_check:
        run_self_check()
        return 0
    if args.data_dir is None:
        parser.error("data_dir is required unless --self-check is used")

    data_dir = args.data_dir.expanduser().resolve()
    repo_root = args.repo_root.expanduser().resolve()
    if not data_dir.is_dir():
        raise SystemExit(f"missing data directory: {data_dir}")

    archive_data: dict[str, bytes] = {}
    archive_report: dict[str, dict] = {}
    for archive_name in ARCHIVES:
        archive = data_dir / archive_name
        if not archive.is_file():
            raise SystemExit(f"missing archive: {archive}")
        blob = archive.read_bytes()
        members = validate_archive(archive, blob)
        archive_data[archive_name] = blob
        archive_report[archive_name] = {
            "sha256": hashlib.sha256(blob).hexdigest(),
            "memberCount": len(members),
            "members": [name for name, _, _ in members],
        }

    item_data = read_member(archive_data["BinaryData.slf"], "itemdesc.edt")
    if item_data is None or len(item_data) < ITEM_RECORD_SIZE * ITEM_COUNT:
        raise SystemExit("ITEMDESC.edt missing or truncated")

    externalized, loaded_sources = load_externalized_items(repo_root)
    items = []
    for index in range(ITEM_COUNT):
        start = index * ITEM_RECORD_SIZE
        name = decode_item_name(item_data[start : start + ITEM_RECORD_SIZE])
        items.append({
            "itemIndex": index,
            **compare_item(index, name, externalized.get(index, [])),
        })

    report = {
        "dataDir": str(data_dir),
        "repositoryRoot": str(repo_root),
        "externalizedSources": loaded_sources,
        "archives": archive_report,
        "resourceIndex": resource_index(archive_data),
        "items": items,
        "collisions": collision_audit(items, archive_data),
        **({"artMetadata": art_metadata(archive_data, True)} if args.art_audit else {}),
        "summary": audit_summary(items, externalized),
    }
    if args.contact_sheet:
        contact_sheet = args.contact_sheet.expanduser().resolve()
        sidecar = write_contact_sheet(archive_data, contact_sheet, args.art_family)
        report["contactSheet"] = str(contact_sheet)
        report["contactSheetMap"] = str(sidecar)
    json.dump(report, sys.stdout, ensure_ascii=False, indent=2)
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
