"""v6.2: builds the new room pictures, the gift icons, the romance bar frame and the placeholder 'after the pill' pictures.
Usage: python3 tools/make_romance_assets.py <folder with the artist's uploads>
Writes assets/map_room.png + assets/map_room_up.png (11 x 7 tiles of 24 px, cropped from the 312x216 exports),
assets/gift_*.png (icons), assets/romance_bar.png (heart + empty track, the fill is drawn by main.c) and prints the track geometry."""
import sys, os
from PIL import Image
up = sys.argv[1] if len(sys.argv) > 1 else '/mnt/user-data/uploads'
def room(src, dst):
    im = Image.open(os.path.join(up, src)).convert('RGBA'); im.crop((24, 24, 288, 192)).save(dst)   # the frame starts one tile in; the grey strip on the right is cut off
room('my_room_new.png', 'assets/map_room.png'); room('my_room_new_upgrade.png', 'assets/map_room_up.png')
def icon(src, dst, h):
    im = Image.open(os.path.join(up, src)).convert('RGBA'); im = im.crop(im.getbbox())
    w = max(1, round(im.width * h / im.height)); im.resize((w, h), Image.NEAREST if im.height > h else Image.LANCZOS).save(dst)
icon('flower.PNG', 'assets/gift_flower.png', 72); icon('a-stack-of-cash-bills-bound-with-a-64x64-pixel-art-0afa.png', 'assets/gift_cash.png', 72)
icon('pill.PNG', 'assets/gift_pill.png', 72); icon('Vaan_s_Ruby_Ring.png', 'assets/gift_ring.png', 72)
bar = Image.open(os.path.join(up, 'bar.PNG')).convert('RGBA'); SC = 250 / bar.width
fw, fh = 250, round(bar.height * SC)
bar.resize((fw, fh), Image.LANCZOS).save('assets/romance_bar.png')
print('BAR', fw, fh, 'track x', round(85 * SC), round(540 * SC), 'y', round(39 * SC), round(80 * SC))
# PLACEHOLDERS for "after the pill": the same pictures with a pink wash (replace these four files with the real art; same size, same names)
for n in ('confident', 'her_neutral', 'her_shock', 'her_smug'):
    im = Image.open('assets/%s.png' % n).convert('RGBA'); r, g, b, a = im.split()
    wash = Image.new('RGBA', im.size, (255, 120, 190, 0)); wash.putalpha(a.point(lambda v: 70 if v > 0 else 0))
    out = Image.alpha_composite(im, wash); out.save('assets/%s_p.png' % n)
# her background for the romance scene: a copy of her room (replace it with the real picture later)
import shutil; shutil.copy('assets/her_room.png', 'assets/rom_room.png')
