#!/usr/bin/env python3
"""v13.18: PLACEHOLDER pictures for Sophia in the PLAYING ROOM (she sits on the pool table), all 752x560 opaque,
made from assets/bg_playroom.png with a label. Replace the files in assets/ with real art (same names, exactly 752x560).
  sc_feed_pr_<outfit>.png      BREASTFEED on the pool table              (replaces sc_feed_lr_<outfit>.png in the playing room)
  sc_bottle_pr_<outfit>.png    Sophia hands you the bottle(s)            (replaces sc_bottle_<outfit>.png in the playing room)
  pump_pr.png                  the peek while she pumps on the pool table (replaces pump_lr.png in the playing room)
  her_hide_no_pr_<outfit>.png  she hides behind the pool table and refuses BREASTFEED  (replaces her_hide_no_<outfit>.png)
  her_hide_yes_pr_<outfit>.png she hides behind the pool table while she says yes     (replaces her_hide_yes_<outfit>.png)
outfits: d8 (her usual outfit), yoga, top (designer top), cow (cow print).
Run from the repo root: python3 tools/make_playroom_scene_placeholders.py"""
import os
from PIL import Image, ImageDraw, ImageFont
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")
def F(n):
    try: return ImageFont.truetype("DejaVuSans-Bold.ttf", n)
    except Exception: return ImageFont.load_default()
OUT = {"d8": "AFTER DAY 8 OUTFIT", "yoga": "YOGA OUTFIT", "top": "DESIGNER TOP", "cow": "COW PRINT OUTFIT"}
base = Image.open(os.path.join(root, "bg_playroom.png")).convert("RGB").resize((752, 560))
def make(name, what, label, tint):
    im = Image.blend(base, Image.new("RGB", (752, 560), tint), 0.25); d = ImageDraw.Draw(im)
    d.rectangle([0, 200, 752, 360], fill=(0, 0, 0))
    d.text((376, 250), "PLACEHOLDER: " + what, font=F(23), fill=(255, 255, 255), anchor="mm")
    d.text((376, 310), label, font=F(34), fill=(250, 215, 90), anchor="mm")
    im.save(os.path.join(root, name)); print(name, im.size)
for o, lb in OUT.items():
    make("sc_feed_pr_%s.png" % o, "BREASTFEED ON POOL TABLE", lb, (255, 225, 170))
    make("sc_bottle_pr_%s.png" % o, "SOPHIA HANDS YOU THE BOTTLE", lb, (200, 225, 255))
    make("her_hide_no_pr_%s.png" % o, "HIDING BEHIND POOL TABLE: NO", lb, (255, 150, 150))
    make("her_hide_yes_pr_%s.png" % o, "HIDING BEHIND POOL TABLE: YES", lb, (255, 190, 215))
make("pump_pr.png", "PEEK: PUMPING ON POOL TABLE", "PLAYING ROOM", (255, 200, 220))
