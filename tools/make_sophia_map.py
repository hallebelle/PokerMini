#!/usr/bin/env python3
"""Sophia's room: crops the artist's export (Sophia_room_walk.png, 480x432 with a transparent border) to the room itself,
   tiles x1..15, y1..16 of 24 px = 360x384 -> assets/map_sophia.png. The game enlarges it 3x (15 x 16 tiles of 72 px).
   Usage (repo root): python3 tools/make_sophia_map.py path/to/Sophia_room_walk.png"""
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert("RGBA")
im.crop((24, 24, 384, 408)).save("assets/map_sophia.png")
