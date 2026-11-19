import sys
from PIL import Image
name, w, h = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
Image.frombytes("RGB", (w, h), sys.stdin.buffer.read(w * h * 3)).save(name)
