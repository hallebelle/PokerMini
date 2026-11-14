# Builds assets/key_card.png from tools/key_card_src.png (the artist's picture, transparent background).
# The empty margin is cropped away and the card is shrunk to SCALE of the source picture (60%), smooth edges.
# Change SCALE here to make the card on the key card scene bigger or smaller, then run: python3 tools/make_key_card.py
from PIL import Image
SCALE = 0.60
im = Image.open("tools/key_card_src.png").convert("RGBA")
im = im.crop(im.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox())
w, h = round(im.width * SCALE), round(im.height * SCALE)
im.resize((w, h), Image.LANCZOS).save("assets/key_card.png")
print("assets/key_card.png", w, "x", h)
