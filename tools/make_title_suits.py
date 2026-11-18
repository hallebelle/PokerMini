"""Builds assets/title_suits.png (the floating suit outlines on the title screen) from tools/suits_sheet.png.
Atlas: 4 suits (spade, heart, diamond, club) across, 3 sizes down. Size k is a square cell of SIZES[k] px at row offset OFFS[k]."""
from PIL import Image
SIZES = [44, 68, 100]
sheet = Image.open("tools/suits_sheet.png").convert("RGBA")
W, H = sheet.size
cells = [(0, 0), (W // 2, 0), (0, H // 2), (W // 2, H // 2)]   # spade, heart, diamond, club
cut = []
for cx, cy in cells:
    part = sheet.crop((cx, cy, cx + W // 2, cy + H // 2))
    cut.append(part.crop(part.getchannel("A").getbbox()))
offs, y = [], 0
for s in SIZES:
    offs.append(y); y += s
atlas = Image.new("RGBA", (4 * SIZES[-1], y), (0, 0, 0, 0))
for k, s in enumerate(SIZES):
    for i, im in enumerate(cut):
        f = (s - 4) / max(im.size)
        sm = im.resize((max(1, round(im.size[0] * f)), max(1, round(im.size[1] * f))), Image.LANCZOS)
        atlas.alpha_composite(sm, (i * s + (s - sm.size[0]) // 2, offs[k] + (s - sm.size[1]) // 2))
atlas.save("assets/title_suits.png")
print("atlas", atlas.size, "sizes", SIZES, "offsets", offs)
