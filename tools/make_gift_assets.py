# Builds the v8.2 store / gift icons in assets/ from the artist's pictures in tools/gift_src/ (transparent PNGs):
#   gift_lotion.png  <- body_lotion.png       gift_deck.png  <- pink_deck.png      gift_yoga.png <- yoga_outfit.png
#   gift_designer.png <- designer_top.png (v10.8, the Designer Top)
#   gift_cow.png <- cow_outfit.png (v11.1, the Cow Print Outfit)
#   gift_gold.png    <- gold_bar.png          gift_dring.png <- diamond_ring.png   gift_super.png <- milk_elixir.png (Super milk drug)
# The empty margin is cropped away and the picture is shrunk with smooth edges to fit in a box of MAX_W x ICON_H.
# Run: python3 tools/make_gift_assets.py
from PIL import Image
import os
SRC = "tools/gift_src"
ICON_H = 72
MAX_W = 90
def crop(im):
    return im.crop(im.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox())
def fit(name, out):
    im = crop(Image.open(os.path.join(SRC, name)).convert("RGBA"))
    sc = min(ICON_H / im.height, MAX_W / im.width)
    w, h = max(1, round(im.width * sc)), max(1, round(im.height * sc))
    # premultiplied resize so the edge does not pick up the (white / black) colour of the transparent pixels
    r, g, b, a = im.split()
    pm = Image.merge("RGBA", (Image.composite(r, Image.new("L", im.size, 0), a), Image.composite(g, Image.new("L", im.size, 0), a), Image.composite(b, Image.new("L", im.size, 0), a), a))
    pm = pm.resize((w, h), Image.LANCZOS)
    r, g, b, a = pm.split()
    px = pm.load(); apx = a.load()
    res = Image.new("RGBA", (w, h)); rp = res.load()
    for y in range(h):
        for x in range(w):
            al = apx[x, y]
            if al < 6: rp[x, y] = (0, 0, 0, 0); continue
            c = px[x, y]; k = 255.0 / al
            rp[x, y] = (min(255, int(c[0] * k)), min(255, int(c[1] * k)), min(255, int(c[2] * k)), al)
    # drop floating specks (tiny separate islands of opaque pixels) left over from the source picture
    seen = set(); removed = 0
    for y in range(h):
        for x in range(w):
            if (x, y) in seen or rp[x, y][3] == 0: continue
            comp = []; st = [(x, y)]; seen.add((x, y))
            while st:
                cx, cy = st.pop(); comp.append((cx, cy))
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        nx, ny = cx + dx, cy + dy
                        if 0 <= nx < w and 0 <= ny < h and (nx, ny) not in seen and rp[nx, ny][3] > 0: seen.add((nx, ny)); st.append((nx, ny))
            if len(comp) < 10:
                for cx, cy in comp: rp[cx, cy] = (0, 0, 0, 0)
                removed += len(comp)
    if removed: print("  removed", removed, "stray pixels")
    res.save(out)
    print(out, w, "x", h)
fit("body_lotion.png", "assets/gift_lotion.png")
fit("pink_deck.png", "assets/gift_deck.png")
fit("yoga_outfit.png", "assets/gift_yoga.png")
fit("gold_bar.png", "assets/gift_gold.png")
fit("diamond_ring.png", "assets/gift_dring.png")
fit("milk_elixir.png", "assets/gift_super.png")
fit("designer_top.png", "assets/gift_designer.png")   # v10.8
fit("cow_outfit.png", "assets/gift_cow.png")   # v11.1 (the Cow Print Outfit)
