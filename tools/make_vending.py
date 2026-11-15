"""v12.5: builds assets/store_menu.png = the lobby vending machine, 752x560, from tools/vend_src/vend2.png (the user's own 645x484 picture, prices already painted on it).
Run from the repo root:  python3 tools/make_vending.py
The only thing added to the picture is a pixel button X (soda, top shelf) and Y (energy drink, bottom shelf) in the white strip left of each orange price box.
The picture is static (no happy face, no falling can). The v12.4 files vend.PNG / store_menu_happy.png are no longer used."""
from PIL import Image
src = Image.open('tools/vend_src/vend2.png').convert('RGB')
DARK, FILL, LIGHT, WHITE = (54, 41, 58), (172, 31, 63), (214, 70, 100), (255, 255, 255)
GLYPH = {'X': ['10001', '01010', '00100', '01010', '10001'],
         'Y': ['10001', '01010', '00100', '00100', '00100']}
def button(im, x0, y0, w, h, ch):   # works on the original 645x484 grid
    px = im.load()
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            edge = x < x0 + 2 or x >= x0 + w - 2 or y < y0 + 2 or y >= y0 + h - 2
            px[x, y] = DARK if edge else FILL
    for x in range(x0 + 2, x0 + w - 2): px[x, y0 + 2] = LIGHT     # 1 px highlight on top
    for x in (x0, x0 + w - 1):                                    # round the 4 corners
        for y in (y0, y0 + h - 1): px[x, y] = (255, 255, 255)
    g = GLYPH[ch]; gs = 3; gx = x0 + (w - 5 * gs) // 2; gy = y0 + (h - 5 * gs) // 2
    for r, row in enumerate(g):
        for c, v in enumerate(row):
            if v == '1':
                for dy in range(gs):
                    for dx in range(gs): px[gx + c * gs + dx, gy + r * gs + dy] = WHITE
im = src.copy()
button(im, 26, 186, 32, 26, 'X')    # top shelf (soda, $5): the orange box is x 60..160, y 185..214
button(im, 26, 398, 32, 26, 'Y')    # bottom shelf (energy drink, $10): the orange box is x 61..161, y 397..424
im.resize((752, 560), Image.LANCZOS).save('assets/store_menu.png')
print('store_menu.png 752x560 written')
