#!/usr/bin/env python3
"""v8.1: the company logos of the STOCKS tab. tools/stock_src/<name>.png (336x336, black background) -> assets/stk_<name>.png (80x80, opaque,
drawn on a black tile in the stock row). Run from the repository root:  python3 tools/make_stock_logos.py"""
from PIL import Image
for n in ["nvidia", "google", "openai", "apple", "sony", "spacex"]:
    im = Image.open("tools/stock_src/%s.png" % n).convert("RGB").resize((80, 80), Image.LANCZOS)
    im.save("assets/stk_%s.png" % n); print("assets/stk_%s.png" % n)
