#!/usr/bin/env python3
"""Poker Mini v13.7 - makes assets/uno_cards.png, the card atlas of the UNO game.

Atlas: 8 columns x 8 rows of 48x74 cards (64 faces, index = row * 8 + col) and, under them, the small card back (28x43) at x 0, y 592.
Picture index (same as the sheet): 0 back, 1 wild, 2..5 wild Y/R/B/G, 6 wild +4, 7..10 wild +4 Y/R/B/G, 11 blank,
12 + 13 * colour + (1..9 -> 0..8, 0 -> 9, +2 -> 10, skip -> 11, reverse -> 12); colour 0 Yellow, 1 Red, 2 Blue, 3 Green. Only 64 pictures are used (the sheet has no more).

Source: tools/uno_src/uno_sheet.png = the 2048x2048 sheet (12 columns x 6 rows, grey background 37,37,37, last row only 4 cards).
Each card is cut out, the grey corners become transparent and the card is area-averaged down to 48x74 (the 2048 sheet is NEVER loaded on the device).
Without that file a PLACEHOLDER atlas is drawn (plain coloured cards): put the real sheet in tools/uno_src/ and run this script again.
Usage: python3 tools/make_uno_cards.py [sheet.png]
"""
import os, sys
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "assets", "uno_cards.png")
CW, CH = 48, 74
COLS, ROWS = 8, 8
SX = [0, 167, 335, 502, 670, 837, 1005, 1175, 1342, 1510, 1677, 1845]
SY = [0, 258, 517, 775, 1034, 1292]
SW, SH = 165, 256
BG = (37, 37, 37)
RGB = [(250, 200, 20), (226, 38, 38), (22, 108, 214), (36, 160, 70)]   # yellow, red, blue, green (placeholder colours)
NAMES = ["Y", "R", "B", "G"]


def cut_card(sheet, idx):
    """one card of the sheet at 48x74 with transparent corners"""
    col, row = idx % 12, idx // 12
    cell = sheet.crop((SX[col], SY[row], SX[col] + SW, SY[row] + SH)).convert("RGB")
    # transparent = grey pixels connected to the cell border
    mask = Image.new("L", cell.size, 255)
    px, mp = cell.load(), mask.load()
    seen = set(); stack = [(x, y) for x in range(SW) for y in (0, SH - 1)] + [(x, y) for y in range(SH) for x in (0, SW - 1)]
    def isbg(p): return abs(p[0] - BG[0]) < 14 and abs(p[1] - BG[1]) < 14 and abs(p[2] - BG[2]) < 14
    while stack:
        x, y = stack.pop()
        if (x, y) in seen or not (0 <= x < SW and 0 <= y < SH): continue
        seen.add((x, y))
        if not isbg(px[x, y]): continue
        mp[x, y] = 0
        stack += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    # fill the grey with the card edge colour so it does not bleed into the average, then reduce RGB and the mask separately
    small = cell.resize((CW, CH), Image.BOX)
    sm = mask.resize((CW, CH), Image.BOX)
    out = small.convert("RGBA"); out.putalpha(sm)
    return out


def font(size):
    for f in ("DejaVuSans-Bold.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"):
        try: return ImageFont.truetype(f, size)
        except Exception: pass
    return ImageFont.load_default()


def placeholder(idx, w=CW, h=CH, ss=4):
    """a plain card, drawn at ss x size and reduced (smooth)"""
    W, H = w * ss, h * ss
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    d.rounded_rectangle((0, 0, W - 1, H - 1), radius=7 * ss, fill=(255, 255, 255, 255))
    inset = 3 * ss
    def face(fill): d.rounded_rectangle((inset, inset, W - 1 - inset, H - 1 - inset), radius=5 * ss, fill=fill)
    def centre(t, size, col=(255, 255, 255), dy=0):
        f = font(size * ss); bb = d.textbbox((0, 0), t, font=f)
        d.text(((W - (bb[2] - bb[0])) / 2 - bb[0], (H - (bb[3] - bb[1])) / 2 - bb[1] + dy * ss), t, font=f, fill=col)
    def corner(t, col):
        f = font(8 * ss); d.text((5 * ss, 4 * ss), t, font=f, fill=col)
    if idx == 0:   # back
        face((20, 20, 24)); d.ellipse((6 * ss, 20 * ss, W - 6 * ss, H - 20 * ss), fill=(214, 40, 40)); centre("UNO", 9, (255, 255, 255))
    elif idx == 11:
        face((20, 20, 24))
    elif idx in (1, 6) or 2 <= idx <= 10:
        face((20, 20, 24)); wild4 = idx >= 6
        cols = RGB if idx in (1, 6) else None
        q = [RGB[0], RGB[1], RGB[3], RGB[2]]
        if idx in (1, 6): pass
        else: c = (idx - 2) if idx <= 5 else (idx - 7); q = [RGB[c]] * 4
        cx, cy, r = W // 2, H // 2, 11 * ss
        d.pieslice((cx - r, cy - r, cx + r, cy + r), 180, 270, fill=q[0]); d.pieslice((cx - r, cy - r, cx + r, cy + r), 270, 360, fill=q[1])
        d.pieslice((cx - r, cy - r, cx + r, cy + r), 90, 180, fill=q[2]); d.pieslice((cx - r, cy - r, cx + r, cy + r), 0, 90, fill=q[3])
        if wild4: corner("+4", (255, 255, 255)); d.text((W - 17 * ss, H - 13 * ss), "+4", font=font(8 * ss), fill=(255, 255, 255))
    else:
        k = idx - 12; c, v = k // 13, k % 13
        face(RGB[c]); t = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "+2", "S", "R"][v]
        d.ellipse((7 * ss, 17 * ss, W - 7 * ss, H - 17 * ss), fill=(255, 255, 255)) if False else None
        centre(t, 24 if len(t) == 1 else 18, (255, 255, 255)); corner(t, (255, 255, 255)); d.text((W - (6 + 5 * len(t)) * ss, H - 13 * ss), t, font=font(8 * ss), fill=(255, 255, 255))
    return im.resize((w, h), Image.LANCZOS)


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "uno_src", "uno_sheet.png")
    atlas = Image.new("RGBA", (CW * COLS, CH * ROWS + CH), (0, 0, 0, 0))
    if os.path.exists(src):
        sheet = Image.open(src).convert("RGB"); print("cutting", src, sheet.size)
        for i in range(64): atlas.paste(cut_card(sheet, i), ((i % COLS) * CW, (i // COLS) * CH))
        back = cut_card(sheet, 0).resize((28, 43), Image.BOX)
    else:
        print("NO SHEET at", src, "- drawing the PLACEHOLDER cards")
        for i in range(64): atlas.paste(placeholder(i), ((i % COLS) * CW, (i // COLS) * CH))
        back = placeholder(0, 28, 43)
    atlas.paste(back, (0, CH * ROWS))
    os.makedirs(os.path.dirname(OUT), exist_ok=True); atlas.save(OUT); print("wrote", OUT, atlas.size)


if __name__ == "__main__":
    main()
