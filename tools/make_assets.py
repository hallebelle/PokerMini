#!/usr/bin/env python3
"""Regenerates the chip, banner and digit images in assets/ from tools/chip_sheet.png.
Usage: python3 tools/make_assets.py [NUMERAL_FONT.ttf] [BANNER_FONT.ttf]
Needs Pillow. Change the fonts here to restyle the banner and chip numbers."""
import sys
from PIL import Image, ImageDraw, ImageFont
NUM_FONT = sys.argv[1] if len(sys.argv) > 1 else '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
BAN_FONT = sys.argv[2] if len(sys.argv) > 2 else '/usr/share/texmf/fonts/opentype/public/tex-gyre/texgyrepagella-bolditalic.otf'
VALUES = ['1', '10', '25', '50']              # black, blue, red, magenta
QUADS = [(0,0,240,240), (240,0,480,240), (0,240,240,480), (240,240,480,480)]
sheet = Image.open('tools/chip_sheet.png').convert('RGBA')

def resize(im, size):                          # premultiplied resize avoids dark fringes
    return im.convert('RGBa').resize(size, Image.LANCZOS).convert('RGBA')

def base_chip(i):
    im = sheet.crop(QUADS[i]); im = im.crop(im.split()[3].getbbox())
    s = max(im.size); out = Image.new('RGBA', (s, s), (0,0,0,0)); out.paste(im, ((s-im.width)//2, (s-im.height)//2)); return out

def with_number(im, txt):
    im = im.copy(); d = ImageDraw.Draw(im); s = im.width; size = int(s*0.5)
    while True:
        f = ImageFont.truetype(NUM_FONT, size); bb = d.textbbox((0,0), txt, font=f)
        if bb[2]-bb[0] <= s*0.42: break
        size -= 2
    d.text((s/2, s/2), txt, font=f, fill=(0,0,0,255), anchor='mm'); return im

def dim(im):
    a = im.split()[3].point(lambda v: int(v*0.35)); im = im.copy(); im.putalpha(a); return im

for i, v in enumerate(VALUES):
    c = base_chip(i); n = resize(with_number(c, v), (32,32))
    n.save(f'assets/chip_{v}.png'); dim(n).save(f'assets/chip_{v}_dim.png')
    top = resize(c, (28,17)); edge = Image.new('RGBA', (28,20), (0,0,0,0))   # flat chip for pot stacks
    dark = top.copy(); px = dark.load()
    for y in range(dark.height):
        for x in range(dark.width):
            r,g,b,a = px[x,y]; px[x,y] = (int(r*.5), int(g*.5), int(b*.5), a)
    edge.alpha_composite(dark, (0,3)); edge.alpha_composite(top, (0,0)); edge.save(f'assets/potchip_{v}.png')
resize(with_number(base_chip(0), '1'), (26,26)).save('assets/chip_icon.png')

K = 3   # supersample for smooth edges
def gradient(w, h, top, bot):
    g = Image.new('RGBA', (w, h))
    for y in range(h):
        t = y/max(1,h-1); g.paste(tuple(int(top[j]+(bot[j]-top[j])*t) for j in range(3))+(255,), (0,y,w,y+1))
    return g

def title(text, top, bot, outline, maxw=215, maxh=44):
    size = 90
    while True:
        f = ImageFont.truetype(BAN_FONT, size*K); st = 3*K
        bb = ImageDraw.Draw(Image.new('L',(1,1))).textbbox((0,0), text, font=f, stroke_width=st)
        if (bb[2]-bb[0])/K <= maxw and (bb[3]-bb[1])/K <= maxh: break
        size -= 1
    w, h = bb[2]-bb[0], bb[3]-bb[1]; im = Image.new('RGBA', (w, h), (0,0,0,0))
    ImageDraw.Draw(im).text((-bb[0], -bb[1]), text, font=f, fill=outline+(255,), stroke_width=st, stroke_fill=outline+(255,))
    mask = Image.new('L', (w, h), 0); ImageDraw.Draw(mask).text((-bb[0], -bb[1]), text, font=f, fill=255)
    im.paste(gradient(w, h, top, bot), (0,0), mask)
    return resize(im, (w//K, h//K))

GOLD = ((255,232,110), (225,160,20), (45,28,0))
RED  = ((235,150,150), (170,60,60), (45,10,10))
SILV = ((235,240,250), (150,165,190), (25,30,45))
title('YOU WIN', *GOLD).save('assets/banner_win.png')
title('YOU LOSE', *RED).save('assets/banner_lose.png')
title('SPLIT POT', *SILV).save('assets/banner_split.png')

def digits(cols, name, cw=17, ch=26):
    chars = '0123456789+-'; atlas = Image.new('RGBA', (cw*len(chars), ch), (0,0,0,0)); f = ImageFont.truetype(BAN_FONT, 22*K)
    for i, c in enumerate(chars):
        cell = Image.new('RGBA', (cw*K, ch*K), (0,0,0,0)); d = ImageDraw.Draw(cell)
        d.text((cw*K/2, ch*K/2), c, font=f, fill=cols[2]+(255,), stroke_width=2*K, stroke_fill=cols[2]+(255,), anchor='mm')
        m = Image.new('L', cell.size, 0); ImageDraw.Draw(m).text((cw*K/2, ch*K/2), c, font=f, fill=255, anchor='mm')
        cell.paste(gradient(cell.width, cell.height, cols[0], cols[1]), (0,0), m)
        atlas.alpha_composite(resize(cell, (cw, ch)), (i*cw, 0))
    atlas.save(name)
digits(GOLD, 'assets/banner_digits.png'); digits(RED, 'assets/banner_digits_lose.png')
print('assets written')
