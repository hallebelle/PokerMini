# Draws the Onion app icons (white chip vector). Run from the repo root: python3 tools/make_icon.py
import math
from PIL import Image, ImageDraw

S = 8          # supersampling
N = 96         # final size

def chip(color):
    W = N * S
    c = W / 2
    R = W * 0.47
    shape = Image.new("L", (W, W), 0)           # 255 = solid
    d = ImageDraw.Draw(shape)
    d.ellipse([c - R, c - R, c + R, c + R], fill=255)
    # six radial gaps through the outer band
    for i in range(6):
        a = i * math.pi / 3 + math.pi / 6
        x1, y1 = c + math.cos(a) * R * 0.74, c + math.sin(a) * R * 0.74
        x2, y2 = c + math.cos(a) * R * 1.1, c + math.sin(a) * R * 1.1
        d.line([x1, y1, x2, y2], fill=0, width=int(W * 0.05))
    # groove ring between rim and centre
    r1, r2 = R * 0.72, R * 0.64
    d.ellipse([c - r1, c - r1, c + r1, c + r1], fill=0)
    d.ellipse([c - r2, c - r2, c + r2, c + r2], fill=255)
    # spade cut out of the centre
    k = W * 0.115
    d.ellipse([c - k * 1.9, c - k * 0.9, c - k * 0.05, c + k * 0.9], fill=0)
    d.ellipse([c + k * 0.05, c - k * 0.9, c + k * 1.9, c + k * 0.9], fill=0)
    d.polygon([(c, c - k * 2.1), (c - k * 1.85, c + k * 0.1), (c + k * 1.85, c + k * 0.1)], fill=0)
    d.polygon([(c, c + k * 0.0), (c - k * 0.75, c + k * 2.0), (c + k * 0.75, c + k * 2.0)], fill=0)
    im = Image.new("RGBA", (W, W), color)
    im.putalpha(shape)
    return im.resize((N, N), Image.LANCZOS)

chip((255, 255, 255, 255)).save("packaging/icon.png")
chip((30, 30, 30, 255)).save("packaging/icon_sel.png")
print("wrote packaging/icon.png and packaging/icon_sel.png")
