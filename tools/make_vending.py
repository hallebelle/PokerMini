"""v12.4: builds assets/store_menu.png (+ store_menu_happy.png) = the lobby vending machine, 752x560, from tools/vend_src/vend.PNG.
Run from the repo root:  python3 tools/make_vending.py
The machine is scaled 3x (nearest neighbour), pushed to the right edge; the left column stays black for the info panel drawn by the game.
Middle shelf, slots 1-3 = SODA cans, bottom shelf, slots 1-3 = ENERGY DRINK (Monster) cans, from assets/drink_soda.png / drink_monster.png.
The happy picture only differs in the smiley (the mouth is flipped)."""
from PIL import Image
S = 3; CROP_X = 6; CW, CH = 752, 560
src = Image.open('tools/vend_src/vend.PNG').convert('RGBA').crop((CROP_X, 0, 210, 178))
def build(happy):
    s = src.copy()
    if happy:   # flip the frown into a smile (mouth rows 30..34, columns 181..194 of the original)
        box = (181 - CROP_X, 30, 195 - CROP_X, 35); m = s.crop(box).transpose(Image.FLIP_TOP_BOTTOM); s.paste(m, box)
    big = s.resize((s.width * S, s.height * S), Image.NEAREST)
    soda = Image.open('assets/drink_soda.png').convert('RGBA'); mon = Image.open('assets/drink_monster.png').convert('RGBA')
    px = s.load()
    for (y0, y1, base, icon) in ((77, 110, 110, soda), (126, 158, 158, mon)):   # rows of the original: fill, shelf bottom
        for y in range(y0, y1 + 1):
            c = px[32 - CROP_X, y]   # the empty gap between cans 1 and 2 gives the wall colour of this row
            for x in range(8 - CROP_X, 83 - CROP_X):
                for dy in range(S):
                    for dx in range(S): big.putpixel((x * S + dx, y * S + dy), c)
        for cx in (19, 45, 71):     # slot centres (original columns)
            ix = (cx - CROP_X) * S - icon.width // 2; iy = base * S - icon.height + 2
            big.alpha_composite(icon, (ix, iy))
    out = Image.new('RGBA', (CW, CH), (0, 0, 0, 255))
    out.alpha_composite(big, (CW - big.width, (CH - big.height) // 2))
    return out
build(False).save('assets/store_menu.png'); build(True).save('assets/store_menu_happy.png')
print('machine at x', CW - (210 - CROP_X) * S, 'y', (CH - 178 * S) // 2)
