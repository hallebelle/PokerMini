/* test shim: lets the -DUSE_SDL1 code path (double buffer + present) be built and run on the desktop with the SDL2 stub.
   SHIM_NOHW=1 makes the double-buffered mode "fail" (to test the fallback); SHIM_DUMP=file writes the first flipped frame (raw BGRA) and exits. */
#ifndef SDL1_SHIM_H
#define SDL1_SHIM_H
#include "../../sdl_stub/SDL2/SDL.h"
#include <stdio.h>
#define SDL_SWSURFACE 0
#define SDL_HWSURFACE 1
#define SDL_DOUBLEBUF 2
#define SDL_MUSTLOCK(s) 0
static inline SDL_Surface *SDL_DisplayFormatAlpha(SDL_Surface *s) { return SDL_ConvertSurfaceFormat(s, 0, 0); }
static inline int SDL_LockSurface(SDL_Surface *s) { (void)s; return 0; }
static inline void SDL_UnlockSurface(SDL_Surface *s) { (void)s; }
static inline SDL_Surface *SDL_SetVideoMode(int w, int h, int d, Uint32 fl) { (void)d; if (getenv("SHIM_NOHW") && (fl & SDL_DOUBLEBUF)) return NULL; return SDL_CreateRGBSurface(0, w, h, 32, 0xFF0000, 0xFF00, 0xFF, 0); }
static inline int SDL_Flip(SDL_Surface *s) { const char *f = getenv("SHIM_DUMP"); if (f) { FILE *o = fopen(f, "wb"); fwrite(s->pixels, 4, (size_t)s->w * s->h, o); fclose(o); exit(0); } return 0; }
#endif
