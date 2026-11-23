#!/usr/bin/env python3
"""v8.6: placeholder pictures for the page-4 scenes (hug / kiss / sleep / breastfeed, one per outfit).
Each is her_cover.png (over her room picture) scaled to 752x560 with a label. Replace the files in assets/ with the real art (same names, exactly 752x560)."""
import os
from PIL import Image, ImageDraw, ImageFont
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
room = Image.open(os.path.join(root, "assets", "rom_room.png")).convert("RGBA").resize((752, 560), Image.LANCZOS)
her = Image.open(os.path.join(root, "assets", "her_cover.png")).convert("RGBA").resize((752, 560), Image.LANCZOS)
base = Image.alpha_composite(room, her).convert("RGB")
tint = {"hug": (255, 190, 200), "kiss": (255, 150, 170), "sleep": (120, 140, 230), "feed": (255, 220, 170)}
try: font = ImageFont.truetype("DejaVuSans-Bold.ttf", 34)
except Exception: font = ImageFont.load_default()
for scene in ("hug", "kiss", "sleep", "feed"):
    for outfit, label in (("d8", "AFTER DAY 8 OUTFIT"), ("yoga", "YOGA OUTFIT")):
        im = Image.blend(base, Image.new("RGB", base.size, tint[scene]), 0.25)
        d = ImageDraw.Draw(im)
        d.rectangle([0, 200, 752, 360], fill=(0, 0, 0))
        d.text((376, 250), "PLACEHOLDER: " + scene.upper(), font=font, fill=(255, 255, 255), anchor="mm")
        d.text((376, 310), label, font=font, fill=(250, 215, 90), anchor="mm")
        im.save(os.path.join(root, "assets", "sc_%s_%s.png" % (scene, outfit)))
