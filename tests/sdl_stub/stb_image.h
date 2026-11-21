/* Local stand-in for stb_image: loads PNGs through python3 + Pillow (tests/sdl_stub/png2raw.py). */
#ifndef STB_STUB_IMAGE_H
#define STB_STUB_IMAGE_H
#include <stdio.h>
#include <stdlib.h>
#ifdef STB_IMAGE_IMPLEMENTATION
static unsigned char *stbi_load(const char *path, int *w, int *h, int *n, int req) {
    char cmd[512]; FILE *p; unsigned char *d; (void)req;
    snprintf(cmd, sizeof cmd, "python3 tests/sdl_stub/png2raw.py '%s' 2>/dev/null", path);
    p = popen(cmd, "r"); if (!p) return NULL;
    if (fread(w, 4, 1, p) != 1 || fread(h, 4, 1, p) != 1) { pclose(p); return NULL; }
    d = (unsigned char *)malloc((size_t)*w * *h * 4);
    if (fread(d, 1, (size_t)*w * *h * 4, p) != (size_t)*w * *h * 4) { free(d); pclose(p); return NULL; }
    pclose(p); *n = 4; return d;
}
static void stbi_image_free(void *p) { free(p); }
#endif
#endif
