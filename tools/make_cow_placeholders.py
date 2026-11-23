#!/usr/bin/env python3
"""v11.1: PLACEHOLDER pictures for the COW PRINT OUTFIT (G.outfit == 3). Each file is the yoga picture with a coloured band, same size and format.
Replace the files in assets/ with the real art (same names). A missing *_cow file is not an error: the game falls back to the designer-top picture, then the yoga one.
  her_home_cow        her everyday picture (door, room, chat, tease, night knock when moved in)   640x480 transparent around her
  her_cover_cow       her covering picture (tease about her body, ASK FOR MILK, BREASTFEED)       640x480 transparent
  her_hide_cow        hiding behind her bedroom door (story sprite 5)                              752x560 opaque
  her_hide_no_cow     hiding, she refuses BREASTFEED at the door                                    752x560 opaque
  her_hide_yes_cow    hiding, she says yes to BREASTFEED at the door (story sprite 6)               752x560 opaque
  her_neutral_cow / her_shock_cow / her_smug_cow / her_caught_cow   the poker table faces            640x480 transparent
  sc_hug_cow / sc_kiss_cow / sc_sleep_cow / sc_feed_cow / sc_feed_lr_cow   the page-4 scene pictures  752x560 opaque
Run from the repo root: python3 tools/make_cow_placeholders.py"""
import os
from PIL import Image, ImageDraw, ImageFont
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")
try: font = ImageFont.truetype("DejaVuSans-Bold.ttf", 28)
except Exception: font = ImageFont.load_default()
WHAT = {"her_home": "EVERYDAY", "her_cover": "COVERING", "her_hide": "HIDING", "her_hide_no": "HIDING, REFUSES", "her_hide_yes": "HIDING, YES",
        "her_neutral": "TABLE NEUTRAL", "her_shock": "TABLE SHOCK", "her_smug": "TABLE SMUG", "her_caught": "TABLE CAUGHT",
        "sc_hug": "HUG", "sc_kiss": "KISS", "sc_sleep": "SLEEP", "sc_feed": "BREASTFEED", "sc_feed_lr": "BREASTFEED, LIVING ROOM"}
for base, what in WHAT.items():
    src = os.path.join(root, base + "_yoga.png")
    if not os.path.exists(src): print("missing", src); continue
    im = Image.open(src); mode = im.mode; im = im.convert("RGBA"); w, h = im.size
    d = ImageDraw.Draw(im)
    d.rectangle([0, h // 2 - 60, w, h // 2 + 36], fill=(20, 20, 20, 225))
    d.text((w // 2, h // 2 - 30), "PLACEHOLDER: COW PRINT", font=font, fill=(255, 255, 255, 255), anchor="mm")
    d.text((w // 2, h // 2 + 6), what, font=font, fill=(255, 255, 255, 255), anchor="mm")
    if mode == "RGB": im = im.convert("RGB")
    im.save(os.path.join(root, base + "_cow.png")); print(base + "_cow.png", im.size, im.mode)
