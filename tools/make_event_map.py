#!/usr/bin/env python3
"""Crops the artist's Event Center export (tools/event_src.png = Map026.png, 408x312, 24 px tiles) to the tiles that belong to the room
   (tiles 1..15 across, 1..9 down = 15 x 9 tiles) and writes assets/map_event.png. The tile grid MG_EVENT in src/main.c is written for exactly this crop.
   Run from the repo root."""
from PIL import Image
T = 24
x0, y0, x1, y1 = 1, 1, 16, 10
im = Image.open("tools/event_src.png").convert("RGBA").crop((x0 * T, y0 * T, x1 * T, y1 * T))
bg = Image.new("RGBA", im.size, (0, 0, 0, 255)); bg.alpha_composite(im); im = bg   # the empty corner becomes black (no see-through pixels)
im.save("assets/map_event.png"); print("assets/map_event.png", im.size, "=", x1 - x0, "x", y1 - y0, "tiles")
