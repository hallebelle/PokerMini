import sys, struct
from PIL import Image
im = Image.open(sys.argv[1]).convert("RGBA")
sys.stdout.buffer.write(struct.pack("ii", im.width, im.height) + im.tobytes())
