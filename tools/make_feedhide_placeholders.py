#!/usr/bin/env python3
"""v10.1: placeholder pictures for BREASTFEED at her bedroom door (mama mode, married).
her_hide_no_<d8|yoga>.png  = she hides behind the door and REFUSES (shown while her refusal line is on screen)
her_hide_yes_<d8|yoga>.png = she hides behind the door and says YES (the first two beats of the BREASTFEED scene)
Both are full 752x560 pictures drawn over the door picture, like her_hide_<d8|yoga>.png. Replace the files in assets/ with the real art (same names).
A missing yoga file falls back to the d8 one; a missing file at all falls back to her_hide_<d8|yoga>.png."""
import os
from PIL import Image, ImageDraw, ImageFont
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")
try: font = ImageFont.truetype("DejaVuSans-Bold.ttf", 30)
except Exception: font = ImageFont.load_default()
for outfit in ("d8", "yoga"):
    base = Image.open(os.path.join(root, "her_hide_%s.png" % outfit)).convert("RGBA")
    for kind, tint, t1 in (("no", (170, 20, 20, 70), "PLACEHOLDER: HIDING, REFUSES"), ("yes", (30, 150, 60, 70), "PLACEHOLDER: HIDING, SAYS YES")):
        im = Image.alpha_composite(base, Image.new("RGBA", base.size, tint))
        d = ImageDraw.Draw(im); w, h = im.size
        d.rectangle([0, h // 2 - 70, w, h // 2 + 40], fill=(0, 0, 0, 200))
        d.text((w // 2, h // 2 - 35), t1, font=font, fill=(255, 255, 255, 255), anchor="mm")
        d.text((w // 2, h // 2 + 5), "her_hide_%s_%s.png" % (kind, outfit), font=font, fill=(250, 215, 90, 255), anchor="mm")
        im.convert("RGB").save(os.path.join(root, "her_hide_%s_%s.png" % (kind, outfit)))
