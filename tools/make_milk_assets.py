# Builds the v6.5 milk pictures in assets/ from the artist's pictures in tools/milk_src/ (transparent PNGs):
#   gift_pump.png  (store + gift list icon, 72 px tall)      <- Pixel_Art_Manual_Breast_Pump.png
#   gift_milk.png  (milk inducer pill icon, 72 px tall)       <- pill2.PNG
#   item_bottle.png (inventory icon, 72 px tall)              <- baby_bottle.PNG
#   bottle_gauge.png (the gauge in her romance scene)         <- bottle_gauge.png   (GAUGE_H px tall)
# The empty margin is cropped away and the picture is shrunk with smooth edges. Run: python3 tools/make_milk_assets.py
from PIL import Image
import os
SRC = "tools/milk_src"
ICON_H = 72
GAUGE_H = 120
def crop(im):
    return im.crop(im.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox())
def fit(name, out, h):
    im = crop(Image.open(os.path.join(SRC, name)).convert("RGBA"))
    w = max(1, round(im.width * h / im.height))
    im.resize((w, h), Image.LANCZOS).save(out)
    print(out, w, "x", h)
fit("Pixel_Art_Manual_Breast_Pump.png", "assets/gift_pump.png", ICON_H)
fit("pill2.PNG", "assets/gift_milk.png", ICON_H)
fit("baby_bottle.PNG", "assets/item_bottle.png", ICON_H)
fit("bottle_gauge.png", "assets/bottle_gauge.png", GAUGE_H)
