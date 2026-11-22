#!/usr/bin/env python3
"""v10.8: PLACEHOLDER pictures for the DESIGNER TOP outfit (G.outfit == 2). Each file is the yoga picture with a coloured band, same size and format.
Replace the files in assets/ with the real art (same names). A missing *_top file is not an error: the game falls back to the yoga picture of the same kind.
  her_home_top        her everyday picture (door, room, chat, tease, night knock when moved in)   640x480 transparent around her
  her_cover_top       her covering picture (tease about her body, ASK FOR MILK, BREASTFEED)       640x480 transparent
  her_hide_top        hiding behind her bedroom door (story sprite 5)                              752x560 opaque
  her_hide_no_top     hiding, she refuses BREASTFEED at the door                                    752x560 opaque
  her_hide_yes_top    hiding, she says yes to BREASTFEED at the door (story sprite 6)               752x560 opaque
  her_neutral_top / her_shock_top / her_smug_top / her_caught_top   the poker table faces            640x480 transparent
  sc_hug_top / sc_kiss_top / sc_sleep_top / sc_feed_top / sc_feed_lr_top   the page-4 scene pictures  752x560 opaque
Run from the repo root: python3 tools/make_top_placeholders.py"""
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
    d.rectangle([0, h // 2 - 60, w, h // 2 + 36], fill=(60, 0, 70, 215))
    d.text((w // 2, h // 2 - 30), "PLACEHOLDER: DESIGNER TOP", font=font, fill=(255, 255, 255, 255), anchor="mm")
    d.text((w // 2, h // 2 + 6), what, font=font, fill=(255, 150, 220, 255), anchor="mm")
    if mode == "RGB": im = im.convert("RGB")
    im.save(os.path.join(root, base + "_top.png")); print(base + "_top.png", im.size, im.mode)
