#!/usr/bin/env python3
"""v13.4: PLACEHOLDER background of the playing room (assets/bg_playroom.png, 752 x 560).
Made from the walking map assets/map_play.png (cover-fit, nearest neighbour, centre crop).
Replace assets/bg_playroom.png with real art (exactly 752 x 560, same file name); run this script only to get the placeholder back."""
from PIL import Image
im = Image.open("assets/map_play.png").convert("RGB")
W, H = 752, 560
k = max(W / im.width, H / im.height)
big = im.resize((round(im.width * k), round(im.height * k)), Image.NEAREST)
x = (big.width - W) // 2; y = (big.height - H) // 2
big.crop((x, y, x + W, y + H)).save("assets/bg_playroom.png")
print("assets/bg_playroom.png", W, H)
