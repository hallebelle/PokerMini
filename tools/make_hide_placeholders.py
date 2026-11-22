#!/usr/bin/env python3
"""v8.7: placeholder pictures. her_home.png (moved-in look, copy of confident.png), her_home_yoga.png (yoga everyday picture, copy of her_cover.png),
her_hide_d8.png / her_hide_yoga.png (she hides behind her bedroom door, drawn over the door picture). Replace the files in assets/ with the real art (same names).
Her pictures are drawn over the background, so keep the transparent areas transparent; the hide pictures may also be full 752x560 pictures."""
import os
from PIL import Image, ImageDraw, ImageFont
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")
def load(n): return Image.open(os.path.join(root, n)).convert("RGBA")
try: font = ImageFont.truetype("DejaVuSans-Bold.ttf", 30)
except Exception: font = ImageFont.load_default()
def label(im, t1, t2):
    d = ImageDraw.Draw(im); w, h = im.size
    d.rectangle([0, h // 2 - 70, w, h // 2 + 40], fill=(0, 0, 0, 200))
    d.text((w // 2, h // 2 - 35), t1, font=font, fill=(255, 255, 255, 255), anchor="mm")
    d.text((w // 2, h // 2 + 5), t2, font=font, fill=(250, 215, 90, 255), anchor="mm")
    return im
def save(im, n): im.save(os.path.join(root, n))
save(label(load("confident.png"), "PLACEHOLDER: MOVED IN", "her_home.png"), "her_home.png")
save(label(load("her_cover.png"), "PLACEHOLDER: YOGA OUTFIT", "her_home_yoga.png"), "her_home_yoga.png")
door = load("bedroom_door.png").resize((752, 560), Image.LANCZOS)
for outfit, src, lab in (("d8", "her_cover.png", "AFTER DAY 8 OUTFIT"), ("yoga", "her_home_yoga.png", "YOGA OUTFIT")):
    her = load(src).resize((752, 560), Image.LANCZOS)
    base = Image.alpha_composite(door, Image.new("RGBA", (752, 560), (0, 0, 0, 120)))
    base = Image.alpha_composite(base, her.crop((0, 0, 752, 560)))
    save(label(base, "PLACEHOLDER: HIDING BEHIND THE DOOR", lab), "her_hide_%s.png" % outfit)
