#!/usr/bin/env python3
"""v9.3: the three room maps. The artist's exports (tools/room_*_src.png) have a 1-tile transparent margin and a grey editor shadow;
this crops each to its exact tiles (24 px tiles, the game enlarges them 3x): base 7x7, upgrade 1 and upgrade 2 11x7."""
from PIL import Image
T = 24
for src, out, tw, th in (('room_base_src.png', '../assets/map_room.png', 7, 7), ('room_up1_src.png', '../assets/map_room_u1.png', 11, 7), ('room_up2_src.png', '../assets/map_room_up.png', 11, 7)):
    im = Image.open(src).convert('RGBA').crop((T, T, T + tw * T, T + th * T)); im.save(out); print(out, im.size)
