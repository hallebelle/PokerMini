# v11.0: the FRIENDSHIP heart = the romance heart (assets/romance_heart.png) with its hue turned from pink to blue.
# Run from the project root: python3 tools/make_friend_heart.py
from PIL import Image
import colorsys
im = Image.open('assets/romance_heart.png').convert('RGBA')
px = im.load()
for y in range(im.height):
    for x in range(im.width):
        r, g, b, a = px[x, y]
        if a == 0: continue
        h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
        if s > 0.15:              # coloured pixels only (keep white shine / dark outline as they are)
            h = 0.60              # blue
        r2, g2, b2 = colorsys.hsv_to_rgb(h, s, v)
        px[x, y] = (int(r2 * 255), int(g2 * 255), int(b2 * 255), a)
im.save('assets/friend_heart.png')
print('friend_heart.png', im.size)
