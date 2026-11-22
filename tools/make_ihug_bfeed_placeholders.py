#!/usr/bin/env python3
"""v13.11: PLACEHOLDER pictures for the two new mama-mode scenes, one per outfit, all 752x560 opaque.
  sc_ihug_<outfit>.png   INTIMATE HUG: Sophia and you lie on her bed, you hug her
  sc_bfeed_<outfit>.png  BOTTLE FEEDING: Sophia feeds you the bottle
  outfits: d8 (her usual outfit), yoga, top (designer top), cow (cow print)
Each is the matching hug / breastfeed placeholder with a new label. Replace the files in assets/ with the real art (same names, exactly 752x560).
The hand-over of BOTTLE FEEDING uses the existing sc_bottle_<outfit>.png (Sophia handing the bottle).
Run from the repo root: python3 tools/make_ihug_bfeed_placeholders.py"""
import os
from PIL import Image, ImageDraw, ImageFont
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")
try: font = ImageFont.truetype("DejaVuSans-Bold.ttf", 34)
except Exception: font = ImageFont.load_default()
SRC = {"ihug": ("hug", (255, 175, 190), "INTIMATE HUG, LYING ON HER BED"), "bfeed": ("feed", (255, 225, 170), "BOTTLE FEEDING")}
OUT = {"d8": "AFTER DAY 8 OUTFIT", "yoga": "YOGA OUTFIT", "top": "DESIGNER TOP", "cow": "COW PRINT OUTFIT"}
for scene, (src, tint, what) in SRC.items():
    for o, label in OUT.items():
        p = os.path.join(root, "sc_%s_%s.png" % (src, o))
        if not os.path.exists(p): print("missing", p); continue
        im = Image.blend(Image.open(p).convert("RGB").resize((752, 560)), Image.new("RGB", (752, 560), tint), 0.25)
        d = ImageDraw.Draw(im)
        d.rectangle([0, 200, 752, 360], fill=(0, 0, 0))
        d.text((376, 250), "PLACEHOLDER: " + what, font=font if len(what) < 22 else ImageFont.truetype("DejaVuSans-Bold.ttf", 28), fill=(255, 255, 255), anchor="mm")
        d.text((376, 310), label, font=font, fill=(250, 215, 90), anchor="mm")
        out = os.path.join(root, "sc_%s_%s.png" % (scene, o)); im.save(out); print(os.path.basename(out), im.size)
