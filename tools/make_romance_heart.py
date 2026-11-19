"""v8.4: cuts the heart out of assets/romance_bar.png (the old horizontal bar art) for the new VERTICAL romance bar.
The track lines that touch the heart on the right are removed by mirroring the clean left half on the rows below the heart's widest point.
Usage (from the repo root): python3 tools/make_romance_heart.py   -> writes assets/romance_heart.png"""
from PIL import Image
im = Image.open('assets/romance_bar.png').convert('RGBA')
h = im.crop((0, 0, 52, 48))
a = h.split()[3]
# horizontal extent of the heart on the clean rows (above the track lines)
xs = [x for y in range(0, 14) for x in range(52) if a.getpixel((x, y)) > 128]
x0, x1 = min(xs), max(xs)
# widest rows are around y 14..30; use the left/right extent there for the mirror axis
rows = [y for y in range(14, 30)]
l = min(x for y in rows for x in range(52) if a.getpixel((x, y)) > 128)
r = max(x for y in rows for x in range(52) if a.getpixel((x, y)) > 128 and x < 52)
axis2 = l + r   # mirror: x -> axis2 - x
out = h.copy(); px = out.load(); src = h.load()
for y in range(30, 48):
    for x in range(52):
        px[x, y] = (0, 0, 0, 0)
    for x in range(52):
        mx = axis2 - x
        if 0 <= mx < 52 and mx <= x - 0 and src[mx, y][3] > 0 and mx < axis2 / 2.0 + 0.01:
            px[mx, y] = src[mx, y]
            if 0 <= x < 52: px[x, y] = src[mx, y]
out = out.crop(out.getbbox())
out.save('assets/romance_heart.png')
print('HEART', out.size, 'bbox-axis', l, r)
