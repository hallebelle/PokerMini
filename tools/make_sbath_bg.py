#!/usr/bin/env python3
"""v13.19: PLACEHOLDER picture of Sophia behind her bathroom door (assets/bg_sbath.png, 752 x 560).
Made from assets/her_hide_d8.png (she hides behind a door) with a small label. Replace assets/bg_sbath.png with real art
(exactly 752 x 560, same file name); run this script only to get the placeholder back. Run from the project folder."""
from PIL import Image, ImageDraw
im = Image.open("assets/her_hide_d8.png").convert("RGB")
if im.size != (752, 560): im = im.resize((752, 560), Image.NEAREST)
d = ImageDraw.Draw(im)
d.rectangle((8, 8, 330, 34), fill=(0, 0, 0)); d.text((14, 15), "PLACEHOLDER: SOPHIA BEHIND BATHROOM DOOR", fill=(255, 255, 255))
im.save("assets/bg_sbath.png"); print("assets/bg_sbath.png", im.size)
