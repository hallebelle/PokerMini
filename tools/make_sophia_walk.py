"""v8.0: Sophia's walking sheet for the living room.
Input  Sophia_sprite.png  (133 x 282, 3 frames x 4 rows: down, left, right, up - the same order as player.png)
Output assets/sophia_walk.png (135 x 284): every frame in an even 45 x 71 cell, centred on the head and torso (the source frames are
cut tight, so the body would jump sideways while she walks), feet on the bottom line of the row."""
import sys
import numpy as np
from PIL import Image
src = Image.open(sys.argv[1] if len(sys.argv) > 1 else "Sophia_sprite.png").convert("RGBA")
al = np.array(src)[:, :, 3] > 32
def segs(occ):
    out, s = [], None
    for i, v in enumerate(occ):
        if v and s is None: s = i
        if not v and s is not None: out.append((s, i - 1)); s = None
    if s is not None: out.append((s, len(occ) - 1))
    return out
cs, rs = segs(al.any(0)), segs(al.any(1))
assert len(cs) == 3 and len(rs) == 4, (cs, rs)
CW, CH = 45, 71
out = Image.new("RGBA", (CW * 3, CH * 4), (0, 0, 0, 0))
for r, (y0, y1) in enumerate(rs):
    for c, (x0, x1) in enumerate(cs):
        sub = al[y0:y1 + 1, x0:x1 + 1]
        top = sub[: max(1, int(sub.shape[0] * 0.45))]          # head and torso
        xs = np.where(top.any(0))[0]
        mid = x0 + (xs.min() + xs.max()) / 2.0                 # their centre in the source
        frame = src.crop((x0, y0, x1 + 1, y1 + 1))
        px = int(round(CW / 2.0 - (mid - x0)))                 # put that centre in the middle of the cell
        px = max(0, min(CW - frame.width, px))
        py = CH - frame.height                                 # feet at the bottom of the cell
        out.paste(frame, (c * CW + px, r * CH + py), frame)
out.save("assets/sophia_walk.png")
print("assets/sophia_walk.png", out.size)
