#!/usr/bin/env python3
"""v8.8: placeholder pictures for the home table set and the yoga covering picture. Replace the files in assets/ with the real art (same names, 640x480 or 752x560, transparent around her).
  Table faces once she lives with you (G.rs == RS_LIVING): her_<neutral|shock|smug>_<home|yoga>.png and the close-up answer her_caught_<home|yoga>.png
    (home = her day-8 / married outfit, yoga = the yoga outfit). They are NOT the covering pictures of her room.
  Her room, married, when she covers her chest (tease about her body, ASK FOR MILK, BREASTFEED): her_cover.png (day-8 outfit, existing) and her_cover_yoga.png (new)."""
import os
from PIL import Image, ImageDraw, ImageFont
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")
def load(n): return Image.open(os.path.join(root, n)).convert("RGBA")
try: font = ImageFont.truetype("DejaVuSans-Bold.ttf", 28)
except Exception: font = ImageFont.load_default()
def label(im, t1, t2):
    d = ImageDraw.Draw(im); w, h = im.size
    d.rectangle([0, h // 2 - 60, w, h // 2 + 36], fill=(0, 0, 0, 200))
    d.text((w // 2, h // 2 - 30), t1, font=font, fill=(255, 255, 255, 255), anchor="mm")
    d.text((w // 2, h // 2 + 6), t2, font=font, fill=(250, 215, 90, 255), anchor="mm")
    return im
def save(im, n): im.save(os.path.join(root, n))
SRC = {"neutral": "her_neutral_p.png", "shock": "her_shock_p.png", "smug": "her_smug_p.png", "caught": "her_caught.png"}
for face, src in SRC.items():
    for outfit, lab in (("home", "MOVED IN"), ("yoga", "YOGA OUTFIT")):
        n = "her_%s_%s.png" % (face, outfit)
        save(label(load(src), "PLACEHOLDER: TABLE %s, %s" % (face.upper(), lab), n), n)
save(label(load("her_cover.png"), "PLACEHOLDER: YOGA, COVERING", "her_cover_yoga.png"), "her_cover_yoga.png")
