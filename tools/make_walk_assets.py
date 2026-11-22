#!/usr/bin/env python3
"""Copies the artist's map exports and sprite sheets into assets/ (run from the repo root, give the folder that holds them).
   map_*.png   : the RPG Maker exports at 408x312 (24 px tiles). The game enlarges them 2x (48 px tiles) with sharp pixels.
   player.png  : walking sheet, 3 frames x 4 rows (down, left, right, up), about 48x72 per frame. Stray single pixels are removed.
   clerk.png   : the receptionist, one front-facing picture (the game enlarges it 2x)."""
import sys, shutil
from PIL import Image
src = sys.argv[1] if len(sys.argv) > 1 else "."
for a, b in (("room.png", "map_room.png"), ("lobby.png", "map_lobby.png"), ("clerk.png", "clerk.png"))   # the three hall floors come from tools/make_floor_maps.py:
    shutil.copy(f"{src}/{a}", f"assets/{b}")
im = Image.open(f"{src}/Indoors---Male.png").convert("RGBA"); px = im.load(); w, h = im.size; n = 0
for y in range(h):
    for x in range(w):
        if px[x, y][3] == 0: continue
        nb = sum(1 for dy in (-1, 0, 1) for dx in (-1, 0, 1) if (dx or dy) and 0 <= x+dx < w and 0 <= y+dy < h and px[x+dx, y+dy][3] > 0)
        if nb == 0: px[x, y] = (0, 0, 0, 0); n += 1
# also drop small floating specks (under 40 pixels, not touching the figure)
seen = set()
for y in range(h):
    for x in range(w):
        if px[x, y][3] > 0 and (x, y) not in seen:
            st = [(x, y)]; seen.add((x, y)); c = []
            while st:
                a, b = st.pop(); c.append((a, b))
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        q = (a+dx, b+dy)
                        if 0 <= q[0] < w and 0 <= q[1] < h and q not in seen and px[q][3] > 0: seen.add(q); st.append(q)
            if len(c) < 40:
                for a, b in c: px[a, b] = (0, 0, 0, 0); n += 1
im.save("assets/player.png"); print("removed", n, "stray pixels")
