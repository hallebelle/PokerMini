/* Minimal software stand-in for SDL2 (only what src/main.c uses). For local checks of --shots / --autoplay where SDL is not installed.
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
   Build: gcc -O1 -Itests/sdl_stub src/main.c -o pq_test -lm   then run ./pq_test --autoplay or --shots (needs python3 + Pillow for PNG I/O). NOT used by CI. */
#ifndef SDL_STUB_H
#define SDL_STUB_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t Uint8; typedef uint16_t Uint16; typedef uint32_t Uint32; typedef int16_t Sint16;
typedef struct { int x, y, w, h; } SDL_Rect;
typedef struct { Uint32 Rmask, Gmask, Bmask, Amask; Uint8 Rshift, Gshift, Bshift, Ashift; Uint8 BitsPerPixel; } SDL_PixelFormat;
typedef struct { SDL_PixelFormat *format; int w, h, pitch; void *pixels; int owns; } SDL_Surface;
typedef struct SDL_Window SDL_Window;
typedef struct { int type; struct { struct { int sym; } keysym; } key; } SDL_Event;
enum { SDL_KEYDOWN = 1, SDL_KEYUP, SDL_QUIT, SDL_INIT_VIDEO = 1, SDL_WINDOWPOS_CENTERED = 0, SDL_PIXELFORMAT_ARGB8888 = 1 };
enum { SDLK_ESCAPE = 27, SDLK_RCTRL = 300, SDLK_SPACE = 32, SDLK_z = 'z', SDLK_LCTRL = 301, SDLK_x = 'x', SDLK_LSHIFT = 302, SDLK_c = 'c', SDLK_LALT = 303,
       SDLK_v = 'v', SDLK_t = 't', SDLK_e = 'e', SDLK_UP = 273, SDLK_DOWN, SDLK_RIGHT, SDLK_LEFT, SDLK_RETURN = 13, SDLK_BACKSPACE = 8, SDLK_r = 'r', SDLK_s = 's' };
static unsigned st_ticks;
static inline int SDL_Init(Uint32 f) { (void)f; return 0; }
static inline void SDL_Quit(void) {}
static inline const char *SDL_GetError(void) { return "stub"; }
static inline void SDL_ShowCursor(int x) { (void)x; }
static inline void SDL_Delay(Uint32 ms) { st_ticks += ms; }
static inline Uint32 SDL_GetTicks(void) { return st_ticks; }
static inline int SDL_PollEvent(SDL_Event *e) { (void)e; return 0; }
static inline void sh_from_mask(Uint32 m, Uint8 *s) { *s = 0; if (!m) return; while (!((m >> *s) & 1)) (*s)++; }
static inline SDL_Surface *SDL_CreateRGBSurface(Uint32 fl, int w, int h, int d, Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    SDL_Surface *s = (SDL_Surface *)calloc(1, sizeof *s); (void)fl; (void)d;
    s->format = (SDL_PixelFormat *)calloc(1, sizeof *s->format); s->w = w; s->h = h; s->pitch = w * 4; s->pixels = calloc((size_t)w * h, 4); s->owns = 1;
    s->format->BitsPerPixel = 32; s->format->Rmask = r; s->format->Gmask = g; s->format->Bmask = b; s->format->Amask = a;
    sh_from_mask(r, &s->format->Rshift); sh_from_mask(g, &s->format->Gshift); sh_from_mask(b, &s->format->Bshift); sh_from_mask(a, &s->format->Ashift);
    return s;
}
static inline SDL_Surface *SDL_CreateRGBSurfaceFrom(void *px, int w, int h, int d, int pitch, Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    SDL_Surface *s = SDL_CreateRGBSurface(0, 1, 1, d, r, g, b, a); free(s->pixels); s->pixels = px; s->w = w; s->h = h; s->pitch = pitch; s->owns = 0; return s;
}
static inline void SDL_FreeSurface(SDL_Surface *s) { if (!s) return; if (s->owns) free(s->pixels); free(s->format); free(s); }
static inline Uint32 SDL_MapRGB(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b) { return ((Uint32)r << f->Rshift) | ((Uint32)g << f->Gshift) | ((Uint32)b << f->Bshift) | f->Amask; }
static inline void SDL_GetRGB(Uint32 p, const SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b) { *r = (p >> f->Rshift) & 255; *g = (p >> f->Gshift) & 255; *b = (p >> f->Bshift) & 255; }
static inline int SDL_FillRect(SDL_Surface *d, const SDL_Rect *r, Uint32 c) {
    int x0 = r ? r->x : 0, y0 = r ? r->y : 0, x1 = r ? r->x + r->w : d->w, y1 = r ? r->y + r->h : d->h, x, y;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 > d->w) x1 = d->w; if (y1 > d->h) y1 = d->h;
    for (y = y0; y < y1; y++) for (x = x0; x < x1; x++) ((Uint32 *)((Uint8 *)d->pixels + y * d->pitch))[x] = c;
    return 0;
}
static inline int SDL_BlitSurface(SDL_Surface *s, const SDL_Rect *sr, SDL_Surface *d, SDL_Rect *dr) {
    int sx = sr ? sr->x : 0, sy = sr ? sr->y : 0, w = sr ? sr->w : s->w, h = sr ? sr->h : s->h, dx = dr ? dr->x : 0, dy = dr ? dr->y : 0, i, j;
    const SDL_PixelFormat *f = s->format, *g = d->format; int alpha = f->Amask != 0;
    if (sx < 0) { w += sx; dx -= sx; sx = 0; } if (sy < 0) { h += sy; dy -= sy; sy = 0; }
    if (dx < 0) { w += dx; sx -= dx; dx = 0; } if (dy < 0) { h += dy; sy -= dy; dy = 0; }
    if (sx + w > s->w) w = s->w - sx; if (sy + h > s->h) h = s->h - sy;
    if (dx + w > d->w) w = d->w - dx; if (dy + h > d->h) h = d->h - dy;
    for (j = 0; j < h; j++) for (i = 0; i < w; i++) {
        Uint32 v = ((Uint32 *)((Uint8 *)s->pixels + (sy + j) * s->pitch))[sx + i], *o = &((Uint32 *)((Uint8 *)d->pixels + (dy + j) * d->pitch))[dx + i];
        int sa = alpha ? (int)((v & f->Amask) >> f->Ashift) : 255, sr_ = (v >> f->Rshift) & 255, sg = (v >> f->Gshift) & 255, sb = (v >> f->Bshift) & 255;
        if (sa == 0) continue;
        if (sa < 255) { int dr_ = (*o >> g->Rshift) & 255, dg = (*o >> g->Gshift) & 255, db = (*o >> g->Bshift) & 255;
            sr_ = (dr_ * (255 - sa) + sr_ * sa) / 255; sg = (dg * (255 - sa) + sg * sa) / 255; sb = (db * (255 - sa) + sb * sa) / 255; }
        *o = ((Uint32)sr_ << g->Rshift) | ((Uint32)sg << g->Gshift) | ((Uint32)sb << g->Bshift) | g->Amask;
    }
    return 0;
}
static inline SDL_Surface *SDL_ConvertSurfaceFormat(SDL_Surface *s, Uint32 fmt, Uint32 fl) {
    SDL_Surface *n = SDL_CreateRGBSurface(0, s->w, s->h, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000); int x, y; (void)fmt; (void)fl;
    for (y = 0; y < s->h; y++) for (x = 0; x < s->w; x++) {
        Uint32 v = ((Uint32 *)((Uint8 *)s->pixels + y * s->pitch))[x]; const SDL_PixelFormat *f = s->format;
        Uint32 a = f->Amask ? (v & f->Amask) >> f->Ashift : 255;
        ((Uint32 *)n->pixels)[y * s->w + x] = (a << 24) | (((v >> f->Rshift) & 255) << 16) | (((v >> f->Gshift) & 255) << 8) | ((v >> f->Bshift) & 255);
    }
    return n;
}
static SDL_Surface *st_screen; static int st_w = 640, st_h = 480;
static inline SDL_Window *SDL_CreateWindow(const char *t, int x, int y, int w, int h, Uint32 f) { (void)t; (void)x; (void)y; (void)f; st_w = w; st_h = h; return (SDL_Window *)1; }
static inline SDL_Surface *SDL_GetWindowSurface(SDL_Window *w) { (void)w; if (!st_screen) st_screen = SDL_CreateRGBSurface(0, st_w, st_h, 32, 0xFF0000, 0xFF00, 0xFF, 0); return st_screen; }
static inline int SDL_UpdateWindowSurface(SDL_Window *w) { (void)w; return 0; }
#endif
