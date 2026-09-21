#!/usr/bin/env python3
# Copyright (C) 2026 buu420
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate the private DW2 fallback message table from an owned USA BIN image.

The output contains game dialogue and is intentionally ignored by Git.
Python 3.10+; standard library only. No files are downloaded.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

DIGI_CHARS = {
    **{i: str(i) for i in range(10)},
    **{0x0A + i: chr(ord("A") + i) for i in range(26)},
    **{0x24 + i: chr(ord("a") + i) for i in range(26)},
    0x42: "&",
    0x44: "?",
    0x45: "!",
    0x46: "/",
    0x49: "-",
    0x54: ",",
    0x55: ".",
    0x56: "'",
    0x57: '"',
    0x58: ";",
    0x59: ":",
    0x5A: "%",
    0x5B: "+",
    0x5D: "#",
    0xFD: " ",
}

DIGI_TOKENS = {
    0xF000: "Name",
    0xF006: "Digimon",
    0xF007: "you",
    0xF008: "the",
    0xF009: "Digi-Bettle",
    0xF00A: "Domain",
    0xF00B: "Guard",
    0xF00C: "Tamer",
    0xF00D: "here",
    0xF00E: "have",
    0xF00F: "Knights",
    0xF010: "and",
    0xF011: "thing",
    0xF012: "Security",
    0xF013: "that",
    0xF014: "Bertran",
    0xF015: "Tournament",
    0xF016: "Crimson",
    0xF017: "Vendor",
    0xF018: "something",
    0xF019: "Item",
    0xF01A: "Falcon",
    0xF01B: "for",
    0xF01C: "That's",
    0xF01D: "Commander",
    0xF01E: "Blood",
    0xF01F: "Leader",
    0xF020: "Attendant",
    0xF021: "Cecilia",
    0xF022: "all",
    0xF023: "mission",
    0xF024: "this",
    0xF025: "MasterTyrannomon",
    0xF026: "Archive",
    0xF027: "Black",
    0xF028: "I'll",
    0xF029: "are",
    0xF02A: "Sword",
    0xF02B: "right",
    0xF02C: "digivolve",
    0xF02D: "enter",
    0xF02E: "What",
    0xF02F: "will",
    0xF030: "come",
    0xF031: "You",
    0xF032: "Coliseum",
    0xF033: "about",
    0xF034: "don't",
    0xF035: "anything",
    0xF036: "Vandar",
    0xF037: "Parts",
    0xF038: "where",
    0xF039: "The",
    0xF03A: "know",
    0xF03B: "Leomon",
    0xF03C: "want",
    0xF03D: "Oldman",
    0xF03E: "like",
    0xF03F: "need",
    0xF040: "Chief",
    0xF041: "with",
    0xF042: "Thank",
    0xF043: "strange",
    0xF044: "Island",
    0xF045: "can",
    0xF046: "realy",
    0xF047: "Blue",
    0xF048: "time",
}

def decode_digi_text(data: bytes) -> str:
    out: list[str] = []
    i = 0
    while i < len(data):
        value = data[i]
        if value == 0xFF:
            break
        if value == 0xF0 and i + 1 < len(data):
            token = (value << 8) | data[i + 1]
            out.append(DIGI_TOKENS.get(token, f"[{token:04X}]"))
            i += 2
            continue
        out.append(DIGI_CHARS.get(value, ""))
        i += 1
    return "".join(out).strip()

def find_digi_strings(data: bytes, min_len: int = 4) -> list[dict[str, object]]:
    hits: list[dict[str, object]] = []
    start = None
    valid = set(DIGI_CHARS) | {0xF0, 0xFF}

    for i, value in enumerate(data + b"\xff"):
        if value in valid:
            if start is None:
                start = i
            if value != 0xFF:
                continue

        if start is not None:
            chunk = data[start:i + 1]
            text = decode_digi_text(chunk)
            if len(text) >= min_len and any(c.isalpha() for c in text):
                hits.append({"offset": start, "text": text[:160]})
            start = None

    return hits

def plausible_speech_text(text: str) -> bool:
    if len(text) < 8:
        return False
    letters = sum(1 for char in text if char.isalpha())
    digits = sum(1 for char in text if char.isdigit())
    if letters < 4:
        return False
    if digits and letters / (letters + digits) < 0.45:
        return False
    if " " not in text and not any(char in text for char in ".!?"):
        return False
    return True

MESSAGE_FILE_INDICES = [3110, 3272, 3111, 3112, 3273, 3274, 3113, 3114, 3275, 3276, 3115, 3116, 3277, 3278, 3279, 3280, 3281, 3282, 3601, 3602, 3603, 3604, 3284, 3285, 3286, 3287, 3638, 3374, 3648, 3649, 2722, 2744, 2746, 3375, 3382, 3650, 3640, 3641, 3642, 3643, 3644, 3645, 3646, 3647, 509, 3328, 3578, 3579, 3580, 3581, 3605, 3606, 3607, 3608, 3609, 3610, 3618, 3619, 3620, 3621, 3639, 3622, 2783, 3366, 3383, 3384, 3385, 3386, 3389, 3407, 3408, 3436, 3437, 3438, 3440, 3443, 3651, 3652, 3294, 3295, 3623, 3390, 3582]
EXPECTED_MESSAGE_SHA256 = '9ec2f346f51c20b9120346db894be13200bc788a0b5dd5e6c109830b10b67034'


def read_sectors(handle, lba, count):
    result = bytearray()
    for sector in range(lba, lba + count):
        handle.seek(sector * 2352 + 24)
        data = handle.read(2048)
        if len(data) != 2048:
            raise ValueError("Short read; expected a MODE2/2352 USA BIN image")
        result.extend(data)
    return bytes(result)


def generate(bin_path, output):
    rows = []
    with bin_path.open("rb") as handle:
        executable = read_sectors(handle, 24, 318)
        if hashlib.sha1(executable).hexdigest() != "e55ed5bf354def07f0cbf4e1fb7fb5f99204f220":
            raise ValueError("Unsupported game revision; expected Digimon World 2 USA SLUS_011.93")
        for index in MESSAGE_FILE_INDICES:
            lba = int.from_bytes(executable[0x33F94 + index * 4:0x33F98 + index * 4], "little")
            count = int.from_bytes(executable[0x37900 + index * 2:0x37902 + index * 2], "little")
            data = read_sectors(handle, lba, count)
            for hit in find_digi_strings(data, min_len=8):
                text = str(hit["text"]).strip()
                if plausible_speech_text(text):
                    rows.append((index, hit["offset"], text))
    digest = hashlib.sha256(json.dumps(rows, ensure_ascii=True, separators=(",", ":")).encode()).hexdigest()
    if digest != EXPECTED_MESSAGE_SHA256:
        raise ValueError("Message data differs from the validated profile; no output written")
    # All validated fallback strings are ASCII; JSON escaping is valid C here.
    content = "/* Generated from the builder's game. Do not publish. */\n"
    content += "".join("   { %d, %d, %s },\n" % (index, offset, json.dumps(text))
                       for index, offset, text in rows)
    output.write_text(content, encoding="utf-8", newline="\n")
    print("Generated %d fallback messages in %s" % (len(rows), output.name))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bin", type=Path, required=True, help="Owned USA MODE2/2352 BIN image")
    parser.add_argument("--output", type=Path,
                        default=Path(__file__).resolve().parents[1] / "accessibility_dw2_messages.inc")
    args = parser.parse_args()
    try:
        generate(args.bin, args.output)
    except (OSError, ValueError) as error:
        parser.exit(1, "error: %s\n" % error)


if __name__ == "__main__":
    main()
