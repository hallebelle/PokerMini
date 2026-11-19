#!/usr/bin/env python3
"""Crops the artist's three floor pictures (tools/floor1_src.png = 1F stair hall, floor2_src.png = 2F hall, floor3_src.png = 3F hall)
   to the tiles that belong to the map (24 px tiles; the soft shadow to the right of the frame is left out) and writes
   assets/map_hall1.png, assets/map_hall.png (2F) and assets/map_hall3.png. Run from the repo root.
   Crop boxes in tiles (x0, y0, x1, y1): the tile grids MG_HALL1 / MG_HALL / MG_HALL3 in src/main.c are written for exactly these crops."""
from PIL import Image
T = 24
JOBS = (("tools/floor1_src.png", "assets/map_hall1.png", (2, 1, 15, 12)),   # 13 x 11 tiles
        ("tools/floor2_src.png", "assets/map_hall.png",  (1, 1, 16, 8)),    # 15 x 7 tiles
        ("tools/floor3_src.png", "assets/map_hall3.png", (1, 1, 15, 9)))    # 14 x 8 tiles
for src, dst, (x0, y0, x1, y1) in JOBS:
    im = Image.open(src).convert("RGBA").crop((x0 * T, y0 * T, x1 * T, y1 * T))
    im.save(dst); print(dst, im.size, "=", (x1 - x0), "x", (y1 - y0), "tiles")
