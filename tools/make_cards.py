#!/usr/bin/env python3
"""v12.9: builds assets/cards.png from your four-colour sheet tools/deck_src/deck_gfx.png.
Sheet: 13 columns (2..10 J Q K A) x 4 rows (diamonds, clubs, hearts, spades), 32 x 32 cells, the 24 x 32 card sits at x 4..27.
The deck is made two-colour: hearts and diamonds RED, spades and clubs BLACK.
Plain cards and aces: every pixel of the suit colour is replaced.
Court cards (J Q K): only the corner indices (x < 5 or x >= 19) are replaced, the painted portraits keep their own colours."""
from PIL import Image
RED = (204, 34, 44, 255)
BLACK = (26, 26, 32, 255)
ROWS = [((240, 107, 63), RED), ((35, 89, 85), BLACK), ((240, 52, 100), RED), ((60, 67, 104), BLACK)]
src = Image.open('tools/deck_src/deck_gfx.png').convert('RGBA')
out = src.copy()
px = out.load()
for r, (old, new) in enumerate(ROWS):
    for c in range(13):
        court = c in (9, 10, 11)
        for y in range(32):
            for x in range(32):
                gx, gy = c * 32 + x, r * 32 + y
                p = px[gx, gy]
                if p[3] == 0 or p[:3] != old: continue
                cx = x - 4                       # x inside the 24 px card
                if court and 5 <= cx < 19: continue
                px[gx, gy] = new
out.save('assets/cards.png')
print('assets/cards.png', out.size)
