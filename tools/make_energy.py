"""Builds the energy bar, drink icons and store picture used by v3.6 from the artwork sheets.
Usage: python3 tools/make_energy.py <energy_bar_progress.png> <STORE_MENU.png> <soda.png> <energy_drink.png>
Writes assets/energy_bar.png (empty frame), assets/energy_seg.png (one segment, 5 fit the bar), assets/store_menu.png,
assets/drink_soda.png, assets/drink_monster.png and prints the geometry constants for main.c."""
import sys, numpy as np
from PIL import Image
sheet, store, soda, mon = sys.argv[1:5]
im = Image.open(sheet).convert('RGBA'); a = np.array(im)
frame = im.crop((600, 562, 1047, 744)).copy()              # empty bar with the bolt
full = a[0:185, 0:446]
# one segment tile from the full bar: x 110..136, rows where it is solid
tile_x0, tile_x1 = 110, 136
col = full[:, 120]; ys = [y for y in np.where(col[:, 3] > 200)[0] if 60 < y < 150]
rows = [y for y in ys if full[y, 120, 2] > 150 and full[y, 120, 0] < 140]
ty0, ty1 = min(rows), max(rows) + 1
tile = Image.fromarray(np.repeat(full[ty0:ty1, 123:124], tile_x1 - tile_x0 + 1, axis=1))   # centre column repeated: a clean segment
x_first, x_end = 109, 432                                    # where the five segments live inside the frame (sheet pixels)
n, gap = 5, 6
w = (x_end - x_first - gap * (n - 1)) // n
tile = tile.resize((w, ty1 - ty0), Image.NEAREST)
SC = 112 / 447.0
fw, fh = round(447 * SC), round(182 * SC)
frame.resize((fw, fh), Image.LANCZOS).save('assets/energy_bar.png')
sw, sh = max(1, round(w * SC)), max(1, round((ty1 - ty0) * SC))
tile.resize((sw, sh), Image.LANCZOS).save('assets/energy_seg.png')
pitch = (w + gap) * SC
print('BAR', fw, fh, 'SEG', sw, sh, 'x0', round(x_first * SC), 'y0', round(ty0 * SC), 'pitch', round(pitch, 2))
Image.open(store).convert('RGBA').save('assets/store_menu.png')
for src, dst in ((soda, 'assets/drink_soda.png'), (mon, 'assets/drink_monster.png')):
    d = Image.open(src).convert('RGBA'); d.resize((46, round(46 * d.height / d.width)), Image.LANCZOS).save(dst)
