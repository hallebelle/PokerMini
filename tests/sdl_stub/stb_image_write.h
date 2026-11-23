#ifndef STB_STUB_WRITE_H
#define STB_STUB_WRITE_H
#include <stdio.h>
#ifdef STB_IMAGE_WRITE_IMPLEMENTATION
static int stbi_write_png(const char *name, int w, int h, int n, const void *data, int stride) {
    char cmd[512]; FILE *p; (void)n; (void)stride;
    snprintf(cmd, sizeof cmd, "python3 tests/sdl_stub/raw2png.py '%s' %d %d", name, w, h);
    p = popen(cmd, "w"); if (!p) return 0; fwrite(data, 1, (size_t)w * h * 3, p); pclose(p); return 1;
}
#endif
#endif
