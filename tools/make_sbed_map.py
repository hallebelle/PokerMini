#!/usr/bin/env python3
"""v9.2: Sophia's walkable bedroom. tools/sbed_src.png (the artist's 240x240 export) -> assets/map_sbed.png.
The room is the part x24..216, y24..192 = 8 x 7 tiles of 24 px (the game enlarges maps 3x -> 72 px tiles).
Grid MG_SBED in src/main.c. Run from the project folder: python3 tools/make_sbed_map.py"""
from PIL import Image
src = Image.open("tools/sbed_src.png").convert("RGBA")
room = src.crop((24, 24, 216, 192))
flat = Image.new("RGBA", room.size, (0, 0, 0, 255)); flat.alpha_composite(room)
flat.save("assets/map_sbed.png")
print("assets/map_sbed.png", flat.size)
