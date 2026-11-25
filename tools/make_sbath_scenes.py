#!/usr/bin/env python3
"""v13.22: PLACEHOLDER pictures for the bathroom scenes (752 x 560 each), made from assets/bg_sbath.png with a tint and a label.
assets/bg_sbpeek.png = Sophia bathing, seen through the bathroom door (PEEP). assets/bg_stub.png = you and Sophia in the bathtub.
Replace them with real art (exactly 752 x 560, same file names); run this script only to get the placeholders back. Run from the project folder."""
from PIL import Image, ImageDraw
base = Image.open("assets/bg_sbath.png").convert("RGB")
if base.size != (752, 560): base = base.resize((752, 560), Image.NEAREST)
for name, tint, label in (("bg_sbpeek.png", (40, 90, 160), "PLACEHOLDER: SOPHIA BATHING (PEEP)"), ("bg_stub.png", (150, 70, 120), "PLACEHOLDER: SOPHIA AND YOU IN THE BATHTUB")):
    im = Image.blend(base, Image.new("RGB", base.size, tint), 0.45)
    d = ImageDraw.Draw(im)
    d.rectangle((8, 8, 330, 34), fill=(0, 0, 0)); d.text((14, 15), label, fill=(255, 255, 255))
    im.save("assets/" + name); print("assets/" + name, im.size)
