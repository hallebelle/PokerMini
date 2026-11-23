#!/usr/bin/env python3
"""v13.21: PLACEHOLDER pictures for BREASTFEED part 2 (mama mode, her bedroom): you and Sophia lie on her bed while she feeds you.
  sc_bedfeed_<outfit>.png   outfits: d8 (her usual outfit), yoga, top (designer top), cow (cow print), all 752x560 opaque
Each one is the matching sleep-together placeholder (the two of you lying on the bed) with a tint and a new label.
Replace the files in assets/ with the real art (same names, exactly 752x560).
Run from the repo root: python3 tools/make_bedfeed_placeholders.py"""
import os
from PIL import Image, ImageDraw, ImageFont
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")
try:
    font = ImageFont.truetype("DejaVuSans-Bold.ttf", 34); small = ImageFont.truetype("DejaVuSans-Bold.ttf", 28)
except Exception:
    font = small = ImageFont.load_default()
OUT = {"d8": "AFTER DAY 8 OUTFIT", "yoga": "YOGA OUTFIT", "top": "DESIGNER TOP", "cow": "COW PRINT OUTFIT"}
WHAT = "BREASTFEED ON THE BED"
for o, label in OUT.items():
    p = os.path.join(root, "sc_sleep_%s.png" % o)
    if not os.path.exists(p):
        print("missing", p); continue
    im = Image.blend(Image.open(p).convert("RGB").resize((752, 560)), Image.new("RGB", (752, 560), (255, 150, 170)), 0.25)
    d = ImageDraw.Draw(im)
    d.rectangle([0, 200, 752, 360], fill=(0, 0, 0))
    d.text((376, 250), "PLACEHOLDER: " + WHAT, font=small, fill=(255, 255, 255), anchor="mm")
    d.text((376, 310), label, font=font, fill=(250, 215, 90), anchor="mm")
    out = os.path.join(root, "sc_bedfeed_%s.png" % o); im.save(out); print(os.path.basename(out), im.size)
