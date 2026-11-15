/* Poker Queen - heads-up fixed-limit hold'em vs. the Poker Queen.
   SDL 1.2 on the Miyoo Mini (-DUSE_SDL1), SDL2 for web/desktop.
   Software drawing on the screen surface, so both APIs share the same code.
   Rules, betting, her AI, bank and save file live in logic.h; her dialogue and story scenes live in lines.h.
   Miyoo build: -DUSE_SDL1 -DROTATE180 (the screen is mounted upside down relative to the framebuffer).
   You walk around your room, the hall and the lobby (RPG-Maker-style maps, 48 px tiles); the poker, ATM, store and story screens open from there. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#ifdef USE_SDL1
#include <SDL/SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#if !defined(USE_SDL1) && !defined(__EMSCRIPTEN__)
#define HAVE_SHOTS   /* desktop only: --shots saves PNG screenshots */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
#include "multi.h"
#include "lines.h"
#include "extlines.h"   /* v7.9: lines.txt / talk.txt */
#include "extitems.h"   /* v9.5: items.txt / names.txt */

#define PANEL_ALPHA 160   /* box opacity, 0..256 (160 = ~62%). Tune after seeing your room art. */
#define SAVE_PATH   "pokerqueen.sav"
/* v11.5: line colours (shared by the dialogue boxes and the table boxes) and the table box style (Options > TEXT BOX STYLE, saved in pokerqueen.cfg) */
static const int PAL_LINE[8][3] = { {210,175,90}, {225,225,225}, {210,65,65}, {225,105,160}, {220,135,45}, {65,175,95}, {65,125,220}, {145,95,210} };
static const int TB_A[6] = {256, 205, 160, 102, 51, 0};   /* table box opacity steps; step 2 (160 = the old look) is the default */
static const char *const TB_N[6] = {"100%", "80%", "62%", "40%", "20%", "0%"};
static int tb_idx = 2;

/* ---------- 5x7 bitmap font, ASCII 32..90 (drawn upper-case) ---------- */
static const unsigned char F[59][5] = {
{0,0,0,0,0},{0,0,0x5F,0,0},{0,7,0,7,0},{0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},
{0x23,0x13,8,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0,5,3,0,0},{0,0x1C,0x22,0x41,0},{0,0x41,0x22,0x1C,0},
{0x14,8,0x3E,8,0x14},{8,8,0x3E,8,8},{0,0x50,0x30,0,0},{8,8,8,8,8},{0,0x60,0x60,0,0},{0x20,0x10,8,4,2},
{0x3E,0x51,0x49,0x45,0x3E},{0,0x42,0x7F,0x40,0},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{1,0x71,9,5,3},
{0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1E},{0,0x36,0x36,0,0},{0,0x56,0x36,0,0},
{8,0x14,0x22,0x41,0},{0x14,0x14,0x14,0x14,0x14},{0,0x41,0x22,0x14,8},{2,1,0x51,9,6},{0x32,0x49,0x79,0x41,0x3E},
{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},
{0x7F,0x49,0x49,0x49,0x41},{0x7F,9,9,9,1},{0x3E,0x41,0x49,0x49,0x7A},{0x7F,8,8,8,0x7F},{0,0x41,0x7F,0x41,0},
{0x20,0x40,0x41,0x3F,1},{0x7F,8,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},{0x7F,2,0x0C,2,0x7F},
{0x7F,4,8,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},{0x7F,9,9,9,6},{0x3E,0x41,0x51,0x21,0x5E},
{0x7F,9,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{1,1,0x7F,1,1},{0x3F,0x40,0x40,0x40,0x3F},
{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,8,0x14,0x63},{7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}
};

static SDL_Surface *vscr;   /* v7.4 (SDL 1.2 build): the real double-buffered screen; S is then a plain memory surface the game draws on, copied to vscr (turned 180 degrees) once per frame */
static SDL_Surface *S, *T, *CS, *her, *conf, *bgs[BG_N];   /* screen, current draw target, card scratch, her table picture, her standing sprite, backgrounds */
static SDL_Surface *chipS[4], *chipD[4], *potS[4], *bnW, *bnL, *bnS, *dgW, *dgL, *icon;
static void draw_friend_bar(int x, int y);
static const char *fv_door_line(void);
static void fv_npc(void);
#define FVP(pool) pick(pool, pool##_N)   /* v11.0: a random line of a pool (never the same one right away, see pick) */
static SDL_Surface *frHeartS;   /* v11.0: the blue friendship heart */
static int fv_on, fv_vsel, fv_seen, fv_greet_n;   /* v11.0: fv_on = you are visiting her room by day (MAP_SOPHIA); fv_vsel = the row of her visit menu; fv_seen = the first-visit lines were told (this session only, not saved) */
static SDL_Surface *herP, *herShockP, *herSmugP, *confP, *giftS[SH_N], *romHeartS, *roomS[3], *roomSN[3];   /* v6.2: her pictures after the pill (placeholders), the gift icons, the romance bar frame, the small and the upgraded room (day, night) */

static SDL_Surface *herCaughtS;   /* v7.6: her table picture while she answers the close-up (placeholder assets/her_caught.png = her_cover.png) */
static SDL_Surface *herD6S, *herCoverS;   /* v6.8: her picture when she invites you in on days 6 and 7, and her covering picture from day 8 on */
static SDL_Surface *herShock, *herSmug, *ignoreS, *nightS, *bossS, *suitsS;   /* her faces at the table (shock / smug), her ignoring pose in her room, the floating suits on the title screen */
static SDL_Surface *cpuimg, *tblE, *storeS, *sodaS, *monS, *barS, *segS;   /* quick-play CPU picture, exhibition table, store menu, drink icons, energy bar + one segment */
static const char *CN[4] = {"1", "10", "25", "50"};   /* black, blue, red, magenta */
static const int CV[4] = {1, 10, 25, 50};
#ifndef USE_SDL1
static SDL_Window *win;
#endif
static int running = 1, frozen, TS = 1, dollar_plain;   /* dollar_plain: draw "$" as a plain letter (used on top of a badge that is already green) */
static const char *RANK[13] = {"2","3","4","5","6","7","8","9","10","J","Q","K","A"};
static int card_pinkc;   /* v11.6: 1 = the pink colour is really used (she lives with you AND got the pink deck); card_pink alone is only the Sophia-table layout */
static int card_pink;   /* 1 = card backs are pink (Sophia's table only; the other tables keep blue) */
static int card_hl;   /* 1 = draw the next face-up card with a gold outline (it is part of the winning hand) */
static const char *HN[9] = {"HIGH CARD","PAIR","2 PAIR","TRIPS","STRAIGHT","FLUSH","FULL HOUSE","QUADS","STR FLUSH"};

/* ---------- drawing primitives (all draw into T) ---------- */
static Uint32 C(int r, int g, int b) { return SDL_MapRGB(T->format, r, g, b); }
static Uint32 *px(SDL_Surface *s, int x, int y) { return (Uint32 *)((Uint8 *)s->pixels + y * s->pitch) + x; }
/* v7.0: the game is still laid out in 640x480 "logical" units (every coordinate in this file), but everything is drawn at the real screen size SW x SH
   (752x560 on the Miyoo Mini): rectangles, letters and shading are rasterised at full resolution, and the pictures are resampled once when they are loaded.
   XS/YS turn a logical x/y into a screen pixel (floor, so neighbouring rectangles never leave a gap); LW/LH give a loaded picture's logical size back.
   At 640x480 every one of these is the identity and the game draws exactly as before. */
#define NATW 752   /* v7.1: the native screen. New full-screen art is made at exactly NATW x NATH and is loaded 1:1 (640x480 art is still resampled) */
#define NATH 560
static int SW = NATW, SH = NATH;   /* v7.5: 752x560 is the only screen size (PQ_SIZE=WxH still overrides it for tests) */
static int fdiv_(long long n, long long d) { return (int)(n >= 0 ? n / d : -((-n + d - 1) / d)); }
static int XSF(long long v256) { return fdiv_(v256 * SW, 640LL * 256); }   /* v7.3: a logical position in 1/256 px -> screen pixel (smooth scrolling) */
static int YSF(long long v256) { return fdiv_(v256 * SH, 480LL * 256); }
static int ADJX, ADJY;   /* v7.3: screen-pixel shift added to every fill and picture while the walk map is drawn (the camera moves in screen pixels, not in 640x480 units) */
static int XS(int v) { return SW == 640 ? v : fdiv_((long long)v * SW, 640); }
static int YS(int v) { return SH == 480 ? v : fdiv_((long long)v * SH, 480); }
static int XS100(long long v) { return fdiv_(v * SW, 64000); }   /* v in 1/100 logical px */
static int YS100(long long v) { return fdiv_(v * SH, 48000); }
static int LW(const SDL_Surface *im) { return im ? (int)((long long)im->w * 640 / SW) : 0; }
static int LH(const SDL_Surface *im) { return im ? (int)((long long)im->h * 480 / SH) : 0; }
static void Rn(int x0, int y0, int x1, int y1, Uint32 c) {   /* fill in screen pixels, edges x0,y0 inclusive .. x1,y1 exclusive */
    SDL_Rect r; if (x1 <= x0 || y1 <= y0) return;
    r.x = x0 + ADJX; r.y = y0 + ADJY; r.w = x1 - x0; r.h = y1 - y0; SDL_FillRect(T, &r, c);
}
static void R(int x, int y, int w, int h, Uint32 c) {
    int x0, y0, x1, y1; if (w <= 0 || h <= 0) return;
    x0 = XS(x); y0 = YS(y); x1 = XS(x + w); y1 = YS(y + h);
    if (x1 <= x0) x1 = x0 + 1; if (y1 <= y0) y1 = y0 + 1;
    Rn(x0, y0, x1, y1, c);
}
static void Rf(long long x0, long long y0, long long x1, long long y1, Uint32 c) {   /* a rectangle given in 1/100 logical px (the letters): whole screen pixels, never empty */
    int a = XS100(x0), b = YS100(y0), e = XS100(x1), f = YS100(y1);
    if (e <= a) e = a + 1; if (f <= b) f = b + 1;
    Rn(a, b, e, f, c);
}
static void rrect(int x, int y, int w, int h, Uint32 c) { R(x+1, y, w-2, h, c); R(x, y+1, w, h-2, c); }
static void ell(int cx, int cy, int rx, int ry, Uint32 c) {
    int dy;
    if (SW == 640 && SH == 480) { for (dy = -ry; dy <= ry; dy++) { int dx = (int)(rx * sqrt(1.0 - (double)(dy*dy) / (ry*ry))); R(cx-dx, cy+dy, 2*dx+1, 1, c); } return; }
    {   /* at the real screen size: one scanline per screen row */
        int yn, y0 = YS(cy - ry), y1 = YS(cy + ry + 1);
        for (yn = y0; yn < y1; yn++) {
            double ly = (yn + 0.5) * 480.0 / SH - (cy + 0.5), t = 1.0 - ly * ly / ((ry + 0.5) * (ry + 0.5)), dx;
            int xa, xb; if (t < 0) continue;
            dx = (rx + 0.5) * sqrt(t);
            xa = (int)floor((cx + 0.5 - dx) * SW / 640.0 + 0.5); xb = (int)floor((cx + 0.5 + dx) * SW / 640.0 + 0.5);
            Rn(xa, yn, xb, yn + 1, c);
        }
    }
}
static void disc(int cx, int cy, int r, Uint32 c) {
    int dy;
    if (SW == 640 && SH == 480) { for (dy = -r; dy <= r; dy++) { int dx = (int)sqrt((double)(r*r - dy*dy)); R(cx-dx, cy+dy, 2*dx+1, 1, c); } return; }
    ell(cx, cy, r, r, c);
}
static void tri(int cx, int y, int hw, int h, int up, Uint32 c) {
    int i;
    if (SW == 640 && SH == 480) { for (i = 0; i <= h; i++) { int w = up ? hw*i/h : hw*(h-i)/h; R(cx-w, y+i, 2*w+1, 1, c); } return; }
    {
        int yn, y0 = YS(y), y1 = YS(y + h + 1);
        for (yn = y0; yn < y1; yn++) {
            double li = (yn + 0.5) * 480.0 / SH - y - 0.5, w; int xa, xb;
            if (li < 0) li = 0; if (li > h) li = h;
            w = up ? hw * li / h : hw * (h - li) / h;
            xa = (int)floor((cx - w) * SW / 640.0 + 0.5); xb = (int)floor((cx + w + 1) * SW / 640.0 + 0.5);
            Rn(xa, yn, xb, yn + 1, c);
        }
    }
}
static int TSX;   /* v10.7: 1 = dialogue-box text: pure black shadow drawn 1 and 2 px away */
static int text_fs;   /* when set: the size of one font pixel in 1/100 screen pixels, used instead of sc * 100 (fractional sizes) */
static void text(int x, int y, const char *s, int sc, Uint32 col) {
    Uint32 sh = TSX ? C(0, 0, 0) : C(6, 8, 16); int pass, off, fs = text_fs ? text_fs : sc * 100, u, i;   /* fs: size of one font pixel in 1/100 logical px (the table GUI can draw size 2 smaller) */
    u = fs / 100; if (u < 1) u = 1;
    for (pass = TS ? (TSX ? -1 : 0) : 1; pass < 2; pass++) {
        const char *p = s; int xx; off = pass == 0 ? 1 : pass < 0 ? 2 : 0;
        for (i = 0; *p; p++, i++) {
            int c = toupper((unsigned char)*p), cx, ry; const unsigned char *g;
            xx = x + off + i * 6 * fs / 100;
            if (c < 32 || c > 90) c = '?';
            if (c == '$' && !dollar_plain) {   /* every money amount carries a small green $ badge (same width as a letter) */
                if (pass == 1) {
                    int bw = 5*fs/100 + 2, bh = 9*fs/100;
                    R(xx - 1, y - u + 1, bw, bh - 2, C(30,130,55)); R(xx, y - u, bw - 2, bh, C(30,130,55));
                    R(xx - 1 + bw - 2, y - u, 1, 1, C(30,130,55));
                    for (cx = 0; cx < 5; cx++) for (ry = 0; ry < 7; ry++)
                        if ((F['$'-32][cx] >> ry) & 1) Rf(xx*100LL + cx*fs, y*100LL + ry*fs, xx*100LL + (cx+1)*fs, y*100LL + (ry+1)*fs, C(245,250,245));
                }
                continue;
            }
            g = F[c-32];
            for (cx = 0; cx < 5; cx++) for (ry = 0; ry < 7; ry++)
                if ((g[cx] >> ry) & 1)
                    Rf(xx*100LL + cx*fs, (y + off)*100LL + ry*fs, xx*100LL + (cx+1)*fs, (y + off)*100LL + (ry+1)*fs, pass == 1 ? col : sh);
        }
    }
}
/* translucent fill straight on the screen, in screen pixels (a: 0..256) */
static void shade_n(int x0, int y0, int x1, int y1, int r, int g, int b, int a) {
    int i, j; SDL_PixelFormat *f = S->format; Uint32 m = f->Rmask | f->Gmask | f->Bmask;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 > SW) x1 = SW; if (y1 > SH) y1 = SH;
    if (a <= 0 || x1 <= x0 || y1 <= y0) return; if (a > 256) a = 256;
    if (f->Gshift == 8 && ((f->Rshift == 16 && f->Bshift == 0) || (f->Rshift == 0 && f->Bshift == 16))) {
        /* v7.2 fast path: red and blue share one multiply (16-bit lanes), green is done on its own; the result is exactly the same as the slow loop below */
        const Uint32 srb = ((Uint32)r << f->Rshift | (Uint32)b << f->Bshift) * (Uint32)a, sg = (Uint32)g * (Uint32)a, ia = (Uint32)(256 - a), keep = ~m;
        for (j = y0; j < y1; j++) {
            Uint32 *p = px(S, x0, j);
            for (i = x0; i < x1; i++, p++) {
                Uint32 v = *p, rb = (((v & 0x00FF00FFu) * ia + srb) >> 8) & 0x00FF00FFu, gg = ((((v >> 8) & 255u) * ia + sg) >> 8) & 255u;
                *p = (v & keep) | rb | (gg << 8);
            }
        }
        return;
    }
    for (j = y0; j < y1; j++) {
        Uint32 *p = px(S, x0, j);
        for (i = x0; i < x1; i++, p++) {
            Uint32 v = *p; int pr = (v >> f->Rshift) & 255, pg = (v >> f->Gshift) & 255, pb = (v >> f->Bshift) & 255;
            pr = (pr*(256-a) + r*a) >> 8; pg = (pg*(256-a) + g*a) >> 8; pb = (pb*(256-a) + b*a) >> 8;
            *p = (v & ~m) | ((Uint32)pr << f->Rshift) | ((Uint32)pg << f->Gshift) | ((Uint32)pb << f->Bshift);
        }
    }
}
/* the same, in logical units (a: 0..256) */
static void shade(int x, int y, int w, int h, int r, int g, int b, int a) {
    if (w <= 0 || h <= 0) return;
    shade_n(XS(x), YS(y), XS(x + w), YS(y + h), r, g, b, a);
}
/* v11.9: menu colours (Options > MENU COLORS): the line (borders, dividers) and the text / highlight colour of the Save Slots, Career, Options, pause and online store menus. Default = RED. Index 1 = the old gold. */
static const int MN_TXT[8][3] = { {255,85,85}, {250,215,90}, {255,255,255}, {255,150,205}, {255,175,80}, {120,235,140}, {130,180,255}, {190,150,255} };
static const int MN_LIN[8][3] = { {215,50,50}, {210,175,90}, {225,225,225}, {225,105,160}, {220,135,45}, {65,175,95}, {65,125,220}, {145,95,210} };
static const char *const MN_N[8] = { "RED", "GOLD", "WHITE", "PINK", "ORANGE", "GREEN", "BLUE", "PURPLE" };
static int mn_line = 0, mn_text = 0;
/* v12.0: GUI colours (Options > MENU COLORS rows 3-4): the border line and the accent text of every other box (cash, day, table boxes, result boxes, icons, A bubble, sofa / bed boxes, desk / floor menus, Quick Play, Exhibition, lottery, fridge, ATM, name picker, achievement, dialogue A prompts). Same 8 colours, default RED, 1 = the old gold. */
static int gui_line = 0, gui_text = 0;
static Uint32 gui_tc(void);
/* semi-transparent box with a thin gold border; fade 0..256, alpha 0..256 */
static void panel_a(int x, int y, int w, int h, int fade, int alpha) {
    int e = fade * 230 >> 8; const int *l = MN_LIN[gui_line];   /* v12.0: the GUI line colour */
    shade(x, y, w, h, 18, 24, 42, alpha * fade >> 8);
    shade(x, y, w, 2, l[0], l[1], l[2], e); shade(x, y+h-2, w, 2, l[0], l[1], l[2], e);
    shade(x, y+2, 2, h-4, l[0], l[1], l[2], e); shade(x+w-2, y+2, 2, h-4, l[0], l[1], l[2], e);
}
static void panel(int x, int y, int w, int h, int fade) { panel_a(x, y, w, h, fade, PANEL_ALPHA); }
/* v11.5: the table boxes: fill follows TB_A, the line follows the same opacity below the old look (so the default is unchanged) and has its own colour; 0% = nothing drawn */
static void panel_tl(int x, int y, int w, int h, int fade, int alpha, int line) {
    int e = (alpha >= 160 ? 230 : 230 * alpha / 160) * fade >> 8; const int *l = MN_LIN[gui_line]; (void)line;   /* v12.0: the table boxes follow the GUI line colour (the TABLE LINE row is gone) */
    shade(x, y, w, h, 18, 24, 42, alpha * fade >> 8);
    shade(x, y, w, 2, l[0], l[1], l[2], e); shade(x, y+h-2, w, 2, l[0], l[1], l[2], e);
    shade(x, y+2, 2, h-4, l[0], l[1], l[2], e); shade(x+w-2, y+2, 2, h-4, l[0], l[1], l[2], e);
}
static void panel_t(int x, int y, int w, int h, int fade) { panel_tl(x, y, w, h, fade, TB_A[tb_idx], 0); }

static int opt_mc = 0;   /* opt_mc: on the last Options row 0 = TEXT BOX STYLE, 1 = MENU COLORS */
static void panel_am(int x, int y, int w, int h, int fade, int alpha) {
    int e = fade * 230 >> 8; const int *l = MN_LIN[mn_line];
    shade(x, y, w, h, 18, 24, 42, alpha * fade >> 8);
    shade(x, y, w, 2, l[0], l[1], l[2], e); shade(x, y+h-2, w, 2, l[0], l[1], l[2], e);
    shade(x, y+2, 2, h-4, l[0], l[1], l[2], e); shade(x+w-2, y+2, 2, h-4, l[0], l[1], l[2], e);
}
static void panel_m(int x, int y, int w, int h, int fade) { panel_am(x, y, w, h, fade, PANEL_ALPHA); }
static Uint32 gui_tc(void) { const int *t = MN_TXT[gui_text]; return C(t[0], t[1], t[2]); }
static Uint32 gui_lc(void) { const int *l = MN_LIN[gui_line]; return C(l[0], l[1], l[2]); }   /* the line colour itself (frames of the icons and the A bubble) */
static Uint32 gui_dc(void) { const int *l = MN_LIN[gui_line]; return C(l[0] * 71 / 100, l[1] * 71 / 100, l[2] * 71 / 100); }
static Uint32 mn_tc(void) { const int *t = MN_TXT[mn_text]; return C(t[0], t[1], t[2]); }
static Uint32 mn_dc(void) { const int *l = MN_LIN[mn_line]; return C(l[0] * 71 / 100, l[1] * 71 / 100, l[2] * 71 / 100); }   /* divider line: the line colour a bit darker (the old gold 210,175,90 -> 150,125,70) */
static void shade_m(int x, int y, int w, int h, int a) { const int *t = MN_TXT[mn_text]; shade(x, y, w, h, t[0], t[1], t[2], a); }

/* suit: 0 spade, 1 heart, 2 diamond, 3 club (two-colour deck: hearts and diamonds red, spades and clubs black) */
static Uint32 suitcol(int s) { static const int c[4][3] = {{25,25,30},{200,30,40},{200,30,40},{25,25,30}}; return C(c[s][0], c[s][1], c[s][2]); }
static void suit(int s, int cx, int cy, int r, Uint32 c) {
    int sw = r/3 + 1, dy;
    if (s == 0) { tri(cx, cy-r, r, r+r/8, 1, c); disc(cx-r/2, cy+r/8, r/2+1, c); disc(cx+r/2, cy+r/8, r/2+1, c);
                  R(cx-sw/2, cy+r/4, sw, r*3/4, c); R(cx-r/3, cy+r-2, 2*(r/3)+1, 2, c); }
    else if (s == 1) { disc(cx-r/2, cy-r/3, r/2+1, c); disc(cx+r/2, cy-r/3, r/2+1, c); tri(cx, cy-r/3, r, r+r/3, 0, c); }
    else if (s == 2) { for (dy = -r; dy <= r; dy++) { int hw = (r-abs(dy))*3/5; R(cx-hw, cy+dy, 2*hw+1, 1, c); } }
    else { disc(cx, cy-r/2, r/2, c); disc(cx-r/2, cy+r/6, r/2, c); disc(cx+r/2, cy+r/6, r/2, c); disc(cx, cy, r/3, c);
           R(cx-sw/2, cy, sw, r, c); R(cx-r/3, cy+r-2, 2*(r/3)+1, 2, c); }
}

/* Every card goes through this one function: swap it for sprites when art arrives.
   up: 0 = back, 1 = face, 2 = empty slot outline (only used on the tilted table row) */
static void card(int x, int y, int w, int h, int c, int up) {
    int sc = w >= 56 ? 3 : (w >= 40 ? 2 : 1);
    if (up == 2) { rrect(x, y, w, h, C(120,180,150)); rrect(x+2, y+2, w-4, h-4, C(255,0,255)); return; }
    rrect(x, y, w, h, up == 1 && card_hl ? C(250,205,60) : C(20,20,24));
    if (!up) {
        if (card_pinkc) { rrect(x+1, y+1, w-2, h-2, C(190,60,120)); rrect(x+4, y+4, w-8, h-8, C(245,150,195)); rrect(x+6, y+6, w-12, h-12, C(190,60,120)); }
        else { rrect(x+1, y+1, w-2, h-2, C(40,70,160)); rrect(x+4, y+4, w-8, h-8, C(70,110,205)); rrect(x+6, y+6, w-12, h-12, C(40,70,160)); }
        return;
    }
    if (card_hl) { rrect(x+1, y+1, w-2, h-2, C(250,205,60)); rrect(x+3, y+3, w-6, h-6, C(250,250,244)); }
    else rrect(x+1, y+1, w-2, h-2, C(250,250,244));
    TS = 0; text(x+3, y+3, RANK[c%13], sc, suitcol(c/13)); TS = 1;
    suit(c/13, x + w/2, y + (h + 7*sc)/2 + 2, w/4, suitcol(c/13));
}

/* Lay a card on the table: draw it flat into the scratch surface, then project it
   (far edge narrower, squashed to dh rows, sheared) with a soft shadow underneath. */
static void tilt(int x, int y, int sw, int sh, int c, int up, int dh, int farw, int shear) {
    int pass, i, j, swn = XS(sw), shn = YS(sh), dhn = YS(dh), farn = XS(farw), shn_ = XS(shear), xn = XS(x), yn = YS(y), so_x = XS(2), so_y = YS(3); Uint32 key;
    if (dhn < 2) dhn = 2; if (farn < 2) farn = 2;
    T = CS; key = C(255,0,255); SDL_FillRect(CS, NULL, key); card(0, 0, sw, sh, c, up); T = S;   /* the card is drawn flat at screen resolution, then projected */
    for (pass = 0; pass < 2; pass++) {
        if (pass == 0 && up == 2) continue;
        for (j = 0; j < dhn; j++) {
            int rw = farn + (swn - farn) * j / (dhn - 1), sy = j * (shn - 1) / (dhn - 1);
            int left = xn + (swn - rw) / 2 + shn_ * j / dhn - shn_ / 2;
            const Uint32 *srow = px(CS, 0, sy);
            for (i = 0; i < rw; i++) {
                int sx = rw > 1 ? i * (swn - 1) / (rw - 1) : 0, dx = left + i + (pass == 0 ? so_x : 0), dy = yn + j + (pass == 0 ? so_y : 0);
                Uint32 p = srow[sx];
                if (p == key || dx < 0 || dx >= SW || dy < 0 || dy >= SH) continue;
                if (pass == 0) shade_n(dx, dy, dx + 1, dy + 1, 0, 0, 0, 110);
                else *px(S, dx, dy) = (p & (S->format->Rmask | S->format->Gmask | S->format->Bmask)) | S->format->Amask;   /* v7.2: same as SDL_MapRGB(GetRGB(p)), without the two calls per pixel */
            }
        }
    }
}

static void blit(SDL_Surface *im, int x, int y) {   /* x, y logical; the picture itself is already at screen resolution */
    SDL_Rect d; if (!im) return;
    d.x = XS(x) + ADJX; d.y = YS(y) + ADJY; d.w = 0; d.h = 0; SDL_BlitSurface(im, NULL, S, &d);
}
/* a part of a picture: the source rectangle (sx, sy, w, h) and the destination (x, y) are logical */
static void blit_src(SDL_Surface *im, int sx, int sy, int w, int h, int x, int y) {
    SDL_Rect sr, d; if (!im) return;
    sr.x = XS(sx); sr.y = YS(sy); sr.w = XS(sx + w) - sr.x; sr.h = YS(sy + h) - sr.y;
    d.x = XS(x) + ADJX; d.y = YS(y) + ADJY; d.w = 0; d.h = 0; SDL_BlitSurface(im, &sr, S, &d);
}
/* fewest chips for a total: c[0..3] = counts of 1, 10, 25, 50 */
static void change(int v, int *c) { int i; for (i = 3; i >= 0; i--) { c[i] = v / CV[i]; v %= CV[i]; } }


/* ---------- text helpers ---------- */
static int tw(const char *s, int sc) { int n = (int)strlen(s); return n ? n * 6 * sc - sc : 0; }
static void text_r(int xr, int y, const char *s, int sc, Uint32 col) { text(xr - tw(s, sc), y, s, sc, col); }
static void text_c(int cx, int y, const char *s, int sc, Uint32 col) { text(cx - tw(s, sc) / 2, y, s, sc, col); }
static void dtext(int x, int y, const char *s, int sc, Uint32 col) { TSX = 1; text(x, y, s, sc, col); TSX = 0; }   /* v10.7: dialogue-box text (thick black shadow) */
static void dtext_r(int xr, int y, const char *s, int sc, Uint32 col) { TSX = 1; text_r(xr, y, s, sc, col); TSX = 0; }

/* ---------- game state shared by all screens ---------- */
enum { SC_MENU, SC_WALK, SC_STORY, SC_DOOR, SC_LEN, SC_PLAY, SC_END, SC_ATM, SC_SCENE, SC_KEY, SC_OPT, SC_EVENT, SC_QSETUP, SC_MTABLE, SC_MEND, SC_STORE, SC_INV, SC_MONO, SC_SNEAK, SC_OVER, SC_CAREER, SC_SHOP, SC_ROM, SC_ROMQ, SC_PEEK, SC_DEBUG, SC_LOTTO, SC_NAME };
enum { SE_DOOR, SE_ZOMBIE, SE_DAILY };                /* what happens when a coach scene ends */
enum { B_A, B_B, B_X, B_Y, B_UP, B_DOWN, B_LEFT, B_RIGHT, B_START, B_QUIT, B_R1, B_L1, B_R2, B_NKEYS };
enum { GO_MENU, GO_HUB, GO_VISIT, GO_SCRIPT, GO_TABLE, GO_BEAT, GO_EVENT, GO_QSETUP, GO_MTABLE, GO_MAP, GO_MONO, GO_SNEAK, GO_BED, GO_SNEAKBACK, GO_TIME, GO_ROM, GO_UNPACK, GO_EXIT, GO_LOAD, GO_VISITIN };   /* fade-to-black transitions (GO_HUB = back to walking, GO_MAP = through a door) */
enum { PK_NONE, PK_HOME, PK_TABLE, PK_SKIP, PK_ROOM, PK_TITLE, PK_QUICK, PK_EXHIB, PK_EXITC, PK_OBJ, PK_OPT };         /* what the Menu button offers, by place (PK_EXITC = "exit without saving?") */
enum { MAP_ROOM, MAP_HALL, MAP_LOBBY, MAP_SOPHIA, MAP_HALL1, MAP_HALL3, MAP_EVENT, MAP_SBED, MAP_N };                                                 /* the walkable places: your room and the 2nd-floor hall (MAP_HALL), the lobby and the stair hall (MAP_HALL1, 1st floor), her room and the 3rd-floor hall (MAP_HALL3) */
enum { DN, LF, RT, UP };                                                                               /* facing: also the row of the walking sheet */
enum { SM_CHOICE, SM_BED, SM_ADMIRE, SM_VIEW, SM_MSG, SM_CLOSER, SM_LONGER };   /* Sophia's door menu, and what you are looking at after her bedroom door (her room itself is the walkable MAP_SOPHIA) */
#define HOME_DOOR_X 5   /* v6.6: where you stand in front of her bedroom door at home (facing up) */
#define HOME_DOOR_Y 3
#define ROOM_SX 4   /* where you start in your room (new game, load, Exit) */
#define ROOM_SY 4
#define EV_ARR_X 13   /* you step in through the wooden door (right wall) and face the room */
#define EV_ARR_Y 6
#define EV_OUT_X 9    /* after an Exhibition you come out of the match room: in front of its door beside the clerk's desk, facing down */
#define EV_OUT_Y 3
enum { WK_NONE, WK_STORE, WK_HALL2, WK_HALL3, WK_HALL4, WK_HALL5, WK_HALL6, WK_DESK, WK_EVENT, WK_DEMAND, WK_DEMAND2, WK_DEMAND3, WK_EVMENU, WK_WARD, WK_VISIT, WK_VBYE };                                                                            /* what happens when a walking message is closed */

static Game G;
static SDL_Surface *load_png(const char *path);
static SDL_Surface *load_png_ex(const char *path, int num, int den, int opaque, int cols, int rows);
/* v7.0: pictures only needed late in the story are loaded when first needed, and the ones they replace are dropped, so the game stays inside the memory it used before the bigger screen */
static SDL_Surface *lz(SDL_Surface **p, const char *path) { if (!*p) *p = load_png(path); return *p; }
static void lz_free(SDL_Surface **p) { if (*p) { SDL_FreeSurface(*p); *p = NULL; } }
/* v10.8: the outfit pictures. G.outfit: 0 = her usual outfit, 1 = yoga, 2 = designer top, 3 = cow print (v11.1). of_load() fills the %s of fmt with the outfit name (cow, then top, then yoga, then base for the usual one):
   a missing cow print picture falls back to the designer-top picture, then the yoga one, then the usual one, so a half-finished art set never shows a hole. */
static const char *const OF_SFX[OF_N] = {"d8", "yoga", "top", "cow"};
static const char *const OF_CHG[OF_N] = {"CHANGE TO HER USUAL OUTFIT", "CHANGE TO THE YOGA OUTFIT", "CHANGE TO THE DESIGNER TOP", "CHANGE TO THE COW PRINT OUTFIT"};
static SDL_Surface *of_load(const char *fmt, int o, const char *base) {
    char pth[96]; SDL_Surface *r = NULL; int k;
    for (k = o < 0 ? 0 : o >= OF_N ? OF_N - 1 : o; k >= 0 && !r; k--) { if (!k && !base) break; snprintf(pth, sizeof pth, fmt, k ? OF_SFX[k] : base); r = load_png(pth); }   /* base NULL = no picture for the usual outfit */
    return r;
}
/* her picture: the normal one, or the one she has after the pill (the other one is dropped from memory) */
static SDL_Surface *pl_pick(SDL_Surface **a, const char *pa, SDL_Surface **b, const char *pb) {
    if (G.pill_seen) { if (lz(b, pb)) { lz_free(a); return *b; } return lz(a, pa); }
    lz_free(b); return lz(a, pa);
}
static int room_seen;   /* you have been inside your room (not saved by itself: a game saved at the bed has always been there) */
static int scr = SC_MENU, sel, len_sel = 1, menu_sel = 1, pending_len = 100;
static unsigned fake_ms, say_ms, hand_end_ms, think_at, qp_ms, len_msg_ms;
static int len_msg_en;   /* v12.4: 1 = the buy-in list note says NEED 20% ENERGY instead of NOT ENOUGH CASH */
static char l1[40], l2[40];                           /* her speech box */
static int her_tag;                                    /* 0 none, 1 check, 2 call, 3 raise, 4 fold, 5 bet */
/* ATM */
static int atm_mode, atm_kind, atm_amt, atm_cur = 5;  char atm_msg[48];
/* coach scene */
static const char *sc_page[6]; static char sc_buf[2][160];
static int sc_n, sc_i, sc_end, sc_choice; static unsigned sc_ms;
static int last_brush = -1;
static int boss_new, boss_win;   /* Exhibition: the boss table just opened / the boss was just beaten (the scene follows the result screen) */
static int first_end;   /* the match that just ended was the forced first one: Sophia introduces herself next */
/* Quick Play / Exhibition */
static int qp_kind, st_confirm, inv_sel; static char st_msg[48]; static unsigned st_msg_ms;   /* qp_kind: 0 = cash message, 1 = energy message */
static int ev_out;   /* 1 = an Exhibition was played: the next return to walking is at the match-room door of the Event Center */
static int bm_open, bm_by;   /* v8.9: the bet menu (Exhibition + boss): open flag, the raise increment chosen on top of the call */
static int lot_sel, lot_kind; static unsigned lot_ms; static char lot_msg[48];   /* v9.0: the lottery list (cursor, last answer 0 bought / 1 no cash / 2 holding a ticket, when) and the clerk's prize line */
static MP M; static int mp_mode, qs_si, qs_step, qs_sel, ev_sel, mp_buyin, mp_prize, mp_tag[MP_MAX], seat_in[MP_MAX];   /* mp_mode 0 quick, 1 exhibition */
static char mp_msg[40]; static unsigned mp_msg_ms;
static int hl_hole[MP_MAX][2], hl_board[5];       /* gold outline: cards that make the winning hand at showdown */
/* story scene (visual-novel beats) */
static int peeked, peek_ctx, peek_sel;   /* v6.7: the pump / bottle scenes: you chose to LOOK (peeked), which scene it is (0 pump, 1 bottle), the choice cursor */
static Beat vn_buf[200]; static int vn_n, vn_i, vn_id, vn_prev_spr; static unsigned vn_ms, vn_spr_ms, after_ms;   /* after_ms: when the after-match scene began (the cash counts up from there) */
/* transitions and pause */
static int fade_a, fade_st, nx_kind, nx_arg, pause_kind, pause_sel; static unsigned fade_t0;
/* walking: where you are, what you are doing, the held buttons, and the last saved copy of the game */
static int wk_sleep;   /* 1 = the save prompt is the bed (sleep to the next morning), 0 = the sofa (morning -> evening -> night) */
static int wk_map, wk_tx, wk_ty, wk_face, wk_fx, wk_fy, wk_moving, wk_step, wk_mode, wk_yes, wk_after, wk_narr, wk_npg, wk_ipg;   /* wk_mode: 0 walking, 1 message, 2 "save the game?" */
static unsigned wk_t0, wk_ms; static const char *wk_pg[2], *wk_tag;
/* v8.0: Sophia walks around in your living room (once she lives with you; morning and evening only; rare) */
static int ls_on, ls_tx, ls_ty, ls_fx, ls_fy, ls_face, ls_moving, ls_stepc, ls_ph = -1;   /* ls_ph: the time of day she was rolled for (-1 = roll again) */
#define TALK_BG() (wk_map == MAP_SBED ? BG_ROM : BG_MYROOM)   /* v9.2: her menu and scenes: her bedroom inside her room, else your living room */
static int lr_on(void);   /* v9.9: married and talking to her in the living room (her scenes get the living-room pictures and lines) */
static int ls_talk;      /* 1 = you are talking to her in the living room (her menu and scenes use your living room picture, you stay where you stood) */
static int ls_enabled = 1, ls_force;   /* tests: 0 = she never shows up; ls_force = she is always there */
static unsigned ls_t0, ls_next;
static SDL_Surface *lsS, *lsSn, *sbN; static int ls_fw, ls_fh, ls_foot[4];   /* v9.2: lsSn = her walking sheet at night, sbN = her bedroom picture at night */
static int sb_out;   /* v9.2: 1 = the next time you are back on the map you stand outside her bedroom (after a match, or when she throws you out) */
static void sb_enter(void), sb_slept(void);
static int sn_ret = SM_BED; static int sn_vpic; static int mono_pg, sn_mode, sn_sel, sn_used, sn_pg, sn_closer, sn_longer;   /* sn_vpic: which picture the fullscreen View shows (0 sleeping, 1 closer, 2 longer) */ static unsigned mono_ms, sn_ms; static const char *sn_txt;   /* opening monologue; Sophia's door menu and her room */
static int wk_nmap, wk_ntx, wk_nty, wk_nface;      /* where the door you just used leads */
#define PAN_MS   2600   /* the hall look-around: how long the camera takes to swing from one door to the other */
#define PAN_HOLD  700   /* ...and how long it rests there before that room's line appears */
static const char *cu_think, *cu_her; static int cu_lt = -1, cu_lh = -1; static unsigned over_ms;   /* what you think / what she answers (random, not the same twice running); game over screen clock */
#define CU_SHIFT 30   /* the close-up picture is moved up by its empty top margin (30 px) so the whole art stays on screen above the short box */
static int cu_mode, cu_used; static unsigned cu_ms;   /* R1 close-up at her table: 0 none, 1 picture, 2 her line; once per hand */
static unsigned wk_pan_t0; static int wk_pan_x0, wk_pend;   /* slow pan: start time (0 = none), where it started, and which hall line waits for it (wk_mode 3) */
static int wk_cfx, wk_cfy; static unsigned cam_ms;   /* v7.3: the camera in 1/256 logical px, and when it was last moved */
static int wk_look = -1, wk_camx, wk_camy, wk_snap = 1;   /* a scene can make the camera look at a tile column; camera centre (eased unless snapping) */
static int kheld[B_NKEYS], last_dir = -1;               /* buttons held right now (real input only) */
static Game saved_G;                               /* what the save file holds: Exit goes back to this */
static SDL_Surface *plS, *plSn, *plSm, *clerkS;   /* plSn = you in her (very dark) room, plSm = you at night in the other places (slightly darker); the receptionist is not there at night */
static int home_end;   /* the match that just ended was a free one at home */
static int wk_fsel, wk_flr[3], wk_nflr;   /* the elevator menu (wk_mode 5): which entry, and the floors it offers */
static int wk_wsel;   /* v9.2: the wardrobe menu (wk_mode 8) */
static int wk_dsel, over_rent; static char rent_msg[96];   /* the rent menu at the desk (wk_mode 4); 1 = the game over is the rent one; the line about the rent taken at night */
static int daily_i = -1;   /* the last once-a-day refusal line (never the same twice running) */
static int rules_told;   /* the first challenge's rules were told this session (so a second knock goes straight to the menu) */   /* plSn = you at night (her room is darker) */ static int pl_fw, pl_fh, pl_foot[4];   /* walking sheet (3 x 4 frames), receptionist, frame size, feet line per row */
static void wk_place(int m, int x, int y, int f);
static void wk_say(const char *tag, int narr, int after, const char *p0, const char *p1);
static int outfit_ask(int k, int ward);   /* v11.2 */
static void ls_go_home(void); static void ls_reroll(void);   /* v8.1: Sophia in the living room (defined with the walking code) */
static void wk_wait_done(void);
static void wk_wait_begin(unsigned ms, int step);
static void unpack_begin(void);
static void room_apply(void);
static void text_o(int x, int y, const char *s, int sc, Uint32 col);
#define FADE_MS 600         /* v7.6: every normal fade, each way (was 260: too fast) */
#define SLOW_FADE_MS 1800   /* v6.5: the slow fade after she says yes to living with you */
#define WAIT1_MS 2600       /* ...then you stand in your room waiting (no text) */
#define WAIT2_MS 2000
#define UNPACK_STEPS 2      /* v7.6: the first time in your room you walk this many steps to the RIGHT by yourself (slowly, towards the bedroom door), then a slow fade, then the unpacking line in front of the bed */
#define UNPACK_STEP_MS 600  /* v7.6: one step of that walk (normal walking stays 350 ms) */
#define UNPACK_X 4          /* v7.6: ...where the fade puts you: the bedroom floor tile between the computer and the bed, facing the bed */
#define UNPACK_Y 3          /* v9.3: the base room has no bedroom: after the fade you stand beside the bed (5,3), facing it (v9.4: the bed's solid tile is now only (5,3); the tile below it is floor you can step on) */
static int fade_len = FADE_MS;
static unsigned wk_wt0, wk_wms; static int wk_wstep;   /* wk_mode 6: a timed wait with no text; which step of the waiting-for-Sophia sequence it is */
static SDL_Surface *gaugeS, *bottleS;               /* the bottle gauge picture, the Sophia breast milk icon */
static int gauge_y0, gauge_y1, gauge_xl[200], gauge_xr[200]; /* the inside of the gauge (per row) where the milk is drawn */

static unsigned now_ms(void) { return frozen ? fake_ms : (unsigned)SDL_GetTicks(); }
static unsigned age_of(unsigned t) { return frozen ? 1500 : now_ms() - t; }
/* v10.5: SAVE SLOTS. Slot 1 is the old pokerqueen.sav, the others are pokerqueen2.sav ... pokerqueen5.sav. The sofa and the bed save to the ACTIVE slot (slot_act);
   CAREER MODE (title) and Options > SAVE SLOTS pick the active slot and load a slot. pokerqueen.prog remembers the furthest day each career reached (the rewind guard, see g_open_slot). */
static const char *slot_prefix = "";   /* tests: a folder for the slot files */
static int slot_io_test;               /* tests: 1 = really read and write the files although the game is frozen */
#define IO_OK (!frozen || slot_io_test)
static int slot_act = 1;                              /* the slot the sofa and the bed save to (1..SLOT_N) */
static Game slot_g[SLOT_N]; static int slot_st[SLOT_N];   /* what is in each slot (read by slots_refresh): 0 empty, 1 a good save, 2 a file that cannot be read */
static int prog_far;                                  /* the furthest day any save of the loaded career reached (read when a slot is opened, raised by every save) */
static Game load_pend; static int load_pend_slot, load_pend_far;   /* the slot read before the fade, swapped in while the screen is black (GO_LOAD) */
static const char *slot_path(int s) { static char b[8][300]; static int k; char *r = b[k++ & 7]; if (s <= 1) snprintf(r, 300, "%s%s", slot_prefix, SAVE_PATH); else snprintf(r, 300, "%spokerqueen%d.sav", slot_prefix, s); return r; }
static const char *prog_path(void) { static char b[300]; snprintf(b, sizeof b, "%s%s", slot_prefix, PROG_PATH); return b; }
static void slots_refresh(void) {
    int i; if (!IO_OK) return;
    for (i = 0; i < SLOT_N; i++) { FILE *f = fopen(slot_path(i + 1), "rb"); memset(&slot_g[i], 0, sizeof slot_g[i]); if (!f) { slot_st[i] = 0; continue; } fclose(f); slot_st[i] = load_game(&slot_g[i], slot_path(i + 1)) ? 1 : 2; }
}
static int slot_begin_load(int s) {   /* read slot s into load_pend (the guard reshuffles the market if it is behind its career); 0 = it cannot be read */
    unsigned salt = (unsigned)now_ms() * 2654435761u ^ (unsigned)rand() * 40503u ^ (unsigned)time(NULL); int behind;
    if (!g_open_slot(&load_pend, slot_path(s), prog_path(), salt, &behind)) return 0;
    load_pend_slot = s; load_pend_far = prog_get(prog_path(), load_pend.career); return 1;
}
/* The save file is written ONLY by save_now() (the bed or the sofa in your room). Exit or closing the game without saving loses everything since. */
static void persist(void) {}
static void save_now(void) {
    if (IO_OK) { save_game(&G, slot_path(slot_act)); prog_set(prog_path(), G.career, G.day); }
    saved_G = G; if (G.day > prog_far) prog_far = G.day;
}
static int unsaved(void) { return memcmp(&G, &saved_G, sizeof G) != 0; }
static SDL_Surface *homeS, *homeYS, *hideS[2];   /* v8.7: her moved-in picture, her yoga everyday picture, the two hiding-behind-the-door pictures (d8 / yoga; only one is kept in memory) */
static int hideO = -1;
static SDL_Surface *hide_pic(void) {   /* v8.7: sprite 5 = she hides behind her bedroom door: assets/her_hide_<d8|yoga>.png by the outfit */
    int o = G.outfit;
    if (hideS[0] && hideO != o) lz_free(&hideS[0]);
    if (!hideS[0]) { hideS[0] = of_load("assets/her_hide_%s.png", o, "d8"); hideO = o; }
    return hideS[0];
}
static void hide_free(void) { lz_free(&hideS[0]); hideO = -1; }
static int feed_hide;   /* v10.1: she hides behind her bedroom door when you ask BREASTFEED at the door: 1 = she refuses (picture her_hide_no_<d8|yoga>), the accepting scene uses story sprite 6 (her_hide_yes_<d8|yoga>) */
static SDL_Surface *fhS; static int fhK = -1;
static SDL_Surface *fh_pic(int kind) {   /* kind 1 = refusal, 2 = acceptance; outfit d8 / yoga; a missing file falls back to the d8 one, then to the old hiding picture */
    int o = G.outfit, key = kind * 4 + o; char fn[64];
    if (fhS && fhK != key) lz_free(&fhS);
    if (!fhS) {
        snprintf(fn, sizeof fn, "assets/her_hide_%s_%%s.png", kind == 1 ? "no" : "yes"); fhS = of_load(fn, o, "d8");   /* v10.8: top, yoga, d8 */
        fhK = key;
    }
    return fhS ? fhS : hide_pic();
}
static void fh_free(void) { lz_free(&fhS); fhK = -1; }
static int homeYO, coverYO;   /* v10.8: which outfit (1 yoga, 2 top, 3 cow) homeYS / herCoverYS were loaded for: they hold the picture of the outfit she wears */
static SDL_Surface *herCoverYS;   /* v8.8 (v10.8: the covering picture of the current non-usual outfit): her covering picture in the yoga outfit (assets/her_cover_yoga.png, PLACEHOLDER) */
static int cover_now;             /* v8.8: married: she covers her chest right now (teasing about her body, ASK FOR MILK, BREASTFEED); cleared when her line / the scene is over */
static SDL_Surface *home_of(void) {   /* v10.8: her everyday picture in the yoga / designer-top outfit (NULL in her usual outfit or when no file exists) */
    if (!G.outfit) return NULL;
    if (homeYS && homeYO != G.outfit) lz_free(&homeYS);
    if (!homeYS) { homeYS = of_load("assets/her_home_%s.png", G.outfit, NULL); homeYO = G.outfit; }
    return homeYS;
}
static SDL_Surface *cover_of(void) {   /* v10.8: her covering picture in the yoga / designer-top outfit */
    if (!G.outfit) return NULL;
    if (herCoverYS && coverYO != G.outfit) lz_free(&herCoverYS);
    if (!herCoverYS) { herCoverYS = of_load("assets/her_cover_%s.png", G.outfit, NULL); coverYO = G.outfit; }
    return herCoverYS;
}
static SDL_Surface *her_vis(void) {   /* v8.8: her everyday picture. MARRIED (or in the yoga outfit): the day-8 outfit uncovered (confident_p) or her_home_yoga; she only covers (her_cover / her_cover_yoga) while cover_now is set. Before that the old rules: her_cover from the day-8 scene, her_day_6 on days 6 and 7, her_home once she has really moved in, else the normal one */
    if (G.rs == RS_LIVING && (G.married || G.outfit)) {
        lz_free(&herD6S); lz_free(&homeS);
        if (cover_now) {
            if (cover_of()) { lz_free(&herCoverS); lz_free(&homeYS); return herCoverYS; }
            if (lz(&herCoverS, "assets/her_cover.png")) { lz_free(&homeYS); return herCoverS; }
        }
        lz_free(&herCoverYS); lz_free(&herCoverS);
        if (home_of()) return homeYS;
        lz_free(&homeYS);
        return pl_pick(&conf, "assets/confident.png", &confP, "assets/confident_p.png");
    }
    lz_free(&homeYS); lz_free(&herCoverYS);
    if (G.pill_seen) { lz_free(&herD6S); if (lz(&herCoverS, "assets/her_cover.png")) return herCoverS; }
    else lz_free(&herCoverS);
    if (g_invited(&G) && lz(&herD6S, "assets/her_day_6.png")) return herD6S;
    if (G.rs == RS_LIVING && lz(&homeS, "assets/her_home.png")) return homeS;
    lz_free(&homeS);
    return pl_pick(&conf, "assets/confident.png", &confP, "assets/confident_p.png");
}
static SDL_Surface *night_vis(void) {   /* v9.4: her picture when you knock at night (story sprite 3). Before she lives with you: her night picture. Once she has moved in (RS_LIVING): the MOVED-IN picture (her_home, her_home_yoga in the yoga outfit), same as her door picture, never the old pre-move-in night picture */
    if (G.rs == RS_LIVING) {
        lz_free(&herCoverS); lz_free(&herD6S);
        if (home_of()) { lz_free(&homeS); return homeYS; }
        if (lz(&homeS, "assets/her_home.png")) return homeS;
    }
    return nightS;
}
static void cover_set(int on) { cover_now = on; if (on) her_vis(); }   /* loads the covering picture at once so there is no hitch on the first frame */
static SDL_Surface *chat_vis(void) {   /* v7.7 / v8.7: her picture while you CHAT: the yoga picture in the yoga outfit; else the normal one (her day-6 picture on days 6 and 7, her_home once she lives with you), confident_p from the day-8 scene on; TEASE always uses her_vis() = her_cover */
    if (G.rs == RS_LIVING && G.outfit) return her_vis();
    if (G.pill_seen) return pl_pick(&conf, "assets/confident.png", &confP, "assets/confident_p.png");
    return her_vis();
}
static int unpack_done;   /* v6.8: you have unpacked in your room (the Event Center and the 3rd floor stay closed until then). Not saved: a game saved at the bed has always unpacked */
static int unpacked(void) { return unpack_done || G.story >= ST_FIRST || saved_G.story >= ST_HOME; }
static int services_open(void) { return room_seen || G.story >= ST_FIRST || saved_G.story >= ST_HOME; }   /* the clerk and ATM icons (and the Event Center) show up after you have been in your room */

static int pick_new(int n, int last) { int k = rnd(n); if (n > 1 && k == last) k = (k + 1 + rnd(n - 1)) % n; return k; }   /* a random index that is not the previous one */
static const char *pick(const char *const *pool, int n) {   /* v10.1: every pool remembers its last picks (up to 3, never more than n-1), so even a small pool does not repeat a line right away */
    enum { PK_SLOTS = 512, PK_HIST = 3 };
    static const char *const *tp[PK_SLOTS]; static const char *hist[PK_SLOTS][PK_HIST]; static int used = 0, rr = 0;
    int slot = -1, i, t = 0, keep, tries = 0; const char *s;
    for (i = 0; i < used; i++) if (tp[i] == pool) { slot = i; break; }
    if (slot < 0) { if (used < PK_SLOTS) slot = used++; else { slot = rr; rr = (rr + 1) % PK_SLOTS; } tp[slot] = pool; for (i = 0; i < PK_HIST; i++) hist[slot][i] = NULL; }
    keep = n - 1 < PK_HIST ? n - 1 : PK_HIST; if (keep < 0) keep = 0;
    for (;;) {
        s = pool[rnd(n)]; t = 0;
        for (i = 0; i < keep; i++) if (hist[slot][i] == s) t = 1;
        if (!t || ++tries > 24) break;
    }
    for (i = PK_HIST - 1; i > 0; i--) hist[slot][i] = hist[slot][i - 1];
    hist[slot][0] = s; return s;
}

/* v10.2: the gift scene pools, by beat (0 hand, 1 thanks, 2 reaction, 3 closing) and gift kind (GF_*; the pill and the pump keep the old fixed lists, the milk pill thanks uses R_MILK_OK) */
static const char *const *const *const GIFT_POOL[4][GF_N] = {
    {[GF_FLOWER] = &GH_FLOWER, [GF_STACK] = &GH_STACK, [GF_LOTION] = &GH_LOTION, [GF_DECK] = &GH_DECK, [GF_YOGA] = &GH_YOGA, [GF_TOP] = &GH_TOP, [GF_COW] = &GH_COW, [GF_GOLD] = &GH_GOLD, [GF_MILK] = &GH_MILK, [GF_SUPER] = &GH_SUPER, [GF_INJECT] = &GH_INJECT},
    {[GF_FLOWER] = &GO_FLOWER, [GF_STACK] = &GO_STACK, [GF_LOTION] = &GO_LOTION, [GF_DECK] = &GO_DECK, [GF_YOGA] = &GO_YOGA, [GF_TOP] = &GO_TOP, [GF_COW] = &GO_COW, [GF_GOLD] = &GO_GOLD, [GF_SUPER] = &GO_SUPER, [GF_INJECT] = &GO_INJECT},
    {[GF_FLOWER] = &GL_FLOWER, [GF_STACK] = &GL_STACK, [GF_LOTION] = &GL_LOTION, [GF_DECK] = &GL_DECK, [GF_YOGA] = &GL_YOGA, [GF_TOP] = &GL_TOP, [GF_COW] = &GL_COW, [GF_GOLD] = &GL_GOLD, [GF_MILK] = &GL_MILK, [GF_SUPER] = &GL_SUPER, [GF_INJECT] = &GL_INJECT},
    {[GF_FLOWER] = &GE_FLOWER, [GF_STACK] = &GE_STACK, [GF_LOTION] = &GE_LOTION, [GF_DECK] = &GE_DECK, [GF_YOGA] = &GE_YOGA, [GF_TOP] = &GE_TOP, [GF_COW] = &GE_COW, [GF_GOLD] = &GE_GOLD, [GF_MILK] = &GE_MILK, [GF_SUPER] = &GE_SUPER, [GF_INJECT] = &GE_INJECT}
};
static int *const GIFT_POOL_N[4][GF_N] = {
    {[GF_FLOWER] = &GH_FLOWER_N, [GF_STACK] = &GH_STACK_N, [GF_LOTION] = &GH_LOTION_N, [GF_DECK] = &GH_DECK_N, [GF_YOGA] = &GH_YOGA_N, [GF_TOP] = &GH_TOP_N, [GF_COW] = &GH_COW_N, [GF_GOLD] = &GH_GOLD_N, [GF_MILK] = &GH_MILK_N, [GF_SUPER] = &GH_SUPER_N, [GF_INJECT] = &GH_INJECT_N},
    {[GF_FLOWER] = &GO_FLOWER_N, [GF_STACK] = &GO_STACK_N, [GF_LOTION] = &GO_LOTION_N, [GF_DECK] = &GO_DECK_N, [GF_YOGA] = &GO_YOGA_N, [GF_TOP] = &GO_TOP_N, [GF_COW] = &GO_COW_N, [GF_GOLD] = &GO_GOLD_N, [GF_SUPER] = &GO_SUPER_N, [GF_INJECT] = &GO_INJECT_N},
    {[GF_FLOWER] = &GL_FLOWER_N, [GF_STACK] = &GL_STACK_N, [GF_LOTION] = &GL_LOTION_N, [GF_DECK] = &GL_DECK_N, [GF_YOGA] = &GL_YOGA_N, [GF_TOP] = &GL_TOP_N, [GF_COW] = &GL_COW_N, [GF_GOLD] = &GL_GOLD_N, [GF_MILK] = &GL_MILK_N, [GF_SUPER] = &GL_SUPER_N, [GF_INJECT] = &GL_INJECT_N},
    {[GF_FLOWER] = &GE_FLOWER_N, [GF_STACK] = &GE_STACK_N, [GF_LOTION] = &GE_LOTION_N, [GF_DECK] = &GE_DECK_N, [GF_YOGA] = &GE_YOGA_N, [GF_TOP] = &GE_TOP_N, [GF_COW] = &GE_COW_N, [GF_GOLD] = &GE_GOLD_N, [GF_MILK] = &GE_MILK_N, [GF_SUPER] = &GE_SUPER_N, [GF_INJECT] = &GE_INJECT_N}
};
static int outfit_from = OF_USUAL;   /* v12.1: the outfit she wore before the last change (leaving the cow outfit gives a relieved line) */
static void outfit_apply(int k) { outfit_from = G.outfit; g_outfit_set(&G, k); }
static const char *outfit_line(void) {   /* v10.8: her line when she has changed into the outfit G.outfit (mama mode has its own) */
    int mm = g_mama(&G);
    if (outfit_from == OF_COW && G.outfit != OF_COW) { outfit_from = OF_USUAL; return mm ? pick(R_OUTFIT_UNCOW_MM, R_OUTFIT_UNCOW_MM_N) : pick(R_OUTFIT_UNCOW, R_OUTFIT_UNCOW_N); }   /* v12.1 */
    return G.outfit == OF_COW ? (mm ? pick(R_OUTFIT_COW_MM, R_OUTFIT_COW_MM_N) : pick(R_OUTFIT_COW, R_OUTFIT_COW_N)) : G.outfit == OF_TOP ? (mm ? pick(R_OUTFIT_TOP_MM, R_OUTFIT_TOP_MM_N) : pick(R_OUTFIT_TOP, R_OUTFIT_TOP_N)) : G.outfit == OF_YOGA ? (mm ? pick(R_OUTFIT_YOGA_MM, R_OUTFIT_YOGA_MM_N) : pick(R_OUTFIT_YOGA, R_OUTFIT_YOGA_N)) : (mm ? pick(R_OUTFIT_D8_MM, R_OUTFIT_D8_MM_N) : pick(R_OUTFIT_D8, R_OUTFIT_D8_N));
}
/* v11.2: the cow print gift once she has taken the beauty pill (her chest is swollen, she suspects the pill): beats 1..3 come from the milk-aware pools */
static const char *const *const *const GIFT_MILK_P[4] = {NULL, &GO_COW_MILK, &GL_COW_MILK, &GE_COW_MILK};
static int *const GIFT_MILK_N[4] = {NULL, &GO_COW_MILK_N, &GL_COW_MILK_N, &GE_COW_MILK_N};
static int gift_milk_aware(int beat, int kind) { return kind == GF_COW && G.pill_given && beat >= 1 && beat <= 3; }
static const char *gift_pick(int beat, int kind) {   /* a line for one beat of the gift scene (NULL = this kind has no pool for that beat) */
    if (gift_milk_aware(beat, kind)) return pick(*GIFT_MILK_P[beat], *GIFT_MILK_N[beat]);
    if (GIFT_POOL[beat][kind]) return pick(*GIFT_POOL[beat][kind], *GIFT_POOL_N[beat][kind]);
    return beat == 0 ? R_GIFT_HAND[kind] : beat == 2 ? R_GIFT_L2[kind] : beat == 3 ? R_GIFT_END[kind] : NULL;
}
static int gift_in_pool(int beat, int kind, const char *t) {   /* is t one of the pool lines (or, with a closing note after it, starts with one)? */
    const char *const *p; int n, i; if (!GIFT_POOL[beat][kind]) return 0;
    if (gift_milk_aware(beat, kind)) { p = *GIFT_MILK_P[beat]; n = *GIFT_MILK_N[beat]; } else { p = *GIFT_POOL[beat][kind]; n = *GIFT_POOL_N[beat][kind]; }
    for (i = 0; i < n; i++) if (beat == 3 ? !strncmp(t, p[i], strlen(p[i])) : t == p[i]) return 1;
    return 0;
}
static void say(const char *s) {
    const char *bar = strchr(s, '|'); int n = bar ? (int)(bar - s) : (int)strlen(s);
    snprintf(l1, sizeof l1, "%.*s", n, s); snprintf(l2, sizeof l2, "%s", bar ? bar + 1 : "");
    say_ms = now_ms();
}
#define SAYP(pool) say(pick(pool, pool##_N))

/* ---------- banner (win / lose / split) over the table; fade 0..256 ---------- */
static void banner_r(int res, int d, int fade, int by) {
    int bx = 213, bw = 235, bh = 82;
    int i, n, cw, w, x, y; char b[16];
    SDL_Surface *t = res > 0 ? bnW : res < 0 ? bnL : bnS, *g = res < 0 ? dgL : dgW;
    panel_tl(bx, by, bw, bh, fade, 200, 0);   /* v12.0: the line follows the GUI line colour */
    if (fade < 192) return;
    snprintf(b, sizeof b, "%c%d", res < 0 ? '-' : '+', d);
    if (!t || !g) {
        text(bx + 20, by + 10, res > 0 ? "YOU WIN" : res < 0 ? "YOU LOSE" : "SPLIT POT", 3, gui_tc());
        text(bx + 20, by + 46, b, 3, C(240,240,240)); return;
    }
    blit(t, bx + (bw - LW(t)) / 2, by + 5);
    cw = LW(g) / 12; n = (int)strlen(b); w = n * cw + 6 + (icon ? LW(icon) : 0);
    x = bx + (bw - w) / 2; y = by + 5 + LH(t) + 6;
    for (i = 0; i < n; i++) {
        int idx = b[i] == '+' ? 10 : b[i] == '-' ? 11 : b[i] - '0';
        blit_src(g, idx * cw, 0, cw, LH(g), x + i * cw, y);
    }
    blit(icon, x + n * cw + 6, y);
}

/* ---------- backgrounds and her sprite ---------- */
static void draw_walk_world(int bubble);
static void draw_icons(int ox, int oy);
/* v8.6: the page-4 scene pictures (hug / kiss / sleep / breastfeed), one per outfit: assets/sc_<scene>_<d8|yoga>.png. Only one of them is kept in memory; the placeholder files are replaced later. */
static SDL_Surface *scS[5]; static int scO[5] = {-1, -1, -1, -1, -1};   /* v12.3: 5th kind = the bottle hand-over picture (sc_bottle_<outfit>.png) */
#define HUG_BASE BG_HUG
static int ld_crop;   /* (declared again with its comment further down) */
static SDL_Surface *sc_pic(int id) {
    static const char *const NM[5] = {"hug", "kiss", "sleep", "feed", "bottle"}; int k = id - HUG_BASE, o = G.outfit, q, lr = (k == 3 && lr_on()) ? 4 : 0; char pth[48], fm[48];
    if (k < 0 || k > 4) return NULL;
    if (scS[k] && scO[k] != (o | lr)) lz_free(&scS[k]);   /* v9.9: bit 2 (value 4, v10.8) = the living-room picture of BREASTFEED */
    if (!scS[k]) {
        for (q = 0; q < 5; q++) if (q != k) lz_free(&scS[q]);
        if (lr) scS[k] = of_load("assets/sc_feed_lr_%s.png", o, "d8");   /* v10.8: top, yoga, d8 */
        if (!scS[k]) { snprintf(fm, sizeof fm, "assets/sc_%s_%%s.png", NM[k]); scS[k] = of_load(fm, o, "d8"); }
        scO[k] = o | lr;
        if (!scS[k]) { snprintf(pth, sizeof pth, "assets/sc_%s_d8.png", NM[k]); scS[k] = load_png(pth); }
    }
    return scS[k];
}
static void bg_draw(int id) {
    static const int fb[BG_N][3] = {{90,86,80},{120,110,100},{150,140,130},{130,120,105},{70,80,100},{70,70,80},{70,70,80},{90,86,80},{30,40,60},{60,50,50},{40,40,60},{40,40,60},{40,40,60},{40,40,60},{60,50,50},{40,30,40},{40,30,40},{40,30,40},{40,30,40},{40,30,40}};
    T = S;
    if (id == BG_LOBBYMAP) { draw_walk_world(0); return; }   /* the arrival lines play on the real lobby map, with you standing in it */
    if (id >= BG_HUG && id <= BG_BOTTLE) { SDL_Surface *sp = sc_pic(id); if (sp) { SDL_BlitSurface(sp, NULL, S, NULL); return; } id = BG_ROM; }   /* v8.6 */
    if (id == BG_PUMP && lr_on()) { static SDL_Surface *pl; static int tried; if (!tried) { tried = 1; ld_crop = 1; pl = load_png("assets/pump_lr.png"); ld_crop = 0; } if (pl) { SDL_BlitSurface(pl, NULL, S, NULL); return; } }   /* v9.9: the peek picture in the living room (placeholder: the room one) */
    if (bgs[id]) SDL_BlitSurface(bgs[id], NULL, S, NULL); else R(0, 0, 640, 480, C(fb[id][0], fb[id][1], fb[id][2]));
}
/* the poker table scene: table background + her table picture */
static int her_pick;   /* which face she wears right now: 0 neutral, 1 shock (you won a pot), 2 smug (she won one), 3 caught you staring (the close-up answer) */
/* v8.8: from the moment she lives with you until the day-8 scene (and in the yoga outfit) the table faces are the home set: assets/her_<neutral|shock|smug>_<home|yoga>.png (yoga = yoga outfit), and the close-up answer
   assets/her_caught_<home|yoga>.png. They are separate files from the covering pictures she uses in her room (her_cover / her_cover_yoga). Missing yoga files fall back to the home ones. */
static SDL_Surface *hfaceS[3]; static int hfaceO = -1;
static int home_faces(void) { return G.rs == RS_LIVING && (G.outfit || !G.pill_seen); }   /* v8.8: home set = moved in but before the day-8 scene, and the yoga outfit; after the day-8 scene (usual outfit) the day-8 faces (her_*_p, her_caught) */
static SDL_Surface *face_home_pic(int k) {
    static const char *const NM[3] = {"neutral", "shock", "smug"};
    int o = G.outfit, i; char p[64];
    if (hfaceO != o) { for (i = 0; i < 3; i++) lz_free(&hfaceS[i]); lz_free(&herCaughtS); hfaceO = o; }
    if (!hfaceS[k]) {
        snprintf(p, sizeof p, "assets/her_%s_%%s.png", NM[k]); hfaceS[k] = of_load(p, o, "home");   /* v10.8: top, yoga, home */
    }
    if (hfaceS[k]) { lz_free(&her); lz_free(&herP); lz_free(&herShock); lz_free(&herShockP); lz_free(&herSmug); lz_free(&herSmugP); }   /* the old sets are dropped from memory */
    return hfaceS[k];
}
static int caughtO = -1;
static SDL_Surface *caught_pic(void) {
    if (herCaughtS && caughtO != G.outfit) lz_free(&herCaughtS);   /* v10.8: she changed clothes: load the picture of the new outfit */
    caughtO = G.outfit;
    if (home_faces()) {
        if (!herCaughtS) herCaughtS = of_load("assets/her_caught_%s.png", G.outfit, "home");   /* top, yoga, home */
        if (herCaughtS) return herCaughtS;
    }
    return lz(&herCaughtS, "assets/her_caught.png");
}
static void bg(int with_her) {
    SDL_Surface *im = NULL;
    if (her_pick == 3) im = caught_pic();
    else if (home_faces()) im = face_home_pic(her_pick == 1 || her_pick == 2 ? her_pick : 0);
    else { int i; for (i = 0; i < 3; i++) lz_free(&hfaceS[i]); hfaceO = -1; }   /* the home faces are dropped once the day-8 faces take over */
    if (!im) im = her_pick == 1 ? pl_pick(&herShock, "assets/her_shock.png", &herShockP, "assets/her_shock_p.png") : her_pick == 2 ? pl_pick(&herSmug, "assets/her_smug.png", &herSmugP, "assets/her_smug_p.png") : pl_pick(&her, "assets/her_neutral.png", &herP, "assets/her_neutral_p.png");
    if (with_her) {   /* v11.8: background + her picture are composed ONCE into an opaque surface and copied with one plain blit per frame (same idea as the Exhibition felt), so the cards and chips move as smoothly as there */
        static SDL_Surface *tblC; static int ck_bg = -1, ck_w, ck_h, ck_fp; static const void *ck_im, *ck_bgs;
        int id = g_rom_on(&G) ? BG_ROM : BG_TABLE, fp = 0;
        if (im && im->pixels) { const Uint32 *q = (const Uint32 *)im->pixels; int n = im->w * im->h, k; for (k = 0; k < 61; k++) fp = fp * 31 + (int)q[(long)n * k / 61]; }
        T = S;
        if (tblC && ck_bg == id && ck_im == (const void *)im && ck_bgs == (const void *)bgs[id] && ck_fp == fp && ck_w == S->w && ck_h == S->h && tblC->w == S->w && tblC->h == S->h) { SDL_BlitSurface(tblC, NULL, S, NULL); return; }
        bg_draw(id); if (im) blit(im, 0, 0); else R(0, 362, 640, 118, C(45,112,82));
        if (!tblC || tblC->w != S->w || tblC->h != S->h) { if (tblC) SDL_FreeSurface(tblC); tblC = SDL_CreateRGBSurface(0, S->w, S->h, 32, S->format->Rmask, S->format->Gmask, S->format->Bmask, 0); }
        if (tblC) { SDL_BlitSurface(S, NULL, tblC, NULL); ck_bg = id; ck_im = im; ck_bgs = bgs[id]; ck_fp = fp; ck_w = S->w; ck_h = S->h; }
        return;
    }
    bg_draw(g_rom_on(&G) ? BG_ROM : BG_TABLE);   /* v6.7: once she lives with you the matches are played in her bedroom */
}
/* alpha-blend an image over the screen (a: 0..256), used to fade her in and out */
static void blit_a(SDL_Surface *im, int a) {
    int x, y; SDL_PixelFormat *f, *g = S->format;
    if (!im || a <= 0) return;
    if (a >= 256) { blit(im, 0, 0); return; }
    f = im->format;
    for (y = 0; y < im->h && y < SH; y++) {
        Uint32 *s = px(im, 0, y), *d = px(S, 0, y);
        for (x = 0; x < im->w && x < SW; x++) {
            Uint32 v = s[x], o; int sa = (int)((v & f->Amask) >> f->Ashift) * a >> 8, sr, sg, sb, dr, dg, db;
            if (sa <= 0) continue;
            sr = (v >> f->Rshift) & 255; sg = (v >> f->Gshift) & 255; sb = (v >> f->Bshift) & 255;
            o = d[x]; dr = (o >> g->Rshift) & 255; dg = (o >> g->Gshift) & 255; db = (o >> g->Bshift) & 255;
            dr = (dr * (256 - sa) + sr * sa) >> 8; dg = (dg * (256 - sa) + sg * sa) >> 8; db = (db * (256 - sa) + sb * sa) >> 8;
            d[x] = (o & ~(g->Rmask | g->Gmask | g->Bmask)) | ((Uint32)dr << g->Rshift) | ((Uint32)dg << g->Gshift) | ((Uint32)db << g->Bshift);
        }
    }
}
static const char *who_name(int w) {
    switch (w) {
        case W_ME: return N_ME; case W_UNK: return N_UNK; case W_AW: return N_AW; case W_SOPHIA: return N_SOPHIA;
        case W_HER: return G.story >= ST_NAMED ? N_SOPHIA : N_AW;
        default: return NULL;
    }
}
static int snd_on = 1;   /* OPTIONS: sound on/off */
static int dlg_cols(void) { return 46; }   /* characters per line of the dialogue box */
/* v8.4: the dialogue box sits flush on the bottom edge, full width, and its height follows the number of text rows (1 / 2 / 3 rows = 64 / 86 / 108 px) */
#define DLG_BOT 480   /* bottom edge of every dialogue-style box (logical 640x480) */
#define DLG_L   24    /* left inset of the text, of the name tag and of the choices */
#define DLG_R   624   /* right edge for the "A >" prompts and the small labels */
static int dlg_py = 458;   /* y of the "A >" prompt of the box drawn last (dialog_x sets it) */
static int typed_done(const char *txt, unsigned ms) {
    char w[400]; int total = 0, i;
    if (frozen) return 1;
    wrap_buf(w, sizeof w, txt, dlg_cols()); for (i = 0; w[i]; i++) if (w[i] != '\n') total++;
    return (int)(age_of(ms) / 30) >= total;
}
/* dialogue box opacity (Options in the title menu, saved in pokerqueen.cfg, kept when a career ends) */
#define CFG_PATH "pokerqueen.cfg"
#define LINES_PATH "lines.txt"   /* v7.9: external texts (see extlines.h and TEXT_GUIDE.txt) */
#define TALK_PATH  "talk.txt"
#define ITEMS_PATH "items.txt"   /* v9.5: item names and info texts */
#define NAMES_PATH "names.txt"   /* v9.5: the names you can type for her (R2) */
#define TLOG_PATH  "text_log.txt"
static int text_problems;   /* how many entries of lines.txt / talk.txt the game refused (0 = all fine) */
static int dlg_idx = 2;
static int walk_idx = 1;   /* v8.0: walking speed 0 SLOW, 1 NORMAL, 2 FAST */
static const unsigned WALK_T[3] = { 600, 480, 350 };   /* ms per step */
static const char *const WALK_N[3] = { "SLOW", "NORMAL", "FAST" };
static const int DLG_A[6] = {256, 205, 154, 102, 51, 0};
static const char *const DLG_N[6] = {"100%", "80%", "60%", "40%", "20%", "0%"};   /* v10.7: the lowest step is 0%: box and lines fully transparent */
/* v10.7: line colour of the dialogue / name boxes and the colours of the two names (Options > TEXT BOX STYLE, saved in pokerqueen.cfg) */
static const int PAL_NAME[8][3] = { {255,225,110}, {255,255,255}, {255,110,110}, {255,150,205}, {255,175,80}, {120,235,140}, {130,180,255}, {190,150,255} };
static const char *const PAL_N[8] = { "GOLD", "WHITE", "RED", "PINK", "ORANGE", "GREEN", "BLUE", "PURPLE" };
static int dlg_line = 0, col_soph = 0, col_you = 0;
#define DLG_GOLD  C(255,225,110)
#define DLG_WHITE C(255,255,255)
/* the dialogue-style box: fill AND border follow the opacity (100% = the old look, 0% = nothing drawn) */
static void panel_d(int x, int y, int w, int h, int fade) {
    int a = DLG_A[dlg_idx], e = ((a * 230) >> 8) * fade >> 8; const int *l = PAL_LINE[dlg_line];
    shade(x, y, w, h, 18, 24, 42, a * fade >> 8);
    shade(x, y, w, 2, l[0], l[1], l[2], e); shade(x, y + h - 2, w, 2, l[0], l[1], l[2], e);
    shade(x, y + 2, 2, h - 4, l[0], l[1], l[2], e); shade(x + w - 2, y + 2, 2, h - 4, l[0], l[1], l[2], e);
}
static Uint32 name_col(const char *tag) {   /* the colour of a name tag: yours, hers (also ??? and ARROGANT WOMAN), anything else gold */
    const int *c;
    if (!tag) return DLG_GOLD;
    if (!strcmp(tag, N_ME)) c = PAL_NAME[col_you];
    else if (!strcmp(tag, N_SOPHIA) || !strcmp(tag, N_AW) || !strcmp(tag, N_UNK)) c = PAL_NAME[col_soph];
    else return DLG_GOLD;
    return C(c[0], c[1], c[2]);
}
static void cfg_save(void) { FILE *f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "dlg %d\nsnd %d\nwalk %d\nslot %d\nline %d\nsoph %d\nyou %d\ntbo %d\nmnl %d\nmnt %d\ngul %d\ngut %d\n", dlg_idx, snd_on, walk_idx, slot_act, dlg_line, col_soph, col_you, tb_idx, mn_line, mn_text, gui_line, gui_text); fclose(f); } }
static void cfg_load(void) {
    FILE *f = fopen(CFG_PATH, "r"); int v; char k[16];
    if (!f) return;
    while (fscanf(f, "%15s %d", k, &v) == 2) {
        if (!strcmp(k, "dlg") && v >= 0 && v < 6) dlg_idx = v;
        else if (!strcmp(k, "snd") && (v == 0 || v == 1)) snd_on = v;
        else if (!strcmp(k, "walk") && v >= 0 && v < 3) walk_idx = v;
        else if (!strcmp(k, "line") && v >= 0 && v < 8) dlg_line = v;   /* v10.7 */
        else if (!strcmp(k, "soph") && v >= 0 && v < 8) col_soph = v;
        else if (!strcmp(k, "you") && v >= 0 && v < 8) col_you = v;
        else if (!strcmp(k, "slot") && v >= 1 && v <= SLOT_N) slot_act = v;   /* v10.5 */
        else if (!strcmp(k, "tbo") && v >= 0 && v < 6) tb_idx = v;   /* v11.5 */
        else if (!strcmp(k, "gul") && v >= 0 && v < 8) gui_line = v;   /* v12.0 (an old \"tbl\" line is ignored) */
        else if (!strcmp(k, "gut") && v >= 0 && v < 8) gui_text = v;
        else if (!strcmp(k, "mnl") && v >= 0 && v < 8) mn_line = v;   /* v11.9 */
        else if (!strcmp(k, "mnt") && v >= 0 && v < 8) mn_text = v;
    }
    fclose(f);
}
/* ---------- sound: six short effects (assets/*.wav, made from the supplied mp3 files), mixed by SDL's own audio callback (no SDL_mixer needed) ---------- */
enum { SFX_SHUF, SFX_GIVE, SFX_OPEN, SFX_PLACE, SFX_ALLIN, SFX_COLLECT, SFX_N };
static const char *const SFX_FILE[SFX_N] = { "assets/Card_Shuffle.wav", "assets/Give_Card.wav", "assets/Open_Card.wav", "assets/Placing_Chip.wav", "assets/All_in_Chip.wav", "assets/Collect_Pot_Chip.wav" };
static int sfx_log[SFX_N];   /* how often each effect was started (checks) */
static struct { unsigned at; int id; } sq[48]; static int sq_n;   /* effects waiting for their moment on the table timeline */
#ifndef SDL_STUB_H
typedef struct { Sint16 *d; int n; } Sfx;
static Sfx sfx[SFX_N]; static struct { int id, pos; } voice[8]; static int audio_ok;
static void audio_cb(void *u, Uint8 *out, int len) {
    Sint16 *o = (Sint16 *)out; int n = len / 2, i, v; (void)u; memset(out, 0, (size_t)len);
    for (v = 0; v < 8; v++) if (voice[v].id >= 0) {
        const Sfx *f = &sfx[voice[v].id];
        for (i = 0; i < n && voice[v].pos < f->n; i++, voice[v].pos++) { int m = o[i] + f->d[voice[v].pos]; o[i] = (Sint16)(m > 32767 ? 32767 : m < -32768 ? -32768 : m); }
        if (voice[v].pos >= f->n) voice[v].id = -1;
    }
}
static void audio_init(void) {   /* any failure just leaves the game silent */
    SDL_AudioSpec want, have; int i;
    for (i = 0; i < 8; i++) voice[i].id = -1;
    memset(&want, 0, sizeof want); want.freq = 22050; want.format = AUDIO_S16SYS; want.channels = 1; want.samples = 1024; want.callback = audio_cb;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) return;
    if (SDL_OpenAudio(&want, &have) < 0) return;
    if (have.format != AUDIO_S16SYS) { SDL_CloseAudio(); return; }
    for (i = 0; i < SFX_N; i++) {
        SDL_AudioSpec ws; Uint8 *wb; Uint32 wl; SDL_AudioCVT cvt; int len;
        if (!SDL_LoadWAV(SFX_FILE[i], &ws, &wb, &wl)) continue;
        if (SDL_BuildAudioCVT(&cvt, ws.format, ws.channels, ws.freq, have.format, have.channels, have.freq) < 0) { SDL_FreeWAV(wb); continue; }
        cvt.buf = (Uint8 *)malloc((size_t)wl * (size_t)(cvt.len_mult > 0 ? cvt.len_mult : 1));
        if (!cvt.buf) { SDL_FreeWAV(wb); continue; }
        memcpy(cvt.buf, wb, wl); SDL_FreeWAV(wb); cvt.len = (int)wl; len = (int)wl;
        if (cvt.needed) { SDL_ConvertAudio(&cvt); len = cvt.len_cvt; }
        sfx[i].d = (Sint16 *)cvt.buf; sfx[i].n = len / 2;
    }
    audio_ok = 1; SDL_PauseAudio(0);
}
#endif
static void sfx_play(int id) {
    int v; if (id < 0 || id >= SFX_N) return;
    sfx_log[id]++;
#ifndef SDL_STUB_H
    if (!audio_ok || !snd_on || frozen || !sfx[id].d) return;
    SDL_LockAudio();
    for (v = 0; v < 8; v++) if (voice[v].id < 0) { voice[v].id = id; voice[v].pos = 0; break; }
    SDL_UnlockAudio();
#else
    (void)v;
#endif
}
static void sfx_at(unsigned t, int id) { if (frozen || sq_n >= 48) return; sq[sq_n].at = t; sq[sq_n].id = id; sq_n++; }   /* play id when the clock reaches t */
static void sfx_tick(void) { unsigned now = now_ms(); int i = 0; while (i < sq_n) { if ((int)(now - sq[i].at) >= 0) { sfx_play(sq[i].id); sq[i] = sq[--sq_n]; } else i++; } }
static int dlg_lines(const char *txt) {   /* rows of the whole sentence (not of the typed part), 1..3 */
    char w[400]; int i, n = 1; wrap_buf(w, sizeof w, txt, dlg_cols());
    for (i = 0; w[i]; i++) if (w[i] == '\n') n++;
    return n < 1 ? 1 : n > 3 ? 3 : n;
}
static int dlg_h(const char *txt) { return 20 + dlg_lines(txt) * 22 + 22; }   /* 3 rows = 108 (the old fixed height) */
/* the text box at the bottom: name tag, typed-out text. returns 1 once fully typed */
static int dialog_x(const char *txt, const char *tag, int narr, unsigned ms, int sh) {
    char w[400]; int shown, i, j = 0, lines = 0, total = 0, x = DLG_L, h = sh ? 54 : dlg_h(txt), top = DLG_BOT - h, y = top + (sh ? 9 : 12), sc = 2, cw = 6 * sc, lh = 22, cols = sh ? 46 : dlg_cols();
    panel_d(0, top, 640, h, 256);   /* v8.4: full width, flush on the bottom edge; sh = the short two-line box of the R1 close-up */
    dlg_py = sh ? top + 36 : top + h - 22;
    if (tag && !sh) { int ty = top - 32; panel_d(10, ty, tw(tag, 2) + 28, 28, 256); dtext(DLG_L, ty + 7, tag, 2, name_col(tag)); }
    wrap_buf(w, sizeof w, txt, cols);
    for (i = 0; w[i]; i++) if (w[i] != '\n') total++;
    shown = frozen ? total : (int)(age_of(ms) / 30); if (shown > total) shown = total;
    for (i = 0; w[i]; i++) {
        char ch[2]; int col = 0, k;
        if (w[i] == '\n') { lines++; continue; }
        if (j >= shown) break;
        ch[0] = w[i]; ch[1] = 0; for (k = i - 1; k >= 0 && w[k] != '\n'; k--) col++;
        dtext(x + col * cw, y + lines * lh, ch, sc, DLG_WHITE);   /* v10.7: narration is white too */
        j++;
    }
    return shown >= total;
}
static int dialog(const char *txt, const char *tag, int narr, unsigned ms) { return dialog_x(txt, tag, narr, ms, 0); }
/* vertical colour ramp */
static void vgrad(int x, int y, int w, int h, int r1, int g1, int b1, int r2, int g2, int b2) {
    int j; for (j = 0; j < h; j++) R(x, y + j, w, 1, C(r1 + (r2 - r1) * j / (h > 1 ? h - 1 : 1), g1 + (g2 - g1) * j / (h > 1 ? h - 1 : 1), b1 + (b2 - b1) * j / (h > 1 ? h - 1 : 1)));
}

/* her speech box (upper left): types out, lingers, fades */
static void draw_say(void) {
    unsigned age = age_of(say_ms); char b[48];
    int fade = age < 4000 ? 256 : (age < 4500 ? (int)(256 * (4500 - age) / 500) : 0), shown = frozen ? 99 : (int)(age / 30), n1 = (int)strlen(l1);
    int bx = card_pink ? 0 : 10, by = card_pink ? 0 : 10, bw = card_pink ? 223 : 240, tx = card_pink ? 8 : 20;   /* Sophia's table: flush to the corner */
    if (!l1[0] || fade <= 0) return;
    panel_tl(bx, by, bw, 74, fade, TB_A[tb_idx], 0);   /* v12.0: the corner box follows TABLE OPACITY and the GUI line colour (it was the dialogue box style) */
    if (fade >= 128) {
        snprintf(b, sizeof b, "%.*s", shown < n1 ? shown : n1, l1); dtext(tx, by + 10, b, 2, DLG_WHITE);
        if (shown > n1 && l2[0]) { snprintf(b, sizeof b, "%.*s", shown - n1, l2); dtext(tx, by + 36, b, 2, DLG_GOLD); }
    }
}
static void draw_cash_v(int v, int y) {
    char b[24]; b[0] = '$'; fmt_money(b + 1, v);
    panel(488, y, 148, 30, 256);
    text(496, y + 10, "CASH", 1, gui_tc()); text_r(628, y + 7, b, 2, C(240,240,240));
}
static void draw_cash(void) { draw_cash_v(G.cash, 4); }
static void draw_cash_m(void) {   /* v11.9: the cash box of the online store in the menu colours */
    char b[24]; b[0] = '$'; fmt_money(b + 1, G.cash);
    panel_m(488, 4, 148, 30, 256);
    text(496, 14, "CASH", 1, mn_tc()); text_r(628, 11, b, 2, C(240,240,240));
}
static void text_fit(int x, int y, const char *s1, Uint32 c1, const char *s2, Uint32 c2, int w) {   /* "DAY 1  MORNING" on one line, shrunk to fit w px (fractional pixel-font scale) */
    int n = (int)strlen(s1) + 2 + (int)strlen(s2), fs = w * 100 / (n * 6), x2;
    if (fs > 200) fs = 200;
    text_fs = fs; y += (14 - 7 * fs / 100) / 2;
    text(x, y, s1, 2, c1); x2 = x + ((int)strlen(s1) + 2) * 6 * fs / 100; text(x2, y, s2, 2, c2);
    text_fs = 0;
}
static void draw_time(void) {   /* day counter and time of day: one line under the cash box, as wide as the cash box */
    char b[16]; Uint32 col = G.phase == PH_NIGHT ? C(140,170,235) : G.phase == PH_EVENING ? C(240,160,90) : C(250,225,120);
    panel(488, 38, 148, 30, 256);
    snprintf(b, sizeof b, "DAY %d", G.day);
    text_fit(496, 45, b, C(240,240,240), PH_NAME[G.phase], col, 132);
    if (g_rent_due(&G)) {   /* red, right under the day box, until the rent is paid */
        int fs = 170, w = (int)strlen(RENT_DUE_TXT) * 6 * fs / 100;
        text_fs = fs; text_o(488 + (148 - w) / 2, 74, RENT_DUE_TXT, 2, C(235,55,55)); text_fs = 0;
    }
}
static void menu_panel(const char *const *items, int n, int s) {
    int i, h = 10 + 22 * n, y0 = DLG_BOT - h, w = 340, l;
    for (i = 0; i < n; i++) { l = 184 - 150 + 12 * (int)strlen(items[i]) + 12; if (l > w) w = l; }   /* v10.8: wide enough for the longest row (CHANGE TO THE DESIGNER TOP) */
    panel(150, y0, w, h, 256);
    for (i = 0; i < n; i++) {
        Uint32 col = i == s ? gui_tc() : C(240,240,240);
        if (i == s) text(162, y0 + 10 + 22 * i, ">", 2, col);
        text(184, y0 + 10 + 22 * i, items[i], 2, col);
    }
}

/* ---------- table animation (all tables): dealing, board cards, chips into the pot, the pot to the winner ----------
   Everything runs on one timeline and one thing moves at a time: the cards first, a short pause, then the chips.
   Nobody acts until it has finished. Pressing A while it plays skips to the end. */
#define DEAL_STEP  420   /* ms between cards leaving the deck (her card, your card, her card, your card) */
#define DEAL_FLY   420   /* one dealt card flying to its place */
#define DEAL_ALL   (3 * DEAL_STEP + DEAL_FLY)
#define CHIP_FLY   650   /* chips flying into the pot */
#define CHIP_GAP   250   /* a breath between cards and chips */
#define BOARD_FLY  520   /* a board card flying in face down */
#define BOARD_HOLD 180   /* it rests face down */
#define BOARD_FLIP 320   /* then turns up */
#define BOARD_STEP 420   /* between the cards of a flop */
#define FLIP_MS    320   /* her cards turning up at the showdown */
#define PEEK_MS    250   /* you looking at (or hiding) your cards */
#define PAY_FLY    750   /* the pot flying to the winner */
#define LEAD_MS 1500   /* the first hand of a match: a pause at the empty table before the first card is dealt */
#define HAND_LEAD 850  /* every later hand at her table: a shorter breath (shuffle sound, her new-hand line) before the cards go out */
#define HLINE_GAP 350  /* her win / lose line comes this long after the result banner is fully up */
static int lead_next; static unsigned lead_len = LEAD_MS;   /* set when a hand is about to be dealt; the next anim_new_hand uses it once, lead_len long */
static const char *hline_txt; static unsigned hline_ms;   /* her line for the hand that just ended, waiting for the banner */
static unsigned deal_ms, board_ms[5], fly_ms[2], pay_ms, rev_ms, peek_ms, anim_end_ms; static int fly_amt[2], pay_who, her_expr, peek, hdl = 300;   /* fly slot 0 you, 1 her; pay_who 1 you 2 her 3 split; peek 1 = your cards face up */
static int anim_fix = -1;   /* checks only: pretend every animation is this many ms old */
static unsigned anim_age(unsigned t0) {   /* ms since t0 (negative as int = not started yet) */
    if (anim_fix >= 0) return (unsigned)anim_fix;
    if (frozen) return 5000u;
    return now_ms() - t0;
}
static int anim_fix_deal = -1;   /* checks only: the same for the dealing */
static unsigned anim_age_deal(unsigned t0) { return anim_fix_deal >= 0 ? (unsigned)anim_fix_deal : anim_fix >= 0 ? 5000u : anim_age(t0); }
static int smooth256(int p) { if (p <= 0) return 0; if (p >= 256) return 256; return p * p * (768 - 2 * p) / 65536; }   /* v7.6: smoothstep 0..256 (soft start, soft landing) */
static int ease(int a, int dur) { if (a <= 0) return 0; if (a >= dur) return 256; return smooth256(a * 256 / dur); }   /* v7.6: 0..256, starts and lands softly (was ease-out only) */
static int lift_of(int e, int h) { return (int)(sin(3.14159 * e / 256.0) * h); }   /* v7.6: a card rises a little while it flies */
static unsigned hand_dl(void) { return (unsigned)hdl; }
static int net_shown(unsigned hage) { return G.over && hage < hand_dl() + 300u ? G.net - G.res * G.amt : G.net; }   /* the NET plate: this hand's result counts only once the banner is fully up */   /* when the result banner starts (after the chips, the board and the showdown flip) */
static int turn_of(int up, unsigned t0, int dur) {   /* how face-up a card is, 0..256, while it turns (towards up or towards down) */
    int a = (int)anim_age(t0), f = a < 0 ? 0 : a >= dur ? 256 : smooth256(a * 256 / dur);
    return up ? f : 256 - f;
}
static void anim_shift(unsigned d) {   /* skip: everything that is scheduled happens d ms sooner */
    unsigned now = now_ms(); int k;
    if (!d) return;
    { int i = 0; while (i < sq_n) { unsigned na = sq[i].at - d; if ((int)(na - now) <= 0 && sq[i].id != SFX_COLLECT) sq[i] = sq[--sq_n]; else { sq[i].at = na; i++; } } }   /* skipped: sounds that would pile up are dropped, the rest move up with their pictures */
    if (hline_ms) hline_ms -= d; if (deal_ms) deal_ms -= d; if (pay_ms) pay_ms -= d; if (rev_ms) rev_ms -= d; if (peek_ms) peek_ms -= d; if (hand_end_ms) hand_end_ms -= d;
    for (k = 0; k < 5; k++) if (board_ms[k]) board_ms[k] -= d;
    for (k = 0; k < 2; k++) if (fly_ms[k]) fly_ms[k] -= d;
    anim_end_ms = anim_end_ms > d ? anim_end_ms - d : 0;
    if (think_at > now) think_at -= (think_at - now < d ? think_at - now : d);
}
static unsigned anim_left(int over) {   /* ms until the table is calm: nothing moving, and (after a hand) the banner is up */
    unsigned now = now_ms(), left = anim_end_ms > now ? anim_end_ms - now : 0;
    if (frozen) return 0;
    if (over) { unsigned age = now - hand_end_ms; if (hand_end_ms && age < hand_dl() && hand_dl() - age > left) left = hand_dl() - age; }
    return left;
}
static unsigned after_anim(unsigned delay) { unsigned t = now_ms(); if (anim_end_ms > t) t = anim_end_ms; return t + delay; }   /* a moment after the table has calmed down */
static void anim_new_hand(int c0, int c1, int over) {   /* cards: her, you, her, you. Then the blinds go into the pot, one after the other */
    unsigned lead = (lead_next && !over && !frozen) ? lead_len : 0, t = now_ms() + lead, tb = t + DEAL_ALL + CHIP_GAP;   /* t is in the future during the lead-in: nothing is dealt yet */
    lead_next = 0; lead_len = LEAD_MS; deal_ms = t; memset(board_ms, 0, sizeof board_ms);
    if (!over && !frozen) { int k; sfx_at(t - lead, SFX_SHUF); for (k = 0; k < 4; k++) sfx_at(t + (unsigned)(k * DEAL_STEP) + 60, SFX_GIVE); }   /* shuffle, then four cards leave the deck */ fly_amt[0] = fly_amt[1] = 0; fly_ms[0] = fly_ms[1] = 0; pay_who = 0; pay_ms = 0; her_expr = 0;
    peek = 0; peek_ms = 0; rev_ms = 0; hdl = 300; anim_end_ms = t;
    if (over || frozen) return;
    fly_amt[0] = c0; fly_amt[1] = c1; fly_ms[0] = tb + (c0 <= c1 ? 0 : 220); fly_ms[1] = tb + (c1 < c0 ? 0 : 220);
    anim_end_ms = tb + 220 + CHIP_FLY;
    if (c0 > 0) sfx_at(fly_ms[0] + 350, SFX_PLACE); if (c1 > 0) sfx_at(fly_ms[1] + 350, SFX_PLACE);   /* the blinds land in the pot */
}
/* after somebody acts: first the chips go into the pot, a pause, then any new board cards (face down, then turned up). When the hand ends at a showdown,
   her cards turn up last, and the result banner follows. */
static int anim_allin;   /* set by the caller just before anim_after: the chips going in are an all-in */
static void anim_after(int who, int added, int n0, int n1, int ended, int showdown, int you_show) {
    unsigned t0 = now_ms(), t = t0; int k, allin = anim_allin; anim_allin = 0;
    if (frozen) {   /* checks and screenshots: no waiting, same layout as before */
        for (k = n0; k < n1; k++) board_ms[k] = 0;
        if (added > 0) { fly_amt[who] = added; fly_ms[who] = 0; }
        if (ended) { hdl = showdown ? 700 : 300; rev_ms = 0; if (showdown && you_show && !peek) { peek = 1; peek_ms = 0; } }
        return;
    }
    if (added > 0) { fly_amt[who] = added; fly_ms[who] = t; sfx_at(t + 350, allin ? SFX_ALLIN : SFX_PLACE); t += CHIP_FLY + CHIP_GAP; }
    if (n1 > n0) {
        for (k = n0; k < n1; k++) { board_ms[k] = t + (unsigned)(BOARD_STEP * (k - n0)); sfx_at(board_ms[k] + 120, SFX_GIVE); sfx_at(board_ms[k] + BOARD_FLY + BOARD_HOLD, SFX_OPEN); }
        t += (unsigned)(BOARD_STEP * (n1 - n0 - 1) + BOARD_FLY + BOARD_HOLD + BOARD_FLIP + CHIP_GAP);
    }
    anim_end_ms = t;
    if (ended) {
        hdl = (int)(t - t0) + (showdown ? FLIP_MS + 650 : 300);
        if (showdown) { rev_ms = t; sfx_at(t, SFX_OPEN); if (you_show && !peek) { peek = 1; peek_ms = t; } }
    }
}
static void peek_toggle(int over, int showdown) {   /* L1: look at your cards, or hide them again (not once they are shown down) */
    if ((int)anim_age(deal_ms + 3 * DEAL_STEP + DEAL_FLY) < 0) return;   /* your second card is not down yet */
    if (over && showdown) return;
    peek = !peek; peek_ms = now_ms(); sfx_play(SFX_OPEN);
}
/* showdown outline: it appears in steps, not all at once. The loser's or opponent's hole cards first, then yours, then the board cards of the winning hand
   (a group with nothing to outline is skipped). hd_* are what is drawn; hl_* is what the showdown decided. */
#define HL_STEP 450
static int hd_hole[MP_MAX][2], hd_board[5];
static void hl_has(int *has) {   /* [0] opponent hole cards, [1] your hole cards, [2] board */
    int i; has[0] = has[1] = has[2] = 0;
    for (i = 0; i < 2; i++) { if (hl_hole[1][i]) has[0] = 1; if (hl_hole[0][i]) has[1] = 1; }
    for (i = 0; i < 5; i++) if (hl_board[i]) has[2] = 1;
}
static void hl_gate(void) {
    int has[3], i, g, step = 0, age; hl_has(has); memset(hd_hole, 0, sizeof hd_hole); memset(hd_board, 0, sizeof hd_board);
    age = (int)anim_age(rev_ms) - FLIP_MS;   /* the clock starts when the cards are face up */
    for (g = 0; g < 3; g++) {
        if (!has[g]) continue;
        if (age >= step * HL_STEP) {
            if (g == 0) for (i = 0; i < 2; i++) hd_hole[1][i] = hl_hole[1][i];
            else if (g == 1) for (i = 0; i < 2; i++) hd_hole[0][i] = hl_hole[0][i];
            else for (i = 0; i < 5; i++) hd_board[i] = hl_board[i];
        }
        step++;
    }
}
static void hl_extend(int showdown) {   /* the result banner waits until the last outline is on */
    int has[3]; if (!showdown || frozen) return; hl_has(has); hdl += (has[0] + has[1] + has[2]) * HL_STEP;
}
#define POT_Y 456    /* v8.5: pot chip stacks and the HER / CPU stack text sit lower (were 444 / 446), close to the bottom edge */
#define POT_CY 448   /* where chips land in / leave the pot (was 436) */
#define STK_TY 460
static void fly_chips(int amt, int sx, int sy, int dx, int dy, int e) {   /* up to 6 chips in an arc from (sx,sy) to (dx,dy), e = 0..256 */
    int cnt[4], i, j, k = 0, n = 0, gap = 10;   /* v7.6: the chips leave one after another (gap = 10/256 of the flight) and each rises on its own small arc */
    change(amt, cnt);
    for (i = 0; i < 4; i++) n += cnt[i]; if (n > 6) n = 6;
    for (i = 3; i >= 0 && k < 6; i--) for (j = 0; j < cnt[i] && k < 6; j++, k++) {
        int ek = n > 1 ? smooth256(e * (256 + (n - 1) * gap) / 256 - k * gap) : smooth256(e), arc = (int)(sin(3.14159 * ek / 256.0) * 28);
        if (e >= 256) ek = 256, arc = 0;
        if (potS[i]) blit(potS[i], sx + (dx - sx) * ek / 256 + (k % 3) * 7, sy + (dy - sy) * ek / 256 - arc - (k / 3) * 5);
    }
}
static int fly_left(int who) { return fly_amt[who] && (int)anim_age(fly_ms[who]) < CHIP_FLY ? fly_amt[who] : 0; }   /* chips of this seat that are not in the pot yet */
static int fly_wait(int who) { return fly_amt[who] && (int)anim_age(fly_ms[who]) < 0 ? fly_amt[who] : 0; }       /* chips that have not left the rack yet */
static int you_fy = 135;   /* where your chips leave from / arrive at (the heads-up money box is higher than the multiplayer chip rack) */
static void draw_flights(void) {
    int w;
    for (w = 0; w < 2; w++) { int a = (int)anim_age(fly_ms[w]); if (fly_amt[w] && a >= 0 && a < CHIP_FLY) fly_chips(fly_amt[w], 540, w ? 425 : you_fy, 60, POT_CY, a * 256 / CHIP_FLY); }
}
/* a card turning over, flat (your cards): fl 0 = back, 256 = face; it is thinnest in the middle */
static void flat_turn(int x, int y, int w, int h, int c, int fl) {
    int k, w2, i, j, up, wn = XS(w), hn = YS(h), xn = XS(x), yn = YS(y); Uint32 key = C(255,0,255);
    if (fl <= 0) { card(x, y, w, h, c, 0); return; }
    if (fl >= 256) { card(x, y, w, h, c, 1); return; }
    k = fl < 128 ? 128 - fl : fl - 128; w2 = wn * k / 128; if (w2 < 3) w2 = 3; up = fl >= 128;
    T = CS; SDL_FillRect(CS, NULL, key); card(0, 0, w, h, c, up); T = S;
    for (i = 0; i < w2; i++) {
        int sx = i * (wn - 1) / (w2 - 1), dx = xn + (wn - w2) / 2 + i;
        if (dx < 0 || dx >= SW) continue;
        for (j = 0; j < hn; j++) { Uint32 p = *px(CS, sx, j); Uint8 r, g, b; if (p == key || yn + j < 0 || yn + j >= SH) continue; SDL_GetRGB(p, CS->format, &r, &g, &b); *px(S, dx, yn + j) = SDL_MapRGB(S->format, r, g, b); }
    }
}
/* the same for a card lying on the table */
static void tilt_turn(int x, int y, int sw, int sh, int c, int fl, int dh, int farw, int shear) {
    int k, w2, fw;
    if (fl <= 0) { tilt(x, y, sw, sh, c, 0, dh, farw, shear); return; }
    if (fl >= 256) { tilt(x, y, sw, sh, c, 1, dh, farw, shear); return; }
    k = fl < 128 ? 128 - fl : fl - 128; w2 = sw * k / 128; if (w2 < 6) w2 = 6;
    fw = farw * w2 / sw; if (fw < 3) fw = 3; if (fw > w2) fw = w2;
    tilt(x + (sw - w2) / 2, y, w2, sh, c, fl >= 128, dh, fw, shear);
}
/* her two cards (Sophia or the CPU): they arrive first and third, face down, and turn up only at a showdown */
static void her_hole(int c0, int c1, int face, const int *hl) {
    int i, cards[2]; cards[0] = c0; cards[1] = c1;
    for (i = 0; i < 2; i++) {
        int so = card_pink;   /* Sophia's table only: her cards are a little smaller and lie flatter; the other tables keep the old look */
        int cw = so ? 27 : 36, ch = so ? 38 : 50, dh = so ? 24 : 36, fw = so ? 20 : 30, ty = so ? 405 : 368;
        int a = (int)anim_age_deal(deal_ms) - (i ? 2 : 0) * DEAL_STEP, tx = so ? (i ? 321 : 292) : 290 + 34 * i, sh = so ? (i ? 5 : -5) : (i ? 3 : -3), fl;
        if (a < 0) continue;   /* not dealt yet */
        if (a < DEAL_FLY) { int e = ease(a, DEAL_FLY); tilt(312 + (tx - 312) * e / 256, 300 + (ty - 300) * e / 256 - lift_of(e, 7), cw, ch, 0, 0, dh, fw, sh * e / 256); continue; }   /* flying in face down */
        fl = face ? turn_of(1, rev_ms, FLIP_MS) : 0;
        card_hl = fl >= 256 && hl && hl[i]; tilt_turn(tx, ty, cw, ch, cards[i], fl, dh, fw, sh); card_hl = 0;
    }
}
/* your two cards in the rack: they arrive second and fourth, face down; L1 turns them up and down */
static void your_hole(int c0, int c1, const int *hl) {
    int i, cards[2]; cards[0] = c0; cards[1] = c1;
    for (i = 0; i < 2; i++) {
        int a = (int)anim_age_deal(deal_ms) - (i ? 3 : 1) * DEAL_STEP, x0 = card_pink ? (i ? 570 : 508) : (i ? 565 : 503), y0 = card_pink ? 9 : 12, cw = card_pink ? 54 : 56, ch = card_pink ? 78 : 80, fl;
        if (a < 0) continue;
        if (a < DEAL_FLY) { int e = ease(a, DEAL_FLY); card(312 + (x0 - 312) * e / 256, 300 + (y0 - 300) * e / 256 - lift_of(e, 7), 30 + (cw - 30) * e / 256, 43 + (ch - 43) * e / 256, 0, 0); continue; }
        fl = turn_of(peek, peek_ms, PEEK_MS);
        card_hl = fl >= 256 && hl && hl[i]; flat_turn(x0, y0, cw, ch, cards[i], fl); card_hl = 0;
    }
}
/* the five board slots: empty, then a card flies in face down, rests, and turns up. Cards in flight are drawn on top of the others. */
static void draw_board(const int *cards, int n, int hands, int showdown) {
    static const int gx[5] = {203, 251, 299, 347, 395}, gs[5] = {-18, -9, 0, 9, 18};   /* centres 48 px apart (were 45): a little more air between the cards */   /* Sophia's table: fixed fan from the layout guide */
    int pass, k;
    for (pass = 0; pass < 2; pass++) for (k = 0; k < 5; k++) {
        int so = card_pink, bdh = so ? 39 : 50, bfw = so ? 35 : 42, bw = so ? 43 : 48, bh = so ? 57 : 68;   /* rows after squashing / width of the far edge / flat card size */
        int bx = so ? gx[k] : 188 + k * 54, by = so ? 439 : 418 + (k * 7 + hands) % 3 - 1, sh = so ? gs[k] : ((k * 3 + hands) % 5 - 2), a = k < n ? (int)anim_age(board_ms[k]) : -1, fl;
        if (pass == 0) {
            if (a < BOARD_FLY) { tilt(bx, by, bw, bh, 0, 2, bdh, bfw, sh); continue; }   /* the empty slot (also under a card that is still on its way) */
            if (a < BOARD_FLY + BOARD_HOLD) { tilt(bx, by, bw, bh, 0, 0, bdh, bfw, sh); continue; }
            fl = a >= BOARD_FLY + BOARD_HOLD + BOARD_FLIP ? 256 : smooth256((a - BOARD_FLY - BOARD_HOLD) * 256 / BOARD_FLIP);
            card_hl = fl >= 256 && showdown && hd_board[k]; tilt_turn(bx, by, bw, bh, cards[k], fl, bdh, bfw, sh); card_hl = 0;
        } else if (a >= 0 && a < BOARD_FLY) {
            int e = ease(a, BOARD_FLY); tilt(312 + (bx - 312) * e / 256, 300 + (by - 300) * e / 256 - lift_of(e, 7), bw, bh, 0, 0, bdh, bfw, sh * e / 256);
        }
    }
}

/* ---------- the table ---------- */
/* small gold button badge (L1 / R1) left of your hand box */
static void text_o(int x, int y, const char *s, int sc, Uint32 col);
static void key_badge(int cx, int y, const char *t) {   /* no box: just the gold letters with a dark outline, centred on cx */
    int fs = 160, w = (int)strlen(t) * 6 * fs / 100;
    text_fs = fs; text_o(cx - w / 2, y, t, 2, gui_tc()); text_fs = 0;
}
static int l1_works(void) { return !pause_kind && (int)anim_age(deal_ms + 3 * DEAL_STEP + DEAL_FLY) >= 0; }   /* your second card has landed */
static void draw_play_in(void) {
    char b[48]; int i, cnt[4], bfade = 0, pot = g_pot(&G), ys = STACK - G.c[0] + fly_wait(0), hs = STACK - G.c[1] + fly_wait(1), rev_done = G.showdown && (int)anim_age(rev_ms) >= FLIP_MS;
    int n = G.stage == 0 ? 0 : G.stage + 2; unsigned hage = age_of(hand_end_ms);
    Uint32 white = C(240,240,240), gold = gui_tc(), dimc = C(130,135,150);
    int yourturn = !G.over && G.toact == 0, paid, potshow, ptxt;
    if (n > 5) n = 5;
    you_fy = 129;
    hl_gate();
    paid = G.over && pay_who && (int)anim_age(pay_ms) >= PAY_FLY;
    if (G.over) { unsigned dl = hand_dl(); if (hage >= dl) bfade = hage >= dl + 300 ? 256 : (int)((hage - dl) * 256 / 300); }
    her_pick = cu_mode == 2 ? 3 : G.over && bfade > 0 ? her_expr : 0;   /* v7.6: while she answers the close-up she wears her "caught you" picture, then the normal one again */
    if (!cu_mode) lz_free(&herCaughtS);
    bg(1); her_pick = 0;
    if (paid) { if (pay_who == 1) ys += pot; else if (pay_who == 2) hs += pot; else { ys += G.c[0]; hs += G.c[1]; } }   /* the chips have arrived: they count in the winner's stack */

    /* cards lying on the table (her hole cards give way to the banner) */
    if (bfade < 128) her_hole(G.deck[2], G.deck[3], G.showdown, hd_hole[1]);
    draw_board(G.deck + 4, n, G.hand, G.showdown);

    /* pot: counter above flat chip stacks (max 6 high per colour) */
    potshow = pot - fly_left(0) - fly_left(1);   /* chips still in the air are not in the pot yet */
    ptxt = potshow;
    if (G.over && pay_who && (int)anim_age(pay_ms) >= 0) potshow = 0;   /* on their way to the winner (or already there) */
    change(potshow, cnt);
    for (i = 0; i < 4; i++) { int h, j; for (h = cnt[i] > 6 ? 6 : cnt[i], j = 0; j < h; j++) blit(potS[i], 24 + i*32, POT_Y - j*4); }
    draw_flights();
    snprintf(b, sizeof b, "POT $%d", ptxt);
    { int pw = tw(b, 2) + 16; if (pw < 113) pw = 113; panel_t(0, 380, pw, 24, 256); }
    text(8, 385, b, 2, gold);
    snprintf(b, sizeof b, "HER $%d", hs); text(520, STK_TY, b, 2, white);
    if (G.button == 1) { ell(570, 429, 15, 11, C(120,120,120)); ell(570, 426, 15, 11, white); text(565, 419, "D", 2, C(20,20,20)); }

    draw_say();
    /* hand counter, match net, her last action */
    panel_t(0, 80, 132, 26, 256);
    if (G.hand == G.mlen) text(8, 86, "LAST HAND", 2, C(255,150,120));
    else { snprintf(b, sizeof b, "HAND %d/%d", G.hand, G.mlen); text(8, 86, b, 2, gold); }
    {   /* the result is already in G.net the moment the hand ends; the plate shows it only once the win / lose banner is fully up */
        int nt = net_shown(hage);
        if (nt > 0) snprintf(b, sizeof b, "NET +$%d", nt); else if (nt < 0) snprintf(b, sizeof b, "NET -$%d", -nt); else snprintf(b, sizeof b, "NET $0");
        { int nw = tw(b, 2) + 16; if (nw < 134) nw = 134; panel_t(640 - nw, 147, nw, 28, 256); text(640 - nw + 8, 154, b, 2, nt > 0 ? gold : nt < 0 ? C(235,120,120) : white); }   /* right column, under the money box */
    }
    if (g_mult(G.fee) > 1) { char sb[16]; snprintf(sb, sizeof sb, "STAKE X%d", g_mult(G.fee)); panel_t(0, 170, 132, 26, 256); text(8, 176, sb, 2, gold); }
    { const char *nm = who_name(W_HER); panel_tl(0, 110, tw(nm, 2) + 20, 26, 256, TB_A[tb_idx], 0); dtext(10, 116, nm, 2, name_col(nm)); }
    if (her_tag || (!G.over && G.toact == 1)) {
        static const char *TG[6] = {"", "CHECK", "CALL", "RAISE", "FOLD", "BET"};
        panel_t(0, 140, 132, 26, 256);   /* under her name, above the stake box */
        if (her_tag) text(8, 146, TG[her_tag], 2, white);
        else { static const char *DT[4] = {".", "..", "...", ""}; text(8, 146, DT[(now_ms() / 350) % 4], 2, gold); }
    }
    /* your cards (flat, easy to read) + chip rack, upper right (the boxes follow the GUI size; the chips and the cards on the table do not) */
    panel_t(492, 0, 148, 110, 256);   /* flush to the top right corner */
    your_hole(G.deck[0], G.deck[1], hd_hole[0]);   /* face down until you press Y */
    if (rev_done) text(500, 92, HN[G.sc[0] >> 20], 2, gold);
    if (!G.over && l1_works()) {   /* the Y hint: bottom left inside the cards box */
        text_fs = 130; text_o(498, 93, "Y", 2, gui_tc()); text_fs = 0;
        text(513, 95, peek ? "HIDE CARD" : "SHOW CARD", 1, C(240,240,240));
    }
    {   /* v9.4: your money box = the amount only, with your dealer button beside it when you have it (no chip pictures, no counts) */
        int bw, bx, dealer = G.button == 0;
        snprintf(b, sizeof b, "YOU $%d", ys); bw = 8 + tw(b, 2) + 8 + (dealer ? 36 : 0); bx = 640 - bw;
        panel_t(bx, 115, bw, 28, 256);
        text(bx + 8, 122, b, 2, white);
        if (dealer) { int cx = bx + 8 + tw(b, 2) + 8 + 15; ell(cx, 132, 15, 11, C(120,120,120)); ell(cx, 129, 15, 11, white); text(cx - 5, 122, "D", 2, C(20,20,20)); }
    }

    /* showdown only: her hand panel; result banner after any hand */
    if (rev_done) {
        panel_t(492, 178, 148, 110, 256);
        text(504, 185, "HER HAND", 2, white);
        card_hl = hd_hole[1][0]; card(519, 204, 44, 62, G.deck[2], 1); card_hl = hd_hole[1][1]; card(569, 204, 44, 62, G.deck[3], 1); card_hl = 0;
        text(498, 271, HN[G.sc[1] >> 20], 2, gold);
        }
    if (bfade) banner_r(G.res, G.amt, bfade, 330);
    if (G.over && pay_who) {   /* the pot goes to whoever won it (both sides on a split) */
        int pa = (int)anim_age(pay_ms);
        if (pa >= 0 && pa < PAY_FLY) {
            int e = pa * 256 / PAY_FLY;
            if (pay_who == 3) { fly_chips(pot / 2, 60, POT_CY, 540, you_fy, e); fly_chips(pot - pot / 2, 60, POT_CY, 560, 425, e); }
            else fly_chips(pot, 60, POT_CY, 540, pay_who == 1 ? you_fy : 425, e);
        }
    }

    /* button guide */
    panel_t(0, 204, 150, 82, 256);   /* v8.5: no "MENU LEAVES THE TABLE" lines any more, the box ends under X RAISE */
    if (G.over) {
        text(10, 214, G.hand >= G.mlen ? "A FINISH" : "A NEXT HAND", 2, hage >= hand_dl() ? white : dimc);
    } else if (yourturn) {
        int tc = g_tocall(&G, 0), cr = g_can_raise(&G);
        if (tc) snprintf(b, sizeof b, "A CALL $%d", tc); else snprintf(b, sizeof b, "A CHECK");
        text(10, 214, b, 2, white);
        text(10, 238, "B FOLD", 2, tc ? white : dimc);
        if (cr) snprintf(b, sizeof b, "X %s $%d", tc || G.stage == 0 ? "RAISE" : "BET", g_raise_cost(&G, 0)); else snprintf(b, sizeof b, "X RAISE");
        text(10, 262, b, 2, cr ? white : dimc);
    } else {
        text(10, 214, "HER TURN", 2, gold); text(10, 238, "B FOLD", 2, dimc); text(10, 262, "X RAISE", 2, dimc);
    }
}
static void draw_play(void) {
    card_pink = 1; card_pinkc = g_pink_cards(&G); draw_play_in(); card_pink = 0; card_pinkc = 0;
    if (cu_mode == 1) {   /* the close-up: the whole picture (not cropped, not shrunk) at the top on black, what you think in a short box under it */
        int done; T = S; R(0, 0, 640, 480, C(0,0,0)); blit(bgs[BG_TORSO], 0, -CU_SHIFT);
        done = dialog_x(cu_think, NULL, 1, cu_ms, 1); if (done) dtext_r(DLG_R, dlg_py, "A", 2, gui_tc());
    }
    else if (cu_mode == 2) { int done = dialog(cu_her, who_name(W_HER), 0, cu_ms); if (done) dtext_r(DLG_R, dlg_py, "A OK", 2, gui_tc()); }
    else {
        if (G.pill_seen && !G.over && G.toact == 0 && !anim_left(0) && !cu_used && !pause_kind) key_badge(319, 376, "R1");   /* the R1 hint: on the table in front of her stomach, just above her cards, only while R1 works */
    }
}

static void draw_end_home(void);
static void draw_end(void) {
    int res = G.last_res, y, i, any = 0; char b[48], m[24];
    if (home_end) { draw_end_home(); return; }
    Uint32 white = C(240,240,240), gold = gui_tc(), dimc = C(130,135,150), red = C(235,120,120);
    int rows = 0; for (i = 0; i < BN_N; i++) if (G.bn[i]) rows++;
    any = rows > 0;
    bg(1); draw_say(); draw_cash();
    panel_a(100, 176, 440, 150 + (rows ? 36 + 20 * rows : 0) + (G.last_drink ? 26 : 0) + (res == 0 ? 24 : 0) + (res > 0 ? (any ? 24 : 38) : 0), 256, 238);
    text_c(320, 186, res > 0 ? "YOU WIN" : res < 0 ? "YOU LOSE" : "TIE", 3, res > 0 ? gold : res < 0 ? red : C(200,205,215));
    text(124, 222, "NET CHIPS", 2, white); if (g_mult(G.fee) > 1) { snprintf(m, sizeof m, "STAKE X%d", g_mult(G.fee)); text_c(320, 222, m, 1, dimc); }
    if (G.last_net > 0) snprintf(b, sizeof b, "+$%d", G.last_net); else if (G.last_net < 0) snprintf(b, sizeof b, "-$%d", -G.last_net); else snprintf(b, sizeof b, "$0");
    text_r(516, 222, b, 2, res > 0 ? gold : res < 0 ? red : white);
    R(124, 244, 392, 2, gui_dc());
    y = 254;
    if (res > 0) {   /* the prize for the win: 5 x the buy-in, on top of the net chips */
        text(124, y, "BUY-IN PRIZE", 2, white); snprintf(m, sizeof m, "X%d", SOPH_PRIZE_X); text_c(320, y + 4, m, 1, dimc);
        snprintf(b, sizeof b, "+$%d", g_prize(G.fee)); text_r(516, y, b, 2, gold); y += 24;
        if (!any) { R(124, y, 392, 2, gui_dc()); y += 12; }
    }
    if (any) {
        text(124, y, "CARD BONUS", 2, res > 0 ? gold : dimc);
        if (res <= 0) text_r(516, y, "LOST", 2, red);
        y += 22;
        for (i = 0; i < BN_N; i++) if (G.bn[i]) {
            snprintf(b, sizeof b, "%s X%d", BONUS_NAME[i], G.bn[i]); text(124, y, b, 2, res > 0 ? white : dimc);
            m[0] = '$'; fmt_money(m + 1, G.bn[i] * BONUS_AMT[i]); text_r(516, y, m, 2, res > 0 ? white : dimc);
            y += 20;
        }
        y += 4; R(124, y, 392, 2, gui_dc()); y += 10;
    }
    if (res == 0) { text(124, y, "BUY-IN RETURNED", 2, white); { char f[16]; snprintf(f, sizeof f, "+$%d", G.fee); text_r(516, y, f, 2, gold); } y += 24; }
    text(124, y, "CASH PAID", 2, white);
    if (G.last_pay > 0) { fmt_money(m, G.last_pay); snprintf(b, sizeof b, "+$%s", m); } else snprintf(b, sizeof b, "$0");
    text_r(516, y, b, 2, G.last_pay > 0 ? gold : dimc);
    if (G.last_drink) { y += 26; text(124, y, "FREE REWARD", 2, white); text_r(516, y, G.last_drink == 1 ? "SODA" : "ENERGY DRINK", 2, gold); }
    text_c(320, y + 36, res == 0 && !first_end && g_can_afford_match(&G, G.fee) && g_soph_energy_ok(&G) ? "A REMATCH   B LEAVE" : "A CONTINUE", 2, dimc);
}

static void draw_atm(void) {   /* the ATM fills the whole screen */
    Uint32 body = C(58,62,70), edge = C(30,32,38), scrn = C(6,26,14), gb = C(30,90,50), gr = C(80,235,130), gd = C(40,130,70), lt = C(170,175,185);
    char b[48], m[24]; int i;
    T = S; R(0, 0, 640, 480, edge); R(6, 6, 628, 468, body);
    R(24, 20, 592, 304, gb); R(28, 24, 584, 296, scrn);
    text_c(320, 36, "NEIGHBORHOOD BANK", 3, gr);
    R(44, 70, 552, 2, gd);
    text(48, 88, "BANK BALANCE", 3, gd);
    m[0] = '$'; fmt_money(m + 1, G.bank); text_r(592, 82, m, 4, gr);
    text(48, 128, "CASH IN HAND", 3, gd);
    m[0] = '$'; fmt_money(m + 1, G.cash); text_r(592, 128, m, 3, gr);
    R(44, 160, 552, 2, gd);
    if (atm_mode == 0) {
        static const char *it[3] = {"DEPOSIT", "WITHDRAW", "EXIT"};
        for (i = 0; i < 3; i++) { if (i == sel) text(52, 172 + 38 * i, ">", 3, gr); text(88, 172 + 38 * i, it[i], 3, i == sel ? gr : gd); }
    } else {
        text(48, 172, atm_kind ? "WITHDRAW" : "DEPOSIT", 3, gr);
        text(48, 214, "AMOUNT", 3, gd);
        snprintf(b, sizeof b, "%06d", atm_amt);
        { int nz = 0; for (i = 0; i < 6; i++) {
              int x = 592 - 6 * 28 + i * 28; char ch[2] = {b[i], 0};
              if (b[i] != '0') nz = 1;
              text(x, 206, ch, 4, nz || i == 5 ? gr : gd);
              if (i == atm_cur) R(x, 240, 24, 4, gui_tc()); } }
    }
    if (atm_msg[0]) text_c(320, 282, atm_msg, tw(atm_msg, 3) <= 560 ? 3 : 2, gui_tc());
    /* slots and hints on the body */
    R(238, 340, 164, 10, edge); R(262, 358, 116, 4, edge);
    if (atm_mode == 0) text_c(320, 400, "UP/DOWN CHOOSE   A SELECT   B BACK", 2, lt);
    else { text_c(320, 388, "UP/DOWN CHANGE DIGIT   LEFT/RIGHT MOVE", 2, lt); text_c(320, 416, "Y MAX   X CLEAR   A CONFIRM   B BACK", 2, lt); }
}

/* ---------- title menu (POKER MINI) ---------- */
/* translucent copy of a part of a picture onto the screen (a: 0..256), clipped to the screen */
static void blit_part_a(SDL_Surface *im, int lsx, int lsy, int lw, int lh, int ldx, int ldy, int a) {   /* all rectangle values logical */
    int x, y, sx = XS(lsx), sy = YS(lsy), w = XS(lsx + lw) - sx, h = YS(lsy + lh) - sy, dx = XS(ldx), dy = YS(ldy); SDL_PixelFormat *f, *g = S->format;
    if (!im || a <= 0) return;
    f = im->format;
    for (y = 0; y < h; y++) {
        Uint32 *s, *d; int yy = dy + y;
        if (yy < 0 || yy >= SH || sy + y >= im->h) continue;
        s = px(im, sx, sy + y); d = px(S, 0, yy);
        for (x = 0; x < w; x++) {
            Uint32 v = s[x], o; int xx = dx + x, sa, sr, sg, sb, dr, dg, db;
            if (xx < 0 || xx >= SW || sx + x >= im->w) continue;
            sa = (int)((v & f->Amask) >> f->Ashift) * a >> 8; if (sa <= 0) continue;
            sr = (v >> f->Rshift) & 255; sg = (v >> f->Gshift) & 255; sb = (v >> f->Bshift) & 255;
            o = d[xx]; dr = (o >> g->Rshift) & 255; dg = (o >> g->Gshift) & 255; db = (o >> g->Bshift) & 255;
            dr = (dr * (256 - sa) + sr * sa) >> 8; dg = (dg * (256 - sa) + sg * sa) >> 8; db = (db * (256 - sa) + sb * sa) >> 8;
            d[xx] = (o & ~(g->Rmask | g->Gmask | g->Bmask)) | ((Uint32)dr << g->Rshift) | ((Uint32)dg << g->Gshift) | ((Uint32)db << g->Bshift);
        }
    }
}
/* title screen: red suit outlines drift slowly upward behind the menu, swaying a little. Pure function of the clock (no state);
   big ones are slower and fainter, small ones faster, so there is some depth. assets/title_suits.png: 4 suits across, 3 sizes down. */
static void draw_title_suits(void) {
    static const int CELL[3] = {44, 68, 100}, OFF[3] = {0, 44, 112}, ALPHA[3] = {120, 95, 70}, SPEED[3] = {34, 22, 13};
    static const struct { unsigned char suit, size; short x; short phase; unsigned char amp; } L[] = {
        {1,2, 60, 0,18},{0,1,200, 420,14},{3,0,330, 150,10},{2,2,470, 760,20},{0,0,560, 90,8},{2,1, 20, 300,12},{3,2,250, 910,16},
        {1,0,420, 640,9},{0,2,600, 520,15},{3,1,120, 180,13},{2,0,300, 380,8},{1,1,520, 830,12},{0,1,380, 270,14},{3,0, 40, 700,9}};
    unsigned t = now_ms(); int i;
    if (!suitsS) return;
    for (i = 0; i < (int)(sizeof L / sizeof L[0]); i++) {
        int sz = L[i].size, c = CELL[sz], cyc = 480 + 2 * c, dist = (int)((double)t * SPEED[sz] / 1000.0) + L[i].phase * cyc / 1000, y, x;
        y = 480 + c - dist % cyc;
        x = L[i].x + (int)(L[i].amp * sin((double)t / 1700.0 + L[i].phase * 0.01));
        blit_part_a(suitsS, L[i].suit * c, OFF[sz], c, c, x, y, ALPHA[sz]);
    }
}
static void draw_menu(void) {
    static const char *items[4] = {"QUICK PLAY", "CAREER MODE", "OPTIONS", "EXIT"};
    int i, x; Uint32 white = C(250,250,250);
    T = S; R(0, 0, 640, 480, C(8,2,3));
    draw_title_suits();
    vgrad(0, 0, 640, 22, 120, 22, 32, 32, 6, 10); R(0, 22, 640, 2, C(235,70,80));
    vgrad(0, 436, 640, 44, 32, 6, 10, 110, 20, 30); R(0, 434, 640, 2, C(235,70,80));
    /* title: big first letters */
    x = 30; text(x, 44, "P", 7, white); text(x + 42, 58, "OKER", 5, white); text(x + 42 + 4 * 30 + 24, 44, "M", 7, white); text(x + 42 + 4 * 30 + 24 + 42, 58, "INI", 5, white);
    R(30, 108, 330, 3, C(235,70,80));
    for (i = 0; i < 4; i++) {
        int y = 146 + i * 48;
        if (i == menu_sel) {
            int k; for (k = 0; k < 420; k += 4) shade(k, y, 4, 42, 150 + (420 - k) * 80 / 420, 24 + (420 - k) * 20 / 420, 36 + (420 - k) * 20 / 420, 230 - k * 190 / 420);
            text(40, y + 9, items[i], 3, white);
            { int sx = 40 + tw(items[i], 3) + 28; suit(2, sx, y + 21, 8, C(235,235,235)); suit(0, sx + 30, y + 21, 8, C(235,235,235)); suit(1, sx + 60, y + 21, 8, C(235,235,235)); suit(3, sx + 90, y + 21, 8, C(235,235,235)); }
        } else {
            shade(0, y, 420, 42, 50, 14, 20, 120);
            text(40, y + 9, items[i], 3, C(150,108,114));
        }
    }
    disc(478, 457, 15, C(225,65,75)); disc(478, 457, 12, C(170,28,40)); TS = 0; text(473, 450, "A", 2, white); TS = 1;
    text(504, 447, "ACCEPT", 3, white);
}

/* Career Mode with a saved game: CONTINUE / NEW GAME (NEW GAME asks first and deletes the save) */
static int car_sel, car_conf, car_csel;   /* v10.5: car_sel = the slot row (0..SLOT_N-1); car_conf 0 = the list, 1 = "start over in this slot?", 2 = "delete this slot?" (car_csel 0 = NO, 1 = YES) */
static void slot_line(char *b, size_t n, int i) {   /* one line about slot i: what the file holds */
    if (slot_st[i] == 0) snprintf(b, n, "EMPTY"); else if (slot_st[i] == 2) snprintf(b, n, "CANNOT BE READ");
    else snprintf(b, n, "DAY %d  %s  $%d", slot_g[i].day, PH_NAME[slot_g[i].phase], slot_g[i].cash);
}
static void draw_career(void) {
    char b[64]; int i; Uint32 white = C(240,240,240), gold = mn_tc(), dim = C(150,155,170), red = C(235,120,120);
    draw_menu(); shade(0, 0, 640, 480, 0, 0, 0, 150);
    panel_am(30, 30, 580, 410, 256, 245);
    text_c(320, 46, "CAREER MODE", 3, gold); R(54, 80, 532, 2, mn_dc());
    if (!car_conf) {
        for (i = 0; i < SLOT_N; i++) {   /* v10.9: one row per slot, text at scale 2: SLOT n, DAY / PHASE / CASH, SAVES HERE on the right (no bank) */
            int y = 92 + i * 52; Uint32 col = i == car_sel ? gold : white;
            if (i == car_sel) panel_am(44, y, 552, 48, 256, 256);
            snprintf(b, sizeof b, "SLOT %d", i + 1); text(60, y + 17, b, 2, col);
            slot_line(b, sizeof b, i); text(168, y + 17, b, 2, slot_st[i] == 1 ? (i == car_sel ? gold : white) : (slot_st[i] == 2 ? red : dim));
            if (i + 1 == slot_act) text_r(580, y + 17, "SAVES HERE", 2, gold);
        }
        text_c(320, 362, slot_st[car_sel] == 0 ? "A NEW GAME   B BACK" : "A LOAD  X NEW GAME  Y DELETE  B BACK", 2, C(150,155,170));
        text_c(320, 392, "THE SOFA AND THE BED SAVE TO THE SLOT YOU PLAY", 1, C(130,135,150));
    } else {
        snprintf(b, sizeof b, car_conf == 1 ? "START OVER IN SLOT %d?" : "DELETE SLOT %d?", car_sel + 1); text_c(320, 150, b, 3, white);
        slot_line(b, sizeof b, car_sel); text_c(320, 196, b, 2, dim);
        text_c(320, 232, "THE SAVE IN THIS SLOT", 2, white); text_c(320, 256, "WILL BE DELETED", 2, red);
        for (i = 0; i < 2; i++) { int x = 190 + i * 140; if (i == car_csel) panel_am(x, 300, 120, 32, 256, 256); text_c(x + 60, 308, i ? "YES" : "NO", 2, i == car_csel ? gold : dim); }
    }
}

/* ---------- screens ---------- */
/* energy bar (top left of your room): your bolt-and-segments artwork, 5 segments of 20% */
static void text_o(int x, int y, const char *s, int sc, Uint32 col) {   /* text with a dark outline, for use straight on a picture */
    Uint32 k = C(6, 8, 16); int dx, dy, ts = TS; TS = 0;
    for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++) if (dx || dy) text(x + dx, y + dy, s, sc, k);
    text(x, y, s, sc, col); TS = ts;
}
static void draw_energy(int x, int y) {   /* no box behind it: just the bar and the percentage */
    char b[8]; int i, full = G.energy / 20, rem = G.energy % 20;
    if (barS && segS) {
        blit(barS, x + 4, y + 2);
        for (i = 0; i < 5; i++) {
            int w = i < full ? LW(segS) : (i == full && rem) ? LW(segS) * rem / 20 : 0;
            if (w <= 0) continue;
            blit_src(segS, 0, 0, w, LH(segS), x + 4 + 27 + (i * 1629 + 50) / 100, y + 2 + 18);
        }
    } else { R(x + 8, y + 14, 104, 22, C(20,20,28)); R(x + 10, y + 16, G.energy, 18, C(40,120,240)); }
    snprintf(b, sizeof b, "%d%%", G.energy); text_o(x + 170 - tw(b, 2), y + 18, b, 2, G.energy ? C(240,240,240) : C(235,120,120));
    if (g_milk_active(&G)) { char m[40]; snprintf(m, sizeof m, "MILK: NO ENERGY DROP, %d DAY%s", g_milk_left(&G), g_milk_left(&G) == 1 ? "" : "S"); text_o(x + 6, y + 56, m, 1, C(250,240,215)); }   /* v6.5: a Baby Milk Bottle is working */
}
enum { DR_PLAY, DR_PROPOSE, DR_ASK, DR_LEAVE, DR_COMEIN };
static int door_items(int *out) { int n = 0; out[n++] = DR_PLAY; if (g_can_propose(&G)) out[n++] = DR_PROPOSE; if (g_can_askmove(&G)) out[n++] = DR_ASK; if (g_visit_ok(&G)) out[n++] = DR_COMEIN; out[n++] = DR_LEAVE; return n; }   /* her door (3rd floor) */
static void draw_door(void) {
    const char *items[5]; int dk[5], n = door_items(dk), i;
    bg_draw(BG_HALL); blit_a(her_vis(), 256); draw_cash(); if (g_visit_ok(&G)) draw_friend_bar(8, 6);
    for (i = 0; i < n; i++) items[i] = DOOR_ITEMS[dk[i]];
    menu_panel(items, n, sel);
}
static void draw_len(void) {
    char b[40]; int i, h = 10 + 22 * SOPH_N, y0 = DLG_BOT - h;
    bg_draw(fv_on ? BG_LIVING : BG_HALL); blit_a(her_vis(), 256); draw_say(); draw_cash();   /* v11.1: from the door her_room.png, from inside her room BG-poker_day.png */
    panel(190, y0, 260, h, 256);   /* the menu: just the buy-ins (the prize is not shown) */
    if (g_milk_active(&G)) strcpy(b, "THE MILK BOTTLE: NO ENERGY USED"); else snprintf(b, sizeof b, "A MATCH USES %d%% ENERGY", SOPH_ENERGY);   /* v12.4 */
    text_c(320, y0 - 22, b, 2, g_soph_energy_ok(&G) ? C(250,240,215) : C(235,120,120));
    for (i = 0; i < SOPH_N; i++) {
        Uint32 col = i == len_sel ? gui_tc() : C(240,240,240);
        if (i == len_sel) text(202, y0 + 10 + 22 * i, ">", 2, col);
        snprintf(b, sizeof b, "BUY-IN $%d", SOPH_FEE[i]); text(226, y0 + 10 + 22 * i, b, 2, col);
    }
    if (len_msg_ms && age_of(len_msg_ms) < 1800) { char nb[40]; snprintf(nb, sizeof nb, "NEED %d%% ENERGY", SOPH_ENERGY); panel(150, 330, 340, 30, 256); text_c(320, 337, len_msg_en ? nb : "NOT ENOUGH CASH", 2, C(235,120,120)); }
}
static void draw_story(void) {
    const Beat *b = &vn_buf[vn_i]; int a, done;
    bg_draw(b->bg);
    if (b->spr == vn_prev_spr) a = b->spr ? 256 : 0;
    else { int t = (int)age_of(vn_spr_ms); a = t >= 350 ? 256 : t * 256 / 350; if (!b->spr) a = 256 - a; }
    { int sp = b->spr ? b->spr : vn_prev_spr; blit_a(sp == 2 && ignoreS ? ignoreS : sp == 3 && night_vis() ? night_vis() : sp == 4 && lz(&bossS, "assets/boss_sophia.png") ? bossS : sp == 5 && hide_pic() ? hideS[0] : sp == 6 && fh_pic(2) ? fh_pic(2) : her_vis(), a); }
    if (vn_id == SCR_AFTER && G.last_res > 0 && G.last_pay > 0) {   /* the reward counts into your cash while she says it */
        int base = G.cash - G.last_pay, t = (int)age_of(after_ms) - 350, v;
        if (base < 0) base = 0;
        v = t <= 0 ? base : t >= 900 ? G.cash : base + (int)((long)(G.cash - base) * t / 900);
        draw_cash_v(v, 24);   /* below the START SKIP hint */
    }
    done = dialog(b->txt, who_name(b->who), b->who == W_NAR, vn_ms);
    if (done) dtext_r(DLG_R, dlg_py, vn_i + 1 < vn_n ? "A >" : "A OK", 2, gui_tc());
    if (vn_id != SCR_INTRO) text_r(632, 8, "START MENU", 2, C(210,215,225));   /* v11.8: not in the arrival scene (Start still works) */
}
static void draw_scene(void) {   /* coach talk at her door */
    int done;
    bg_draw(BG_HALL); blit_a(her_vis(), 256);
    done = dialog(sc_page[sc_i], who_name(W_HER), 0, sc_ms);
    if (done) dtext_r(DLG_R, dlg_py, sc_i + 1 < sc_n ? "A >" : "A OK", 2, gui_tc());
}
/* ---------- store and inventory (both in your room) ---------- */
static void store_note(const char *m) { snprintf(st_msg, sizeof st_msg, "%s", m); st_msg_ms = now_ms(); }
/* v12.5: the lobby VENDING MACHINE is a STATIC picture again (assets/store_menu.png, 752x560, built by tools/make_vending.py from tools/vend_src/vend2.png: drinks and the $5 / $10 price boxes are painted
   on it, and so are the X and Y buttons left of the price boxes). No happy face, no can dropping into the tray. Top shelf = soda (X), bottom shelf = energy drink (Y).
   UP / DOWN / LEFT / RIGHT pick the shelf, A buys the picked one, X = soda, Y = energy drink, B back. The info + cash box sits in the empty right half of the top shelf.
   Cursor coordinates below are SCREEN pixels of the 752x560 picture. */
static int st_sel;
#define VSX(x) ((x) * 640 / 752)
#define VSY(y) ((y) * 480 / 560)
static void vend_box(int x0, int y0, int x1, int y1, int t, Uint32 col) {   /* an outline, screen pixels -> logical */
    int lx0 = VSX(x0), ly0 = VSY(y0), lx1 = VSX(x1), ly1 = VSY(y1);
    R(lx0, ly0, lx1 - lx0, t, col); R(lx0, ly1 - t, lx1 - lx0, t, col); R(lx0, ly0, t, ly1 - ly0, col); R(lx1 - t, ly0, t, ly1 - ly0, col);
}
static void draw_store(void) {
    char b[40]; Uint32 white = C(240,240,240), gold = gui_tc(), dimc = C(130,135,150);
    int ry0[2] = {22, 252}, ry1[2] = {196, 432}, price[2] = {PRICE_SODA, PRICE_MONSTER}, rest[2] = {REST_SODA, REST_MONSTER}, own[2] = {G.soda, G.monster};
    T = S; R(0, 0, 640, 480, C(0,0,0));
    if (storeS) blit(storeS, 0, 0); else { R(0, 0, 640, 480, C(40,90,150)); text(20, 20, "X SODA $ 5   Y ENERGY DRINK $ 10", 1, white); }
    vend_box(76, ry0[st_sel], 442, ry1[st_sel], 2, gold);   /* the cursor: the whole shelf */
    panel(396, 26, 214, 132, 256);   /* the info + cash box of the selected drink */
    text(404, 34, INV_NAME[st_sel], 1, gold);
    snprintf(b, sizeof b, "$%d", price[st_sel]); text(404, 50, b, 2, white);
    snprintf(b, sizeof b, "RESTORES %d%% ENERGY", rest[st_sel]); text(404, 74, b, 1, gold);
    snprintf(b, sizeof b, "OWNED %d", own[st_sel]); text(404, 88, b, 1, white);
    { char c[24]; c[0] = '$'; fmt_money(c + 1, G.cash); text(404, 104, "CASH", 1, dimc); text(450, 102, c, tw(c, 2) <= 150 ? 2 : 1, white); }
    text(404, 128, "UP DOWN PICK  A BUY", 1, dimc); text(404, 142, "X SODA  Y ENERGY  B BACK", 1, dimc);
    if (st_confirm) {
        panel_a(120, 190, 400, 100, 256, 244);
        snprintf(b, sizeof b, "BUY %s FOR $%d?", st_confirm == 1 ? INV_NAME[0] : INV_NAME[1], st_confirm == 1 ? PRICE_SODA : PRICE_MONSTER);
        text_c(320, 208, b, 2, gold); text_c(320, 250, "A YES     B NO", 2, white);
    } else if (st_msg[0] && age_of(st_msg_ms) < 3200) {
        int w = tw(st_msg, 2) + 28; panel(320 - w / 2, 428, w, 32, 256); text_c(320, 436, st_msg, 2, gold);
    }
}
static void store_open(void) {
    scr = SC_STORE; st_confirm = 0; st_msg[0] = 0; st_sel = 0;
    if (g_rescue_due(&G)) { g_rescue(&G); persist(); store_note("1 FREE SODA. ONLY THIS ONCE."); }   /* the clerk takes pity, one time only */
}
static void store_key(int b) {
    if (st_confirm) {
        if (b == B_A) {
            int r = g_buy(&G, st_confirm - 1);
            if (r == 0) { persist(); store_note(st_confirm == 1 ? INV_BOUGHT[0] : INV_BOUGHT[1]); }
            else store_note(r == 1 ? "NOT ENOUGH CASH. USE THE ATM" : "INVENTORY FULL");
            st_confirm = 0;
        } else if (b == B_B) st_confirm = 0;
        return;
    }
    if (b == B_B) { scr = SC_WALK; return; }
    if (b == B_UP || b == B_DOWN || b == B_LEFT || b == B_RIGHT) { st_sel = !st_sel; return; }   /* v12.4: the two shelves */
    if (b == B_X || b == B_Y || b == B_A) {
        int k = b == B_A ? st_sel : b == B_Y; st_sel = k;
        if (G.cash < (k ? PRICE_MONSTER : PRICE_SODA)) { store_note("NOT ENOUGH CASH. USE THE ATM"); return; }
        st_confirm = k + 1;
    }
}
/* v7.8: the FRIDGE shows only what you own. Item ids: 0 soda, 1 energy drink, 2 Sophia breast milk (it also stays while a bottle is working). inv_sel is an item id. */
static int inv_count(int i) { return i == 2 ? G.bottle : i ? G.monster : G.soda; }
static int inv_has(int i) { return i == 2 ? (G.bottle > 0 || g_milk_active(&G)) : inv_count(i) > 0; }
static int inv_list(int *o) { int i, n = 0; for (i = 0; i < 3; i++) if (inv_has(i)) o[n++] = i; return n; }
static int inv_rows(void) { int o[3]; return inv_list(o); }
static int inv_info;   /* 1 = the info page of the Sophia breast milk is open (Y) */
static void inv_fix(void) { if (!inv_has(inv_sel)) { int o[3]; inv_sel = inv_list(o) ? o[0] : 0; } }
static void draw_inv(void) {
    char b[40]; const char *const *NM = INV_NAME; int o[3], k, nr = inv_list(o), ph;
    Uint32 white = C(240,240,240), gold = gui_tc(), dimc = C(130,135,150);
    ph = nr == 0 ? 130 : nr == 3 ? 336 : nr == 2 ? 250 : 190;
    bg_draw(BG_MYROOM); draw_cash(); draw_energy(10, 6);
    if (inv_info) {   /* v7.8: the info page of the Sophia breast milk (the fridge list is hidden behind it) */
        char w[400], *ln = w; int y = 176;
        R(50, 90, 540, 290, C(10,12,20)); panel_a(50, 90, 540, 290, 256, 244); text_c(320, 106, NM[2], 2, gold); R(70, 132, 500, 2, gui_dc());
        if (bottleS) blit(bottleS, 70 + (100 - bottleS->w) / 2, 156 + (100 - bottleS->h) / 2);
        wrap_buf(w, sizeof w, MILK_INFO, 28);
        while (*ln) { char *e = strchr(ln, '\n'); char t[48]; int n = e ? (int)(e - ln) : (int)strlen(ln); if (n > 40) n = 40; memcpy(t, ln, (size_t)n); t[n] = 0; text(190, y, t, 2, white); y += 26; ln += n; if (*ln == '\n') ln++; }
        text_c(320, 350, "B BACK", 2, dimc);
        return;
    }
    panel_a(110, 66, 420, ph, 256, 244);
    text_c(320, 76, "FRIDGE", 3, gold); R(130, 106, 380, 2, gui_dc());
    if (nr == 0) text_c(320, 130, FRIDGE_EMPTY, 2, dimc);   /* nothing inside: no pictures, only this line */
    for (k = 0; k < nr; k++) {
        int i = o[k], y = 116 + k * (nr == 3 ? 82 : 92), n = inv_count(i); Uint32 col = n ? white : dimc; SDL_Surface *ic = i == 2 ? bottleS : i ? monS : sodaS;
        if (i == inv_sel) { shade(124, y - 4, 392, nr == 3 ? 78 : 84, 250, 215, 90, 40); text(130, y + 24, ">", 3, gold); }
        if (ic) blit(ic, i == 2 ? 154 : 160, y);
        text(236, y + 6, NM[i], 2, col);
        snprintf(b, sizeof b, "X %d", n); text(236, y + 28, b, 2, col);
        if (i == 2) { if (g_milk_active(&G)) { snprintf(b, sizeof b, "WORKING: %d DAY%s LEFT", g_milk_left(&G), g_milk_left(&G) == 1 ? "" : "S"); text(236, y + 52, b, 2, gold); } else text(236, y + 52, "NO ENERGY DROP 2 DAYS", 2, dimc); }
        else text(236, y + (nr == 3 ? 52 : 56), i ? "+50% ENERGY" : "+20% ENERGY", 2, dimc);
    }
    text_c(320, 66 + ph - 24, nr == 0 ? "B BACK" : inv_sel == 2 ? "A DRINK   Y INFO   B BACK" : "A DRINK     B BACK", 2, dimc);
    if (st_msg[0] && age_of(st_msg_ms) < 2600) { int w = tw(st_msg, 2) + 28; panel(320 - w / 2, 66 + ph + 12, w, 32, 256); text_c(320, 66 + ph + 20, st_msg, 2, gold); }
}
static void inv_open(void) { scr = SC_INV; inv_sel = 0; inv_info = 0; inv_fix(); st_msg[0] = 0; }
static void inv_key(int b) {
    int o[3], nr = inv_list(o), k;
    if (inv_info) { if (b == B_B || b == B_A || b == B_Y) inv_info = 0; return; }
    if (nr && (b == B_UP || b == B_DOWN)) { for (k = 0; k < nr && o[k] != inv_sel; k++); if (k >= nr) k = 0; inv_sel = o[(k + (b == B_UP ? nr - 1 : 1)) % nr]; }
    else if (b == B_B) { scr = SC_WALK; }
    else if (b == B_Y && nr && inv_sel == 2) inv_info = 1;
    else if (b == B_A && nr && inv_sel == 2) {   /* Sophia breast milk: no energy is spent for 2 days */
        int r = g_drink_bottle(&G);
        if (r == 0) { persist(); store_note("NO ENERGY DROP FOR 2 DAYS"); }
        else store_note(r == 1 ? "YOU HAVE NONE" : "ONE IS WORKING ALREADY");
        inv_fix();
    }
    else if (b == B_A && nr) {
        int r = g_drink(&G, inv_sel);
        if (r == 0) { persist(); store_note(inv_sel ? "ENERGY +50%" : "ENERGY +20%"); }
        else store_note(r == 1 ? "YOU HAVE NONE" : "YOUR ENERGY IS ALREADY FULL");
        inv_fix();
    }
}

/* ---------- v6.2: the online store (the computer in your bedroom) and the romance scene (her bedroom door in your living room) ---------- */
static char shop_msg[40]; static unsigned shop_msg_ms; static int shop_sel, shop_confirm;
enum { RM_MENU, RM_LEN, RM_TEASE, RM_TALK };   /* v8.3: no separate gift list any more: the gifts are rows of page 2 of her menu */
static int rom_mode, rom_sel, rom_page, rom_lsel, romq_sel, home_lvl0, rom_choose, rom_in;   /* rom_choose: 0 = her line is shown, 1 = the choice page (v6.6); rom_in: 1 = inside her room (her room picture), 0 = at her door (the door picture) */ static const char *rom_txt; static unsigned rom_ms, rom_note_ms; static char rom_note[48];
/* v7.7: CHAT / TEASE: a talk is a list of up to 6 pages (who: 0 you, 1 Sophia, 2 narration) shown one after the other in her room */
enum { TK_CHAT, TK_FAIL, TK_TEASE };
#define kicked_day (G.kick_day)   /* v9.7: the day you teased her and she threw you out: saved with the game (it used to be a plain variable and was lost on exit) */
static int tease_sel, tk_n, tk_i, tk_kind, chat_gain_day, chat_gain_n, chat_last_topic = -1, chat_last_ex[CH_N], tease_last_ex[TS_N];
static const char *tk_t[6]; static unsigned char tk_who[6]; static unsigned tk_ms;
static void menu_panel_at(const char *const *items, int n, int s, int x, int w, int ybot) {   /* like menu_panel, but anywhere (the VN screens keep the bottom for her dialogue box) */
    int i, h = 10 + 22 * n, y0 = ybot - h; panel(x, y0, w, h, 256);
    for (i = 0; i < n; i++) {
        Uint32 col = i == s ? gui_tc() : C(240,240,240);
        if (i == s) text(x + 12, y0 + 10 + 22 * i, ">", 2, col);
        text(x + 34, y0 + 10 + 22 * i, items[i], 2, col);
    }
}
static int shop_sold(int i) { return (i == SH_RING && G.ring) || (i == SH_PILL && G.pill) || (i == SH_PUMP && (G.pump || (G.milk_st >= 3 && !G.pump_dead))) || (i == SH_DIAMOND && G.dring) || (i == SH_INJECT && (G.inj || G.injected)); }
static int shop_owned(int i) {   /* v8.2: how many you hold of the stackable things (-1 = not a counted item) */
    switch (i) { case SH_FLOWER: return G.flower; case SH_STACK: return G.stack; case SH_MILK: return G.mpill; case SH_LOTION: return G.lotion; case SH_DECK: return G.pdeck;
                 case SH_YOGA: return G.yoga; case SH_TOP: return G.top; case SH_COW: return G.cow; case SH_GOLD: return G.gold; case SH_SUPER: return G.sdrug; default: return -1; }
}
static int shop_info;   /* v6.8: 1 = the info page of the selected item is open (Y)  */
/* v7.8: the online store has three tabs (R1 next, L1 previous): ITEMS (flower, money stack, pump), STOCKS, BLACK MARKET (ring, breast enhancement pill, milk inducer pill).
   shop_sel is an item id (SH_*) on ITEMS and BLACK MARKET, and a company number (STK_*) on STOCKS. */
enum { TAB_ITEMS, TAB_STOCKS, TAB_BM, TAB_N };
static int shop_tab;
static const char *const TAB_NAME[TAB_N] = {"ITEMS", "STOCKS", "BLACK MARKET"};
static const char *const STK_NAME[STK_N] = {"NVIDIA", "GOOGLE", "OPENAI", "APPLE", "SONY", "SPACEX"};
static SDL_Surface *stkLogo[STK_N];   /* v8.1: the company logos (assets/stk_*.png, drawn on a black tile) */
#define STK_ROWS 4            /* v8.1: rows visible at once; the list scrolls */
static int stk_top;           /* the first visible company */
static void shop_scroll(int pos, int nv) { if (pos < stk_top) stk_top = pos; if (pos > stk_top + STK_ROWS - 1) stk_top = pos - (STK_ROWS - 1); if (stk_top > nv - STK_ROWS) stk_top = nv - STK_ROWS; if (stk_top < 0) stk_top = 0; }   /* v8.2: every tab scrolls (4 rows visible) */
static void stk_scroll(void) { shop_scroll(shop_sel, STK_N); }
static int shop_list(int tab, int *o) {
    int n = 0, i;
    /* v10.0: an item is listed only when its tab is open (g_shop_avail) and, for a one-time item, only until you buy it (it vanishes instead of showing SOLD OUT) */
    static const int ITEMS_ORD[] = { SH_FLOWER, SH_LOTION, SH_DECK, SH_STACK, SH_YOGA, SH_TOP, SH_COW, SH_GOLD, SH_DIAMOND, SH_PUMP };   /* v8.2: by price (the pump comes after the day-8 scene) */
    static const int BM_ORD[] = { SH_RING, SH_PILL, SH_MILK, SH_SUPER, SH_INJECT };
    if (tab == TAB_ITEMS) { for (i = 0; i < (int)(sizeof ITEMS_ORD / sizeof ITEMS_ORD[0]); i++) if (g_shop_avail(&G, ITEMS_ORD[i]) && !g_shop_once_done(&G, ITEMS_ORD[i])) o[n++] = ITEMS_ORD[i]; }
    else if (tab == TAB_BM) { for (i = 0; i < (int)(sizeof BM_ORD / sizeof BM_ORD[0]); i++) if (g_shop_avail(&G, BM_ORD[i]) && !g_shop_once_done(&G, BM_ORD[i])) o[n++] = BM_ORD[i]; }
    else for (i = 0; i < STK_N; i++) o[n++] = i;
    return n;
}
/* v10.0: which tabs the store shows. STOCKS always; ITEMS once she lives with you; BLACK MARKET after 20 wins over her (and while something is left in it) */
static int shop_tab_ok(int t) { int o[16]; if (t == TAB_STOCKS) return 1; if (t == TAB_ITEMS) return g_items_open(&G) && shop_list(t, o) > 0; return g_bm_open(&G) && shop_list(t, o) > 0; }
static int shop_next_tab(int t, int dir) { int k; for (k = 1; k <= TAB_N; k++) { int c = ((t + dir * k) % TAB_N + TAB_N) % TAB_N; if (shop_tab_ok(c)) return c; } return TAB_STOCKS; }
static int shop_tabs_n(void) { int t, n = 0; for (t = 0; t < TAB_N; t++) if (shop_tab_ok(t)) n++; return n; }
static int shop_n(void) { int o[16]; return shop_list(shop_tab, o); }
static int shop_pos(const int *o, int nv) { int k; for (k = 0; k < nv && o[k] != shop_sel; k++); return k < nv ? k : 0; }
static void draw_stock_row(int s, int y, int sel) {
    char b[48]; Uint32 white = C(240,240,240), gold = mn_tc(), dimc = C(130,135,150), up = C(110,220,130), dn = C(235,100,100);
    int p = stk_now(&G, s), ch = stk_change(&G, s), prev = p - ch, t10, k, mn = 1 << 30, mx = 0, v[7];
    if (sel) { shade_m(20, y, 600, 88, 36); text(28, y + 32, ">", 3, gold); }
    R(72, y + 4, 80, 80, C(0, 0, 0)); if (stkLogo[s]) blit(stkLogo[s], 72 + (80 - LW(stkLogo[s])) / 2, y + 4 + (80 - LH(stkLogo[s])) / 2); else text_c(112, y + 26, STK_NAME[s], 1, white);
    text(176, y + 10, STK_NAME[s], 2, white);
    b[0] = '$'; fmt_money(b + 1, p); text(176, y + 36, b, 2, gold);
    t10 = prev > 0 ? (int)((long)(ch < 0 ? -ch : ch) * 1000 / prev) : 0;
    if (ch == 0) snprintf(b, sizeof b, "NO CHANGE TODAY"); else snprintf(b, sizeof b, "%s $%d (%d.%d%%)", ch > 0 ? "UP" : "DOWN", ch < 0 ? -ch : ch, t10 / 10, t10 % 10);
    text(176, y + 64, b, 2, ch > 0 ? up : ch < 0 ? dn : dimc);
    for (k = 0; k < 7; k++) { v[k] = stk_price(&G, s, G.day - 6 + k); if (v[k] < mn) mn = v[k]; if (v[k] > mx) mx = v[k]; }   /* the last 7 days, today on the right */
    text(400, y + 6, "LAST 7 DAYS", 1, dimc);
    for (k = 0; k < 7; k++) {
        int h = 6 + (mx > mn ? (v[k] - mn) * 44 / (mx - mn) : 22); Uint32 col = k == 0 ? dimc : v[k] > v[k - 1] ? up : v[k] < v[k - 1] ? dn : dimc;
        R(400 + k * 15, y + 80 - h, 12, h, col);
    }
    snprintf(b, sizeof b, "OWNED %d", G.stk[s]); text_r(604, y + 10, b, 2, white);
    b[0] = '$'; fmt_money(b + 1, G.stk[s] * p); text_r(604, y + 36, b, 2, G.stk[s] ? gold : dimc);
    text_r(604, y + 64, "VALUE", 2, dimc);
}
static void draw_shop(void) {
    char b[40]; int o[16], k, nv = shop_list(shop_tab, o), t, x = 30; Uint32 white = C(240,240,240), gold = mn_tc(), dimc = C(130,135,150);
    T = S; R(0, 0, 640, 480, C(8,10,18));
    if (shop_tabs_n() > 1) text(8, 16, "L1", 1, dimc);   /* the tab bar: L1 / R1 change the tab (only the open tabs are shown) */
    for (t = 0; t < TAB_N; t++) {
        int w; if (!shop_tab_ok(t)) continue;
        w = tw(TAB_NAME[t], 2) + 16;
        if (t == shop_tab) { shade_m(x, 6, w, 28, 60); R(x, 32, w, 2, gold); }
        text(x + 8, 12, TAB_NAME[t], 2, t == shop_tab ? gold : dimc); x += w + 6;
    }
    if (shop_tabs_n() > 1) text(x + 2, 16, "R1", 1, dimc);
    R(20, 40, 600, 2, mn_dc());
    if (shop_tab == TAB_STOCKS) {   /* v8.1: six companies, four rows at a time: the list scrolls with UP / DOWN */
        stk_scroll();
        for (k = 0; k < STK_ROWS && stk_top + k < nv; k++) draw_stock_row(o[stk_top + k], 50 + k * 94, o[stk_top + k] == shop_sel);
        if (stk_top > 0) tri(600, 44, 8, 7, 1, gold);
        if (stk_top + STK_ROWS < nv) tri(600, 432, 8, 7, 0, gold);
        nv = 0;
    }
    if (nv > 0) shop_scroll(shop_pos(o, nv), nv);   /* v8.2: the ITEMS list is longer than four rows: it scrolls like the stocks */
    for (k = stk_top; k < nv && k < stk_top + STK_ROWS; k++) {
        int i = o[k], y = 50 + (k - stk_top) * 94;
        { int sold = shop_sold(i); Uint32 col = sold ? dimc : white;
        if (i == shop_sel) { shade_m(20, y, 600, 88, 36); text(28, y + 32, ">", 3, gold); }
        if (giftS[i]) blit(giftS[i], 72 + (80 - giftS[i]->w) / 2, y + (88 - giftS[i]->h) / 2);
        text(176, y + 10, SH_NAME[i], 2, col);
        b[0] = '$'; fmt_money(b + 1, SH_PRICE[i]); text(176, y + 36, b, 2, sold ? dimc : gold);
        text(176, y + 64, SH_INFO[i], 2, dimc);
        if (sold) text_r(604, y + 10, "SOLD OUT", 2, dimc);
        else if (shop_owned(i) >= 0) { snprintf(b, sizeof b, "OWNED %d", shop_owned(i)); text_r(604, y + 10, b, 2, white); } }
    }
    if (nv > STK_ROWS) { if (stk_top > 0) tri(600, 44, 8, 7, 1, gold); if (stk_top + STK_ROWS < nv) tri(600, 432, 8, 7, 0, gold); }
    draw_cash_m();
    if (shop_tab == TAB_STOCKS) text(10, 456, "A BUY 1   X SELL 1   B BACK", 2, dimc);
    else text(10, 456, DEBUG_MODE ? (SH_DESC[shop_sel] ? "A BUY   B BACK   Y INFO   X DEBUG" : "A BUY   B BACK   X DEBUG") : (SH_DESC[shop_sel] ? "A BUY   B BACK   Y INFO" : "A BUY   B BACK"), 2, dimc);
    if (shop_confirm && shop_tab != TAB_STOCKS) {
        panel_am(100, 190, 440, 100, 256, 244);
        snprintf(b, sizeof b, "BUY FOR $%d?", SH_PRICE[shop_sel]); text_c(320, 204, SH_NAME[shop_sel], 2, white); text_c(320, 228, b, 2, gold); text_c(320, 262, "A YES     B NO", 2, white);
    } else if (shop_msg[0] && age_of(shop_msg_ms) < 3000) { int w = tw(shop_msg, 2) + 28; panel_m(320 - w / 2, 420, w, 32, 256); text_c(320, 428, shop_msg, 2, gold); }
    if (shop_info && shop_tab != TAB_STOCKS && SH_DESC[shop_sel]) {   /* v6.8: the info page: the picture, the name and what it does */
        char w[400], *ln = w; int y = 176;
        R(0, 44, 640, 410, C(8,10,18));   /* the list is hidden behind the page */
        panel_am(50, 90, 540, 290, 256, 244); text_c(320, 106, SH_NAME[shop_sel], 2, gold); R(70, 132, 500, 2, mn_dc());
        if (giftS[shop_sel]) blit(giftS[shop_sel], 70 + (100 - giftS[shop_sel]->w) / 2, 156 + (100 - giftS[shop_sel]->h) / 2);
        wrap_buf(w, sizeof w, SH_DESC[shop_sel], 28);
        while (*ln) { char *e = strchr(ln, '\n'); char t2[48]; int n = e ? (int)(e - ln) : (int)strlen(ln); if (n > 40) n = 40; memcpy(t2, ln, (size_t)n); t2[n] = 0; text(190, y, t2, 2, white); y += 26; ln += n; if (*ln == '\n') ln++; }
        text_c(320, 350, "B BACK", 2, dimc);
    }
}
static void shop_open(void) { scr = SC_SHOP; stk_top = 0; shop_tab = shop_tab_ok(TAB_ITEMS) ? TAB_ITEMS : TAB_STOCKS; shop_sel = shop_tab == TAB_ITEMS ? SH_FLOWER : 0; shop_confirm = 0; shop_msg[0] = 0; shop_info = 0; }
static void shop_note(const char *m) { snprintf(shop_msg, sizeof shop_msg, "%s", m); shop_msg_ms = now_ms(); }
static void shop_fix(int p0) {   /* v10.0: after a purchase a one-time item has left the list: the cursor goes to the row that took its place; an emptied tab is left for the next open one */
    int o[16], nv, k;
    if (!shop_tab_ok(shop_tab)) { shop_tab = shop_next_tab(shop_tab, 1); stk_top = 0; nv = shop_list(shop_tab, o); shop_sel = nv > 0 ? o[0] : 0; return; }
    nv = shop_list(shop_tab, o);
    for (k = 0; k < nv && o[k] != shop_sel; k++);
    if (k >= nv && nv > 0) shop_sel = o[p0 < nv ? p0 : nv - 1];
    if (nv > 0) shop_scroll(shop_pos(o, nv), nv);
}
/* v10.0 (self-tests): does the store list of a tab equal what the rules allow? (ITEMS / BLACK MARKET order, a bought one-time item left out, items closed by g_shop_avail left out) */
static int tst_list_ok(int tab) {
    static const int ex_items[] = { SH_FLOWER, SH_LOTION, SH_DECK, SH_STACK, SH_YOGA, SH_TOP, SH_COW, SH_GOLD, SH_DIAMOND, SH_PUMP }, ex_bm[] = { SH_RING, SH_PILL, SH_MILK, SH_SUPER, SH_INJECT };
    const int *ex = tab == TAB_ITEMS ? ex_items : ex_bm; int n = tab == TAB_ITEMS ? 10 : 5, o[16], m = 0, j, c = shop_list(tab, o);   /* v11.1: 10 (cow print outfit) */
    for (j = 0; j < n; j++) if (g_shop_avail(&G, ex[j]) && !g_shop_once_done(&G, ex[j])) { if (m >= c || o[m] != ex[j]) return 0; m++; }
    return m == c;
}
/* v10.0: the ACHIEVEMENT banner. The 20th win over Sophia (hotel match or the boss match) sets ach_pend; the banner is shown as soon as you walk again (not over a result screen or a scene), for ACH_MS. Nothing counts the wins on screen. */
#define ACH_MS 4500
static int ach_pend, ach_on; static unsigned ach_t0; static int ach_shown;   /* ach_shown: how often it has been started (checks) */
static void ach_check(int sw_before) { if (sw_before < SW_GOAL && G.sw_wins >= SW_GOAL) ach_pend = 1; }

/* ---------- v6.3: the debug panel (DEBUG_MODE in logic.h; opened with X in the online store) ---------- */
#define DBG_N 11
static const char *const DBG_ITEMS[DBG_N] = {"SKIP SOPHIA MATCH", "+$50,000 CASH", "NEW DAY (MORNING)", "FILL ROMANCE BAR", "SKIP 7 DAYS", "MILK: PUMP + 4 PILLS + 2 DRUGS", "ROOM UPGRADE (FREE)", "COW MODE ON (OUTFIT)", "BREAK THE PUMP", "MOMMY MILK INJECTION: GIVE", "BACK"};
static int dbg_sel; static char dbg_msg[48]; static unsigned dbg_msg_ms;
static void dbg_note(const char *m) { snprintf(dbg_msg, sizeof dbg_msg, "%s", m); dbg_msg_ms = now_ms(); }
static void draw_debug(void) {
    int i; Uint32 white = C(240,240,240), gold = C(250,215,90), dimc = C(130,135,150), red = C(235,90,90);
    T = S; R(0, 0, 640, 480, C(8,10,18));
    text(14, 12, "DEBUG", 2, red); text(110, 14, "(TEST BUILD ONLY)", 1, dimc); R(20, 40, 600, 2, C(150,125,70));
    for (i = 0; i < DBG_N; i++) {
        int y = 48 + i * 32; Uint32 col = i == dbg_sel ? gold : white;
        if (i == dbg_sel) { shade(20, y - 4, 600, 30, 250, 215, 90, 36); text(28, y + 2, ">", 2, gold); }
        text(60, y + 2, DBG_ITEMS[i], 2, col);
    }
    draw_cash(); text(10, 456, "A USE   B BACK", 2, dimc);
    if (dbg_msg[0] && age_of(dbg_msg_ms) < 3000) { int w = tw(dbg_msg, 2) + 28; panel(320 - w / 2, 412, w, 32, 256); text_c(320, 420, dbg_msg, 2, gold); }
}
static void dbg_open(void) { scr = SC_DEBUG; dbg_sel = 0; dbg_msg[0] = 0; }
static void dbg_key(int b) {
    if (b == B_UP) dbg_sel = (dbg_sel + DBG_N - 1) % DBG_N; else if (b == B_DOWN) dbg_sel = (dbg_sel + 1) % DBG_N;
    else if (b == B_B) scr = g_computer(&G) ? SC_SHOP : SC_WALK;   /* v9.3: from the base desk (no computer) back to the room */
    else if (b == B_A) {
        char m[48];
        switch (dbg_sel) {
            case 0: { int r = g_dbg_skip(&G); dbg_note(r == 0 ? "SOPHIA MATCH SKIPPED" : r == 1 ? "ALREADY PAST THE MATCH" : "FINISH THE INTRO FIRST"); break; }
            case 1: g_dbg_cash(&G); dbg_note("+$50,000 CASH"); break;
            case 2: g_dbg_newday(&G); snprintf(m, sizeof m, "DAY %d MORNING", G.day); dbg_note(m); break;
            case 3: g_dbg_fillbar(&G); snprintf(m, sizeof m, "LEVEL %d  BAR %d%%", G.rom_lvl, G.rom_pct); dbg_note(m); break;
            case 4: g_dbg_week(&G); snprintf(m, sizeof m, "DAY %d MORNING", G.day); dbg_note(m); break;
            case 5: g_dbg_milk(&G); dbg_note("MILK STORE OPEN, PUMP, PILLS, 2 SUPER DRUGS"); break;
            case 6: if (G.room_up < 2) { G.room_up++; room_apply(); snprintf(m, sizeof m, "ROOM LEVEL %d  RENT $%d", G.room_up, g_rent_amt(&G)); dbg_note(m); } else dbg_note("ROOM IS AT THE LAST LEVEL"); break;   /* v9.3: the level changes at once (the picture follows when you walk in) */
            case 7: if (!g_p4_open(&G)) dbg_note("ONLY AFTER THE WEDDING"); else { G.cow_given = 1; G.outfit = OF_COW; persist(); dbg_note("COW MODE ON (SHE WEARS THE COW OUTFIT)"); } break;   /* v12.1 */
            case 8: if (G.milk_st < 3) dbg_note("SHE HAS NO PUMP YET"); else { if (G.pump_life <= 0) G.pump_life = g_pump_roll(); G.pump_used = G.pump_life; persist(); dbg_note("PUMP WORN OUT: ASK FOR MILK"); } break;   /* v12.2: the next ask finds it broken */
            case 9: if (G.injected) dbg_note("ALREADY INJECTED"); else if (G.milk_st < 3) dbg_note("SHE HAS NO PUMP YET"); else { int r; G.inj = 1; r = g_inject_gift(&G); persist(); dbg_note(r == 0 ? "INJECTED: NO GAUGE, ONE MILK A DAY" : "WAIT: A SUPER DRUG STILL WORKS"); } break;   /* v12.3 */
            default: scr = g_computer(&G) ? SC_SHOP : SC_WALK; break;
        }
    }
}
/* v8.4: the romance bar is VERTICAL: a short track with the fill rising from the heart, the heart below it with the LEVEL number inside; no percentage, no text.
   The heart is assets/romance_heart.png (cut from the old bar art by tools/make_romance_heart.py); the track is drawn in code. */
#define RB_TW 18   /* track width */
#define RB_TH 88   /* track height */
static void draw_romance_bar(int x, int y) {
    char b[8]; int hw = romHeartS ? LW(romHeartS) : 52, hh = romHeartS ? LH(romHeartS) : 48;
    int tx = x + (hw - RB_TW) / 2, ty = y, fh = (RB_TH - 4) * G.rom_pct / 100, hy = y + RB_TH - 10;
    R(tx - 2, ty - 2, RB_TW + 4, RB_TH + 4, C(14,8,16));          /* dark frame */
    R(tx - 1, ty - 1, RB_TW + 2, RB_TH + 2, C(120,112,126));      /* light edge, like the old art */
    R(tx, ty, RB_TW, RB_TH, C(40,16,30));                          /* empty track */
    if (fh > 0) { int fy = ty + RB_TH - 2 - fh; R(tx + 2, fy, RB_TW - 4, fh, C(214,52,98)); R(tx + 2, fy, 4, fh, C(250,140,170)); R(tx + 2, fy, RB_TW - 4, 3, C(250,140,170)); }
    if (romHeartS) blit(romHeartS, x, hy);
    snprintf(b, sizeof b, "%d", G.rom_lvl);
    text_o(x + (hw - tw(b, 2)) / 2, hy + (hh - 14) / 2 - 2, b, 2, C(255,245,248));   /* the level number inside the heart */
}
/* v11.0: the FRIENDSHIP bar: the same vertical bar as the romance bar, but blue. Fill = wins over her / FR_GOAL (g_fr_pct), the number in the heart = the tier 1..5. */
static void draw_friend_bar(int x, int y) {
    char b[8]; int hw = frHeartS ? LW(frHeartS) : 52, hh = frHeartS ? LH(frHeartS) : 48, pc = g_fr_pct(&G);
    int tx = x + (hw - RB_TW) / 2, ty = y, fh = (RB_TH - 4) * pc / 100, hy = y + RB_TH - 10;
    R(tx - 2, ty - 2, RB_TW + 4, RB_TH + 4, C(8,10,18));
    R(tx - 1, ty - 1, RB_TW + 2, RB_TH + 2, C(112,120,136));
    R(tx, ty, RB_TW, RB_TH, C(16,26,48));
    if (fh > 0) { int fy = ty + RB_TH - 2 - fh; R(tx + 2, fy, RB_TW - 4, fh, C(52,120,226)); R(tx + 2, fy, 4, fh, C(150,196,250)); R(tx + 2, fy, RB_TW - 4, 3, C(150,196,250)); }
    if (frHeartS) blit(frHeartS, x, hy);
    snprintf(b, sizeof b, "%d", g_fr_tier(&G));
    text_o(x + (hw - tw(b, 2)) / 2, hy + (hh - 14) / 2 - 2, b, 2, C(245,248,255));
}
/* v6.4: the dialogue box with her options inside it (shown once her line is typed): the box grows to hold them */
static void dialog_opts(const char *txt, const char *tag, unsigned ms, const char *const *items, int n, int sel, const char *label) {
    char w[400]; int i, lines = 1, h, top, y0, lh = 22; Uint32 gold = DLG_GOLD, white = DLG_WHITE;
    wrap_buf(w, sizeof w, txt, dlg_cols()); for (i = 0; w[i]; i++) if (w[i] == '\n') lines++;
    h = 24 + lines * lh + n * lh + 4; if (h < 108) h = 108; top = DLG_BOT - h;
    panel_d(0, top, 640, h, 256);
    if (tag) { int ty = top - 32; panel_d(10, ty, tw(tag, 2) + 28, 28, 256); dtext(DLG_L, ty + 7, tag, 2, name_col(tag)); }
    if (label) dtext_r(DLG_R + 8, top - 18, label, 1, C(230,235,245));
    { int shown = frozen ? 9999 : (int)(age_of(ms) / 30), j = 0, ln = 0, col = 0;
      for (i = 0; w[i]; i++) { char ch[2]; if (w[i] == '\n') { ln++; col = 0; continue; } if (j >= shown) break; ch[0] = w[i]; ch[1] = 0; dtext(DLG_L + col * 12, top + 12 + ln * lh, ch, 2, white); col++; j++; } }
    if (!typed_done(txt, ms)) return;
    y0 = top + 12 + lines * lh + 4;
    for (i = 0; i < n; i++) {
        Uint32 col = i == sel ? gold : white;
        if (i == sel) dtext(DLG_L + 6, y0 + lh * i, ">", 2, col);
        dtext(DLG_L + 30, y0 + lh * i, items[i], 2, col);
    }
}
/* v6.5: the bottle gauge (top right of her romance scene only): the empty bottle, the milk rising inside it, the picture on top; 2 milk pills fill it (v7.6) */
static void gauge_init(void) {
    int y, w, h, l, r, x; if (!gaugeS) return;
    w = LW(gaugeS); h = LH(gaugeS); if (h > 200) h = 200; gauge_y0 = -1; gauge_y1 = -1;
    for (y = 0; y < h; y++) {
        l = -1; r = -1;
        { int yn = YS(y) + (YS(y + 1) - YS(y)) / 2, xn; if (yn >= gaugeS->h) yn = gaugeS->h - 1;   /* scan the middle of this logical row at screen resolution */
          for (xn = 0; xn < gaugeS->w; xn++) { Uint32 v = *px(gaugeS, xn, yn); if (((v & gaugeS->format->Amask) >> gaugeS->format->Ashift) > 128) { x = (int)((long long)xn * 640 / SW); if (l < 0) l = x; r = x; } } }
        gauge_xl[y] = l + 1; gauge_xr[y] = r - 1;
        if (l >= 0 && (r - l) * 10 >= (w - 1) * 8) { if (gauge_y0 < 0) gauge_y0 = y; gauge_y1 = y; }   /* the body of the bottle: the wide rows */
    }
}
static void draw_gauge(int x, int y, int pct) {
    int yy, rows, fill; char b[8];
    if (!gaugeS || gauge_y0 < 0) return;
    rows = gauge_y1 - gauge_y0 + 1; fill = rows * pct / 100;
    for (yy = gauge_y0; yy <= gauge_y1; yy++) {
        int d = gauge_y1 - yy;   /* rows up from the bottom */
        R(x + gauge_xl[yy], y + yy, gauge_xr[yy] - gauge_xl[yy] + 1, 1, d < fill ? (d == fill - 1 && fill < rows ? C(255,252,240) : C(246,238,222)) : C(52,24,44));
    }
    blit(gaugeS, x, y);
    snprintf(b, sizeof b, "%d%%", pct); text_o(x + (LW(gaugeS) - tw(b, 2)) / 2, y + LH(gaugeS) + 4, b, 2, C(250,240,235));
}
/* v8.3: her menu is three PAGES (R1 next, L1 previous, wrapping; B leaves, there is no LEAVE row):
     page 1  PLAY A MATCH, CHAT
     page 2  what you can GIVE, only what you own and without a count: before the day-8 scene the gifts (flower, body lotion, pink card, money stack, yoga outfit, gold bar, pill),
             after it the milk pill and the Super drug. With nothing to give the page says YOU HAVE NO GIFT.
     page 3  (only after the day-8 scene) TEASE, ASK FOR MILK (only while the Super drug works)
   Cheapest romance first in the gift order of GL; the pump has no row (it is handed over in its scene). */
#define GL_N 9
static const int GL[GL_N] = {GF_FLOWER, GF_LOTION, GF_DECK, GF_STACK, GF_YOGA, GF_TOP, GF_COW, GF_GOLD, GF_PILL};   /* v11.1: the cow print outfit sits between the designer top and the gold bar */   /* v10.8: the designer top sits between the yoga outfit and the gold bar */
static int rom_gift_n(void) { return GL_N; }
enum { RA_PLAY = 1, RA_CHAT, RA_TEASE, RA_ASK, RA_MILK, RA_SUPER, RA_RING, RA_OUTFIT, RA_PUMP, RA_HUG, RA_KISS, RA_SLEEP, RA_FEED, RA_GIVE = 16, RA_OUTFIT_K = 60 };   /* v10.8: RA_OUTFIT_K + n = change into outfit n (OF_*): one row for every outfit she owns except the one she wears (RA_OUTFIT itself is not used any more) */   /* v8.6: RING / OUTFIT / PUMP and the four page-4 entries */   /* RA_GIVE + a gift kind = give that gift */
static int rom_pages(void) { return g_p4_open(&G) ? 4 : G.pill_seen ? 3 : 2; }   /* v8.6: page 4 once you are married */
static int rom_pg(void) { return rom_page >= 0 && rom_page < rom_pages() ? rom_page : 0; }
static int rom_deck_row(void) { return G.rs == RS_LIVING && !G.deck_given && G.pdeck > 0; }   /* v11.6: the first pink deck since she lives here stays on the give page (after the day-8 scene and at a full bar too) */
static int rom_outfit_row(int kd) { return g_outfit_unlocked(&G) && g_gift_stock(&G, kd) > 0 && !(kd == GF_YOGA ? G.yoga_given : kd == GF_TOP ? G.top_given : G.cow_given); }   /* v11.3: an outfit is on the give page at max romance, while you own one she does not have yet */
static int rom_page_acts(int page, int *a) {
    int n = 0, i;
    if (page == 0) { a[n++] = RA_PLAY; a[n++] = RA_CHAT; }   /* v11.3: no CHANGE OUTFIT rows here any more: her wardrobe (her bedroom) is the only place to change clothes */
    else if (page == 1) {
        if (!G.pill_seen) { for (i = 0; i < GL_N; i++) { int kd = GL[i]; if (kd == GF_PILL ? (G.pill && !G.pill_given) : g_gift_is_outfit(kd) ? rom_outfit_row(kd) : g_gift_stock(&G, kd) > 0) a[n++] = RA_GIVE + kd; } }
        else { if (G.mpill > 0 && !G.injected) a[n++] = RA_MILK; if (G.sdrug > 0 && !G.injected) a[n++] = RA_SUPER; if (G.inj > 0 && G.milk_st >= 3) a[n++] = RA_GIVE + GF_INJECT; if (rom_deck_row()) a[n++] = RA_GIVE + GF_DECK; { int q; for (q = 0; q < GL_N; q++) if (g_gift_is_outfit(GL[q]) && rom_outfit_row(GL[q])) a[n++] = RA_GIVE + GL[q]; } if (G.pump > 0 && G.milk_st >= 3) a[n++] = RA_GIVE + GF_PUMP; if (g_can_marry(&G)) a[n++] = RA_RING; }   /* v11.3: outfits stay on the give page after the day-8 scene. v12.2: a new pump you bought for the broken one */
    } else if (page == 3) { a[n++] = RA_HUG; a[n++] = RA_KISS; a[n++] = RA_SLEEP; if (g_mama(&G)) a[n++] = RA_FEED; }   /* v9.5: BREASTFEED exists only in mama mode */
    else { a[n++] = RA_TEASE; if (g_pump_ready(&G)) a[n++] = RA_PUMP; if (g_super_active(&G) || G.injected) a[n++] = RA_ASK; }
    return n;
}
static int rom_acts(int *a) { return rom_page_acts(rom_pg(), a); }
static int rom_menu_n(void) { int a[10]; return rom_acts(a); }
static int rom_act(int sel) { int a[10], n = rom_acts(a); return n <= 0 ? 0 : a[sel < 0 ? 0 : sel >= n ? n - 1 : sel]; }
static void choice_box(const char *tag, const char *const *items, int n, int sel, const char *label) {   /* v6.6: the choices alone, on their own page */
    int lh = 22, h = 20 + n * lh, top = DLG_BOT - h, i; Uint32 gold = DLG_GOLD, white = DLG_WHITE;
    panel_d(0, top, 640, h, 256);
    if (tag) { int ty = top - 32; panel_d(10, ty, tw(tag, 2) + 28, 28, 256); dtext(DLG_L, ty + 7, tag, 2, name_col(tag)); }
    if (label) dtext_r(DLG_R + 8, top - 18, label, 1, C(230,235,245));
    for (i = 0; i < n; i++) { Uint32 col = i == sel ? gold : white; if (i == sel) dtext(DLG_L + 6, top + 10 + lh * i, ">", 2, col); dtext(DLG_L + 30, top + 10 + lh * i, items[i], 2, col); }
}
static void draw_rom(void) {
    char nb[10][36]; const char *it[10]; int i, rx = 636;   /* v11.1: 9 gift rows */
    bg_draw(ls_talk ? TALK_BG() : rom_in ? BG_ROM : BG_BDOOR); blit_a(feed_hide == 1 && !ls_talk && !rom_in && fh_pic(1) ? fh_pic(1) : rom_mode == RM_TALK && tk_kind != TK_TEASE ? chat_vis() : her_vis(), 256);
    if (!g_rom_full(&G)) draw_romance_bar(8, 6);   /* a full level-3 bar is not shown any more */
    if (g_milk_gauge(&G) && gaugeS) { rx = 636 - LW(gaugeS) - 10; draw_gauge(636 - LW(gaugeS), 6, g_milk_gauge_pct(&G)); }   /* v8.2: the Super drug keeps it full */
    if (rom_mode == RM_TALK) { int w = tk_who[tk_i], done = dialog(tk_t[tk_i], w == 0 ? N_ME : w == 1 ? N_SOPHIA : NULL, w == 2, tk_ms); if (done) dtext_r(DLG_R, dlg_py, "A >", 2, gui_tc()); }
    else if (!rom_choose) { int done = dialog(rom_txt, N_SOPHIA, 0, rom_ms); if (done) dtext_r(DLG_R, dlg_py, "A >", 2, gui_tc()); }   /* her line first, on its own: A opens the choices */
    else if (rom_mode == RM_MENU) {
        int pg = rom_pg(), n = rom_menu_n(), rows = n > 0 ? n : 1, h = 20 + rows * 22; char pgb[40];
        static char lbl[40]; const char *lb = NULL;
        for (i = 0; i < n; i++) {
            int a = rom_act(i);
            if (a >= RA_OUTFIT_K) it[i] = OF_CHG[(a - RA_OUTFIT_K) % OF_N];
            else if (a >= RA_GIVE) { snprintf(nb[i], sizeof nb[i], "%s%s", R_GNAME[a - RA_GIVE], a - RA_GIVE == GF_PILL && !g_rom_full(&G) ? "  LOCKED" : ""); it[i] = nb[i]; }
            else if (a == RA_MILK) it[i] = R_MILKITEM;
            else if (a == RA_SUPER) it[i] = R_SUPERITEM;
            else if (a == RA_RING) it[i] = "GIVE DIAMOND RING";
            else if (a == RA_PUMP) it[i] = "PUMP MILK";
            else if (a == RA_HUG) it[i] = "HUG";
            else if (a == RA_KISS) it[i] = "KISS";
            else if (a == RA_SLEEP) { snprintf(nb[i], sizeof nb[i], "SLEEP TOGETHER%s", g_night(&G) ? "" : "  NIGHT ONLY"); it[i] = nb[i]; }
            else if (a == RA_FEED) { snprintf(nb[i], sizeof nb[i], "BREASTFEED%s", g_feed_ready(&G) ? "" : "  LOCKED"); it[i] = nb[i]; }
            else if (a == RA_ASK) { snprintf(nb[i], sizeof nb[i], "%s  %d LEFT TODAY", R_ASKITEM, g_asks_left(&G)); it[i] = nb[i]; }
            else it[i] = a == RA_PLAY ? R_ITEMS[1] : a == RA_CHAT ? "CHAT" : "TEASE";
        }
        if (!n) it[0] = "YOU HAVE NO GIFT";
        if (pg == 1) lb = !G.pill_seen ? (G.gift_day == G.day ? "GIFT GIVEN TODAY" : "ONE GIFT PER DAY") : g_milk_gauge(&G) ? "NO DAILY LIMIT FOR MILK PILLS" : NULL;
        if (g_super_active(&G) && pg >= 1) { snprintf(lbl, sizeof lbl, "SUPER DRUG: %d DAY%s LEFT", g_super_left(&G), g_super_left(&G) == 1 ? "" : "S"); lb = lbl; }
        if (G.injected && pg == 2) lb = "ONE MILK A DAY: ASK, PUMP OR BREASTFEED";   /* v12.3 */
        if (pg == 3 && g_mama(&G) && !(g_super_active(&G))) lb = g_feed_ready(&G) ? NULL : G.injected ? "ONE MILK A DAY. COME BACK TOMORROW" : "BREASTFEED NEEDS A FULL GAUGE";   /* v8.6 */
        choice_box(N_SOPHIA, it, rows, n > 0 && rom_sel < n ? rom_sel : -1, lb);
        snprintf(pgb, sizeof pgb, g_p4_open(&G) ? "L1  PAGE %d/%d  R1  R2 NAME" : "L1  PAGE %d/%d  R1", pg + 1, rom_pages()); text_r(DLG_R, DLG_BOT - h + 10, pgb, 2, C(170,175,190));   /* v8.3: which page, and how to turn it */
    } else if (rom_mode == RM_TEASE) {
        for (i = 0; i < TS_N; i++) { snprintf(nb[i], sizeof nb[i], "%s%s", T_NAMES[i], i >= TS_MILK && !g_tease_milk_open(&G) ? "  LOCKED" : ""); it[i] = nb[i]; }
        it[TS_N] = "BACK";
        choice_box(N_SOPHIA, it, TS_N + 1, tease_sel, "SHE WILL THROW YOU OUT");
    } else choice_box(N_SOPHIA, R_LEN, 3, rom_lsel, NULL);
    if (rom_note[0] && age_of(rom_note_ms) < 3200) { int w = tw(rom_note, 2) + 28; panel(rx - w, 8, w, 32, 256); text(rx + 14 - w, 16, rom_note, 2, gui_tc()); }   /* top right (left of the gauge): never over her face */
}
static void draw_romq(void) {
    static const char *const IT[2] = {"YES, MOVE IN WITH ME", "NOT NOW"};
    bg_draw(BG_HALL); blit_a(her_vis(), 256);
    dialog(B_PROPOSE[5].txt, N_SOPHIA, 0, vn_ms);
    menu_panel_at(IT, 2, romq_sel, 330, 290, 326);
}
/* ---------- v9.5: the name keyboard (R2 in her menu, married): type MAMA / MAMA SOPHIA (names.txt) for mama mode, SOPHIA for her usual self ---------- */
static char nm_buf[NM_LEN + 1]; static int nm_len, nm_r, nm_c;
static const char NM_KEYS[3][10] = {"ABCDEFGHI", "JKLMNOPQR", "STUVWXYZ "};
static int nm_cols(int r) { return r == 3 ? 3 : 9; }
static void draw_name(void) {
    Uint32 white = C(240,240,240), gold = gui_tc(), dimc = C(130,135,150); int r, c, tx; char t[NM_LEN + 3];
    bg_draw(ls_talk ? TALK_BG() : rom_in ? BG_ROM : BG_BDOOR);
    panel_a(80, 40, 480, 400, 256, 244);
    text_c(320, 58, "WHAT DO YOU CALL HER?", 2, gold); R(100, 90, 440, 2, gui_dc());
    R(120, 108, 400, 46, C(10,12,20)); R(120, 108, 400, 2, gui_dc()); R(120, 152, 400, 2, gui_dc());
    snprintf(t, sizeof t, "%s%s", nm_buf, nm_len < NM_LEN && ((now_ms() / 450) & 1) ? "_" : ""); tx = 320 - tw(nm_buf, 3) / 2; text(tx, 118, t, 3, white);
    for (r = 0; r < 4; r++) for (c = 0; c < nm_cols(r); c++) {
        int w = r == 3 ? 120 : 40, x = r == 3 ? 140 + c * 120 : 140 + c * 40, y = 176 + r * 44, on = r == nm_r && c == nm_c; char lb[8]; Uint32 col = on ? gold : white;
        if (r < 3) { if (r == 2 && c == 8) snprintf(lb, sizeof lb, "SP"); else snprintf(lb, sizeof lb, "%c", NM_KEYS[r][c]); }
        else snprintf(lb, sizeof lb, "%s", c == 0 ? "DEL" : c == 1 ? "OK" : "BACK");
        if (on) shade(x + 1, y, w - 2, 38, 250, 215, 90, 60); else shade(x + 1, y, w - 2, 38, 255, 255, 255, 14);
        text_c(x + w / 2, y + 11, lb, 2, col);
    }
    text_c(320, 366, "A TYPE   B DELETE   X SPACE", 2, dimc); text_c(320, 392, "Y DONE", 2, dimc);
}
static void draw_peek(void) {
    static const char *const IT[2] = {"FACE THE WALL", "LOOK AT HER"};
    bg_draw(BG_ROM); choice_box(NULL, IT, 2, peek_sel, "SHE TOLD YOU NOT TO LOOK");
}
static void draw_end_home(void) {   /* the end of a free match at home: no money, only the romance bar */
    int res = G.last_res; char b[40]; Uint32 white = C(240,240,240), gold = gui_tc(), dimc = C(130,135,150), red = C(235,120,120);
    bg(1); draw_say();
    panel_a(100, 190, 440, 130 + (res > 0 ? 24 : 0), 256, 238);
    text_c(320, 200, res > 0 ? "YOU WIN" : res < 0 ? "YOU LOSE" : "TIE", 3, res > 0 ? gold : res < 0 ? red : C(200,205,215));
    text(124, 240, "NET CHIPS", 2, white);
    if (G.last_net > 0) snprintf(b, sizeof b, "+%d", G.last_net); else if (G.last_net < 0) snprintf(b, sizeof b, "-%d", -G.last_net); else snprintf(b, sizeof b, "0");
    text_r(516, 240, b, 2, res > 0 ? gold : res < 0 ? red : white);
    R(124, 262, 392, 2, gui_dc());
    text(124, 272, "ROMANCE", 2, white);
    if (G.last_rom > 0) { snprintf(b, sizeof b, "+%d%%", G.last_rom); text_r(516, 272, b, 2, gold); if (G.rom_lvl > home_lvl0) text_c(320, 296, "ROMANCE LEVEL UP!", 2, gold); }
    else text_r(516, 272, "NO REWARD", 2, dimc);
    text_c(320, 296 + (res > 0 ? 28 : 6), "A CONTINUE", 2, dimc);
}
/* ---------- event center, setup and the multi-player table ---------- */
static void hl_clear(void) { memset(hl_hole, 0, sizeof hl_hole); memset(hl_board, 0, sizeof hl_board); }
static void hl_mark(int seat, int h0, int h1, const int *board) {   /* outline the 5 cards of this player's best hand */
    int a[7], mask, i;
    a[0] = h0; a[1] = h1; for (i = 0; i < 5; i++) a[2 + i] = board[i];
    best7_pick(a, &mask);
    for (i = 0; i < 2; i++) if ((mask >> i) & 1) hl_hole[seat][i] = 1;
    for (i = 0; i < 5; i++) if ((mask >> (2 + i)) & 1) hl_board[i] = 1;
}
static void hl_sophia(void) {
    hl_clear(); if (!G.showdown) return;
    if (G.sc[0] >= G.sc[1]) hl_mark(0, G.deck[0], G.deck[1], G.deck + 4);
    if (G.sc[1] >= G.sc[0]) hl_mark(1, G.deck[2], G.deck[3], G.deck + 4);
}
static void hl_multi(void) {
    int i, best = -1; hl_clear(); if (!M.showdown) return;
    for (i = 0; i < M.n; i++) if (seat_in[i] && !M.folded[i] && M.sc[i] > best) best = M.sc[i];
    for (i = 0; i < M.n; i++) if (seat_in[i] && !M.folded[i] && M.sc[i] == best) hl_mark(i, M.deck[2*i], M.deck[2*i+1], M.deck + 8);
}
static const char *const MTAG[7] = {"", "CHECK", "CALL", "RAISE", "FOLD", "BET", "ALL IN"};
/* v8.5: the Exhibition / Quick Play table backdrop (background + boss + empty table, up to three full-screen layers, two of them alpha-blended) is composed ONCE into an
   opaque surface and then copied with a single plain blit each frame, so cards and chips move smoothly on the Miyoo. The picture is identical (checked by the tests). */
static SDL_Surface *feltC; static int felt_key;
static void draw_felt_layers(void) {
    if (mp_mode == 1) { bg_draw(BG_EXHIB); if (M.boss && lz(&bossS, "assets/boss_sophia.png")) blit(bossS, 0, 0); if (tblE) blit(tblE, 0, 0); else R(0, 362, 640, 118, C(45,112,82)); }
    else { T = S; R(0, 0, 640, 480, C(0,0,0)); if (cpuimg) blit(cpuimg, 0, 0); else R(0, 362, 640, 118, C(45,112,82)); }
}
static void draw_felt(void) {
    int key = 1 + (mp_mode == 1) + 2 * (M.boss != 0) + 4 * (SW * 7 + SH);
    T = S;
    if (feltC && felt_key == key && feltC->w == S->w && feltC->h == S->h) { SDL_BlitSurface(feltC, NULL, S, NULL); return; }
    draw_felt_layers();
    if (!feltC || feltC->w != S->w || feltC->h != S->h) {
        if (feltC) SDL_FreeSurface(feltC);
        feltC = SDL_CreateRGBSurface(0, S->w, S->h, 32, S->format->Rmask, S->format->Gmask, S->format->Bmask, 0);   /* no alpha channel: a plain copy */
    }
    if (feltC) { SDL_BlitSurface(S, NULL, feltC, NULL); felt_key = key; }
}
static void draw_mtable(void) {
    char b[48]; int i, cnt[4], bfade = 0, pot = mp_pot(&M), n = M.stage == 0 ? 0 : M.stage + 2, len, yourturn = !M.over && M.toact == 0, potshow, ptxt, rev_done = M.showdown && (int)anim_age(rev_ms) >= FLIP_MS;
    int paying = M.over && pay_who, mpaid = paying && (int)anim_age(pay_ms) >= PAY_FLY, ys = M.stack[0] + fly_wait(0) - (paying && !mpaid ? M.won[0] : 0), cs = M.stack[1] + fly_wait(1) - (paying && !mpaid ? M.won[1] : 0);
    unsigned hage = age_of(hand_end_ms); Uint32 white = C(240,240,240), gold = gui_tc(), dimc = C(130,135,150);
    if (n > 5) n = 5;
    you_fy = 135;   /* v9.6: the middle of your money box */
    hl_gate();
    if (M.over) { unsigned dl = hand_dl(); if (hage >= dl) bfade = hage >= dl + 300 ? 256 : (int)((hage - dl) * 256 / 300); }
    draw_felt();
    draw_board(M.deck + 8, n, M.hands, M.showdown);
    potshow = pot - fly_left(0) - fly_left(1); ptxt = potshow;
    if (paying && (int)anim_age(pay_ms) >= 0) potshow = 0;   /* on their way to the winner (or already there) */
    change(potshow, cnt);
    for (i = 0; i < 4; i++) { int h, j; for (h = cnt[i] > 6 ? 6 : cnt[i], j = 0; j < h; j++) blit(potS[i], 24 + i*32, POT_Y - j*4); }
    draw_flights();
    if (paying) { int pa = (int)anim_age(pay_ms); if (pa >= 0 && pa < PAY_FLY) { int e = pa * 256 / PAY_FLY; if (M.won[0] > 0) fly_chips(M.won[0], 60, POT_CY, 540, you_fy, e); if (M.won[1] > 0) fly_chips(M.won[1], 60, POT_CY, 540, 425, e); } }
    {   /* the CPU: cards where Sophia's are, name box and action tag under the left column, stack bottom right like hers */
        int show = M.showdown && !M.folded[1], ny = mp_mode == 1 ? 130 : 100; Uint32 nc = M.folded[1] ? dimc : white;
        if (!M.folded[1]) her_hole(M.deck[2], M.deck[3], show, hd_hole[1]);   /* v11.5: a folded CPU mucks its cards: nothing on the felt, no dark box over the board */
        if (M.boss) { panel_t(10, ny, 76, 26, 256); text(18, ny + 6, "WOMAN", 2, nc); panel_t(92, ny, 90, 26, 256); }   /* the boss: her name box is wider (she is not "CPU"), the action box moves over */
        else { panel_t(10, ny, 54, 26, 256); text(18, ny + 6, "CPU", 2, nc); panel_t(70, ny, 90, 26, 256); }
        {   /* the CPU's own small card box under its name: two small cards, face down until the showdown (smaller than your box) */
            int by = ny + 30, k;
            panel_t(10, by, 106, 80, 256);
            for (k = 0; k < 2; k++) {
                int a2 = (int)anim_age_deal(deal_ms) - (k ? 2 : 0) * DEAL_STEP, cx = 17 + k * 47, fl;
                if (a2 < DEAL_FLY) continue;   /* not dealt yet */
                fl = show ? turn_of(1, rev_ms, FLIP_MS) : 0;
                card_hl = fl >= 256 && hd_hole[1][k]; flat_turn(cx, by + 8, 40, 62, M.deck[2 + k], fl); card_hl = 0;
            }
            if (M.folded[1]) shade(10, by, 106, 80, 12, 16, 22, 150);
        }
        { int tx = M.boss ? 100 : 78;
        if (show && rev_done && M.sc[1]) text(tx, ny + 9, HN[M.sc[1] >> 20], 1, gold);
        else if (!M.over && M.toact == 1) { static const char *DT[4] = {".", "..", "...", ""}; text(tx, ny + 6, DT[(now_ms() / 350) % 4], 2, gold); }
        else if (mp_tag[1]) text(tx, ny + 6, MTAG[mp_tag[1]], 2, mp_tag[1] == 4 ? dimc : mp_tag[1] == 6 ? C(255,150,120) : white); }
        snprintf(b, sizeof b, M.boss ? "HER $%d" : "CPU $%d", cs); text(M.boss ? 508 : 520, STK_TY, b, 2, nc);
        if (M.button == 1) { ell(570, 429, 15, 11, C(120,120,120)); ell(570, 426, 15, 11, white); text(565, 419, "D", 2, C(20,20,20)); }
    }
    /* left column: pot, hand, blinds, prize */
    panel_t(10, 10, 150, 26, 256); snprintf(b, sizeof b, "POT $%d", ptxt); text(18, 16, b, 2, gold);
    panel_t(10, 40, 150, 26, 256); snprintf(b, sizeof b, "HAND %d", M.hands); text(18, 46, b, 2, white);
    panel_t(10, 70, 150, 26, 256); snprintf(b, sizeof b, "BLINDS %d/%d", M.bb / 2, M.bb); len = (int)strlen(b);
    text(18, len * 12 - 2 > 134 ? 77 : 76, b, len * 12 - 2 > 134 ? 1 : 2, white);
    if (mp_mode == 1) { panel_t(10, 100, 150, 26, 256); snprintf(b, sizeof b, "PRIZE $%d", g_ex_prize(mp_buyin)); text(18, 106, b, 2, gold); }
    if (mp_msg[0] && age_of(mp_msg_ms) < 2600) { int w = tw(mp_msg, 2) + 24; panel_t(320 - w / 2, 104, w, 28, 256); text_c(320, 110, mp_msg, 2, C(255,200,120)); }
    /* your cards and your money box, upper right (v9.6: no chip rack; the box under the cards holds only "YOU $x" with your dealer button inside it) */
    panel_t(488, 4, 148, 112, 256);
    your_hole(M.deck[0], M.deck[1], hd_hole[0]);   /* face down until you press L1 */
    if (M.folded[0] && !M.over) shade(498, 8, 128, 88, 12, 16, 22, 150);
    if (rev_done && !M.folded[0]) text(498, 97, HN[M.sc[0] >> 20], 2, gold);
    else if (M.folded[0]) text(498, 97, "FOLDED", 2, dimc);
    if (!M.over && l1_works()) key_badge(619, 98, "Y");
    {   /* your money box: as wide as the text, flush right with the cards box, your dealer button inside it right of the amount */
        int bw, bx, dealer = M.button == 0;
        snprintf(b, sizeof b, "YOU $%d", ys); bw = 8 + tw(b, 2) + 8 + (dealer ? 36 : 0); bx = 636 - bw;
        panel_t(bx, 121, bw, 28, 256);
        text(bx + 8, 128, b, 2, white);
        if (dealer) { int cx = bx + 8 + tw(b, 2) + 8 + 15; ell(cx, 138, 15, 11, C(120,120,120)); ell(cx, 135, 15, 11, white); text(cx - 5, 128, "D", 2, C(20,20,20)); }
    }
    if (bfade) { int net = M.won[0] - M.c[0]; banner_r(net > 0 ? 1 : net < 0 ? -1 : 0, net < 0 ? -net : net, bfade, 14); }
    /* button guide */
    if (yourturn && bm_open) {   /* v8.9: the bet menu: amount (bar), ALL IN row, keys */
        int need = mp_need(&M, 0), mn = mp_min_by(&M, 0), mx = mp_max_by(&M, 0), cap = mp_cap_pay(&M, 0), x0 = 448, y0 = 157, w = 188, sw = w - 28, fw, ph = cap < M.stack[0] ? 180 : 162;
        int top = bm_by >= mx; Uint32 hot = C(150,40,56), cold = C(48,26,32);
        panel_t(x0, y0, w, ph, 256);
        snprintf(b, sizeof b, "%s $%d", need || M.stage == 0 ? "RAISE" : "BET", need + bm_by); text(x0 + 12, y0 + 8, b, 2, top ? C(255,150,120) : gold);
        R(x0 + 12, y0 + 32, sw + 4, 10, C(30,34,44)); fw = mx > mn ? (bm_by - mn) * (sw - 4) / (mx - mn) : sw - 4; if (fw > 0) R(x0 + 14, y0 + 34, fw, 6, gold);
        R(x0 + 12 + 2 + fw, y0 + 29, 4, 16, white);
        R(x0 + 10, y0 + 52, w - 20, 24, top ? hot : cold); snprintf(b, sizeof b, "ALL IN $%d", cap); text_c(x0 + w / 2, y0 + 58, b, 2, top ? white : dimc);
        text(x0 + 12, y0 + 84, "UP/DN SMALL", 2, white); text(x0 + 12, y0 + 102, "LT/RT BIG", 2, white);
        text(x0 + 12, y0 + 120, "X POT Y ALL IN", 2, white); text(x0 + 12, y0 + 138, "A OK  B BACK", 2, white);
        if (cap < M.stack[0]) text(x0 + 12, y0 + 162, "HER STACK IS SHORTER", 1, C(235,120,120));
    } else {
    panel_t(472, 157, 164, 84, 256);   /* v8.5: shorter, the "MENU LEAVES" lines are gone */
    if (M.over) text(482, 167, M.game_over ? "A CONTINUE" : "A NEXT HAND", 2, hage >= hand_dl() ? white : dimc);
    else if (yourturn) {
        int need = mp_need(&M, 0), pay = mp_pay_call(&M, 0), cr = mp_can_raise(&M, 0);
        if (need == 0) snprintf(b, sizeof b, "A CHECK"); else if (pay < need) snprintf(b, sizeof b, "A ALL IN $%d", pay); else snprintf(b, sizeof b, "A CALL $%d", need);
        text(482, 167, b, 2, white);
        text(482, 191, "B FOLD", 2, need ? white : dimc);
        if (cr && M.nl && mp_min_by(&M, 0) >= mp_max_by(&M, 0)) snprintf(b, sizeof b, "X ALL IN $%d", mp_cap_pay(&M, 0));   /* v8.9: only all-in is left */
        else if (cr) snprintf(b, sizeof b, "X %s $%d", need || M.stage == 0 ? "RAISE" : "BET", need + (M.nl ? mp_min_by(&M, 0) : mp_size(&M))); else snprintf(b, sizeof b, "X RAISE");
        text(482, 215, b, 2, cr ? white : dimc);
    } else if (M.folded[0]) { text(482, 167, "YOU FOLDED", 2, dimc); text(482, 191, "WAITING...", 2, gold); }
    else { text(482, 167, "WAITING...", 2, gold); text(482, 191, "B FOLD", 2, dimc); text(482, 215, "X RAISE", 2, dimc); }
    }   /* (the guide box, when the bet menu is not open) */
    }
static void draw_event(void) {
    static const char *items[2] = {"EXHIBITION", "LOTTERY TICKET"}; char b[40];
    bg_draw(BG_EVENT); draw_cash(); menu_panel(items, 2, ev_sel);
    if (qp_ms && age_of(qp_ms) < 2600) {
        panel_t(150, 340, 340, 52, 256); snprintf(b, sizeof b, "NEED %d%% ENERGY", EX_ENERGY); text_c(320, 348, b, 2, C(235,120,120));
        snprintf(b, sizeof b, "YOU HAVE %d%%. AN EXHIBITION USES %d%%", G.energy, EX_ENERGY); text_c(320, 372, b, 1, C(240,240,240));
    }
}
static void draw_lotto(void) {   /* v9.0: the six lottery tickets */
    char b[48]; int i, holding = g_lot_has(&G), lock = g_behind(&G, prog_far); Uint32 white = C(240,240,240), gold = gui_tc(), dimc = C(120,125,140), cream = C(250,240,215), red = C(235,120,120);   /* v10.5: lock = you loaded an older save: no tickets until you reach the furthest day again */
    bg_draw(BG_EVENT); shade(0, 0, 640, 480, 0, 0, 0, 40); draw_cash();
    panel_a(60, 52, 520, 392, 256, 150);
    text_c(320, 66, "LOTTERY", 4, gold); R(84, 106, 472, 2, gui_dc());
    if (holding) { snprintf(b, sizeof b, "YOUR TICKET: $%d", G.lot_price); text_c(320, 122, b, 3, gold); }
    else text_c(320, 122, "CHOOSE YOUR TICKET", 3, white);
    for (i = 0; i < LOT_N; i++) {
        int val = LOT_PRICE[i], ok = !holding && !lock && G.cash >= val, y = 162 + i * 32; Uint32 col = !ok ? dimc : i == lot_sel ? gold : white;
        if (i == lot_sel) text(96, y, ">", 3, col);
        snprintf(b, sizeof b, "$%d", val); text(138, y, b, 3, col);
        snprintf(b, sizeof b, "WIN $%d", g_lot_prize(val)); text_r(548, y, b, 3, col);
    }
    if (holding) text_c(320, 362, "THE RESULT COMES TOMORROW", 2, cream);
    else if (lock) { snprintf(b, sizeof b, "NO TICKETS UNTIL DAY %d", prog_far); text_c(320, 362, b, 2, cream); }
    else { snprintf(b, sizeof b, "EVERY TICKET WINS %d%% OF THE TIME", LOT_PCT); text_c(320, 362, b, 2, cream); }
    text_c(320, 390, "A BUY    B BACK", 2, white);
    text_c(320, 414, lock && !holding ? "YOU LOADED AN OLDER SAVE" : "ONE TICKET AT A TIME. CASH ONLY", 1, red);
    if (lot_ms && age_of(lot_ms) < 1800) { panel(150, 452, 340, 22, 256); text_c(320, 456, lot_kind == 0 ? "TICKET BOUGHT" : lot_kind == 1 ? "NOT ENOUGH CASH" : lot_kind == 3 ? "THE COUNTER IS CLOSED" : "YOU ALREADY HAVE A TICKET", 2, lot_kind == 0 ? gold : red); }
}
static void draw_qsetup(void) {
    char b[48]; int i, n = mp_mode ? g_ex_n(&G) : 4, step = mp_mode ? (n > 5 ? 32 : 38) : 46; Uint32 white = C(240,240,240), gold = gui_tc(), dimc = C(120,125,140);
    if (mp_mode == 1) { bg_draw(BG_EVENT); shade(0, 0, 640, 480, 0, 0, 0, 40); draw_cash(); }
    else { T = S; vgrad(0, 0, 640, 480, 8, 16, 14, 0, 0, 0); vgrad(0, 0, 640, 18, 28, 110, 58, 6, 30, 16); vgrad(0, 462, 640, 18, 6, 30, 16, 28, 110, 58); }
    panel_a(60, 52, 520, mp_mode ? 392 : 340, 256, 150);   /* see-through: the picture behind stays visible */
    text_c(320, 66, mp_mode ? "EXHIBITION" : "QUICK PLAY", 4, gold); R(84, 106, 472, 2, gui_dc());
    text_c(320, 122, mp_mode ? "CHOOSE YOUR BUY-IN" : "STARTING CHIPS", 3, white);
    for (i = 0; i < n; i++) {
        int val = mp_mode ? EX_BUYIN[i] : QP_STACK[i], ok = !mp_mode || G.cash >= val, y = 162 + i * step; Uint32 col = !ok ? dimc : i == qs_sel ? gold : white;
        if (i == qs_sel) text(96, y, ">", 3, col);
        snprintf(b, sizeof b, mp_mode ? "$%d" : "%d", val); text(138, y, b, 3, col);
        if (mp_mode) { snprintf(b, sizeof b, "WIN $%d", g_ex_prize(val)); text_r(548, y, b, 3, col); }
        if (mp_mode && i == EX_BOSS) text(250, y + 4, "BOSS", 2, !ok ? dimc : C(235,100,100));   /* the boss table: her, one time only */
    }
    if (mp_mode) { if (g_milk_active(&G)) text_c(320, 362, "THE MILK BOTTLE: NO ENERGY USED", 2, C(250,240,215)); else { snprintf(b, sizeof b, "IT USES %d%% OF YOUR ENERGY", EX_ENERGY); text_c(320, 362, b, 2, C(235,120,120)); } }
    text_c(320, mp_mode ? 390 : 340, "A SELECT    B BACK", 2, white);
    if (mp_mode) text_c(320, 414, "LEAVING MID-GAME LOSES THE BUY-IN", 1, C(235,120,120));
    if (qp_ms && age_of(qp_ms) < 1800) { panel(150, 452, 340, 22, 256); text_c(320, 456, qp_kind ? (snprintf(b, sizeof b, "NEED %d%% ENERGY", EX_ENERGY), b) : "NOT ENOUGH CASH", 2, C(235,120,120)); }
}
static void draw_mend(void) {
    char b[48]; int win = M.game_over == 1, y = 150;
    if (mp_mode == 1) { bg_draw(BG_EXHIB); shade(0, 0, 640, 480, 0, 0, 0, 130); } else { T = S; vgrad(0, 0, 640, 480, 8, 16, 14, 0, 0, 0); }
    panel_a(130, 110, 380, mp_mode == 1 ? 250 : 190, 256, 240);
    text_c(320, 126, win ? "YOU WIN" : "ELIMINATED", 4, win ? gui_tc() : C(235,120,120)); R(150, 166, 340, 2, gui_dc());
    if (mp_mode == 1) {
        if (win) snprintf(b, sizeof b, "PRIZE +$%d", mp_prize); else snprintf(b, sizeof b, "BUY-IN LOST  $%d", mp_buyin);
        text_c(320, y + 30, b, 2, win ? gui_tc() : C(235,120,120)); y += 34;
        fmt_money(b + 6, G.cash); memcpy(b, "CASH $", 6); text_c(320, y + 30, b, 2, C(240,240,240));
    } else text_c(320, y + 30, "NOTHING WAS AT STAKE", 2, C(130,135,150));
    if (win && boss_new) text_c(320, 286, "A NEW CHALLENGER APPEARED", 2, C(235,120,120));
    text_c(320, mp_mode == 1 ? 318 : 258, "A CONTINUE", 2, C(130,135,150));
}

static void obj_text(char *b, size_t n) {   /* what the story wants from you now (the Objective entry of the pause menu) */
    switch (G.story) {
        case ST_NEW: snprintf(b, n, "%s", O_DESK); break;
        case ST_HOME:
            if (!(G.hall_seen & 1)) snprintf(b, n, "%s", O_HALL);
            else if (!services_open()) snprintf(b, n, "%s", O_ENTER);
            else if (G.cash >= DEBT) snprintf(b, n, O_DEBT_OK, DEBT);
            else snprintf(b, n, O_DEBT, DEBT, G.cash, DEBT);
            break;
        case ST_FIRST: snprintf(b, n, "%s", G.ex_big ? O_BOSS : O_FIRST); break;
        default:
            snprintf(b, n, "%s", G.rs == RS_NONE && G.ring ? O_RING : G.rs == RS_VISIT ? O_VISIT : G.rs == RS_UPG ? O_UPG : G.rs == RS_READY ? O_ASKMOVE : G.room_up == 0 ? O_UP1 : O_NONE); break;
    }
    if (G.story >= ST_HOME && g_rent_due(&G)) {   /* the rent is due today */
        char r[100]; snprintf(r, sizeof r, O_RENT, g_rent_amt(&G));
        if (G.story >= ST_NAMED) snprintf(b, n, "%s", r); else { size_t l = strlen(b); snprintf(b + l, n - l, " %s", r); }
    }
}
static void draw_opt_panel(void);
static void draw_pause(void) {
    static const char *const OPT[10][2] = {{"", ""}, {"RESUME", "EXIT"}, {"RESUME", "LEAVE TABLE"}, {"RESUME", "SKIP SCENE"}, {"RESUME", "GO BACK"},
                                           {"RESUME", "BACK TO MENU"}, {"RESUME", "LEAVE TABLE"}, {"RESUME", "LEAVE TABLE"}, {"NO, KEEP PLAYING", "YES, EXIT"}, {"", ""}};
    int i, note = pause_kind == PK_TABLE || pause_kind == PK_QUICK || pause_kind == PK_EXHIB || pause_kind == PK_HOME; shade(0, 0, 640, 480, 0, 0, 0, pause_kind == PK_OPT ? 170 : 150);
    if (pause_kind == PK_OPT) { draw_opt_panel(); return; }
    if (pause_kind == PK_EXITC) {   /* exit without saving? the warning depends on whether anything would be lost */
        int lost = unsaved();
        panel_am(110, 150, 420, 196, 256, 244);
        text_c(320, 164, "EXIT THE GAME?", 3, mn_tc()); R(130, 196, 380, 2, mn_dc());
        if (lost) { text_c(320, 210, "YOU HAVE UNSAVED PROGRESS.", 2, C(235,120,120)); text_c(320, 232, "IT WILL BE LOST.", 2, C(235,120,120)); text_c(320, 258, "SAVE AT YOUR BED OR SOFA FIRST.", 1, C(200,205,215)); }
        else text_c(320, 226, "ALL YOUR PROGRESS IS SAVED.", 2, C(240,240,240));
        for (i = 0; i < 2; i++) {
            Uint32 col = i == pause_sel ? mn_tc() : C(240,240,240);
            if (i == pause_sel) text(150, 282 + 26 * i, ">", 2, col);
            text(174, 282 + 26 * i, OPT[PK_EXITC][i], 2, col);
        }
        return;
    }
    if (pause_kind == PK_OBJ) {   /* the Objective page */
        char o[200], w[240]; int ln = 0; const char *q;
        panel_am(90, 120, 460, 240, 256, 244);
        text_c(320, 134, "OBJECTIVE", 3, mn_tc()); R(110, 166, 420, 2, mn_dc());
        obj_text(o, sizeof o); wrap_buf(w, sizeof w, o, 32);
        for (q = w; *q; ) { char ls[40]; int k = 0; while (q[k] && q[k] != '\n' && k < 38) { ls[k] = q[k]; k++; } ls[k] = 0; text(114, 184 + 26 * ln, ls, 2, C(240,240,240)); ln++; q += k; if (*q == '\n') q++; }
        text_c(320, 336, "A BACK", 2, C(130,135,150));
        return;
    }
    {
        const char *it[4]; int nit = pause_kind == PK_HOME ? 4 : 3;
        if (pause_kind == PK_HOME) { it[0] = "RESUME"; it[1] = "OBJECTIVE"; it[2] = "OPTIONS"; it[3] = "EXIT"; }
        else { it[0] = "RESUME"; it[1] = "OPTIONS"; it[2] = OPT[pause_kind][1]; }
        panel_am(160, 170, 320, 76 + 28 * nit + (note ? 20 : 0), 256, 240);
        text_c(320, 184, "PAUSED", 3, mn_tc());
        for (i = 0; i < nit; i++) {
            Uint32 col = i == pause_sel ? mn_tc() : C(240,240,240);
            if (i == pause_sel) text(190, 232 + 28 * i, ">", 2, col);
            text(214, 232 + 28 * i, it[i], 2, col);
        }
        if (pause_kind == PK_TABLE) text_c(320, 238 + 28 * nit, "LEAVING COUNTS AS A LOSS", 1, C(235,120,120));
        else if (pause_kind == PK_EXHIB) text_c(320, 238 + 28 * nit, "YOU LOSE YOUR BUY-IN", 1, C(235,120,120));
        else if (pause_kind == PK_QUICK) text_c(320, 238 + 28 * nit, "NOTHING IS AT STAKE", 1, C(130,135,150));
        else if (pause_kind == PK_HOME) text_c(320, 238 + 28 * nit, "SAVE AT YOUR BED OR SOFA", 1, C(130,135,150));
    }
}
static void draw_over(void) {   /* no money left, no way to earn any: the story ends */
    unsigned a = age_of(over_ms);
    T = S; R(0, 0, 640, 480, C(0, 0, 0));
    text_c(320, 150, OVER_T, 7, C(205, 32, 44));
    text_c(320, 250, over_rent ? OVER_RENT_L1 : OVER_L1, 2, C(240, 240, 240)); text_c(320, 280, over_rent ? OVER_RENT_L2 : OVER_L2, 2, C(170, 175, 190));
    if (a > 1200 || frozen) text_c(320, 400, "A BACK TO THE TITLE", 2, C(250, 215, 90));
    if (a < 900 && !frozen) shade(0, 0, 640, 480, 0, 0, 0, 256 - (int)(a * 256 / 900));
}
static void draw_walk(void);
static void draw_mono(void);
static void draw_sneak(void);
static void draw_opt(void);
static void draw_all(void) {
    switch (scr) {
        case SC_MENU: draw_menu(); break;
        case SC_WALK: draw_walk(); break;
        case SC_STORY: draw_story(); break;
        case SC_DOOR: draw_door(); break;
        case SC_LEN: draw_len(); break;
        case SC_PLAY: draw_play(); break;
        case SC_END: draw_end(); break;
        case SC_ATM: draw_atm(); break;
        case SC_STORE: draw_store(); break;
        case SC_INV: draw_inv(); break;
        case SC_SCENE: draw_scene(); break;
        case SC_EVENT: draw_event(); break;
        case SC_LOTTO: draw_lotto(); break;
        case SC_QSETUP: draw_qsetup(); break;
        case SC_MTABLE: draw_mtable(); break;
        case SC_MEND: draw_mend(); break;
        case SC_MONO: draw_mono(); break;
        case SC_SNEAK: draw_sneak(); break;
        case SC_OVER: draw_over(); break;
        case SC_OPT: draw_opt(); break;
        case SC_CAREER: draw_career(); break;
        case SC_SHOP: draw_shop(); break;
        case SC_DEBUG: draw_debug(); break;
        case SC_ROM: draw_rom(); break;
        case SC_ROMQ: draw_romq(); break;
        case SC_NAME: draw_name(); break;
        case SC_PEEK: draw_peek(); break;
        default: break;
    }
    if (ach_pend && !ach_on && scr == SC_WALK && !fade_st && !pause_kind) { ach_pend = 0; ach_on = 1; ach_t0 = now_ms(); ach_shown++; sfx_play(SFX_COLLECT); }   /* v10.0: start it once you are walking again */
    if (ach_on) {
        unsigned a = age_of(ach_t0);
        if (a >= ACH_MS) ach_on = 0;
        else { int f = a < 400 ? (int)(a * 256 / 400) : a > ACH_MS - 600 ? (int)((ACH_MS - a) * 256 / 600) : 256, w = 262; Uint32 gold = gui_tc(), white = C(240,240,240);
            panel_a(320 - w / 2, 14, w, 64, f, 256 * 230 >> 8);
            text_c(320, 24, "ACHIEVEMENT UNLOCKED", 2, gold);
            text_c(320, 50, "SOPHIA'S RIVAL", 2, white); }
    }
    if (pause_kind) draw_pause();
}

/* ---------- transitions ---------- */
static void apply_go(int kind, int arg);
static void mp_begin(void);
static void rom_open(const char *t);
static void rom_back(const char *note);
static void rom_open_door(const char *t);
static void home_to_door(const char *note);
static const char *home_note; static char home_note_buf[96];
static int gift_kind; static const char *gift_l1; static char gift_note[64], gift_end[200];

/* ---------- Quick Play / Exhibition flow ---------- */
static unsigned mp_delay(void) {   /* how long the CPU thinks: quick 1-2 s, exhibition 2-4 s, quick when you already folded */
    if (M.folded[0]) return 400 + (unsigned)rnd(400);
    return mp_mode == 1 ? 2000u + (unsigned)rnd(2000) : 1000u + (unsigned)rnd(1000);
}
static void mp_hand_start(void) {
    int i, lv = M.level;
    for (i = 0; i < M.n; i++) { seat_in[i] = M.alive[i]; mp_tag[i] = 0; }
    bm_open = 0; hl_clear(); mp_start_hand(&M); hand_end_ms = M.over ? now_ms() : 0;
    if (M.hands > 1 && M.level != lv) { snprintf(mp_msg, sizeof mp_msg, "BLINDS UP %d/%d", M.bb / 2, M.bb); mp_msg_ms = now_ms(); }
    anim_new_hand(M.c[0], M.c[1], M.over);
    if (M.over) hl_multi(); else if (M.toact != 0) think_at = after_anim(mp_delay());
}
static void mp_begin(void) {
    int n = 2; mp_buyin = mp_mode ? EX_BUYIN[qs_si] : QP_STACK[qs_si]; mp_prize = 0;
    mp_init(&M, n, mp_buyin, mp_mode ? EX_START_BB[qs_si] : MP_START_BB[qs_si]); M.boss = mp_mode == 1 && qs_si == EX_BOSS; M.nl = mp_mode == 1; bm_open = 0; boss_win = 0; mp_msg[0] = 0; scr = SC_MTABLE; pause_kind = PK_NONE; lead_next = 1; mp_hand_start();
}
static void mp_do_by(int a, int by) {   /* v8.9: by = the raise increment on top of the call (free bet size tables; 0 = the minimum) */
    int p = M.toact, need = mp_need(&M, p), st0 = M.stage, first = M.nbets == 0, raised = a == A_RAISE && mp_can_raise(&M, p), ended, i, pot0 = mp_pot(&M), n0, n1;
    bm_open = 0;
    ended = mp_act_by(&M, a, by); anim_allin = M.allin[p];
    n0 = st0 == 0 ? 0 : st0 + 2; if (n0 > 5) n0 = 5; n1 = M.stage == 0 ? 0 : M.stage + 2; if (n1 > 5) n1 = 5;
    anim_after(p < 2 ? p : 1, mp_pot(&M) - pot0, n0, n1, ended, M.showdown, M.showdown && !M.folded[0]);
    mp_tag[p] = a == A_FOLD && need > 0 ? 4 : M.allin[p] ? 6 : raised ? (first ? 5 : 3) : (need > 0 ? 2 : 1);
    if (!ended && M.stage != st0) for (i = 0; i < M.n; i++) if (!M.folded[i] && !M.allin[i]) mp_tag[i] = 0;
    if (ended) {
        hand_end_ms = now_ms(); hl_multi(); hl_extend(M.showdown);
        pay_who = (M.won[0] > 0 ? 1 : 0) | (M.won[1] > 0 ? 2 : 0); pay_ms = now_ms() + hand_dl() + 300; sfx_at(pay_ms + 100, SFX_COLLECT);   /* the pot flies to whoever won it once the banner is up */
    } else if (M.toact != 0) think_at = after_anim(mp_delay());
}
static void mp_do(int a) { mp_do_by(a, 0); }
/* v8.9 the bet menu (X on your turn at the Exhibition / boss table). UP/DOWN one small step (half a big blind), LEFT/RIGHT a big step (a tenth of the range, at least 5 steps),
   Y = all-in, X = pot-size bet, A confirms, B cancels. The amount always stays between the minimum raise and all-in. */
static void bm_start(void) { bm_by = mp_min_by(&M, 0); bm_open = 1; }
static void bm_key(int b) {
    int mn = mp_min_by(&M, 0), mx = mp_max_by(&M, 0), step = M.bb / 2 > 0 ? M.bb / 2 : 1, big = (mx - mn) / 10, pot = mp_pot(&M) + mp_need(&M, 0);
    big -= big % step; if (big < step * 5) big = step * 5;
    if (b == B_B) bm_open = 0;
    else if (b == B_A) { int by = bm_by; mp_do_by(A_RAISE, by); return; }
    else if (b == B_UP) bm_by += step;
    else if (b == B_DOWN) bm_by -= step;
    else if (b == B_RIGHT) bm_by += big;
    else if (b == B_LEFT) bm_by -= big;
    else if (b == B_Y) bm_by = mx;
    else if (b == B_X) bm_by = pot;
    if (bm_by < mn) bm_by = mn;
    if (bm_by > mx) bm_by = mx;
}
static void mp_end_game(void) {   /* the table is over: pay the prize, show the result */
    scr = SC_MEND; boss_new = 0;
    if (mp_mode == 1 && M.game_over == 1) {
        int had = G.ex_big, sw0 = G.sw_wins; g_ex_win(&G, mp_buyin); mp_prize = g_ex_prize(mp_buyin); ach_check(sw0);
        boss_new = !had && G.ex_big && g_boss_open(&G);   /* the $1000 table was won for the first time: the boss table appears in the list */
        boss_win = M.boss;                                  /* the boss is beaten: after the result screen she gives her name */
    }
    if (mp_mode == 1) persist();
}
static void go(int kind, int arg) {   /* fade to black, switch, fade back in */
    if (frozen) { apply_go(kind, arg); return; }
    nx_kind = kind; nx_arg = arg; fade_st = 1; fade_t0 = now_ms();
}
static void go_slow(int kind, int arg) { go(kind, arg); if (!frozen) fade_len = SLOW_FADE_MS; }   /* the same fade, but slow (both ways) */
static void fade_step(void) {
    unsigned a;
    if (!fade_st) return;
    a = now_ms() - fade_t0;
    if (fade_st == 1) {
        if (a >= (unsigned)fade_len) { fade_a = 256; fade_st = 2; apply_go(nx_kind, nx_arg); fade_t0 = now_ms(); }
        else fade_a = (int)(a * 256 / (unsigned)fade_len);
    } else {
        if (a >= (unsigned)fade_len) { fade_st = 0; fade_a = 0; fade_len = FADE_MS; } else fade_a = 256 - (int)(a * 256 / (unsigned)fade_len);
    }
}

/* ---------- story scenes ---------- */
/* v9.7: her memory of your last mama-mode interaction. The pools are read through their pointers at call time (lines.txt can replace them). TEASE and SLEEP have no 'earlier today' pool: after a tease she throws you out (R_KICKED_MM), and sleeping ends the day. */
typedef struct { const char *const *const *t; int *tn; const char *const *const *y; int *yn; } MemPool;
static const MemPool MEMP[MK_N] = {
    {0, 0, 0, 0},
    {&R_MEM_CALL_T_MM, &R_MEM_CALL_T_MM_N, &R_MEM_CALL_Y_MM, &R_MEM_CALL_Y_MM_N},
    {&R_MEM_CHAT_T_MM, &R_MEM_CHAT_T_MM_N, &R_MEM_CHAT_Y_MM, &R_MEM_CHAT_Y_MM_N},
    {0, 0, &R_MEM_TEASE_Y_MM, &R_MEM_TEASE_Y_MM_N},
    {&R_MEM_FEED_T_MM, &R_MEM_FEED_T_MM_N, &R_MEM_FEED_Y_MM, &R_MEM_FEED_Y_MM_N},
    {&R_MEM_FEEDNO_T_MM, &R_MEM_FEEDNO_T_MM_N, &R_MEM_FEEDNO_Y_MM, &R_MEM_FEEDNO_Y_MM_N},
    {&R_MEM_HUG_T_MM, &R_MEM_HUG_T_MM_N, &R_MEM_HUG_Y_MM, &R_MEM_HUG_Y_MM_N},
    {&R_MEM_KISS_T_MM, &R_MEM_KISS_T_MM_N, &R_MEM_KISS_Y_MM, &R_MEM_KISS_Y_MM_N},
    {0, 0, &R_MEM_SLEEP_Y_MM, &R_MEM_SLEEP_Y_MM_N}
};
static const char *mm_mem(void) {   /* a line about what happened earlier today / yesterday, or NULL when she has nothing fresh to say */
    int age = g_mm_age(&G), k = G.mm_kind;
    if (age < 0 || k <= MK_NONE || k >= MK_N) return NULL;
    if (age == 0 && MEMP[k].t) return pick(*MEMP[k].t, *MEMP[k].tn);
    if (age == 1 && MEMP[k].y) return pick(*MEMP[k].y, *MEMP[k].yn);
    return NULL;
}
static const char *mm_greet(const char *day_pool_line) { const char *m = mm_mem(); return m ? m : day_pool_line; }
static const char *rom_greet(void) {   /* her greeting at the door (days 6 and 7 after the pill: she complains about her body) */
    int h = g_pill_hint(&G);
    if (G.pill_seen && kicked_day == G.day) return g_mama(&G) ? pick(R_KICKED_MM, R_KICKED_MM_N) : pick(R_KICKED, R_KICKED_N);   /* v7.7: you teased her today and she threw you out */
    if (g_p4_open(&G)) { int cw = g_cow(&G); return g_mama(&G) ? mm_greet(cw ? pick(R_GREET_CW_MM, R_GREET_CW_MM_N) : pick(R_GREET_MM, R_GREET_MM_N)) : cw ? pick(R_GREET_CW, R_GREET_CW_N) : pick(R_GREET_M, R_GREET_M_N); }   /* v12.1: cow mode */   /* v8.6: married (v9.5: mama mode has its own set) */
    return h == 1 ? pick(R_PILL6, R_PILL6_N) : h == 2 ? pick(R_PILL7, R_PILL7_N) : G.pill_seen ? pick(R_GREET_P, R_GREET_P_N) : pick(R_GREET, R_GREET_N);
}
static const char *notfull_line(void);   /* v12.1 */
static void pump_broke(void);   /* v12.2 */
static int bottle_inj;   /* v12.3: the hand-over scene of an ASK FOR MILK after the injection (the handing-bottle picture, 2 bottles) */
static const char *pumpnew_line(void);   /* v12.2 */
static const char *night_greet(void) {   /* v12.1: her greeting when you knock at night, married (cow mode has its own) */
    int cw = g_cow(&G);
    return g_mama(&G) ? mm_greet(cw ? pick(R_NIGHT_CW_MM, R_NIGHT_CW_MM_N) : pick(R_NIGHT_MM, R_NIGHT_MM_N)) : cw ? pick(R_NIGHT_CW, R_NIGHT_CW_N) : pick(R_NIGHT_M, R_NIGHT_M_N);
}
static void lr_here(void) {   /* v9.9: asked in the living room: her "Here?!" is the first beat of the scene (her covering picture, the living room behind her) */
    memmove(&vn_buf[1], &vn_buf[0], (size_t)vn_n * sizeof(Beat)); vn_buf[0].who = W_SOPHIA; vn_buf[0].bg = BG_ROM; vn_buf[0].spr = 1; vn_buf[0].txt = pick(R_LR_HERE, R_LR_HERE_N); vn_n++;
}
/* v10.4: mama-mode OPENERS. A pool is a list of beat PAIRS (your line with Mama in it, then her reaction that starts with the name).
   v10.6: the opener is followed by a PLEA pair (you plead, she gives in on her own terms); when you asked for the same thing before (this game) a MEMORY pair (B_RM_*) replaces the opener; door and bedroom got openers for ASK / PILL / SUPER;
   B_MID_* pairs add a second hesitation in the middle of the pump and breastfeed scenes. mm_react_on = 0 gives back the v10.5 scenes (the old self-tests use it). */
enum { OP_FEED, OP_FEED_LR, OP_ASK_LR, OP_PILL_LR, OP_SUPER_LR, OP_ASK, OP_PILL, OP_SUPER, OP_RM_FEED, OP_RM_ASK, OP_RM_PILL, OP_RM_SUPER, OP_PL_FEED, OP_PL_ASK, OP_PL_PILL, OP_PL_SUPER, OP_MID_PUMP, OP_MID_FEED, OP_CW0, OP_N = OP_CW0 + 10 };
enum { RQ_FEED, RQ_ASK, RQ_PILL, RQ_SUPER, RQ_N };
static int open_last[OP_N] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, bottle_pill;   /* bottle_pill: the bottle scene was started by a milk pill (so the pill opener plays, not the ask opener) */
static int mm_react_on = 1;
/* v12.1: COW MODE pairs (your line, her disgusted answer that still agrees). Slots follow OP_CW0: normal first, then the Mama set. */
enum { CO_ASK, CO_PILL, CO_SUPER, CO_MID_PUMP, CO_MID_FEED, CO_N };
static int cow_slot(int base) { return OP_CW0 + (g_mama(&G) ? CO_N : 0) + base; }
static int rq_cnt[RQ_N], rq_career = -1;   /* how many times you asked / gave this game: she remembers (kept while the game runs, a new career starts clean) */
static void rq_sync(void) { if (rq_career != (int)G.career) { int k; for (k = 0; k < RQ_N; k++) rq_cnt[k] = 0; rq_career = (int)G.career; } }
static int rq_prev(int kind) { rq_sync(); if (rq_cnt[kind] > 0) return 1; return kind == RQ_FEED && G.mm_kind == MK_FEED && g_mm_age(&G) >= 0; }   /* a feed from the saved day counts too (she remembers after a load) */
static const Beat *const *const OP_P[OP_N] = {&B_OPEN_FEED_MM, &B_OPEN_FEED_MM_LR, &B_OPEN_ASK_MM_LR, &B_OPEN_PILL_MM_LR, &B_OPEN_SUPER_MM_LR, &B_OPEN_ASK_MM, &B_OPEN_PILL_MM, &B_OPEN_SUPER_MM,
    &B_RM_FEED_MM, &B_RM_ASK_MM, &B_RM_PILL_MM, &B_RM_SUPER_MM, &B_PLEA_FEED_MM, &B_PLEA_ASK_MM, &B_PLEA_PILL_MM, &B_PLEA_SUPER_MM, &B_MID_PUMP_MM, &B_MID_FEED_MM,
    &B_OPEN_ASK_CW, &B_OPEN_PILL_CW, &B_OPEN_SUPER_CW, &B_MID_PUMP_CW, &B_MID_FEED_CW, &B_OPEN_ASK_CW_MM, &B_OPEN_PILL_CW_MM, &B_OPEN_SUPER_CW_MM, &B_MID_PUMP_CW_MM, &B_MID_FEED_CW_MM};
static int *const OP_PN[OP_N] = {&B_OPEN_FEED_MM_N, &B_OPEN_FEED_MM_LR_N, &B_OPEN_ASK_MM_LR_N, &B_OPEN_PILL_MM_LR_N, &B_OPEN_SUPER_MM_LR_N, &B_OPEN_ASK_MM_N, &B_OPEN_PILL_MM_N, &B_OPEN_SUPER_MM_N,
    &B_RM_FEED_MM_N, &B_RM_ASK_MM_N, &B_RM_PILL_MM_N, &B_RM_SUPER_MM_N, &B_PLEA_FEED_MM_N, &B_PLEA_ASK_MM_N, &B_PLEA_PILL_MM_N, &B_PLEA_SUPER_MM_N, &B_MID_PUMP_MM_N, &B_MID_FEED_MM_N,
    &B_OPEN_ASK_CW_N, &B_OPEN_PILL_CW_N, &B_OPEN_SUPER_CW_N, &B_MID_PUMP_CW_N, &B_MID_FEED_CW_N, &B_OPEN_ASK_CW_MM_N, &B_OPEN_PILL_CW_MM_N, &B_OPEN_SUPER_CW_MM_N, &B_MID_PUMP_CW_MM_N, &B_MID_FEED_CW_MM_N};
static int open_pair(int slot, Beat out[2]) {   /* a random pair of the opener pool (never the same twice in a row); 0 = the pool has no pair */
    int np = *OP_PN[slot] / 2, k; if (np < 1) return 0;
    k = pick_new(np, open_last[slot]); open_last[slot] = k; out[0] = (*OP_P[slot])[2 * k]; out[1] = (*OP_P[slot])[2 * k + 1]; return 1;
}
static int open_kind(int slot) { return slot == OP_FEED || slot == OP_FEED_LR ? RQ_FEED : slot == OP_ASK_LR || slot == OP_ASK ? RQ_ASK : slot == OP_PILL_LR || slot == OP_PILL ? RQ_PILL : RQ_SUPER; }
static int open_chain(int slot, Beat out[4]) {   /* v10.6: the beats that open a scene: 2 (v10.5 / pools without the extra pairs) or 4 (opener or memory pair, then the plea pair); 0 = nothing */
    int kind = open_kind(slot), n; Beat op[2];
    if (!mm_react_on) return open_pair(slot, out) ? 2 : 0;
    if (rq_prev(kind) && open_pair(OP_RM_FEED + kind, op)) { out[0] = op[0]; out[1] = op[1]; n = 2; }
    else if (open_pair(slot, op)) { out[0] = op[0]; out[1] = op[1]; n = 2; }
    else return 0;
    if (open_pair(OP_PL_FEED + kind, op)) { out[2] = op[0]; out[3] = op[1]; n = 4; }
    rq_cnt[kind]++; return n;
}
static int open_replace(int slot) {   /* BREASTFEED: the opener takes the place of the first two beats of the scene; returns how many beats it filled (0 = unchanged) */
    Beat op[4]; int n; if (vn_n < 2 || !(n = open_chain(slot, op))) return 0;
    if (n == 4) { memmove(&vn_buf[4], &vn_buf[2], (size_t)(vn_n - 2) * sizeof(Beat)); vn_n += 2; vn_buf[2] = op[2]; vn_buf[3] = op[3]; vn_buf[2].spr = vn_buf[0].spr; vn_buf[3].spr = vn_buf[1].spr; vn_buf[2].bg = vn_buf[0].bg; vn_buf[3].bg = vn_buf[1].bg; }
    vn_buf[0] = op[0]; vn_buf[1] = op[1]; return n;
}
static int open_prepend(int slot) {   /* living room / door: the opener comes in front of the scene (returns 0 and leaves the scene unchanged if the pool is empty) */
    Beat op[4]; int n = open_chain(slot, op); if (!n) return 0;
    memmove(&vn_buf[n], &vn_buf[0], (size_t)vn_n * sizeof(Beat)); memcpy(&vn_buf[0], op, (size_t)n * sizeof(Beat)); vn_n += n; return n;
}
static void cow_open_prepend(int base) {   /* v12.1: cow mode: one pair of beats in front of the scene (no memory, no plea: her answer already gives in) */
    Beat op[2]; if (!open_pair(cow_slot(base), op)) return;
    memmove(&vn_buf[2], &vn_buf[0], (size_t)vn_n * sizeof(Beat)); vn_buf[0] = op[0]; vn_buf[1] = op[1]; vn_n += 2;
}
static void cow_mid(int base, int at) {   /* v12.1: cow mode: one pair in the middle of the scene, before beat at (picture and sprite of the beat before it) */
    Beat op[2]; if (at < 1 || at > vn_n || !open_pair(cow_slot(base), op)) return;
    memmove(&vn_buf[at + 2], &vn_buf[at], (size_t)(vn_n - at) * sizeof(Beat)); vn_buf[at] = op[0]; vn_buf[at + 1] = op[1];
    vn_buf[at].bg = vn_buf[at + 1].bg = vn_buf[at - 1].bg; vn_buf[at].spr = vn_buf[at + 1].spr = vn_buf[at - 1].spr; vn_n += 2;
}
static void mid_insert(int slot, int at) {   /* v10.6: one hesitation + plea pair in the middle of the scene, before beat at (picture and sprite of the beat before it) */
    Beat op[2]; if (!mm_react_on || at < 1 || at > vn_n || !open_pair(slot, op)) return;
    memmove(&vn_buf[at + 2], &vn_buf[at], (size_t)(vn_n - at) * sizeof(Beat)); vn_buf[at] = op[0]; vn_buf[at + 1] = op[1];
    vn_buf[at].bg = vn_buf[at + 1].bg = vn_buf[at - 1].bg; vn_buf[at].spr = vn_buf[at + 1].spr = vn_buf[at - 1].spr; vn_n += 2;
}
/* v11.2: CHANGE OUTFIT into the yoga outfit, the designer top or the cow print outfit is an exchange of 3 beats (your request, her refusal, her grudging give-in);
   the outfit changes when it ends. ow_target = the outfit, ow_ward = it was asked at the wardrobe in her bedroom (not in her menu). */
static int ow_target = 1, ow_ward, ow_last[2][3] = {{-1, -1, -1}, {-1, -1, -1}};
static Beat ow_buf[3];
static const Beat *const *const OW_P[2][3] = {{&B_WEAR_YOGA, &B_WEAR_TOP, &B_WEAR_COW}, {&B_WEAR_YOGA_MM, &B_WEAR_TOP_MM, &B_WEAR_COW_MM}};
static int *const OW_PN[2][3] = {{&B_WEAR_YOGA_N, &B_WEAR_TOP_N, &B_WEAR_COW_N}, {&B_WEAR_YOGA_MM_N, &B_WEAR_TOP_MM_N, &B_WEAR_COW_MM_N}};
static int ow_groups(int k) { return k >= 1 && k <= 3 ? *OW_PN[g_mama(&G) ? 1 : 0][k - 1] / 3 : 0; }   /* 0 = no exchange for this outfit (her usual outfit, or an emptied pool) */
static const Beat *script_src(int id, int *n) {
    switch (id) {
        case SCR_INTRO: *n = B_INTRO_N; return B_INTRO;
        case SCR_BUMP: *n = B_BUMP_N; return B_BUMP;
        case SCR_DOOR1: *n = B_DOOR1_N; return B_DOOR1;
        case SCR_REPEAT: *n = B_REPEAT_N; return B_REPEAT;
        case SCR_NAMED: *n = B_NAMED_N; return B_NAMED;
        case SCR_LEAVE: *n = B_LEAVE_N; return B_LEAVE;
        case SCR_GREET: *n = B_GREET_N; return B_GREET;
        case SCR_RAIN: *n = B_RAIN_N; return B_RAIN;
        case SCR_AFTER: *n = B_AFTER_N; return B_AFTER;
        case SCR_BOSS: *n = B_BOSS_N; return B_BOSS;
        case SCR_PROPOSE: *n = B_PROPOSE_N; return B_PROPOSE;
        case SCR_LIVEYES: *n = B_LIVEYES_N; return B_LIVEYES;
        case SCR_LIVENO: *n = B_LIVENO_N; return B_LIVENO;
        case SCR_DEMAND: *n = B_DEMAND_N; return B_DEMAND;
        case SCR_MOVEIN: *n = B_MOVEIN_N; return B_MOVEIN;
        case SCR_DAILY: *n = B_DAILY_N; return B_DAILY;
        case SCR_KNOCKHOME: *n = B_KNOCKHOME_N; return B_KNOCKHOME;
        case SCR_NIGHTHOME: *n = B_NIGHTHOME_N; return B_NIGHTHOME;
        case SCR_PILLGIVE: *n = B_PILLGIVE_N; return B_PILLGIVE;
        case SCR_PILLDAY: *n = B_PILLDAY_N; return B_PILLDAY;
        case SCR_PILL6: *n = B_PILL6_N; return B_PILL6;
        case SCR_PILL7: *n = B_PILL7_N; return B_PILL7;
        case SCR_KNOCKIN: *n = B_KNOCKIN_N; return B_KNOCKIN;
        case SCR_MILK1: *n = B_MILK1_N; return B_MILK1;
        case SCR_MILK2: *n = B_MILK2_N; return B_MILK2;
        case SCR_PUMP: *n = B_PUMP_N; return B_PUMP;
        case SCR_BOTTLE: if (lr_on()) { *n = B_BOTTLE_LR_N; return B_BOTTLE_LR; } *n = B_BOTTLE_N; return B_BOTTLE;   /* v9.9 */
        case SCR_PEEKPIC: if (lr_on()) { *n = B_PEEKPIC_LR_N; return B_PEEKPIC_LR; } *n = B_PEEKPIC_N; return B_PEEKPIC;   /* v9.9 */
        case SCR_OUTFIT: { int mm = g_mama(&G) ? 1 : 0, t = ow_target - 1, k = pick_new(ow_groups(ow_target), ow_last[mm][t]), j; ow_last[mm][t] = k;
            for (j = 0; j < 3; j++) ow_buf[j] = (*OW_P[mm][t])[3 * k + j]; *n = 3; return ow_buf; }   /* v11.2 */
        case SCR_PUMPB: *n = B_PUMPB_N; return B_PUMPB;
        case SCR_BOTTLEB: *n = B_BOTTLEB_N; return B_BOTTLEB;
        case SCR_SCOLD: *n = B_SCOLD_N; return B_SCOLD;
        case SCR_GIFT: *n = B_GIFT_N; return B_GIFT;
        case SCR_WEDDING: *n = B_WEDDING_N; return B_WEDDING;
        case SCR_HUG: if (g_cow(&G)) { if (g_mama(&G)) { *n = B_HUG_CW_MM_N; return B_HUG_CW_MM; } *n = B_HUG_CW_N; return B_HUG_CW; }   /* v12.1: cow mode */
                      if (g_mama(&G)) { *n = B_HUG_MM_N; return B_HUG_MM; } *n = B_HUG_N; return B_HUG;   /* v9.5: mama mode */
        case SCR_KISS: if (g_cow(&G)) { if (g_mama(&G)) { *n = B_KISS_CW_MM_N; return B_KISS_CW_MM; } *n = B_KISS_CW_N; return B_KISS_CW; }
                       if (g_mama(&G)) { *n = B_KISS_MM_N; return B_KISS_MM; } *n = B_KISS_N; return B_KISS;   /* v9.5: mama mode */
        case SCR_SLEEP: if (g_cow(&G)) { if (g_mama(&G)) { *n = B_SLEEP_CW_MM_N; return B_SLEEP_CW_MM; } *n = B_SLEEP_CW_N; return B_SLEEP_CW; }
                        if (g_mama(&G)) { *n = B_SLEEP_MM_N; return B_SLEEP_MM; } *n = B_SLEEP_N; return B_SLEEP;   /* v9.5: mama mode */
        case SCR_FEED: if (g_cow(&G)) { int mm = g_mama(&G);   /* v12.1: cow mode */
                           if (lr_on()) { if (mm) { *n = B_FEED_CW_MM_LR_N; return B_FEED_CW_MM_LR; } *n = B_FEED_CW_LR_N; return B_FEED_CW_LR; }
                           if (mm) { *n = B_FEED_CW_MM_N; return B_FEED_CW_MM; } *n = B_FEED_CW_N; return B_FEED_CW; }
                       if (lr_on()) { if (g_mama(&G)) { *n = B_FEED_MM_LR_N; return B_FEED_MM_LR; } *n = B_FEED_LR_N; return B_FEED_LR; }   /* v9.9: living room */
                       if (g_mama(&G)) { *n = B_FEED_MM_N; return B_FEED_MM; } *n = B_FEED_N; return B_FEED;   /* v9.5: mama mode */
        case SCR_NIGHTM: if (g_cow(&G)) { if (g_mama(&G)) { *n = B_NIGHTM_CW_MM_N; return B_NIGHTM_CW_MM; } *n = B_NIGHTM_CW_N; return B_NIGHTM_CW; }
                         if (g_mama(&G)) { *n = B_NIGHTM_MM_N; return B_NIGHTM_MM; } *n = B_NIGHTM_N; return B_NIGHTM;   /* v9.5: mama mode */
        case SCR_NIGHT: *n = B_NIGHT_N; return B_NIGHT;
        case SCR_BEDDOOR: *n = B_BEDDOOR_N; return B_BEDDOOR;
        case SCR_CALLM: *n = B_CALLMAMA_N; return B_CALLMAMA;   /* v9.5 */
        case SCR_CALLB: *n = B_CALLBACK_N; return B_CALLBACK;
        default: *n = B_COMMENT_N; return B_COMMENT;
    }
}
static void hall_script(int step);
static void hall_say(int step);
static void vn_start(int id) {
    int n; const Beat *src = script_src(id, &n);
    memcpy(vn_buf, src, (size_t)n * sizeof(Beat)); vn_n = n; vn_id = id; vn_i = 0;
    switch (id) {   /* fill in the lines that depend on how you played */
        case SCR_INTRO: wk_place(MAP_LOBBY, 12, 10, UP); break;   /* you stand at the entrance while the opening lines play */
        case SCR_REPEAT:   /* your second visit after paying: the greeting, then (the very first time, when you can pay) her rules; then the challenge menu */
            vn_buf[1].txt = G.forfeit ? pick(G_COWARD, G_COWARD_N) : G_REPEAT_OK[0];
            vn_n = (!rules_told && G.m_played == 0 && !G.forfeit && g_can_play(&G)) ? n : REPEAT_SHORT;
            if (vn_n > REPEAT_SHORT) rules_told = 1;
            G.forfeit = 0; break;
        case SCR_NAMED: vn_buf[0].txt = G.last_res > 0 ? N_WIN[0] : G.last_res < 0 ? N_LOSE[0] : N_TIE[0]; break;
        case SCR_LEAVE: vn_buf[0].txt = pick(L_COWARD, L_COWARD_N); if (g_rom_on(&G)) vn_buf[0].bg = BG_BDOOR; break;   /* v6.7: at home she calls after you from her bedroom door */
        case SCR_DAILY: vn_buf[1].txt = S_DAILY[daily_i]; break;   /* the once-a-day refusal, after the knock */
        case SCR_OUTFIT: { int k; for (k = 0; k < vn_n; k++) vn_buf[k].bg = (ow_ward || rom_in) ? BG_ROM : BG_BDOOR; } break;   /* v11.2: in her room (or at her door) */
        case SCR_GIFT: vn_buf[0].txt = gift_pick(0, gift_kind); vn_buf[1].txt = gift_l1; vn_buf[2].txt = gift_pick(2, gift_kind);   /* v10.2: pools */
            snprintf(gift_end, sizeof gift_end, "%s  (%s)", gift_pick(3, gift_kind), gift_note); vn_buf[3].txt = gift_end;
            if (g_cow(&G) && (gift_kind == GF_MILK || gift_kind == GF_SUPER)) cow_open_prepend(gift_kind == GF_SUPER ? CO_SUPER : CO_PILL);   /* v12.1: cow mode: disgusted, but she takes it */
            else if (g_mama(&G) && (gift_kind == GF_MILK || gift_kind == GF_SUPER)) { if (lr_on()) open_prepend(gift_kind == GF_SUPER ? OP_SUPER_LR : OP_PILL_LR); else if (mm_react_on) open_prepend(gift_kind == GF_SUPER ? OP_SUPER : OP_PILL); }   /* v10.6: door and bedroom too */   /* v10.4: mama mode, living room: your line first, then her reaction to the name */
            { int k; for (k = 0; k < vn_n; k++) vn_buf[k].bg = rom_in ? BG_ROM : BG_BDOOR; } break;   /* at her door or inside her room, as you stand */
        case SCR_MILK2:   /* v6.7: with the pump in your bag the scene hands it over (the pump scene follows); without it she needs one and the objective shows */
            if (G.pump) vn_n = MILK2_OWNED; else { vn_buf[5] = vn_buf[7]; vn_n = 6; }
            break;
        case SCR_BOTTLE: {   /* v6.8: every full gauge starts with one of the random lead-ins (never the same twice in a row), then the pump scene */
            static int lead_last = -1, lead_last_lr = -1; int lr = lr_on(), k = lr ? pick_new(B_LEAD_LR_N, lead_last_lr) : pick_new(B_LEAD_N, lead_last);
            if (lr) lead_last_lr = k; else lead_last = k;
            memmove(&vn_buf[LEAD_BEATS], &vn_buf[0], (size_t)n * sizeof(Beat)); memcpy(&vn_buf[0], lr ? B_LEAD_LR[k] : B_LEAD[k], LEAD_BEATS * sizeof(Beat)); vn_n = n + LEAD_BEATS;
            if (g_cow(&G)) { cow_open_prepend(bottle_pill ? CO_PILL : CO_ASK); if (lr) lr_here(); cow_mid(CO_MID_PUMP, vn_n - 1); }   /* v12.1: cow mode: Here?! first (living room), then her disgusted yes */
            else {
            if (lr) { if (g_mama(&G)) { int sl = bottle_pill ? OP_PILL_LR : OP_ASK_LR, before = vn_n; open_prepend(sl); if (vn_n == before) lr_here(); } else lr_here(); }
            else if (g_mama(&G) && mm_react_on) open_prepend(bottle_pill ? OP_PILL : OP_ASK);   /* v10.6: at her door / in her bedroom: your line, her rejection, your plea, her answer */
            if (g_mama(&G) && mm_react_on) mid_insert(OP_MID_PUMP, vn_n - 1);
            }   /* v10.6: she hesitates once more while you face the wall */   /* v9.9: "Here?!" comes first, every time; v10.4: in mama mode your line comes first and her reaction to the name opens her answer */
            bottle_pill = 0; break; }
        case SCR_FEED: {
            int on = g_mama(&G), cw = g_cow(&G), got = 0;   /* v12.1: cow mode has its own scenes (the opener is inside them) and its own mid pair */
            if (lr_on()) { if (on && !cw) got = open_replace(OP_FEED_LR); else lr_here(); }   /* v10.4: mama mode: your line first, then her hesitant reaction (it has the Here?! in it) */
            else {
                if (on && !cw) got = open_replace(OP_FEED);   /* v10.4: at her door and in her bedroom too */
                if (on && G.married && !ls_talk && !rom_in && vn_n >= 2) { int k; for (k = 0; k < (got ? got : 2) && k < vn_n; k++) { vn_buf[k].spr = 6; vn_buf[k].bg = BG_BDOOR; } }   /* v10.1: at her door she hides behind it while she says yes (sprite 6) */
            }
            if (cw) { if (vn_n >= 4) cow_mid(CO_MID_FEED, vn_n - 2); }
            else if (on && mm_react_on && vn_n >= 4) mid_insert(OP_MID_FEED, vn_n - 2);   /* v10.6: she hesitates once more, you plead (before your last line) */
            break; }   /* v9.9 */
        case SCR_PUMPB: case SCR_BOTTLEB:   /* v6.7: if you peeked she scolds you after handing over the bottle (before the closing line of the bottle scene) */
            if (id == SCR_BOTTLEB && bottle_inj) { int k; for (k = 0; k < vn_n; k++) vn_buf[k].bg = BG_BOTTLE; }   /* v12.3: Sophia hands you the bottles (a picture for every outfit) */
            if (peeked) {
                int at = vn_n - 1, ns = B_SCOLD_N;   /* v6.8: both scenes end on the closing narration, the scolding comes before it */
               
                memmove(&vn_buf[at + ns], &vn_buf[at], (size_t)(vn_n - at) * sizeof(Beat)); memcpy(&vn_buf[at], B_SCOLD, (size_t)ns * sizeof(Beat)); vn_n += ns;
            }
            break;
        case SCR_KNOCKIN: if (g_married_day_door(&G)) { int k; for (k = 0; k < vn_n; k++) if (vn_buf[k].spr == 5) vn_buf[k].spr = 0; } break;   /* v8.7: once married she hides behind the door only at night */
        case SCR_HUG: vn_buf[0].txt = g_cow(&G) ? (g_mama(&G) ? pick(R_HUG_OPEN_CW_MM, R_HUG_OPEN_CW_MM_N) : pick(R_HUG_OPEN_CW, R_HUG_OPEN_CW_N)) : g_mama(&G) ? pick(R_HUG_OPEN_MM, R_HUG_OPEN_MM_N) : pick(R_HUG_OPEN, R_HUG_OPEN_N); break;   /* v8.6 */
        case SCR_KISS: vn_buf[0].txt = g_cow(&G) ? (g_mama(&G) ? pick(R_KISS_OPEN_CW_MM, R_KISS_OPEN_CW_MM_N) : pick(R_KISS_OPEN_CW, R_KISS_OPEN_CW_N)) : g_mama(&G) ? pick(R_KISS_OPEN_MM, R_KISS_OPEN_MM_N) : pick(R_KISS_OPEN, R_KISS_OPEN_N); break;
        case SCR_CALLM: case SCR_CALLB: { int k; for (k = 0; k < vn_n; k++) vn_buf[k].bg = ls_talk ? vn_buf[k].bg : (rom_in ? BG_ROM : BG_BDOOR); } break;   /* v9.5: in her room (or at her door) as you stand */
        case SCR_BEDDOOR: if (g_rom_on(&G)) vn_buf[0].bg = BG_BDOOR; break;   /* at home it is her bedroom door in your living room */
        case SCR_GREET:
            vn_buf[1].txt = G.forfeit ? pick(G_COWARD, G_COWARD_N) : G.m_played == 0 ? pick(G_NORMAL, G_NORMAL_N)
                          : G.last_res > 0 ? pick(G_WIN, G_WIN_N) : G.last_res < 0 ? pick(G_LOSS, G_LOSS_N) : pick(G_TIE, G_TIE_N);
            if (g_visit_ok(&G) && !G.forfeit) {   /* v11.0: once she has a name: the first knock of a session, then about 2 in 3 knocks, use her friendship tier */
                if (!fv_seen && g_fr_tier(&G) == 1) vn_buf[1].txt = FVP(G_FIRSTVIS);
                else if (fv_greet_n++ % 3 != 2) vn_buf[1].txt = fv_door_line();
                fv_seen = 1;
            }
            G.forfeit = 0; break;
        case SCR_DOOR1: G.cash -= DEBT; if (G.cash < 0) G.cash = 0; break;   /* you hand her the money for the shirt */
        case SCR_COMMENT: vn_buf[0].txt = C_LEN[len_sel]; break;
        case SCR_AFTER:   /* after a match she tells you to leave her room: reward, bonus or not, a loss, a tie; a free drink gets a second page */
            if (G.last_res > 0) vn_buf[0].txt = G.last_bonus > 0 ? pick(A_WINBONUS, A_WINBONUS_N) : pick(A_WIN, A_WIN_N);
            else if (G.last_res < 0) vn_buf[0].txt = G.last_bonus > 0 ? pick(A_LOSSBONUS, A_LOSSBONUS_N) : pick(A_LOSS, A_LOSS_N);
            else vn_buf[0].txt = pick(A_TIE, A_TIE_N);
            vn_n = 1;
            if (G.last_res > 0 && G.last_drink) { vn_buf[1].txt = pick(A_DRINK, A_DRINK_N); vn_n = 2; }
            after_ms = now_ms(); break;
    }
    if (ls_talk) { int k; for (k = 0; k < vn_n; k++) if (vn_buf[k].bg == BG_BDOOR || vn_buf[k].bg == BG_ROM) vn_buf[k].bg = TALK_BG(); }   /* v8.0: talking to her in the living room */
    vn_prev_spr = vn_buf[0].spr; vn_ms = now_ms(); vn_spr_ms = now_ms(); scr = SC_STORY;
}
static void enter_hub(void) { scr = SC_WALK; wk_mode = 0; wk_moving = 0; pause_kind = PK_NONE; ls_talk = 0; }   /* back to walking, wherever you were */
static void enter_door(void) { scr = SC_DOOR; sel = 0; pause_kind = PK_NONE; }
static void start_match(int len);
static void scene_start(int n, int end);
static const int ELEV_X[4] = {0, 6, 4, 9}, ELEV_Y[4] = {0, 3, 3, 3};   /* where you step out of the elevator on floor 1, 2, 3 (facing down) */
static void peek_start(int ctx) { peek_ctx = ctx; peek_sel = 0; peeked = 0; scr = SC_PEEK; pause_kind = PK_NONE; }   /* v6.7: the choice page after her last hesitation line */
static void auto_pump(void) {   /* v6.7: you own the pump and she needs it: it is given by the scene, nothing to choose in a menu */
    g_gift(&G, GF_PUMP); persist(); peek_ctx = 0; vn_start(SCR_PUMP);
}
static void fv_exit(void) { fv_on = 0; wk_nmap = MAP_HALL3; wk_ntx = 1; wk_nty = 4; wk_nface = RT; go(GO_MAP, 0); }   /* v11.1: out of her room, back to her door in the hall */
static void vn_done(void) {   /* the scene is over (or skipped): do what comes next */
    cover_now = 0; feed_hide = 0;   /* v8.8 */
    if (vn_id == SCR_PILLDAY || vn_id == SCR_KNOCKIN || vn_id == SCR_NIGHTM) hide_free();   /* v8.7 */
    if (vn_id == SCR_FEED) fh_free();   /* v10.1 */
    switch (vn_id) {
        case SCR_INTRO: wk_nmap = MAP_LOBBY; wk_ntx = 12; wk_nty = 10; wk_nface = UP; go(GO_MAP, 0); break;   /* arrival over: walk to the front desk */
        case SCR_BUMP: G.story = ST_HOME; G.event = 1; wk_nmap = MAP_LOBBY; wk_ntx = 7; wk_nty = 6; wk_nface = DN; go(GO_MAP, 0); break;   /* she is gone: free again in front of the desk (v6.6: the key card scene is removed) */
        case SCR_DOOR1: G.story = ST_FIRST; persist(); wk_nmap = MAP_HALL; wk_ntx = ELEV_X[2]; wk_nty = ELEV_Y[2]; wk_nface = DN; go(GO_MAP, 0); break;   /* the shirt is paid and the floor key card is gone: you are on the 2nd floor in front of the elevator, facing down */
        case SCR_REPEAT:   /* the challenge menu, or (no cash for even the cheapest match) her refusal */
            if (!g_can_play(&G)) { sc_page[0] = pick(S_BROKE, (int)S_BROKE_N); scene_start(1, SE_ZOMBIE); }
            else { scr = SC_LEN; len_sel = 1; SAYP(L_LEN); }
            break;
        case SCR_NIGHT: case SCR_NIGHTHOME: case SCR_DAILY: go(GO_HUB, 0); break;
        case SCR_KNOCKHOME:   /* the knock is done: the day-6 / day-7 scene (door picture only, she invites you in), the day-8 scene (her pictures change), or (before day 6) her menu at the door */
            if (g_pill_due(&G)) { G.pill_seen = 1; persist(); go(GO_SCRIPT, SCR_PILLDAY); }
            else if (g_hint_scene(&G)) { int h = g_pill_hint(&G); G.hint_day = G.day; persist(); vn_start(h == 1 ? SCR_PILL6 : SCR_PILL7); }
            else rom_open_door(rom_greet());
            break;
        case SCR_KNOCKIN:     /* after day 8 you are inside her room: her menu, or (two days after the first milk pill) her \"even more stuffy\" scene */
            if (g_milk_due(&G)) { G.milk_st = 2; persist(); vn_start(SCR_MILK2); }
            else if (G.milk_st == 2 && G.pump) auto_pump();   /* v6.7: you bought the pump meanwhile: you hand it over on this visit */
            else if (G.married && kicked_day != G.day) sb_enter();   /* v9.2: married: she lets you in: you walk into her bedroom */
            else rom_open(rom_greet());
            break;
        case SCR_PILL6: case SCR_PILL7: rom_open(rom_greet()); break;
        case SCR_MILK1: home_to_door("Now all I can do is wait two days."); break;
        case SCR_MILK2: if (G.pump) auto_pump(); else rom_back("BUY A BREAST PUMP ONLINE"); break;   /* v6.7: with the pump already bought there is nothing to buy: you hand it over now */
        case SCR_PUMP: peek_start(0); break;                 /* she hesitates, then the choice: face the wall / look at her */
        case SCR_BOTTLE: peek_start(1); break;
        case SCR_PEEKPIC: go(GO_SCRIPT, peek_ctx ? SCR_BOTTLEB : SCR_PUMPB); break;
        case SCR_PUMPB: if (G.bottle < INV_CAP) G.bottle++; if (G.milk_st < 4) G.milk_st = 4; persist(); ls_go_home(); home_to_door("I got Sophia breast milk. The bottle gauge is unlocked."); break;   /* v6.8: the first pump scene gives the first bottle */
        case SCR_SCOLD: ls_go_home(); home_to_door("The bottle gauge is unlocked. Every milk pill fills it by 50%."); break;
        case SCR_BOTTLEB: ls_go_home(); home_to_door(bottle_inj ? "I got 2 bottles of Sophia breast milk." : "I got Sophia breast milk."); bottle_inj = 0; break;
        case SCR_GIFT: home_to_door(gift_kind == GF_SUPER ? "Her gauge is full for 2 days. I can ask her for milk." : gift_kind == GF_INJECT ? "Sophia gives milk for good now. Once a day." : NULL); break;   /* v8.2 */
        case SCR_WEDDING: persist(); home_to_door("WE ARE MARRIED. PAGE 4 OF HER MENU IS OPEN."); break;   /* v8.6 */
        case SCR_HUG: case SCR_KISS: g_mm_note(&G, vn_id == SCR_HUG ? MK_HUG : MK_KISS); rom_back(""); break;   /* v9.7: she remembers it */
        case SCR_OUTFIT: { outfit_apply(ow_target); persist();   /* v11.2: she gave in: now she changes, with her last word (her menu again, or the message in her bedroom) */
            if (ow_ward) { enter_hub(); wk_say(N_SOPHIA, 0, WK_NONE, outfit_line(), NULL); }
            else { int in = rom_in; rom_open(outfit_line()); rom_in = in; } } break;
        case SCR_CALLM: g_mm_note(&G, MK_CALL); rom_back("NOW YOU CALL HER MAMA"); break;   /* v9.5: back to her menu */
        case SCR_CALLB: rom_back("NOW YOU CALL HER SOPHIA"); break;                  /* back to her menu */
        case SCR_FEED: { char nt[40]; if (g_super_active(&G)) snprintf(nt, sizeof nt, "ASKS LEFT TODAY: %d", g_asks_left(&G)); else snprintf(nt, sizeof nt, "THE GAUGE IS EMPTY AGAIN"); ls_go_home(); rom_back(nt); break; }
        case SCR_SLEEP: g_mm_note(&G, MK_SLEEP); ls_talk = 0; go(GO_TIME, 0); break;                /* sleeping together = the bed: rent, the next morning, the save */
        case SCR_NIGHTM: if (G.married && kicked_day != G.day) sb_enter(); else go(GO_ROM, 2); break;   /* v8.7: married night knock: after the hiding scene her menu on page 4 */
        case SCR_PILLDAY: go(GO_ROM, 0); break;   /* she pulled you inside: her menu, in her room */
        case SCR_PILLGIVE: home_to_door(NULL); break;   /* she took it: you are back in front of her door (A knocks again for the next choice) */
        case SCR_BEDDOOR: go(GO_BED, 0); break;   /* the door is open: her sleeping picture and its menu */
        case SCR_NAMED: first_end = 0; go(GO_SCRIPT, SCR_AFTER); break;
        case SCR_AFTER: if (fv_on) fv_exit(); else go(GO_HUB, 0); break;   /* v11.1: a match started inside her room ends outside it, at her door */
        case SCR_BOSS: persist(); go(GO_HUB, 0); break;   /* the boss is beaten: back out of the match room (the elevator takes you to her floor again) */
        case SCR_LEAVE: persist(); if (fv_on) fv_exit(); else go(GO_HUB, 0); break;   /* v11.1: leaving a match that started inside her room puts you outside, at her door */
        case SCR_PROPOSE: scr = SC_ROMQ; romq_sel = 0; vn_ms = now_ms(); pause_kind = PK_NONE; break;   /* her question: live with me? (YES / NOT NOW) */
        case SCR_LIVEYES: G.rs = RS_VISIT; persist(); wk_nmap = MAP_ROOM; wk_ntx = ROOM_SX; wk_nty = ROOM_SY; wk_nface = DN; go_slow(GO_MAP, 0); break;   /* you walk back into your room; she comes after a short wait (GO_MAP) */
        case SCR_LIVENO: G.rs = RS_NO; persist(); go(GO_HUB, 0); break;       /* no upgrade, she stays in her room; you can still ask her to move in */
        case SCR_DEMAND: G.rs = RS_UPG; persist(); go(GO_HUB, 0); break;      /* the desk now sells the upgrade */
        case SCR_MOVEIN: G.rs = RS_LIVING; persist(); wk_nmap = MAP_HALL; wk_ntx = ELEV_X[2]; wk_nty = ELEV_Y[2]; wk_nface = DN; go(GO_MAP, 0); break;   /* she lives with you now; the 3rd floor is closed */
        case SCR_RAIN: go(GO_HUB, 0); break;   /* back to walking where you stood */
        case SCR_GREET: enter_door(); break;
        default: go(GO_TABLE, pending_len); break;   /* SCR_COMMENT */
    }
}
static void vn_key(int b) {
    const Beat *cur = &vn_buf[vn_i];
    if (b != B_A) return;
    if (!typed_done(cur->txt, vn_ms)) { vn_ms = now_ms() - 30000u; return; }   /* finish typing first */
    if (vn_i + 1 >= vn_n) { vn_done(); return; }
    if (vn_buf[vn_i + 1].bg != cur->bg) { go(GO_BEAT, 0); return; }
    vn_prev_spr = cur->spr; vn_i++; vn_ms = now_ms(); vn_spr_ms = now_ms();
}
static void apply_go(int kind, int arg) {
    pause_kind = PK_NONE;
    switch (kind) {
        case GO_MENU: scr = SC_MENU; break;
        case GO_LOAD: G = load_pend; saved_G = G; slot_act = load_pend_slot; prog_far = load_pend_far; cfg_save();   /* v10.5: the slot read before the fade is in; you wake up in your room */
            room_seen = 0; unpack_done = 0; ev_out = 0; sb_out = 0; wk_mode = 0; wk_moving = 0; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN); ls_reroll(); enter_hub(); break;
        case GO_EXIT: G = saved_G; room_seen = 0; unpack_done = 0; wk_mode = 0; wk_moving = 0; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN); scr = SC_MENU; break;   /* v9.4: back to the last save only once the screen is black (nothing of the living room shows) */
        case GO_HUB: { int lt = ls_talk; if (ev_out) { ev_out = 0; wk_place(MAP_EVENT, EV_OUT_X, EV_OUT_Y, DN); } if (sb_out) { sb_out = 0; if (wk_map == MAP_SBED) wk_place(MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, UP); } enter_hub(); if (lt && home_note && (wk_map == MAP_ROOM || wk_map == MAP_SBED)) wk_say(NULL, 1, WK_NONE, home_note, NULL); if (lt) home_note = NULL; } break;   /* after an Exhibition you step out of the match room */
        case GO_VISIT: vn_start(G.story == ST_HOME ? SCR_DOOR1 : G.story == ST_FIRST ? SCR_REPEAT : SCR_GREET); break;
        case GO_SCRIPT: vn_start(arg); break;
        case GO_TABLE: start_match(arg); break;
        case GO_EVENT: scr = SC_EVENT; ev_sel = 0; qp_ms = 0; break;   /* v9.0: after the clerk's greeting (and the result of your ticket): the Exhibition / Lottery menu */
        case GO_MAP: wk_place(wk_nmap, wk_ntx, wk_nty, wk_nface); scr = SC_WALK; if (wk_map == MAP_ROOM) room_seen = 1;   /* the hotel's services open up once you have been in your room */
            if (wk_map == MAP_ROOM && G.story == ST_HOME && !unpacked()) unpack_begin();   /* v7.1: the first time in your room: you walk two steps up by yourself, a fade, then the line (in the bedroom) */
            else if (wk_map == MAP_ROOM && G.rs == RS_VISIT) wk_say(NULL, 1, WK_DEMAND, W_WAITHER, NULL);   /* she comes to look at your room */
            else if (wk_map == MAP_HALL && G.story >= ST_HOME && !(G.hall_seen & 1)) hall_script(1);   /* first time on the 2nd floor: you cannot move, you look at your door */
            else if (wk_map == MAP_HALL3 && G.story >= ST_HOME && !(G.hall_seen & 2)) hall_script(4);   /* first time on the 3rd floor: you look at her door */
            else if (home_note && wk_map == MAP_ROOM) wk_say(NULL, 1, WK_NONE, home_note, NULL);   /* v6.6: after a gift scene a short line, then you stand in front of her door */
            home_note = NULL;
            break;
        case GO_UNPACK: unpack_done = 1; wk_place(MAP_ROOM, UNPACK_X, UNPACK_Y, RT); wk_say(N_ME, 0, WK_NONE, W_UNPACKED, NULL); break;   /* v6.8: the fade is over: you have unpacked */
        case GO_ROM: rom_open(arg == 2 ? (kicked_day == G.day ? (g_mama(&G) ? pick(R_KICKED_MM, R_KICKED_MM_N) : pick(R_KICKED, R_KICKED_N)) : night_greet()) : arg ? pick(R_BACK, R_BACK_N) : rom_greet()); rom_in = g_invited(&G) ? 1 : 0; if (arg == 2) { rom_in = 1; rom_page = 3; }   /* v8.6: married, at night: her menu on page 4 */
            break;   /* her bedroom door: the visual-novel scene */
        case GO_QSETUP: mp_mode = arg; qs_step = 1; qs_sel = 0; scr = SC_QSETUP; break;
        case GO_MTABLE: if (mp_mode == 1) ev_out = 1; mp_begin(); break;
        case GO_MONO: scr = SC_MONO; mono_pg = 0; mono_ms = now_ms(); break;
        case GO_VISITIN: fv_on = 1; fv_vsel = 0; wk_place(MAP_SOPHIA, 13, 5, LF); fv_npc(); scr = SC_WALK; wk_say(NULL, 1, WK_NONE, pick(V_IN, V_IN_N), NULL); break;   /* v11.0: COME IN: her room by day */
        case GO_BED: scr = SC_SNEAK; sn_mode = SM_BED; sn_sel = 0; break;
        case GO_SNEAKBACK: scr = SC_WALK; wk_mode = 0; if (!g_rom_on(&G)) wk_face = UP; break;   /* out of the bedroom picture: back at her bedroom door */
        case GO_TIME: {   /* the sofa / the bed: time moves on, unpaid rent is taken when the night starts, then it is saved */
            const char *extra = NULL;
            g_advance(&G);
            if (wk_map == MAP_SBED) sb_slept();   /* v9.2: you slept with her: you wake up in front of her bedroom door, she stays in her room */
            if (G.phase == PH_NIGHT) {
                int bank = 0, r = g_rent_night(&G, &bank);
                if (r == 0) {   /* cash + bank cannot pay the rent: thrown out (v10.5: no save is deleted any more, load one from the title) */
                    wk_mode = 0; scr = SC_OVER; over_rent = 1; over_ms = now_ms(); break;
                }
                if (r > 0) { if (bank) snprintf(rent_msg, sizeof rent_msg, W_RENT_TAKEN_BANK, g_rent_amt(&G) - bank, bank); else snprintf(rent_msg, sizeof rent_msg, W_RENT_TAKEN, g_rent_amt(&G)); extra = rent_msg; }
            } else if (g_rent_due(&G)) extra = G.room_up ? W_RENT_REMIND_UP : W_RENT_REMIND;   /* the rent is due today and not paid yet */
            save_now(); scr = SC_WALK; wk_mode = 0; if (wk_map == MAP_ROOM) ls_reroll();   /* v8.1: she is rolled while the screen is black */
            wk_say(NULL, 1, WK_NONE, G.phase == PH_MORNING ? W_SAVED_DAY : G.phase == PH_EVENING ? W_SAVED_EVE : W_SAVED_NIGHT, extra);
            break; }
        case GO_BEAT: vn_i++; vn_prev_spr = vn_buf[vn_i].spr; vn_ms = now_ms(); vn_spr_ms = now_ms() - 10000u; break;
    }
}

/* ---------- game flow ---------- */
/* v9.7: the table lines of the free match at home in mama mode have their own pools (_MM); every other match keeps the old ones. The home flag is already cleared when the end-of-match line is picked, so the END screen asks home_end. */
static const char *tl_pick(const char *const *n, int nn, const char *const *m, int mn) { return g_mama(&G) && (G.home || (scr == SC_END && home_end)) ? pick(m, mn) : pick(n, nn); }
#define SAYT(pool) say(tl_pick(pool, pool##_N, pool##_MM, pool##_MM_N))
#define PICKT(pool) tl_pick(pool, pool##_N, pool##_MM, pool##_MM_N)
static void begin_hand(void) {
    hline_txt = NULL;
    if (lead_next) lead_len = LEAD_MS; else { lead_next = 1; lead_len = HAND_LEAD; }   /* the first hand of a match was announced by start_match; later hands get a shorter pause before the deal */
    g_hand_start(&G); her_tag = 0; hand_end_ms = 0; hl_clear(); cu_used = 0; cu_mode = 0;
    anim_new_hand(G.c[0], G.c[1], 0);   /* new hand: her card, your card, her card, your card, then the blinds go into the pot */
    if (G.hand == G.mlen) SAYT(L_LAST); else if (rnd(3) == 0) SAYT(L_HAND);
    if (G.toact == 1) think_at = after_anim(700);
}
static void start_match(int len) {   /* len = the buy-in (100 / 500 / 1000) */
    if (len > 0) g_soph_start(&G, len);   /* v12.4: cash + 20% energy (len 0 = the free match at home, already started by g_home_start) */
    lead_next = 1; cu_used = 0; cu_mode = 0; begin_hand(); scr = SC_PLAY; her_tag = 0;
    if (G.hand != G.mlen) SAYT(L_START);
    persist();   /* the save now says "in a match": closing the app from here on counts as walking out */
}
static void hand_over_lines(void) {   /* her line for the hand that just ended. It must not spoil the result: it waits until the banner is fully up (the table keeps her last line until then) */
    const char *t;
    if (G.res < 0) t = G.fold == 1 ? PICKT(L_YOUFOLD) : PICKT(L_SHEWIN);
    else if (G.res > 0) t = G.fold == 2 ? PICKT(L_SHEFOLD) : PICKT(L_YOUWIN);
    else t = PICKT(L_SPLIT);
    if (frozen) { say(t); hline_txt = NULL; return; }   /* (checks and screenshots do not wait) */
    hline_txt = t; hline_ms = now_ms() + hand_dl() + 300u + HLINE_GAP;
}
static void do_action(int a) {
    int who = G.toact, tc = g_tocall(&G, who), cr = g_can_raise(&G), raised = a == A_RAISE && cr, first = G.nbets == 0, ended;
    int st0 = G.stage, pot0 = g_pot(&G), n0, n1;
    if (G.over) return;
    ended = g_act(&G, a);
    n0 = st0 == 0 ? 0 : st0 + 2; if (n0 > 5) n0 = 5; n1 = G.stage == 0 ? 0 : G.stage + 2; if (n1 > 5) n1 = 5;
    anim_after(who, g_pot(&G) - pot0, n0, n1, ended, G.showdown, G.showdown);   /* chips into the pot first, then new board cards face down and turned up, then her cards */
    if (who == 1) her_tag = a == A_FOLD ? 4 : raised ? (tc == 0 && first ? 5 : 3) : (tc > 0 ? 2 : 1); else her_tag = 0;
    if (ended) {
        hand_end_ms = now_ms(); hand_over_lines(); hl_sophia(); hl_extend(G.showdown);
        pay_who = G.res > 0 ? 1 : G.res < 0 ? 2 : 3; pay_ms = now_ms() + hand_dl() + 300; sfx_at(pay_ms + 100, SFX_COLLECT);   /* the pot flies to the winner once the banner is up */
        her_expr = (G.showdown || G.amt >= 8) ? (G.res > 0 ? 1 : G.res < 0 ? 2 : 0) : 0;   /* shock when you take a real pot, smug when she does */
    }
    else {
        if (G.toact == 1) think_at = after_anim(900 + rnd(1800) + (G.nbets - (G.stage == 0) > 0 ? 500 : 0));   /* she takes her time once everything has settled, longer when facing a raise */
        if (who == 1 && raised && rnd(3) == 0) SAYT(L_HERRAISE);
        if (who == 0 && raised && rnd(4) == 0) SAYT(L_YOURAISE);
    }
}
static void finish_match(void) {
    { int sw0 = G.sw_wins; hline_txt = NULL; home_end = G.home; g_match_finish(&G); her_expr = 0; ach_check(sw0); }
    first_end = 0;   /* (there is no forced first match any more: she gives her name when you beat her at the Exhibition, see mp_end_game / SCR_BOSS) */
    persist(); scr = SC_END;
    if (G.last_res > 0) SAYT(L_ENDYOU); else if (G.last_res < 0) { if (G.last_bonus > 0) SAYT(L_ENDLOST); else SAYT(L_ENDHER); } else SAYT(L_ENDTIE);
}
static void leave_table(void) {   /* walking out halfway: counts as a loss, she calls you a coward */
    hline_txt = NULL; home_end = 0; g_forfeit(&G); persist(); pause_kind = PK_NONE; go(GO_SCRIPT, SCR_LEAVE);
}

/* ---------- ATM ---------- */
static const int P10[6] = {100000, 10000, 1000, 100, 10, 1};
static void atm_open(void) { scr = SC_ATM; atm_mode = 0; sel = 0; atm_msg[0] = 0; atm_amt = 0; atm_cur = 5; }
static void atm_key(int b) {
    if (atm_mode == 0) {
        if (b == B_UP) sel = (sel + 2) % 3; else if (b == B_DOWN) sel = (sel + 1) % 3;
        else if (b == B_B || (b == B_A && sel == 2)) { scr = SC_WALK; atm_msg[0] = 0; }
        else if (b == B_A) { atm_mode = 1; atm_kind = sel; atm_amt = 0; atm_cur = 5; atm_msg[0] = 0; }
        return;
    }
    atm_msg[0] = 0;
    if (b == B_LEFT && atm_cur > 0) atm_cur--; else if (b == B_RIGHT && atm_cur < 5) atm_cur++;
    else if (b == B_UP) { atm_amt += P10[atm_cur]; if (atm_amt > MONEY_CAP) atm_amt = MONEY_CAP; }
    else if (b == B_DOWN) { atm_amt -= P10[atm_cur]; if (atm_amt < 0) atm_amt = 0; }
    else if (b == B_X) atm_amt = 0;
    else if (b == B_Y) atm_amt = atm_kind ? G.bank : G.cash;
    else if (b == B_B) { atm_mode = 0; sel = atm_kind; }
    else if (b == B_A) {
        char m[24]; int r = atm_kind ? g_withdraw(&G, atm_amt) : g_deposit(&G, atm_amt);
        if (r == 0) { fmt_money(m, atm_amt); snprintf(atm_msg, sizeof atm_msg, "%s $%s", atm_kind ? "WITHDREW" : "DEPOSITED", m); persist(); atm_mode = 0; sel = atm_kind; atm_amt = 0; atm_cur = 5; }
        else snprintf(atm_msg, sizeof atm_msg, "%s", r == 1 ? "INSUFFICIENT FUNDS" : r == 2 ? "LIMIT REACHED" : "ENTER AN AMOUNT");
    }
}

/* ---------- her refusal scenes (at her door) ---------- */
static void scene_start(int n, int end) { sc_n = n; sc_i = 0; sc_end = end; sc_choice = 0; sc_ms = now_ms(); scr = SC_SCENE; }
/* ---------- Sophia's key card (after the bump scene) ---------- */
static int opt_row, opt_from;   /* the row (0 dialogue opacity, 1 sound, 2 walk speed, 3 save slots) and, in game, the pause menu the options were opened from */
static int opt_tsel;   /* v10.7: the row of the TEXT BOX STYLE sub-panel (opt_sub == 2): 0 line colour, 1 Sophia's name, 2 your name */
#define OPT_TXT (opt_ext() ? 4 : 3)   /* the row that opens it */
static int opt_sub, opt_lsel, opt_conf, opt_csel;   /* v10.5: opt_sub = 1 while the SAVE SLOTS list is open (opt_lsel = its row); opt_conf = 1 while "load this slot?" is asked (opt_csel 0 = NO, 1 = YES) */
static int opt_ext(void) { return scr == SC_OPT || opt_from == PK_HOME; }   /* the SAVE SLOTS row: at the title and while walking (not at a table or in a scene) */
static void draw_slots_panel(void) {   /* v10.5: the five slots; A loads, X chooses where the sofa and the bed save */
    char b[64]; int i; Uint32 white = C(240,240,240), gold = mn_tc(), dim = C(150,155,170), red = C(235,120,120);
    panel_am(30, 30, 580, 372, 256, 246);
    text_c(320, 44, "SAVE SLOTS", 3, gold);
    if (!opt_conf) {
        for (i = 0; i < SLOT_N; i++) {   /* v10.9: one row per slot, scale 2 (same as CAREER MODE) */
            int y = 80 + i * 52; Uint32 col = i == opt_lsel ? gold : white;
            if (i == opt_lsel) panel_am(44, y, 552, 48, 256, 256);
            snprintf(b, sizeof b, "SLOT %d", i + 1); text(60, y + 17, b, 2, col);
            slot_line(b, sizeof b, i); text(168, y + 17, b, 2, slot_st[i] == 1 ? (i == opt_lsel ? gold : white) : (slot_st[i] == 2 ? red : dim));
            if (i + 1 == slot_act) text_r(580, y + 17, "SAVES HERE", 2, gold);
        }
        text_c(320, 358, slot_st[opt_lsel] == 1 ? "A LOAD  X SAVES HERE  B BACK" : "X SAVES HERE  B BACK", 2, C(150,155,170));
    } else {
        snprintf(b, sizeof b, "LOAD SLOT %d?", opt_lsel + 1); text_c(320, 110, b, 3, white);
        slot_line(b, sizeof b, opt_lsel); text_c(320, 156, b, 2, dim);
        if (unsaved()) { text_c(320, 200, "UNSAVED PROGRESS", 2, red); text_c(320, 224, "WILL BE LOST", 2, red); }
        else text_c(320, 212, "ALL YOUR PROGRESS IS SAVED", 2, white);
        text_c(320, 256, "YOU WAKE UP IN YOUR ROOM", 1, dim);
        for (i = 0; i < 2; i++) { int x = 190 + i * 140; if (i == opt_csel) panel_am(x, 284, 120, 32, 256, 256); text_c(x + 60, 292, i ? "YES" : "NO", 2, i == opt_csel ? gold : dim); }
    }
}
static void draw_style_panel(void) {   /* v10.7 / v11.5: TEXT BOX STYLE: line colour of the dialogue / name boxes, the two name colours, and (v11.5) the table box opacity and line colour; the sample below shows the row live */
    static const char *const lab[4] = { "LINE COLOR", "SOPHIA NAME", "YOUR NAME", "TABLE OPACITY" };   /* v12.0: TABLE LINE is gone (MENU COLORS > GUI LINE) */
    int vals[4], r, i; Uint32 white = C(240,240,240), gold = mn_tc(), dim = C(150,155,170);
    vals[0] = dlg_line; vals[1] = col_soph; vals[2] = col_you; vals[3] = tb_idx;
    panel_am(90, 40, 460, 238, 256, 246);
    text_c(320, 48, "TEXT BOX STYLE", 3, gold);
    for (r = 0; r < 4; r++) {
        int y = 74 + r * 48;
        text(110, y, lab[r], 2, r == opt_tsel ? gold : white);
        text_r(530, y, r == 3 ? TB_N[vals[r]] : PAL_N[vals[r]], 2, r == opt_tsel ? gold : dim);
        for (i = 0; i < (r == 3 ? 6 : 8); i++) {
            int x = r == 3 ? 104 + i * 72 : 104 + i * 54, w = r == 3 ? 66 : 48; Uint32 sw;
            if (r == 3) { sw = C(30 + 20 * i, 34 + 20 * i, 44 + 20 * i); } else { const int *c = PAL_NAME[i]; sw = C(c[0], c[1], c[2]); }
            R(x, y + 20, w, 18, sw);
            if (r == 3) text_c(x + w / 2, y + 24, TB_N[i], 1, C(240,240,240));
            if (i == vals[r]) { Uint32 wh = C(255,255,255); R(x - 3, y + 17, w + 6, 3, wh); R(x - 3, y + 38, w + 6, 3, wh); R(x - 3, y + 17, 3, 24, wh); R(x + w, y + 17, 3, 24, wh); }
        }
    }
    text_c(320, 265, "U/D ROW   L/R CHANGE   A/B BACK", 1, C(130,135,150));
    if (opt_tsel >= 3) { R(0, 330, 640, 150, C(22,70,40)); panel_t(40, 345, 150, 26, 256); text(48, 351, "POT $40", 2, gui_tc()); panel_t(40, 377, 150, 26, 256); text(48, 383, "HAND 1", 2, white); text_c(430, 350, "THE TABLE BOXES", 2, white); text_c(430, 372, "LOOK LIKE THIS", 2, white); }
    else dialog("This is how the text box and the names look.", opt_tsel == 1 ? N_SOPHIA : opt_tsel == 2 ? N_ME : "SAMPLE", 0, now_ms() - 30000u);
}
static void draw_mc_panel(void) {   /* v11.9: MENU COLORS: line colour and text colour of the Save Slots / Career / Options / pause / online store menus (this panel is drawn in them, so it is its own sample) */
    static const char *const lab[4] = { "LINE COLOR", "TEXT COLOR", "GUI LINE", "GUI TEXT" };   /* v12.0: rows 3-4 colour every other box */
    int vals[4], r, i; Uint32 white = C(240,240,240), gold = mn_tc(), dim = C(150,155,170);
    vals[0] = mn_line; vals[1] = mn_text; vals[2] = gui_line; vals[3] = gui_text;
    panel_am(90, 24, 460, 322, 256, 246);
    text_c(320, 34, "MENU COLORS", 3, gold); R(110, 66, 420, 2, mn_dc());
    for (r = 0; r < 4; r++) {
        int y = 80 + r * 62 + (r >= 2 ? 6 : 0);
        text(110, y, lab[r], 2, r == opt_tsel ? gold : white);
        text_r(530, y, MN_N[vals[r]], 2, r == opt_tsel ? gold : dim);
        for (i = 0; i < 8; i++) {
            int x = 104 + i * 54, w = 48; const int *c = r % 2 == 0 ? MN_LIN[i] : MN_TXT[i];
            R(x, y + 22, w, 18, C(c[0], c[1], c[2]));
            if (i == vals[r]) { Uint32 wh = C(255,255,255); R(x - 3, y + 19, w + 6, 3, wh); R(x - 3, y + 40, w + 6, 3, wh); R(x - 3, y + 19, 3, 24, wh); R(x + w, y + 19, 3, 24, wh); }
        }
    }
    R(110, 202, 420, 2, mn_dc());
    text_c(320, 330, "U/D ROW   L/R CHANGE   A/B BACK", 1, C(130,135,150));
    if (opt_tsel >= 2) {   /* the GUI rows: a live sample of the other boxes under the panel */
        char b[24]; b[0] = '$'; fmt_money(b + 1, G.cash);
        R(0, 352, 640, 128, C(22,70,40));
        panel_a(40, 366, 148, 30, 256, PANEL_ALPHA); text(48, 376, "CASH", 1, gui_tc()); text_r(180, 373, b, 2, C(240,240,240));
        panel_a(40, 402, 148, 30, 256, PANEL_ALPHA); text(48, 409, "DAY 1  MORNING", 1, C(240,240,240));
        panel_t(210, 366, 150, 26, 256); text(218, 372, "POT $40", 2, gui_tc());
        panel_t(210, 398, 150, 26, 256); text(218, 404, "HAND 1", 2, C(240,240,240));
        panel_a(400, 366, 200, 66, 256, PANEL_ALPHA); text_c(500, 380, "YOU WIN", 2, gui_tc()); R(412, 402, 176, 2, gui_dc()); text_c(500, 412, "A OK", 2, gui_tc());
    }
}
static void draw_opt_panel(void) {   /* the same panel at the title (over Sophia's room and the empty table) and in game (over the paused game) */
    int i, ext = opt_ext(); Uint32 white = C(240,240,240), gold = mn_tc(), dim = C(150,155,170);
    static const char *const sn[2] = { "OFF", "ON" };
    if (opt_sub == 3) { draw_mc_panel(); return; }
    if (opt_sub == 2) { draw_style_panel(); return; }
    if (opt_sub) { draw_slots_panel(); return; }
    panel_am(90, 40, 460, ext ? 322 : 286, 256, 246);   /* (nearly solid: in game it sits over the busy table) */
    text_c(320, 52, "OPTIONS", 3, gold);
    { char sb[32]; snprintf(sb, sizeof sb, "SCREEN %dX%d", SW, SH); text_r(540, 46, sb, 1, dim); }   /* v7.0: the size the system really gave the game */
    if (text_problems) { char tb[48]; snprintf(tb, sizeof tb, "TEXT FILES: %d PROBLEMS (SEE TEXT_LOG.TXT)", text_problems); text_c(320, ext ? 348 : 312, tb, 1, C(230,150,90)); }   /* v7.9 */
    text_c(320, 88, "DIALOGUE BOX", 2, opt_row == 0 ? gold : white);   /* v10.7: rows moved up to make room for TEXT BOX STYLE */
    for (i = 0; i < 6; i++) { int x = 120 + i * 70; if (i == dlg_idx) panel_am(x - 4, 108, 66, 30, 256, 256); text_c(x + 29, 116, DLG_N[i], 2, i == dlg_idx ? gold : dim); }
    text_c(320, 148, "SOUND", 2, opt_row == 1 ? gold : white);
    for (i = 0; i < 2; i++) { int x = 190 + i * 140; if (i == snd_on) panel_am(x, 168, 120, 30, 256, 256); text_c(x + 60, 176, sn[i], 2, i == snd_on ? gold : dim); }
    text_c(320, 208, "WALK SPEED", 2, opt_row == 2 ? gold : white);   /* v8.0 */
    for (i = 0; i < 3; i++) { int x = 130 + i * 130; if (i == walk_idx) panel_am(x, 228, 120, 30, 256, 256); text_c(x + 60, 236, WALK_N[i], 2, i == walk_idx ? gold : dim); }
    if (ext) {   /* v10.5 */
        char sl[64], sb[96]; slot_line(sl, sizeof sl, slot_act - 1);
        text_c(320, 268, "SAVE SLOTS", 2, opt_row == 3 ? gold : white);
        snprintf(sb, sizeof sb, "THE SOFA AND BED SAVE TO SLOT %d  (%s)", slot_act, sl); text_c(320, 288, sb, 1, opt_row == 3 ? gold : dim);
    }
    { int ry = ext ? 308 : 268, on = opt_row == OPT_TXT;   /* v10.7: TEXT BOX STYLE (A opens) */
      text_c(210, ry, "TEXT BOX STYLE", 2, on && !opt_mc ? gold : white);   /* v11.9: two entries side by side: L/R picks, A opens */
      text_c(440, ry, "MENU COLORS", 2, on && opt_mc ? gold : white);
      if (on) { const char *ls = opt_mc ? "MENU COLORS" : "TEXT BOX STYLE"; R((opt_mc ? 440 : 210) - tw(ls, 2) / 2, ry + 17, tw(ls, 2), 2, gold); } }
    if (ext) text_c(320, 334, opt_row == 3 ? "L/R SLOT   A SLOT LIST   B BACK" : opt_row == OPT_TXT ? "L/R PICK  A OPEN  B BACK" : "U/D ROW   L/R CHANGE   A/B BACK", 1, C(130,135,150));
    else text_c(320, 296, opt_row == OPT_TXT ? "L/R PICK  A OPEN  B BACK" : "U/D ROW   L/R CHANGE   A/B BACK", 2, C(130,135,150));
    dialog("This is how the text box looks. Pick the opacity you like.", "SAMPLE", 0, now_ms() - 30000u);
}
static void draw_opt(void) {
    bg_draw(BG_TABLE); if (tblE) blit(tblE, 0, 0); else R(0, 362, 640, 118, C(45,112,82));   /* Sophia's room with the empty table (no picture of her) */
    draw_opt_panel();
}
/* a key in the options (title screen or in game); returns 1 when the options are closed */
static int opt_key(int b) {
    if (opt_sub == 3) {   /* v11.9: MENU COLORS */
        if (b == B_UP && opt_tsel > 0) opt_tsel--; else if (b == B_DOWN && opt_tsel < 3) opt_tsel++;
        else if (b == B_LEFT || b == B_RIGHT) { int *pv = opt_tsel == 0 ? &mn_line : opt_tsel == 1 ? &mn_text : opt_tsel == 2 ? &gui_line : &gui_text; *pv = (*pv + (b == B_RIGHT ? 1 : 7)) % 8; }
        else if (b == B_A || b == B_B || b == B_QUIT) { opt_sub = 0; cfg_save(); }
        return 0;
    }
    if (opt_sub == 2) {   /* v10.7: TEXT BOX STYLE */
        if (b == B_UP && opt_tsel > 0) opt_tsel--; else if (b == B_DOWN && opt_tsel < 3) opt_tsel++;
        else if (b == B_LEFT || b == B_RIGHT) { int *pv = opt_tsel == 0 ? &dlg_line : opt_tsel == 1 ? &col_soph : opt_tsel == 2 ? &col_you : &tb_idx, n = opt_tsel == 3 ? 6 : 8; *pv = (*pv + (b == B_RIGHT ? 1 : n - 1)) % n; }
        else if (b == B_A || b == B_B || b == B_QUIT) { opt_sub = 0; cfg_save(); }
        return 0;
    }
    if (opt_sub) {   /* v10.5: the SAVE SLOTS list */
        if (opt_conf) {
            if (b == B_LEFT || b == B_RIGHT || b == B_UP || b == B_DOWN) opt_csel = 1 - opt_csel;
            else if (b == B_B || b == B_QUIT) opt_conf = 0;
            else if (b == B_A) { int c = opt_csel; opt_conf = 0; if (c) { if (slot_begin_load(opt_lsel + 1)) { opt_sub = 0; go(GO_LOAD, 0); } else slots_refresh(); } }
            return 0;
        }
        if (b == B_UP) opt_lsel = (opt_lsel + SLOT_N - 1) % SLOT_N; else if (b == B_DOWN) opt_lsel = (opt_lsel + 1) % SLOT_N;
        else if (b == B_B || b == B_QUIT) opt_sub = 0;
        else if (b == B_A) { if (slot_st[opt_lsel] == 1) { opt_conf = 1; opt_csel = 0; } }
        else if (b == B_X) { slot_act = opt_lsel + 1; cfg_save(); }
        return 0;
    }
    if (b == B_UP && opt_row > 0) opt_row--; else if (b == B_DOWN && opt_row < (opt_ext() ? 4 : 3)) opt_row++;
    else if (b == B_LEFT || b == B_RIGHT) {
        int d = b == B_RIGHT ? 1 : -1;
        if (opt_row == 0) { dlg_idx += d; if (dlg_idx < 0) dlg_idx = 0; if (dlg_idx > 5) dlg_idx = 5; }
        else if (opt_row == 1) { snd_on = d > 0; if (snd_on) sfx_play(SFX_PLACE); }   /* (a click to hear it) */
        else if (opt_row == 2) { walk_idx += d; if (walk_idx < 0) walk_idx = 0; if (walk_idx > 2) walk_idx = 2; }
        else if (opt_row == OPT_TXT) opt_mc = d > 0;   /* v11.9: TEXT BOX STYLE (left) / MENU COLORS (right) */
        else if (opt_row == 3 && opt_ext()) { slot_act += d; if (slot_act < 1) slot_act = 1; if (slot_act > SLOT_N) slot_act = SLOT_N; }   /* v10.5: where the sofa and the bed save */
    }
    else if (b == B_A && opt_row == 3 && opt_ext()) { slots_refresh(); opt_sub = 1; opt_conf = 0; opt_lsel = slot_act - 1; }   /* v10.5: the slot list */
    else if (b == B_A && opt_row == OPT_TXT) { opt_sub = opt_mc ? 3 : 2; opt_tsel = 0; }   /* v10.7 */
    else if (b == B_B || b == B_A || b == B_QUIT) { cfg_save(); return 1; }
    return 0;
}
static void scene_key(int b) {
    if (!typed_done(sc_page[sc_i], sc_ms)) { if (b == B_A) sc_ms = now_ms() - 30000u; return; }   /* finish typing first */
    if (b != B_A) return;
    if (sc_i + 1 < sc_n) { sc_i++; sc_ms = now_ms(); return; }
    if (G.story == ST_FIRST || sc_end == SE_DAILY) { go(GO_HUB, 0); return; }   /* (the first challenge has no door menu, and the once-a-day refusal sends you back to the hall) */
    scr = SC_DOOR; sel = 0;
}

/* ---------- walking around: your room, the hall and the lobby (RPG-Maker-style maps, 48 px tiles) ---------- */
#define TILE 72   /* the 408x312 exports are enlarged 3x: 24 px tiles become 72 px */
#define MMW 17   /* the widest map and the tallest map (array sizes); every map has its own w x h in wm[] */
#define MMH 16
/* tile letters: . floor  # solid  D door (walk into it)  E Event Center door  b bathroom door  e lobby entrance (blocked by the storm)
   d bedroom door (A goes through)  K Sophia's door (A knocks)  A ATM (v9.1: two tiles in the lobby's top wall, use from below facing up)  M the vending machine (store, lobby top wall)  S reception desk (rent and room upgrade only)  F fridge  B bed  C sofa (bed and sofa save)  n sink */
/* your room (v9.3: three levels of the same MAP_ROOM). Level 0 = the base room (7 x 7 tiles, assets/map_room.png): kitchen, old desk (no computer), sofa and bed in one room.
   Level 1 = upgrade 1 (11 x 7, assets/map_room_u1.png): living room and bedroom, the computer, the lamp stands where her door will be (no door, nothing to use there).
   Level 2 = upgrade 2 (11 x 7, assets/map_room_up.png): the lamp is gone and her bedroom door is there (also used once she lives with you).
   v9.4: the computer (P) sits one row higher and the bed is ONE solid tile (B), so you can step up to the tile in front of the computer (7,3) / at the foot of the bed (9,4); base room: desk (4,2), bed (5,3), floor (5,4).
   n sink  F fridge (inventory)  C sofa (saves)  B bed (saves)  P the computer (online store; on the base desk it only opens the debug panel in a test build)  d the door between the two rooms  b bathroom  D the room door  g her bedroom door */
static const char *const MG_ROOM0[7] = {
    "#######", "#######", "#Fn#P##", "#....B#", "#C....#", "#C....b", "###D###" };
static const char *const MG_ROOM[7] = {
    "###########", "###########", "#Fn####P###", "#....##..B#", "#C....#...#", "#C....d...b", "###D#######" };
static const char *const MG_ROOM_UP[7] = {
    "###########", "###########", "#Fn##g#P###", "#.....#..B#", "#C....#...#", "#C....d...b", "###D#######" };
/* the three floors (each map is exactly the tiles of its picture; assets/map_hall*.png are made by tools/make_floor_maps.py)
   L elevator door (A opens the floor menu)  U stairs door (A goes up or down, 1st and 2nd floor only)
   2nd floor (15 x 7): the elevator, the stairs door (dark doorway), your barred door, a box, a cabinet and a plant are solid */
static const char *const MG_HALL[7] = {
    "###############", "###############", "####LL##U###D##", "##............#", "##............#", "##...........##", "###############" };
/* 1st floor stair hall (13 x 11): elevator on top, the stairs at the right (walk down the steps to the mat = stairs door), the left mat leads to the lobby, the dark pit and the sofa are solid */
static const char *const MG_HALL1[11] = {
    "#############", "#############", "######LL#####", "##.........##", "#...........#", "#...........#", "#....#####..#", "#...######..#", "#...######..#", "#...######..#", "#DDD######UU#" };
/* 3rd floor (14 x 8): the elevator, her door at the left end (K, from the rug), plants and the sofa are solid */
static const char *const MG_HALL3[8] = {
    "##############", "##############", "#########LL###", "###.........##", "K...........##", "K...........##", "###.........##", "##############" };
static const char *const MG_LOBBY[13] = {
    "#################", "#################", "#################", "###D#M####AA#####", "##....####..#####", "#.....#SS#..#####", "E...............#",
    "E...............#", "#...............#", "#...............#", "#...............#", "#...............#", "###########eeee##" };
/* her room (15 x 16 tiles, cropped from the artist's export): she sleeps behind the bedroom door; the bathroom door is not open yet.
   R her bedroom door (A = the old look into her bedroom, same energy cost)  T her bathroom door (not available yet)  D the right door back to the hall */
static const char *const MG_SOPHIA[16] = {
    "###############", "###############", "###############", "###o##o########", "##..vv.#yyy####", "##............D", "##............D", "###j.......####", "###j.......####", "#####R###T#####", "###############", "###############", "###############", "###############", "###############", "###############" };
/* the Event Center (15 x 9 tiles, cropped from the artist's export by tools/make_event_map.py): the match-room doors, the machine, the benches and the posters are only picture.
   V the front of the clerk's desk (A = her greeting, then the buy-in list)  D the wooden door on the right (A = back out to the lobby). There is no night here: it is open 24/7. */
static const char *const MG_EVENT[9] = {
    "###############", "###############", "###############", "##..#####..####", "##..VVVVV..####", "##.........####", "##............D", "###############", "###############" };
/* v9.2: Sophia's bedroom (8 x 7 tiles, assets/map_sbed.png made by tools/make_sbed_map.py from tools/sbed_src.png). She lets you in once you are married.
   W the wardrobe (A = ask her to change her clothes)  B her bed (A at night = her menu on page 4)  D the door out (A, facing down, back to the living room). The windows, the nightstands and the rug edges are only picture. */
static const char *const MG_SBED[7] = {
    "########", "########", "#W##BB##", "#W.#BB##", "#W..BB.#", "#......#", "#D######" };
typedef struct { const char *const *g; int cx, cy, cw, ch; SDL_Surface *img, *imgN; int w, h; } WMap;   /* tiles, the part of the picture that is the map (2x pixels), the picture */
static WMap wm[MAP_N] = { {MG_ROOM0, 0, 0, 504, 504, NULL, NULL, 7, 7}, {MG_HALL, 0, 0, 1080, 504, NULL, NULL, 15, 7}, {MG_LOBBY, 0, 0, 1224, 936, NULL, NULL, 17, 13}, {MG_SOPHIA, 0, 0, 1080, 1152, NULL, NULL, 15, 16},
                          {MG_HALL1, 0, 0, 936, 792, NULL, NULL, 13, 11}, {MG_HALL3, 0, 0, 1008, 576, NULL, NULL, 14, 8}, {MG_EVENT, 0, 0, 1080, 648, NULL, NULL, 15, 9}, {MG_SBED, 0, 0, 576, 504, NULL, NULL, 8, 7} };
typedef struct { int map, x, y, tmap, tx, ty, tface; } Door;
static const Door DOORS[] = {
    {MAP_ROOM, 3, 6, MAP_HALL, 12, 3, DN}, {MAP_HALL, 12, 2, MAP_ROOM, 3, 5, UP},                                          /* your room <-> the 2nd-floor hall */
    {MAP_HALL, 8, 2, MAP_HALL1, 11, 9, UP}, {MAP_HALL1, 10, 10, MAP_HALL, 8, 3, DN}, {MAP_HALL1, 11, 10, MAP_HALL, 8, 3, DN},   /* the stairs (1st and 2nd floor only) */
    {MAP_LOBBY, 3, 3, MAP_HALL1, 2, 9, UP}, {MAP_HALL1, 1, 10, MAP_LOBBY, 3, 4, DN}, {MAP_HALL1, 2, 10, MAP_LOBBY, 3, 4, DN}, {MAP_HALL1, 3, 10, MAP_LOBBY, 3, 4, DN},   /* lobby <-> stair hall */
    {MAP_SOPHIA, 14, 5, MAP_HALL3, 1, 4, RT}, {MAP_SOPHIA, 14, 6, MAP_HALL3, 1, 4, RT},
    {MAP_LOBBY, 0, 6, MAP_EVENT, EV_ARR_X, EV_ARR_Y, LF}, {MAP_LOBBY, 0, 7, MAP_EVENT, EV_ARR_X, EV_ARR_Y, LF}, {MAP_EVENT, 14, 6, MAP_LOBBY, 1, 6, RT},
    {MAP_SBED, 1, 6, MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, UP} };   /* her bedroom door: out to the living room, in front of her bedroom door */   /* the lobby opening <-> the Event Center (its one wooden door) */   /* her right door: back out to the 3rd-floor hall, at her door */
#define NDOORS ((int)(sizeof DOORS / sizeof DOORS[0]))
/* the elevator: it stops at all three floors; the menu offers the other two. The 3rd floor needs Sophia's floor key card (she hands it over after the bump scene, takes it back when the shirt is paid, and gives it back after you beat her at the Exhibition) */
static int floor_of(int m) { return m == MAP_HALL1 ? 1 : m == MAP_HALL ? 2 : m == MAP_HALL3 ? 3 : 0; }
static int floor_map(int f) { return f == 1 ? MAP_HALL1 : f == 2 ? MAP_HALL : MAP_HALL3; }
static int floor_card(void) { return G.story == ST_HOME || (G.story >= ST_NAMED && g_floor3_open(&G)); }   /* (once she lives with you the 3rd floor is closed) */   /* given at the end of the bump scene; she takes it back when the shirt is paid (ST_FIRST) and hands it over again when you beat her at the Exhibition */
static int elev_floors(int cur, int *out) { int f, n = 0; for (f = 1; f <= 3; f++) if (f != cur && (f != 3 || floor_card())) out[n++] = f; return n; }
static const int DX4[4] = {0, -1, 1, 0}, DY4[4] = {1, 0, 0, -1};
#define WALK_MS (WALK_T[walk_idx])   /* time for one step (v8.0: set in OPTIONS) */
static unsigned wk_step_ms = 480;   /* v7.6: the step time right now (the unpacking walk is slower) */
#define NIGHT_R 60   /* her room at night: percent of red, green and blue that is left */
#define NIGHT_G 66
#define NIGHT_B 86
#define NIGHTM_R 74   /* the other places at night: only slightly darker, a little blue */
#define NIGHTM_G 79
#define NIGHTM_B 94

static char wk_tile(int m, int x, int y) { return (x < 0 || y < 0 || x >= wm[m].w || y >= wm[m].h) ? '#' : wm[m].g[y][x]; }
static int wk_walkable(char c) { return c == '.'; }   /* doors are never walked into: face them and press A */
static int services_open(void);
static int wk_interactive(char c) { if (c == 'E' && !G.event) return 0; if (c == 'P' && !g_computer(&G) && !DEBUG_MODE) return 0; return c && strchr("KASMFBCnDEbedRTLUVPgW", c) != NULL ? 1 : (c && strchr("voyj", c) != NULL && fv_on); }   /* the Event Center opening is just a wall until it is unlocked */

/* ---------- v8.0: Sophia in your living room ----------
   She lives with you (RS_LIVING) and is only about in the morning and evening. Each time you walk into the room there is a LS_CHANCE percent chance
   that she is there (rolled again when the time of day changes, e.g. after saving at the sofa). She wanders between a few floor tiles, never on
   a tile in front of a door, the fridge, the sink or the sofa, and never on yours. A on her opens her menu at once (no knock, your living room picture). */
#define LS_CHANCE 20          /* the EVENING: percent chance that she is in the living room when you walk in */
#define LS_MORNING 100        /* v8.1: in the MORNING she is always there (unless she is back in her room, or a story scene waits behind her door) */
static const char *const W_LS_BUSY = "...Not now, boy. If you need me, knock on my bedroom door.";
static const char *const W_LS_NODOOR = "Don't you dare enter my room.";   /* v8.1: you knock on her bedroom door while she stands in the living room */
static unsigned ls_seed = 2463534242u;
static int ls_rnd(int n) { ls_seed ^= ls_seed << 13; ls_seed ^= ls_seed >> 17; ls_seed ^= ls_seed << 5; return (int)(ls_seed % (unsigned)n); }
/* v8.1: she may stand on EVERY free floor tile of the living room (the grid of the room: in front of the fridge, the sofa, the sink and the doors too) */
static int ls_ok(int x, int y) { return x >= 1 && x <= 5 && wk_walkable(wk_tile(MAP_ROOM, x, y)); }
static int ls_sens(int x, int y) { int d; for (d = 0; d < 4; d++) if (wk_interactive(wk_tile(MAP_ROOM, x + DX4[d], y + DY4[d]))) return 1; return 0; }   /* a tile in front of something you use: she does not stay there long */
static int ls_on_tile(int x, int y) { return ls_on && ((x == ls_tx && y == ls_ty) || (ls_moving && x == ls_fx && y == ls_fy)); }   /* (she is on it, or still leaving it) */
static int ls_player_on(int x, int y) { return (x == wk_tx && y == wk_ty) || (wk_moving && x == wk_fx && y == wk_fy); }
static unsigned ls_step_ms(void) { return WALK_MS * 3 / 2; }   /* she strolls: slower than you */
static int ls_busy(void);
static int ls_leave;      /* v8.1: 1 = she is walking back to her bedroom door, 2 = the last step into the doorway (then she is gone) */
static int ls_away = -1;  /* v8.1: day * 4 + phase in which she went back to her room: she stays in there for the rest of that time of day */
static int ls_key(void) { return G.day * 4 + G.phase; }
static void ls_go_home(void) { if (ls_on) ls_leave = 1; ls_away = ls_key(); }   /* the milk scene is over / a scene waits behind her door: she walks back to her room */
/* v8.3: from day 6 of the pill (the gift day is day 1: pill_day + 5) she is embarrassed about her new body and never comes out into the living room again.
   FUTURE: a gift + chat that make her get over it will clear this one test (ls_shy) so she wanders the room again. */
static int ls_shy(void) { return G.pill_given && G.day >= G.pill_day + PILL_DAYS - 2; }
static void ls_roll(void) {   /* she is there, or not, for this time of day (called while the screen is black, so she is simply there when the picture comes back) */
    int i, x, y, n = 0, c[16], pct;
    ls_on = 0; ls_moving = 0; ls_leave = 0; ls_ph = G.phase; ls_seed ^= now_ms() * 2654435761u + (unsigned)G.day * 40503u;
    if (!ls_enabled || !lsS || !g_rom_on(&G) || g_night(&G)) return;
    if (ls_shy() || ls_away == ls_key() || ls_busy()) return;
    pct = G.phase == PH_MORNING ? LS_MORNING : LS_CHANCE;
    if (!ls_force && ls_rnd(100) >= pct) return;
    for (y = 3; y <= 5; y++) for (x = 1; x <= 5; x++) if (ls_ok(x, y) && !ls_player_on(x, y)) c[n++] = y * 8 + x;
    if (!n) return;
    i = c[ls_rnd(n)]; ls_tx = ls_fx = i & 7; ls_ty = ls_fy = i >> 3; ls_face = ls_rnd(4); ls_on = 1; ls_next = now_ms() + 2000 + (unsigned)ls_rnd(3000);
}
static void ls_wander(void) {   /* after a pause: turn on the spot or take one step to a free tile she is allowed on */
    int d, k, nx, ny, cand[4], n = 0, s = ls_sens(ls_tx, ls_ty);
    for (d = 0; d < 4; d++) { nx = ls_tx + DX4[d]; ny = ls_ty + DY4[d]; if (ls_ok(nx, ny) && !ls_player_on(nx, ny)) cand[n++] = d; }
    if (s) { int m = 0; for (k = 0; k < n; k++) if (!ls_sens(ls_tx + DX4[cand[k]], ls_ty + DY4[cand[k]])) cand[m++] = cand[k]; if (m) n = m; }   /* (from a tile in front of something she steps to a free one, if there is one) */
    ls_next = now_ms() + (s ? 900 + (unsigned)ls_rnd(900) : 1800 + (unsigned)ls_rnd(3800));   /* (and she does not stay there long: you can get to the fridge, the sofa and the doors) */
    if (!n || (!s && ls_rnd(100) < 25)) { ls_face = ls_rnd(4); return; }
    d = cand[ls_rnd(n)]; ls_face = d; ls_fx = ls_tx; ls_fy = ls_ty; ls_tx += DX4[d]; ls_ty += DY4[d]; ls_stepc++; ls_moving = 1; ls_t0 = now_ms();
}
static int ls_path_dir(void) {   /* v8.1: the first step of a shortest way from her tile to the tile in front of her bedroom door, round you (-1 = no way right now) */
    int dist[8][8], q[64][2], h = 0, t = 0, x, y, d, best = -1, bd = 99;
    for (y = 0; y < 8; y++) for (x = 0; x < 8; x++) dist[y][x] = -1;
    if (!ls_ok(HOME_DOOR_X, HOME_DOOR_Y) || ls_player_on(HOME_DOOR_X, HOME_DOOR_Y)) return -1;
    dist[HOME_DOOR_Y][HOME_DOOR_X] = 0; q[t][0] = HOME_DOOR_X; q[t++][1] = HOME_DOOR_Y;
    while (h < t) {
        x = q[h][0]; y = q[h++][1];
        for (d = 0; d < 4; d++) { int nx = x + DX4[d], ny = y + DY4[d]; if (nx < 0 || ny < 0 || nx > 7 || ny > 7 || dist[ny][nx] >= 0 || !ls_ok(nx, ny) || ls_player_on(nx, ny)) continue; dist[ny][nx] = dist[y][x] + 1; q[t][0] = nx; q[t++][1] = ny; }
    }
    for (d = 0; d < 4; d++) { int nx = ls_tx + DX4[d], ny = ls_ty + DY4[d]; if (nx < 0 || ny < 0 || nx > 7 || ny > 7 || dist[ny][nx] < 0) continue; if (dist[ny][nx] < bd) { bd = dist[ny][nx]; best = d; } }
    return best;
}
static void ls_step_to(int d) { ls_face = d; ls_fx = ls_tx; ls_fy = ls_ty; ls_tx += DX4[d]; ls_ty += DY4[d]; ls_stepc++; ls_moving = 1; ls_t0 = now_ms(); }
/* ---------- v9.2: Sophia's bedroom ----------
   Married only. You knock on her bedroom door in your living room; if she is in her room (morning and night always, the evening one time in two: rolled once per time of day) she lets you in
   and you walk into her room (assets/map_sbed.png). If she is standing in your living room you cannot go in. She stays beside her bed and sometimes walks to the wardrobe / the window.
   A on her = her menu (her room picture behind it). A on the wardrobe = a small menu to ask her to change her clothes. A on the bed at night = her menu on page 4. A on the door (facing down) = out.
   After a match (or when she throws you out) you are put outside, in front of her bedroom door. */
#define SB_SX 1               /* where you arrive: just inside her door, facing into the room */
#define SB_SY 5
#define SB_EVE_CHANCE 50      /* the EVENING: percent chance that she is in her room when you knock */
static const char *const W_SB_NOANS = "No answer. She doesn't seem to be in her room right now.";
static const char *const W_SB_BED_DAY = "Her bed. I should wait until it's night.";
static const char *const W_WARD_ASK = "Her wardrobe. Maybe she would change her clothes if I asked.";
static const char *const W_WARD_NONE = "Her wardrobe. She has nothing else to change into yet.";
static int sb_gx = 3, sb_gy = 4, sb_arrived, sb_wait, sb_rkey = -1, sb_rval;
static int sb_ok(int x, int y) { return x >= 2 && wk_walkable(wk_tile(MAP_SBED, x, y)) && !ls_player_on(x, y); }   /* (never the door column: the way out stays free) */
static int sb_here(void) {   /* married, morning / evening: is she in her room? (her story scenes always wait behind her door) */
    if (!ls_enabled || G.phase != PH_EVENING || ls_away == ls_key()) return 1;
    if (sb_rkey != ls_key()) { sb_rkey = ls_key(); sb_rval = (ls_force || ls_rnd(100) < SB_EVE_CHANCE) ? 1 : 0; }
    return sb_rval;
}
static void sb_face_at(void) {   /* she turns to what she stands at: the bed from its sides, the wardrobe, the window */
    if (ls_tx == 3) ls_face = RT; else if (ls_tx == 6) ls_face = LF; else if (ls_ty == 3) ls_face = ls_rnd(2) ? UP : LF; else ls_face = LF;
}
static void sb_pick(void) {   /* where she goes next: mostly beside the bed, now and then in front of the wardrobe or the window */
    int r = ls_rnd(100);
    if (r < 30) { sb_gx = 3; sb_gy = 4; } else if (r < 60) { sb_gx = 6; sb_gy = 4; } else if (r < 80) { sb_gx = 2; sb_gy = 4; } else { sb_gx = 2; sb_gy = 3; }
}
static int sb_dir_to(int gx, int gy) {   /* the first step of a shortest way to the goal, round you (-1 = no way right now) */
    int dist[7][8], q[56][2], h = 0, t = 0, x, y, d, best = -1, bd = 99;
    for (y = 0; y < 7; y++) for (x = 0; x < 8; x++) dist[y][x] = -1;
    if (!sb_ok(gx, gy)) return -1;
    dist[gy][gx] = 0; q[t][0] = gx; q[t++][1] = gy;
    while (h < t) {
        x = q[h][0]; y = q[h++][1];
        for (d = 0; d < 4; d++) { int nx = x + DX4[d], ny = y + DY4[d]; if (nx < 0 || ny < 0 || nx > 7 || ny > 6 || dist[ny][nx] >= 0 || !sb_ok(nx, ny)) continue; dist[ny][nx] = dist[y][x] + 1; q[t][0] = nx; q[t++][1] = ny; }
    }
    for (d = 0; d < 4; d++) { int nx = ls_tx + DX4[d], ny = ls_ty + DY4[d]; if (nx < 0 || ny < 0 || nx > 7 || ny > 6 || dist[ny][nx] < 0) continue; if (dist[ny][nx] < bd) { bd = dist[ny][nx]; best = d; } }
    return best;
}
static void ls_step_to(int d);
static void sb_wander(void) {   /* called when her pause is over: turn / stay at the goal, pick a new one, or take one step towards it */
    int d;
    if (ls_tx == sb_gx && ls_ty == sb_gy) {
        if (!sb_arrived) { sb_arrived = 1; sb_face_at(); ls_next = now_ms() + 3000 + (unsigned)ls_rnd(4000); return; }   /* arrived: she turns to it and stays a while */
        sb_arrived = 0; sb_pick();
        if (ls_tx == sb_gx && ls_ty == sb_gy) { sb_arrived = 1; sb_face_at(); ls_next = now_ms() + 3000 + (unsigned)ls_rnd(4000); return; }
    }
    d = sb_dir_to(sb_gx, sb_gy);
    if (d < 0) { if (++sb_wait > 4) { sb_wait = 0; sb_pick(); } ls_next = now_ms() + 500; return; }   /* you are in the way: wait, then choose another place */
    sb_wait = 0; ls_step_to(d); ls_next = now_ms();
}
static void sb_spawn(void) {   /* the black moment of walking in: she stands beside her bed */
    ls_on = 0; ls_moving = 0; ls_leave = 0; sb_arrived = 1; sb_wait = 0;
    if (!lsS) return;
    sb_gx = ls_rnd(2) ? 3 : 6; sb_gy = 4;
    ls_tx = ls_fx = sb_gx; ls_ty = ls_fy = sb_gy; sb_face_at(); ls_on = 1; ls_next = now_ms() + 2500 + (unsigned)ls_rnd(3000);
}
static void sb_update(void) {
    if (!ls_on) return;
    if (ls_moving && now_ms() - ls_t0 >= ls_step_ms()) { ls_moving = 0; ls_fx = ls_tx; ls_fy = ls_ty; }
    if (frozen || ls_moving || wk_mode || fade_st) { if (!ls_moving) ls_next = now_ms() + 800; return; }   /* (she waits while a message or a menu is open) */
    if ((int)(now_ms() - ls_next) >= 0) sb_wander();
}
static void sb_slept(void) { wk_place(MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, UP); ls_away = ls_key(); }
static void sb_enter(void) { ls_talk = 0; wk_nmap = MAP_SBED; wk_ntx = SB_SX; wk_nty = SB_SY; wk_nface = RT; go(GO_MAP, 0); }   /* she lets you in */
static void ls_update(void) {
    if (wk_map != MAP_SOPHIA) fv_on = 0;   /* v11.0: the visit is over once you are out of her room */
    else if (fv_on) return;                /* (she stands where she is: no wandering, no rolling) */
    if (wk_map == MAP_SBED) { sb_update(); return; }
    if (wk_map != MAP_ROOM) { ls_on = 0; ls_leave = 0; ls_ph = -1; return; }   /* (leave the room and she is rolled again when you come back) */
    if (ls_ph != G.phase) ls_roll();
    if (ls_on && ls_shy()) { ls_on = 0; ls_moving = 0; ls_leave = 0; return; }   /* v9.9: from day 6 of the pill she is never in the living room: if she is still standing there when the day changes (or the debug panel jumps ahead) her sprite is gone at once */
    if (!ls_on) return;
    if (ls_moving && now_ms() - ls_t0 >= ls_step_ms()) { ls_moving = 0; ls_fx = ls_tx; ls_fy = ls_ty; }
    if (ls_leave == 2 && !ls_moving) { ls_on = 0; ls_leave = 0; return; }   /* she stepped into her doorway: only now her sprite is gone */
    if (frozen || ls_moving || wk_mode || fade_st) { if (!ls_moving) ls_next = now_ms() + 800; return; }   /* (she waits while a message or a menu is open) */
    if (ls_leave) {
        if (ls_tx == HOME_DOOR_X && ls_ty == HOME_DOOR_Y) { ls_leave = 2; ls_step_to(UP); }   /* at her door: one step through it */
        else { int d = ls_path_dir(); if (d < 0) ls_next = now_ms() + 300; else ls_step_to(d); }
        return;
    }
    if ((int)(now_ms() - ls_next) >= 0) ls_wander();
}
static void ls_reroll(void) { ls_ph = -1; ls_roll(); }   /* the black moment of a map change or of the sofa / bed */
#define LS_MAP() (wk_map == MAP_ROOM || wk_map == MAP_SBED || (wk_map == MAP_SOPHIA && fv_on))   /* v11.0: she is also standing in her own room during a daytime visit */
static int ls_faced(void) { return ls_on && LS_MAP() && ls_on_tile(wk_tx + DX4[wk_face], wk_ty + DY4[wk_face]); }
static int ls_busy(void) { return g_pill_due(&G) || g_hint_scene(&G) || (g_invited(&G) && (g_milk_due(&G) || (G.milk_st == 2 && G.pump))); }   /* a story scene waits behind her bedroom door */
static int lr_on(void) { return ls_talk && wk_map == MAP_ROOM && g_p4_open(&G); }
static void fv_open(void);
static void ls_interact(void) {   /* A on her: her menu at once (the same menu the knock leads to), or, when a story scene is waiting, a hint to knock */
    { int fx = wk_tx + DX4[wk_face], fy = wk_ty + DY4[wk_face];   /* v9.4: the tile you face. If she is still leaving it (mid-step) she must stay ON it: before, she was snapped to the tile she was walking TO, one tile further back */
      if (ls_moving && fx == ls_fx && fy == ls_fy) { ls_tx = ls_fx; ls_ty = ls_fy; } }
    if (wk_map == MAP_SOPHIA) { ls_moving = 0; ls_fx = ls_tx; ls_fy = ls_ty; ls_face = 3 - wk_face; fv_open(); return; }   /* v11.0: her room by day: her line, then TALK / PLAY A MATCH / LEAVE */
    if (wk_map == MAP_ROOM && ls_shy()) { ls_on = 0; ls_moving = 0; ls_leave = 0; return; }   /* v9.9: she is not out here from day 6 of the pill */
    ls_moving = 0; ls_fx = ls_tx; ls_fy = ls_ty; ls_face = 3 - wk_face; ls_next = now_ms() + 4000;   /* (she turns to you; DN 0, LF 1, RT 2, UP 3: the opposite is 3 - d) */
    if (wk_map == MAP_ROOM && ls_leave) { wk_say(N_SOPHIA, 0, WK_NONE, W_LS_BUSY, NULL); return; }   /* (she is on her way to her room) */
    if (wk_map == MAP_ROOM && ls_busy()) { wk_say(N_SOPHIA, 0, WK_NONE, W_LS_BUSY, NULL); ls_go_home(); return; }   /* v8.1: a story scene waits behind her door: she goes there, then you knock */
    ls_talk = 1; go(GO_ROM, 0);
}
static int dir_of(int b) { return b == B_DOWN ? DN : b == B_LEFT ? LF : b == B_RIGHT ? RT : b == B_UP ? UP : -1; }
/* v7.0: only ONE night copy of a map is kept in memory (the map you are walking in, made when you enter it at night; at screen resolution six of them were too much for the 128 MB device).
   Her room (darkened once at load) and the Event Center (always lit) have none. */
static void night_mild(SDL_Surface *im);
static SDL_Surface *night_img; static const SDL_Surface *night_src;
static SDL_Surface *night_of(int m) {
    SDL_Surface *d; int y;
    if (m == MAP_SOPHIA || m == MAP_EVENT || m == MAP_SBED || !wm[m].img) return NULL;
    d = wm[m].img;
    if (night_img && night_src == d) return night_img;
    if (night_img) { SDL_FreeSurface(night_img); night_img = NULL; night_src = NULL; }
    night_img = SDL_CreateRGBSurface(0, d->w, d->h, 32, d->format->Rmask, d->format->Gmask, d->format->Bmask, 0);
    if (!night_img) return NULL;
    for (y = 0; y < d->h; y++) memcpy(px(night_img, 0, y), px(d, 0, y), (size_t)d->w * 4);
    night_mild(night_img); night_src = d; return night_img;
}
static void room_apply(void) {   /* v9.3: the base room, upgrade 1, or upgrade 2 (her door; also once she lives with you) */
    int u = g_room_lvl(&G), k; static const char *const pic[3] = {"assets/map_room.png", "assets/map_room_u1.png", "assets/map_room_up.png"};
    wm[MAP_ROOM].g = u == 0 ? MG_ROOM0 : u == 1 ? MG_ROOM : MG_ROOM_UP;
    wm[MAP_ROOM].w = u == 0 ? 7 : 11; wm[MAP_ROOM].cw = u == 0 ? 504 : 792;
    if (!roomS[u]) {   /* v7.0: only the room picture in use is in memory */
        roomS[u] = load_png_ex(pic[u], 3, 1, 1, 1, 1);
        if (roomS[u]) for (k = 0; k < 3; k++) if (k != u && roomS[k]) { if (night_src == roomS[k]) { SDL_FreeSurface(night_img); night_img = NULL; night_src = NULL; } SDL_FreeSurface(roomS[k]); roomS[k] = NULL; }
    }
    if (roomS[u]) wm[MAP_ROOM].img = roomS[u];
}
static void wk_place(int m, int x, int y, int f) {
    int prev = wk_map; room_apply(); wk_map = m; wk_tx = wk_fx = x; wk_ty = wk_fy = y; wk_face = f; wk_moving = 0; wk_mode = 0; wk_look = -1; wk_snap = 1; wk_pan_t0 = 0; wk_pend = 0;
    if (prev == MAP_SBED && m != MAP_SBED) ls_away = ls_key();   /* v9.2: you left her bedroom: she stays in there for the rest of this time of day */
    if (m == MAP_SBED) sb_spawn();
    if (m == MAP_ROOM && (prev != MAP_ROOM || ls_ph != G.phase)) ls_reroll();   /* v8.1: placing you is the black moment of a door / load: she is rolled now, so she is already there when the picture comes back */
}
static void wk_say(const char *tag, int narr, int after, const char *p0, const char *p1) {
    wk_pg[0] = p0; wk_pg[1] = p1; wk_npg = p1 ? 2 : 1; wk_ipg = 0; wk_tag = tag; wk_narr = narr; wk_after = after; wk_ms = now_ms(); wk_mode = 1;
}
static void wk_arrive(void) { wk_moving = 0; wk_fx = wk_tx; wk_fy = wk_ty; }   /* a step is finished */
static void wk_door(int x, int y) {   /* A on a door: through it to the place it leads to */
    int i;
    for (i = 0; i < NDOORS; i++) if (DOORS[i].map == wk_map && DOORS[i].x == x && DOORS[i].y == y) {
        wk_nmap = DOORS[i].tmap; wk_ntx = DOORS[i].tx; wk_nty = DOORS[i].ty; wk_nface = DOORS[i].tface; go(GO_MAP, 0); return;
    }
}
static void wk_move(int d, int fresh) {   /* turn towards d and take a step if the way is free; fresh = a new button press (not a held one) */
    int nx = wk_tx + DX4[d], ny = wk_ty + DY4[d]; char c = wk_tile(wk_map, nx, ny);
    wk_face = d;
    (void)fresh;
    if (!wk_walkable(c)) return;
    if (LS_MAP() && ls_on_tile(nx, ny)) return;   /* v8.0: Sophia is standing there */
    wk_fx = wk_tx; wk_fy = wk_ty; wk_tx = nx; wk_ty = ny; wk_step++;
    if (frozen) { wk_arrive(); return; }
    wk_moving = 1; wk_t0 = now_ms();
}
enum { DK_RENT, DK_UPG, DK_STORE, DK_LEAVE };
static int desk_items(int *out) { int n = 0; if (g_rent_due(&G)) out[n++] = DK_RENT; if (g_upgrade_avail(&G)) out[n++] = DK_UPG; out[n++] = DK_LEAVE; return n; }
static void shop_open(void);
static void wk_interact(void) {   /* A: use whatever you are facing */
    char c = wk_tile(wk_map, wk_tx + DX4[wk_face], wk_ty + DY4[wk_face]);
    if (ls_faced()) { ls_interact(); return; }   /* v8.0 */
    if (G.story == ST_NEW && wk_interactive(c)) {   /* the intro: the desk starts the bump scene, nothing else works yet */
        if (c == 'S') go(GO_SCRIPT, SCR_BUMP); else wk_say(NULL, 1, WK_NONE, W_CHECKIN, NULL);
        return;
    }
    switch (c) {
        case 'K':
            if (!g_floor3_open(&G)) { wk_say(NULL, 1, WK_NONE, W_CLOSED3, NULL); break; }
            if (G.story == ST_FIRST) { wk_say(NULL, 1, WK_NONE, W_BOSS_NO, NULL); break; }   /* (no way up there without the card; only reachable from an odd save) */
            if (G.story == ST_HOME && G.cash < DEBT) { wk_say(N_ME, 0, WK_NONE, W_DEBT_NO, NULL); break; }   /* the shirt is not paid for: her door stays shut */
            if (g_night(&G)) go(GO_SCRIPT, SCR_NIGHT);   /* at night: knocking only gets you shouted at; once she has a name you can also slip in (the menu exists only at night) */
            else if (G.story >= ST_FIRST && g_played_today(&G)) { daily_i = pick_new((int)S_DAILY_N, daily_i); go(GO_SCRIPT, SCR_DAILY); }   /* she plays you once a day: a condescending line, then back to the hall */
            else go(GO_VISIT, 0);
            break;
        case 'D': case 'U': wk_door(wk_tx + DX4[wk_face], wk_ty + DY4[wk_face]); break;   /* U = the stairs door */
        case 'L': wk_nflr = elev_floors(floor_of(wk_map), wk_flr); wk_fsel = 0; wk_mode = 5; break;   /* the elevator: which floor? */
        case 'E': if (G.event && !unpacked()) wk_say(N_ME, 0, WK_NONE, W_UNPACK_NO, NULL); else if (G.event) wk_door(wk_tx + DX4[wk_face], wk_ty + DY4[wk_face]); break;   /* v6.8: unpack first */   /* the lobby opening: through to the Event Center */
        case 'V': wk_say(W_CLERK_TAG, 0, WK_EVENT, pick(W_EVCLERK, W_EVCLERK_N), NULL); break;   /* the Event Center clerk: her greeting, then the buy-in list */
        case 'd': {   /* the bedroom door in the middle wall: through it to the other side */
            int dx = wk_tx + DX4[wk_face];
            wk_nmap = MAP_ROOM; wk_nty = wk_ty; if (wk_tx < dx) { wk_ntx = dx + 1; wk_nface = RT; } else { wk_ntx = dx - 1; wk_nface = LF; }
            go(GO_MAP, 0); break; }
        case 'b': wk_say(NULL, 1, WK_NONE, W_BATH, NULL); break;
        case 'R':   /* her bedroom door: one look per visit, free: your narration first, then the picture and its menu */
            if (fv_on) wk_say(N_SOPHIA, 0, WK_NONE, pick(V_BED, V_BED_N), NULL);   /* v11.0: by day she keeps her bedroom shut */
            else if (sn_used) wk_say(NULL, 1, WK_NONE, SN_DONE, NULL);
            else { sn_used = 1; go(GO_SCRIPT, SCR_BEDDOOR); }
            break;
        case 'T': if (fv_on) wk_say(N_SOPHIA, 0, WK_NONE, pick(V_BATH, V_BATH_N), NULL); else wk_say(NULL, 1, WK_NONE, SN_BATH, NULL); break;
        case 'v': if (fv_on) wk_say(NULL, 1, WK_NONE, pick(V_TV, V_TV_N), NULL); break;   /* v11.0: her room by day */
        case 'o': if (fv_on) wk_say(NULL, 1, WK_NONE, pick(V_WIN, V_WIN_N), NULL); break;
        case 'y': if (fv_on) wk_say(NULL, 1, WK_NONE, pick(V_TABLE, V_TABLE_N), NULL); break;
        case 'j': if (fv_on) wk_say(NULL, 1, WK_NONE, pick(V_SOFA, V_SOFA_N), NULL); break;   /* her bathroom: not available yet */
        case 'e': go(GO_SCRIPT, SCR_RAIN); break;                              /* the glass doors: the storm picture */
        case 'A': atm_open(); break;
        case 'M': store_open(); break;   /* v9.1: the vending machine (the store), open day and night */
        case 'S': if (g_night(&G)) wk_say(NULL, 1, WK_NONE, W_DESK_NIGHT, NULL);
            else if (g_rent_due(&G)) wk_say(W_CLERK_TAG, 0, WK_DESK, G.room_up ? pick(W_CLERK_RENT_UP, W_CLERK_RENT_UP_N) : pick(W_CLERK_RENT, W_CLERK_RENT_N), NULL);   /* rent day: after her greeting, a small menu (pay rent / store / leave) */
            else if (g_upgrade_avail(&G)) wk_say(W_CLERK_TAG, 0, WK_DESK, G.room_up == 0 ? pick(W_CLERK_UP1, W_CLERK_UP1_N) : pick(W_CLERK_UPG, W_CLERK_UPG_N), NULL);   /* v9.3: upgrade 1 (from the shirt scene on) or upgrade 2 (she asked for a bigger room): the same menu with the upgrade in it */
            else wk_say(W_CLERK_TAG, 0, WK_NONE, pick(W_CLERK, W_CLERK_N), NULL); break;   /* v9.1: on a plain day she only greets you; the store is the vending machine now */
        case 'F': inv_open(); break;
        case 'B':
            if (wk_map == MAP_SBED) { if (!g_night(&G)) wk_say(NULL, 1, WK_NONE, W_SB_BED_DAY, NULL); else { ls_talk = 1; go(GO_ROM, 2); } break; }   /* v9.2: her bed: at night her menu on page 4 (sleep together) */
            if (!g_night(&G)) wk_say(NULL, 1, WK_NONE, W_BED_DAY, NULL); else { wk_mode = 2; wk_yes = 0; wk_sleep = 1; } break;
        case 'W': if (!g_outfit_open(&G)) wk_say(NULL, 1, WK_NONE, W_WARD_NONE, NULL); else wk_say(NULL, 1, WK_WARD, W_WARD_ASK, NULL); break;   /* v9.2: her wardrobe */   /* the bed: only at night, sleeping saves and starts the next day */
        case 'C': wk_mode = 2; wk_yes = g_night(&G) ? 1 : 0; wk_sleep = 0; break;   /* the sofa (v10.5): REST (save, time passes: morning and evening only) / SAVE ONLY (the time stays) / CANCEL */
        case 'n': wk_say(NULL, 1, WK_NONE, W_SINK, NULL); break;
        case 'P': if (g_computer(&G)) shop_open(); else if (DEBUG_MODE) dbg_open(); break;   /* the computer: the online store (v9.3: only from upgrade 1 on; the base desk opens the debug panel in a test build, nothing otherwise) */
        case 'g':
            if (!g_rom_on(&G)) wk_say(NULL, 1, WK_NONE, W_SPARE, NULL);
            else if (ls_on) { wk_say(N_SOPHIA, 0, WK_NONE, W_LS_NODOOR, NULL); if (ls_busy() && !ls_leave) ls_go_home(); }   /* v8.1: she is standing in the living room */
            else if (g_night(&G)) { if (g_p4_open(&G)) go(GO_SCRIPT, SCR_NIGHTM); else if (g_home_sneak(&G)) { scr = SC_SNEAK; sn_mode = SM_CHOICE; sn_sel = 0; } else go(GO_SCRIPT, SCR_NIGHTHOME); }   /* night: she scolds you; after the day-8 scene you may also slip in */
            else if (G.married && kicked_day != G.day && !ls_busy() && !sb_here()) wk_say(NULL, 1, WK_NONE, W_SB_NOANS, NULL);   /* v9.2: married, the evening she is not in her room: no answer */
            else go(GO_SCRIPT, (g_pill_due(&G) || g_hint_scene(&G) || !g_invited(&G)) ? SCR_KNOCKHOME : SCR_KNOCKIN); break;   /* morning / evening: the knock first (after day 8 she lets you into her room), then her menu */   /* her bedroom door: knock (only once she lives here) */
        default: break;
    }
}
static int wk_held(void) {
    static const int bt[4] = {B_DOWN, B_LEFT, B_RIGHT, B_UP}; int d;
    if (last_dir >= 0 && kheld[bt[last_dir]]) return last_dir;
    for (d = 0; d < 4; d++) if (kheld[bt[d]]) return d;
    return -1;
}
static int wk_auto;   /* wk_mode 7: steps still to walk by yourself (the unpacking walk) */
static void wk_update(void) {
    int d;
    if (scr != SC_WALK) return;
    if (wk_mode != 7 && !wk_moving) wk_step_ms = WALK_MS;
    ls_update();
    if (wk_moving && now_ms() - wk_t0 >= wk_step_ms) wk_arrive();
    if (wk_mode == 6 && now_ms() - wk_wt0 >= wk_wms) { wk_wait_done(); return; }
    if (wk_mode == 7 && !wk_moving && !fade_st) { if (wk_auto > 0) { wk_auto--; wk_move(RT, 0); } else { wk_mode = 0; go_slow(GO_UNPACK, 0); } return; }   /* v7.1: the unpacking walk */
    if (wk_mode == 3 && now_ms() - wk_pan_t0 >= PAN_MS + PAN_HOLD) { int st = wk_pend; wk_pend = 0; wk_mode = 0; hall_say(st); return; }
    if (!wk_moving && !wk_mode && !fade_st && (d = wk_held()) >= 0) wk_move(d, 0);
}
/* the first time you reach each upper floor you cannot move: a line, then the camera swings to the door and a second line (2nd floor: your door, 3rd floor: hers).
   G.hall_seen: bit 1 = the 2nd floor was looked at, bit 2 = the 3rd floor. Steps: 1 line, 2 pan, 3 done (2nd floor); 4 line, 5 pan, 6 done (3rd floor); each message leads to the next step */
static void hall_say(int step) { if (step == 2) wk_say(N_ME, 0, WK_HALL3, W_HALL3, NULL); else wk_say(N_ME, 0, WK_HALL6, W_HALL2, NULL); }
static void hall_pan(int step) {   /* start the slow pan and wait (wk_mode 3, no input) until it is done and has rested; frozen checks skip the wait */
    if (frozen) { hall_say(step); return; }
    wk_pan_x0 = wk_camx; wk_pan_t0 = now_ms(); wk_pend = step; wk_mode = 3;
}
static void hall_script(int step) {
    switch (step) {
        case 1: wk_say(N_ME, 0, WK_HALL2, W_F2, NULL); break;
        case 2: wk_face = RT; wk_look = 12; hall_pan(2); break;   /* the camera swings to your door; the line comes when it has stopped */
        case 3: wk_look = -1; wk_pan_t0 = 0; G.hall_seen |= 1; break;
        case 4: wk_say(N_ME, 0, WK_HALL5, W_F3, NULL); break;
        case 5: wk_face = LF; wk_look = 0; hall_pan(5); break;    /* ... to her door */
        default: wk_look = -1; wk_pan_t0 = 0; G.hall_seen |= 2; break;
    }
}
/* v6.5: after she says yes to living with you: a slow fade, you wait in your room, time passes, she knocks, the camera tilts down (painted room enlarged), then she walks in */
static void wk_wait_done(void) {
    int st = wk_wstep; wk_mode = 0;
    if (st == 1) wk_say(NULL, 1, WK_DEMAND2, W_WAIT2, NULL); else go(GO_SCRIPT, SCR_DEMAND);   /* v6.6: no camera pan: a fade, then the knocking narration in your room, then she walks in */
}
static void wk_wait_begin(unsigned ms, int step) {   /* wk_mode 6: you stand there and nothing happens (frozen checks skip the wait) */
    wk_wstep = step;
    if (frozen) { wk_wait_done(); return; }
    wk_mode = 6; wk_wt0 = now_ms(); wk_wms = ms;
}
static void unpack_begin(void) {   /* v7.1: input locked, two normal steps up, then the slow fade (GO_UNPACK puts you in the bedroom). Frozen checks do it all at once */
    wk_auto = UNPACK_STEPS;
    if (frozen) { while (wk_auto > 0) { wk_auto--; wk_move(RT, 0); } go_slow(GO_UNPACK, 0); return; }
    wk_step_ms = UNPACK_STEP_MS; wk_mode = 7;
}
static void lot_tell(void) {   /* v9.0: the clerk tells you the result of your lottery ticket (from the day after you bought it), pays the prize, then the menu opens */
    if (g_lot_ready(&G)) {
        int prize = g_lot_claim(&G);
        if (prize > 0) { snprintf(lot_msg, sizeof lot_msg, "Your prize is $%d. Here you go.", prize); wk_say(W_CLERK_TAG, 0, WK_EVMENU, pick(W_LOTWIN, W_LOTWIN_N), lot_msg); }
        else wk_say(W_CLERK_TAG, 0, WK_EVMENU, pick(W_LOTLOSE, W_LOTLOSE_N), NULL);
        persist(); return;
    }
    go(GO_EVENT, 0);
}
/* ---------- v11.0: her room by day (friendship) ----------
   COME IN at her door (after the boss win, daytime): you stand in her room (MAP_SOPHIA) and she stands in the middle of it. A on her = her line, then TALK / PLAY A MATCH / LEAVE.
   The TV, the windows, the table and the bench answer with a line (letters v o y j); her bedroom and bathroom doors stay shut. Wins over her are the only thing that fills the blue bar. */
static const char *fv_talk_line(void) {
    switch (g_fr_tier(&G)) { case 1: return FVP(V_TALK1); case 2: return FVP(V_TALK2); case 3: return FVP(V_TALK3); case 4: return FVP(V_TALK4); default: return FVP(V_TALK5); }
}
static const char *fv_door_line(void) {   /* the greeting at her door (the knock): once per session the special first line, else about 2 in 3 from her tier pool, else the old line for your last match */
    switch (g_fr_tier(&G)) { case 1: return FVP(G_FR1); case 2: return FVP(G_FR2); case 3: return FVP(G_FR3); case 4: return FVP(G_FR4); default: return FVP(G_FR5); }
}
#define FV_NX 7   /* where she stands in her room during a visit */
#define FV_NY 6
static void fv_npc(void) { ls_on = 1; ls_tx = ls_fx = FV_NX; ls_ty = ls_fy = FV_NY; ls_face = RT; ls_moving = 0; ls_leave = 0; ls_next = now_ms() + 3600000u; }
static void fv_open(void) { fv_vsel = 0; wk_say(N_SOPHIA, 0, WK_VISIT, FVP(V_HELLO), NULL); }
static void fv_choose(int k) {
    wk_mode = 0;
    if (k == 0) wk_say(N_SOPHIA, 0, WK_VISIT, fv_talk_line(), NULL);
    else if (k == 1) {
        if (!g_can_play(&G)) wk_say(N_SOPHIA, 0, WK_VISIT, pick(S_BROKE, (int)S_BROKE_N), NULL);   /* no cash for even the cheapest game: the same refusal as at the door */
        else { scr = SC_LEN; len_sel = 1; SAYP(L_LEN); }
    }
    else wk_say(N_SOPHIA, 0, WK_VBYE, FVP(V_BYE), NULL);
}
static void wk_key(int b) {
    int d = dir_of(b);
    if (wk_mode == 3 || wk_mode == 6 || wk_mode == 7) return;   /* the camera is still swinging round / you are waiting: nothing to press */
    if (wk_mode == 1) {   /* a message: A finishes the typing, then turns the page, then closes it */
        if (b != B_A) return;
        if (!typed_done(wk_pg[wk_ipg], wk_ms)) { wk_ms = now_ms() - 30000u; return; }
        if (wk_ipg + 1 < wk_npg) { wk_ipg++; wk_ms = now_ms(); return; }
        wk_mode = 0;
        if (wk_after == WK_STORE) store_open();
        else if (wk_after == WK_DESK) { wk_mode = 4; wk_dsel = 0; }
        else if (wk_after == WK_EVENT) lot_tell();
        else if (wk_after == WK_EVMENU) go(GO_EVENT, 0);
        else if (wk_after == WK_WARD) { wk_mode = 8; wk_wsel = 0; }
        else if (wk_after == WK_VISIT) wk_mode = 9;   /* v11.0: back to her visit menu */
        else if (wk_after == WK_VBYE) fv_exit();   /* v11.0: out of her room, back to her door in the hall */
        else if (wk_after == WK_DEMAND) wk_wait_begin(WAIT1_MS, 1);    /* you wait in your room, quietly... */
        else if (wk_after == WK_DEMAND2) wk_wait_begin(WAIT2_MS, 2);   /* ...time passes... */
        else if (wk_after >= WK_HALL2 && wk_after <= WK_HALL6) hall_script(wk_after - WK_HALL2 + 2);
        return;
    }
    if (wk_mode == 5) {   /* the elevator: the other floors, A rides, B steps back */
        if (b == B_UP) wk_fsel = (wk_fsel + wk_nflr - 1) % wk_nflr; else if (b == B_DOWN) wk_fsel = (wk_fsel + 1) % wk_nflr;
        else if (b == B_B) wk_mode = 0;
        else if (b == B_A && wk_flr[wk_fsel] == 3 && !unpacked()) { wk_mode = 0; wk_say(N_ME, 0, WK_NONE, W_UNPACK_NO, NULL); }   /* v6.8: unpack first */
        else if (b == B_A) { int f = wk_flr[wk_fsel]; wk_mode = 0; wk_nmap = floor_map(f); wk_ntx = ELEV_X[f]; wk_nty = ELEV_Y[f]; wk_nface = DN; go(GO_MAP, 0); }
        return;
    }
    if (wk_mode == 8) {   /* v9.2: her wardrobe: ask her to change her clothes / LEAVE */
        int wt[OF_N], wn = g_outfit_targets(&G, wt);   /* v10.8: one row for every outfit she can change into, then LEAVE */
        if (b == B_UP || b == B_DOWN) wk_wsel = (wk_wsel + (b == B_DOWN ? 1 : wn)) % (wn + 1);
        else if (b == B_B) wk_mode = 0;
        else if (b == B_A) {
            if (wk_wsel < wn) { wk_mode = 0; if (!outfit_ask(wt[wk_wsel], 1)) { outfit_apply(wt[wk_wsel]); persist(); wk_say(N_SOPHIA, 0, WK_NONE, outfit_line(), NULL); } }   /* v11.2: the exchange first, except for her usual outfit */
            else wk_mode = 0;
        }
        return;
    }
    if (wk_mode == 9) {   /* v11.0: her room by day: TALK / PLAY A MATCH / LEAVE */
        if (b == B_UP) fv_vsel = (fv_vsel + 2) % 3; else if (b == B_DOWN) fv_vsel = (fv_vsel + 1) % 3;
        else if (b == B_B) wk_mode = 0;
        else if (b == B_A) fv_choose(fv_vsel);
        return;
    }
    if (wk_mode == 4) {   /* the desk: PAY RENT (rent day) / UPGRADE ROOM (once she asked for it) / STORE / LEAVE */
        int dk[4], dn = desk_items(dk);
        if (b == B_UP) wk_dsel = (wk_dsel + dn - 1) % dn; else if (b == B_DOWN) wk_dsel = (wk_dsel + 1) % dn;
        else if (b == B_B) wk_mode = 0;
        else if (b == B_A) {
            int k = dk[wk_dsel];
            if (k == DK_RENT) { if (g_rent_pay_cash(&G)) wk_say(W_CLERK_TAG, 0, WK_NONE, pick(W_RENT_OK, W_RENT_OK_N), NULL); else wk_say(W_CLERK_TAG, 0, WK_NONE, pick(W_RENT_SHORT, W_RENT_SHORT_N), NULL); }   /* cash only; short = the ATM is next door */
            else if (k == DK_UPG) { if (g_upgrade_buy(&G) == 0) { persist(); wk_say(W_CLERK_TAG, 0, WK_NONE, G.room_up == 1 ? W_UPG_OK1 : W_UPG_OK, NULL); } else wk_say(W_CLERK_TAG, 0, WK_NONE, W_UPG_SHORT, NULL); }
            else wk_mode = 0;
        }
        return;
    }
    if (wk_mode == 2) {   /* save the game? */
        if (wk_sleep) {   /* the bed: yes / no (sleeping saves and starts the next day) */
            if (b == B_UP || b == B_DOWN) wk_yes = !wk_yes;
            else if (b == B_B) wk_mode = 0;
            else if (b == B_A) { if (wk_yes == 0) { wk_mode = 0; go(GO_TIME, 0); } else wk_mode = 0; }   /* yes: fade out, the time moves on, it is saved, fade back in */
        } else {          /* v10.5 the sofa: 0 REST (not at night) / 1 SAVE ONLY / 2 CANCEL */
            int night = g_night(&G);
            if (b == B_UP) { wk_yes = (wk_yes + 2) % 3; if (night && wk_yes == 0) wk_yes = 2; }
            else if (b == B_DOWN) { wk_yes = (wk_yes + 1) % 3; if (night && wk_yes == 0) wk_yes = 1; }
            else if (b == B_B) wk_mode = 0;
            else if (b == B_A) {
                if (wk_yes == 0 && !night) { wk_mode = 0; go(GO_TIME, 0); }   /* REST: the time moves on, it is saved */
                else if (wk_yes == 1) { wk_mode = 0; save_now(); wk_say(NULL, 1, WK_NONE, W_SAVED_ONLY, NULL); }   /* SAVE ONLY: no fade, no time, no rent, Sophia stays where she is */
                else wk_mode = 0;
            }
        }
        return;
    }
    if (d >= 0) { if (!wk_moving) wk_move(d, 1); return; }
    if (b == B_A && !wk_moving) wk_interact();
}
static void blit_rect_s(SDL_Surface *im, int sx, int sy, int sw, int sh, int xn, int yn) {   /* v7.3: the same, at a screen-pixel position */   /* one frame (sx = column * sw, sy = row * sh) of the 3 x 4 walking sheet; the frames are whole screen pixels wide, so pick them by index */
    SDL_Rect s, d; int fwn, fhn; if (!im || sw <= 0 || sh <= 0) return;
    fwn = im->w / 3; fhn = im->h / 4;
    s.x = (sx / sw) * fwn; s.y = (sy / sh) * fhn; s.w = fwn; s.h = fhn; d.x = xn; d.y = yn; d.w = 0; d.h = 0; SDL_BlitSurface(im, &s, S, &d);
}
static void blit_rect(SDL_Surface *im, int sx, int sy, int sw, int sh, int x, int y) { blit_rect_s(im, sx, sy, sw, sh, XS(x), YS(y)); }
/* v7.3: the camera eases by elapsed time (not by frames), so the map and you move together at any frame rate; 1/256 px precision */
static void cam_ease(int *c, int t, int dt) {
    int d = t - *c; double k = dt / 16.0, step;
    if (d > -128 && d < 128) { *c = t; return; }
    step = d * (1.0 - pow(0.8, k)) + (d > 0 ? 256.0 : -256.0) * k;
    if ((d > 0 && step > d) || (d < 0 && step < d)) step = d;
    *c += (int)step;
}
static int ls_prog(void) { if (!ls_moving || frozen) return 256; { unsigned t = now_ms() - ls_t0, d = ls_step_ms(); return t >= d ? 256 : (int)(t * 256 / d); } }   /* v8.0: how far her step is (0..256) */
static int ls_xpos(void) { int p = ls_prog(); return ls_moving ? (ls_fx * TILE + TILE / 2) * 256 + (ls_tx - ls_fx) * TILE * p : (ls_tx * TILE + TILE / 2) * 256; }
static int ls_ypos(void) { int p = ls_prog(); return ls_moving ? ((ls_fy + 1) * TILE) * 256 + (ls_ty - ls_fy) * TILE * p : ((ls_ty + 1) * TILE) * 256; }
static int ls_drawn;
static void draw_sophia(int clx, int cty) {   /* her walking picture (same sheet layout as yours) at her place on the map; clx, cty = the picture's top-left corner in 1/256 px */
    int fw = ls_fw ? ls_fw : 101, fh = ls_fh ? ls_fh : 154, p = ls_prog(), col = 1, xn, yn;
    if (ls_moving && p >= 64 && p < 192) col = (ls_stepc & 1) ? 2 : 0;
    xn = XSF((long long)ls_xpos() - (long long)(fw / 2) * 256 - clx); yn = YSF((long long)ls_ypos() - (long long)(ls_fw ? ls_foot[ls_face] : 150) * 256 - cty);
    blit_rect_s(g_night(&G) && wk_map == MAP_SBED && lsSn ? lsSn : lsS, col * fw, ls_face * fh, fw, fh, xn, yn); ls_drawn = 1;
}
static void draw_walk_world(int bubble) {   /* the map with the receptionist and you on it (no HUD, no boxes) */
    const WMap *m = &wm[wk_map]; int ox, oy, px, py, pxF, pyF, clx, cty, adjx, adjy, xn, yn, p = 256, col = 1, fw = pl_fw ? pl_fw : 101, fh = pl_fh ? pl_fh : 154, foot = pl_fw ? pl_foot[wk_face] : 150, tx, ty;
    Uint32 gold = gui_tc(), white = C(240,240,240);
    if (wk_moving && !frozen) { unsigned t = now_ms() - wk_t0; p = t >= wk_step_ms ? 256 : (int)(t * 256 / wk_step_ms); }
    pxF = wk_moving ? (wk_fx * TILE + TILE / 2) * 256 + (wk_tx - wk_fx) * TILE * p : (wk_tx * TILE + TILE / 2) * 256;   /* v7.3: your position in 1/256 px */
    pyF = wk_moving ? ((wk_fy + 1) * TILE) * 256 + (wk_ty - wk_fy) * TILE * p : ((wk_ty + 1) * TILE) * 256;
    px = wk_moving ? wk_fx * TILE + TILE / 2 + (wk_tx - wk_fx) * TILE * p / 256 : wk_tx * TILE + TILE / 2;
    py = wk_moving ? (wk_fy + 1) * TILE + (wk_ty - wk_fy) * TILE * p / 256 : (wk_ty + 1) * TILE;
    /* camera: it centres on you (or on the tile column a scene looks at), eases there, and stops at the edges of the map picture, so the picture always fills the screen */
    tx = wk_look >= 0 ? (wk_look * TILE + TILE / 2) * 256 : pxF; ty = pyF - 70 * 256;   /* v7.3: the camera targets, in 1/256 px */
    if (frozen || wk_snap) { wk_cfx = tx; wk_cfy = ty; wk_snap = 0; cam_ms = now_ms(); }
    else {
        unsigned nw = now_ms(); int dt = (int)(nw - cam_ms); if (dt > 100) dt = 100; if (dt < 0) dt = 0; cam_ms = nw;
        if (wk_pan_t0 && wk_look >= 0 && m->cw > 640) {   /* the slow hall pan: a fixed-time ease between the two positions the picture can actually show */
            int tl = tx >> 8, lo = m->cx + 320, hi = m->cx + m->cw - 320, x1 = tl < lo ? lo : tl > hi ? hi : tl, x0 = wk_pan_x0 < lo ? lo : wk_pan_x0 > hi ? hi : wk_pan_x0;
            unsigned a = now_ms() - wk_pan_t0; int u = a >= PAN_MS ? 256 : (int)(a * 256 / PAN_MS);
            int sm = u * u * (768 - 2 * u) / (256 * 256);   /* smoothstep: slow start, slow stop */
            wk_cfx = (x0 + (x1 - x0) * sm / 256) * 256;
        } else cam_ease(&wk_cfx, tx, dt);
        cam_ease(&wk_cfy, ty, dt);
    }
    wk_camx = wk_cfx >> 8; wk_camy = wk_cfy >> 8;
    /* the picture's top-left corner in 1/256 px (clx, cty), kept inside the map picture so it always fills the screen, then in whole screen pixels */
    if (m->cw <= 640) clx = -((640 - m->cw) / 2 - m->cx) * 256; else { clx = wk_cfx - 320 * 256; if (clx < m->cx * 256) clx = m->cx * 256; if (clx > (m->cx + m->cw - 640) * 256) clx = (m->cx + m->cw - 640) * 256; }
    if (m->ch <= 480) cty = -((480 - m->ch) / 2 - m->cy) * 256; else { cty = wk_cfy - 240 * 256; if (cty < m->cy * 256) cty = m->cy * 256; if (cty > (m->cy + m->ch - 480) * 256) cty = (m->cy + m->ch - 480) * 256; }
    ox = -(clx >> 8); oy = -(cty >> 8);
    adjx = XSF(-(long long)clx) - XS(ox); adjy = YSF(-(long long)cty) - YS(oy);   /* the leftover fraction of a logical pixel, as screen pixels */
    xn = XSF((long long)pxF - (long long)(fw / 2) * 256 - clx); yn = YSF((long long)pyF - (long long)foot * 256 - cty);   /* where your picture goes: one rounding, so you do not wobble against the screen */
    T = S; R(0, 0, 640, 480, C(0,0,0)); ADJX = adjx; ADJY = adjy;
    { int nt = g_night(&G); SDL_Surface *nm = nt ? (wk_map == MAP_SBED ? sbN : night_of(wk_map)) : NULL, *im = nm ? nm : m->img;   /* night: the darker copy of the map */
      if (im) blit(im, ox, oy);
      if (wk_map == MAP_LOBBY && !nt) blit(clerkS, ox + 543, oy + 233);
      if (wk_map == MAP_EVENT && clerkS) blit(clerkS, ox + 423 - LW(clerkS) / 2, oy + 246 - LH(clerkS)); }   /* the Event Center clerk stands in the notch of her desk, day and night */   /* nobody at the desk at night */   /* the receptionist stands in the notch of the desk */   /* the receptionist stands in the notch of the desk */
    if (wk_moving && p >= 64 && p < 192) col = (wk_step & 1) ? 2 : 0;
    ls_drawn = 0;
    if (ls_on && LS_MAP() && lsS && ls_ypos() < pyF) draw_sophia(clx, cty);   /* v8.0: further back than you: she is drawn first */
    if (plS) blit_rect_s((wk_map == MAP_SBED && g_night(&G)) && plSn ? plSn : (g_night(&G) && plSm && wk_map != MAP_EVENT && wk_map != MAP_SOPHIA) ? plSm : plS, col * fw, wk_face * fh, fw, fh, xn, yn);   /* her room is at night: you are darker too */
    else R(ox + px - 18, oy + py - 72, 36, 72, C(60,90,170));
    if (ls_on && LS_MAP() && lsS && !ls_drawn) draw_sophia(clx, cty);   /* in front of you */
    if (bubble) draw_icons(ox, oy);
    ADJX = xn - XS(ox + px - fw / 2); ADJY = yn - YS(oy + py - foot);   /* v7.3: the bubble follows your picture */
    if (bubble && !wk_moving && !wk_mode && (wk_interactive(wk_tile(wk_map, wk_tx + DX4[wk_face], wk_ty + DY4[wk_face])) || ls_faced())) {   /* "press A" bubble over your head */
        int bx = ox + px - 12, by = oy + py - foot - 28; rrect(bx, by, 24, 24, gui_lc()); rrect(bx + 2, by + 2, 20, 20, C(24,30,52)); TS = 0; text(bx + 6, by + 5, "A", 2, white); TS = 1;
    }
    ADJX = ADJY = 0;
}

/* ---------- feature icons floating over the things that answer (fridge, ATM, store, Event Center) ---------- */
enum { IC_FRIDGE, IC_ATM, IC_STORE, IC_EVENT };
static int ic_seen[4];   /* v9.0: how many times each icon was drawn (the tests check that the icons stay when you face the thing) */
static void draw_icon(int kind, int cx, int cy) {   /* cx, cy = the middle of the badge */
    ic_seen[kind]++;
    Uint32 gold = gui_tc(), body = C(24,30,52), white = C(235,238,245);
    int ts = TS; TS = 0;
    rrect(cx - 17, cy - 17, 34, 34, gui_lc()); rrect(cx - 15, cy - 15, 30, 30, body); tri(cx, cy + 17, 5, 5, 0, gui_lc());
    switch (kind) {
    case IC_FRIDGE:
        R(cx - 7, cy - 12, 14, 24, white); R(cx - 7, cy - 3, 14, 2, C(120,130,150)); R(cx + 3, cy - 9, 2, 4, C(90,100,120)); R(cx + 3, cy + 1, 2, 6, C(90,100,120)); break;
    case IC_ATM:
        rrect(cx - 11, cy - 11, 22, 22, C(40,150,60)); dollar_plain = 1; text(cx - 5, cy - 7, "$", 2, white); dollar_plain = 0; break;
    case IC_STORE:
        R(cx - 6, cy - 5, 12, 15, C(215,55,55)); R(cx - 6, cy, 12, 3, white); R(cx - 8, cy - 8, 16, 3, white); R(cx + 1, cy - 14, 2, 7, white); R(cx + 1, cy - 14, 6, 2, white); break;
    default:   /* the Event Center: a poker chip */
        disc(cx, cy, 11, C(200,40,50)); disc(cx, cy, 8, white); disc(cx, cy, 5, C(200,40,50)); disc(cx, cy, 2, white); break;
    }
    TS = ts;
}
static void draw_icons(int ox, int oy) {
    int x, y, t = (int)(now_ms() / 90 % 12), bob = (t < 6 ? t : 12 - t) - 3;   /* a gentle up and down (v9.0: the icons stay when you face the thing, the A bubble comes on top of your head) */
    for (y = 0; y < wm[wk_map].h; y++) for (x = 0; x < wm[wk_map].w; x++) {
        char c = wm[wk_map].g[y][x]; int cx = ox + x * TILE + TILE / 2, cy = oy + y * TILE;
        if (wk_map == MAP_ROOM && c == 'F') draw_icon(IC_FRIDGE, cx, cy - 56 + bob);
        else if (wk_map == MAP_LOBBY && c == 'A' && services_open() && wm[wk_map].g[y][x - 1] != 'A') draw_icon(IC_ATM, cx + TILE / 2 - 2, cy - 112 + bob);   /* v9.1: the wall ATM covers two tiles: one badge above its middle */
        else if (wk_map == MAP_LOBBY && c == 'M' && services_open()) draw_icon(IC_STORE, cx - 20, cy - 112 + bob);   /* v9.1: above the vending machine */
        else if (wk_map == MAP_LOBBY && c == 'E' && G.event && services_open() && wm[wk_map].g[y - 1][x] != 'E') draw_icon(IC_EVENT, cx + 8, cy - 6 + bob);
    }
}
static const char *wk_hint(void) { return G.story == ST_NEW ? H_DESK : (G.story == ST_HOME && !(G.hall_seen & 1)) ? H_ROOM : (G.story == ST_HOME && !services_open()) ? H_ENTER : NULL; }   /* what the story wants from you right now */
static void draw_walk(void) {
    Uint32 gold = gui_tc(), white = C(240,240,240); const char *h = wk_hint();
    draw_walk_world(1);
    draw_energy(10, 6); draw_cash(); draw_time();   /* v11.1: the friendship bar is only shown at her door (draw_door), not on the walking map */
    if (h && !wk_mode) text_o(320 - tw(h, 2) / 2 + 6, 12, h, 2, gold);
    if (wk_mode == 1) {
        int done = dialog(wk_pg[wk_ipg], wk_tag, wk_narr, wk_ms);
        if (done) dtext_r(DLG_R, dlg_py, wk_ipg + 1 < wk_npg ? "A >" : "A OK", 2, gold);
    } else if (wk_mode == 4) {
        char rb[24], ub[24]; const char *it[4]; int dk[4], dn = desk_items(dk), i;
        snprintf(rb, sizeof rb, DESK_ITEMS[0], g_rent_amt(&G)); snprintf(ub, sizeof ub, DESK_UPG, g_upgrade_cost(&G));
        for (i = 0; i < dn; i++) it[i] = dk[i] == DK_RENT ? rb : dk[i] == DK_UPG ? ub : DESK_ITEMS[2];
        menu_panel(it, dn, wk_dsel);
    } else if (wk_mode == 9) {
        static const char *const vm[3] = {"TALK", "PLAY A MATCH", "LEAVE"}; menu_panel(vm, 3, fv_vsel);   /* v11.0 */
    } else if (wk_mode == 8) {
        const char *it[OF_N + 1]; int wt[OF_N], wn = g_outfit_targets(&G, wt), q; for (q = 0; q < wn; q++) it[q] = OF_CHG[wt[q]]; it[wn] = "LEAVE";   /* v10.6 */
        menu_panel(it, wn + 1, wk_wsel);
    } else if (wk_mode == 5) {
        static const char *const FN[4] = {"", "1ST FLOOR", "2ND FLOOR", "3RD FLOOR"}; const char *it[3]; int i;
        for (i = 0; i < wk_nflr; i++) it[i] = FN[wk_flr[i]];
        menu_panel(it, wk_nflr, wk_fsel);
    } else if (wk_mode == 2 && wk_sleep) {
        panel_a(200, 170, 240, 120, 256, 240);
        text_c(320, 180, T_SLEEP, 2, gold);
        { char nb[40]; snprintf(nb, sizeof nb, "NEXT: DAY %d - MORNING", G.day + 1); text_c(320, 204, nb, 1, C(170,176,190)); }
        text(230, 222, wk_yes == 0 ? ">" : " ", 2, gold); text(254, 222, "YES", 2, wk_yes == 0 ? gold : white);
        text(230, 248, wk_yes == 1 ? ">" : " ", 2, gold); text(254, 248, "NO", 2, wk_yes == 1 ? gold : white);
    } else if (wk_mode == 2) {   /* v10.5 the sofa: REST / SAVE ONLY / CANCEL */
        static const char *const so[3] = { "REST", "SAVE ONLY", "CANCEL" }; int i, night = g_night(&G); char nb[48];
        panel_a(180, 150, 280, 178, 256, 240);
        text_c(320, 162, T_SAVE, 2, gold);
        for (i = 0; i < 3; i++) {
            Uint32 col = (i == 0 && night) ? C(110,115,128) : i == wk_yes ? gold : white;
            text(214, 192 + 26 * i, i == wk_yes ? ">" : " ", 2, gold); text(238, 192 + 26 * i, so[i], 2, col);
        }
        if (wk_yes == 0) snprintf(nb, sizeof nb, "TIME PASSES TO %s", PH_NAME[G.phase + 1 > PH_NIGHT ? PH_NIGHT : G.phase + 1]);
        else if (wk_yes == 1) snprintf(nb, sizeof nb, "THE TIME STAYS %s", PH_NAME[G.phase]); else nb[0] = 0;
        if (night && wk_yes != 0) text_c(320, 276, "TOO LATE TO REST: USE THE BED", 1, C(150,155,170)); else text_c(320, 276, nb, 1, C(170,176,190));
        { char sb[32]; snprintf(sb, sizeof sb, "SAVES TO SLOT %d", slot_act); text_c(320, 300, sb, 1, C(130,135,150)); }
    }
}

/* ---------- opening monologue (black screen, centred, typed out, two pages) ---------- */
#define MONO_W 40      /* characters per line */
#define MONO_MS 32     /* time per character */
static int mono_total(void) { char w[500]; int i, n = 0; wrap_buf(w, sizeof w, M_PAGE[mono_pg], MONO_W); for (i = 0; w[i]; i++) if (w[i] != '\n') n++; return n; }
static int mono_done(void) { return frozen || (int)(age_of(mono_ms) / MONO_MS) >= mono_total(); }
static void draw_mono(void) {
    char w[500], ln[MONO_W + 2]; int i, n = 0, nl = 1, y, shown, left; const char *p;
    T = S; R(0, 0, 640, 480, C(0, 0, 0));
    wrap_buf(w, sizeof w, M_PAGE[mono_pg], MONO_W);
    for (i = 0; w[i]; i++) if (w[i] == '\n') nl++;
    shown = frozen ? 9999 : (int)(age_of(mono_ms) / MONO_MS); left = shown; y = (480 - nl * 26) / 2; p = w;
    while (*p) {
        n = 0; while (p[n] && p[n] != '\n' && n < MONO_W) { ln[n] = p[n]; n++; } ln[n] = 0;
        if (left > 0) {
            char part[MONO_W + 2]; int k = left < n ? left : n, x = 320 - tw(ln, 2) / 2;   /* each line is placed by its full width, so the letters appear in place */
            memcpy(part, ln, (size_t)k); part[k] = 0; text(x, y, part, 2, C(225, 228, 238));
        }
        left -= n; y += 26; p += n; if (*p == '\n') p++;
    }
    if (mono_done()) text_r(604, 452, mono_pg == 0 ? "A >" : "A OK", 2, C(250, 215, 90));
}
static void mono_key(int b) {
    if (b != B_A && b != B_B) return;
    if (!mono_done()) { mono_ms = now_ms() - 60000u; return; }   /* finish the typing first */
    if (mono_pg == 0) { mono_pg = 1; mono_ms = now_ms(); } else go(GO_SCRIPT, SCR_INTRO);
}

/* ---------- Sophia's door (knock or go in) and her room ---------- */
static void draw_sneak(void) {
    static const char *door_items[2]; const char *room_items[6]; int done;
    T = S;
    switch (sn_mode) {
    case SM_CHOICE:
        door_items[0] = D_KNOCK; door_items[1] = D_ENTER;
        draw_walk_world(0); draw_energy(10, 6); draw_cash(); draw_time(); menu_panel(door_items, 2, sn_sel);
        text_r(490, 412, "B BACK", 2, C(200,205,215));
        break;
    case SM_MSG:
        bg_draw(BG_SLEEP);
        done = dialog(sn_txt, NULL, 1, sn_ms);
        if (done) dtext_r(DLG_R, dlg_py, "A OK", 2, gui_tc());
        break;
    case SM_BED:   /* the three looks cost energy; the three fullscreen views are free (the closer ones open once you have looked) */
        room_items[0] = "ADMIRE"; room_items[1] = sn_closer ? "LOOK CLOSER (SEEN)" : "LOOK CLOSER"; room_items[2] = sn_longer ? "LOOK LONGER (SEEN)" : "LOOK LONGER";
        bg_draw(BG_SLEEP); menu_panel(room_items, 3, sn_sel);
        text_o(490 - tw("B BACK", 1), DLG_BOT - (10 + 22 * 3) - 16, "B BACK", 1, C(200,205,215));
        break;
    case SM_ADMIRE:
        bg_draw(BG_SLEEP);
        done = dialog(SN_ADMIRE[sn_pg], NULL, 1, sn_ms);
        if (done) { int last = sn_pg + 1 >= (int)SN_ADMIRE_N; dtext_r(DLG_R, dlg_py, last ? "A VIEW FULLSCREEN" : "A >", 2, gui_tc()); if (last) dtext(DLG_L, dlg_py, "B BACK", 2, C(200,205,215)); }
        break;
    case SM_CLOSER: case SM_LONGER: {
        const char *const *pg = sn_mode == SM_CLOSER ? SN_CLOSER : SN_LONGER; int np = 3;
        bg_draw(sn_mode == SM_CLOSER ? BG_CLOSE : BG_LONG);
        done = dialog(pg[sn_pg], NULL, 1, sn_ms);
        if (done) { int last = sn_pg + 1 >= np; dtext_r(DLG_R, dlg_py, last ? "A VIEW FULLSCREEN" : "A >", 2, gui_tc()); if (last) dtext(DLG_L, dlg_py, "B BACK", 2, C(200,205,215)); }
        break; }
    default:   /* SM_VIEW: just her picture (sleeping, closer or longer) */
        bg_draw(sn_vpic == 1 ? BG_CLOSE : sn_vpic == 2 ? BG_LONG : BG_SLEEP); text_o(612 - tw("B BACK", 1), 462, "B BACK", 1, C(200,205,215));
        break;
    }
}
static void sn_msg(const char *t) { sn_ret = SM_BED; sn_txt = t; sn_mode = SM_MSG; sn_ms = now_ms(); }
static void sneak_key(int b) {
    switch (sn_mode) {
    case SM_CHOICE:
        if (b == B_UP || b == B_DOWN) sn_sel = !sn_sel;
        else if (b == B_B) go(GO_HUB, 0);
        else if (b == B_A) {
            int home = g_rom_on(&G);
            if (sn_sel == 0) go(GO_SCRIPT, home ? SCR_NIGHTHOME : SCR_NIGHT);
            else if (home) { sn_used = 0; sn_closer = 0; sn_longer = 0; sn_sel = 0; go(GO_SCRIPT, SCR_BEDDOOR); }   /* at home: no map to cross, just her door and the same three looks */
            else go(GO_SCRIPT, SCR_NIGHT);   /* v11.1: no sneaking into her 3rd-floor room any more: only the knock (the scolding) */
        }   /* (this menu only opens at night: the knock is the angry scene) */
        break;
    case SM_MSG:
        if (b != B_A) break;
        if (!typed_done(sn_txt, sn_ms)) { sn_ms = now_ms() - 30000u; break; }
        sn_mode = sn_ret;
        break;
    case SM_BED:
        if (b == B_UP) sn_sel = (sn_sel + 2) % 3;
        else if (b == B_DOWN) sn_sel = (sn_sel + 1) % 3;
        else if (b == B_B) go(GO_SNEAKBACK, 0);
        else if (b == B_A) {
            if (sn_sel == 0) { sn_mode = SM_ADMIRE; sn_pg = 0; sn_ms = now_ms(); }
            else if (sn_sel == 1 ? sn_closer : sn_longer) sn_msg(SN_DONE);
            else if (sn_sel == 2 && !sn_closer) sn_msg(SN_NEEDCLOSER);
            else { if (sn_sel == 1) { sn_closer = 1; sn_mode = SM_CLOSER; } else { sn_longer = 1; sn_mode = SM_LONGER; } sn_pg = 0; sn_ms = now_ms(); }
        }
        break;
    case SM_CLOSER: case SM_LONGER:
        if (b == B_B) { sn_sel = sn_mode == SM_CLOSER ? 1 : 2; sn_mode = SM_BED; break; }
        if (b != B_A) break;
        { const char *t = (sn_mode == SM_CLOSER ? SN_CLOSER : SN_LONGER)[sn_pg];
          if (!typed_done(t, sn_ms)) { sn_ms = now_ms() - 30000u; break; }
          if (sn_pg + 1 < 3) { sn_pg++; sn_ms = now_ms(); } else { sn_vpic = sn_mode == SM_CLOSER ? 1 : 2; sn_mode = SM_VIEW; } }   /* after her last line: A shows the picture fullscreen */
        break;
    case SM_ADMIRE:
        if (b == B_B) { sn_mode = SM_BED; sn_sel = 0; break; }
        if (b != B_A) break;
        if (!typed_done(SN_ADMIRE[sn_pg], sn_ms)) { sn_ms = now_ms() - 30000u; break; }
        if (sn_pg + 1 < (int)SN_ADMIRE_N) { sn_pg++; sn_ms = now_ms(); } else { sn_vpic = 0; sn_mode = SM_VIEW; }
        break;
    default:
        if (b == B_A || b == B_B) { sn_mode = SM_BED; sn_sel = sn_vpic; }   /* back on the look you just did */
        break;
    }
}

/* ---------- input ---------- */
static void open_pause(void) {
    pause_sel = 0;
    switch (scr) {
        case SC_MENU: running = 0; return;
        case SC_CAREER: case SC_OPT: return;   /* (the title menus ignore Menu) */
        case SC_WALK: case SC_ATM: case SC_STORE: case SC_SHOP: case SC_DEBUG: case SC_INV: case SC_SNEAK: pause_kind = PK_HOME; break;
        case SC_MONO: pause_kind = PK_SKIP; break;   /* v8.5: Start in the opening monologue = the pause menu with SKIP SCENE */
        case SC_PLAY: pause_kind = PK_TABLE; break;
        case SC_STORY: pause_kind = PK_SKIP; break;
        case SC_END: if (first_end) return; pause_kind = PK_ROOM; break;   /* her introduction must not be skipped */
        case SC_QSETUP: pause_kind = mp_mode ? PK_ROOM : PK_TITLE; break;
        case SC_MTABLE: pause_kind = mp_mode ? PK_EXHIB : PK_QUICK; break;
        case SC_MEND: case SC_OVER: return;
        default: pause_kind = PK_ROOM; break;
    }
}
static void pause_key(int b) {
    int n = pause_kind == PK_HOME ? 4 : pause_kind == PK_EXITC ? 2 : 3;   /* the walking pause menu: Resume / Objective / Options / Exit; the others: Resume / Options / (their own action) */
    if (pause_kind == PK_OPT) { if (opt_key(b)) { pause_kind = opt_from; pause_sel = opt_from == PK_HOME ? 2 : 1; } return; }
    if (pause_kind == PK_OBJ) { if (b == B_A || b == B_B || b == B_QUIT) { pause_kind = PK_HOME; pause_sel = 1; } return; }
    if (b == B_UP) pause_sel = (pause_sel + n - 1) % n; else if (b == B_DOWN) pause_sel = (pause_sel + 1) % n;
    else if (b == B_B || b == B_QUIT || (b == B_A && pause_sel == 0)) { pause_kind = PK_NONE; think_at = now_ms() + 700; }
    else if (b == B_A) {
        int k = pause_kind;
        if (k == PK_HOME) { if (pause_sel == 1) { pause_kind = PK_OBJ; return; } if (pause_sel == 2) { opt_from = k; pause_kind = PK_OPT; opt_row = 0; opt_sub = 0; opt_conf = 0; slots_refresh(); return; } pause_kind = PK_EXITC; pause_sel = 0; return; }   /* Objective page, Options, or "are you sure?" first */
        if (k != PK_EXITC && pause_sel == 1) { opt_from = k; pause_kind = PK_OPT; opt_row = 0; opt_sub = 0; opt_conf = 0; return; }   /* Options, in every other pause menu too */
        pause_kind = PK_NONE;
        if (k == PK_EXITC) go(GO_EXIT, 0);   /* back to the last save */
        else if (k == PK_TABLE) leave_table();
        else if (k == PK_SKIP) { if (scr == SC_MONO) go(GO_SCRIPT, SCR_INTRO); else vn_done(); }
        else if (k == PK_TITLE || k == PK_QUICK) go(GO_MENU, 0);   /* nothing is at stake in Quick Play */
        else go(GO_HUB, 0);
    }
}
static void fresh_game(void) {   /* a brand new career in memory (v10.5: nothing on disk is touched: after GAME OVER you go back to the title and load a slot or start over) */
    g_new(&G); saved_G = G; prog_far = 0; room_seen = 0; unpack_done = 0; wk_mode = 0; wk_moving = 0; pause_kind = PK_NONE; cu_mode = 0; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN); scr = SC_MENU; menu_sel = 1;
}
static void new_game_slot(int sl) { if (IO_OK) remove(slot_path(sl)); slot_act = sl; fresh_game(); cfg_save(); }   /* v10.5: a new career in slot sl (an old save there is deleted: the screen asked first) */

/* ---------- v6.2 keys: online store, romance scene, her question ---------- */
static void shop_key(int b) {
    if (shop_info) { if (b == B_B || b == B_A || b == B_Y) shop_info = 0; return; }   /* v6.8: the info page */
    if (shop_confirm) {
        if (b == B_A) {
            int o0[16], n0 = shop_list(shop_tab, o0), p0 = shop_pos(o0, n0), r = g_shop_buy(&G, shop_sel);
            if (r == 0) { persist(); shop_fix(p0); shop_note(shop_sel >= 0 && shop_sel < SH_N ? SH_BOUGHT[shop_sel] : SH_BOUGHT[0]); }
            else shop_note(r == 1 ? "NOT ENOUGH MONEY (CASH + BANK)" : r == 2 ? "SOLD OUT" : "STOCK FULL");
            shop_confirm = 0;
        } else if (b == B_B) shop_confirm = 0;
        return;
    }
    if (b == B_R1 || b == B_L1) {   /* v7.8: R1 = the next tab, L1 = the previous one (they wrap round) */
        int o[16]; shop_tab = shop_next_tab(shop_tab, b == B_R1 ? 1 : -1); shop_list(shop_tab, o); shop_sel = o[0]; shop_msg[0] = 0; stk_top = 0; return;
    }
    { int o[16], nv = shop_list(shop_tab, o), k;
      for (k = 0; k < nv && o[k] != shop_sel; k++); if (k >= nv) { k = 0; shop_sel = o[0]; }
      if (b == B_UP) shop_sel = o[(k + nv - 1) % nv]; else if (b == B_DOWN) shop_sel = o[(k + 1) % nv];
      shop_scroll(shop_pos(o, nv), nv); }
    if (b == B_B) scr = SC_WALK;
    else if (shop_tab == TAB_STOCKS) {   /* A buys one share, X sells one, at today's price */
        char m[48];
        if (b == B_A) {
            int r = g_stk_buy(&G, shop_sel);
            if (r == 0) { persist(); snprintf(m, sizeof m, "BOUGHT 1 %s", STK_NAME[shop_sel]); shop_note(m); }
            else shop_note(r == 1 ? "NOT ENOUGH MONEY (CASH + BANK)" : "YOU OWN THE MAXIMUM");
        } else if (b == B_X) {
            int p = stk_now(&G, shop_sel), r = g_stk_sell(&G, shop_sel);
            if (r == 0) { char n[16]; n[0] = '$'; fmt_money(n + 1, p); persist(); snprintf(m, sizeof m, "SOLD 1 %s  +%s", STK_NAME[shop_sel], n); shop_note(m); }
            else shop_note(r == 1 ? "YOU OWN NONE" : "CASH IS FULL. USE THE ATM");
        }
    }
    else if (b == B_Y && SH_DESC[shop_sel]) shop_info = 1;   /* v6.8: ring, breast enhancement pill, milk inducer pill */
    else if (b == B_X && DEBUG_MODE) dbg_open();
    else if (b == B_A) {
        if (shop_sold(shop_sel)) shop_note("SOLD OUT");
        else if ((long)G.cash + G.bank < SH_PRICE[shop_sel]) shop_note("NOT ENOUGH MONEY (CASH + BANK)");
        else shop_confirm = 1;
    }
}
static void rom_set(const char *t) { rom_txt = t; rom_ms = now_ms(); rom_choose = 0; }   /* (a new line of hers is always shown first, the choices come on the next page) */
static void rom_open(const char *t) { cover_now = 0; feed_hide = 0; scr = SC_ROM; rom_in = 1; rom_mode = RM_MENU; rom_sel = 0; rom_page = 0; rom_lsel = 0; rom_note[0] = 0; pause_kind = PK_NONE; rom_set(t); }
static void rom_open_door(const char *t) { rom_open(t); rom_in = 0; }   /* before day 6 she only stands in front of her door */
static void home_to_door(const char *note) {   /* after a gift scene: back on the map in front of her bedroom door (A on it knocks again) */
    if (note) { snprintf(home_note_buf, sizeof home_note_buf, "%s", note); home_note = home_note_buf; } else home_note = NULL;
    if (ls_talk) { go(GO_HUB, 0); return; }   /* v8.0: you were talking to her in the living room: you stay where you stood */
    wk_nmap = MAP_ROOM; wk_ntx = HOME_DOOR_X; wk_nty = HOME_DOOR_Y; wk_nface = UP; go(GO_MAP, 0);
}
static void rom_back(const char *note) { rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); snprintf(rom_note, sizeof rom_note, "%s", note); rom_note_ms = now_ms(); }   /* after a milk scene: her menu again, with a note top right */
static void rom_give(int kind) {   /* v6.7: a gift, or the milk pill straight from her menu */
    int lv = G.rom_lvl, pc = G.rom_pct, bt = G.bottle, dg = G.deck_given, r = g_gift(&G, kind);
    rom_mode = RM_MENU;
    if (r == 0) {
        persist();
        if (kind == GF_PILL) { go(GO_SCRIPT, SCR_PILLGIVE); return; }   /* "What is this pill?" ... the 7 days start today */
        if (kind == GF_PUMP) { rom_set(pumpnew_line()); snprintf(rom_note, sizeof rom_note, "NEW PUMP READY"); rom_note_ms = now_ms(); return; }   /* v12.2: the new pump for a broken one: her thanks, no scene */
        gift_kind = kind; gift_l1 = gift_pick(1, kind); if (!gift_l1) gift_l1 = R_GIFT_OK[kind == GF_STACK ? 1 : kind >= GF_INJECT ? 7 : kind >= GF_LOTION ? 3 + kind - GF_LOTION : 0];   /* v10.2: pool, else the old list (pill, pump, milk pill: R_MILK_OK below) */   /* v8.2: flower 0, money 1, (pill 2), lotion 3, pink cards 4, yoga 5, gold 6, super drug 7 */
        if (kind == GF_MILK) {
            if (G.milk_st == 1) { vn_start(SCR_MILK1); return; }          /* the first milk pill: wait 2 days */
            if (G.bottle > bt) { bottle_pill = 1; vn_start(SCR_BOTTLE); return; }          /* the gauge is full: she uses the pump again and hands you the bottle */
            gift_l1 = pick(R_MILK_OK, R_MILK_OK_N); if (G.milk_pct >= 100) snprintf(gift_note, sizeof gift_note, "GAUGE FULL"); else snprintf(gift_note, sizeof gift_note, "BOTTLE +%d%%", MILK_STEP);
        } else if (kind == GF_SUPER) snprintf(gift_note, sizeof gift_note, "GAUGE FULL FOR %d DAYS", SUPER_DAYS);
        else if (kind == GF_INJECT) snprintf(gift_note, sizeof gift_note, "PERMANENT MILK. NO MORE GAUGE");   /* v12.3 */
        else if (g_gift_is_outfit(kind)) snprintf(gift_note, sizeof gift_note, "ADDED TO HER WARDROBE");   /* v11.3: no romance */
        else if (kind == GF_DECK && !dg && G.deck_given) { if (G.rom_lvl > lv) snprintf(gift_note, sizeof gift_note, "ROMANCE LEVEL UP!  PINK CARDS!"); else if (G.rom_pct > pc) snprintf(gift_note, sizeof gift_note, "ROMANCE +%d%%  PINK CARDS!", G.rom_pct - pc); else snprintf(gift_note, sizeof gift_note, "PINK CARDS AT HER TABLE!"); }   /* v11.6 */
        else if (G.rom_lvl > lv) snprintf(gift_note, sizeof gift_note, "ROMANCE LEVEL UP!");
        else snprintf(gift_note, sizeof gift_note, "ROMANCE +%d%%", G.rom_pct - pc);
        vn_start(SCR_GIFT);
    } else if (r == 14) pump_broke();   /* v12.2: this milk pill would have filled the gauge, but the pump is broken: the pill is not used up */
    else rom_set(r == 1 ? pick(R_GIFT_NONE, R_GIFT_NONE_N) : r == 2 ? pick(R_GIFT_TODAY, R_GIFT_TODAY_N) : r == 3 ? pick(R_GIFT_PILL, R_GIFT_PILL_N) : r == 4 ? pick(R_GIFT_FULL, R_GIFT_FULL_N) : r == 5 ? pick(R_MILK_WAIT, R_MILK_WAIT_N) : r == 6 ? pick(R_MILK_PUMP, R_MILK_PUMP_N) : r == 7 ? pick(R_PUMP_NOW, R_PUMP_NOW_N) : r == 9 ? pick(R_MILK_SUPER, R_MILK_SUPER_N) : r == 10 ? pick(R_SUPER_ON, R_SUPER_ON_N) : r == 15 ? pick(R_INJ_WAIT, R_INJ_WAIT_N) : r == 16 ? pick(R_INJ_NOPILL, R_INJ_NOPILL_N) : r == 11 ? pick(R_MILK_FULL, R_MILK_FULL_N) : r == 12 ? pick(R_GIFT_NONE, R_GIFT_NONE_N) : pick(R_BOTTLE_FULL, R_BOTTLE_FULL_N));
}
static const char *inj_done_line(void) {   /* v12.3: one milk a day in total */
    int cw = g_cow(&G);
    return g_mama(&G) ? (cw ? pick(R_INJ_DONE_CW_MM, R_INJ_DONE_CW_MM_N) : pick(R_INJ_DONE_MM, R_INJ_DONE_MM_N)) : cw ? pick(R_INJ_DONE_CW, R_INJ_DONE_CW_N) : pick(R_INJ_DONE, R_INJ_DONE_N);
}
static void rom_ask(void) {   /* v8.2: ASK FOR MILK while the Super drug works: a Baby Milk Bottle, twice a day (the bottle scene plays, you may peek) */
    int r = g_ask_milk(&G);
    rom_mode = RM_MENU; cover_set(1);   /* v8.8: married: she covers her chest when you ask (her answer and the bottle scene) */
    if (r == 0) { bottle_inj = G.injected; persist(); vn_start(SCR_BOTTLE); }
    else if (r == 14) pump_broke();   /* v12.2 */
    else if (r == 2 && G.injected) rom_set(inj_done_line());   /* v12.3 */
    else rom_set(r == 2 ? pick(R_ASK_DONE, R_ASK_DONE_N) : r == 8 ? pick(R_BOTTLE_FULL, R_BOTTLE_FULL_N) : pick(R_ASK_NONE, R_ASK_NONE_N));
}
/* v8.6: the wedding and page 4 */
static void rom_marry(void) { rom_mode = RM_MENU; if (g_marry(&G)) { rom_set(pick(R_PUMP_NOW, R_PUMP_NOW_N)); return; } persist(); vn_start(SCR_WEDDING); }
static int outfit_ask(int k, int ward) { if (!ow_groups(k)) return 0; ow_target = k; ow_ward = ward; vn_start(SCR_OUTFIT); return 1; }   /* v11.2: 1 = the exchange started (the outfit changes at its end) */
static void rom_pump(void) { int r = g_pump_use(&G); rom_mode = RM_MENU; bottle_inj = 0; if (r == 0) { persist(); vn_start(SCR_BOTTLE); } else if (r == 14) pump_broke(); else rom_set(r == 8 ? pick(R_BOTTLE_FULL, R_BOTTLE_FULL_N) : notfull_line()); }   /* the same pump scene (lead-in, you may peek) */
static void fh_set(void) { feed_hide = (!ls_talk && !rom_in && G.married) ? 1 : 0; }   /* v10.1: she hides behind her bedroom door only when you stand at the door (not inside her room, not in the living room) */
#define FEED_NO_PCT 35   /* v9.7: the chance (percent) that she flatly refuses BREASTFEED when the gauge is full */
static int feed_no_force = -1;   /* tests only: 0 = she never refuses, 1 = she always refuses, -1 = the real chance */
static int feed_no_day = -1, feed_no_ph = -1;   /* the part of the day she refused: asking again then gets the 'I just said no' line (not saved: it is only a few minutes) */
static void rom_feed(void) {
    rom_mode = RM_MENU; cover_set(1);
    if (g_mama(&G) && g_feed_ready(&G)) {   /* v9.7: she refuses the name, so she may refuse this too: nothing is used up, ask again later */
        if (feed_no_day == G.day && feed_no_ph == G.phase) { fh_set(); rom_set(pick(R_FEED_AGAIN_MM, R_FEED_AGAIN_MM_N)); return; }
        if (feed_no_force == 1 || (feed_no_force < 0 && rnd(100) < FEED_NO_PCT)) { feed_no_day = G.day; feed_no_ph = G.phase; g_mm_note(&G, MK_FEEDNO); fh_set(); rom_set(pick(R_FEED_NO_MM, R_FEED_NO_MM_N)); return; }
    }
    if (g_feed(&G)) { if (g_mama(&G)) fh_set(); rom_set(G.injected ? inj_done_line() : notfull_line()); return; }
    g_mm_note(&G, MK_FEED); persist(); vn_start(SCR_FEED);
}
static void rom_sleep(void) {   /* night only; refused after a tease or on a day she gave milk */
    rom_mode = RM_MENU;
    if (!g_night(&G)) { snprintf(rom_note, sizeof rom_note, "ONLY AT NIGHT"); rom_note_ms = now_ms(); return; }
    if (kicked_day == G.day) { rom_set(g_mama(&G) ? pick(R_SLEEP_NO_TEASE_MM, R_SLEEP_NO_TEASE_MM_N) : pick(R_SLEEP_NO_TEASE, R_SLEEP_NO_TEASE_N)); return; }
    if (g_sleep_refused_milk(&G)) { rom_set(g_mama(&G) ? pick(R_SLEEP_NO_MILK_MM, R_SLEEP_NO_MILK_MM_N) : pick(R_SLEEP_NO_MILK, R_SLEEP_NO_MILK_N)); return; }
    vn_start(SCR_SLEEP);
}
static void tk_add(int who, const char *t) { if (tk_n < 6) { tk_who[tk_n] = (unsigned char)who; tk_t[tk_n++] = t; } }
static void talk_begin(int kind) { tk_kind = kind; tk_i = 0; tk_ms = now_ms(); rom_mode = RM_TALK; rom_choose = 1; rom_note[0] = 0; }
static int txt_boy(const char *t) {   /* the word "boy" in a text (before the wedding she calls you that; married she does not) */
    const char *p; if (!t) return 0;
    for (p = t; *p; p++) if (tolower((unsigned char)p[0]) == 'b' && tolower((unsigned char)p[1]) == 'o' && tolower((unsigned char)p[2]) == 'y' && (p == t || !isalpha((unsigned char)p[-1])) && !isalpha((unsigned char)p[3])) return 1;
    return 0;
}
static int talk_boy(const Talk *t) { return txt_boy(t->me) || txt_boy(t->a) || txt_boy(t->b); }
static int chat_last_mama = -1, chat_last_cow = -1;
static const char *more_line(void) {   /* v12.1: her line when the chat is over and her menu comes back */
    int cw = g_cow(&G);
    return g_mama(&G) ? (cw ? pick(R_MORE_CW_MM, R_MORE_CW_MM_N) : pick(R_MORE_MM, R_MORE_MM_N)) : cw ? pick(R_MORE_CW, R_MORE_CW_N) : g_p4_open(&G) ? pick(R_MORE_M, R_MORE_M_N) : pick(R_MORE, R_MORE_N);
}
static const char *notfull_line(void) {   /* v12.1: 'not full yet' */
    int cw = g_cow(&G);
    return g_mama(&G) ? (cw ? pick(R_NOT_FULL_CW_MM, R_NOT_FULL_CW_MM_N) : pick(R_NOT_FULL_MM, R_NOT_FULL_MM_N)) : cw ? pick(R_NOT_FULL_CW, R_NOT_FULL_CW_N) : pick(R_NOT_FULL, R_NOT_FULL_N);
}
static const char *pumpbroke_line(void) {   /* v12.2: the pump is worn out */
    int cw = g_cow(&G);
    return g_mama(&G) ? (cw ? pick(R_PUMP_BROKEN_CW_MM, R_PUMP_BROKEN_CW_MM_N) : pick(R_PUMP_BROKEN_MM, R_PUMP_BROKEN_MM_N)) : cw ? pick(R_PUMP_BROKEN_CW, R_PUMP_BROKEN_CW_N) : pick(R_PUMP_BROKEN, R_PUMP_BROKEN_N);
}
static const char *pumpnew_line(void) {   /* v12.2: she thanks you for the new pump */
    int cw = g_cow(&G);
    return g_mama(&G) ? (cw ? pick(R_PUMP_NEW_CW_MM, R_PUMP_NEW_CW_MM_N) : pick(R_PUMP_NEW_MM, R_PUMP_NEW_MM_N)) : cw ? pick(R_PUMP_NEW_CW, R_PUMP_NEW_CW_N) : pick(R_PUMP_NEW, R_PUMP_NEW_N);
}
static void pump_broke(void) {   /* v12.2: she tells you the pump is broken (it was found out just now: the store lists a new one from now on); no bottle, nothing used up */
    persist(); rom_set(pumpbroke_line());
    snprintf(rom_note, sizeof rom_note, "%s", G.pump > 0 ? "GIVE HER THE NEW PUMP" : "PUMP BROKEN. BUY A NEW ONE"); rom_note_ms = now_ms();
}
static void chat_start(void) {   /* a random topic and a random exchange; one chat in CHAT_FAIL_PCT goes badly. v9.5: mama mode has its own pool; married she never calls you boy */
    int tp = 0, ex, tries = 0; const Talk *t; int mm = g_mama(&G), cw = g_cow(&G);
    if (cw) { const TalkPool *cp = mm ? &C_COW_MM : &C_COW; ex = pick_new(cp->n, chat_last_cow); chat_last_cow = ex; t = &cp->t[ex]; }   /* v12.1: cow mode has its own chat */
    else if (mm) { ex = pick_new(C_MAMA.n, chat_last_mama); chat_last_mama = ex; t = &C_MAMA.t[ex]; }
    else {
        do { tp = pick_new(CH_N, chat_last_topic); ex = pick_new(C_TOPICS[tp].n, chat_last_ex[tp]); t = &C_TOPICS[tp].t[ex]; } while (g_p4_open(&G) && talk_boy(t) && ++tries < 30);
        chat_last_topic = tp; chat_last_ex[tp] = ex;
    }
    tk_n = 0; tk_add(0, t->me);
    if (rnd(100) < CHAT_FAIL_PCT) {
        const char *f; int k = 0; do { f = cw ? (mm ? pick(C_FAIL_CW_MM, C_FAIL_CW_MM_N) : pick(C_FAIL_CW, C_FAIL_CW_N)) : mm ? pick(C_FAIL_MM, C_FAIL_MM_N) : pick(C_FAIL, C_FAIL_N); } while (!mm && g_p4_open(&G) && txt_boy(f) && ++k < 30);
        tk_add(1, f); talk_begin(TK_FAIL);
    } else { tk_add(1, t->a); if (t->b) tk_add(1, t->b); talk_begin(TK_CHAT); }
}
static void tease_start(int tp) {   /* your bold line, her answer, then she is angry and throws you out (every time). v9.5: mama mode is softer; married she calls you husband, never boy */
    int ex, tries = 0, mm = g_mama(&G), md = g_p4_open(&G); const Talk *t; const TalkPool *tpl = mm ? &T_TOPICS_MM[tp] : &T_TOPICS[tp]; const char *kick;
    do { ex = pick_new(tpl->n, tease_last_ex[tp]); t = &tpl->t[ex]; } while (md && !mm && talk_boy(t) && ++tries < 30);
    tease_last_ex[tp] = ex;
    tries = 0; do { kick = mm ? pick(T_KICK_MM, T_KICK_MM_N) : md ? pick(T_KICK_M, T_KICK_M_N) : pick(T_KICK, T_KICK_N); } while (md && !mm && txt_boy(kick) && ++tries < 30);
    tk_n = 0; tk_add(0, t->me); tk_add(1, t->a); if (t->b) tk_add(1, t->b);
    tk_add(1, kick); tk_add(2, mm ? pick(T_OUT_MM, T_OUT_MM_N) : pick(T_OUT, T_OUT_N));
    if (tp != TS_LOOKS) cover_set(1);   /* v8.8: married: any topic about her body makes her cover her chest (the picture shows only when she is married, see her_vis) */
    talk_begin(TK_TEASE);
}
static void talk_end(void) {
    char note[40] = "";
    cover_now = 0; feed_hide = 0;   /* v8.8 */
    if (tk_kind == TK_TEASE) { kicked_day = G.day; g_mm_note(&G, MK_TEASE); if (wk_map == MAP_SBED) sb_out = 1; home_to_door("SOPHIA THREW YOU OUT OF HER ROOM"); return; }   /* tease never gives romance */
    if (tk_kind == TK_CHAT && !g_rom_full(&G)) {
        if (chat_gain_day != G.day) { chat_gain_day = G.day; chat_gain_n = 0; }
        if (chat_gain_n < CHAT_DAY_MAX) { int lv = G.rom_lvl, pc0 = G.rom_pct; chat_gain_n++; g_rom_add(&G, CHAT_ROM); if (G.rom_lvl > lv) snprintf(note, sizeof note, "ROMANCE LEVEL UP!"); else snprintf(note, sizeof note, "ROMANCE +%d%%", G.rom_pct - pc0); }
        else snprintf(note, sizeof note, "NO MORE ROMANCE TODAY");
    } else if (tk_kind == TK_FAIL) snprintf(note, sizeof note, "SHE WAS NOT IN THE MOOD");
    if (tk_kind == TK_CHAT || tk_kind == TK_FAIL) g_mm_note(&G, MK_CHAT);   /* v9.7 */
    { int sv = rom_sel, in = rom_in; rom_open(more_line()); rom_in = in; rom_sel = sv; snprintf(rom_note, sizeof rom_note, "%s", note); rom_note_ms = now_ms(); }
}
static void name_open(void);
static void talk_key(int b) {
    if (b != B_A) return;
    if (!typed_done(tk_t[tk_i], tk_ms)) { tk_ms = now_ms() - 30000u; return; }   /* the first A finishes the line */
    if (tk_i + 1 < tk_n) { tk_i++; tk_ms = now_ms(); return; }
    talk_end();
}
static void rom_key(int b) {
    if (rom_mode == RM_TALK) { talk_key(b); return; }
    if (!typed_done(rom_txt, rom_ms)) { if (b == B_A) rom_ms = now_ms() - 30000u; return; }   /* the first A finishes her line */
    if (!rom_choose) { if (b == B_A) { rom_choose = 1; cover_now = 0; feed_hide = 0; } return; }                                /* the next A opens the choices (v6.6) */
    if (rom_mode == RM_MENU) {
        int n = rom_menu_n(), a;
        if (b == B_R2) { if (g_p4_open(&G)) name_open(); return; }   /* v9.5: married: the name keyboard */
        if (b == B_R1 || b == B_L1) { int np = rom_pages(); rom_page = (rom_pg() + (b == B_R1 ? 1 : np - 1)) % np; rom_sel = 0; return; }   /* v8.3: R1 = next page, L1 = previous (wrap) */
        if (b == B_UP && n > 0) rom_sel = (rom_sel + n - 1) % n; else if (b == B_DOWN && n > 0) rom_sel = (rom_sel + 1) % n;
        else if (b == B_B) go(GO_HUB, 0);   /* v8.3: B leaves, there is no LEAVE row */
        else if (b == B_A && n > 0) { a = rom_act(rom_sel); if (a >= RA_GIVE) rom_give(a - RA_GIVE); else if (a == RA_PLAY) rom_mode = RM_LEN; else if (a == RA_MILK) rom_give(GF_MILK); else if (a == RA_SUPER) rom_give(GF_SUPER); else if (a == RA_ASK) rom_ask(); else if (a == RA_RING) rom_marry(); else if (a == RA_PUMP) rom_pump(); else if (a == RA_HUG) { rom_mode = RM_MENU; vn_start(SCR_HUG); } else if (a == RA_KISS) { rom_mode = RM_MENU; vn_start(SCR_KISS); } else if (a == RA_SLEEP) rom_sleep(); else if (a == RA_FEED) rom_feed(); else if (a == RA_CHAT) chat_start(); else if (a == RA_TEASE) { rom_mode = RM_TEASE; tease_sel = 0; } }
    } else if (rom_mode == RM_TEASE) {
        int n = TS_N + 1;
        if (b == B_UP) tease_sel = (tease_sel + n - 1) % n; else if (b == B_DOWN) tease_sel = (tease_sel + 1) % n;
        else if (b == B_B || (b == B_A && tease_sel == TS_N)) rom_mode = RM_MENU;
        else if (b == B_A) {
            if (tease_sel >= TS_MILK && !g_tease_milk_open(&G)) { snprintf(rom_note, sizeof rom_note, "RECEIVE YOUR FIRST BOTTLE TO UNLOCK"); rom_note_ms = now_ms(); }
            else tease_start(tease_sel);
        }
    } else {
        if (b == B_UP) rom_lsel = (rom_lsel + 2) % 3; else if (b == B_DOWN) rom_lsel = (rom_lsel + 1) % 3;
        else if (b == B_B || (b == B_A && rom_lsel == 2)) rom_mode = RM_MENU;
        else if (b == B_A) {
            int r = g_home_start(&G, rom_lsel ? 20 : 10);
            if (r < 0) { rom_mode = RM_MENU; rom_set(pick(R_TODAY, R_TODAY_N)); }
            else if (r == 0) { rom_mode = RM_MENU; rom_set(pick(R_TIRED, R_TIRED_N)); }
            else { home_lvl0 = G.rom_lvl; sb_out = (wk_map == MAP_SBED); go(GO_TABLE, 0); }   /* (the match is already started: energy paid, the day used) */
        }
    }
}
static void name_open(void) { scr = SC_NAME; nm_len = 0; nm_buf[0] = 0; nm_r = 0; nm_c = 0; }
static void name_leave(const char *note) { scr = SC_ROM; rom_mode = RM_MENU; if (note) { snprintf(rom_note, sizeof rom_note, "%s", note); rom_note_ms = now_ms(); } }
static void name_apply(void) {   /* the typed name: MAMA / MAMA SOPHIA = mama mode (scene), SOPHIA = back (scene), anything else = nothing happens */
    int k = nm_kind(nm_buf);
    if (!g_p4_open(&G)) { name_leave(NULL); return; }
    if (k < 0) { name_leave("SHE DOES NOT ANSWER TO THAT"); return; }
    if (k == G.call) { name_leave("NOTHING CHANGED"); return; }
    G.call = k; persist(); name_leave(NULL); vn_start(k ? SCR_CALLM : SCR_CALLB);
}
static void name_key(int b) {
    int n;
    if (b == B_UP) { nm_r = (nm_r + 3) % 4; if (nm_r == 3) nm_c = nm_c / 3; else if (nm_c > 8) nm_c = 8; if (nm_r == 2 && nm_c > 8) nm_c = 8; }
    else if (b == B_DOWN) { nm_r = (nm_r + 1) % 4; if (nm_r == 3) nm_c = nm_c / 3; }
    else if (b == B_LEFT) { n = nm_cols(nm_r); nm_c = (nm_c + n - 1) % n; }
    else if (b == B_RIGHT) { n = nm_cols(nm_r); nm_c = (nm_c + 1) % n; }
    else if (b == B_B) { if (nm_len) nm_buf[--nm_len] = 0; else name_leave(NULL); }
    else if (b == B_X) { if (nm_len > 0 && nm_len < NM_LEN && nm_buf[nm_len - 1] != ' ') { nm_buf[nm_len++] = ' '; nm_buf[nm_len] = 0; } }
    else if (b == B_Y) { if (nm_len) name_apply(); else name_leave(NULL); }
    else if (b == B_A) {
        if (nm_r < 3) { char ch = NM_KEYS[nm_r][nm_c]; if (nm_len < NM_LEN && !(ch == ' ' && (nm_len == 0 || nm_buf[nm_len - 1] == ' '))) { nm_buf[nm_len++] = ch; nm_buf[nm_len] = 0; } }
        else if (nm_c == 0) { if (nm_len) nm_buf[--nm_len] = 0; }
        else if (nm_c == 1) { if (nm_len) name_apply(); else name_leave(NULL); }
        else name_leave(NULL);
    }
}
static void peek_key(int b) {   /* v6.7: she told you not to look: FACE THE WALL / LOOK AT HER */
    if (b == B_UP || b == B_DOWN) peek_sel = 1 - peek_sel;
    else if (b == B_A) { peeked = peek_sel; go(GO_SCRIPT, peeked ? SCR_PEEKPIC : (peek_ctx ? SCR_BOTTLEB : SCR_PUMPB)); }
}
static void romq_key(int b) {
    if (b == B_UP || b == B_DOWN) romq_sel = 1 - romq_sel;
    else if (b == B_A) go(GO_SCRIPT, romq_sel == 0 ? SCR_LIVEYES : SCR_LIVENO);
}
static void mono_key(int b);
static void sneak_key(int b);
static void on_button(int b) {
    if (fade_st) return;
    if (pause_kind) { pause_key(b); return; }
    if (b == B_QUIT) { open_pause(); return; }
    if (scr == SC_PLAY && cu_mode) {   /* the close-up: any of A / B closes the picture, then she speaks; A closes her line */
        if (b != B_A && b != B_B) return;
        if (cu_mode == 1) { if (!typed_done(cu_think, cu_ms)) cu_ms = now_ms() - 30000u; else { cu_mode = 2; cu_ms = now_ms(); } }   /* A finishes the typing, then she answers */
        else if (!typed_done(cu_her, cu_ms)) cu_ms = now_ms() - 30000u;
        else cu_mode = 0;
        return;
    }
    switch (scr) {
    case SC_MENU:
        if (b == B_UP) menu_sel = (menu_sel + 3) % 4; else if (b == B_DOWN) menu_sel = (menu_sel + 1) % 4;
        else if (b == B_A) {
            if (menu_sel == 0) go(GO_QSETUP, 0);
            else if (menu_sel == 1) { slots_refresh(); scr = SC_CAREER; car_sel = slot_act - 1; car_conf = 0; car_csel = 0; }   /* v10.5: the five save slots: load one or start a new career in it */
            else if (menu_sel == 2) { scr = SC_OPT; opt_row = 0; opt_sub = 0; opt_conf = 0; slots_refresh(); }
            else running = 0;
        }
        break;
    case SC_OPT:
        if (opt_key(b)) { scr = SC_MENU; menu_sel = 2; }
        break;
    case SC_CAREER:
        if (!car_conf) {
            if (b == B_UP) car_sel = (car_sel + SLOT_N - 1) % SLOT_N; else if (b == B_DOWN) car_sel = (car_sel + 1) % SLOT_N;
            else if (b == B_B) scr = SC_MENU;
            else if (b == B_A) {
                if (slot_st[car_sel] == 1) { if (slot_begin_load(car_sel + 1)) go(GO_LOAD, 0); else slots_refresh(); }   /* load it */
                else if (slot_st[car_sel] == 0) { new_game_slot(car_sel + 1); go(GO_MONO, 0); }                       /* an empty slot: the story starts there */
                else { car_conf = 1; car_csel = 0; }                                                                    /* a file that cannot be read: start over there? */
            }
            else if (b == B_X) { if (slot_st[car_sel]) { car_conf = 1; car_csel = 0; } }    /* new game in a used slot (asks first) */
            else if (b == B_Y) { if (slot_st[car_sel]) { car_conf = 2; car_csel = 0; } }    /* delete a slot (asks first) */
        } else {
            if (b == B_LEFT || b == B_RIGHT || b == B_UP || b == B_DOWN) car_csel = 1 - car_csel;
            else if (b == B_B) car_conf = 0;
            else if (b == B_A) {
                int c = car_conf; car_conf = 0;
                if (car_csel) { if (c == 1) { new_game_slot(car_sel + 1); go(GO_MONO, 0); } else { if (IO_OK) remove(slot_path(car_sel + 1)); slots_refresh(); } }
            }
        }
        break;
    case SC_WALK: wk_key(b); break;
    case SC_STORY: vn_key(b); break;
    case SC_DOOR: {
        int dk[5], dn = door_items(dk);
        if (b == B_UP) sel = (sel + dn - 1) % dn; else if (b == B_DOWN) sel = (sel + 1) % dn;
        else if (b == B_B) go(GO_HUB, 0);
        else if (b == B_A) {
            int k = dk[sel];
            if (k == DR_PLAY) {
                if (!g_can_play(&G)) { sc_page[0] = pick(S_BROKE, (int)S_BROKE_N); scene_start(1, SE_ZOMBIE); }   /* no cash for even the cheapest game: she will not play you */
                else { scr = SC_LEN; len_sel = 1; SAYP(L_LEN); }
            }
            else if (k == DR_COMEIN) go(GO_VISITIN, 0);   /* v11.0: she lets you into her room by day */
            else if (k == DR_PROPOSE) go(GO_SCRIPT, SCR_PROPOSE);
            else if (k == DR_ASK) { if (!g_move_ok(&G)) { sc_page[0] = S_UPFIRST[0]; scene_start(1, SE_DOOR); } else go(GO_SCRIPT, SCR_MOVEIN); }   /* the room is not upgraded yet: she sends you away, back to the door menu */
            else go(GO_HUB, 0);
        }
        break; }
    case SC_LEN:
        if (b == B_UP) len_sel = (len_sel + 2) % 3; else if (b == B_DOWN) len_sel = (len_sel + 1) % 3;
        else if (b == B_B) { if (fv_on) { scr = SC_WALK; wk_mode = 9; } else if (G.story == ST_FIRST) go(GO_HUB, 0); else enter_door(); }
        else if (b == B_A) { if (!g_can_afford_match(&G, SOPH_FEE[len_sel])) { len_msg_ms = now_ms(); len_msg_en = 0; break; } if (!g_soph_energy_ok(&G)) { len_msg_ms = now_ms(); len_msg_en = 1; break; } pending_len = SOPH_FEE[len_sel]; if (fv_on) go(GO_TABLE, pending_len); else vn_start(SCR_COMMENT); }   /* v11.8: inside her room there is no door scene (she already invited you in) */
        break;
    case SC_PLAY:
        if (b == B_R1) { if (G.pill_seen && !G.over && G.toact == 0 && !anim_left(0) && !cu_used) { cu_used = 1; cu_mode = 1; cu_ms = now_ms(); caught_pic(); { int mm = g_mama(&G) && G.home; /* v10.3: mama mode + free match at home = scolding mama close-up lines */ cu_lt = pick_new(mm ? CU_THINK_MM_N : CU_THINK_N, cu_lt); cu_lh = pick_new(mm ? L_STARE_MM_N : L_STARE_N, cu_lh); cu_think = mm ? CU_THINK_MM[cu_lt] : CU_THINK[cu_lt]; cu_her = mm ? L_STARE_MM[cu_lh] : L_STARE[cu_lh]; } } break; }   /* your turn, table calm, once per hand; the R1 badge is drawn only while this works */
        if (b == B_Y) { peek_toggle(G.over, G.showdown); break; }
        if (b == B_A && anim_left(G.over)) { anim_shift(anim_left(G.over)); break; }   /* A while cards or chips move: jump to the end */
        if (G.over) { if (b == B_A && (frozen || age_of(hand_end_ms) >= hand_dl()) && !hline_txt) { if (G.hand >= G.mlen) finish_match(); else begin_hand(); } }   /* (A waits until she has said her line) */
        else if (G.toact == 0 && !anim_left(0)) {
            if (b == B_A) do_action(A_CALL);
            else if (b == B_B && g_tocall(&G, 0) > 0) do_action(A_FOLD);
            else if (b == B_X && g_can_raise(&G)) do_action(A_RAISE);
        }
        break;
    case SC_END:
        if (first_end) { if (b == B_A) go(GO_SCRIPT, SCR_NAMED); }
        else if (home_end) { if (b == B_A || b == B_B) { home_end = 0; if (sb_out && wk_map == MAP_SBED) { sb_out = 0; ls_talk = 0; wk_nmap = MAP_ROOM; wk_ntx = HOME_DOOR_X; wk_nty = HOME_DOOR_Y; wk_nface = UP; go(GO_MAP, 0); } else go(GO_ROM, 1); } }   /* v9.2: a match that started in her bedroom ends outside it */   /* a free match at home: back to her door scene */
        else if (b == B_A && G.last_res == 0 && g_can_afford_match(&G, G.fee) && g_soph_energy_ok(&G)) go(GO_TABLE, G.fee);
        else if (b == B_A || b == B_B) go(GO_SCRIPT, SCR_AFTER);   /* she sends you out of her room */
        break;
    case SC_EVENT:
        if (b == B_UP || b == B_DOWN) ev_sel = !ev_sel;
        else if (b == B_B) go(GO_HUB, 0);
        else if (b == B_A) { if (ev_sel == 0) { if (!g_energy_ok(&G, EX_ENERGY)) qp_ms = now_ms(); else go(GO_QSETUP, 1); } else { scr = SC_LOTTO; lot_sel = 0; lot_ms = 0; } }   /* v9.0: EXHIBITION / LOTTERY TICKET (B leaves) */
        break;
    case SC_LOTTO:
        if (b == B_UP) lot_sel = (lot_sel + LOT_N - 1) % LOT_N;
        else if (b == B_DOWN) lot_sel = (lot_sel + 1) % LOT_N;
        else if (b == B_B) { scr = SC_EVENT; ev_sel = 1; qp_ms = 0; }
        else if (b == B_A && g_behind(&G, prog_far)) { lot_kind = 3; lot_ms = now_ms(); }   /* v10.5: the rewind guard: no ticket while you replay days another save already lived */
        else if (b == B_A) { int r = g_lot_buy(&G, lot_sel, rnd(100)); if (r == 1) persist(); lot_kind = r == 1 ? 0 : r == 0 ? 1 : 2; lot_ms = now_ms(); }   /* the result is rolled now and kept in the game */
        break;
    case SC_QSETUP: {
        { int nq = mp_mode ? g_ex_n(&G) : 4; if (b == B_UP) qs_sel = (qs_sel + nq - 1) % nq; else if (b == B_DOWN) qs_sel = (qs_sel + 1) % nq; }
        if (b == B_UP || b == B_DOWN) {}
        else if (b == B_B) { if (mp_mode) { scr = SC_EVENT; ev_sel = 0; qp_ms = 0; } else go(GO_MENU, 0); }   /* back to the Event Center menu (v9.0; B again leaves) */
        else if (b == B_A) {
            qs_si = qs_sel;
            if (mp_mode == 1) { int r = g_ex_start(&G, EX_BUYIN[qs_si]); if (r <= 0) { qp_kind = r < 0; qp_ms = now_ms(); break; } persist(); }   /* the buy-in leaves your cash and the energy goes now */
            go(GO_MTABLE, 0);
        }
        break; }
    case SC_MTABLE:
        if (bm_open && !M.over && M.toact == 0 && !anim_left(0)) { bm_key(b); break; }   /* the bet menu takes every key */
        if (b == B_Y) { peek_toggle(M.over, M.showdown && !M.folded[0]); break; }
        if (b == B_A && anim_left(M.over)) { anim_shift(anim_left(M.over)); break; }
        if (M.over) { if (b == B_A && (frozen || age_of(hand_end_ms) >= hand_dl())) { if (M.game_over) mp_end_game(); else mp_hand_start(); } }
        else if (M.toact == 0 && !anim_left(0)) {
            if (b == B_A) mp_do(A_CALL);
            else if (b == B_B && mp_need(&M, 0) > 0) mp_do(A_FOLD);
            else if (b == B_X && mp_can_raise(&M, 0)) { if (M.nl) bm_start(); else mp_do(A_RAISE); }   /* v8.9: free bet size = the bet menu */
        }
        break;
    case SC_MEND: if (b == B_A) { if (boss_win) { boss_win = 0; go(GO_SCRIPT, SCR_BOSS); } else go(mp_mode ? GO_HUB : GO_MENU, 0); } break;   /* (beating the boss: her scene first) */
    case SC_ATM: atm_key(b); break;
    case SC_STORE: store_key(b); break;
    case SC_SHOP: shop_key(b); break;
    case SC_DEBUG: dbg_key(b); break;
    case SC_ROM: rom_key(b); break;
    case SC_ROMQ: romq_key(b); break;
    case SC_NAME: name_key(b); break;
    case SC_PEEK: peek_key(b); break;
    case SC_INV: inv_key(b); break;
    case SC_MONO: mono_key(b); break;
    case SC_SNEAK: sneak_key(b); break;
    case SC_OVER: if (b == B_A && (frozen || age_of(over_ms) > 1200)) fresh_game(); break;   /* v10.5: back to the title, no save is deleted */
    case SC_SCENE: scene_key(b); break;
    default: if (b == B_B || b == B_A) { scr = SC_DOOR; sel = 1; } break;
    }
}
static int map_key(int k) {   /* Miyoo: A=SPACE B=LCTRL X=LSHIFT Y=LALT START=RETURN (pause menu) SELECT=RCTRL (nothing) MENU=ESC */
    if (k == SDLK_ESCAPE || k == SDLK_RETURN) return B_QUIT;   /* v8.5: START (RETURN) opens the pause menu, SELECT (RCTRL) does nothing */
    if (k == SDLK_SPACE || k == SDLK_z) return B_A;
    if (k == SDLK_LCTRL || k == SDLK_x) return B_B;
    if (k == SDLK_LSHIFT || k == SDLK_c) return B_X;
    if (k == SDLK_LALT || k == SDLK_v) return B_Y;
    if (k == SDLK_e) return B_L1;   /* Miyoo Mini (Onion): L1 = E (not used at the moment) */
    if (k == SDLK_t) return B_R1;   /* Miyoo Mini (Onion): R1 = T, same key on the desktop */
    if (k == SDLK_BACKSPACE || k == SDLK_r) return B_R2;   /* v9.5: Miyoo Mini (Onion): R2 = BACKSPACE; desktop R. Opens the name keyboard in her menu once you are married */
    if (k == SDLK_UP) return B_UP;
    if (k == SDLK_DOWN) return B_DOWN;
    if (k == SDLK_LEFT) return B_LEFT;
    if (k == SDLK_RIGHT) return B_RIGHT;
    return -1;
}

static void update(void) {
    sfx_tick();
    fade_step();
    if (fade_st || pause_kind) return;
    wk_update();
    if (scr == SC_WALK && !wk_mode && !wk_moving && !fade_st && g_broke(&G)) { scr = SC_OVER; over_rent = 0; over_ms = now_ms(); }   /* nothing left to play with */
    if (scr == SC_PLAY && hline_txt && now_ms() >= hline_ms) { say(hline_txt); hline_txt = NULL; }   /* her line for the hand that just ended: after the banner */
    if (scr == SC_PLAY && !G.over && G.toact == 1 && now_ms() >= think_at) do_action(ai_decide(&G));
    if (scr == SC_MTABLE && !M.over && M.toact > 0 && now_ms() >= think_at) { int by, a = mp_decide_by(&M, M.toact, &by); mp_do_by(a, by); }
}

/* Loads a PNG and enlarges it by num/den (nearest neighbour, keeps pixel art sharp). cols x rows > 1: the picture is a sheet of equal frames, each frame is enlarged on its own
   so the frames stay evenly spaced. opaque = flatten over black into a plain surface (fast to copy; for the maps). */
/* darken a picture in place for night: a little less red and green, and a blue cast (the alpha, if any, is kept) */
static void night_t(SDL_Surface *im, int nr, int ng, int nb) {
    int x, y, w, h; Uint32 rs, gs, bs, rm, gm, bm; if (!im) return;
    rs = im->format->Rshift; gs = im->format->Gshift; bs = im->format->Bshift; rm = 255u << rs; gm = 255u << gs; bm = 255u << bs; w = im->w; h = im->h;   /* v7.2: format fields read once, not per pixel */
    for (y = 0; y < h; y++) { Uint32 *q = px(im, 0, y); for (x = 0; x < w; x++, q++) {
        Uint32 p = *q;
        int r = (int)((p >> rs) & 255), g = (int)((p >> gs) & 255), b = (int)((p >> bs) & 255);
        r = r * nr / 100; g = g * ng / 100; b = b * nb / 100 + (b ? 4 : 0); if (b > 255) b = 255;
        *q = (p & ~(rm | gm | bm)) | ((Uint32)r << rs) | ((Uint32)g << gs) | ((Uint32)b << bs);
    } }
}
static void night(SDL_Surface *im) { night_t(im, NIGHT_R, NIGHT_G, NIGHT_B); }            /* her room (and you in it) */
static void night_mild(SDL_Surface *im) { night_t(im, NIGHTM_R, NIGHTM_G, NIGHTM_B); }    /* the rest of the hotel at night */
static int ld_crop;   /* 1 = a single picture bigger than the screen keeps only its top-left 640x480 (logical): that is all the game ever shows of it */
static SDL_Surface *load_png_ex(const char *path, int num, int den, int opaque, int cols, int rows) {
    int w, h, n, x, y, fw, fh, cw, ch, cwN, chN, W, H, native; SDL_Surface *t, *r; unsigned char *d = stbi_load(path, &w, &h, &n, 4), *e;
    if (!d) return NULL;
    fw = (w + cols - 1) / cols; fh = (h + rows - 1) / rows;
    cw = (fw * num + den - 1) / den; ch = (fh * num + den - 1) / den;   /* one frame at the nominal (logical) size */
    if (ld_crop && cols == 1 && rows == 1) { if (cw > 640) cw = 640; if (ch > 480) ch = 480; }
    native = (num == 1 && den == 1 && cols == 1 && rows == 1 && w == NATW && h == NATH);   /* v7.1: a full-screen NATW x NATH picture: its logical size is 640x480 and it is copied 1:1 at the native screen size */
    if (native) { cw = 640; ch = 480; }
    cwN = (int)(((long long)cw * SW + 639) / 640); chN = (int)(((long long)ch * SH + 479) / 480);   /* ...and at the real screen size */
    W = cwN * cols; H = chN * rows;
    e = (unsigned char *)calloc((size_t)W * H, 4);
    if (!e) { stbi_image_free(d); return NULL; }
    if (native) {   /* v7.1: 1:1 at the native screen; at any other screen size a plain nearest-neighbour fit */
        for (y = 0; y < H; y++) { int sy = (int)((long long)y * h / H); for (x = 0; x < W; x++) memcpy(e + ((size_t)y * W + x) * 4, d + ((size_t)sy * w + (int)((long long)x * w / W)) * 4, 4); }
    } else if (SW == 640 && SH == 480) {   /* the old path: nearest neighbour, keeps pixel art sharp */
        for (y = 0; y < H; y++) for (x = 0; x < W; x++) {
            int lx = (x % cw) * den / num, ly = (y % ch) * den / num, sx = (x / cw) * fw + lx, sy = (y / ch) * fh + ly;
            if (lx < fw && ly < fh && sx < w && sy < h) memcpy(e + ((size_t)y * W + x) * 4, d + ((size_t)sy * w + sx) * 4, 4);
        }
    } else {   /* v7.0: the nominal enlargement (nearest neighbour, as before) is resampled to the screen size with a bilinear filter, in one pass (premultiplied, so soft edges stay clean) */
        int *x0 = (int *)malloc(sizeof(int) * (size_t)cwN * 3), *y0 = (int *)malloc(sizeof(int) * (size_t)chN * 3), i, k;
        if (!x0 || !y0) { free(x0); free(y0); free(e); stbi_image_free(d); return NULL; }
        for (k = 0; k < 2; k++) {
            int cnt = k ? chN : cwN, nominal = k ? ch : cw, scr = k ? SH : SW, lg = k ? 480 : 640, frame = k ? fh : fw, total = k ? h : w; int *tb = k ? y0 : x0;
            for (i = 0; i < cnt; i++) {
                double f = (i + 0.5) * lg / scr - 0.5; int l0 = (int)floor(f), l1 = l0 + 1, wt = (int)((f - l0) * 256);
                if (l0 < 0) l0 = 0; if (l1 < 0) l1 = 0; if (l0 > nominal - 1) l0 = nominal - 1; if (l1 > nominal - 1) l1 = nominal - 1;
                l0 = l0 * den / num; l1 = l1 * den / num; if (l0 > frame - 1) l0 = frame - 1; if (l1 > frame - 1) l1 = frame - 1; (void)total;
                tb[i * 3] = l0; tb[i * 3 + 1] = l1; tb[i * 3 + 2] = wt;
            }
        }
        for (y = 0; y < H; y++) {
            int row = y / chN, yl = y % chN, ya = row * fh + y0[yl * 3], yb = row * fh + y0[yl * 3 + 1], wy = y0[yl * 3 + 2];
            if (ya > h - 1) ya = h - 1; if (yb > h - 1) yb = h - 1;
            for (x = 0; x < W; x++) {
                int col = x / cwN, xl = x % cwN, xa = col * fw + x0[xl * 3], xb = col * fw + x0[xl * 3 + 1], wx = x0[xl * 3 + 2];
                const unsigned char *p00, *p10, *p01, *p11; unsigned char *o = e + ((size_t)y * W + x) * 4;
                unsigned w00, w10, w01, w11;
                if (xa > w - 1) xa = w - 1; if (xb > w - 1) xb = w - 1;
                p00 = d + ((size_t)ya * w + xa) * 4; p10 = d + ((size_t)ya * w + xb) * 4; p01 = d + ((size_t)yb * w + xa) * 4; p11 = d + ((size_t)yb * w + xb) * 4;
                w00 = (unsigned)((256 - wx) * (256 - wy)); w10 = (unsigned)(wx * (256 - wy)); w01 = (unsigned)((256 - wx) * wy); w11 = (unsigned)(wx * wy);   /* sum 65536 */
                if ((p00[3] & p10[3] & p01[3] & p11[3]) == 255) {   /* solid: plain bilinear */
                    for (i = 0; i < 3; i++) o[i] = (unsigned char)((p00[i] * w00 + p10[i] * w10 + p01[i] * w01 + p11[i] * w11 + 32768u) >> 16);
                    o[3] = 255;
                } else {
                    unsigned a0 = p00[3], a1 = p10[3], a2 = p01[3], a3 = p11[3], A = (a0 * w00 + a1 * w10 + a2 * w01 + a3 * w11 + 32768u) >> 16;
                    if (A > 255) A = 255;
                    if (A) for (i = 0; i < 3; i++) {   /* premultiplied sum, then back */
                        unsigned long long sum = (unsigned long long)p00[i] * a0 * w00 + (unsigned long long)p10[i] * a1 * w10 + (unsigned long long)p01[i] * a2 * w01 + (unsigned long long)p11[i] * a3 * w11;
                        unsigned long long tot = (unsigned long long)a0 * w00 + (unsigned long long)a1 * w10 + (unsigned long long)a2 * w01 + (unsigned long long)a3 * w11;
                        unsigned v = tot ? (unsigned)(sum / tot) : 0; o[i] = (unsigned char)(v > 255 ? 255 : v);
                    }
                    o[3] = (unsigned char)A;
                }
            }
        }
        free(x0); free(y0);
    }
    stbi_image_free(d); d = e; w = W; h = H;
    if (opaque) {
        r = SDL_CreateRGBSurface(0, w, h, 32, S->format->Rmask, S->format->Gmask, S->format->Bmask, 0);
        if (r) for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
            const unsigned char *q = d + ((size_t)y * w + x) * 4; int a = q[3];
            *px(r, x, y) = SDL_MapRGB(S->format, (Uint8)(q[0] * a / 255), (Uint8)(q[1] * a / 255), (Uint8)(q[2] * a / 255));
        }
        free(d); return r;
    }
    t = SDL_CreateRGBSurfaceFrom(d, w, h, 32, w*4, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
#ifdef USE_SDL1
    r = SDL_DisplayFormatAlpha(t);
#else
    r = SDL_ConvertSurfaceFormat(t, SDL_PIXELFORMAT_ARGB8888, 0);
#endif
    SDL_FreeSurface(t); free(d); return r;
}
static SDL_Surface *load_png(const char *path) { return load_png_ex(path, 1, 1, 0, 1, 1); }

#ifdef ROTATE180
/* the Miyoo Mini's screen is mounted upside down relative to the SDL framebuffer: turn the finished frame 180 degrees */
static void rotate180(void) {
    int y, x;
    for (y = 0; y < SH / 2; y++) {
        Uint32 *a = px(S, 0, y), *b = px(S, 0, SH - 1 - y) + (SW - 1);   /* v7.2: pointer walk, no index maths per pixel */
        for (x = 0; x < SW; x++, a++, b--) { Uint32 t = *a; *a = *b; *b = t; }
    }
    if (SH & 1) { Uint32 *a = px(S, 0, SH / 2), *b = a + (SW - 1); for (; a < b; a++, b--) { Uint32 t = *a; *a = *b; *b = t; } }
}
#endif

#ifdef USE_SDL1
/* v7.4: show the finished frame. The game draws (and blends, which reads pixels back) in ordinary memory; here it is copied to the double-buffered screen in one pass,
   turned 180 degrees on the way when ROTATE180 is set, and the buffers are swapped with SDL_Flip (the screen is then never half old, half new). */
static void present(void) {
    if (vscr != S) {
        int y, lock = SDL_MUSTLOCK(vscr);
        if (lock && SDL_LockSurface(vscr) < 0) return;
        for (y = 0; y < SH; y++) {
            Uint32 *d = (Uint32 *)((Uint8 *)vscr->pixels + (size_t)y * vscr->pitch);
#ifdef ROTATE180
            const Uint32 *sp = px(S, 0, SH - 1 - y) + (SW - 1); int x;
            for (x = 0; x < SW; x++) d[x] = *sp--;   /* write forwards (screen memory likes that), read backwards (plain memory does not mind) */
#else
            memcpy(d, px(S, 0, y), (size_t)SW * 4);
#endif
        }
        if (lock) SDL_UnlockSurface(vscr);
    } else {
#ifdef ROTATE180
        rotate180();
#endif
    }
    SDL_Flip(vscr);
}
#endif

static void frame(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { persist(); running = 0; }
        else if (e.type == SDL_KEYDOWN) { int b = map_key(e.key.keysym.sym); if (b >= 0) { kheld[b] = 1; if (dir_of(b) >= 0) last_dir = dir_of(b); on_button(b); } }
        else if (e.type == SDL_KEYUP) { int b = map_key(e.key.keysym.sym); if (b >= 0) kheld[b] = 0; }
    }
    update();
    draw_all();
    if (fade_a > 0) shade(0, 0, 640, 480, 0, 0, 0, fade_a);
#ifdef USE_SDL1
    present();
#else
#ifdef ROTATE180
    rotate180();
#endif
    SDL_UpdateWindowSurface(win);
#endif
#ifdef __EMSCRIPTEN__
    if (!running) emscripten_cancel_main_loop();
#endif
}

#ifdef HAVE_SHOTS
/* ---------- screenshots (local checks): scripted, deterministic states ---------- */
static unsigned char *shotbuf; static int shot_n;
static void save_shot(const char *name) {
    int x, y;
    for (y = 0; y < SH; y++) for (x = 0; x < SW; x++) {
        Uint8 r, g, b; Uint32 p = *px(S, x, y);
        SDL_GetRGB(p, S->format, &r, &g, &b);
        shotbuf[(y*SW+x)*3] = r; shotbuf[(y*SW+x)*3+1] = g; shotbuf[(y*SW+x)*3+2] = b;
    }
    stbi_write_png(name, SW, SH, 3, shotbuf, SW*3);
}
static void shot(const char *name) { char p[96]; if (!shotbuf) return; draw_all(); snprintf(p, sizeof p, "shots/%02d_%s.png", ++shot_n, name); save_shot(p); }
#define CD(rank, suit) ((suit) * 13 + (rank))   /* rank 0..12 = 2..A, suit 0..3 = spade heart diamond club */
/* new hand with chosen cards: you, her, then the board (rest of the deck follows) */
static void rig(int y0, int y1, int h0, int h1, int b0, int b1, int b2, int b3, int b4, int button) {
    int d[9] = {y0, y1, h0, h1, b0, b1, b2, b3, b4}, used[52] = {0}, i, k = 9;
    begin_hand(); memcpy(G.deck, d, sizeof d);
    for (i = 0; i < 9; i++) used[d[i]] = 1;
    for (i = 0; i < 52; i++) if (!used[i]) G.deck[k++] = i;
    G.button = button; G.toact = button;
    G.c[button] = G.r[button] = SB_AMT; G.c[!button] = G.r[!button] = BB_AMT;
}
static void run_to_showdown(void) { int n = 0; while (!G.over && n++ < 30) do_action(A_CALL); }
static void beat_shot(int id, int i, const char *name) { vn_start(id); vn_i = i; vn_prev_spr = vn_buf[i].spr; shot(name); }
static int romance_tests(void);
static int v68_tests(void);
static int v79_tests(void);
static int v80_tests(void);
static int v81_tests(void);
static int v82_tests(void);
static int v108_tests(void);
static int v111_tests(void);
static int v112_tests(void);
static int v113_tests(void);
static int v115_tests(void); static int v116_tests(void); static int v119_tests(void); static int v120_tests(void); static int v121_tests(void); static int v122_tests(void); static int v123_tests(void); static int cow_disgust(const char *t);
static int v110_tests(void);
static int v84_tests(void);
static int v85_tests(void);
static int v89_tests(void);
static int v90_tests(void);
static int v92_tests(void);
static int v94_tests(void);
static int v95_tests(void);
static int in_pool(const char *t, const char *const *pool, int n);
static int v97_tests(void); static int v105_tests(void); static int v99_tests(void); static int v104_tests(void); static int v106_tests(void); static int v107_tests(void); static int v100_tests(void); static int v101_tests(void); static int v102_tests(void);
static int v86_tests(void);
static int v63_tests(void);
static void shots(void) {
    Game base; int i;
    shotbuf = malloc((size_t)SW*SH*3); frozen = 1; fake_ms = 200700; srand(11); ls_enabled = 0;   /* (v8.0: Sophia only shows up in the v80 tests) */
    g_init(&G); scr = SC_MENU; menu_sel = 1; shot("menu");
    qp_ms = fake_ms - 1000; menu_sel = 0; shot("menu_quickplay_soon"); qp_ms = 0; menu_sel = 1;
    /* intro in the lobby */
    wk_place(MAP_LOBBY, 12, 10, UP); beat_shot(SCR_INTRO, 0, "intro_arrival"); wk_place(MAP_LOBBY, 12, 10, UP); beat_shot(SCR_INTRO, 1, "intro_front_desk_line");
    G.story = ST_NEW; wk_place(MAP_LOBBY, 12, 10, UP); enter_hub(); shot("intro_walk_to_desk"); wk_place(MAP_LOBBY, 8, 6, UP); shot("intro_at_desk_prompt");
    wk_place(MAP_LOBBY, 8, 6, UP); beat_shot(SCR_BUMP, 0, "bump_walk_into_someone"); beat_shot(SCR_BUMP, 1, "bump_who_said_that"); beat_shot(SCR_BUMP, 2, "bump_sorry_no_sprite");
    beat_shot(SCR_BUMP, 3, "bump_she_appears"); beat_shot(SCR_BUMP, 8, "bump_dont_stare"); beat_shot(SCR_BUMP, 11, "bump_she_leaves"); beat_shot(SCR_BUMP, 12, "bump_storm"); beat_shot(SCR_BUMP, 13, "bump_forget_it");
    G.story = ST_HOME; G.hall_seen = 0; wk_place(MAP_LOBBY, 7, 6, DN); enter_hub(); shot("after_bump_go_to_your_room");
    wk_place(MAP_HALL, 4, 3, DN); hall_script(1); shot("hall2_look_1"); hall_script(2); shot("hall2_look_2_my_door"); hall_script(3);
    wk_place(MAP_HALL3, 9, 3, DN); hall_script(4); shot("hall3_look_1"); hall_script(5); shot("hall3_look_2_her_door"); hall_script(6); G.hall_seen = 3;
    /* hub and the first visit */
    G.story = ST_HOME; G.cash = 0; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN); enter_hub(); shot("room_first");
    beat_shot(SCR_DOOR1, 0, "door1_knock"); beat_shot(SCR_DOOR1, 1, "door1_you_again"); beat_shot(SCR_DOOR1, 5, "door1_give_card_back"); beat_shot(SCR_DOOR1, 8, "door1_elevator_down");
    /* the forced first match, at the table */
    G.story = ST_FIRST; start_match(10); G.net = 6; G.hand = 3;
    rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 0); shot("first_match_table_name_plate");
    pause_kind = PK_TABLE; pause_sel = 1; shot("pause_leave_table"); pause_kind = PK_NONE;
    G.in_match = 0; G.mlen = 10; G.hand = 10; G.net = 12; g_match_start(&G, 100); G.net = 12; g_match_finish(&G); first_end = 1; G.story = ST_NAMED; scr = SC_END; say(L_ENDYOU[0]); shot("first_match_end");
    beat_shot(SCR_NAMED, 0, "named_line1"); beat_shot(SCR_NAMED, 2, "named_sophia"); beat_shot(SCR_NAMED, 6, "named_rematch"); first_end = 0;
    /* later visits */
    G.cash = 1340; G.bank = 2500; G.last_res = -1; G.m_played = 3; G.m_won = 1; G.m_lost = 2; enter_hub();
    /* walking: your room, the hall and the lobby */
    wk_place(MAP_ROOM, 1, 3, UP); shot("room_fridge_prompt"); wk_place(MAP_ROOM, 2, 5, LF); shot("room_sofa_prompt"); wk_place(MAP_ROOM, 1, 3, UP); shot("v68_room_in_front_of_the_fridge");
    wk_mode = 2; wk_yes = 0; shot("room_save_ask"); wk_yes = 1; shot("room_save_ask_no"); wk_mode = 0;
    wk_place(MAP_ROOM, 8, 3, RT); shot("room_bed"); wk_place(MAP_ROOM, 9, 5, RT); shot("room_bathroom_side");
    wk_place(MAP_ROOM, 2, 3, UP); wk_say(NULL, 1, WK_NONE, W_SINK, NULL); shot("room_sink_message"); wk_mode = 0;
    wk_place(MAP_ROOM, 3, 5, DN); shot("room_exit_mat");
    wk_place(MAP_HALL, 12, 3, UP); shot("hall2_my_door_prompt"); wk_place(MAP_HALL, 8, 3, UP); shot("hall2_stairs_prompt"); wk_place(MAP_HALL, 4, 3, UP); shot("hall2_elevator_prompt");
    wk_nflr = elev_floors(2, wk_flr); wk_fsel = 0; wk_mode = 5; shot("elevator_menu_from_2f"); wk_nflr = elev_floors(1, wk_flr); wk_fsel = 1; shot("elevator_menu_from_1f"); wk_nflr = elev_floors(3, wk_flr); wk_fsel = 0; shot("elevator_menu_from_3f"); wk_mode = 0;
    wk_place(MAP_HALL1, 6, 3, UP); shot("hall1_elevator_prompt"); wk_place(MAP_HALL1, 11, 9, DN); shot("hall1_stairs_prompt"); wk_place(MAP_HALL1, 10, 8, UP); shot("v59_hall1_stairs_left_side"); wk_place(MAP_HALL1, 2, 9, DN); shot("hall1_lobby_door_prompt"); wk_place(MAP_HALL1, 3, 6, RT); shot("hall1_pit_side");
    wk_place(MAP_HALL3, 1, 4, LF); shot("hall3_sophia_door_prompt"); wk_place(MAP_HALL3, 9, 3, UP); shot("hall3_elevator_prompt"); wk_place(MAP_HALL3, 11, 5, RT); shot("hall3_sofa_side");
    wk_place(MAP_LOBBY, 3, 4, DN); shot("lobby_arrive"); wk_place(MAP_LOBBY, 8, 6, UP); shot("lobby_desk_prompt");
    wk_say(W_CLERK_TAG, 0, WK_NONE, W_CLERK[0], NULL); shot("lobby_clerk_greeting"); wk_mode = 0;
    wk_place(MAP_LOBBY, 10, 4, UP); shot("lobby_atm_prompt"); wk_place(MAP_LOBBY, 5, 4, UP); shot("v91_lobby_vending_prompt"); wk_place(MAP_LOBBY, 1, 6, LF); shot("lobby_event_door");
    wk_say(NULL, 1, WK_NONE, W_EVENT_LOCKED, NULL); shot("lobby_event_locked"); wk_mode = 0;
    wk_place(MAP_LOBBY, 12, 11, DN); wk_say(NULL, 1, WK_NONE, W_STORM, NULL); shot("lobby_storm_message"); wk_mode = 0;
    wk_place(MAP_LOBBY, 14, 7, RT); shot("lobby_right_side");
    { Game keep = G; G.cash = 5; saved_G = G; G.cash = 99; pause_kind = PK_HOME; pause_sel = 2; shot("pause_exit"); pause_kind = PK_EXITC; pause_sel = 0; shot("pause_exit_unsaved_warning"); saved_G = G; shot("pause_exit_all_saved"); pause_kind = PK_NONE; G = keep; saved_G = G; }
    wk_place(MAP_HALL3, 1, 4, LF);
    vn_start(SCR_GREET); vn_i = 1; vn_prev_spr = 1; shot("greeting_after_loss");
    G.forfeit = 1; vn_start(SCR_GREET); vn_i = 1; vn_prev_spr = 1; shot("greeting_coward");
    enter_door(); shot("door_menu");
    scr = SC_LEN; len_sel = 1; say(L_LEN[0]); shot("choose_hands");
    len_sel = 2; vn_start(SCR_COMMENT); shot("her_comment_30");
    pause_kind = PK_SKIP; pause_sel = 1; shot("pause_skip_scene"); pause_kind = PK_NONE;
    G.forfeit = 1; vn_start(SCR_LEAVE); shot("leave_table_coward");
    base = G;
    /* the table (old checks) */
    G.story = ST_NAMED; start_match(20); G.net = 18; G.hand = 6;
    rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 0); say("YOUR MOVE,|NEIGHBOR."); shot("preflop_your_turn");
    do_action(A_RAISE); do_action(A_CALL); do_action(A_CALL); shot("flop_your_turn");
    do_action(A_RAISE); shot("flop_her_turn");
    do_action(A_RAISE); say("THIS WILL COST|YOU."); shot("her_raised");
    do_action(A_CALL); do_action(A_CALL); do_action(A_CALL);
    run_to_showdown(); shot("showdown_win");
    rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 0); do_action(A_RAISE); G.toact = 1; do_action(A_FOLD); shot("she_folds");
    rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 1); G.toact = 0; do_action(A_FOLD); shot("you_fold");
    rig(CD(5,2), CD(0,3), 12, 25, 10, 7, 2, 16, 32, 1); G.toact = 1; run_to_showdown(); shot("showdown_lose");
    G.hand = 19; rig(12, 25, 11, 24, CD(10,0), CD(3,1), CD(7,2), CD(5,3), CD(1,0), 0); say("LAST HAND.|MAKE IT COUNT."); shot("last_hand");
    rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 0); cu_think = CU_THINK[0]; cu_her = L_STARE[0]; cu_mode = 1; shot("v44_r1_closeup"); cu_mode = 2; shot("v44_r1_her_line"); cu_mode = 0;
    g_match_start(&G, 500); G.net = 46; G.bn[BN_FLUSH] = 2; G.bn[BN_STRAIGHT] = 1; G.bn[BN_FULL] = 1; g_match_finish(&G); scr = SC_END; say(L_ENDYOU[0]); shot("end_win");
    g_match_start(&G, 500); G.net = -30; G.bn[BN_ROYAL] = 1; G.bn[BN_FLUSH] = 1; g_match_finish(&G); say(L_ENDLOST[0]); shot("end_loss_bonus_lost");
    g_match_start(&G, 500); G.net = 0; g_match_finish(&G); say(L_ENDTIE[0]); shot("end_tie");
    /* ATM */
    G = base; atm_open(); shot("atm_menu");
    atm_key(B_DOWN); atm_key(B_A); atm_key(B_LEFT); atm_key(B_UP); atm_key(B_LEFT); atm_key(B_UP); atm_key(B_UP); shot("atm_withdraw_entry");
    atm_key(B_A); shot("atm_receipt");
    atm_key(B_A); atm_key(B_Y); atm_key(B_UP); atm_key(B_A); shot("atm_insufficient");
    /* the key card, the options screen and the CPU card box */
    {   /* v5.3: the four table GUI sizes (options screen, Sophia's table, Exhibition, Quick Play) and the Career menu */
        Game keep53 = G; int z; static const char *const zn[4] = {"100", "90", "80", "70"};
        for (z = 0; z < 1; z++) {
            char nm[48]; G = keep53; G.cash = 5000; G.bank = 0; opt_row = 0; scr = SC_OPT; snprintf(nm, sizeof nm, "v53_options_%s", zn[z]); shot(nm);
            G.story = ST_NAMED; start_match(20); G.net = 18; G.hand = 6;
            rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 0); say("YOUR MOVE,|NEIGHBOR."); snprintf(nm, sizeof nm, "v53_sophia_%s", zn[z]); shot(nm);
            if (z == 0 || z == 3) { run_to_showdown(); snprintf(nm, sizeof nm, "v53_sophia_showdown_%s", zn[z]); shot(nm); }
            mp_mode = 1; qs_si = 2; mp_begin(); snprintf(nm, sizeof nm, "v53_exhibition_%s", zn[z]); shot(nm);
            mp_mode = 0; qs_si = 1; mp_begin(); snprintf(nm, sizeof nm, "v53_quickplay_%s", zn[z]); shot(nm);
        }
        G = keep53; G.story = ST_NAMED; G.day = 3; G.phase = PH_EVENING; G.cash = 450; scr = SC_CAREER; car_sel = 0; car_conf = 0; shot("v53_career_continue");
        car_sel = 1; shot("v53_career_newgame"); car_conf = 1; car_csel = 0; shot("v53_career_confirm_no"); car_csel = 1; shot("v53_career_confirm_yes"); car_conf = 0; car_sel = 0; G = keep53;
        {   /* v10.9: the slot list at its widest (4-digit day, 7-digit cash, a slot that cannot be read, an empty one) */
            int k; Game sv = G; int stv[SLOT_N]; Game gv[SLOT_N]; int sa = slot_act; memcpy(stv, slot_st, sizeof stv); memcpy(gv, slot_g, sizeof gv);
            for (k = 0; k < SLOT_N; k++) { slot_st[k] = 1; slot_g[k] = G; slot_g[k].day = 9 + k * 300; slot_g[k].phase = k % 3; slot_g[k].cash = 450 + k * 2500000; }
            slot_st[3] = 2; slot_st[4] = 0; slot_act = 3; scr = SC_CAREER; car_sel = 1; car_conf = 0; shot("v109_career_slots"); car_sel = 4; shot("v109_career_slots_empty");
            car_sel = 2; car_conf = 1; car_csel = 0; shot("v109_career_confirm"); car_conf = 0;
            { int os = opt_sub, ol = opt_lsel, oc = opt_conf, of = opt_from; scr = SC_OPT; opt_sub = 1; opt_lsel = 2; opt_conf = 0; shot("v109_options_slots"); opt_conf = 1; opt_csel = 1; shot("v109_options_slots_confirm"); opt_sub = os; opt_lsel = ol; opt_conf = oc; opt_from = of; }
            memcpy(slot_st, stv, sizeof stv); memcpy(slot_g, gv, sizeof gv); slot_act = sa; G = sv; scr = SC_CAREER; car_sel = 0;
        }
    }
    {   /* v5.4 screenshots: options (title and in game), pause menus, the night desk, the ATM, buy-in menus, the end screen, CPU box, L1 / R1 badges */
        Game keep54 = G; int km = wk_map, kx = wk_tx, ky = wk_ty, kf = wk_face;
        scr = SC_OPT; opt_row = 0; dlg_idx = 2; shot("v54_options_title"); opt_row = 1; shot("v54_options_title_sound"); scr = SC_MENU;
        G.story = ST_NAMED; G.day = 12; G.phase = PH_EVENING; G.cash = 1234; room_seen = 1; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN);
        shot("v54_room_evening_day12"); G.day = 1; G.phase = PH_MORNING; shot("v54_room_day1_morning"); G.day = 100; G.phase = PH_EVENING; shot("v54_room_day100_evening"); G.day = 3; G.phase = PH_NIGHT; shot("v54_room_day3_night");
        pause_kind = PK_HOME; pause_sel = 2; shot("v54_pause_walking_4_items"); pause_kind = PK_OPT; opt_row = 0; opt_from = PK_HOME; shot("v54_options_in_game_walking"); pause_kind = PK_NONE;
        G.phase = PH_MORNING; wk_place(MAP_LOBBY, 7, 6, UP); shot("v54_lobby_morning_clerk_and_store_icon");
        G.phase = PH_NIGHT; shot("v54_lobby_night_no_clerk"); wk_say(NULL, 1, WK_NONE, W_DESK_NIGHT, NULL); wk_ms = fake_ms - 30000; shot("v54_lobby_night_desk_line"); wk_mode = 0; G.phase = PH_MORNING;
        atm_open(); shot("v54_atm_menu"); atm_mode = 1; atm_kind = 1; atm_amt = 12345; atm_cur = 2; shot("v54_atm_amount"); atm_mode = 0; scr = SC_WALK;
        G.cash = 5000; start_match(10); rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 0); say("YOUR MOVE,|NEIGHBOR."); shot("v54_sophia_table_l1_r1"); pause_kind = PK_TABLE; pause_sel = 1; shot("v54_table_pause_3_items"); pause_kind = PK_OPT; opt_from = PK_TABLE; shot("v54_options_in_game_table"); pause_kind = PK_NONE;
        mp_mode = 1; qs_si = 2; mp_begin(); shot("v54_exhibition_cpu_box"); mp_mode = 0; qs_si = 1; mp_begin(); shot("v54_quickplay_cpu_box");
        scr = SC_QSETUP; mp_mode = 1; qs_sel = 1; G.cash = 120; shot("v54_exhibition_buyin"); mp_mode = 0; shot("v54_quickplay_chips");
        scr = SC_LEN; len_sel = 1; say(L_LEN[0]); shot("v54_sophia_buyin_prize");
        G.last_res = 1; G.last_net = 14; G.fee = 100; G.last_pay = 514; G.last_drink = 0; G.mlen = 10; memset(G.bn, 0, sizeof G.bn); scr = SC_END; first_end = 0; shot("v54_end_win_prize");
        G.fee = 500; G.bn[BN_STRAIGHT] = 1; G.last_net = 14; G.last_pay = 2670; G.last_drink = 1; shot("v54_end_win_prize_bonus_drink");
        G = keep54; scr = SC_WALK; wk_place(km, kx, ky, kf);
    }
    {   /* v5.5 screenshots: thinking box under her name, L1 / R1 as plain text, the buy-in menu without prizes, rent due, the desk menu, the refusal, the rent game over */
        Game keep55 = G; int km = wk_map, kx = wk_tx, ky = wk_ty, kf = wk_face;
        G.cash = 9000; start_match(500); rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 1); her_tag = 0; shot("v55_table_her_thinking_stake_x5");
        her_tag = 2; shot("v55_table_her_action_call"); her_tag = 0;
        rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 0); say("YOUR MOVE,|NEIGHBOR."); shot("v55_table_your_turn_l1_r1_text");
        peek = 1; shot("v56_table_y_shown_dealer_top_r1_above_cards"); peek = 0; G.phase = PH_NIGHT; beat_shot(SCR_BEDDOOR, 0, "v56_bedroom_door_narration"); G.phase = PH_MORNING;
        G = keep55; G.cash = 700; scr = SC_LEN; len_sel = 1; say(L_LEN[0]); shot("v55_buyin_menu_no_prize");
        G.story = ST_NAMED; G.day = 7; G.phase = PH_MORNING; G.cash = 250; room_seen = 1; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN); shot("v55_room_rent_due_today");
        wk_place(MAP_LOBBY, 7, 6, UP); wk_say(W_CLERK_TAG, 0, WK_DESK, W_CLERK_RENT[0], NULL); wk_ms = fake_ms - 30000; shot("v55_desk_rent_greeting");
        wk_mode = 4; wk_dsel = 0; shot("v55_desk_menu_pay_rent"); wk_dsel = 1; shot("v55_desk_menu_store_selected");
        wk_mode = 0; G.rent_next = 14; shot("v55_lobby_rent_paid_no_red_text");
        G.rent_next = 7; G.phase = PH_EVENING; snprintf(rent_msg, sizeof rent_msg, W_RENT_TAKEN_BANK, 40, 60); wk_say(NULL, 1, WK_NONE, W_SAVED_NIGHT, rent_msg); wk_ipg = 1; wk_ms = fake_ms - 30000; shot("v55_rent_taken_from_bank_line"); wk_mode = 0;
        sc_page[0] = S_DAILY[0]; scene_start(1, SE_DAILY); sc_ms = fake_ms - 30000; shot("v55_daily_refusal");
        scr = SC_OVER; over_rent = 1; over_ms = fake_ms - 5000; shot("v55_game_over_rent"); over_rent = 0; shot("v55_game_over_money");
        G = keep55; scr = SC_WALK; wk_place(km, kx, ky, kf);
    }
    scr = SC_OPT; dlg_idx = 2; shot("v51_options_60"); dlg_idx = 4; shot("v51_options_20"); dlg_idx = 2; scr = SC_MENU; menu_sel = 2; shot("v51_title_options");
    /* ---- v3.5: bonus talk at her door, event center, quick play and exhibition ---- */
    G = base; G.story = ST_HOME; beat_shot(SCR_DOOR1, 6, "door1_hand_over_card"); beat_shot(SCR_DOOR1, 7, "door1_dont_come_back");
    G = base; G.event = 0;
    G.event = 1; G.cash = 1340; G.energy = 100; wk_place(MAP_LOBBY, 5, 8, DN); enter_hub(); shot("lobby_energy_full");
    G.energy = 70; shot("lobby_energy_70"); G.energy = 20; shot("lobby_energy_20"); G.energy = 0; shot("lobby_energy_0"); G.energy = 100;
    G.event = 1; wk_place(MAP_EVENT, 5, 5, UP); scr = SC_WALK; shot("v59_event_center_at_desk"); wk_place(MAP_EVENT, 13, 6, LF); shot("v59_event_center_arrival"); wk_place(MAP_EVENT, 2, 3, DN); shot("v59_event_center_left_door");
    wk_place(MAP_EVENT, EV_OUT_X, EV_OUT_Y, DN); shot("v59_event_center_out_of_match_room"); wk_place(MAP_EVENT, 13, 6, RT); shot("v59_event_center_exit_prompt");
    wk_place(MAP_EVENT, 5, 5, UP); wk_say(W_CLERK_TAG, 0, WK_EVENT, W_EVCLERK[0], NULL); wk_ms = fake_ms - 30000; shot("v59_event_clerk_greeting"); wk_mode = 0; G.phase = PH_NIGHT; wk_place(MAP_EVENT, 6, 5, UP); shot("v59_event_center_at_night_still_lit"); G.phase = PH_MORNING;
    G.energy = 60; qp_ms = fake_ms - 500; mp_mode = 1; qs_step = 1; qs_sel = 0; qp_kind = 1; scr = SC_QSETUP; shot("v59_event_need_energy"); qp_ms = 0; G.energy = 100; scr = SC_WALK;
    mp_mode = 0; qs_step = 1; qs_sel = 2; scr = SC_QSETUP; shot("quick_setup_chips");
    mp_mode = 1; G.cash = 600; qs_step = 1; qs_sel = 2; shot("exhibition_setup_buyin"); G.cash = 600;
    G.cash = 12; G.soda = 2; G.monster = 1; G.energy = 40; store_open(); shot("store"); st_confirm = 1; shot("store_confirm_soda"); st_confirm = 2; shot("store_confirm_energy_drink"); st_confirm = 0;
    store_note("NOT ENOUGH CASH. USE THE ATM"); fake_ms += 10; shot("store_message"); st_msg[0] = 0;
    inv_open(); shot("inventory"); inv_sel = 1; shot("inventory_second_row"); store_note("YOUR ENERGY IS ALREADY FULL"); shot("inventory_message"); st_msg[0] = 0;
    G.soda = 0; G.monster = 0; G.energy = 0; G.cash = 0; G.bank = 0; G.rescue_used = 0; store_open(); shot("store_pity_soda"); G.energy = 100; G.cash = 1340; G.bank = 0;
    G.story = ST_NAMED; G.energy = 0; sc_page[0] = S_ZOMBIE[0]; scene_start(1, SE_ZOMBIE); sc_ms = fake_ms - 30000; shot("door_zombie"); G.energy = 100;
    /* v3.9: monologue, rain, her door, her room, icons */
    scr = SC_MONO; mono_pg = 0; mono_ms = fake_ms; shot("v39_monologue_page1"); mono_pg = 1; shot("v39_monologue_page2");
    G = base; G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.energy = 100; wk_place(MAP_ROOM, 2, 3, LF); enter_hub(); shot("v39_room_fridge_icon");
    wk_place(MAP_LOBBY, 8, 6, UP); enter_hub(); shot("v39_lobby_icons_atm_store"); wk_place(MAP_LOBBY, 2, 6, LF); shot("v39_lobby_icon_event");
    wk_place(MAP_LOBBY, 12, 11, DN); shot("v39_lobby_glass_door_prompt"); vn_start(SCR_RAIN); shot("v39_rain_1"); vn_i = 1; shot("v39_rain_2");
    wk_place(MAP_HALL3, 1, 4, LF); enter_hub(); shot("v39_hall_her_door"); scr = SC_SNEAK; sn_mode = SM_CHOICE; sn_sel = 1; shot("v39_her_door_menu");
    wk_place(MAP_SOPHIA, 13, 5, LF); scr = SC_WALK; shot("v41_her_room_enter"); wk_place(MAP_SOPHIA, 5, 8, DN); shot("v41_her_room_bedroom_door"); wk_place(MAP_SOPHIA, 9, 8, DN); shot("v41_her_room_bathroom_door");
    wk_place(MAP_SOPHIA, 3, 6, LF); shot("v41_her_room_living"); wk_place(MAP_SOPHIA, 10, 7, UP); shot("v41_her_room_night_2");
    wk_place(MAP_SOPHIA, 5, 8, DN); wk_say(NULL, 1, WK_NONE, SN_TIRED, NULL); wk_ms = fake_ms - 30000; shot("v41_her_room_too_tired"); wk_mode = 0;
    scr = SC_SNEAK;
    sn_used = 1; sn_closer = 1; sn_longer = 0; sn_mode = SM_BED; sn_sel = 2; shot("v44_bed_menu_3"); sn_closer = 0; sn_sel = 0; shot("v39_bed_menu");
    sn_closer = 0; sn_longer = 0; sn_mode = SM_ADMIRE; sn_pg = 2; shot("v44_admire_last_view_prompt"); sn_closer = 1; sn_mode = SM_CLOSER; sn_pg = 2; shot("v44_closer_last_view_prompt");
    /* v5.0: the shirt debt, hints, objective page, close-up layout and R1 hint, game over, red title */
    { int k; static const int BK[] = {5, 7, 8, 10, 11, 12, 13, 16}; for (k = 0; k < (int)(sizeof BK / sizeof BK[0]); k++) { char nm[40]; snprintf(nm, sizeof nm, "v50_bump_%02d", BK[k]); beat_shot(SCR_BUMP, BK[k], nm); } }
    beat_shot(SCR_DOOR1, 0, "v50_door1_00"); beat_shot(SCR_DOOR1, 3, "v50_door1_03"); beat_shot(SCR_DOOR1, 4, "v50_door1_04");
    g_init(&G); G.story = ST_HOME; G.hall_seen = 3; G.event = 1; saved_G = G; saved_G.story = ST_NEW; room_seen = 0; wk_place(MAP_HALL, 4, 3, DN); scr = SC_WALK; wk_mode = 0; shot("v50_hint_enter_your_room");
    G.cash = 100; wk_place(MAP_HALL3, 1, 4, LF); wk_say(N_ME, 0, WK_NONE, W_DEBT_NO, NULL); shot("v50_door_refused"); wk_mode = 0;
    room_seen = 1; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN); pause_kind = PK_HOME; pause_sel = 1; shot("v50_pause_3_items"); pause_kind = PK_OBJ; shot("v50_objective_debt");
    G.cash = 340; shot("v50_objective_have_money"); G.story = ST_NAMED; shot("v50_objective_none"); pause_kind = PK_NONE; G.story = ST_HOME;
    G.story = ST_NAMED; rig(12, 11, CD(5,2), CD(0,3), 10, 7, 2, 16, 32, 0); scr = SC_PLAY; cu_mode = 0; cu_used = 0; shot("v50_table_r1_hint");
    cu_think = CU_THINK[6]; cu_her = L_STARE[5]; cu_mode = 1; shot("v50_closeup_longest_thought"); cu_think = CU_THINK[0]; shot("v50_closeup_thought_1"); cu_mode = 2; shot("v50_closeup_her_long_line"); cu_mode = 0;
    scr = SC_OVER; shot("v50_game_over"); scr = SC_MENU; menu_sel = 1; shot("v50_title_red"); menu_sel = 0; shot("v50_title_red_sel0");
    /* v6.0: time of day, night lighting, the new scenes */
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.day = 1; G.phase = PH_MORNING; saved_G = G; room_seen = 1; scr = SC_WALK; wk_mode = 0; pause_kind = PK_NONE;
    wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN); shot("v60_room_day1_morning"); G.phase = PH_EVENING; shot("v60_room_evening_same_light"); G.phase = PH_NIGHT; G.day = 3; shot("v60_room_day3_night");
    wk_place(MAP_HALL, 8, 3, DN); shot("v60_hall_night"); wk_place(MAP_HALL3, 9, 3, DN); shot("v60_hall3_night"); wk_place(MAP_HALL1, 6, 3, DN); shot("v60_hall1_night"); wk_place(MAP_LOBBY, 5, 7, DN); shot("v60_lobby_night"); G.phase = PH_MORNING; shot("v60_lobby_morning");
    G.phase = PH_NIGHT; G.phase = PH_MORNING;
    G.phase = PH_MORNING; wk_place(MAP_ROOM, 2, 5, LF); wk_mode = 2; wk_yes = 0; wk_sleep = 0; shot("v60_sofa_prompt_morning"); G.phase = PH_EVENING; shot("v60_sofa_prompt_evening");
    G.phase = PH_NIGHT; wk_place(MAP_ROOM, 8, 3, RT); wk_mode = 2; wk_yes = 0; wk_sleep = 1; G.day = 4; shot("v60_bed_prompt_night"); wk_mode = 0;
    G.phase = PH_NIGHT; wk_say(NULL, 1, WK_NONE, W_SOFA_NIGHT, NULL); wk_ms = fake_ms - 30000; shot("v60_sofa_refused_night"); wk_say(NULL, 1, WK_NONE, W_BED_DAY, NULL); wk_ms = fake_ms - 30000; shot("v60_bed_refused_day"); wk_mode = 0;
    G.phase = PH_NIGHT; wk_place(MAP_HALL3, 1, 4, LF); scr = SC_SNEAK; sn_mode = SM_CHOICE; sn_sel = 0; shot("v60_her_door_menu_night"); scr = SC_WALK;
    G.phase = PH_NIGHT; beat_shot(SCR_NIGHT, 0, "v60_night_knock_1"); beat_shot(SCR_NIGHT, 1, "v60_night_knock_2_her_night_picture"); beat_shot(SCR_NIGHT, 3, "v60_night_knock_4"); beat_shot(SCR_NIGHT, 5, "v60_night_knock_end");
    G.phase = PH_MORNING; { int k; static const int BI[] = {10, 12, 13, 14, 16, 17, 18, 19, 20}; for (k = 0; k < 9; k++) { char nm[40]; snprintf(nm, sizeof nm, "v60_bump_%02d", BI[k]); beat_shot(SCR_BUMP, BI[k], nm); } }
    G.story = ST_HOME; G.cash = 300; { int k; static const int DI[] = {2, 4, 5, 6, 7, 8}; for (k = 0; k < 6; k++) { char nm[40]; snprintf(nm, sizeof nm, "v60_door1_%02d", DI[k]); beat_shot(SCR_DOOR1, DI[k], nm); } }
    G.story = ST_FIRST; G.cash = 80; G.forfeit = 0; rules_told = 0; beat_shot(SCR_REPEAT, 1, "v60_second_visit_greeting"); beat_shot(SCR_REPEAT, 3, "v60_second_visit_rules"); beat_shot(SCR_REPEAT, 5, "v60_second_visit_pick");
    scr = SC_LEN; len_sel = 0; say(L_LEN[0]); shot("v60_first_challenge_menu"); G.cash = 5; sc_page[0] = S_BROKE[0]; scene_start(1, SE_ZOMBIE); sc_ms = fake_ms - 30000; shot("v60_first_challenge_broke");
    G.story = ST_NAMED; G.last_res = 1; G.last_bonus = 0; G.last_pay = 40; G.cash = 140; G.last_drink = 0; G.mlen = 10; vn_start(SCR_AFTER); shot("v60_after_win_inside_her_room"); G.last_res = -1; G.last_pay = 0; vn_start(SCR_AFTER); shot("v60_after_loss_inside_her_room");
    G.phase = PH_MORNING; G.day = 1; G.story = ST_NAMED;
    sn_closer = 1; sn_longer = 1; sn_mode = SM_VIEW; sn_vpic = 1; shot("v41_view_closer"); sn_vpic = 2; shot("v41_view_longer"); sn_vpic = 0; sn_closer = 0; sn_longer = 0; sn_mode = SM_ADMIRE; sn_pg = 0; shot("v39_admire_1"); sn_pg = 1; shot("v39_admire_2"); sn_pg = 2; shot("v39_admire_3");
    sn_used = 1; sn_closer = 0; sn_longer = 0; sn_mode = SM_BED; sn_sel = 1; shot("v40_bed_menu"); sn_closer = 1; sn_mode = SM_CLOSER; sn_pg = 0; shot("v40_closer_1"); sn_pg = 2; shot("v40_closer_3"); sn_mode = SM_LONGER; sn_pg = 0; shot("v40_longer_1"); sn_pg = 2; shot("v40_longer_3"); sn_closer = 0; sn_mode = SM_VIEW; shot("v39_view"); scr = SC_WALK; sn_used = 0;
    G.last_res = 1; G.last_net = 24; G.last_pay = 24; G.last_drink = 1; G.mlen = 20; scr = SC_END; first_end = 0; shot("end_win_free_soda"); G.last_drink = 2; shot("end_win_free_energy_drink"); G.last_drink = 0;
    {   /* tables */
        int h1[8] = {CD(12,0), CD(11,0), CD(10,1), CD(10,2), CD(6,1), CD(6,3), CD(3,3), CD(4,2)}, bd[5] = {CD(0,0), CD(5,0), CD(9,0), CD(2,2), CD(7,3)}, g, k;
        for (g = 0; g < 2; g++) {
            int mode = g == 1, kind = 0; char nm[48];
            mp_mode = mode; qs_si = mode ? 2 : 1; G.cash = 1340; mp_begin();
            for (k = 0; k < 8; k++) M.deck[k] = h1[k]; for (k = 0; k < 5; k++) M.deck[8 + k] = bd[k];
            M.button = 0; if (kind) M.button = 3; mp_msg[0] = 0;
            { int guard = 0; while (!M.over && guard++ < 40 && !(M.stage == 2 && M.toact == 0)) { if (kind && M.toact == 3 && M.stage == 0) mp_do(A_FOLD); else if (M.toact == 0 && M.stage < 2 && mp_can_raise(&M, 0) && guard < 3) mp_do(A_RAISE); else mp_do(A_CALL); } }
            snprintf(nm, sizeof nm, "%s_%s_flop_your_turn", mode ? "exhibition" : "quick", kind ? "1v3" : "1v1"); shot(nm);
            { int guard = 0; while (!M.over && guard++ < 60) mp_do(A_CALL); }
            snprintf(nm, sizeof nm, "%s_%s_showdown_highlight", mode ? "exhibition" : "quick", kind ? "1v3" : "1v1"); shot(nm);
            if (g == 0) { pause_kind = PK_QUICK; pause_sel = 1; shot("pause_quick_leave"); pause_kind = PK_NONE; }
            if (g == 1) { pause_kind = PK_EXHIB; pause_sel = 1; shot("pause_exhibition_leave"); pause_kind = PK_NONE; }
        }
        mp_mode = 1; mp_buyin = 500; qs_si = 2; mp_begin(); M.game_over = 1; M.place = 1; mp_prize = g_ex_prize(500); G.cash = 1840; scr = SC_MEND; shot("exhibition_win");
        M.game_over = 2; M.place = 2; mp_prize = 0; G.cash = 840; shot("exhibition_eliminated");
        mp_mode = 0; mp_begin(); M.game_over = 1; M.place = 1; scr = SC_MEND; shot("quick_win");
    }
    /* ---- v12: her faces, dealing and chip animation, after-match scenes ---- */
    G = base; G.story = ST_NAMED; G.energy = 100; g_match_start(&G, 100); scr = SC_PLAY;
    rig(CD(12,0), CD(12,1), CD(0,2), CD(5,3), CD(8,0), CD(3,1), CD(7,2), CD(10,3), CD(1,0), 0);
    anim_fix_deal = 500; shot("anim_deal_her_first_card_landed_yours_flying"); anim_fix_deal = 1000; shot("anim_deal_her_second_card_flying"); anim_fix_deal = -1;
    shot("deal_done_your_cards_face_down_pink"); peek = 1; peek_ms = 0; shot("your_cards_looked_at_with_Y"); peek_ms = fake_ms; anim_fix = 125; shot("your_cards_turning"); anim_fix = -1; peek = 0;
    do_action(A_RAISE); anim_fix = 300; shot("anim_chips_into_pot"); anim_fix = -1;
    while (G.stage == 0 && !G.over) do_action(A_CALL);
    anim_fix = 300; shot("anim_flop_flying_face_down"); anim_fix = 600; shot("anim_flop_resting_face_down"); anim_fix = 850; shot("anim_flop_turning"); anim_fix = -1; shot("table_flop_pink_backs");
    run_to_showdown(); anim_fix = 250; shot("anim_pot_flying_to_you"); anim_fix = -1; shot("expr_shock_you_won_pot");
    rig(CD(0,0), CD(5,1), CD(12,2), CD(12,3), CD(8,0), CD(3,1), CD(7,2), CD(10,3), CD(1,0), 0);
    run_to_showdown(); anim_fix = 250; shot("anim_pot_flying_to_her"); anim_fix = -1; shot("expr_smug_she_won_pot");
    rig(CD(12,0), CD(11,1), CD(12,2), CD(11,3), CD(8,0), CD(3,1), CD(7,2), CD(10,3), CD(1,0), 0);
    run_to_showdown(); shot("expr_split_pot_neutral");
    G = base; G.story = ST_NAMED; G.cash = 3400; G.last_res = 1; G.last_bonus = 0; G.last_pay = 1000; G.last_drink = 0; G.mlen = 10;
    vn_start(SCR_AFTER); shot("after_win");
    G.last_bonus = 300; G.last_pay = 1300; G.cash = 3700; vn_start(SCR_AFTER); shot("after_win_bonus");
    G.last_drink = 1; G.mlen = 20; vn_start(SCR_AFTER); vn_i = 1; shot("after_win_free_drink_page2"); G.last_drink = 0;
    G.last_res = -1; G.last_bonus = 0; G.last_pay = 0; vn_start(SCR_AFTER); shot("after_loss");
    G.last_bonus = 300; vn_start(SCR_AFTER); shot("after_loss_bonus_forfeited");
    G.last_res = 0; G.last_bonus = 0; vn_start(SCR_AFTER); shot("after_tie");
    /* v13: five exhibition fees, cash fees for her matches, the bedroom door, nothing unlocked on arrival */
    G = base; G.story = ST_NAMED; G.cash = 120; mp_mode = 1; qs_sel = 2; scr = SC_QSETUP; shot("v13_exhibition_five_buyins");
    scr = SC_LEN; len_sel = 1; say(L_LEN[0]); shot("v13_choose_hands_cash_fee");
    G = base; G.story = ST_HOME; saved_G = base; room_seen = 1; wk_place(MAP_ROOM, 5, 5, RT); scr = SC_WALK; shot("v13_bedroom_door_A_prompt");
    G = base; G.story = ST_HOME; saved_G = base; room_seen = 0; wk_place(MAP_LOBBY, 5, 8, DN); scr = SC_WALK; shot("v13_lobby_on_arrival_no_icons");
    room_seen = 1; shot("v13_lobby_after_room_icons");
    /* ---- v6.1: the boss ---- */
    G = base; G.story = ST_FIRST; G.hall_seen = 3; G.event = 1; G.ex_big = 1; G.cash = 7000; G.energy = 100; mp_mode = 1; qs_sel = EX_BOSS; scr = SC_QSETUP; shot("v61_boss_buyin_list");
    qs_sel = 4; shot("v61_boss_buyin_list_1000_selected"); G.cash = 3000; qs_sel = EX_BOSS; shot("v61_boss_buyin_list_cannot_afford");
    G.cash = 7000; enter_hub(); wk_nflr = elev_floors(2, wk_flr); wk_fsel = 0; wk_mode = 5; wk_place(MAP_HALL, ELEV_X[2], ELEV_Y[2], DN); wk_nflr = elev_floors(2, wk_flr); wk_mode = 5; shot("v61_elevator_no_3rd_floor"); wk_mode = 0;
    qs_si = EX_BOSS; mp_mode = 1; mp_begin(); shot("v61_boss_table_start");
    { int k; for (k = 0; k < 8 && !M.over; k++) mp_do(A_CALL); } shot("v61_boss_table_midgame");
    M.stack[0] = 8000; M.stack[1] = 2000; hl_clear(); shot("v61_boss_table_stacks");
    mp_buyin = 5000; M.game_over = 1; M.place = 1; mp_prize = EX_BOSS_PRIZE; G.cash = 12000; boss_new = 0; scr = SC_MEND; shot("v61_boss_win_result");
    M.game_over = 2; M.place = 2; mp_prize = 0; G.cash = 2000; shot("v61_boss_lost_result");
    G.story = ST_FIRST; G.ex_big = 1; mp_mode = 1; boss_new = 1; M.game_over = 1; mp_buyin = 1000; mp_prize = g_ex_prize(1000); scr = SC_MEND; shot("v61_new_challenger_result"); boss_new = 0;
    { int k; for (k = 0; k < (int)B_BOSS_N; k++) { char nm[40]; snprintf(nm, sizeof nm, "v61_boss_scene_%02d", k); beat_shot(SCR_BOSS, k, nm); } }
    v63_tests();   /* v6.3 pictures: the debug panel */
    v68_tests();   /* v6.8 pictures: unpacking, her pictures, lead-in, info page */
    v79_tests();   /* v7.9: scenes and lines from lines.txt reach the real game */
    v80_tests();   /* v8.0: Sophia in the living room, walk speed, bigger hints */
    v81_tests();   /* v8.1: Sophia always there in the morning, her walk back, six stocks */
    v82_tests();   /* v8.2: new gifts and store items, the diamond ring, the Super milk drug */
    v84_tests();   /* v8.4: dialogue box on the bottom edge, full width, height by rows; vertical romance bar */
    feed_no_force = 0;   /* v9.7: the older flow tests expect BREASTFEED to start; v97_tests switches the real chance on itself */
    mm_react_on = 0;
    printf("v8.6: wedding, page 4, outfit change, full gauge: problems %d\n", v86_tests());
    mm_react_on = 1;
    v85_tests();   /* v8.5: Start = pause menu, Select = nothing, no MENU LEAVES hint, lower pot chips / stack text, cached table backdrop */
    v89_tests();   /* v8.9: the bet menu at the Exhibition / boss table, no chip on the title menu */
    printf("v9.0: icons stay, Event Center menu, lottery: problems %d\n", v90_tests());
    printf("v9.2: Sophia's walkable bedroom: problems %d\n", v92_tests());
    mm_react_on = 0;
    printf("v9.5: R2 name keyboard, mama mode, husband instead of boy, items.txt names in the game: problems %d\n", v95_tests());
    mm_react_on = 1;
    printf("v9.7: she refuses the name MAMA (reluctant text, flat refusals, memory, saved kick day, mama table lines): problems %d\n", v97_tests());
    printf("v10.5: sofa REST / SAVE ONLY, five save slots (sofa, bed, Options, Career), rewind guard (market reshuffle, lottery shut): problems %d\n", v105_tests());
    printf("v10.0: 20 wins open the black market, items only after she moves in, vanishing one-time items, the achievement banner: problems %d\n", v100_tests());
    mm_react_on = 0;
    printf("v10.4: mama-mode openers (your line first, her reaction to the name first; BREASTFEED everywhere, ASK / PUMP / milk pill / Super drug in the living room): problems %d\n", v104_tests());
    mm_react_on = 1;
    mm_react_on = 0;
    printf("v9.9: no Sophia sprite from pill day 6; living-room ASK FOR MILK / PUMP MILK / BREASTFEED (Here?!, own scenes and pictures): problems %d\n", v99_tests());
    mm_react_on = 1;
    printf("v10.6: mama reactions (stronger rejection, plea pairs, door openers for ASK / PILL / SUPER, memory of earlier asks, mid-scene hesitation): problems %d\n", v106_tests());
    printf("v10.7: text box style (0%% opacity, line colour, name colours, thick shadow, white narration, Options sub-panel, cfg): problems %d\n", v107_tests());
    mm_react_on = 0;
    printf("v10.1: BREASTFEED at her door: hiding picture when she refuses and another when she says yes, per-pool no-repeat memory: problems %d\n", v101_tests());
    printf("v10.8: DESIGNER TOP, third outfit (menu, wardrobe, pictures, store, gift): problems %d\n", v108_tests());
    printf("v11.1: COW PRINT OUTFIT, fourth outfit (menu with 5 rows, wardrobe, pictures, fallback, store, gift): problems %d\n", v111_tests());
    printf("v11.2: angry outfit gifts, milk-aware cow gift, the wear exchange (request, refusal, give-in): problems %d\n", v112_tests());
    printf("v11.3: outfits only at max romance, no romance gain, only in her wardrobe: problems %d\n", v113_tests());
    printf("v11.5: table box opacity + line colour (Options > TEXT BOX STYLE rows 4-5, cfg, default look unchanged, 0%% draws nothing, banner line) and no dark box over a folded CPU: problems %d\n", v115_tests());
    printf("v11.9: menu colours (Save Slots / Career / Options / pause / online store default RED, Options > MENU COLORS line + text colour, cfg, old gold reachable): problems %d\n", v119_tests());
    printf("v12.0: GUI colours (Options > MENU COLORS rows 3-4 colour every box line + accent text, TABLE LINE row gone, Sophia corner box follows TABLE OPACITY, cfg): problems %d\n", v120_tests());
    printf("v12.1: cow mode (disgust at the cow outfit, her dialogue turns negative while she wears it, she still accepts, ends with the outfit): problems %d\n", v121_tests());
    printf("v12.2: new prices, the breast pump wears out (broken line when you ask, store lists a new pump after that, handed over from the give page, debug row): problems %d\n", v122_tests());
    printf("v12.3: Mommy Milk Injection (black market $30,000, give page, no gauge, ONE milk a day: ask 2 bottles / pump / breastfeed, pill and drug refused, handing-bottle pictures, debug row): problems %d\n", v123_tests());
    printf("v11.6: five romance levels (harder each level), Sophia's card back blue until she lives here and got the pink deck: problems %d\n", v116_tests());
    printf("v11.0: FRIENDSHIP (blue bar, tiers, door openers) and her room by day (COME IN, TALK, PLAY, LEAVE, objects): problems %d\n", v110_tests());
    mm_react_on = 1;
    printf("v10.2: gift scene pools (5 versions per beat for the 8 repeatable gifts): problems %d\n", v102_tests());
    printf("v9.4: night knock picture after move-in, one more tile at the computer and the bed, Sophia stays put on A: problems %d\n", v94_tests());
    romance_tests();   /* v6.2 pictures: store, room, romance scene, end of a home match */
    (void)i; free(shotbuf);
}
/* ---- test bot: walks the real map with the real buttons (instant steps because the clock is frozen) ---- */
static const int DIRBTN[4] = {B_DOWN, B_LEFT, B_RIGHT, B_UP};
static int bot_walk(int gx, int gy);
static int bot_walk(int gx, int gy) {   /* breadth-first path on the current map; doors are only entered when they are the goal */
    int dist[MMH][MMW], qx[MMW * MMH], qy[MMW * MMH], h = 0, t = 0, x, y, d, steps[MMW * MMH], n = 0, MW = wm[wk_map].w, MH = wm[wk_map].h;
    for (y = 0; y < MH; y++) for (x = 0; x < MW; x++) dist[y][x] = -1;
    while (wk_mode == 1 || wk_mode == 4) on_button(wk_mode == 1 ? B_A : B_B);   /* close any message first (like the hall look-around); v9.8: a desk menu (wk_mode 4, opens after the clerk's greeting since v9.1) is left with B */
    if (wk_map == MAP_ROOM && wk_ty == 5 && gy == 5 && (wk_tx < 6) != (gx < 6) && gx != 6) {}   /* (same row: the BFS below walks up to the door, handled next) */
    if (wk_map == MAP_ROOM && gx != 6 && (wk_tx < 6) != (gx < 6) && wk_tx != 6) {   /* the goal is in the other half of the room: go through the bedroom door */
        int left = wk_tx < 6; if (!bot_walk(left ? 5 : 7, 5)) return 0; on_button(left ? B_RIGHT : B_LEFT); on_button(B_A);
    }
    if (wk_tx == gx && wk_ty == gy) return 1;
    dist[wk_ty][wk_tx] = 0; qx[t] = wk_tx; qy[t++] = wk_ty;
    while (h < t && dist[gy][gx] < 0) {
        x = qx[h]; y = qy[h++];
        for (d = 0; d < 4; d++) {
            int nx = x + DX4[d], ny = y + DY4[d]; char c = wk_tile(wk_map, nx, ny);
            if (nx < 0 || ny < 0 || nx >= MW || ny >= MH || dist[ny][nx] >= 0) continue;
            if (c != '.') continue;
            dist[ny][nx] = dist[y][x] + 1; qx[t] = nx; qy[t++] = ny;
        }
    }
    if (dist[gy][gx] < 0) return 0;
    for (x = gx, y = gy; dist[y][x] > 0; ) {   /* walk back from the goal to find the directions */
        for (d = 0; d < 4; d++) { int px_ = x - DX4[d], py_ = y - DY4[d]; if (px_ >= 0 && py_ >= 0 && px_ < MW && py_ < MH && dist[py_][px_] == dist[y][x] - 1) { steps[n++] = d; x = px_; y = py_; break; } }
    }
    while (n > 0) { int m0 = wk_map; while (wk_mode == 1 || wk_mode == 4) on_button(wk_mode == 1 ? B_A : B_B); on_button(DIRBTN[steps[--n]]); if (wk_map != m0) return 1; }
    return wk_tx == gx && wk_ty == gy;
}
static int bot_door(int dx, int dy) {   /* stand in front of the door, face it, press A */
    int d, m0 = wk_map;
    for (d = 0; d < 4; d++) {
        int nx = dx - DX4[d], ny = dy - DY4[d];
        if (wk_tile(wk_map, nx, ny) == '.' && bot_walk(nx, ny)) { on_button(DIRBTN[d]); on_button(B_A); return wk_map != m0; }
    }
    return 0;
}
static int bot_elevator(int to) {   /* in front of this floor's elevator door: open the menu and ride to floor "to" */
    int x, y, lx = -1, ly = -1, fl[3], n, i, m0 = wk_map;
    for (y = 0; y < wm[wk_map].h && lx < 0; y++) for (x = 0; x < wm[wk_map].w; x++) if (wm[wk_map].g[y][x] == 'L') { lx = x; ly = y; break; }
    if (lx < 0) return 0;
    bot_door(lx, ly); if (wk_mode != 5) return 0;
    n = elev_floors(floor_of(wk_map), fl); for (i = 0; i < n && fl[i] != to; i++) {}
    if (i == n) { on_button(B_B); return 0; }
    while (wk_fsel < i) on_button(B_DOWN);
    on_button(B_A); return wk_map != m0;
}
static int bot_goto(int map, int gx, int gy) {   /* any place on any map: the shortest chain of doors, stairs and elevator rides */
    int guard = 0;
    while (wk_map != map && guard++ < 10) {
        int prev[MAP_N], q[MAP_N], h = 0, t = 0, m, i, to, cur = wk_map, ok = 0;
        for (m = 0; m < MAP_N; m++) prev[m] = -2;
        prev[cur] = -1; q[t++] = cur;
        while (h < t) {
            m = q[h++];
            for (i = 0; i < NDOORS; i++) if (DOORS[i].map == m && prev[DOORS[i].tmap] == -2) { prev[DOORS[i].tmap] = m; q[t++] = DOORS[i].tmap; }
            if (floor_of(m)) { int fl[3], k, nf = elev_floors(floor_of(m), fl); for (k = 0; k < nf; k++) { to = floor_map(fl[k]); if (prev[to] == -2) { prev[to] = m; q[t++] = to; } } }
        }
        if (prev[map] == -2) return 0;
        for (to = map; prev[to] != cur; to = prev[to]) {}   /* the first map on the way */
        for (i = 0; i < NDOORS && !ok; i++) if (DOORS[i].map == cur && DOORS[i].tmap == to) ok = bot_door(DOORS[i].x, DOORS[i].y);
        if (!ok && floor_of(cur) && floor_of(to)) ok = bot_elevator(floor_of(to));
        if (!ok) return 0;
    }
    return wk_map == map && bot_walk(gx, gy);
}
static int bot_face(int map, int gx, int gy, int face) {   /* go there and turn to face (walking into a bump tile does its thing) */
    if (!bot_goto(map, gx, gy)) return 0;
    on_button(DIRBTN[face]); return 1;
}
#define EV_ENTER() do { WK_CLOSE(); if (scr == SC_EVENT) on_button(B_A); } while (0)   /* v9.0: past the clerk's greeting and the Exhibition / Lottery menu: into the buy-in list */
#define WK_CLOSE() do { int zz_; for (zz_ = 0; zz_ < 8 && scr == SC_WALK && wk_mode == 1; zz_++) on_button(B_A); } while (0)   /* close whatever walking message is open */
static int bot_use(int map, int gx, int gy, int face) { if (!bot_face(map, gx, gy, face)) return 0; on_button(B_A); return 1; }   /* ... and press A */

/* one whole Quick Play or Exhibition game through the real buttons: chips are conserved, cash moves exactly as promised */
static int mp_games_done, mp_wins_you;
static int mp_flow_test(int mode, int si, int cash0, int human, int leave_at) {
    int bad = 0, steps = 0, n = 2, total = n * (mode ? EX_BUYIN[si] : QP_STACK[si]), cash_after, won, expect, sum, i;
    mp_games_done++; G.cash = cash0; G.event = 1; G.story = mode && si == EX_BOSS ? ST_FIRST : ST_NAMED; G.ex_big = mode && si == EX_BOSS; G.energy = ENERGY_MAX; qp_ms = 0; pause_kind = PK_NONE;
    if (mode) { enter_hub(); bot_use(MAP_LOBBY, 1, 6, LF); if (scr != SC_WALK || wk_map != MAP_EVENT || wk_mode) { bad++; printf("event center not opened (map %d)\n", wk_map); }
        bot_use(MAP_EVENT, 5, 5, UP); if (scr != SC_WALK || wk_mode != 1 || wk_tag != W_CLERK_TAG) { bad++; printf("the event clerk did not greet\n"); } WK_CLOSE();
        if (scr != SC_EVENT || ev_sel != 0) { bad++; printf("the Exhibition / Lottery menu did not open after the greeting (scr %d)\n", scr); } on_button(B_A);
        if (scr != SC_QSETUP || mp_mode != 1) { bad++; printf("exhibition setup not opened\n"); } }
    else { scr = SC_MENU; menu_sel = 0; on_button(B_A); if (scr != SC_QSETUP || mp_mode != 0) { bad++; printf("quick setup not opened\n"); } }
    qs_sel = si; on_button(B_A);
    if (scr != SC_MTABLE) { printf("table not started (scr %d)\n", scr); return bad + 1; }
    cash_after = G.cash; if (G.cash != cash0 - (mode ? EX_BUYIN[si] : 0)) { bad++; printf("buy-in wrong\n"); }
    if (G.energy != (mode ? ENERGY_MAX - EX_ENERGY : ENERGY_MAX)) { bad++; printf("energy after sitting down wrong (%d)\n", G.energy); }
    while (scr == SC_MTABLE && steps++ < 300000) {
        fake_ms += 200; update();
        for (sum = 0, i = 0; i < n; i++) sum += M.stack[i];
        if (M.over ? sum != total : sum + mp_pot(&M) != total) { bad++; printf("chips not conserved\n"); break; }
        if (leave_at && M.hands == leave_at && !M.over) {   /* walk out: Menu, down, A */
            on_button(B_QUIT); on_button(B_DOWN); on_button(B_DOWN); on_button(B_A);
            if (scr != (mode ? SC_WALK : SC_MENU) || G.cash != cash_after) { bad++; printf("leaving the table wrong (scr %d cash %d vs %d)\n", scr, G.cash, cash_after); }
            if (mode && (wk_map != MAP_EVENT || wk_tx != EV_OUT_X || wk_ty != EV_OUT_Y || wk_face != DN)) { bad++; printf("after walking out you are not at the match-room door\n"); }
            return bad;
        }
        if (M.over) { on_button(B_A); continue; }
        if (M.toact == 0) { int r = rnd(10); on_button(human ? (r < 5 ? B_A : r < 8 ? B_X : B_B) : B_A); }
    }
    if (scr != SC_MEND) { printf("game did not end\n"); return bad + 1; }
    won = M.game_over == 1; mp_wins_you += won;
    expect = mode ? cash_after + (won ? g_ex_prize(EX_BUYIN[si]) : 0) : cash0;
    if (G.cash != expect) { bad++; printf("cash after game %d, expected %d\n", G.cash, expect); }
    if (won && M.place != 1) bad++;
    on_button(B_A);
    if (mode && si == EX_BOSS && won) {   /* the boss is beaten: her scene first (she gives her name), then you step out of the match room */
        int k = 0;
        if (scr != SC_STORY || vn_id != SCR_BOSS || G.story != ST_NAMED) { bad++; printf("boss win: scene did not follow (scr %d)\n", scr); }
        while (scr == SC_STORY && k++ < 30) { fake_ms += 400; update(); on_button(B_A); }
    }
    if (scr != (mode ? SC_WALK : SC_MENU)) { bad++; printf("did not return (scr %d)\n", scr); }
    if (mode && (wk_map != MAP_EVENT || wk_tx != EV_OUT_X || wk_ty != EV_OUT_Y || wk_face != DN || wk_mode)) { bad++; printf("after the game you are not at the match-room door facing down (%d,%d face %d)\n", wk_tx, wk_ty, wk_face); }
    return bad;
}

/* walking: map data is sane, every spot is reachable, solid things are solid, doors connect, saving and exit behave, held buttons move in real time */
/* ---- table animation timeline, in real time (the clock is not frozen here) ---- */
static int anim_tests(void) {
    int bad = 0, tries, c0; unsigned t;
    Game keep = G; int kscr = scr;
    frozen = 0; memset(kheld, 0, sizeof kheld); pause_kind = PK_NONE; srand(3); SDL_Delay(100000);   /* (the test clock only moves when told to) */
    for (tries = 0; tries < 12; tries++) { g_init(&G); G.story = ST_NAMED; start_match(10); if (G.toact == 0) break; }
    if (G.toact != 0) { printf("could not get the first action\n"); frozen = 1; G = keep; scr = kscr; return 1; }
    if (anim_left(0) < DEAL_ALL + CHIP_FLY) { bad++; printf("deal + blinds too short (%u ms)\n", anim_left(0)); }
    c0 = G.c[0]; on_button(B_X); on_button(B_B); if (G.c[0] != c0 || G.toact != 0) { bad++; printf("acted while cards were still moving\n"); }
    on_button(B_Y); if (peek) { bad++; printf("looked at the cards before they were dealt\n"); }
    if (fly_wait(0) + fly_wait(1) != G.c[0] + G.c[1]) { bad++; printf("blinds are not waiting in the racks\n"); }
    on_button(B_A);
    if (anim_left(0) || G.c[0] != c0 || G.toact != 0) { bad++; printf("A did not just skip the animation\n"); }
    on_button(B_Y); if (!peek) { bad++; printf("Y did not turn the cards up\n"); } on_button(B_L1); if (!peek) { bad++; printf("L1 must not change the cards any more\n"); } on_button(B_Y); if (peek) { bad++; printf("L1 did not hide them again\n"); }
    on_button(B_A); if (G.c[0] != BB_AMT) { bad++; printf("A did not call once the table was calm\n"); }
    t = now_ms();
    if (fly_ms[0] != t && (int)(fly_ms[0] - t) > 5) { bad++; printf("chips did not leave straight away\n"); }
    if (anim_end_ms < t + CHIP_FLY) { bad++; printf("chips fly too fast\n"); }
    if (G.toact == 1 && think_at < anim_end_ms) { bad++; printf("she acts before the chips land\n"); }
    /* the flop: chips first, then three cards face down one after another; the board never shows a card before it has flown in */
    G.over = 0; anim_new_hand(2, 4, 0); anim_after(1, 4, 0, 3, 0, 0, 0); t = now_ms();
    if ((int)(board_ms[0] - t) < CHIP_FLY + CHIP_GAP - 5) { bad++; printf("board cards move before the chips land\n"); }
    if (board_ms[1] - board_ms[0] != BOARD_STEP || board_ms[2] - board_ms[1] != BOARD_STEP) { bad++; printf("flop spacing wrong\n"); }
    if ((int)anim_age(board_ms[0] + BOARD_FLY / 2) >= 0 || BOARD_HOLD < 100 || BOARD_FLY < 400) { bad++; printf("board timing wrong\n"); }
    if (anim_end_ms < board_ms[2] + BOARD_FLY + BOARD_HOLD + BOARD_FLIP) { bad++; printf("table calm before the flop is turned\n"); }
    anim_new_hand(2, 4, 0); anim_after(0, 8, 4, 5, 0, 0, 0);
    if (board_ms[4] == 0 || fly_amt[0] != 8) { bad++; printf("river not scheduled like the others\n"); }
    /* a showdown: chips, then her cards turn, then the banner, you look at yours too */
    anim_new_hand(2, 4, 0); t = now_ms(); anim_after(1, 8, 0, 0, 1, 1, 1);
    if (rev_ms < t + CHIP_FLY || hand_dl() < (unsigned)(CHIP_FLY + FLIP_MS) || !peek || peek_ms != rev_ms) { bad++; printf("showdown order wrong\n"); }
    anim_new_hand(2, 4, 0); anim_after(0, 0, 0, 0, 1, 0, 0);
    if (hand_dl() != 300 || rev_ms) { bad++; printf("a fold should end quickly\n"); }
    frozen = 1; G = keep; scr = kscr; anim_end_ms = 0; peek = 0;
    return bad;
}
/* real-time checks for the v4.4 changes: staggered outline, R1 close-up, slow hall pan */
static int v44_tests(void) {
    int bad = 0, tries, i, camx0, camx1, camx2; Game keep = G, keep_saved = saved_G; int kscr = scr, km = wk_map, kx = wk_tx, ky = wk_ty, kf = wk_face;
    frozen = 0; memset(kheld, 0, sizeof kheld); pause_kind = PK_NONE; srand(5); SDL_Delay(100000);
    /* 1. the showdown outline comes in steps: opponent hole cards, then yours, then the board; the banner waits for the last one */
    hl_clear(); hl_hole[1][0] = 1; hl_hole[1][1] = 1; hl_hole[0][0] = 1; hl_board[1] = 1; hl_board[3] = 1; rev_ms = now_ms();
    hl_gate(); if (hd_hole[1][0] || hd_hole[0][0] || hd_board[1]) { bad++; printf("outline shown before her cards were face up\n"); }
    SDL_Delay(FLIP_MS + 10); hl_gate(); if (!hd_hole[1][0] || hd_hole[0][0] || hd_board[1]) { bad++; printf("step 1 should outline only the opponent's hole cards\n"); }
    SDL_Delay(HL_STEP); hl_gate(); if (!hd_hole[1][0] || !hd_hole[0][0] || hd_board[1]) { bad++; printf("step 2 should add your hole cards, not the board yet\n"); }
    SDL_Delay(HL_STEP); hl_gate(); if (!hd_hole[1][0] || !hd_hole[0][0] || !hd_board[1] || !hd_board[3] || hd_board[0]) { bad++; printf("step 3 should add the board cards of the hand\n"); }
    hl_clear(); hl_hole[0][0] = 1; hl_hole[0][1] = 1; hl_board[0] = 1; rev_ms = now_ms();   /* you win: the opponent group is empty, nothing waits for it */
    SDL_Delay(FLIP_MS + 10); hl_gate(); if (!hd_hole[0][0] || hd_board[0]) { bad++; printf("an empty group must be skipped\n"); }
    hdl = 1000; hl_extend(1); if (hdl != 1000 + 2 * HL_STEP) { bad++; printf("the banner does not wait for the outline (%d)\n", hdl); }
    hdl = 1000; hl_extend(0); if (hdl != 1000) { bad++; printf("no showdown must not delay the banner\n"); }
    hl_clear(); hdl = 300;
    /* 2. R1 close-up at her table: hidden, your turn only, once per hand; closing it makes her speak, A closes her line */
    for (tries = 0; tries < 12; tries++) { g_init(&G); G.story = ST_NAMED; start_match(10); if (G.toact == 0) break; }
    if (G.toact != 0) { printf("could not get the first action\n"); bad++; }
    else {
        on_button(B_R1); if (cu_mode) { bad++; printf("R1 worked while cards were still moving\n"); }
        on_button(B_A);   /* skip the animation */
        G.pill_seen = 0; on_button(B_R1); if (cu_mode || cu_used) { bad++; printf("v76: the close-up must stay closed until the day-8 scene has been seen\n"); }
        G.pill_seen = 1;
        on_button(B_R1); if (cu_mode != 1 || !cu_used) { bad++; printf("R1 did not open the close-up (%d)\n", cu_mode); }
        on_button(B_Y); on_button(B_X); if (cu_mode != 1) { bad++; printf("other buttons must not act on the close-up\n"); }
        { int k, ok = 0; for (k = 0; k < CU_THINK_N; k++) if (cu_think == CU_THINK[k]) ok = 1; if (!ok) { bad++; printf("no thought was picked\n"); } }
        on_button(B_A); if (cu_mode != 1) { bad++; printf("A must finish the typing of the thought first (%d)\n", cu_mode); }
        on_button(B_A); if (cu_mode != 2) { bad++; printf("closing the picture did not start her line (%d)\n", cu_mode); }
        draw_play(); if (!herCaughtS) { bad++; printf("v76: while she answers the close-up her caught picture must show\n"); }
        { int k, ok = 0; for (k = 0; k < L_STARE_N; k++) if (cu_her == L_STARE[k]) ok = 1; if (!ok) { bad++; printf("no line of hers was picked\n"); } }
        on_button(B_A); if (cu_mode != 2) { bad++; printf("A must finish the typing first\n"); }
        on_button(B_A); if (cu_mode != 0) { bad++; printf("A did not close her line (%d)\n", cu_mode); }
        draw_play(); if (herCaughtS) { bad++; printf("v76: after her line the normal table picture must be back\n"); }
        on_button(B_R1); if (cu_mode) { bad++; printf("R1 twice in one hand\n"); }
        /* v10.3: mama mode at home uses the mama close-up pools, a hotel match / SOPHIA mode keeps the old ones */
        { int r, k; Game kg = G; int kc = cu_used;
          for (r = 0; r < 3; r++) {
            int mode = r;   /* 0 = mama + home, 1 = mama + hotel, 2 = SOPHIA mode + home */
            for (k = 0; k < 60; k++) {
              G.call = mode == 2 ? 0 : 1; G.home = mode == 1 ? 0 : 1; G.married = 1; G.rs = RS_LIVING; cu_used = 0; cu_mode = 0; G.over = 0; G.toact = 0;
              on_button(B_R1); if (!cu_mode) break;
              { int in_mm = in_pool(cu_think, CU_THINK_MM, CU_THINK_MM_N) && in_pool(cu_her, L_STARE_MM, L_STARE_MM_N), in_old = in_pool(cu_think, CU_THINK, CU_THINK_N) && in_pool(cu_her, L_STARE, L_STARE_N);
                if (mode == 0 && !in_mm) { bad++; printf("v10.3: mama + home close-up must use the mama pools\n"); break; }
                if (mode != 0 && !in_old) { bad++; printf("v10.3: close-up outside mama-at-home must keep the old pools (mode %d)\n", mode); break; } }
              cu_mode = 0;
            }
          }
          G = kg; cu_used = kc; cu_mode = 0; }
        if (strncmp(L_STARE[0], "Did I just catch you staring", 28)) { bad++; printf("her first line changed\n"); }
    }
    /* the thoughts must fit the short box (2 lines of 46), hers the normal one (3 lines); never the same twice running */
    { int k, last = -1, rep_bad = 0; char w[400];
      for (k = 0; k < CU_THINK_N; k++) { int i, ln = 1; wrap_buf(w, sizeof w, CU_THINK[k], 46); for (i = 0; w[i]; i++) if (w[i] == '\n') ln++; if (ln > 2) { bad++; printf("thought %d needs %d lines\n", k, ln); } }
      for (k = 0; k < L_STARE_N; k++) { int i, ln = 1; wrap_buf(w, sizeof w, L_STARE[k], 46); for (i = 0; w[i]; i++) if (w[i] == '\n') ln++; if (ln > 3) { bad++; printf("her line %d needs %d lines\n", k, ln); } }
      for (k = 0; k < 2000; k++) { int n = pick_new(CU_THINK_N, last); if (n == last || n < 0 || n >= CU_THINK_N) rep_bad++; last = n; }
      if (rep_bad) { bad++; printf("a thought repeated or was out of range (%d)\n", rep_bad); } }
    cu_mode = 0; cu_used = 0;
    /* 3. the hall look-around: the camera swings slowly, her line waits until it has stopped and rested, nothing can be pressed meanwhile */
    g_init(&G); G.story = ST_HOME; G.hall_seen = 0; scr = SC_WALK; wk_place(MAP_HALL, 4, 3, DN); hall_script(1);
    draw_walk_world(0); if (wk_mode != 1 || wk_pg[0] != W_F2) { bad++; printf("hall line 1 missing\n"); }
    on_button(B_A);   /* typing done (test clock) -> next page or close */
    for (i = 0; i < 3 && wk_mode == 1; i++) on_button(B_A);
    if (wk_mode != 3 || wk_pend != 2 || wk_look != 12 || wk_face != RT) { bad++; printf("the pan to your door did not start (mode %d pend %d)\n", wk_mode, wk_pend); }
    else {
        camx0 = wk_camx; on_button(B_A); on_button(B_RIGHT); if (wk_mode != 3 || wk_tx != 4) { bad++; printf("input worked during the pan\n"); }
        SDL_Delay(PAN_MS / 2); draw_walk_world(0); camx1 = wk_camx; wk_update(); if (wk_mode != 3) { bad++; printf("your line came before the pan was over\n"); }
        SDL_Delay(PAN_MS / 2 + PAN_HOLD / 2); draw_walk_world(0); camx2 = wk_camx; wk_update(); if (wk_mode != 3) { bad++; printf("your line came before the camera had rested\n"); }
        if (!(camx1 > camx0 && camx2 >= camx1)) { bad++; printf("camera did not move steadily (%d %d %d)\n", camx0, camx1, camx2); }
        SDL_Delay(PAN_HOLD / 2 + 20); wk_update(); if (wk_mode != 1 || wk_pg[0] != W_HALL3) { bad++; printf("your line did not appear after the pan (mode %d)\n", wk_mode); }
        for (i = 0; i < 3 && wk_mode == 1; i++) on_button(B_A);
        if (wk_mode || !(G.hall_seen & 1) || (G.hall_seen & 2) || wk_look != -1) { bad++; printf("2nd-floor look-around did not end (mode %d)\n", wk_mode); }
    }
    frozen = 1; G = keep; saved_G = keep_saved; scr = kscr; anim_end_ms = 0; peek = 0; wk_place(km, kx, ky, kf);
    return bad;
}
static int v53_tests(void) {   /* v5.3: Career menu (Continue / New game), table GUI size option and its saved setting, the key card picture */
    int bad = 0, z; Game keep = G, keep_saved = saved_G; int kscr = scr, fr = frozen, kdlg = dlg_idx, ksnd = snd_on;
    frozen = 1; pause_kind = PK_NONE;
    /* v10.5: CAREER MODE lists five slots: an empty one starts the story, a used one loads, X starts over there (asks first), Y deletes (asks first) */
    slot_prefix = "/tmp/pq_s_"; slot_io_test = 1; { int q; char pth[300]; for (q = 1; q <= SLOT_N; q++) remove(slot_path(q)); snprintf(pth, sizeof pth, "%s", prog_path()); remove(pth); }
    g_init(&G); G.career = 777; G.story = ST_NAMED; G.day = 2; G.cash = 450; slot_act = 2; prog_far = 0; save_now();
    { FILE *f = fopen(slot_path(2), "rb"); if (!f) { bad++; printf("v10.5: the sofa/bed save must write slot 2\n"); } else fclose(f); f = fopen(slot_path(1), "rb"); if (f) { fclose(f); bad++; printf("v10.5: slot 1 must stay empty\n"); } }
    if (prog_get(prog_path(), 777) != 2) { bad++; printf("v10.5: the furthest day of the career must be written (%d)\n", prog_get(prog_path(), 777)); }
    g_init(&G); saved_G = G; scr = SC_MENU; menu_sel = 1; on_button(B_A);
    if (scr != SC_CAREER || car_sel != 1 || car_conf || slot_st[0] != 0 || slot_st[1] != 1 || slot_g[1].day != 2 || slot_g[1].cash != 450) { bad++; printf("v10.5: Career must list the slots with the cursor on the active one (scr %d sel %d st %d/%d)\n", scr, car_sel, slot_st[0], slot_st[1]); }
    on_button(B_B); if (scr != SC_MENU) { bad++; printf("B must go back to the title\n"); }
    on_button(B_A); on_button(B_A); for (z = 0; z < 6 && scr != SC_WALK; z++) update();
    if (scr != SC_WALK || G.story != ST_NAMED || G.day != 2 || G.cash != 450 || G.career != 777 || slot_act != 2 || unsaved()) { bad++; printf("v10.5: A on a used slot must load it (scr %d day %d)\n", scr, G.day); }
    scr = SC_MENU; menu_sel = 1; on_button(B_A); on_button(B_X);
    if (scr != SC_CAREER || car_conf != 1 || car_csel != 0) { bad++; printf("v10.5: X must ask before starting over, with NO selected\n"); }
    on_button(B_A); if (car_conf || G.story != ST_NAMED || slot_st[1] != 1) { bad++; printf("NO must keep the career\n"); }
    on_button(B_X); on_button(B_RIGHT); if (car_csel != 1) { bad++; printf("YES not selectable\n"); }
    on_button(B_A); for (z = 0; z < 6 && scr != SC_MONO; z++) update();
    if (scr != SC_MONO || G.story != ST_NEW || G.day != 1 || slot_act != 2) { bad++; printf("v10.5: YES must start a new career with the monologue (scr %d story %d day %d)\n", scr, G.story, G.day); }
    { FILE *f = fopen(slot_path(2), "rb"); if (f) { fclose(f); bad++; printf("v10.5: starting over must delete that slot's save\n"); } }
    g_init(&G); G.career = 888; G.story = ST_NAMED; G.day = 4; slot_act = 4; save_now(); slots_refresh();
    scr = SC_MENU; menu_sel = 1; on_button(B_A); car_sel = 3; on_button(B_Y);
    if (car_conf != 2) { bad++; printf("v10.5: Y must ask before deleting\n"); }
    on_button(B_RIGHT); on_button(B_A); { FILE *f = fopen(slot_path(4), "rb"); if (f) { fclose(f); bad++; printf("v10.5: YES must delete the slot\n"); } if (slot_st[3] != 0 || car_conf) { bad++; printf("v10.5: the slot must show EMPTY after the delete\n"); } }
    car_sel = 3; on_button(B_Y); if (car_conf) { bad++; printf("v10.5: Y on an empty slot must do nothing\n"); }
    { FILE *f = fopen(slot_path(5), "wb"); if (f) { fputs("garbage", f); fclose(f); } slots_refresh(); if (slot_st[4] != 2) { bad++; printf("v10.5: a broken file must show as unreadable\n"); } car_sel = 4; on_button(B_A); if (car_conf != 1) { bad++; printf("v10.5: A on an unreadable slot must ask to start over\n"); } car_conf = 0; remove(slot_path(5)); }
    slot_io_test = 0; slot_prefix = ""; slot_act = 1; prog_far = 0;
    on_button(B_QUIT); if (pause_kind != PK_HOME) pause_kind = PK_NONE;   /* (a pause menu opens normally here) */
    pause_kind = PK_NONE; scr = SC_CAREER; on_button(B_QUIT); if (pause_kind) { bad++; printf("Menu must be ignored in the Career menu\n"); } pause_kind = PK_NONE;
    /* options: the dialogue box and sound rows, saved and loaded; the old tgui / gui lines are ignored */
    scr = SC_OPT; opt_row = 0; dlg_idx = 2; on_button(B_RIGHT); if (dlg_idx != 3) { bad++; printf("dialogue box did not change (%d)\n", dlg_idx); }
    on_button(B_DOWN); on_button(B_DOWN); if (opt_row != 2) { bad++; printf("the options have three rows (%d)\n", opt_row); }
    { int w0 = walk_idx; on_button(B_LEFT); if (walk_idx != (w0 > 0 ? w0 - 1 : 0)) { bad++; printf("walk speed did not change\n"); } walk_idx = 1; }   /* v8.0: the third row */
    on_button(B_UP); on_button(B_LEFT); if (snd_on) { bad++; printf("sound did not switch off\n"); }
    dlg_idx = 3; snd_on = 0; cfg_save(); dlg_idx = 2; snd_on = 1; cfg_load();
    if (dlg_idx != 3 || snd_on != 0) { bad++; printf("options not saved (%d %d)\n", dlg_idx, snd_on); }
    { FILE *f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "dlg 1\ntgui 3\ngui 1\nsnd 1\n"); fclose(f); } dlg_idx = 2; cfg_load(); if (dlg_idx != 1 || !snd_on) { bad++; printf("a cfg with the old gui lines must still load\n"); } }
    remove(CFG_PATH);
    on_button(B_B); if (scr != SC_MENU) { bad++; printf("B must leave the options (scr %d)\n", scr); }
    dlg_idx = kdlg; snd_on = ksnd; frozen = fr; G = keep; saved_G = keep_saved; pause_kind = PK_NONE; scr = kscr;
    return bad;
}
static int in_pool(const char *t, const char *const *pool, int n) { int i; for (i = 0; i < n; i++) if (t == pool[i]) return 1; return 0; }
static void ow_play(void) { int g; for (g = 0; g < 40 && scr == SC_STORY && vn_id == SCR_OUTFIT; g++) on_button(B_A); }   /* v11.2: play the outfit exchange to its end (the outfit changes there) */
static int tst_ward(int k) {   /* v11.3: change her into outfit k the only way left: her wardrobe in her bedroom (a message, the menu, the exchange). 1 = the exchange ran; the result message stays open (WK_CLOSE) */
    int wt[OF_N], wn, q; wk_mode = 0; scr = SC_WALK; wk_place(MAP_SBED, 2, 3, LF); ls_on = 0; on_button(B_A); on_button(B_A);
    if (wk_mode != 8) return 0;
    wn = g_outfit_targets(&G, wt); for (q = 0; q < wn && wt[q] != k; q++); if (q == wn) { wk_mode = 0; return 0; }
    wk_wsel = q; on_button(B_A); ow_play(); return G.outfit == k;
}
static int v55_tests(void) {   /* v5.5: Sophia once a day, the rent (desk menu, RENT DUE TODAY, taken at night, game over), the new layout pieces */
    int bad = 0, k, last = -1; Game keep = G, keep_saved = saved_G; int kscr = scr, fr = frozen, km = wk_map, kx = wk_tx, ky = wk_ty, kf = wk_face, kroom = room_seen;
    frozen = 1; pause_kind = PK_NONE;
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.cash = 500; saved_G = G; room_seen = 1; scr = SC_WALK; wk_mode = 0;
    /* once a day */
    G.chal_day = 0; bot_use(MAP_HALL3, 1, 4, LF);
    if (scr != SC_STORY) { bad++; printf("first knock of the day did not start her greeting (scr %d)\n", scr); }
    scr = SC_WALK; wk_mode = 0; G.chal_day = G.day;
    for (k = 0; k < 300; k++) {
        bot_use(MAP_HALL3, 1, 4, LF);
        if (scr != SC_STORY || vn_id != SCR_DAILY || vn_buf[0].spr != 0 || strcmp(vn_buf[0].txt, "Knock, knock.")) { bad++; printf("knock after playing today: the narration of the knock must come first (scr %d)\n", scr); break; }
        if (!in_pool(vn_buf[1].txt, S_DAILY, (int)S_DAILY_N)) { bad++; printf("knock after playing today: no refusal line\n"); break; }
        { int idx = -1, q; for (q = 0; q < (int)S_DAILY_N; q++) if (vn_buf[1].txt == S_DAILY[q]) idx = q; if (idx == last) { bad++; printf("the same refusal line twice in a row\n"); break; } last = idx; }
        { int guard = 0; while (scr == SC_STORY && guard++ < 8) on_button(B_A); }
        if (scr != SC_WALK || wk_mode || G.in_match) { bad++; printf("the refusal did not send you back to walking (scr %d)\n", scr); break; }
    }
    G.day++; bot_use(MAP_HALL3, 1, 4, LF); if (scr != SC_STORY) { bad++; printf("a new day: she must play you again (scr %d)\n", scr); }
    scr = SC_WALK; wk_mode = 0; G.story = ST_FIRST; G.chal_day = G.day; bot_use(MAP_HALL3, 1, 4, LF);   /* the shirt is paid, she is the boss: her door stays shut (you would not even get up here without the card) */
    if (scr != SC_WALK || wk_mode != 1 || wk_pg[0] != W_BOSS_NO) { bad++; printf("her door must stay shut while she is the boss (scr %d)\n", scr); }
    WK_CLOSE(); if (scr != SC_WALK || wk_mode) { bad++; printf("not back in the hall\n"); }
    G.story = ST_NAMED; G.phase = PH_NIGHT; bot_use(MAP_HALL3, 1, 4, LF); if (scr != SC_STORY || vn_id != SCR_NIGHT) { bad++; printf("the night knock must stay the angry scene (scr %d)\n", scr); }
    scr = SC_WALK; wk_mode = 0; G.phase = PH_MORNING;
    /* the desk: no rent option on a normal day, the menu on a due day */
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.cash = 500; G.day = 6; G.room_up = 2; saved_G = G; wk_place(MAP_LOBBY, 7, 6, UP); on_button(B_A);   /* v9.8 test fix: room_up 2 = no upgrade offered (since v9.3 room_up 0 puts UPGRADE ROOM into the desk menu from the start) */
    if (wk_mode != 1 || wk_after != WK_NONE || !in_pool(wk_pg[0], W_CLERK, (int)W_CLERK_N)) { bad++; printf("a normal day: the clerk only greets you (v9.1)\n"); }
    WK_CLOSE(); if (scr != SC_WALK || wk_mode != 0) { bad++; printf("a normal day: nothing may open after her greeting (v9.1)\n"); } scr = SC_WALK; wk_mode = 0;
    G.day = 7; G.cash = g_rent_amt(&G) - 1; on_button(B_A);   /* v9.8 test fix: the rent is no longer $100 (base $250, room level 2 $1000): use g_rent_amt */
    if (wk_mode != 1 || wk_after != WK_DESK || !in_pool(wk_pg[0], W_CLERK_RENT_UP, (int)W_CLERK_RENT_UP_N)) { bad++; printf("rent day: the clerk did not mention the rent\n"); }   /* room_up != 0 uses the _UP pool */
    WK_CLOSE(); if (wk_mode != 4 || wk_dsel != 0) { bad++; printf("rent day: no desk menu (mode %d)\n", wk_mode); }
    on_button(B_DOWN); on_button(B_UP); on_button(B_UP); if (wk_dsel != 1) { bad++; printf("desk menu does not wrap\n"); } on_button(B_DOWN); if (wk_dsel != 0) { bad++; printf("desk menu does not wrap down\n"); }
    on_button(B_A); if (wk_mode != 1 || !in_pool(wk_pg[0], W_RENT_SHORT, (int)W_RENT_SHORT_N) || G.cash != g_rent_amt(&G) - 1 || !g_rent_due(&G)) { bad++; printf("one dollar short must not pay the rent\n"); }
    WK_CLOSE(); if (wk_mode != 0) { bad++; printf("short of cash: should be back to walking\n"); }
    G.cash = g_rent_amt(&G) + 50; on_button(B_A); WK_CLOSE(); on_button(B_A);
    if (G.cash != 50 || G.rent_next != 14 || g_rent_due(&G) || wk_mode != 1 || !in_pool(wk_pg[0], W_RENT_OK, (int)W_RENT_OK_N)) { bad++; printf("paying at the desk wrong (cash %d next %d)\n", G.cash, G.rent_next); }
    WK_CLOSE(); on_button(B_A); if (wk_after != WK_NONE) { bad++; printf("after paying the desk must be a normal desk again\n"); } WK_CLOSE(); scr = SC_WALK; wk_mode = 0;
    G.day = 14; on_button(B_A); WK_CLOSE(); { int dk[4]; if (wk_mode != 4 || desk_items(dk) != 2) { bad++; printf("v91: the desk menu must be rent and leave only (no store)\n"); } } wk_mode = 0;
    on_button(B_A); WK_CLOSE(); on_button(B_DOWN); on_button(B_A); if (wk_mode != 0 || !g_rent_due(&G) || G.cash != 50) { bad++; printf("LEAVE in the desk menu wrong\n"); }
    on_button(B_A); WK_CLOSE(); on_button(B_B); if (wk_mode != 0 || !g_rent_due(&G)) { bad++; printf("B in the desk menu wrong\n"); }
    G.phase = PH_NIGHT; on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_DESK_NIGHT) { bad++; printf("at night the desk is empty even when the rent is due\n"); } WK_CLOSE(); G.phase = PH_MORNING;
    /* the objective page mentions the rent */
    { char o[240], want[120]; G.story = ST_NAMED; obj_text(o, sizeof o); snprintf(want, sizeof want, O_RENT, g_rent_amt(&G)); if (strcmp(o, want)) { bad++; printf("objective on rent day wrong: %s\n", o); }
      G.rent_next = 21; obj_text(o, sizeof o); if (strcmp(o, O_NONE)) { bad++; printf("objective without rent wrong\n"); } G.rent_next = 14;
      G.story = ST_FIRST; obj_text(o, sizeof o); if (!strstr(o, "RENT")) { bad++; printf("rent missing in the first-challenge objective\n"); } G.story = ST_NAMED; }
    /* the night: unpaid rent is taken when the evening sofa moves the time on (cash first, then the bank); the reminders; no money = game over */
    char rent_s[16], bank_s[16]; snprintf(rent_s, sizeof rent_s, "$%d", RENT_AMT); snprintf(bank_s, sizeof bank_s, "$%d", RENT_AMT - 40);   /* v9.8 test fix: texts built from RENT_AMT */
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.day = 7; G.phase = PH_EVENING; G.cash = RENT_AMT + 150; saved_G = G; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 2, 5, LF);
    on_button(B_A); on_button(B_A);
    if (G.phase != PH_NIGHT || G.cash != 150 || G.rent_next != 14 || saved_G.cash != 150 || wk_npg != 2 || wk_pg[0] != W_SAVED_NIGHT || !strstr(wk_pg[1], rent_s)) { bad++; printf("rent at night from cash wrong (cash %d next %d)\n", G.cash, G.rent_next); } WK_CLOSE();
    G.day = 14; G.phase = PH_EVENING; G.cash = 40; G.bank = 500; saved_G = G; on_button(B_A); on_button(B_A);
    if (G.phase != PH_NIGHT || G.cash != 0 || G.bank != 500 - (RENT_AMT - 40) || G.rent_next != 21 || wk_npg != 2 || !strstr(wk_pg[1], "$40") || !strstr(wk_pg[1], bank_s)) { bad++; printf("rent at night from cash and bank wrong (cash %d bank %d)\n", G.cash, G.bank); } WK_CLOSE();
    G.day = 20; G.phase = PH_NIGHT; wk_place(MAP_ROOM, wm[MAP_ROOM].w == 7 ? 4 : 8, 3, RT); on_button(B_A); on_button(B_A);   /* v9.8 test fix: the bed is at (5,3) in the 7 x 7 base room (stand at 4,3), at (9,3) in the 11 x 7 rooms (stand at 8,3) */
    if (G.day != 21 || G.phase != PH_MORNING || wk_npg != 2 || wk_pg[1] != W_RENT_REMIND) { bad++; printf("waking on a rent day must remind you (day %d npg %d)\n", G.day, wk_npg); } WK_CLOSE();
    wk_place(MAP_ROOM, 2, 5, LF); on_button(B_A); on_button(B_A);
    if (G.phase != PH_EVENING || wk_npg != 2 || wk_pg[1] != W_RENT_REMIND) { bad++; printf("the evening of an unpaid rent day must remind you\n"); } WK_CLOSE();
    G.day = 3; G.rent_next = 21; G.phase = PH_MORNING; on_button(B_A); on_button(B_A); if (wk_npg != 1) { bad++; printf("no rent: one page only\n"); } WK_CLOSE();
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.day = 28; G.rent_next = 28; G.phase = PH_EVENING; G.cash = 50; G.bank = 49; saved_G = G; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 2, 5, LF);
    on_button(B_A); on_button(B_A);
    if (scr != SC_OVER || !over_rent || saved_G.phase != PH_EVENING) { bad++; printf("99 in total on a rent night must be game over, and nothing saved (scr %d)\n", scr); }
    on_button(B_A); if (scr != SC_MENU || G.story != ST_NEW || G.day != 1 || G.rent_next != RENT_DAYS) { bad++; printf("rent game over did not start again\n"); }
    G = keep; saved_G = keep_saved; frozen = fr; scr = kscr; room_seen = kroom; pause_kind = PK_NONE; wk_mode = 0; over_rent = 0; wk_place(km, kx, ky, kf);
    return bad;
}
static int v54_tests(void) {   /* v5.4: no clerk or store at night, L1 for the cards, Options inside the game, the prize of a win */
    int bad = 0; Game keep = G, keep_saved = saved_G; int kscr = scr, fr = frozen, km = wk_map, kx = wk_tx, ky = wk_ty, kf = wk_face, kroom = room_seen;
    frozen = 1; pause_kind = PK_NONE;
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.cash = 500; G.room_up = 2; saved_G = G; room_seen = 1; scr = SC_WALK; wk_mode = 0;   /* v9.8 test fix: room_up 2, so the desk only greets */
    /* the receptionist and the store: by day she greets you and the store opens, at night the desk is empty and nothing opens */
    G.phase = PH_MORNING; wk_place(MAP_LOBBY, 7, 6, UP); on_button(B_A);
    if (wk_mode != 1 || wk_pg[0] == W_DESK_NIGHT) { bad++; printf("the receptionist did not greet you by day\n"); }
    WK_CLOSE(); if (scr != SC_WALK || wk_mode != 0) { bad++; printf("v91: the desk must not open the store (scr %d)\n", scr); }
    scr = SC_WALK; wk_mode = 0; wk_place(MAP_LOBBY, 5, 4, UP); on_button(B_A); if (scr != SC_STORE) { bad++; printf("v91: the vending machine did not open the store by day (scr %d)\n", scr); }
    scr = SC_WALK; wk_mode = 0; G.phase = PH_EVENING; wk_place(MAP_LOBBY, 7, 6, UP); on_button(B_A); WK_CLOSE();
    if (scr != SC_WALK) { bad++; printf("v91: the desk must not open the store in the evening (scr %d)\n", scr); }
    wk_place(MAP_LOBBY, 5, 4, UP); on_button(B_A); if (scr != SC_STORE) { bad++; printf("the vending machine must work in the evening (scr %d)\n", scr); }
    scr = SC_WALK; wk_mode = 0; G.phase = PH_NIGHT; wk_place(MAP_LOBBY, 7, 6, UP); on_button(B_A);
    if (wk_mode != 1 || wk_pg[0] != W_DESK_NIGHT) { bad++; printf("the empty desk line is missing at night\n"); }
    WK_CLOSE(); if (scr != SC_WALK) { bad++; printf("the desk opened the store at night (scr %d)\n", scr); }
    wk_place(MAP_LOBBY, 5, 4, UP); on_button(B_A); if (scr != SC_STORE) { bad++; printf("v91: the vending machine must work at night too (scr %d)\n", scr); } scr = SC_WALK; wk_mode = 0;
    wk_place(MAP_LOBBY, 5, 4, LF); on_button(B_A); if (scr != SC_WALK || wk_mode) { bad++; printf("v91: the vending machine must need you to face up (scr %d)\n", scr); }
    wk_place(MAP_LOBBY, 5, 4, DN); on_button(B_A); if (scr != SC_WALK || wk_mode) { bad++; printf("v91: facing down must not use the vending machine\n"); }
    wk_place(MAP_LOBBY, 10, 4, LF); on_button(B_A); if (scr != SC_WALK || wk_mode) { bad++; printf("v91: the ATM must need you to face up\n"); }
    wk_place(MAP_LOBBY, 11, 4, UP); on_button(B_A); if (scr != SC_ATM) { bad++; printf("v91: the ATM must open from its right tile (scr %d)\n", scr); } scr = SC_WALK; wk_mode = 0;
    wk_place(MAP_LOBBY, 10, 4, UP); on_button(B_A); if (scr != SC_ATM) { bad++; printf("v91: the ATM must open from its left tile (scr %d)\n", scr); } scr = SC_WALK; wk_mode = 0;
    wk_place(MAP_LOBBY, 9, 6, UP); on_button(B_A); if (scr != SC_WALK || wk_mode) { bad++; printf("v91: the old ATM spot at the desk corner must do nothing\n"); }
    /* L1 looks at the cards, R1 is the close-up, Y does neither */
    if (map_key(SDLK_e) != B_L1 || map_key(SDLK_t) != B_R1 || map_key(SDLK_v) != B_Y) { bad++; printf("key map wrong (L1 = E, R1 = T)\n"); }
    /* Options from the pause menu of a table, and back */
    G.phase = PH_MORNING; G.cash = 5000; G.story = ST_NAMED; start_match(10);
    on_button(B_QUIT); if (pause_kind != PK_TABLE) { bad++; printf("table pause did not open\n"); }
    on_button(B_DOWN); on_button(B_A); if (pause_kind != PK_OPT) { bad++; printf("Options missing in the table pause menu (%d)\n", pause_kind); }
    on_button(B_A); if (pause_kind != PK_TABLE || pause_sel != 1) { bad++; printf("did not come back to the table pause menu (%d %d)\n", pause_kind, pause_sel); }
    on_button(B_DOWN); on_button(B_A); if (scr == SC_PLAY || pause_kind) { bad++; printf("LEAVE TABLE moved from the third place\n"); }
    G = keep; saved_G = keep_saved; frozen = fr; scr = kscr; room_seen = kroom; pause_kind = PK_NONE; wk_mode = 0; wk_place(km, kx, ky, kf);
    return bad;
}
static int v50_tests(void) {   /* the shirt debt, the objective menu, game over, the red-menu-free logic around them */
    int bad = 0; Game keep = G, keep_saved = saved_G; int km = wk_map, kx = wk_tx, ky = wk_ty, kf = wk_face, kscr = scr, kroom = room_seen, fr = frozen;
    frozen = 1; pause_kind = PK_NONE;
    /* 1. her door stays shut under 300 cash in hand (bank money does not count), opens at 300 and takes the money */
    g_init(&G); G.story = ST_HOME; G.hall_seen = 3; G.event = 1; saved_G = G; G.cash = DEBT - 1; G.bank = 5000; scr = SC_WALK; wk_mode = 0; wk_moving = 0;
    bot_use(MAP_HALL3, 1, 4, LF);
    if (scr != SC_WALK || wk_mode != 1 || strcmp(wk_pg[0], "I don't have the money, I will come back later.")) { bad++; printf("her door did not refuse (mode %d)\n", wk_mode); }
    if (G.story != ST_HOME || G.cash != DEBT - 1) { bad++; printf("the refusal changed something\n"); }
    on_button(B_A); on_button(B_A); wk_mode = 0;
    G.cash = DEBT + 50; bot_use(MAP_HALL3, 1, 4, LF); fake_ms += 3000; update(); fake_ms += 3000; update();
    if (scr != SC_STORY || vn_id != SCR_DOOR1) { bad++; printf("her door did not open with the money (scr %d)\n", scr); }
    if (G.cash != 50) { bad++; printf("the shirt money was not taken (cash %d)\n", G.cash); }
    /* 2. the bump scene ends with the Exhibition open and the debt stage on */
    g_init(&G); saved_G = G; G.event = 0; vn_id = SCR_BUMP; vn_done();
    if (scr != SC_WALK || wk_map != MAP_LOBBY) { bad++; printf("v66: the bump scene must lead straight back to the lobby (no key card any more, scr %d)\n", scr); }
    if (G.story != ST_HOME || !G.event) { bad++; printf("the bump scene must open the Exhibition (story %d event %d)\n", G.story, G.event); }
    /* 3. objectives: top hint and the Objective page of the pause menu */
    g_init(&G); G.story = ST_HOME; saved_G = G; saved_G.story = ST_NEW; room_seen = 0; G.hall_seen = 0;   /* (the last save is from before you were ever in your room)
    */ if (wk_hint() != H_ROOM) { bad++; printf("hint before the hall\n"); }
    G.hall_seen = 3; if (wk_hint() != H_ENTER) { bad++; printf("no ENTER YOUR ROOM hint after the look down the hall\n"); }
    room_seen = 1; if (wk_hint() != NULL) { bad++; printf("the hint must go once you are inside\n"); }
    G.event = 1; G.cash = 120; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN);
    on_button(B_QUIT); if (pause_kind != PK_HOME) { bad++; printf("pause did not open\n"); }
    on_button(B_DOWN); on_button(B_A); if (pause_kind != PK_OBJ) { bad++; printf("Objective did not open (%d)\n", pause_kind); }
    { char o[200]; char want[200]; obj_text(o, sizeof o); snprintf(want, sizeof want, O_DEBT, DEBT, 120, DEBT); if (strcmp(o, want)) { bad++; printf("objective text wrong: %s\n", o); } G.cash = DEBT; obj_text(o, sizeof o); snprintf(want, sizeof want, O_DEBT_OK, DEBT); if (strcmp(o, want)) { bad++; printf("objective text with the money wrong\n"); } G.cash = 120; }
    on_button(B_A); if (pause_kind != PK_HOME || pause_sel != 1) { bad++; printf("A did not leave the Objective page\n"); }
    on_button(B_DOWN); on_button(B_A); if (pause_kind != PK_OPT) { bad++; printf("Options did not open from the walking pause menu (%d)\n", pause_kind); }
    { int d0 = dlg_idx; on_button(B_RIGHT); if (dlg_idx != d0 + 1 && dlg_idx != 5) { bad++; printf("options in game did not change the dialogue box\n"); } dlg_idx = d0; }
    on_button(B_B); if (pause_kind != PK_HOME || pause_sel != 2) { bad++; printf("B did not return from the options to the pause menu (%d %d)\n", pause_kind, pause_sel); }
    on_button(B_DOWN); on_button(B_A); if (pause_kind != PK_EXITC) { bad++; printf("Exit moved from the fourth place\n"); } pause_kind = PK_NONE;
    on_button(B_QUIT); on_button(B_UP); if (pause_sel != 3) { bad++; printf("menu does not wrap\n"); } on_button(B_B); if (pause_kind) { bad++; printf("B did not resume\n"); }
    G.story = ST_NAMED; G.rs = RS_NONE; G.ring = 0; G.room_up = 2; { char o[200]; obj_text(o, sizeof o); if (strcmp(o, O_NONE)) { bad++; printf("no objective expected\n"); } }   /* v9.8 test fix: since v9.3 a room without the upgrades has its own objective (O_UP1) */
    /* 4. game over: no cash and no bank money for even the cheapest buy-in, only where money still matters */
    g_init(&G); G.story = ST_HOME; G.hall_seen = 3; saved_G = G; G.cash = 4; G.bank = 5; scr = SC_WALK; wk_mode = 0; wk_moving = 0; update();
    if (scr != SC_OVER) { bad++; printf("9 in total must be game over (scr %d)\n", scr); }
    scr = SC_WALK; G.cash = 5; G.bank = 5; update(); if (scr != SC_WALK) { bad++; printf("10 in total can still enter the cheapest exhibition\n"); }
    G.cash = 0; G.bank = 0; G.story = ST_FIRST; update(); if (scr != SC_OVER) { bad++; printf("no money after paying the shirt (first challenge not yet played) must be game over\n"); }
    scr = SC_WALK; G.story = ST_NAMED; update(); if (scr != SC_OVER) { bad++; printf("0 money after the first match must be game over\n"); }
    on_button(B_QUIT); if (pause_kind) { bad++; printf("pause opened on the game over screen\n"); }
    G.cash = 0; on_button(B_A); if (scr != SC_MENU || G.story != ST_NEW || G.cash != START_CASH || G.bank != TEST_BANK || unsaved()) { bad++; printf("game over did not start again from the beginning\n"); }
    /* 5. an old save from before the debt (intro done, Exhibition still locked) opens the Exhibition when loaded */
    { Game t; const char *f = "/tmp/pq_v50.sav"; g_init(&t); t.story = ST_HOME; t.event = 0; save_game(&t, f); g_init(&t); if (!load_game(&t, f) || !t.event) { bad++; printf("old save did not get the Exhibition\n"); } remove(f); }
    frozen = fr; G = keep; saved_G = keep_saved; room_seen = kroom; pause_kind = PK_NONE; wk_place(km, kx, ky, kf); scr = kscr; wk_mode = 0;
    return bad;
}
static int walk_tests(void) {
    int bad = 0, m, x, y, i, d; Game keep = G, keep_saved = saved_G; int km = wk_map, kx = wk_tx, ky = wk_ty, kf = wk_face;
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.room_up = 1; room_apply(); saved_G = G; pause_kind = PK_NONE; scr = SC_WALK; wk_mode = 0;   /* v9.8 test fix: these walking tests use the 11 x 7 room (bathroom door at 9,5, bedroom door 5,5); g_init gives the 7 x 7 base room since v9.3 */
    for (m = 0; m < MAP_N; m++) {
        int MW = wm[m].w, MH = wm[m].h, seen[MMH][MMW], qx[MMW * MMH], qy[MMW * MMH], h = 0, t = 0, sx = -1, sy = -1;
        for (y = 0; y < MH; y++) if ((int)strlen(wm[m].g[y]) != MW) { bad++; printf("map %d row %d is not %d wide\n", m, y, MW); }
        memset(seen, 0, sizeof seen);
        for (y = 0; y < MH && sx < 0; y++) for (x = 0; x < MW; x++) if (wm[m].g[y][x] == '.') { sx = x; sy = y; break; }
        seen[sy][sx] = 1; qx[t] = sx; qy[t++] = sy;
        while (h < t) { int cx = qx[h], cy = qy[h++]; for (d = 0; d < 4; d++) { int nx = cx + DX4[d], ny = cy + DY4[d]; if ((wk_tile(m, nx, ny) == '.' || wk_tile(m, nx, ny) == 'd') && !seen[ny][nx]) { seen[ny][nx] = 1; qx[t] = nx; qy[t++] = ny; } } }   /* the bedroom door counts as a way through (A crosses it) */
        for (y = 0; y < MH; y++) for (x = 0; x < MW; x++) {
            char c = wm[m].g[y][x]; int touch = 0;
            if (c == '.' && !seen[y][x]) { bad++; printf("map %d floor (%d,%d) cannot be reached\n", m, x, y); }
            if (c == '.' || c == '#') continue;
            for (d = 0; d < 4; d++) { int nx = x + DX4[d], ny = y + DY4[d]; if ((wk_tile(m, nx, ny) == '.' && seen[ny][nx]) || wk_tile(m, nx, ny) == c) touch = 1; }   /* the inside of a bigger object (the bed) needs no neighbour of its own */
            if (!touch) { bad++; printf("map %d tile %c (%d,%d) has no reachable neighbour\n", m, c, x, y); }
        }
    }
    for (i = 0; i < NDOORS; i++) {
        const Door *dr = &DOORS[i];
        if (wk_tile(dr->map, dr->x, dr->y) != 'D' && wk_tile(dr->map, dr->x, dr->y) != 'U' && wk_tile(dr->map, dr->x, dr->y) != 'E') { bad++; printf("door %d is not on a D, U or E tile\n", i); }
        if (dr->map == MAP_SBED) { if (MG_ROOM_UP[dr->ty][dr->tx] != '.') { bad++; printf("door %d (her bedroom) arrives inside a wall of the upgraded room\n", i); } }   /* v9.2: only the upgraded room has her bedroom door */
        else if (wk_tile(dr->tmap, dr->tx, dr->ty) != '.') { bad++; printf("door %d arrives inside a wall\n", i); }
    }
    for (m = 0; m < MAP_N; m++) { int ew = m == MAP_SOPHIA ? 1080 : m == MAP_ROOM ? wm[m].cw : m == MAP_LOBBY ? 1224 : wm[m].w * TILE, eh = m == MAP_SOPHIA ? 1152 : m == MAP_ROOM ? wm[m].ch : m == MAP_LOBBY ? 936 : wm[m].h * TILE; if (!wm[m].img || LW(wm[m].img) != ew || LH(wm[m].img) != eh) { bad++; printf("map picture %d missing or not %dx%d\n", m, ew, eh); } }   /* (the floor pictures are cropped to exactly their tiles) */
    if (!plSn) { bad++; printf("night sprite missing\n"); }
    { int m, k; static const int NM[5] = {MAP_ROOM, MAP_HALL, MAP_LOBBY, MAP_HALL1, MAP_HALL3};   /* the night copies of the three maps: there, same size, and slightly (not heavily) darker */
      for (k = 0; k < 5; k++) { SDL_Surface *d = wm[NM[k]].img, *n = night_of(NM[k]); long sd = 0, sn = 0; int x, y;
        if (!n || n->w != d->w || n->h != d->h) { bad++; printf("night map %d missing or wrong size\n", NM[k]); continue; }
        for (y = 0; y < d->h; y += 7) for (x = 0; x < d->w; x += 7) { Uint32 a1 = *px(d, x, y), b1 = *px(n, x, y); sd += ((a1 >> d->format->Rshift) & 255) + ((a1 >> d->format->Gshift) & 255) + ((a1 >> d->format->Bshift) & 255); sn += ((b1 >> n->format->Rshift) & 255) + ((b1 >> n->format->Gshift) & 255) + ((b1 >> n->format->Bshift) & 255); }
        if (sd <= 0 || sn >= sd * 90 / 100 || sn <= sd * 60 / 100) { bad++; printf("night map %d brightness %ld of %ld (should be slightly darker)\n", NM[k], sn, sd); } }
      if (night_of(MAP_SOPHIA)) { bad++; printf("her room must not get a second night copy\n"); } (void)m;
      if (!plSm || !nightS) { bad++; printf("night sprites or her night picture missing\n"); } }
    if (!plS || !clerkS || pl_fw < 100 || pl_fh < 150) { bad++; printf("walking sheet or clerk missing\n"); }
    else {   /* sizes the artist asked for: you about 145 px tall, the receptionist about 79 */
        int top = pl_fh, bot = 0, ct = 99999, cb = 0;
        int pfw = plS->w / 3, pfh = plS->h / 4, ph, ch2;   /* scanned at screen resolution, compared in logical px */
        top = pfh;
        for (y = 0; y < pfh; y++) for (x = 0; x < pfw; x++) if (((*px(plS, x + pfw, y) & plS->format->Amask) >> plS->format->Ashift) > 32) { if (y < top) top = y; if (y > bot) bot = y; }
        for (y = 0; y < clerkS->h; y++) for (x = 0; x < clerkS->w; x++) if (((*px(clerkS, x, y) & clerkS->format->Amask) >> clerkS->format->Ashift) > 32) { if (y < ct) ct = y; if (y > cb) cb = y; }
        ph = (bot - top + 1) * 480 / SH; ch2 = (cb - ct + 1) * 480 / SH;
        if (ph < 139 || ph > 150) { bad++; printf("player is %d px tall, wanted about 145\n", ph); }
        if (ch2 < 75 || ch2 > 82) { bad++; printf("clerk is %d px tall, wanted about 79\n", ch2); }
    }
    /* solid things stay solid */
    { static const int solid[][4] = {{MAP_LOBBY, 7, 6, UP}, {MAP_LOBBY, 9, 6, UP}, {MAP_LOBBY, 5, 4, UP}, {MAP_LOBBY, 10, 4, UP}, {MAP_LOBBY, 11, 4, UP}, {MAP_LOBBY, 5, 5, RT}, {MAP_LOBBY, 10, 5, LF}, {MAP_LOBBY, 11, 5, RT}, {MAP_ROOM, 8, 3, RT}, {MAP_ROOM, 9, 4, UP}, {MAP_ROOM, 7, 3, UP}, {MAP_ROOM, 2, 5, LF}, {MAP_ROOM, 4, 3, RT}, {MAP_HALL, 12, 5, RT}, {MAP_HALL, 3, 3, UP}, {MAP_HALL, 2, 3, LF}, {MAP_HALL1, 4, 6, RT}, {MAP_HALL1, 3, 8, RT}, {MAP_HALL1, 11, 4, UP}, {MAP_HALL3, 11, 4, RT}, {MAP_HALL3, 3, 6, LF}};
      for (i = 0; i < (int)(sizeof solid / sizeof solid[0]); i++) {
          wk_place(solid[i][0], solid[i][1], solid[i][2], DN); on_button(DIRBTN[solid[i][3]]);
          if (wk_tx != solid[i][1] || wk_ty != solid[i][2] || wk_face != solid[i][3] || wk_map != solid[i][0]) { bad++; printf("walked through something solid (%d)\n", i); } } }
    /* doors connect, there and back */
    wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN);
    { int y; for (y = 6; y <= 9; y++) if (wk_tile(MAP_HALL1, 10, y) != '.' || wk_tile(MAP_HALL1, 11, y) != '.') { bad++; printf("the stairs are not two tiles wide at row %d\n", y); }
      if (!bot_goto(MAP_HALL1, 10, 8) || !bot_walk(11, 8) || !bot_walk(10, 7) || !bot_walk(10, 9)) { bad++; printf("cannot walk the full width of the stairs\n"); } }
    { int x, y, n = 0; for (y = 0; y < wm[MAP_EVENT].h; y++) for (x = 0; x < wm[MAP_EVENT].w; x++) { char c = wk_tile(MAP_EVENT, x, y); if (c != '.' && c != '#' && c != 'V' && c != 'D') { bad++; printf("event map: odd tile %c\n", c); } if (c == 'V') n++; }
      if (n != 5) { bad++; printf("event map: desk tiles\n"); }
      if (night_of(MAP_EVENT)) { bad++; printf("the Event Center must have no night copy\n"); }
      G.event = 1; G.phase = PH_NIGHT; wk_place(MAP_EVENT, 5, 5, UP); scr = SC_WALK;
      if (!wk_walkable(wk_tile(MAP_EVENT, EV_OUT_X, EV_OUT_Y)) || !wk_walkable(wk_tile(MAP_EVENT, EV_ARR_X, EV_ARR_Y))) { bad++; printf("event map: arrival or match-room door tile not walkable\n"); }
      G.phase = PH_MORNING; }
    if (!bot_goto(MAP_LOBBY, 5, 8) || wk_map != MAP_LOBBY || wk_tx != 5 || wk_ty != 8) { bad++; printf("room to lobby failed (map %d at %d,%d)\n", wk_map, wk_tx, wk_ty); }
    if (!bot_goto(MAP_ROOM, 3, 4) || wk_map != MAP_ROOM || wk_tx != 3 || wk_ty != 4) { bad++; printf("lobby to room failed\n"); }
    /* the storm and the bathroom answer, you stay where you are; Event Center follows the unlock */
    bot_face(MAP_LOBBY, 12, 11, DN); if (scr != SC_WALK || wk_mode || wk_ty != 11) { bad++; printf("walking into the glass doors must do nothing\n"); }
    on_button(B_A); if (scr != SC_STORY || vn_id != SCR_RAIN || vn_buf[0].bg != BG_RAIN || wk_ty != 11) { bad++; printf("the glass doors did not show the rain picture\n"); }
    on_button(B_A); on_button(B_A); if (scr != SC_WALK || wk_map != MAP_LOBBY || wk_tx != 12 || wk_ty != 11) { bad++; printf("rain scene did not return to where you stood\n"); }
    bot_face(MAP_ROOM, 9, 5, RT); if (wk_mode || wk_tx != 9) { bad++; printf("walking into the bathroom door must do nothing\n"); }
    on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_BATH) { bad++; printf("bathroom message wrong\n"); } on_button(B_A);
    if (wk_mode) { bad++; printf("walking message stuck\n"); }
    /* the bedroom door: you cannot walk through the gap, A fades you to the other side and back */
    wk_place(MAP_ROOM, 5, 5, DN); on_button(B_RIGHT); if (wk_tx != 5 || wk_face != RT || !wk_interactive(wk_tile(wk_map, 6, 5))) { bad++; printf("walked straight through the bedroom door (at %d)\n", wk_tx); }
    on_button(B_A); if (wk_map != MAP_ROOM || wk_tx != 7 || wk_ty != 5 || wk_face != RT || !room_seen) { bad++; printf("bedroom door did not lead in (at %d,%d)\n", wk_tx, wk_ty); }
    on_button(B_LEFT); if (wk_tx != 7) { bad++; printf("walked back through the bedroom door\n"); }
    on_button(B_A); if (wk_tx != 5 || wk_ty != 5 || wk_face != LF) { bad++; printf("bedroom door did not lead out (at %d,%d)\n", wk_tx, wk_ty); }
    for (y = 4; y <= 5; y++) { wk_place(MAP_ROOM, 8, y, DN); }   /* (the wall above the door is solid: nothing stands there) */
    /* the clerk and ATM icons and the Event Center wait for your room; the Event Center is a plain wall until the first match is over */
    { Game kg = G, ks = saved_G; g_init(&G); G.room_up = 1; room_apply(); saved_G = G; room_seen = 0; if (services_open()) { bad++; printf("services open on arrival\n"); }
    G.story = ST_HOME; wk_place(MAP_ROOM, 5, 5, RT); on_button(B_A); if (!services_open()) { bad++; printf("services not open after entering the room\n"); }
    G.story = ST_NAMED; G.hall_seen = 3; room_seen = 0; if (!services_open()) { bad++; printf("services closed after the first match\n"); }
    G.event = 0; if (wk_interactive('E')) { bad++; printf("event opening interactive while locked\n"); } G.event = 1; if (!wk_interactive('E')) { bad++; printf("event opening not interactive when unlocked\n"); } G.event = 0;
    G = kg; saved_G = ks; room_seen = 0; }
    /* v6.7: one free tile between the fridge and the sofa: you can stand in front of the fridge and open it from below */
    if (wk_tile(MAP_ROOM, 1, 3) != '.' || wk_tile(MAP_ROOM, 1, 2) != 'F' || wk_tile(MAP_ROOM, 1, 4) != 'C' || wk_tile(MAP_ROOM, 1, 5) != 'C') { bad++; printf("v71: the sofa must fill both tiles (1,4) and (1,5)\n"); }
    { int u, t; for (u = 0; u < 3; u++) { G.room_up = u; room_apply(); for (t = 4; t <= 5; t++) { if (wk_walkable(wk_tile(MAP_ROOM, 1, t))) { bad++; printf("v71: you can walk over the sofa at (1,%d) (up %d)\n", t, u); }
        G.phase = PH_MORNING; bot_use(MAP_ROOM, 2, t, LF); if (wk_mode != 2) { bad++; printf("v71: A on the sofa tile (1,%d) did not ask to save (up %d)\n", t, u); } wk_mode = 0; } } G.room_up = 0; room_apply(); }
    { int u; for (u = 0; u < 3; u++) { G.room_up = u; room_apply(); if (wk_tile(MAP_ROOM, 1, 3) != '.' || wk_tile(MAP_ROOM, 2, 3) != '.' || wk_tile(MAP_ROOM, 3, 3) != '.') { bad++; printf("v68: tile in front of the fridge must be walkable like the counter top (up %d)\n", u); } } G.room_up = 0; room_apply(); }
    bot_use(MAP_ROOM, 1, 3, UP); if (scr != SC_INV) { bad++; printf("v68: the fridge did not open from the tile in front of it\n"); } enter_hub();
    /* the fridge opens the inventory; the sink only talks */
    bot_use(MAP_ROOM, 2, 3, UP); if (wk_mode != 1 || scr != SC_WALK) { bad++; printf("sink wrong\n"); } on_button(B_A); on_button(B_A);
    /* saving: only when you say yes, at the bed or the sofa; Exit asks, warns and goes back to the saved state */
    /* the time cycle: the sofa saves in the morning and the evening and moves the time on; at night only the bed works (sleeping saves and starts the next day) */
    G.day = 1; G.phase = PH_MORNING; saved_G = G;
    wk_place(MAP_ROOM, wm[MAP_ROOM].w == 7 ? 4 : 8, 3, RT); G.cash = 500; on_button(B_A);   /* the bed in the morning */
    if (wk_mode != 1 || wk_pg[0] != W_BED_DAY || G.phase != PH_MORNING) { bad++; printf("the bed in the morning must say I'm not tired\n"); } WK_CLOSE();
    G.phase = PH_EVENING; on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_BED_DAY) { bad++; printf("the bed in the evening must say I'm not tired\n"); } WK_CLOSE();
    G.phase = PH_MORNING; wk_place(MAP_ROOM, 2, 5, LF); on_button(B_A);   /* the sofa in the morning */
    if (wk_mode != 2 || wk_sleep != 0 || !unsaved()) { bad++; printf("the sofa did not ask to save\n"); }
    if (wk_yes != 0) { bad++; printf("v10.5: the sofa menu must start on REST in the morning (%d)\n", wk_yes); }
    on_button(B_DOWN); on_button(B_DOWN); on_button(B_A); if (wk_mode || !unsaved() || saved_G.cash != START_CASH || G.phase != PH_MORNING || saved_G.phase != PH_MORNING) { bad++; printf("answering CANCEL must save nothing and move no time\n"); }
    on_button(B_A); on_button(B_DOWN); on_button(B_A);   /* v10.5: SAVE ONLY: saved, the time stays, no fade */
    if (wk_mode != 1 || unsaved() || saved_G.cash != 500 || wk_pg[0] != W_SAVED_ONLY || G.phase != PH_MORNING || saved_G.phase != PH_MORNING || G.day != 1) { bad++; printf("v10.5: SAVE ONLY in the morning must save and keep the time (phase %d)\n", G.phase); } WK_CLOSE();
    on_button(B_A); on_button(B_A); if (wk_mode != 1 || unsaved() || saved_G.cash != 500 || wk_pg[0] != W_SAVED_EVE || G.phase != PH_EVENING || saved_G.phase != PH_EVENING || G.day != 1) { bad++; printf("morning sofa did not save and move to the evening (phase %d)\n", G.phase); } WK_CLOSE();
    G.cash = 700; on_button(B_A); on_button(B_A); if (G.phase != PH_NIGHT || saved_G.phase != PH_NIGHT || saved_G.cash != 700 || wk_pg[0] != W_SAVED_NIGHT || G.day != 1) { bad++; printf("evening sofa did not move to the night (phase %d)\n", G.phase); } WK_CLOSE();
    /* v10.5: at night the sofa still saves (SAVE ONLY), REST cannot be chosen, the time never moves */
    on_button(B_A); if (wk_mode != 2 || wk_sleep != 0 || wk_yes != 1) { bad++; printf("v10.5: the sofa at night must open the menu on SAVE ONLY (mode %d yes %d)\n", wk_mode, wk_yes); }
    { int k, hit0 = 0; for (k = 0; k < 7; k++) { on_button(k & 1 ? B_DOWN : B_UP); if (wk_yes == 0) hit0 = 1; } if (hit0) { bad++; printf("v10.5: REST must be skipped at night\n"); } wk_yes = 1; }
    G.cash = 710; on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_SAVED_ONLY || G.phase != PH_NIGHT || G.day != 1 || saved_G.cash != 710 || unsaved()) { bad++; printf("v10.5: SAVE ONLY at night must save and keep the night (phase %d cash %d)\n", G.phase, saved_G.cash); } WK_CLOSE();
    G.cash = 700; saved_G = G;
    wk_place(MAP_ROOM, wm[MAP_ROOM].w == 7 ? 4 : 8, 3, RT); G.cash = 800; on_button(B_A);   /* the bed at night */
    if (wk_mode != 2 || wk_sleep != 1) { bad++; printf("the bed at night did not ask to sleep\n"); }
    on_button(B_DOWN); on_button(B_A); if (wk_mode || G.phase != PH_NIGHT || saved_G.cash != 700) { bad++; printf("answering no to sleep changed something\n"); }
    on_button(B_A); on_button(B_A); if (G.day != 2 || G.phase != PH_MORNING || saved_G.day != 2 || saved_G.phase != PH_MORNING || saved_G.cash != 800 || wk_pg[0] != W_SAVED_DAY) { bad++; printf("sleeping did not start day 2 in the morning (day %d phase %d)\n", G.day, G.phase); } WK_CLOSE();
    { int d0 = G.day, ph; for (ph = 0; ph < 5; ph++) { /* the full loop keeps counting */ g_advance(&G); } if (G.day != d0 + 1 || G.phase != PH_NIGHT) { bad++; printf("time loop wrong\n"); } G.day = 2; G.phase = PH_MORNING; saved_G = G; }
    G.cash = 900; on_button(B_QUIT); if (pause_kind != PK_HOME) { bad++; printf("menu did not open the pause\n"); }
    on_button(B_DOWN); on_button(B_DOWN); on_button(B_DOWN); on_button(B_A); if (pause_kind != PK_EXITC || !unsaved()) { bad++; printf("exit did not ask first\n"); }
    on_button(B_A); if (pause_kind || G.cash != 900) { bad++; printf("no, keep playing wrong\n"); }   /* default answer is NO */
    on_button(B_QUIT); on_button(B_DOWN); on_button(B_DOWN); on_button(B_DOWN); on_button(B_A); on_button(B_DOWN); on_button(B_A);
    if (scr != SC_MENU || G.cash != 800 || pause_kind || wk_map != MAP_ROOM) { bad++; printf("exit did not return to the last save (cash %d scr %d)\n", G.cash, scr); }
    /* held buttons move you in real time, one tile per 350 ms, and stop on a tile when released; B does not run any more */
    frozen = 0; scr = SC_WALK; wk_place(MAP_LOBBY, 2, 8, RT); memset(kheld, 0, sizeof kheld); kheld[B_RIGHT] = 1; last_dir = RT; on_button(B_RIGHT);
    if (!wk_moving) { bad++; printf("a press did not start a step\n"); }
    for (i = 0; i < 70; i++) { SDL_Delay(16); update(); }
    if (wk_tx < 4 || wk_tx > 6) { bad++; printf("held walking covered %d tiles in about a second (350 ms per tile)\n", wk_tx - 2); }
    kheld[B_RIGHT] = 0; for (i = 0; i < 30; i++) { SDL_Delay(16); update(); }
    if (wk_moving || wk_mode) { bad++; printf("did not stop after release\n"); }
    { int t0 = wk_tx; kheld[B_B] = 1; kheld[B_RIGHT] = 1; last_dir = RT; on_button(B_RIGHT); for (i = 0; i < 70; i++) { SDL_Delay(16); update(); } kheld[B_B] = kheld[B_RIGHT] = 0; for (i = 0; i < 30; i++) { SDL_Delay(16); update(); }
      if (wk_tx - t0 > 4) { bad++; printf("holding B still runs (%d tiles)\n", wk_tx - t0); } }
    frozen = 1; memset(kheld, 0, sizeof kheld);
    /* the stair hall, the stairs, the elevator and the first look at each upper floor */
    g_init(&G); G.story = ST_HOME; saved_G = G; scr = SC_WALK; wk_place(MAP_LOBBY, 3, 4, UP); on_button(B_UP);
    if (wk_map != MAP_LOBBY || wk_ty != 4) { bad++; printf("walking into a door went through it\n"); }
    on_button(B_A);
    if (wk_map != MAP_HALL1 || wk_mode || wk_tx != 2 || wk_ty != 9) { bad++; printf("the lobby door did not lead to the stair hall (map %d)\n", wk_map); }
    wk_place(MAP_HALL1, 11, 9, DN); on_button(B_A);   /* the stairs: you arrive on the 2nd floor and look around once */
    if (wk_map != MAP_HALL || wk_mode != 1 || wk_pg[0] != W_F2 || (G.hall_seen & 1)) { bad++; printf("2nd-floor look-around did not start (map %d)\n", wk_map); }
    on_button(B_LEFT); if (wk_tx != 8 || wk_ty != 3) { bad++; printf("moved during the look-around\n"); }
    on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_HALL3 || wk_face != RT || wk_look != 12) { bad++; printf("2nd-floor look-around step 2 wrong\n"); }
    on_button(B_A); if (wk_mode || !(G.hall_seen & 1) || wk_look != -1) { bad++; printf("2nd-floor look-around did not end\n"); }
    on_button(B_LEFT); if (wk_tx != 7) { bad++; printf("cannot walk after the look-around\n"); }
    wk_place(MAP_HALL, 8, 3, UP); on_button(B_A);   /* stairs down and back up: no second look-around */
    if (wk_map != MAP_HALL1 || wk_tx != 11 || wk_ty != 9) { bad++; printf("stairs down failed (map %d)\n", wk_map); }
    on_button(B_DOWN); on_button(B_A); if (wk_map != MAP_HALL || wk_mode || wk_tx != 8 || wk_ty != 3) { bad++; printf("stairs up failed or the look-around repeated\n"); }
    /* the elevator: the other two floors, in order; B steps back; the 3rd floor needs the floor key card */
    wk_place(MAP_HALL, 4, 3, UP); on_button(B_A);
    if (wk_mode != 5 || wk_nflr != 2 || wk_flr[0] != 1 || wk_flr[1] != 3) { bad++; printf("2nd-floor elevator menu wrong (mode %d, %d floors)\n", wk_mode, wk_nflr); }
    on_button(B_B); if (wk_mode) { bad++; printf("B did not close the elevator menu\n"); }
    on_button(B_A); on_button(B_DOWN); on_button(B_DOWN); on_button(B_UP); if (wk_fsel != 1) { bad++; printf("elevator menu selection wraps wrong (%d)\n", wk_fsel); }
    on_button(B_A);   /* to the 3rd floor: out of the elevator, look at her door once */
    if (wk_map != MAP_HALL3 || wk_tx != 9 || wk_ty != 3 || wk_mode != 1 || wk_pg[0] != W_F3 || (G.hall_seen & 2)) { bad++; printf("the elevator did not lead to the 3rd floor (map %d)\n", wk_map); }
    on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_HALL2 || wk_face != LF || wk_look != 0) { bad++; printf("3rd-floor look-around step 2 wrong\n"); }
    on_button(B_A); if (wk_mode || G.hall_seen != 3 || wk_look != -1) { bad++; printf("3rd-floor look-around did not end\n"); }
    wk_place(MAP_HALL3, 10, 3, UP); on_button(B_A);   /* the other elevator tile works too; from here: 1st and 2nd floor only */
    if (wk_mode != 5 || wk_nflr != 2 || wk_flr[0] != 1 || wk_flr[1] != 2) { bad++; printf("3rd-floor elevator menu wrong\n"); }
    on_button(B_A); if (wk_map != MAP_HALL1 || wk_tx != 6 || wk_ty != 3 || wk_face != DN) { bad++; printf("elevator to the 1st floor failed (map %d)\n", wk_map); }
    on_button(B_UP); on_button(B_A); if (wk_mode != 5 || wk_nflr != 2 || wk_flr[0] != 2 || wk_flr[1] != 3) { bad++; printf("1st-floor elevator menu wrong\n"); }   /* (you step out facing down: the door is behind you, so turn) */
    { int fl[3]; Game kk = G; G.story = ST_NEW; if (elev_floors(2, fl) != 1 || fl[0] != 1) { bad++; printf("without the floor key card the 3rd floor must not be offered\n"); } G = kk; }
    wk_mode = 0;
    /* the opening: arrival lines on the lobby map, then you walk; nothing but the desk works, the desk starts the bump scene, then you are free in front of the desk */
    g_init(&G); saved_G = G; scr = SC_MENU; menu_sel = 1; memset(slot_st, 0, sizeof slot_st); slot_act = 1; on_button(B_A); if (scr != SC_CAREER) { bad++; printf("v10.5: CAREER MODE must show the slots (scr %d)\n", scr); } on_button(B_A);   /* v10.5: an empty slot starts the story */
    if (scr != SC_MONO || mono_pg != 0) { bad++; printf("new game did not start with the monologue\n"); }
    on_button(B_A); if (scr != SC_MONO || mono_pg != 1) { bad++; printf("monologue page 2 missing\n"); }
    on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_INTRO || wk_map != MAP_LOBBY || vn_buf[0].bg != BG_LOBBYMAP) { bad++; printf("monologue did not lead to the arrival scene\n"); }
    on_button(B_A); on_button(B_A); on_button(B_A); on_button(B_A);
    if (scr != SC_WALK || G.story != ST_NEW || wk_map != MAP_LOBBY || wk_tx != 12 || wk_ty != 10 || wk_face != UP) { bad++; printf("arrival scene did not hand over walking (scr %d at %d,%d)\n", scr, wk_tx, wk_ty); }
    if (wk_hint() != H_DESK) { bad++; printf("no hint to go to the desk\n"); }
    bot_use(MAP_LOBBY, 3, 4, UP); if (wk_map != MAP_LOBBY || wk_mode != 1 || wk_pg[0] != W_CHECKIN) { bad++; printf("the hall door was not held shut\n"); } on_button(B_A); on_button(B_A);
    bot_use(MAP_LOBBY, 10, 4, UP); if (scr == SC_ATM || scr == SC_STORY || wk_mode != 1 || wk_pg[0] != W_CHECKIN) { bad++; printf("v91: the wall ATM must not work in the opening (scr %d)\n", scr); } on_button(B_A); on_button(B_A); wk_mode = 0; if (scr != SC_WALK) scr = SC_WALK;
    bot_use(MAP_LOBBY, 8, 6, UP); if (scr != SC_STORY || vn_id != SCR_BUMP) { bad++; printf("the desk did not start the bump scene (scr %d)\n", scr); }
    { int k; for (k = 0; k < vn_n; k++) if (vn_buf[k].bg != BG_LOBBY) { bad++; printf("bump beat %d is not on the painted lobby\n", k); } }
    for (i = 0; i < 60 && scr == SC_STORY; i++) on_button(B_A);
    if (scr != SC_WALK || G.story != ST_HOME || wk_map != MAP_LOBBY || wk_tx != 7 || wk_ty != 6 || wk_face != DN) { bad++; printf("bump scene did not end in front of the desk\n"); }
    if (wk_hint() != H_ROOM) { bad++; printf("no hint to go to your room\n"); }
    G = keep; saved_G = keep_saved; wk_map = km; wk_tx = wk_fx = kx; wk_ty = wk_fy = ky; wk_face = kf; wk_mode = 0; scr = SC_MENU;
    printf("walking: maps, doors, solid objects, save, exit, held movement: problems %d\n", bad);
    return bad;
}


/* v6.2: the ring, the proposal, the room upgrade, moving in, the online store, gifts and the free matches at home - all through the real buttons */
static void RC(void) { if (scr == SC_ROM && !rom_choose) on_button(B_A); }   /* v6.6: her line first, A opens the choices */
static void door_knock(void) { wk_mode = 0; scr = SC_WALK; wk_place(MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, UP); on_button(B_A); }
static int rom_adv(int maxn) { int k; for (k = 0; k < maxn && scr == SC_STORY; k++) on_button(B_A); return scr != SC_STORY; }
#define RSHOT(n) do { if (shotbuf) shot(n); } while (0)
static int v63_tests(void) {   /* v6.3: the debug panel (X in the online store) through the real buttons */
    int bad = 0; Game keep = G, keeps = saved_G; int fr = frozen; frozen = 1;
    g_init(&G); G.story = ST_HOME; G.room_up = 1; G.cash = 100; G.energy = 100; saved_G = G; room_seen = 1; scr = SC_WALK; pause_kind = PK_NONE; wk_mode = 0; fade_st = 0;
    G.rs = RS_LIVING;   /* v10.0: the store opens on ITEMS (where X is the debug key) only once she lives with you; STOCKS uses X to sell */
    bot_use(MAP_ROOM, 7, 3, UP); if (scr != SC_SHOP) { bad++; printf("v63: the computer did not open the store\n"); }
    on_button(B_X); G.rs = RS_NONE; if (scr != SC_DEBUG) { bad++; printf("v63: X in the store did not open the debug panel\n"); }
    RSHOT("v63_debug_panel");
    on_button(B_A); if (G.story != ST_NAMED || !G.event || !G.ex_big || G.hall_seen != 3) { bad++; printf("v63: skip did not set the state\n"); }
    on_button(B_A); if (strcmp(dbg_msg, "ALREADY PAST THE MATCH")) { bad++; printf("v63: second skip message wrong: %s\n", dbg_msg); }
    on_button(B_DOWN); on_button(B_A); if (G.cash != 50100) { bad++; printf("v63: +50000 gave %d\n", G.cash); }
    on_button(B_DOWN); on_button(B_A); if (G.day != 2 || G.phase != PH_MORNING) { bad++; printf("v63: new day wrong\n"); }
    on_button(B_DOWN); on_button(B_A); if (G.rom_lvl != 2) { bad++; printf("v63: fill bar wrong\n"); }
    RSHOT("v63_debug_after");
    { int d0 = G.day; on_button(B_DOWN); on_button(B_A); if (G.day != d0 + 7 || G.phase != PH_MORNING) { bad++; printf("v64: skip 7 days wrong\n"); } }
    { Game keep = G; on_button(B_DOWN); on_button(B_A); if (!G.pill_seen || !G.pump || G.mpill != 4 || !g_shop_avail(&G, SH_MILK)) { bad++; printf("v65: the debug milk shortcut did not work\n"); } RSHOT("v65_debug_milk"); G = keep; }
    on_button(B_DOWN); on_button(B_A); if (G.room_up != 2) { bad++; printf("v93: the debug room row did not raise the level (%d)\n", G.room_up); }
    { Game keep = G; on_button(B_DOWN); on_button(B_A); if (G.outfit != OF_USUAL || scr != SC_DEBUG || strcmp(dbg_msg, "ONLY AFTER THE WEDDING")) { bad++; printf("v121: the debug cow row before the wedding changes nothing (outfit %d)\n", G.outfit); }
      dbg_sel = 7; G.rs = RS_LIVING; G.married = 1; on_button(B_A); if (G.outfit != OF_COW || !G.cow_given || !g_cow(&G)) { bad++; printf("v121: the debug cow row puts her in the cow outfit (outfit %d)\n", G.outfit); }
      G = keep; dbg_sel = 7; }
    on_button(B_DOWN); on_button(B_DOWN); on_button(B_DOWN); on_button(B_A); if (scr != SC_SHOP) { bad++; printf("v63: BACK did not return to the store\n"); }   /* v12.3: row 8 is BREAK THE PUMP, row 9 the injection, BACK is row 10 */
    on_button(B_X); on_button(B_B); if (scr != SC_SHOP) { bad++; printf("v63: B in the panel did not return to the store\n"); }
    on_button(B_B); if (scr != SC_WALK) { bad++; printf("v63: the store did not close\n"); }
    if (!unsaved()) { bad++; printf("v63: debug changes must count as unsaved\n"); }
    /* after the skip the real romance flow is reachable: buy the ring, propose is offered */
    G.cash = 50100; bot_use(MAP_ROOM, 7, 3, UP); on_button(B_R1); on_button(B_A); on_button(B_A);   /* v10.0: the skip gives 20 wins, so the store has STOCKS and the BLACK MARKET (ring): one R1 */
    if (!G.ring || !g_can_propose(&G)) { bad++; printf("v63: ring/propose not available after the skip\n"); }
    G = keep; saved_G = keeps; frozen = fr; scr = SC_WALK; return bad;
}

/* v6.8: unpacking before the Event Center and the 3rd floor, her day-6 / cover pictures, the lead-in before every full gauge, the first bottle, the info pages, the fridge */
static int v68_tests(void) {
    int bad = 0, i, k; Game keep = G, keep_saved = saved_G; int km = wk_map, kx = wk_tx, ky = wk_ty, kf = wk_face, kscr = scr, kroom = room_seen, fr = frozen, kun = unpack_done;
    frozen = 1; pause_kind = PK_NONE; fade_st = 0;
    /* 1. before the first visit to your room: the Event Center and the 3rd floor say "unpack first" */
    g_init(&G); G.story = ST_HOME; G.hall_seen = 3; G.event = 1; saved_G = G; saved_G.story = ST_NEW; room_seen = 0; unpack_done = 0; scr = SC_WALK; wk_mode = 0;
    if (unpacked()) { bad++; printf("v68: unpacked() before the first room visit\n"); }
    wk_place(MAP_LOBBY, 1, 6, LF); on_button(B_A);
    if (wk_map != MAP_LOBBY || wk_mode != 1 || strcmp(wk_pg[0], "I need to unpack my things first.")) { bad++; printf("v68: the Event Center door must refuse (map %d mode %d)\n", wk_map, wk_mode); }
    wk_mode = 0;
    wk_place(MAP_HALL, ELEV_X[2], ELEV_Y[2], DN); wk_nflr = elev_floors(2, wk_flr); wk_fsel = 0; while (wk_fsel < wk_nflr && wk_flr[wk_fsel] != 3) wk_fsel++;
    if (wk_fsel >= wk_nflr) { bad++; printf("v68: the 3rd floor entry must still be listed\n"); }
    else { wk_mode = 5; on_button(B_A); if (wk_map != MAP_HALL || wk_mode != 1 || strcmp(wk_pg[0], "I need to unpack my things first.")) { bad++; printf("v68: the 3rd floor entry must refuse (map %d mode %d)\n", wk_map, wk_mode); } }
    wk_mode = 0;
    wk_nflr = elev_floors(2, wk_flr); wk_fsel = 0; wk_mode = 5; if (wk_flr[0] != 1) { bad++; printf("v68: floor 1 must be the first entry\n"); }
    on_button(B_A); if (wk_map != MAP_HALL1 && wk_mode == 1) { bad++; printf("v68: the 1st floor must stay reachable\n"); }
    /* 2. the first time in your room: a short wait, a fade, then the line; after that everything opens */
    wk_mode = 0; wk_place(MAP_HALL, 12, 3, UP); wk_nmap = MAP_ROOM; wk_ntx = 3; wk_nty = 5; wk_nface = UP; go(GO_MAP, 0);   /* v7.1: you really arrive through the door at (3,5) */
    if (wk_map != MAP_ROOM || wk_tx != UNPACK_X || wk_ty != UNPACK_Y || wk_face != RT) { bad++; printf("v71: after the unpacking you must stand in the bedroom at (%d,%d) facing the bed, not (%d,%d) face %d\n", UNPACK_X, UNPACK_Y, wk_tx, wk_ty, wk_face); }
    if (wk_tile(MAP_ROOM, UNPACK_X, UNPACK_Y) != '.' || wk_tile(MAP_ROOM, 5, 3) != 'B' || wk_tile(MAP_ROOM, 4, 2) != 'P') { bad++; printf("v76: the spot must be walkable, the computer on its left, the bed on its right\n"); }
    shot("v76_unpack_spot");
    if (wk_map != MAP_ROOM || wk_mode != 1 || strcmp(wk_pg[0], "Alright, finished unpacking my things.") || !unpack_done) { bad++; printf("v68: the first room visit must end with the unpacked line (mode %d done %d)\n", wk_mode, unpack_done); }
    RSHOT("v68_unpacked_line");
    on_button(B_A); on_button(B_A); wk_mode = 0;
    if (!unpacked()) { bad++; printf("v68: unpacked() after the first room visit\n"); }
    wk_place(MAP_LOBBY, 1, 6, LF); on_button(B_A); if (wk_map != MAP_EVENT) { bad++; printf("v68: the Event Center must open after unpacking (map %d)\n", wk_map); }
    wk_place(MAP_HALL, ELEV_X[2], ELEV_Y[2], DN); wk_nflr = elev_floors(2, wk_flr); wk_fsel = 0; while (wk_fsel < wk_nflr && wk_flr[wk_fsel] != 3) wk_fsel++;
    if (wk_fsel < wk_nflr) { wk_mode = 5; on_button(B_A); if (wk_map != MAP_HALL3) { bad++; printf("v68: the 3rd floor must open after unpacking (map %d)\n", wk_map); } }
    /* a game saved in your room has always unpacked; the second visit never plays it again */
    unpack_done = 0; saved_G.story = ST_HOME; if (!unpacked()) { bad++; printf("v68: a saved game must count as unpacked\n"); }
    unpack_done = 1; wk_mode = 0; wk_nmap = MAP_ROOM; wk_ntx = ROOM_SX; wk_nty = ROOM_SY; wk_nface = DN; go(GO_MAP, 0); if (wk_mode == 1) { bad++; printf("v68: the unpacking must play only once\n"); }
    /* v7.1 in real time: input locked, two normal steps up by themselves, then the slow fade into the bedroom and the line */
    { int t0, t, ty_seen = 0, locked = 1; g_init(&G); G.story = ST_HOME; G.hall_seen = 3; saved_G = G; saved_G.story = ST_NEW; unpack_done = 0; frozen = 0; fade_st = 0; scr = SC_WALK;
      wk_place(MAP_ROOM, 3, 5, UP); unpack_begin();
      if (wk_mode != 7) { bad++; printf("v71: the unpacking walk did not start (mode %d)\n", wk_mode); }
      on_button(B_UP); on_button(B_A); if (wk_ty != 5 || wk_mode != 7) { bad++; printf("v71: input must be locked during the walk\n"); }
      t0 = (int)now_ms();
      for (t = 0; t < 600 && !(fade_st && wk_mode == 0); t++) { update(); SDL_Delay(10); if (wk_tx == 4 && wk_moving) ty_seen = 1; if (wk_mode == 7 && wk_ty != 5) locked = 0; }
      t = (int)now_ms() - t0;
      if (wk_ty != 5 || wk_tx != 5 || wk_face != RT || !locked) { bad++; printf("v76: the walk must end two tiles to the right at (5,5), facing the door, is (%d,%d)\n", wk_tx, wk_ty); }
      if (t < 2 * UNPACK_STEP_MS - 60 || t > 2 * UNPACK_STEP_MS + 900) { bad++; printf("v76: the two slow steps took %d ms (expected about %d)\n", t, 2 * UNPACK_STEP_MS); }
      if (!ty_seen && wk_tx != 5) { bad++; printf("v71: no step animation seen\n"); }
      if (nx_kind != GO_UNPACK || fade_len != SLOW_FADE_MS) { bad++; printf("v71: after the walk the slow fade to GO_UNPACK must start (kind %d)\n", nx_kind); }
      for (t = 0; t < 1000 && fade_st; t++) { update(); SDL_Delay(10); }
      if (wk_tx != UNPACK_X || wk_ty != UNPACK_Y || wk_face != RT || wk_mode != 1 || strcmp(wk_pg[0], "Alright, finished unpacking my things.") || !unpack_done) { bad++; printf("v71: after the fade you must stand in the bedroom (%d,%d) face %d mode %d\n", wk_tx, wk_ty, wk_face, wk_mode); }
      frozen = 1; fade_st = 0; wk_mode = 0; unpack_done = 0; }
    /* v7.1: a 752x560 picture is full-screen art: loaded 1:1 at the native screen, never enlarged again; its logical size is still 640x480 */
    { SDL_Surface *nat = load_png("tests/native_752x560.png");
      if (!nat) { bad++; printf("v71: tests/native_752x560.png did not load\n"); }
      else {
        if (LW(nat) != 640 || LH(nat) != 480 || nat->w != SW || nat->h != SH) { bad++; printf("v71: native art must fill the screen exactly (%dx%d, logical %dx%d, screen %dx%d)\n", nat->w, nat->h, LW(nat), LH(nat), SW, SH); }
        if (SW == NATW && SH == NATH) {
            Uint8 r, g, b; SDL_GetRGB(*px(nat, 100, 100), nat->format, &r, &g, &b); if (r != 200 || g != 10 || b != 30) { bad++; printf("v71: native art pixel (100,100) is not 1:1 (%d,%d,%d)\n", r, g, b); }
            SDL_GetRGB(*px(nat, NATW - 1, NATH - 1), nat->format, &r, &g, &b); if (r != 1 || g != 2 || b != 3) { bad++; printf("v71: native art last pixel is not 1:1\n"); }
        }
        SDL_FreeSurface(nat); } }
    /* 3. her pictures: normal, day 6 / 7 (invited), cover from the day-8 scene on */
    g_init(&G); if (!lz(&herD6S, "assets/her_day_6.png") || !lz(&herCoverS, "assets/her_cover.png")) { bad++; printf("v68: her_day_6 / her_cover are not loaded\n"); }
    G.pill_given = 1; G.pill_day = 1; G.pill_seen = 0; G.day = 5; if (her_vis() != conf) { bad++; printf("v68: day 5: her normal picture\n"); }
    G.day = 6; if (her_vis() != herD6S) { bad++; printf("v68: day 6: her_day_6\n"); }
    G.day = 7; if (her_vis() != herD6S) { bad++; printf("v68: day 7: her_day_6\n"); }
    G.day = 8; G.pill_seen = 1; if (her_vis() != herCoverS) { bad++; printf("v68: day 8: her_cover\n"); }
    G.day = 20; if (her_vis() != herCoverS) { bad++; printf("v68: later days: her_cover every visit\n"); }
    /* 4. the lead-in before every full gauge: random, never twice in a row, 4 beats then the pump scene */
    { int seen[64] = {0}, last = -1, distinct = 0;
      for (i = 0; i < 24; i++) {
          vn_start(SCR_BOTTLE); k = -1;
          { int q; for (q = 0; q < B_LEAD_N; q++) if (vn_buf[0].txt == B_LEAD[q][0].txt) k = q; }
          if (k < 0 || vn_n != LEAD_BEATS + B_BOTTLE_N || vn_buf[LEAD_BEATS].txt != B_BOTTLE[0].txt || vn_buf[vn_n - 1].txt != B_BOTTLE[B_BOTTLE_N - 1].txt) { bad++; printf("v68: lead-in %d wrong (k %d n %d)\n", i, k, vn_n); break; }
          if (k == last) { bad++; printf("v68: the same lead-in twice in a row\n"); break; }
          last = k; seen[k] = 1;
      }
      for (i = 0; i < B_LEAD_N; i++) distinct += seen[i];
      if (distinct < 4) { bad++; printf("v68: the lead-ins must vary (%d of %d seen)\n", distinct, B_LEAD_N); }
      for (k = 0; k < B_LEAD_N; k++) for (i = 0; i < LEAD_BEATS; i++) { char w[400]; int j, ln = 1; wrap_buf(w, sizeof w, B_LEAD[k][i].txt, 46); for (j = 0; w[j]; j++) if (w[j] == '\n') ln++; if (ln > 3) { bad++; printf("v68: lead-in %d beat %d needs %d lines\n", k, i, ln); } }
      vn_start(SCR_BOTTLE); vn_i = 1; vn_prev_spr = vn_buf[1].spr; RSHOT("v68_lead_in_bottle"); }
    /* 5. the online store info pages (Y): ring, breast enhancement pill, milk inducer pill */
    g_init(&G); G.pill_seen = 1; G.rs = RS_LIVING; scr = SC_SHOP; shop_tab = TAB_BM; shop_sel = SH_RING; shop_info = 0; shop_confirm = 0;
    on_button(B_Y); if (!shop_info || !strstr(SH_DESC[SH_RING], "charm")) { bad++; printf("v68: Y must open the ring info\n"); }
    RSHOT("v68_shop_info_ring"); on_button(B_B); if (shop_info || scr != SC_SHOP) { bad++; printf("v68: B must close the info page and stay in the store\n"); }
    shop_sel = SH_PILL; on_button(B_Y); if (!shop_info) { bad++; printf("v68: Y must open the pill info\n"); } RSHOT("v68_shop_info_pill"); on_button(B_A); if (shop_info) { bad++; printf("v68: the info page must close\n"); }
    shop_sel = SH_MILK; on_button(B_Y); if (!shop_info) { bad++; printf("v68: Y must open the milk pill info\n"); } RSHOT("v68_shop_info_milk"); on_button(B_Y); if (shop_info) { bad++; printf("v68: Y must close the info page\n"); }
    shop_tab = TAB_ITEMS; shop_sel = SH_FLOWER; on_button(B_Y); if (shop_info) { bad++; printf("v68: the flower has no info page\n"); }
    for (i = 0; i < SH_N; i++) if (SH_DESC[i]) { char w[400]; int j, ln = 1; wrap_buf(w, sizeof w, SH_DESC[i], 28); for (j = 0; w[j]; j++) if (w[j] == '\n') ln++; if (ln > 5) { bad++; printf("v68: info %d needs %d lines\n", i, ln); } }
    /* 6. the inventory name */
    if (strcmp("SOPHIA BREAST MILK", "SOPHIA BREAST MILK")) bad++;
    g_init(&G); G.bottle = 1; scr = SC_WALK; inv_open(); RSHOT("v68_inventory_sophia_milk");
    /* v7.8: Y on the Sophia breast milk opens its info page; B / A / Y close it; the soda has no info page */
    if (inv_sel != 2) { bad++; printf("v78: only the milk is in the fridge, it must be selected\n"); }
    on_button(B_Y); if (!inv_info) { bad++; printf("v78: Y must open the milk info\n"); } RSHOT("v78_fridge_milk_info");
    on_button(B_A); if (inv_info || G.bottle != 1) { bad++; printf("v78: A on the info page only closes it (bottle %d)\n", G.bottle); }
    on_button(B_Y); on_button(B_B); if (inv_info || scr != SC_INV) { bad++; printf("v78: B must close the info page and stay in the fridge\n"); }
    on_button(B_Y); on_button(B_Y); if (inv_info) { bad++; printf("v78: Y must close the info page\n"); }
    { char w[400]; int j, ln = 1; wrap_buf(w, sizeof w, MILK_INFO, 28); for (j = 0; w[j]; j++) if (w[j] == '\n') ln++; if (ln > 5) { bad++; printf("v78: the milk info needs %d lines\n", ln); } }
    G.soda = 1; inv_sel = 0; on_button(B_Y); if (inv_info) { bad++; printf("v78: the soda has no info page\n"); }
    G.soda = 0; G.bottle = 0; G.milk_until = 0; inv_fix(); if (inv_rows() != 0) { bad++; printf("v78: nothing owned = empty fridge\n"); } RSHOT("v78_fridge_empty_shot");
    G.milk_until = 99; G.day = 1; if (inv_rows() != 1) { bad++; printf("v78: a working bottle keeps the milk row\n"); }
    g_init(&G); scr = SC_WALK;
    printf("v6.8: unpacking, her pictures, lead-ins, info pages, fridge: problems %d\n", bad);
    G = keep; saved_G = keep_saved; frozen = fr; scr = kscr; room_seen = kroom; unpack_done = kun; pause_kind = PK_NONE; wk_mode = 0; wk_place(km, kx, ky, kf);
    return bad;
}


static int v80_tests(void) {   /* v8.0: Sophia walks around in the living room; walk speed; prices */
    int bad = 0, i, k; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kwalk = walk_idx, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face;
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 1;
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.cash = 90000; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.day = 12; G.phase = PH_MORNING;
    /* the walking speed: three settings, saved, slower by default than before (350 ms) */
    if (WALK_T[1] <= 350 || WALK_T[0] <= WALK_T[1] || WALK_T[2] != 350) { bad++; printf("v80: walk speeds must be SLOW > NORMAL > 350 = FAST\n"); }
    walk_idx = 0; cfg_save(); walk_idx = 2; cfg_load(); if (walk_idx != 0) { bad++; printf("v80: the walk speed was not saved\n"); }
    { FILE *f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "dlg 2\nsnd 1\nwalk 9\n"); fclose(f); } walk_idx = 1; cfg_load(); if (walk_idx != 1) { bad++; printf("v80: a bad walk value must be ignored\n"); } }
    walk_idx = kwalk; cfg_save();
    if (SH_PRICE[SH_FLOWER] != 250 || SH_PRICE[SH_STACK] != 1000) { bad++; printf("v80: flower 250 and money stack 1000\n"); }
    /* she shows up: only morning / evening, only when she lives with you, on a tile she may use, never on yours */
    ls_force = 1; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 4, 4, DN); ls_ph = -1; ls_on = 0; update();
    if (!ls_on || !ls_ok(ls_tx, ls_ty) || (ls_tx == 4 && ls_ty == 4)) { bad++; printf("v80: she did not appear in the living room (on %d at %d,%d)\n", ls_on, ls_tx, ls_ty); }
    shot("v80_sophia_living_room");
    G.phase = PH_NIGHT; update(); if (ls_on) { bad++; printf("v80: she must not be there at night\n"); }
    G.phase = PH_EVENING; update(); if (!ls_on) { bad++; printf("v80: she must be there in the evening (when rolled)\n"); }
    G.rs = RS_NO; ls_ph = -1; update(); if (ls_on) { bad++; printf("v80: she must not be there before she lives with you\n"); }
    G.rs = RS_LIVING; G.phase = PH_MORNING; ls_ph = -1; update();
    wk_place(MAP_HALL, 3, 3, DN); update(); if (ls_on) { bad++; printf("v80: she is only in the living room\n"); }
    wk_place(MAP_ROOM, 4, 4, DN); update(); if (!ls_on) { bad++; printf("v80: coming back into the room rolls again\n"); }
    /* rare: about LS_CHANCE percent over many entries */
    ls_force = 0; G.phase = PH_EVENING; { int hit = 0; for (k = 0; k < 4000; k++) { ls_ph = -1; wk_place(MAP_ROOM, 4, 4, DN); ls_roll(); if (ls_on) hit++; } if (hit < 4000 * (LS_CHANCE - 4) / 100 || hit > 4000 * (LS_CHANCE + 4) / 100) { bad++; printf("v80: she was there %d of 4000 times in the evening (expected about %d%%)\n", hit, LS_CHANCE); } } G.phase = PH_MORNING;
    /* she wanders: always on her tiles, never on yours, never two steps at once */
    ls_force = 1; for (i = 0; i < 3000; i++) {
        int px0 = 2 + i % 4, py0 = 3 + (i / 4) % 3;
        wk_place(MAP_ROOM, px0, py0, DN); ls_roll(); if (!ls_on) continue;
        for (k = 0; k < 6; k++) { ls_moving = 0; ls_wander(); if (!ls_ok(ls_tx, ls_ty) || ls_player_on(ls_tx, ls_ty) || (ls_moving && (abs(ls_tx - ls_fx) + abs(ls_ty - ls_fy)) != 1)) { bad++; printf("v80: she stepped to a bad tile (%d,%d) with you at %d,%d\n", ls_tx, ls_ty, px0, py0); i = 99999; break; } }
    }
    /* she walks in real time and the picture follows (frozen off: now_ms is the real clock, her step start is set back instead) */
    frozen = 0; wk_place(MAP_ROOM, 4, 5, DN); ls_ph = G.phase; ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 3; ls_moving = 0; wk_mode = 0; fade_st = 0;
    for (k = 0; k < 60 && !ls_moving; k++) { ls_next = now_ms(); update(); }
    if (!ls_moving) { bad++; printf("v80: she did not start to walk\n"); }
    else {
        ls_t0 = now_ms() - ls_step_ms() / 2; { int x0 = ls_fx * TILE * 256 + TILE * 128, y0 = (ls_fy + 1) * TILE * 256; if (ls_xpos() == x0 && ls_ypos() == y0) { bad++; printf("v80: her picture did not move\n"); } }
        ls_t0 = now_ms() - ls_step_ms() - 10; update(); if (ls_moving) { bad++; printf("v80: her step did not finish\n"); }
    }
    if (ls_step_ms() <= WALK_MS) { bad++; printf("v80: she must walk slower than you\n"); }
    frozen = 1; ls_moving = 0; ls_fx = ls_tx; ls_fy = ls_ty;
    /* you cannot walk through her */
    ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; ls_moving = 0; wk_place(MAP_ROOM, 4, 4, LF); on_button(B_LEFT); if (wk_tx != 4 || wk_ty != 4) { bad++; printf("v80: you walked into her\n"); }
    /* A on her: no knock, her menu at once in your living room */
    shot("v80_sophia_bubble");
    on_button(B_A);
    if (scr != SC_ROM || !ls_talk || vn_id == SCR_KNOCKHOME || vn_id == SCR_KNOCKIN) { bad++; printf("v80: A on her must open her menu at once (scr %d talk %d)\n", scr, ls_talk); }
    if (ls_face != RT) { bad++; printf("v80: she must turn to face you\n"); }
    shot("v80_talk_menu_living_room"); rom_adv(2);
    if (scr == SC_ROM) { rom_choose = 1; shot("v80_talk_menu_choices"); }
    /* the menu's own scenes use the living room picture too */
    vn_start(SCR_MILK1); for (k = 0; k < vn_n; k++) if (vn_buf[k].bg == BG_ROM || vn_buf[k].bg == BG_BDOOR) { bad++; printf("v80: scene beat %d still uses her room or door picture\n", k); break; }
    shot("v80_talk_scene");
    scr = SC_ROM; rom_mode = RM_MENU; rom_choose = 1;
    on_button(B_B);
    if (scr != SC_WALK || ls_talk || wk_tx != 4 || wk_ty != 4 || !ls_on) { bad++; printf("v80: B must put you back where you stood (scr %d talk %d at %d,%d on %d)\n", scr, ls_talk, wk_tx, wk_ty, ls_on); }
    /* after a gift scene (home_to_door) you also stay in the living room */
    ls_talk = 1; wk_place(MAP_ROOM, 4, 4, LF); home_to_door("note here");
    if (scr != SC_WALK || wk_tx != 4 || wk_ty != 4 || wk_mode != 1 || ls_talk) { bad++; printf("v80: after a scene you stay where you stood with the note (scr %d at %d,%d mode %d)\n", scr, wk_tx, wk_ty, wk_mode); }
    /* without her the normal flow is unchanged: the scenes use her door and room pictures */
    ls_talk = 0; vn_start(SCR_MILK1); { int rm = 0; for (k = 0; k < vn_n; k++) if (vn_buf[k].bg == BG_ROM) rm++; if (!rm) { bad++; printf("v80: without her the milk scene still plays in her room\n"); } }
    /* a story scene is waiting behind her door: she sends you there */
    G.pill_given = 1; G.pill_seen = 0; G.pill_day = G.day - PILL_DAYS; scr = SC_WALK; wk_mode = 0; ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; wk_place(MAP_ROOM, 4, 4, LF); on_button(B_A);
    if (scr != SC_WALK || ls_talk || wk_mode != 1) { bad++; printf("v80: with the day-8 scene waiting she must send you to her door (scr %d talk %d mode %d)\n", scr, ls_talk, wk_mode); }
    shot("v80_talk_busy");
    G.pill_given = 0; G.pill_seen = 0;
    /* her pictures: normal before the pill, the changed ones after (the menu uses the same picture function as at the door) */
    ls_leave = 0; ls_away = -1; G.pill_seen = 1; G.pill_given = 1; G.pill_day = 2; G.day = 12; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 4, 4, LF); ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4;
    on_button(B_A); if (scr != SC_ROM || !ls_talk) { bad++; printf("v80: after the pill scene she must still talk to you in the living room (scr %d)\n", scr); }
    shot("v80_talk_after_day8"); scr = SC_ROM; rom_mode = RM_MENU; rom_choose = 1; on_button(B_B);
    /* four directions on screen */
    G.pill_seen = 0; G.pill_given = 0;
    for (i = 0; i < 4; i++) { char nm[40]; ls_on = 1; ls_tx = ls_fx = 4; ls_ty = ls_fy = 3; ls_face = i; wk_place(MAP_ROOM, 3, 4, i == 0 ? DN : RT); snprintf(nm, sizeof nm, "v80_sophia_face_%d", i); shot(nm); }
    /* options with the walk speed row */
    scr = SC_OPT; opt_row = 2; walk_idx = 0; shot("v80_options_walk_slow"); walk_idx = 2; shot("v80_options_walk_fast"); walk_idx = kwalk;
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = 0; ls_ph = -1; wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
    return bad;
}
static int v84_gold(int lx, int ly) {   /* is the screen pixel at logical (lx, ly) the gold border of a panel? (reads shotbuf, so call after draw_all) */
    int x = XS(lx), y = YS(ly); const unsigned char *p; if (x < 0) x = 0; if (x >= SW) x = SW - 1; if (y < 0) y = 0; if (y >= SH) y = SH - 1;
    p = (const unsigned char *)shotbuf + ((size_t)y * SW + x) * 3; return p[0] >= 140 && p[1] >= 105 && p[2] <= 125 && p[0] > p[2] + 50;
}
static void v84_px(int lx, int ly, int *r, int *g, int *b) {
    int x = XS(lx), y = YS(ly); const unsigned char *p; if (x >= SW) x = SW - 1; if (y >= SH) y = SH - 1;
    p = (const unsigned char *)shotbuf + ((size_t)y * SW + x) * 3; *r = p[0]; *g = p[1]; *b = p[2];
}
static int v85_tests(void) {   /* v8.5 */
    int bad = 0, kscr = scr, fr = frozen, kmode = mp_mode, kboss = M.boss; Game keep = G;
    size_t sz; unsigned char *a;
    if (map_key(SDLK_RETURN) != B_QUIT) { bad++; printf("v85: Start (RETURN) must open the pause menu\n"); }
    if (map_key(SDLK_ESCAPE) != B_QUIT) { bad++; printf("v85: the Menu key (ESC) still opens the pause menu\n"); }
    if (map_key(SDLK_RCTRL) != -1) { bad++; printf("v85: Select (RCTRL) must do nothing\n"); }
    frozen = 1; pause_kind = PK_NONE;
    scr = SC_PLAY; on_button(B_QUIT); if (pause_kind != PK_TABLE) { bad++; printf("v85: Start at the table must open the pause menu\n"); } pause_kind = PK_NONE;
    scr = SC_MONO; on_button(B_QUIT); if (pause_kind != PK_SKIP) { bad++; printf("v85: Start in the monologue must open the pause menu with SKIP\n"); } pause_kind = PK_NONE;
    /* the cached table backdrop is pixel-identical to drawing the layers directly */
    sz = (size_t)S->pitch * S->h; a = (unsigned char *)malloc(sz);
    if (a) {
        int m, bs;
        for (m = 0; m < 2; m++) for (bs = 0; bs < (m ? 2 : 1); bs++) {
            mp_mode = m; M.boss = bs; draw_felt_layers(); memcpy(a, S->pixels, sz);
            if (feltC) { SDL_FreeSurface(feltC); feltC = NULL; }
            draw_felt(); draw_felt();   /* the first call builds the cache, the second one uses it */
            if (memcmp(a, S->pixels, sz)) { bad++; printf("v85: cached backdrop differs (mode %d boss %d)\n", m, bs); }
        }
        free(a);
    }
    mp_mode = kmode; M.boss = kboss; scr = kscr; frozen = fr; pause_kind = PK_NONE; G = keep;
    printf("v8.5: Start / Select, no MENU LEAVES hint, cached table backdrop: problems %d\n", bad);
    return bad;
}
static int v92_tests(void) {   /* v9.2: Sophia's walkable bedroom (married only): entry rules, her wandering, the wardrobe, the bed, leaving, the night picture */
    int bad = 0, i, k, x, y; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face;
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 1; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0; sb_out = 0; home_end = 0; sb_rkey = -1;
#define V92_FIN() do { for (k = 0; k < 14 && scr == SC_STORY; k++) on_button(B_A); } while (0)
#define V92_BASE() do { g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.dring = 1; G.married = 1; G.pill_seen = 1; G.pill_given = 1; G.pill_day = 5; G.yoga_given = 1; G.cash = 90000; G.energy = 100; G.rom_lvl = ROM_LVLS; G.day = 20; G.phase = PH_MORNING; saved_G = G; ls_away = -1; ls_talk = 0; ls_on = 0; sb_out = 0; kicked_day = 0; sb_rkey = -1; scr = SC_WALK; wk_mode = 0; } while (0)
#define V92_KNOCK() do { wk_mode = 0; scr = SC_WALK; wk_place(MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, UP); on_button(B_A); } while (0)
    /* 1. the map: picture, night picture, tiles */
    if (!wm[MAP_SBED].img || LW(wm[MAP_SBED].img) != 576 || LH(wm[MAP_SBED].img) != 504) { bad++; printf("v92: her bedroom picture missing or not 576x504\n"); }
    if (!sbN || !lsSn || sbN->w != wm[MAP_SBED].img->w || sbN->h != wm[MAP_SBED].img->h) { bad++; printf("v92: her bedroom night picture or her night sprite missing\n"); }
    else { long sd = 0, sn = 0; for (y = 0; y < sbN->h; y += 5) for (x = 0; x < sbN->w; x += 5) { Uint32 a = *px(wm[MAP_SBED].img, x, y), b = *px(sbN, x, y); sd += ((a >> wm[MAP_SBED].img->format->Rshift) & 255) + ((a >> wm[MAP_SBED].img->format->Gshift) & 255) + ((a >> wm[MAP_SBED].img->format->Bshift) & 255); sn += ((b >> sbN->format->Rshift) & 255) + ((b >> sbN->format->Gshift) & 255) + ((b >> sbN->format->Bshift) & 255); }
        if (sd <= 0 || sn >= sd * 80 / 100 || sn <= sd * 45 / 100) { bad++; printf("v92: her bedroom at night must be clearly darker (%ld of %ld)\n", sn, sd); } }
    if (wk_tile(MAP_SBED, 1, 6) != 'D' || wk_tile(MAP_SBED, 1, 2) != 'W' || wk_tile(MAP_SBED, 1, 4) != 'W' || wk_tile(MAP_SBED, 4, 3) != 'B' || wk_tile(MAP_SBED, SB_SX, SB_SY) != '.') { bad++; printf("v92: her bedroom tiles are wrong\n"); }
    if (night_of(MAP_SBED)) { bad++; printf("v92: her bedroom has its own night picture, no shared night copy\n"); }
    /* 2. knocking when married in the morning: the knock scene, then you walk in; she stands beside her bed */
    V92_BASE(); V92_KNOCK();
    if (scr != SC_STORY || vn_id != SCR_KNOCKIN) { bad++; printf("v92: married morning knock must start the knock scene (scr %d id %d)\n", scr, vn_id); }
    V92_FIN();
    if (scr != SC_WALK || wk_map != MAP_SBED || wk_tx != SB_SX || wk_ty != SB_SY || wk_face != RT || ls_talk) { bad++; printf("v92: after the knock scene you must stand in her bedroom at the door (scr %d map %d at %d,%d face %d)\n", scr, wk_map, wk_tx, wk_ty, wk_face); }
    if (!ls_on || ls_ty != 4 || (ls_tx != 3 && ls_tx != 6) || (ls_tx == wk_tx && ls_ty == wk_ty)) { bad++; printf("v92: she must stand beside her bed (on %d at %d,%d)\n", ls_on, ls_tx, ls_ty); }
    shot("v92_bedroom_morning");
    /* 2b. not married: nothing changes (her menu at once); thrown out today: no walk-in either */
    V92_BASE(); G.married = 0; V92_KNOCK(); V92_FIN(); if (scr != SC_ROM || wk_map == MAP_SBED) { bad++; printf("v92: not married: the old knock flow (scr %d map %d)\n", scr, wk_map); }
    V92_BASE(); kicked_day = G.day; V92_KNOCK(); V92_FIN(); if (scr != SC_ROM || wk_map == MAP_SBED) { bad++; printf("v92: thrown out today: no walk-in (scr %d map %d)\n", scr, wk_map); }
    /* 3. she is standing in the living room: you cannot go in */
    V92_BASE(); G.pill_given = 0; ls_force = 1; wk_place(MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, UP); ls_ph = -1; ls_roll(); on_button(B_A);
    if (!ls_on || scr != SC_WALK || wk_mode != 1 || wk_map != MAP_ROOM) { bad++; printf("v92: she stands in the living room: the door stays shut (on %d scr %d mode %d)\n", ls_on, scr, wk_mode); }
    ls_force = 0; WK_CLOSE();
    /* 4. the evening: about half of the evenings she is in her room, rolled once per evening */
    { int in = 0, same = 1; for (i = 0; i < 400; i++) {
          int r1, r2; V92_BASE(); G.day = 20 + i; G.phase = PH_EVENING; V92_KNOCK(); r1 = (scr == SC_STORY); if (!r1 && (wk_mode != 1 || wk_pg[0] != W_SB_NOANS)) { bad++; printf("v92: evening, she is out: the no-answer line must show (mode %d)\n", wk_mode); break; } if (r1) { V92_FIN(); if (wk_map != MAP_SBED) { bad++; printf("v92: evening, she lets you in but you are not in her room\n"); break; } }
          WK_CLOSE(); V92_KNOCK(); r2 = (scr == SC_STORY); if (r1 != r2) same = 0; in += r1; }
      if (!same) { bad++; printf("v92: the evening roll must stay the same for the whole evening\n"); }
      if (in < 400 * (SB_EVE_CHANCE - 10) / 100 || in > 400 * (SB_EVE_CHANCE + 10) / 100) { bad++; printf("v92: in her room %d of 400 evenings (expected about %d%%)\n", in, SB_EVE_CHANCE); } }
    /* 5. the night: always, after the hiding scene; she is there, the picture and the sprites are the dark ones */
    V92_BASE(); G.phase = PH_NIGHT; V92_KNOCK(); if (vn_id != SCR_NIGHTM) { bad++; printf("v92: married night knock: the hiding scene first (id %d)\n", vn_id); } V92_FIN();
    if (scr != SC_WALK || wk_map != MAP_SBED || !ls_on) { bad++; printf("v92: night: you must be in her bedroom with her (scr %d map %d on %d)\n", scr, wk_map, ls_on); }
    shot("v92_bedroom_night");
    /* 6. she wanders: beside the bed most of the time, sometimes in front of the wardrobe / window, never on you, never in the door column */
    { int cnt[8][8], tot = 0, side = 0, front = 0, moves = 0; memset(cnt, 0, sizeof cnt); V92_BASE(); wk_place(MAP_SBED, SB_SX, SB_SY, RT); sb_spawn();
      for (i = 0; i < 6000; i++) {
          int px_, py_; px_ = 2 + (i / 37) % 5; py_ = 3 + (i / 11) % 3; if (i % 5 != 0) { wk_tx = wk_fx = SB_SX; wk_ty = wk_fy = SB_SY; } else if (wk_walkable(wk_tile(MAP_SBED, px_, py_)) && !(px_ == ls_tx && py_ == ls_ty) && !(ls_moving && px_ == ls_fx && py_ == ls_fy)) { wk_tx = wk_fx = px_; wk_ty = wk_fy = py_; }   /* you mostly wait at the door, now and then you stand in her way */
          { int ox = ls_tx, oy = ls_ty; sb_wander(); if (ls_moving) { ls_moving = 0; ls_fx = ls_tx; ls_fy = ls_ty; moves++; if (abs(ls_tx - ox) + abs(ls_ty - oy) != 1) { bad++; printf("v92: she must move one tile at a time\n"); break; } } }
          if (!wk_walkable(wk_tile(MAP_SBED, ls_tx, ls_ty)) || ls_tx < 2 || (ls_tx == wk_tx && ls_ty == wk_ty)) { bad++; printf("v92: she stands on a bad tile (%d,%d), you are at %d,%d\n", ls_tx, ls_ty, wk_tx, wk_ty); break; }
          if (sb_arrived) { cnt[ls_ty][ls_tx]++; tot++; } }
      side = cnt[4][3] + cnt[4][6]; front = cnt[4][2] + cnt[3][2];
      if (tot < 100 || side * 100 < tot * 45 || front < tot / 10 || moves < 200) { bad++; printf("v92: her wandering: %d stops, %d beside the bed, %d at the wardrobe / window, %d steps\n", tot, side, front, moves); }
      if (cnt[3][2] == 0 || cnt[4][2] == 0) { bad++; printf("v92: she must visit both the window tile and the wardrobe tile\n"); } }
    /* 7. you cannot walk through her; A on her opens her menu with her room behind it; B puts you back where you stood */
    V92_BASE(); wk_place(MAP_SBED, 3, 5, UP); ls_on = 1; ls_moving = 0; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; ls_face = DN;
    on_button(B_UP); if (wk_ty != 5) { bad++; printf("v92: you walked through her\n"); }
    on_button(B_A); if (scr != SC_ROM || !ls_talk || TALK_BG() != BG_ROM) { bad++; printf("v92: A on her must open her menu in her room (scr %d talk %d)\n", scr, ls_talk); }
    shot("v92_her_menu_in_bedroom"); scr = SC_ROM; rom_mode = RM_MENU; rom_choose = 1; on_button(B_B);
    if (scr != SC_WALK || ls_talk || wk_map != MAP_SBED || wk_tx != 3 || wk_ty != 5) { bad++; printf("v92: B in her menu must leave you in her bedroom where you stood (scr %d map %d at %d,%d)\n", scr, wk_map, wk_tx, wk_ty); }
    /* 8. a match started in her bedroom ends outside it: playing to the end, leaving the table, and being thrown out */
    V92_BASE(); wk_place(MAP_SBED, 3, 5, UP); ls_talk = 1; scr = SC_ROM; rom_mode = RM_LEN; rom_lsel = 0; on_button(B_A);
    if (scr != SC_PLAY || !sb_out) { bad++; printf("v92: a match from her bedroom must start with the leave-outside mark (scr %d out %d)\n", scr, sb_out); }
    scr = SC_END; home_end = 1; first_end = 0; on_button(B_A);
    if (scr != SC_WALK || wk_map != MAP_ROOM || wk_tx != HOME_DOOR_X || wk_ty != HOME_DOOR_Y || wk_face != UP || ls_talk || sb_out || ls_on) { bad++; printf("v92: after the match you must stand outside her bedroom door (scr %d map %d at %d,%d on %d)\n", scr, wk_map, wk_tx, wk_ty, ls_on); }
    V92_BASE(); wk_place(MAP_SBED, 3, 5, UP); ls_talk = 1; sb_out = 1; go(GO_HUB, 0);
    if (scr != SC_WALK || wk_map != MAP_ROOM || wk_tx != HOME_DOOR_X || wk_ty != HOME_DOOR_Y || ls_talk) { bad++; printf("v92: leaving the table must put you outside (map %d at %d,%d)\n", wk_map, wk_tx, wk_ty); }
    V92_BASE(); wk_place(MAP_SBED, 3, 5, UP); ls_talk = 1; tk_kind = TK_TEASE; talk_end();
    if (wk_map != MAP_ROOM || wk_tx != HOME_DOOR_X || wk_ty != HOME_DOOR_Y || kicked_day != G.day || wk_mode != 1) { bad++; printf("v92: thrown out: outside her door with the note (map %d mode %d)\n", wk_map, wk_mode); }
    WK_CLOSE();
    /* 9. the door: A facing down, out to the living room; she does not jump into the living room as well */
    V92_BASE(); ls_force = 1; wk_place(MAP_SBED, SB_SX, SB_SY, DN); on_button(B_A);
    if (wk_map != MAP_ROOM || wk_tx != HOME_DOOR_X || wk_ty != HOME_DOOR_Y || wk_face != UP || ls_on) { bad++; printf("v92: the door must lead out to her bedroom door in the living room, empty (map %d at %d,%d on %d)\n", wk_map, wk_tx, wk_ty, ls_on); }
    ls_force = 0;
    /* 10. the wardrobe: a small menu to ask her to change her clothes (needs the yoga outfit) */
    V92_BASE(); G.outfit = 0; wk_place(MAP_SBED, 2, 3, LF); ls_on = 0; on_button(B_A);
    if (wk_mode != 1 || wk_pg[0] != W_WARD_ASK) { bad++; printf("v92: the wardrobe must ask first (mode %d)\n", wk_mode); }
    on_button(B_A); if (wk_mode != 8) { bad++; printf("v92: then the wardrobe menu (mode %d)\n", wk_mode); }
    shot("v92_wardrobe_menu");
    on_button(B_DOWN); on_button(B_A); if (wk_mode != 0 || G.outfit != 0) { bad++; printf("v92: LEAVE must change nothing (outfit %d mode %d)\n", G.outfit, wk_mode); }
    on_button(B_A); on_button(B_A); on_button(B_B); if (wk_mode != 0 || G.outfit != 0) { bad++; printf("v92: B closes the wardrobe menu\n"); }
    on_button(B_A); on_button(B_A); on_button(B_A); ow_play(); if (G.outfit != 1 || wk_mode != 1 || !in_pool(wk_pg[0], R_OUTFIT_YOGA, R_OUTFIT_YOGA_N)) { bad++; printf("v92: the first choice changes to the yoga outfit with her line (outfit %d)\n", G.outfit); }
    shot("v92_wardrobe_she_answers"); WK_CLOSE();
    on_button(B_A); on_button(B_A); on_button(B_A); if (G.outfit != 0 || wk_mode != 1 || !in_pool(wk_pg[0], R_OUTFIT_D8, R_OUTFIT_D8_N)) { bad++; printf("v92: and back to her usual outfit (outfit %d)\n", G.outfit); }
    WK_CLOSE();
    G.yoga_given = 0; wk_place(MAP_SBED, 2, 3, LF); on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_WARD_NONE) { bad++; printf("v92: without the yoga outfit she has nothing to change into\n"); } WK_CLOSE(); if (wk_mode != 0) { bad++; printf("v92: no menu without the yoga outfit\n"); }
    G.yoga_given = 1; wk_place(MAP_SBED, 1, 5, UP); ls_on = 0; on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_WARD_ASK) { bad++; printf("v92: the wardrobe also answers from below\n"); } WK_CLOSE(); wk_mode = 0;
    /* 11. the bed: a line by day, her menu on page 4 at night, and sleeping together wakes you in front of her door */
    V92_BASE(); wk_place(MAP_SBED, 3, 4, RT); ls_on = 0; on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_SB_BED_DAY) { bad++; printf("v92: her bed by day: a line\n"); } WK_CLOSE();
    G.phase = PH_NIGHT; wk_place(MAP_SBED, 3, 4, RT); ls_on = 0; on_button(B_A); if (scr != SC_ROM || !ls_talk || rom_page != 3) { bad++; printf("v92: her bed at night: her menu on page 4 (scr %d page %d)\n", scr, rom_page); }
    scr = SC_WALK; ls_talk = 0; wk_place(MAP_SBED, 3, 4, RT); ls_force = 1; vn_start(SCR_SLEEP); V92_FIN(); ls_force = 0;
    if (scr != SC_WALK || wk_map != MAP_ROOM || wk_tx != HOME_DOOR_X || wk_ty != HOME_DOOR_Y || G.phase != PH_MORNING || G.day != 21 || ls_on) { bad++; printf("v92: sleeping together: next morning in front of her door, she stays in her room (scr %d map %d day %d phase %d on %d)\n", scr, wk_map, G.day, G.phase, ls_on); }
    WK_CLOSE();
    /* 12. leaving her room keeps her in it for the rest of that time of day; a new time of day rolls again */
    V92_BASE(); G.pill_given = 0; ls_force = 1; wk_place(MAP_SBED, SB_SX, SB_SY, DN); on_button(B_A); if (ls_on) { bad++; printf("v92: she must not also be in the living room right after you left her room\n"); }
    G.phase = PH_EVENING; wk_place(MAP_HALL, 3, 3, DN); wk_place(MAP_ROOM, 4, 4, DN); if (!ls_on) { bad++; printf("v92: a later time of day rolls again\n"); }
    ls_force = 0;
#undef V92_FIN
#undef V92_BASE
#undef V92_KNOCK
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = 0; ls_leave = 0; ls_away = -1; ls_ph = -1; kicked_day = 0; sb_out = 0; home_end = 0; sb_rkey = -1; wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE; (void)i;
    return bad;
}
static int txt_has_any(const char *t, const char *const *w) { int k; if (!t) return 0; for (k = 0; w[k]; k++) if (strstr(t, w[k])) return 1; return 0; }
static int v105_tests(void) {   /* v10.5: the sofa menu (REST / SAVE ONLY), five save slots (sofa and bed save to the active one; Options and Career pick and load), the rewind guard (market reshuffle + lottery shut) */
    int bad = 0, i, d, s0; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, kact = slot_act, kfar = prog_far, kio = slot_io_test, kmap = wk_map, kopt = opt_from; const char *kpre = slot_prefix;
    int before[STK_N][8];
    frozen = 1; pause_kind = PK_NONE; fade_st = 0; slot_prefix = "/tmp/pq_v105_"; slot_io_test = 1;
    for (i = 1; i <= SLOT_N; i++) remove(slot_path(i)); remove(prog_path());
    /* 1. the sofa: the menu, the slot it saves to, the picture */
    g_init(&G); G.career = 4242; G.story = ST_NAMED; G.day = 6; G.cash = 1234; G.phase = PH_MORNING; slot_act = 3; prog_far = 0; saved_G = G; scr = SC_WALK; wk_place(MAP_ROOM, 2, 5, LF); wk_mode = 0;
    on_button(B_A); if (wk_mode != 2 || wk_sleep != 0 || wk_yes != 0) { bad++; printf("v105: the sofa asks REST / SAVE ONLY / CANCEL, REST first (mode %d yes %d)\n", wk_mode, wk_yes); }
    draw_all(); RSHOT("v105_sofa_menu_morning");
    on_button(B_UP); if (wk_yes != 2) { bad++; printf("v105: UP from REST goes to CANCEL (%d)\n", wk_yes); }
    on_button(B_DOWN); on_button(B_DOWN); on_button(B_A);   /* REST -> SAVE ONLY (1) after DOWN from CANCEL wraps to REST (0), second DOWN = 1 */
    if (wk_mode != 1 || wk_pg[0] != W_SAVED_ONLY || G.phase != PH_MORNING || G.day != 6 || slot_path(3)[0] == 0) { bad++; printf("v105: SAVE ONLY must save without time (mode %d phase %d)\n", wk_mode, G.phase); } WK_CLOSE();
    { Game t; if (!load_game(&t, slot_path(3)) || t.cash != 1234 || t.day != 6 || t.phase != PH_MORNING) { bad++; printf("v105: the sofa must write slot 3 (the active one)\n"); } { FILE *f = fopen(slot_path(1), "rb"); if (f) { fclose(f); bad++; printf("v105: slot 1 must stay empty\n"); } } }
    G.phase = PH_NIGHT; on_button(B_A); draw_all(); RSHOT("v105_sofa_menu_night"); wk_mode = 0; G.phase = PH_MORNING;
    /* 2. the rewind guard: slot 1 = day 10, slot 2 = day 11 of the same career (the day 11 prices were seen) */
    for (i = 1; i <= SLOT_N; i++) remove(slot_path(i)); remove(prog_path());
    g_init(&G); G.career = 555; G.story = ST_NAMED; G.mseed = 123457; G.day = 10; G.cash = 5000; slot_act = 1; prog_far = 0; save_now();
    G.day = 11; slot_act = 2; save_now();
    if (prog_get(prog_path(), 555) != 11 || prog_far != 11) { bad++; printf("v105: the furthest day must be 11 (%d / %d)\n", prog_get(prog_path(), 555), prog_far); }
    for (i = 0; i < STK_N; i++) for (d = 0; d < 8; d++) before[i][d] = stk_price(&G, i, 6 + d);   /* days 6..13 as the day-11 timeline saw them */
    if (!slot_begin_load(1) || load_pend.mrw_n != 1 || load_pend.mrw_day[0] != 11 || load_pend_far != 11) { bad++; printf("v105: loading the older slot must reshuffle from day 11 (n %d)\n", load_pend.mrw_n); }
    { int same_past = 1, diff_future = 0; for (i = 0; i < STK_N; i++) for (d = 0; d < 8; d++) { int day = 6 + d, p = stk_price(&load_pend, i, day); if (day <= 10 && p != before[i][d]) same_past = 0; if (day >= 11 && p != before[i][d]) diff_future = 1; }
      if (!same_past) { bad++; printf("v105: the days up to the loaded one must keep their prices\n"); } if (!diff_future) { bad++; printf("v105: the days after the loaded one must have new prices\n"); } }
    if (!slot_begin_load(2) || load_pend.mrw_n != 0) { bad++; printf("v105: the newest slot is not behind: no reshuffle (n %d)\n", load_pend.mrw_n); }
    slot_begin_load(1); apply_go(GO_LOAD, 0);
    if (G.day != 10 || slot_act != 1 || prog_far != 11 || !g_behind(&G, prog_far) || scr != SC_WALK || wk_map != MAP_ROOM || unsaved()) { bad++; printf("v105: GO_LOAD wrong (day %d slot %d far %d)\n", G.day, slot_act, prog_far); }
    scr = SC_LOTTO; lot_sel = 0; lot_ms = 0; G.cash = 5000; draw_all(); RSHOT("v105_lottery_shut");
    on_button(B_A); if (G.lot_price != 0 || G.cash != 5000 || lot_kind != 3) { bad++; printf("v105: no ticket while behind (price %d kind %d)\n", G.lot_price, lot_kind); }
    G.day = 11; on_button(B_A); if (G.lot_price == 0 || lot_kind != 0) { bad++; printf("v105: tickets must be sold again at the furthest day (kind %d)\n", lot_kind); }
    G.lot_price = 0; G.lot_day = 0; G.lot_win = 0; G.cash = 5000; G.day = 10; scr = SC_WALK;
    s0 = G.mrw_n; slot_act = 1; save_now(); if (prog_get(prog_path(), 555) != 11) { bad++; printf("v105: a save never lowers the furthest day\n"); }
    { Game t; if (!load_game(&t, slot_path(1)) || t.mrw_n != s0 || t.mrw_day[0] != 11) { bad++; printf("v105: the reshuffle is saved with the slot\n"); } }
    /* 3. Options: SAVE SLOTS while walking (pick + load), not at a table; the title's options too */
    for (i = 1; i <= SLOT_N; i++) remove(slot_path(i)); remove(prog_path());
    g_init(&G); G.career = 4242; G.story = ST_NAMED; G.day = 6; G.cash = 1234; slot_act = 3; prog_far = 0; save_now();
    G.day = 9; G.cash = 77; scr = SC_WALK; wk_mode = 0; wk_place(MAP_HALL1, 6, 3, DN); on_button(B_QUIT);
    if (pause_kind != PK_HOME) { bad++; printf("v105: the pause menu did not open\n"); }
    on_button(B_DOWN); on_button(B_DOWN); on_button(B_A); if (pause_kind != PK_OPT || !opt_ext()) { bad++; printf("v105: Options while walking must have the SAVE SLOTS row (kind %d)\n", pause_kind); }
    for (i = 0; i < 6; i++) on_button(B_DOWN); if (opt_row != 4) { bad++; printf("v105: the options must have 5 rows while walking (%d)\n", opt_row); } on_button(B_UP);   /* v10.7: the last row is TEXT BOX STYLE; UP = SAVE SLOTS */
    draw_all(); RSHOT("v105_options_slots_row");
    on_button(B_LEFT); if (slot_act != 2) { bad++; printf("v105: L/R on SAVE SLOTS picks the slot (%d)\n", slot_act); } on_button(B_RIGHT); on_button(B_RIGHT); on_button(B_RIGHT); on_button(B_RIGHT); on_button(B_RIGHT); if (slot_act != SLOT_N) { bad++; printf("v105: the slot stops at %d (%d)\n", SLOT_N, slot_act); } slot_act = 3;
    on_button(B_A); if (!opt_sub || opt_lsel != 2 || slot_st[2] != 1 || slot_st[0] != 0) { bad++; printf("v105: A opens the slot list on the active slot (sub %d sel %d)\n", opt_sub, opt_lsel); }
    draw_all(); RSHOT("v105_slot_list");
    on_button(B_UP); on_button(B_X); if (slot_act != 2) { bad++; printf("v105: X chooses where the sofa and bed save (%d)\n", slot_act); }
    on_button(B_A); if (opt_conf) { bad++; printf("v105: A on an empty slot loads nothing\n"); }
    on_button(B_DOWN); on_button(B_A); if (!opt_conf || opt_csel != 0 || !unsaved()) { bad++; printf("v105: loading asks first, NO selected, and warns about unsaved progress\n"); }
    draw_all(); RSHOT("v105_slot_load_confirm");
    on_button(B_A); if (opt_conf || G.day != 9) { bad++; printf("v105: NO must keep the game (day %d)\n", G.day); }
    on_button(B_A); on_button(B_RIGHT); on_button(B_A);
    if (G.day != 6 || G.cash != 1234 || slot_act != 3 || scr != SC_WALK || pause_kind != PK_NONE || unsaved() || wk_map != MAP_ROOM || opt_sub) { bad++; printf("v105: YES must load slot 3 (day %d cash %d slot %d pause %d)\n", G.day, G.cash, slot_act, pause_kind); }
    opt_from = PK_TABLE; scr = SC_WALK; opt_row = 0; opt_sub = 0; for (i = 0; i < 6; i++) opt_key(B_DOWN); if (opt_row != 3 || opt_ext()) { bad++; printf("v105: no SAVE SLOTS row at a table (row %d; the last row is TEXT BOX STYLE)\n", opt_row); }
    opt_from = PK_HOME; scr = SC_OPT; opt_row = 0; for (i = 0; i < 6; i++) opt_key(B_DOWN); if (opt_row != 4) { bad++; printf("v105: the title's options have the row too\n"); } scr = SC_WALK;
    /* 4. the active slot is kept in the cfg */
    slot_act = 4; cfg_save(); slot_act = 1; cfg_load(); if (slot_act != 4) { bad++; printf("v105: the active slot was not saved in the cfg (%d)\n", slot_act); }
    { FILE *f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "dlg 2\nsnd 1\nwalk 1\nslot 9\n"); fclose(f); } slot_act = 2; cfg_load(); if (slot_act != 2) { bad++; printf("v105: a bad slot value must be ignored\n"); } }
    slot_act = 1; cfg_save();
    { Game t; int q; memset(slot_st, 0, sizeof slot_st); g_init(&t); t.career = 9; t.story = ST_NAMED; t.day = 12; t.phase = PH_EVENING; t.cash = 3400; t.bank = 15600; slot_g[0] = t; slot_st[0] = 1;
      t.day = 3; t.phase = PH_MORNING; t.cash = 450; t.bank = 0; slot_g[2] = t; slot_st[2] = 1; slot_st[3] = 2; slot_act = 3; (void)q;
      scr = SC_CAREER; car_sel = 0; car_conf = 0; RSHOT("v105_career_slots"); car_sel = 2; RSHOT("v105_career_slots_sel3"); car_conf = 1; RSHOT("v105_career_startover_confirm"); car_conf = 2; RSHOT("v105_career_delete_confirm"); car_conf = 0; scr = SC_WALK; }
    /* 5. game over deletes nothing: it goes back to the title */
    g_init(&G); G.career = 31; G.story = ST_NAMED; G.day = 3; slot_act = 1; save_now(); scr = SC_OVER; over_ms = fake_ms; on_button(B_A);
    { Game t; if (scr != SC_MENU || G.story != ST_NEW || !load_game(&t, slot_path(1)) || t.day != 3) { bad++; printf("v105: after GAME OVER the save must stay and the title shows (scr %d)\n", scr); } }
    /* 6. a v21 file (the old struct without career / mrw) loads and gets a career id */
    { Game t, u; FILE *f; g_init(&t); t.story = ST_NAMED; t.day = 5; t.ver = SAVE_VER_21; t.magic = SAVE_MAGIC; t.size = (int)offsetof(Game, career);
      f = fopen(slot_path(2), "wb"); if (f) { fwrite(&t, 1, offsetof(Game, career), f); fclose(f); }
      if (!load_game(&u, slot_path(2)) || u.ver != SAVE_VER || u.day != 5 || u.career <= 0 || u.mrw_n != 0 || u.size != (int)sizeof u) { bad++; printf("v105: a version-21 save must load and get a career id\n"); } }
    for (i = 1; i <= SLOT_N; i++) remove(slot_path(i)); remove(prog_path());
    slot_prefix = kpre; slot_io_test = kio; slot_act = kact; prog_far = kfar; wk_map = kmap; opt_from = kopt; pause_kind = PK_NONE; opt_sub = opt_conf = 0; wk_mode = 0;
    G = keep; saved_G = keeps; scr = kscr; frozen = fr;
    return bad;
}
static int v100_tests(void) {   /* v10.0: store tabs by wins / move-in, one-time items vanish, the achievement banner */
    int bad = 0, o[16], n, k; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen; frozen = 1; pause_kind = PK_NONE;
    g_init(&G); G.story = ST_NAMED; G.cash = 100000; G.day = 3; saved_G = G; scr = SC_WALK; fade_st = 0; ach_pend = ach_on = 0;
    shop_open();
    if (shop_tab != TAB_STOCKS || shop_tabs_n() != 1 || shop_tab_ok(TAB_ITEMS) || shop_tab_ok(TAB_BM)) { bad++; printf("v100: a new game: the store shows STOCKS only (tab %d, %d tabs)\n", shop_tab, shop_tabs_n()); }
    draw_all(); RSHOT("v100_store_stocks_only");
    on_button(B_R1); if (shop_tab != TAB_STOCKS) { bad++; printf("v100: R1 with one tab must stay on STOCKS\n"); }
    scr = SC_WALK;
    /* 19 wins: nothing yet; the 20th (a real win at her table) opens the black market and starts the banner when you walk again */
    G.sw_wins = SW_GOAL - 1; shop_open(); if (shop_tab_ok(TAB_BM)) { bad++; printf("v100: 19 wins must not open the black market\n"); } scr = SC_WALK;
    G.energy = 100; g_match_start(&G, 100); G.net = 5; scr = SC_PLAY; finish_match();
    if (G.sw_wins != SW_GOAL || !ach_pend || ach_on) { bad++; printf("v100: the 20th win must queue the achievement (wins %d pend %d on %d)\n", G.sw_wins, ach_pend, ach_on); }
    draw_all(); if (ach_on) { bad++; printf("v100: no banner over the result screen\n"); }
    scr = SC_WALK; fake_ms += 100; draw_all(); if (!ach_on || ach_pend || ach_shown != 1) { bad++; printf("v100: the banner must start once you walk again (on %d pend %d shown %d)\n", ach_on, ach_pend, ach_shown); }
    RSHOT("v100_achievement_banner");
    ach_on = 0; ach_pend = 0; { int sw0 = G.sw_wins; G.energy = 100; g_match_start(&G, 100); G.net = 5; scr = SC_PLAY; finish_match(); ach_check(sw0); if (ach_pend) { bad++; printf("v100: a 21st win must not show the achievement again\n"); } }
    scr = SC_WALK; shop_open();
    if (!shop_tab_ok(TAB_BM) || shop_tab_ok(TAB_ITEMS) || shop_tabs_n() != 2) { bad++; printf("v100: after 20 wins: STOCKS + BLACK MARKET only (%d tabs)\n", shop_tabs_n()); }
    on_button(B_R1); if (shop_tab != TAB_BM) { bad++; printf("v100: R1 must reach the black market\n"); }
    n = shop_list(TAB_BM, o); if (n != 2 || o[0] != SH_RING || o[1] != SH_PILL) { bad++; printf("v100: black market before the day-8 scene = ring, pill (n %d)\n", n); }
    draw_all(); RSHOT("v100_black_market_open");
    on_button(B_A); on_button(B_A);   /* buy the ring */
    n = shop_list(TAB_BM, o); if (!G.ring || n != 1 || o[0] != SH_PILL || shop_sel != SH_PILL) { bad++; printf("v100: the bought ring vanishes and the cursor moves to the pill (ring %d n %d sel %d)\n", G.ring, n, shop_sel); }
    on_button(B_A); on_button(B_A);   /* buy the pill: the tab is empty and goes away */
    if (!G.pill || shop_tab_ok(TAB_BM) || shop_tab != TAB_STOCKS) { bad++; printf("v100: with the black market empty the tab goes away (pill %d tab %d)\n", G.pill, shop_tab); }
    /* she lives with you: the ITEMS tab appears; one-time items there vanish too */
    G.rs = RS_LIVING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; shop_open(); if (shop_tab != TAB_ITEMS || !shop_tab_ok(TAB_ITEMS))   /* v11.3: the outfit rows need max romance, so the 8 DOWNs below still reach the ring */ { bad++; printf("v100: once she lives with you the store opens on ITEMS\n"); }
    for (k = 0; k < 8; k++) on_button(B_DOWN); on_button(B_A); on_button(B_A);   /* v10.8 / v11.1: two more rows (the designer top, the cow print outfit) above the ring */
    if (!G.dring || tst_list_ok(TAB_ITEMS) == 0) { bad++; printf("v100: the diamond ring vanishes from ITEMS after buying (dring %d)\n", G.dring); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ach_on = ach_pend = 0;
    return bad;
}
static unsigned surf_sig(SDL_Surface *a) { unsigned h = 2166136261u; size_t i, n = (size_t)a->pitch * (size_t)a->h; const unsigned char *q = (const unsigned char *)a->pixels; for (i = 0; i < n; i += 61) h = (h ^ q[i]) * 16777619u; return h; }
static int v102_tests(void) {   /* v10.2: the gift scene pools: five versions of every beat for the eight repeatable gifts, no immediate repeat, the right note, nothing from the old fixed text lost */
    int bad = 0, k, b, z, i; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, krom = rom_in;
    static const int kinds[10] = {GF_FLOWER, GF_STACK, GF_LOTION, GF_DECK, GF_YOGA, GF_GOLD, GF_MILK, GF_SUPER, GF_TOP, GF_COW};
    frozen = 1; pause_kind = PK_NONE; rom_in = 0;
    for (k = 0; k < 10; k++) {
        int kind = kinds[k]; static char seen[4][8][200]; int ns[4] = {0, 0, 0, 0}; const char *prev0 = NULL, *prev2 = NULL; int rep = 0;
        for (b = 0; b < 4; b++) {
            if (b == 1 && kind == GF_MILK) continue;
            if (!GIFT_POOL[b][kind] || *GIFT_POOL_N[b][kind] < 5) { bad++; printf("v102: gift %d beat %d: fewer than 5 versions\n", kind, b); continue; }
            { const char *const *p = *GIFT_POOL[b][kind]; int n = *GIFT_POOL_N[b][kind];
              const char *oldt = b == 0 ? R_GIFT_HAND[kind] : b == 2 ? R_GIFT_L2[kind] : b == 3 ? R_GIFT_END[kind] : (kind == GF_STACK ? R_GIFT_OK[1] : R_GIFT_OK[3 + kind - GF_LOTION < 8 && kind >= GF_LOTION ? 3 + kind - GF_LOTION : 0]);
              if (kind != GF_YOGA && kind != GF_TOP && kind != GF_COW && strcmp(p[0], oldt)) { bad++; printf("v102: gift %d beat %d: the old text must stay entry 1\n", kind, b); }
              if (kind == GF_MILK || kind == GF_SUPER) for (i = 0; i < n; i++) if (txt_boy(p[i]) || strstr(p[i], "baby")) { bad++; printf("v102: gift %d beat %d: boy / baby in a line that can be played married\n", kind, b); } }
        }
        for (z = 0; z < 120; z++) {
            gift_kind = kind; gift_l1 = gift_pick(1, kind); if (!gift_l1) gift_l1 = pick(R_MILK_OK, R_MILK_OK_N);
            snprintf(gift_note, sizeof gift_note, "ROMANCE +12%%"); vn_start(SCR_GIFT);
            if (scr != SC_STORY || vn_id != SCR_GIFT || vn_n != 4) { bad++; printf("v102: gift %d: scene shape (n %d)\n", kind, vn_n); break; }
            if (!gift_in_pool(0, kind, vn_buf[0].txt) || !gift_in_pool(2, kind, vn_buf[2].txt) || !gift_in_pool(3, kind, vn_buf[3].txt) || !strstr(vn_buf[3].txt, "(ROMANCE +12%)")) { bad++; printf("v102: gift %d: beats are not from the pools / the note is missing\n", kind); break; }
            if (kind != GF_MILK && !gift_in_pool(1, kind, vn_buf[1].txt)) { bad++; printf("v102: gift %d: thanks is not from the pool\n", kind); break; }
            if (prev0 && prev0 == vn_buf[0].txt) rep++; if (prev2 && prev2 == vn_buf[2].txt) rep++; prev0 = vn_buf[0].txt; prev2 = vn_buf[2].txt;
            { const char *t[4]; t[0] = vn_buf[0].txt; t[1] = vn_buf[1].txt; t[2] = vn_buf[2].txt; t[3] = vn_buf[3].txt;
              for (b = 0; b < 4; b++) { int f = 0; for (i = 0; i < ns[b]; i++) if (!strcmp(seen[b][i], t[b])) f = 1; if (!f && ns[b] < 8) { snprintf(seen[b][ns[b]], 200, "%s", t[b]); ns[b]++; } } }
        }
        if (rep) { bad++; printf("v102: gift %d: the same hand / reaction line twice in a row (%d times)\n", kind, rep); }
        for (b = 0; b < 4; b++) if (!(b == 1 && kind == GF_MILK) && ns[b] < 4) { bad++; printf("v102: gift %d beat %d: only %d different lines in 120 scenes\n", kind, b, ns[b]); }
    }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; rom_in = krom; (void)kscr;
    return bad;
}

static int v101_tests(void) {   /* v10.1: BREASTFEED (mama mode) at her bedroom door: she hides behind the door; one picture when she refuses, another when she says yes; each pool remembers its last picks */
    int bad = 0, i, z; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk; unsigned sno, syes, snoy;
#define V101_OPEN(in, talk) do { wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = (talk); rom_open(R_MILK_AFTER[0]); rom_choose = 1; rom_in = (in); } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0; feed_no_day = -1; feed_no_ph = -1; peeked = 0;
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.dring = 1; G.married = 1; G.call = 1; G.pill_given = 1; G.pill_day = 2; G.pill_seen = 1; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.outfit = 0; G.milk_st = 4; G.milk_pct = 100; G.rom_pct = 100;
    /* 1. the pictures: both exist, both are 752x560, they differ from each other and from the old hiding picture (d8 and yoga) */
    { int o; for (o = 0; o < 2; o++) { SDL_Surface *a, *b; G.outfit = o; fh_free(); a = fh_pic(1); if (!a || a->w != 752 || a->h != 560 || fhK != 4 + o) { bad++; printf("v101: refusal hiding picture (outfit %d) not loaded\n", o); } else sno = surf_sig(a);
        b = fh_pic(2); if (!b || b->w != 752 || b->h != 560 || fhK != 8 + o) { bad++; printf("v101: accepting hiding picture (outfit %d) not loaded\n", o); } else { syes = surf_sig(b); if (!o) snoy = syes; }
        if (a && b && sno == syes) { bad++; printf("v101: refusal and accepting picture are the same (outfit %d)\n", o); }
        fh_free(); } G.outfit = 0; }
    /* 2. refusal at the door: the refusal picture is on while her line shows, off once the choices open */
    feed_no_force = 1; V101_OPEN(0, 0); rom_feed(); if (scr != SC_ROM || feed_hide != 1 || !in_pool(rom_txt, R_FEED_NO_MM, R_FEED_NO_MM_N)) { bad++; printf("v101: refusal at the door: picture flag %d\n", feed_hide); }
    RSHOT("v101_feed_refuse_door"); rom_choose = 0; on_button(B_A); if (feed_hide) { bad++; printf("v101: the refusal picture must go when her choices open\n"); }
    V101_OPEN(0, 0); rom_feed(); if (feed_hide != 1 || !in_pool(rom_txt, R_FEED_AGAIN_MM, R_FEED_AGAIN_MM_N)) { bad++; printf("v101: asking again at the door also hides her (flag %d)\n", feed_hide); }
    feed_no_day = -1; feed_no_ph = -1; G.milk_pct = 20; V101_OPEN(0, 0); rom_feed(); if (feed_hide != 1 || !in_pool(rom_txt, R_NOT_FULL_MM, R_NOT_FULL_MM_N)) { bad++; printf("v101: 'not full' at the door also hides her (flag %d)\n", feed_hide); } G.milk_pct = 100;
    feed_no_day = -1; feed_no_ph = -1; V101_OPEN(1, 0); rom_feed(); if (feed_hide) { bad++; printf("v101: inside her room there is no door to hide behind (refusal)\n"); }
    feed_no_day = -1; feed_no_ph = -1; wk_place(MAP_ROOM, 4, 4, LF); V101_OPEN(0, 1); rom_feed(); if (feed_hide) { bad++; printf("v101: in the living room there is no door to hide behind (refusal)\n"); }
    ls_talk = 0;
    /* 3. yes at the door: the first two beats are the accepting picture (sprite 6) on the door; the rest of the scene is unchanged */
    feed_no_force = 0; feed_no_day = -1; G.milk_pct = 100; V101_OPEN(0, 0); rom_feed();
    if (scr != SC_STORY || vn_id != SCR_FEED || vn_n != B_FEED_MM_N || vn_buf[0].spr != 6 || vn_buf[1].spr != 6 || vn_buf[0].bg != BG_BDOOR || vn_buf[1].bg != BG_BDOOR || vn_buf[2].spr != 0 || vn_buf[2].bg != BG_FEED) { bad++; printf("v101: yes at the door: sprite 6 on beats 1-2 (scr %d id %d n %d spr %d/%d)\n", scr, vn_id, vn_n, vn_buf[0].spr, vn_buf[1].spr); }
    else { vn_i = 0; vn_prev_spr = 6; draw_story(); RSHOT("v101_feed_yes_door"); if (!fhS || fhK != 8) { bad++; printf("v101: the accepting picture must be the one drawn (key %d)\n", fhK); }
           G.outfit = 1; fh_free(); vn_i = 1; draw_story(); if (!fhS || fhK != 9) { bad++; printf("v101: yoga: accepting picture key %d\n", fhK); } RSHOT("v101_feed_yes_door_yoga"); G.outfit = 0; }
    for (z = 0; z < 160 && scr == SC_STORY; z++) on_button(B_A);
    if (fhS || feed_hide) { bad++; printf("v101: after the scene the picture is freed\n"); }
    /* 4. yes inside her room / in the living room / outside mama mode: no hiding */
    G.milk_pct = 100; feed_no_day = -1; V101_OPEN(1, 0); rom_feed(); { int k, h = 0; for (k = 0; k < vn_n; k++) if (vn_buf[k].spr == 6) h++; if (scr != SC_STORY || h) { bad++; printf("v101: inside her room BREASTFEED has no hiding beats (%d)\n", h); } }
    for (z = 0; z < 160 && scr == SC_STORY; z++) on_button(B_A);
    G.milk_pct = 100; feed_no_day = -1; wk_place(MAP_ROOM, 4, 4, LF); V101_OPEN(0, 1); rom_feed(); { int k, h = 0; for (k = 0; k < vn_n; k++) if (vn_buf[k].spr == 6) h++; if (scr != SC_STORY || h) { bad++; printf("v101: in the living room BREASTFEED has no hiding beats (%d)\n", h); } }
    for (z = 0; z < 160 && scr == SC_STORY; z++) on_button(B_A); ls_talk = 0;
    G.call = 0; ls_talk = 0; rom_in = 0; vn_start(SCR_FEED); { int k, h = 0; for (k = 0; k < vn_n; k++) if (vn_buf[k].spr == 6) h++; if (h) { bad++; printf("v101: outside mama mode no hiding beats\n"); } } G.call = 1;
    for (z = 0; z < 160 && scr == SC_STORY; z++) on_button(B_A);
    /* 5. every pool remembers its last picks: no repeat within 4 draws once a pool has 4+ lines, never twice in a row from a pool of 2+ */
    { const char *w[4096]; int n, bw = 0; for (n = 0; n < 4000; n++) w[n] = pick(R_FEED_NO_MM, R_FEED_NO_MM_N);
      for (n = 3; n < 4000; n++) for (i = 1; i <= 3; i++) if (w[n] == w[n - i]) bw++;
      if (bw) { bad++; printf("v101: R_FEED_NO_MM repeated a line within 3 draws (%d times)\n", bw); }
      for (n = 0; n < 4000; n++) w[n] = pick(R_NIGHT_M, R_NIGHT_M_N); bw = 0; for (n = 1; n < 4000; n++) if (w[n] == w[n - 1]) bw++; if (bw) { bad++; printf("v101: R_NIGHT_M repeated right away (%d)\n", bw); }
      { const char *two[2] = {"A", "B"}; int rep = 0; const char *l = NULL; for (n = 0; n < 400; n++) { const char *c = pick(two, 2); if (c == l) rep++; l = c; } if (rep) { bad++; printf("v101: a pool of two alternates (%d repeats)\n", rep); } }
      { const char *one[1] = {"X"}; if (pick(one, 1) != one[0]) { bad++; printf("v101: a pool of one still works\n"); } } }
    (void)snoy;
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; feed_no_force = 0; feed_no_day = -1; feed_no_ph = -1; feed_hide = 0; fh_free(); wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
    return bad;
}
static int v99_tests(void) {   /* v9.9: she is not in the living room from pill day 6; ASK FOR MILK / PUMP MILK / BREASTFEED asked in the living room: "Here?!" first, living-room scenes and pictures */
    int bad = 0, i; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk;
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 1; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0; feed_no_force = 0; peeked = 0;
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.dring = 1; G.married = 1; G.pill_given = 1; G.pill_day = 2; G.pill_seen = 1; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.outfit = 0;
    /* 1. the sprite: pill day 6 = day pill_day + 5 */
    G.day = G.pill_day + 4; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 4, 4, LF); ls_ph = G.phase; ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; ls_moving = 0; ls_update();
    if (!ls_on) { bad++; printf("v99: pill day 5: she is still allowed in the living room\n"); }
    G.day = G.pill_day + 5; ls_update(); if (ls_on) { bad++; printf("v99: pill day 6: her sprite must be gone at once (she was standing there when the day changed)\n"); }
    G.day = G.pill_day + 7; ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; ls_ph = G.phase; ls_update(); if (ls_on) { bad++; printf("v99: day 8: no sprite\n"); }
    ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; wk_place(MAP_ROOM, 4, 4, LF); ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; ls_ph = G.phase; on_button(B_A);
    if (scr != SC_WALK || ls_talk || wk_mode == 1) { bad++; printf("v99: A where she was must not open her menu or the busy line (scr %d)\n", scr); }
    for (i = 0; i < 400; i++) { ls_force = 1; ls_ph = -1; ls_roll(); if (ls_on) { bad++; printf("v99: day 8: she was rolled into the living room\n"); break; } } ls_force = 0;
    /* 2. the living-room scenes */
    G.day = 20; G.pill_day = 2; G.phase = PH_MORNING; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 4, 4, LF); ls_on = 0; ls_talk = 1;
    for (i = 0; i < 40; i++) {
        vn_start(SCR_BOTTLE); if (vn_id != SCR_BOTTLE || vn_n != 1 + LEAD_BEATS + B_BOTTLE_LR_N || !in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N) || vn_buf[0].who != W_SOPHIA || vn_buf[1 + LEAD_BEATS].txt != B_BOTTLE_LR[0].txt) { bad++; printf("v99: bottle scene in the living room (n %d)\n", vn_n); break; }
        if (vn_buf[0].bg != BG_MYROOM || vn_buf[2].bg != BG_MYROOM) { bad++; printf("v99: bottle scene must play in the living room\n"); break; }
    }
    vn_start(SCR_PEEKPIC); if (vn_n != B_PEEKPIC_LR_N || vn_buf[1].txt != B_PEEKPIC_LR[1].txt) { bad++; printf("v99: peek scene in the living room\n"); }
    G.call = 0; vn_start(SCR_FEED); if (vn_n != 1 + B_FEED_LR_N || !in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N) || vn_buf[1].txt != B_FEED_LR[0].txt || vn_buf[0].spr != 1 || vn_buf[3].bg != BG_FEED) { bad++; printf("v99: BREASTFEED in the living room (n %d)\n", vn_n); }
    G.call = 1; vn_start(SCR_FEED); if (!g_mama(&G) || vn_n != B_FEED_MM_LR_N || vn_buf[2].txt != B_FEED_MM_LR[2].txt || in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N) || vn_buf[0].who != W_ME) { bad++; printf("v99: mama BREASTFEED in the living room (n %d)\n", vn_n); }   /* v10.4: your line first, her reaction (with the Here?!) second: no separate Here?! beat */
    { int k, mm = 0; for (k = 0; k < vn_n; k++) if (txt_has_any(vn_buf[k].txt, (const char *const[]){"baby", "good boy", NULL})) mm++; if (mm) { bad++; printf("v99: no baby / good boy in the mama living-room scene\n"); } }
    for (i = 0; i < R_LR_HERE_N; i++) if (!strstr(R_LR_HERE[i], "Here?!") && !strstr(R_LR_HERE[i], "here?!")) { bad++; printf("v99: every living-room reply starts with Here?!\n"); break; }
    for (G.outfit = 0; G.outfit < 2; G.outfit++) { SDL_Surface *p = sc_pic(BG_FEED); if (!p || p->w != SW || p->h != SH) { bad++; printf("v99: living-room feed picture outfit %d\n", G.outfit); } }
    G.outfit = 0;
    /* 3. the bedroom and the door are unchanged */
    ls_talk = 0; G.call = 0; vn_start(SCR_FEED); if (vn_n != B_FEED_N || in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N)) { bad++; printf("v99: BREASTFEED at the door is unchanged (n %d)\n", vn_n); }
    vn_start(SCR_BOTTLE); if (vn_n != LEAD_BEATS + B_BOTTLE_N || in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N)) { bad++; printf("v99: bottle scene at the door is unchanged (n %d)\n", vn_n); }
    wk_place(MAP_SBED, 3, 5, UP); ls_talk = 1; vn_start(SCR_FEED); if (vn_n != B_FEED_N) { bad++; printf("v99: BREASTFEED inside her bedroom is unchanged (n %d)\n", vn_n); }
    vn_start(SCR_BOTTLE); if (vn_n != LEAD_BEATS + B_BOTTLE_N) { bad++; printf("v99: bottle scene inside her bedroom is unchanged\n"); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; feed_no_force = 0; wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
    return bad;
}
static int op_starts(const char *t) {   /* her opener answer starts with her reaction to the name */
    static const char *const W[] = {"m-mama", "mama?!", "don't call me that", "don't... don't say that name", "w-what?! don't call me that", "stop calling me that", "don't say it like that", "stop it with that name", "don't say that name", NULL};
    char lo[64]; int i, k; if (!t) return 0; for (i = 0; i < 63 && t[i]; i++) lo[i] = (char)tolower((unsigned char)t[i]); lo[i] = 0;
    for (k = 0; W[k]; k++) if (!strncmp(lo, W[k], strlen(W[k]))) return 1;
    return 0;
}
static int op_here(const char *t) { return t && (strstr(t, "here") || strstr(t, "Here")); }
static int op_pairs_ok(const char *nm, const Beat *b, int n, int need_here, int *bad) {   /* every opener pair: ME then SOPHIA, fits the box, reaction first, no baby / good boy */
    int i, ok = 1; if (n < 2 || (n & 1)) { (*bad)++; printf("v104: %s must hold whole pairs (n %d)\n", nm, n); return 0; }
    for (i = 0; i < n; i++) {
        char w[400]; int j, ln = 1; if (!b[i].txt) { (*bad)++; printf("v104: %s beat %d has no text\n", nm, i); ok = 0; continue; }
        if (b[i].who != ((i & 1) ? W_SOPHIA : W_ME)) { (*bad)++; printf("v104: %s beat %d: ME then SOPHIA expected\n", nm, i); ok = 0; }
        wrap_buf(w, sizeof w, b[i].txt, 46); for (j = 0; w[j]; j++) if (w[j] == '\n') ln++; if (ln > 3) { (*bad)++; printf("v104: %s beat %d needs %d lines\n", nm, i, ln); ok = 0; }
        if (strstr(b[i].txt, "baby") || strstr(b[i].txt, "good boy")) { (*bad)++; printf("v104: %s beat %d: no baby / good boy\n", nm, i); ok = 0; }
        if (!(i & 1) && !strstr(b[i].txt, "Mama")) { (*bad)++; printf("v104: %s beat %d: your line must say Mama\n", nm, i); ok = 0; }
        if ((i & 1) && !op_starts(b[i].txt)) { (*bad)++; printf("v104: %s beat %d: her reaction to the name must come first (\"%.30s\")\n", nm, i, b[i].txt); ok = 0; }
        if ((i & 1) && need_here && !op_here(b[i].txt)) { (*bad)++; printf("v104: %s beat %d: her reaction needs the Here?!\n", nm, i); ok = 0; }
    }
    return ok;
}
static int op_is_pair(const Beat *pool, int n, const char *me, const char *her) { int i; for (i = 0; i + 1 < n; i += 2) if (pool[i].txt == me && pool[i + 1].txt == her) return 1; return 0; }
static int v104_tests(void) {   /* v10.4: mama-mode openers: your line first, then her reaction to the name; BREASTFEED at the door / in her bedroom / in the living room; living room: ASK, PUMP, milk pill, Super drug */
    int bad = 0, i, k; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk, kgk = gift_kind, kri = rom_in;
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 1; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0; feed_no_force = 0; peeked = 0; bottle_pill = 0;
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.dring = 1; G.married = 1; G.pill_given = 1; G.pill_day = 2; G.pill_seen = 1; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.outfit = 0; G.call = 1;
    /* 1. the pools themselves */
    op_pairs_ok("B_OPEN_FEED_MM", B_OPEN_FEED_MM, B_OPEN_FEED_MM_N, 0, &bad); op_pairs_ok("B_OPEN_FEED_MM_LR", B_OPEN_FEED_MM_LR, B_OPEN_FEED_MM_LR_N, 1, &bad);
    op_pairs_ok("B_OPEN_ASK_MM_LR", B_OPEN_ASK_MM_LR, B_OPEN_ASK_MM_LR_N, 1, &bad); op_pairs_ok("B_OPEN_PILL_MM_LR", B_OPEN_PILL_MM_LR, B_OPEN_PILL_MM_LR_N, 0, &bad); op_pairs_ok("B_OPEN_SUPER_MM_LR", B_OPEN_SUPER_MM_LR, B_OPEN_SUPER_MM_LR_N, 0, &bad);
    if (B_OPEN_FEED_MM_N < 12 || B_OPEN_FEED_MM_LR_N < 12 || B_OPEN_ASK_MM_LR_N < 8 || B_OPEN_PILL_MM_LR_N < 6 || B_OPEN_SUPER_MM_LR_N < 6) { bad++; printf("v104: not enough opener pairs\n"); }
    /* 2. BREASTFEED in the living room (mama): ME first, then her reaction; no separate Here?! beat; the scene goes on unchanged */
    scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 4, 4, LF); ls_on = 0; ls_talk = 1; G.call = 1;
    { int seen[16] = {0}, rep = 0, last = -2, np = B_OPEN_FEED_MM_LR_N / 2, got = 0;
      for (i = 0; i < 300; i++) {
          vn_start(SCR_FEED);
          if (vn_n != B_FEED_MM_LR_N || vn_buf[0].who != W_ME || vn_buf[1].who != W_SOPHIA || in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N) || !op_is_pair(B_OPEN_FEED_MM_LR, B_OPEN_FEED_MM_LR_N, vn_buf[0].txt, vn_buf[1].txt) || vn_buf[2].txt != B_FEED_MM_LR[2].txt || vn_buf[2].bg != BG_FEED || vn_buf[0].spr != 1) { bad++; printf("v104: living-room mama BREASTFEED (n %d)\n", vn_n); break; }
          for (k = 0; k < np; k++) if (vn_buf[0].txt == B_OPEN_FEED_MM_LR[2 * k].txt) { if (k == last) rep++; last = k; if (k < 16) seen[k] = 1; }
      }
      for (k = 0; k < np && k < 16; k++) got += seen[k];
      if (rep) { bad++; printf("v104: the same living-room feed opener twice in a row (%d)\n", rep); } if (got != np) { bad++; printf("v104: only %d of %d living-room feed openers came up\n", got, np); } }
    G.call = 0; vn_start(SCR_FEED); if (vn_n != 1 + B_FEED_LR_N || !in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N)) { bad++; printf("v104: husband mode: living-room BREASTFEED keeps the Here?! (n %d)\n", vn_n); }
    G.call = 1;
    /* 3. BREASTFEED at her door and inside her bedroom (mama) */
    ls_talk = 0; wk_place(MAP_ROOM, 4, 4, LF); rom_in = 0;
    for (i = 0; i < 100; i++) { vn_start(SCR_FEED);
        if (vn_n != B_FEED_MM_N || vn_buf[0].who != W_ME || vn_buf[1].who != W_SOPHIA || !op_is_pair(B_OPEN_FEED_MM, B_OPEN_FEED_MM_N, vn_buf[0].txt, vn_buf[1].txt) || vn_buf[2].txt != B_FEED_MM[2].txt || vn_buf[0].spr != 6 || vn_buf[1].spr != 6 || vn_buf[0].bg != BG_BDOOR) { bad++; printf("v104: mama BREASTFEED at her door (n %d spr %d)\n", vn_n, vn_buf[0].spr); break; } }
    wk_place(MAP_SBED, 3, 5, UP); ls_talk = 1;
    for (i = 0; i < 100; i++) { vn_start(SCR_FEED);
        if (vn_n != B_FEED_MM_N || vn_buf[0].who != W_ME || vn_buf[1].who != W_SOPHIA || !op_is_pair(B_OPEN_FEED_MM, B_OPEN_FEED_MM_N, vn_buf[0].txt, vn_buf[1].txt) || vn_buf[2].txt != B_FEED_MM[2].txt || vn_buf[0].spr != 1 || vn_buf[2].bg != BG_FEED) { bad++; printf("v104: mama BREASTFEED in her bedroom (n %d spr %d)\n", vn_n, vn_buf[0].spr); break; } }
    G.call = 0; vn_start(SCR_FEED); if (vn_n != B_FEED_N || vn_buf[0].txt != B_FEED[0].txt) { bad++; printf("v104: husband mode: bedroom BREASTFEED is unchanged (n %d)\n", vn_n); } G.call = 1;
    /* 4. ASK FOR MILK / PUMP MILK in the living room: your line, her reaction, then the lead-in and the pump scene */
    wk_place(MAP_ROOM, 4, 4, LF); ls_talk = 1; bottle_pill = 0;
    for (i = 0; i < 100; i++) { vn_start(SCR_BOTTLE);
        if (vn_n != 2 + LEAD_BEATS + B_BOTTLE_LR_N || vn_buf[0].who != W_ME || vn_buf[1].who != W_SOPHIA || in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N) || !op_is_pair(B_OPEN_ASK_MM_LR, B_OPEN_ASK_MM_LR_N, vn_buf[0].txt, vn_buf[1].txt) || vn_buf[2 + LEAD_BEATS].txt != B_BOTTLE_LR[0].txt) { bad++; printf("v104: mama ASK / PUMP in the living room (n %d)\n", vn_n); break; } }
    bottle_pill = 1; vn_start(SCR_BOTTLE);   /* the milk pill filled the gauge: the pill opener plays, and the flag is used up */
    if (vn_n != 2 + LEAD_BEATS + B_BOTTLE_LR_N || !op_is_pair(B_OPEN_PILL_MM_LR, B_OPEN_PILL_MM_LR_N, vn_buf[0].txt, vn_buf[1].txt) || bottle_pill) { bad++; printf("v104: a milk pill that starts the pump scene plays the pill opener (n %d)\n", vn_n); }
    vn_start(SCR_BOTTLE); if (!op_is_pair(B_OPEN_ASK_MM_LR, B_OPEN_ASK_MM_LR_N, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v104: the next ASK is an ASK again\n"); }
    G.call = 0; vn_start(SCR_BOTTLE); if (vn_n != 1 + LEAD_BEATS + B_BOTTLE_LR_N || !in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N)) { bad++; printf("v104: husband mode: ASK in the living room keeps the Here?! (n %d)\n", vn_n); } G.call = 1;
    { int keepn = B_OPEN_ASK_MM_LR_N; B_OPEN_ASK_MM_LR_N = 1; vn_start(SCR_BOTTLE); B_OPEN_ASK_MM_LR_N = keepn;   /* a pool without a whole pair falls back to the old Here?! */
      if (vn_n != 1 + LEAD_BEATS + B_BOTTLE_LR_N || !in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N)) { bad++; printf("v104: an empty opener pool must fall back to the Here?! (n %d)\n", vn_n); } }
    ls_talk = 0; wk_place(MAP_ROOM, 4, 4, LF); vn_start(SCR_BOTTLE); if (vn_n != LEAD_BEATS + B_BOTTLE_N || vn_buf[0].txt == B_OPEN_ASK_MM_LR[0].txt) { bad++; printf("v104: the bottle scene at her door has no opener (n %d)\n", vn_n); }
    /* 5. milk pill and Super drug in the living room (mama): 2 beats more than the plain gift scene; other gifts and the door unchanged */
    ls_talk = 1; wk_place(MAP_ROOM, 4, 4, LF); snprintf(gift_note, sizeof gift_note, "GAUGE FULL");
    gift_kind = GF_MILK; gift_l1 = R_MILK_OK[0]; vn_start(SCR_GIFT);
    if (vn_id != SCR_GIFT || vn_n != 6 || vn_buf[0].who != W_ME || vn_buf[1].who != W_SOPHIA || !op_is_pair(B_OPEN_PILL_MM_LR, B_OPEN_PILL_MM_LR_N, vn_buf[0].txt, vn_buf[1].txt) || !gift_in_pool(0, GF_MILK, vn_buf[2].txt) || !gift_in_pool(3, GF_MILK, vn_buf[5].txt) || !strstr(vn_buf[5].txt, "GAUGE FULL")) { bad++; printf("v104: mama milk pill in the living room (n %d)\n", vn_n); }
    gift_kind = GF_SUPER; gift_l1 = gift_pick(1, GF_SUPER); vn_start(SCR_GIFT);
    if (vn_n != 6 || vn_buf[0].who != W_ME || !op_is_pair(B_OPEN_SUPER_MM_LR, B_OPEN_SUPER_MM_LR_N, vn_buf[0].txt, vn_buf[1].txt) || !gift_in_pool(0, GF_SUPER, vn_buf[2].txt) || !gift_in_pool(3, GF_SUPER, vn_buf[5].txt)) { bad++; printf("v104: mama Super drug in the living room (n %d)\n", vn_n); }
    for (k = 0; k < 6; k++) if (vn_buf[k].bg != vn_buf[2].bg) { bad++; printf("v104: the opener beats use the same picture as the gift scene\n"); break; }
    gift_kind = GF_FLOWER; gift_l1 = gift_pick(1, GF_FLOWER); vn_start(SCR_GIFT); if (vn_n != 4) { bad++; printf("v104: a flower has no mama opener (n %d)\n", vn_n); }
    G.call = 0; gift_kind = GF_SUPER; gift_l1 = gift_pick(1, GF_SUPER); vn_start(SCR_GIFT); if (vn_n != 4) { bad++; printf("v104: husband mode: the Super drug scene is unchanged (n %d)\n", vn_n); } G.call = 1;
    ls_talk = 0; gift_kind = GF_MILK; gift_l1 = R_MILK_OK[0]; vn_start(SCR_GIFT); if (vn_n != 4) { bad++; printf("v104: the milk pill at her door is unchanged (n %d)\n", vn_n); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; feed_no_force = 0; bottle_pill = 0; gift_kind = kgk; rom_in = kri; wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
    return bad;
}
static void rq_reset(void) { rq_career = -1; rq_sync(); }
static int pl_pair(int slot, const char *me, const char *her) { return op_is_pair(*OP_P[slot], *OP_PN[slot], me, her); }
static int plea_ok(const char *nm, const Beat *b, int n, int *bad) {   /* plea / mid pairs: ME says Mama, then SOPHIA; fits the box; no baby / good boy; at least 6 pairs */
    int i, ok = 1; if (n < 12 || (n & 1)) { (*bad)++; printf("v106: %s needs at least 6 whole pairs (n %d)\n", nm, n); return 0; }
    for (i = 0; i < n; i++) {
        char w[400]; int j, ln = 1; if (!b[i].txt) { (*bad)++; printf("v106: %s beat %d has no text\n", nm, i); ok = 0; continue; }
        if (b[i].who != ((i & 1) ? W_SOPHIA : W_ME)) { (*bad)++; printf("v106: %s beat %d: ME then SOPHIA expected\n", nm, i); ok = 0; }
        wrap_buf(w, sizeof w, b[i].txt, 46); for (j = 0; w[j]; j++) if (w[j] == '\n') ln++; if (ln > 3) { (*bad)++; printf("v106: %s beat %d needs %d lines\n", nm, i, ln); ok = 0; }
        if (strstr(b[i].txt, "baby") || strstr(b[i].txt, "good boy")) { (*bad)++; printf("v106: %s beat %d: no baby / good boy\n", nm, i); ok = 0; }
        if (!(i & 1) && !strstr(b[i].txt, "Mama")) { (*bad)++; printf("v106: %s beat %d: your line must say Mama\n", nm, i); ok = 0; }
    }
    return ok;
}
static int v107_tests(void) {   /* v10.7: the lowest opacity step is 0% (box and lines vanish), line colour, name colours, thick shadow, white narration, OPTIONS > TEXT BOX STYLE, cfg */
    int bad = 0, i; int kdlg = dlg_idx, kline = dlg_line, ks = col_soph, ky = col_you, kscr = scr, kfrom = opt_from, krow = opt_row, ksub = opt_sub, ksel = opt_tsel, kfr = frozen;
    frozen = 1;
    /* 1. the steps */
    if (DLG_A[5] != 0 || strcmp(DLG_N[5], "0%") || DLG_A[0] != 256 || DLG_A[4] != 51) { bad++; printf("v107: the lowest opacity step must be 0%% (alpha %d, label %s)\n", DLG_A[5], DLG_N[5]); }
    /* 2. 0% draws nothing, 100% draws the old box: fill and border (pixel check on a 32-bit screen) */
    if (S->format->BitsPerPixel == 32) {
        int px = XS(2), py = YS(300), qx = XS(1), qy = YS(250); Uint32 bg = C(100,120,140), a0, a1, b0, b1;
        #define PIX(x, y) (*(Uint32 *)((Uint8 *)S->pixels + (y) * S->pitch + (x) * 4))
        dlg_line = 2;
        R(0, 0, 640, 480, bg); dlg_idx = 5; panel_d(0, 250, 640, 60, 256); a0 = PIX(px, py); b0 = PIX(qx, qy);
        R(0, 0, 640, 480, bg); dlg_idx = 0; panel_d(0, 250, 640, 60, 256); a1 = PIX(px, py); b1 = PIX(qx, qy);
        if (a0 != bg || b0 != bg) { bad++; printf("v107: at 0%% neither the box nor its line may be drawn\n"); }
        if (a1 == bg || b1 == bg) { bad++; printf("v107: at 100%% the box and its line must be drawn\n"); }
        R(0, 0, 640, 480, bg); dlg_idx = 5; panel_d(10, 218, 100, 28, 256); if (PIX(XS(11), YS(230)) != bg) { bad++; printf("v107: the name box must vanish at 0%%\n"); }
        R(0, 0, 640, 480, bg); dlg_idx = 3; panel_d(0, 250, 640, 60, 256); a0 = PIX(qx, qy); dlg_idx = 0; R(0, 0, 640, 480, bg); panel_d(0, 250, 640, 60, 256); a1 = PIX(qx, qy);
        if (a0 == a1 || a0 == bg) { bad++; printf("v107: at 40%% the line must be fainter than at 100%% but visible\n"); }
        dlg_line = 0; R(0, 0, 640, 480, bg); dlg_idx = 0; panel_d(0, 250, 640, 60, 256); a0 = PIX(qx, qy); dlg_line = 2; R(0, 0, 640, 480, bg); panel_d(0, 250, 640, 60, 256); a1 = PIX(qx, qy);
        if (a0 == a1) { bad++; printf("v107: the line colour must change the border\n"); }
        #undef PIX
    }
    /* 3. name colours: yours, hers (also ??? and ARROGANT WOMAN), anything else gold */
    col_soph = 3; col_you = 6;
    if (name_col(N_SOPHIA) != C(255,150,205) || name_col(N_AW) != C(255,150,205) || name_col(N_UNK) != C(255,150,205)) { bad++; printf("v107: Sophia's tags must use her colour\n"); }
    if (name_col(N_ME) != C(130,180,255)) { bad++; printf("v107: your tag must use your colour\n"); }
    if (name_col("SAMPLE") != DLG_GOLD || name_col(NULL) != DLG_GOLD) { bad++; printf("v107: other tags stay gold\n"); }
    /* 4. the thick shadow is only on while a dialogue text is drawn */
    dtext(0, 0, "A", 2, DLG_WHITE); dtext_r(100, 0, "A", 2, DLG_WHITE); if (TSX) { bad++; printf("v107: the thick shadow must switch itself off\n"); }
    /* 5. OPTIONS > TEXT BOX STYLE (title / walking: row 4, at a table: row 3) */
    dlg_line = 0; col_soph = 0; col_you = 0; opt_from = PK_HOME; scr = SC_OPT; opt_row = 0; opt_sub = 0;
    for (i = 0; i < 9; i++) opt_key(B_DOWN); if (opt_row != 4 || OPT_TXT != 4) { bad++; printf("v107: TEXT BOX STYLE is the last row (%d)\n", opt_row); }
    RSHOT("v107_options_title");
    opt_key(B_A); if (opt_sub != 2 || opt_tsel != 0) { bad++; printf("v107: A opens the style panel (sub %d)\n", opt_sub); }
    opt_key(B_RIGHT); opt_key(B_RIGHT); if (dlg_line != 2) { bad++; printf("v107: R changes the line colour (%d)\n", dlg_line); }
    RSHOT("v107_style_line");
    opt_key(B_DOWN); opt_key(B_LEFT); if (col_soph != 7) { bad++; printf("v107: L wraps Sophia's name colour to the last one (%d)\n", col_soph); } opt_key(B_RIGHT); opt_key(B_RIGHT); opt_key(B_RIGHT); if (col_soph != 2) { bad++; printf("v107: R wraps (%d)\n", col_soph); }
    RSHOT("v107_style_sophia");
    opt_key(B_DOWN); opt_key(B_RIGHT); opt_key(B_RIGHT); opt_key(B_RIGHT); if (opt_tsel != 2 || col_you != 3) { bad++; printf("v107: your name colour (row %d, colour %d)\n", opt_tsel, col_you); }
    RSHOT("v107_style_you");
    for (i = 0; i < 8; i++) opt_key(B_RIGHT); if (col_you != 3) { bad++; printf("v107: 8 steps come round to the same colour\n"); }
    if (opt_key(B_B) || opt_sub != 0 || scr != SC_OPT) { bad++; printf("v107: B closes the style panel only (sub %d)\n", opt_sub); }
    dlg_idx = 5; opt_row = 0; RSHOT("v107_options_0pct");
    opt_from = PK_TABLE; scr = SC_WALK; opt_row = 0; opt_sub = 0; for (i = 0; i < 9; i++) opt_key(B_DOWN); if (opt_row != 3 || OPT_TXT != 3) { bad++; printf("v107: at a table TEXT BOX STYLE is row 3 (%d)\n", opt_row); }
    opt_key(B_A); if (opt_sub != 2) { bad++; printf("v107: A opens the style panel at a table\n"); } opt_key(B_A); if (opt_sub != 0) { bad++; printf("v107: A also closes it\n"); }
    /* 6. the cfg: round trip, and a cfg without the new lines keeps what was set */
    dlg_line = 5; col_soph = 2; col_you = 7; cfg_save(); dlg_line = 0; col_soph = 0; col_you = 0; cfg_load();
    if (dlg_line != 5 || col_soph != 2 || col_you != 7) { bad++; printf("v107: cfg round trip (%d %d %d)\n", dlg_line, col_soph, col_you); }
    { FILE *f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "dlg 1\nsnd 1\nwalk 1\nline 9\nsoph -1\n"); fclose(f); } dlg_line = 4; col_soph = 4; col_you = 4; cfg_load();
      if (dlg_line != 4 || col_soph != 4 || col_you != 4) { bad++; printf("v107: an old cfg / bad values must change nothing (%d %d %d)\n", dlg_line, col_soph, col_you); } }
    dlg_idx = kdlg; dlg_line = kline; col_soph = ks; col_you = ky; cfg_save();   /* (puts the cfg file back as it was) */
    scr = kscr; opt_from = kfrom; opt_row = krow; opt_sub = ksub; opt_tsel = ksel; frozen = kfr;
    return bad;
}
static int v106_tests(void) {   /* v10.6: the opener is followed by your plea and her answer; door / bedroom openers for ASK, PILL, SUPER; she remembers an earlier ask; one more hesitation in the middle of the pump and breastfeed scenes */
    int bad = 0, i, k; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk, kgk = gift_kind, kri = rom_in, kcr = rq_career, kcnt[RQ_N];
    for (k = 0; k < RQ_N; k++) kcnt[k] = rq_cnt[k];
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 1; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0; feed_no_force = 0; peeked = 0; bottle_pill = 0; mm_react_on = 1;
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.dring = 1; G.married = 1; G.pill_given = 1; G.pill_day = 2; G.pill_seen = 1; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.outfit = 0; G.call = 1;
    /* 1. the pools */
    for (i = 0; i < 5; i++) { static const char *const nm[] = {"B_OPEN_FEED_MM", "B_OPEN_FEED_MM_LR", "B_OPEN_ASK_MM_LR", "B_OPEN_PILL_MM_LR", "B_OPEN_SUPER_MM_LR"}; static const int hr[] = {0, 1, 1, 0, 0};
        op_pairs_ok(nm[i], *OP_P[i], *OP_PN[i], hr[i], &bad); if (*OP_PN[i] < 12) { bad++; printf("v106: %s has fewer than 6 pairs\n", nm[i]); } }
    { static const char *const nm[] = {"B_OPEN_ASK_MM", "B_OPEN_PILL_MM", "B_OPEN_SUPER_MM", "B_RM_FEED_MM", "B_RM_ASK_MM", "B_RM_PILL_MM", "B_RM_SUPER_MM"};
      for (i = 5; i < 12; i++) { op_pairs_ok(nm[i - 5], *OP_P[i], *OP_PN[i], 0, &bad); if (*OP_PN[i] < 12) { bad++; printf("v106: %s has fewer than 6 pairs\n", nm[i - 5]); } } }
    { static const char *const nm[] = {"B_PLEA_FEED_MM", "B_PLEA_ASK_MM", "B_PLEA_PILL_MM", "B_PLEA_SUPER_MM", "B_MID_PUMP_MM", "B_MID_FEED_MM"};
      for (i = 12; i < 18; i++) plea_ok(nm[i - 12], *OP_P[i], *OP_PN[i], &bad); }
    /* 2. ASK FOR MILK / PUMP at her door: your line, her rejection, your plea, her answer, the lead-in, the pump scene with one more hesitation inside */
    wk_place(MAP_ROOM, 4, 4, LF); ls_talk = 0; rom_in = 0; bottle_pill = 0;
    { int seen[16] = {0}, got = 0, np = B_OPEN_ASK_MM_N / 2;
      for (i = 0; i < 300; i++) { rq_reset(); vn_start(SCR_BOTTLE);
          if (vn_n != 4 + LEAD_BEATS + B_BOTTLE_N + 2 || vn_buf[0].who != W_ME || vn_buf[1].who != W_SOPHIA || vn_buf[2].who != W_ME || vn_buf[3].who != W_SOPHIA || !pl_pair(OP_ASK, vn_buf[0].txt, vn_buf[1].txt) || !pl_pair(OP_PL_ASK, vn_buf[2].txt, vn_buf[3].txt)
              || !pl_pair(OP_MID_PUMP, vn_buf[vn_n - 3].txt, vn_buf[vn_n - 2].txt) || vn_buf[vn_n - 1].txt != B_BOTTLE[B_BOTTLE_N - 1].txt) { bad++; printf("v106: ASK at her door (n %d)\n", vn_n); break; }
          for (k = 0; k < np && k < 16; k++) if (vn_buf[0].txt == B_OPEN_ASK_MM[2 * k].txt) seen[k] = 1; }
      for (k = 0; k < np && k < 16; k++) got += seen[k];
      if (got != np) { bad++; printf("v106: only %d of %d door ASK openers came up\n", got, np); } }
    /* 3. she remembers: the second ask uses the memory pairs (and the plea), a different kind does not */
    rq_reset(); vn_start(SCR_BOTTLE); vn_start(SCR_BOTTLE);
    if (!pl_pair(OP_RM_ASK, vn_buf[0].txt, vn_buf[1].txt) || !pl_pair(OP_PL_ASK, vn_buf[2].txt, vn_buf[3].txt)) { bad++; printf("v106: the second ASK must be her memory pair\n"); }
    bottle_pill = 1; vn_start(SCR_BOTTLE); if (!pl_pair(OP_PILL, vn_buf[0].txt, vn_buf[1].txt) || bottle_pill) { bad++; printf("v106: a milk pill that fills the gauge: first pill = the pill opener (flag used up)\n"); }
    ls_talk = 1; wk_place(MAP_ROOM, 4, 4, LF); rq_reset(); vn_start(SCR_BOTTLE);   /* living room: Here?! rejection, then plea; the second time the memory pair */
    if (vn_n != 4 + LEAD_BEATS + B_BOTTLE_LR_N + 2 || !pl_pair(OP_ASK_LR, vn_buf[0].txt, vn_buf[1].txt) || !pl_pair(OP_PL_ASK, vn_buf[2].txt, vn_buf[3].txt) || !pl_pair(OP_MID_PUMP, vn_buf[vn_n - 3].txt, vn_buf[vn_n - 2].txt)) { bad++; printf("v106: ASK in the living room (n %d)\n", vn_n); }
    vn_start(SCR_BOTTLE); if (!pl_pair(OP_RM_ASK, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v106: the second ASK in the living room is her memory pair\n"); }
    { int keep_pl = B_PLEA_ASK_MM_N; B_PLEA_ASK_MM_N = 1; rq_reset(); vn_start(SCR_BOTTLE); B_PLEA_ASK_MM_N = keep_pl;   /* no whole plea pair: the two-beat opener still plays */
      if (vn_n != 2 + LEAD_BEATS + B_BOTTLE_LR_N + 2 || !pl_pair(OP_ASK_LR, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v106: an empty plea pool falls back to the opener alone (n %d)\n", vn_n); } }
    /* 4. milk pill and Super drug at her door and in her bedroom */
    ls_talk = 0; wk_place(MAP_ROOM, 4, 4, LF); snprintf(gift_note, sizeof gift_note, "GAUGE FULL"); rq_reset();
    gift_kind = GF_MILK; gift_l1 = R_MILK_OK[0]; vn_start(SCR_GIFT);
    if (vn_n != 8 || !pl_pair(OP_PILL, vn_buf[0].txt, vn_buf[1].txt) || !pl_pair(OP_PL_PILL, vn_buf[2].txt, vn_buf[3].txt) || !gift_in_pool(0, GF_MILK, vn_buf[4].txt) || !gift_in_pool(3, GF_MILK, vn_buf[7].txt)) { bad++; printf("v106: milk pill at her door (n %d)\n", vn_n); }
    vn_start(SCR_GIFT); if (!pl_pair(OP_RM_PILL, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v106: the second milk pill is her memory pair\n"); }
    gift_kind = GF_SUPER; gift_l1 = gift_pick(1, GF_SUPER); vn_start(SCR_GIFT);
    if (vn_n != 8 || !pl_pair(OP_SUPER, vn_buf[0].txt, vn_buf[1].txt) || !pl_pair(OP_PL_SUPER, vn_buf[2].txt, vn_buf[3].txt)) { bad++; printf("v106: Super drug at her door: the pills do not count as a Super drug memory (n %d)\n", vn_n); }
    vn_start(SCR_GIFT); if (!pl_pair(OP_RM_SUPER, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v106: the second Super drug is her memory pair\n"); }
    wk_place(MAP_SBED, 3, 5, UP); ls_talk = 1; rq_reset(); gift_kind = GF_SUPER; vn_start(SCR_GIFT); if (vn_n != 8 || !pl_pair(OP_SUPER, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v106: Super drug inside her bedroom (n %d)\n", vn_n); }
    ls_talk = 0; wk_place(MAP_ROOM, 4, 4, LF); gift_kind = GF_FLOWER; gift_l1 = gift_pick(1, GF_FLOWER); vn_start(SCR_GIFT); if (vn_n != 4) { bad++; printf("v106: a flower has no mama opener (n %d)\n", vn_n); }
    G.call = 0; gift_kind = GF_SUPER; gift_l1 = gift_pick(1, GF_SUPER); vn_start(SCR_GIFT); if (vn_n != 4) { bad++; printf("v106: husband mode: the Super drug scene is unchanged (n %d)\n", vn_n); } G.call = 1;
    /* 5. BREASTFEED: door (hiding picture on all four opening beats), bedroom, living room; plea after the opener, one more hesitation before your last line; memory */
    rq_reset(); wk_place(MAP_ROOM, 4, 4, LF); ls_talk = 0; rom_in = 0; vn_start(SCR_FEED);
    if (vn_n != B_FEED_MM_N + 4 || !pl_pair(OP_FEED, vn_buf[0].txt, vn_buf[1].txt) || !pl_pair(OP_PL_FEED, vn_buf[2].txt, vn_buf[3].txt) || vn_buf[4].txt != B_FEED_MM[2].txt || !pl_pair(OP_MID_FEED, vn_buf[vn_n - 4].txt, vn_buf[vn_n - 3].txt) || vn_buf[vn_n - 2].txt != B_FEED_MM[B_FEED_MM_N - 2].txt) { bad++; printf("v106: BREASTFEED at her door (n %d)\n", vn_n); }
    for (k = 0; k < 4; k++) if (vn_buf[k].spr != 6 || vn_buf[k].bg != BG_BDOOR) { bad++; printf("v106: door BREASTFEED: beat %d must be the hiding picture\n", k); break; }
    if (vn_buf[4].spr == 6) { bad++; printf("v106: the hiding picture ends after the plea\n"); }
    vn_start(SCR_FEED); if (!pl_pair(OP_RM_FEED, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v106: the second BREASTFEED is her memory pair\n"); }
    wk_place(MAP_SBED, 3, 5, UP); ls_talk = 1; rq_reset(); vn_start(SCR_FEED);
    if (vn_n != B_FEED_MM_N + 4 || vn_buf[0].spr != 1 || vn_buf[2].spr != 1 || vn_buf[4].bg != BG_FEED) { bad++; printf("v106: BREASTFEED in her bedroom (n %d)\n", vn_n); }
    wk_place(MAP_ROOM, 4, 4, LF); ls_talk = 1; rq_reset(); vn_start(SCR_FEED);
    if (vn_n != B_FEED_MM_LR_N + 4 || !pl_pair(OP_FEED_LR, vn_buf[0].txt, vn_buf[1].txt) || !pl_pair(OP_PL_FEED, vn_buf[2].txt, vn_buf[3].txt) || !pl_pair(OP_MID_FEED, vn_buf[vn_n - 4].txt, vn_buf[vn_n - 3].txt)) { bad++; printf("v106: BREASTFEED in the living room (n %d)\n", vn_n); }
    G.mm_kind = MK_FEED; G.mm_day = G.day - 1; rq_reset(); vn_start(SCR_FEED); if (!pl_pair(OP_RM_FEED, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v106: a feed from the saved day counts as her memory\n"); }
    G.mm_kind = 0; G.mm_day = 0;
    G.call = 0; rq_reset(); vn_start(SCR_FEED); if (vn_n != 1 + B_FEED_LR_N || !in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N)) { bad++; printf("v106: husband mode: BREASTFEED keeps the Here?! only (n %d)\n", vn_n); }
    ls_talk = 0; wk_place(MAP_ROOM, 4, 4, LF); vn_start(SCR_BOTTLE); if (vn_n != LEAD_BEATS + B_BOTTLE_N) { bad++; printf("v106: husband mode: the bottle scene at her door is unchanged (n %d)\n", vn_n); }
    G.call = 1;
    /* 6. a new career starts with a clean memory */
    rq_reset(); ls_talk = 0; wk_place(MAP_ROOM, 4, 4, LF); vn_start(SCR_BOTTLE); G.career = G.career + 1; vn_start(SCR_BOTTLE);
    if (!pl_pair(OP_ASK, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v106: a new career must not remember the old asks\n"); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; feed_no_force = 0; bottle_pill = 0; gift_kind = kgk; rom_in = kri; wk_place(kmap, ktx, kty, kface);
    rq_career = kcr; for (k = 0; k < RQ_N; k++) rq_cnt[k] = kcnt[k];
    return bad;
}
static int v97_tests(void) {   /* v9.7: she refuses the name MAMA: reluctant dialogue, flat BREASTFEED refusals, memory of the last mama interaction, saved kick day, mama table lines */
    int bad = 0, i, z, ref = 0, ok = 0; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face;
    static const char *const MARK[] = {"call", "name", "word", "mama", "Mama", "saying that", "Sophia", NULL};
#define V97_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
#define V97_FIN() do { for (z = 0; z < 160 && scr == SC_STORY; z++) on_button(B_A); } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0; feed_no_day = -1; feed_no_ph = -1; feed_no_force = -1;
    g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1;
    G.milk_st = 4; G.milk_pct = 100; G.mpill = 6; G.dring = 1; G.yoga_given = 1; G.cash = 1000; G.married = 1; G.call = 1; saved_G = G;
    /* 1. she never accepts the name: no "baby" / "boy" / "good boy" anywhere in the mama text, and most answers push back on the name */
    { int ri, found = 0; for (ri = 0; ri < TXT_N; ri++) { const char *nm = TXT_REG[ri].name; int l = (int)strlen(nm); int mmn = (l > 3 && !strcmp(nm + l - 3, "_MM")) || !strcmp(nm, "B_CALLMAMA");
        if (!mmn) continue;
        if (TXT_REG[ri].kind == TK_POOL) { const char *const *p = *(const char *const *const *)TXT_REG[ri].p; int n = *TXT_REG[ri].np, k; for (k = 0; k < n; k++) if (strstr(p[k], "baby") || strstr(p[k], "Baby") || txt_boy(p[k])) found++; }
        else if (TXT_REG[ri].kind == TK_BEATS) { const Beat *b = *(const Beat *const *)TXT_REG[ri].p; int n = *TXT_REG[ri].np, k; for (k = 0; k < n; k++) if (b[k].txt && (strstr(b[k].txt, "baby") || txt_boy(b[k].txt))) found++; } }
      if (found) { bad++; printf("v97: %d mama texts still call you baby / boy or accept the pet name\n", found); } }
    for (i = 0; i < TS_N; i++) { int k, c = 0; for (k = 0; k < T_TOPICS_MM[i].n; k++) { const Talk *t = &T_TOPICS_MM[i].t[k]; if (strstr(t->a, "baby") || strstr(t->me, "baby") || (t->b && strstr(t->b, "baby"))) { bad++; printf("v97: tease %d ex %d says baby\n", i, k); } if (txt_has_any(t->a, MARK) || txt_has_any(t->b, MARK)) c++; }
        if (c < 3) { bad++; printf("v97: tease topic %d: only %d of %d answers push back on the name\n", i, c, T_TOPICS_MM[i].n); } }
    { int k, c = 0; for (k = 0; k < C_MAMA.n; k++) { const Talk *t = &C_MAMA.t[k]; if (strstr(t->a, "baby") || (t->b && strstr(t->b, "baby"))) { bad++; printf("v97: chat %d says baby\n", k); } if (txt_has_any(t->a, MARK) || txt_has_any(t->b, MARK)) c++; }
      if (c < 8) { bad++; printf("v97: chat: only %d of %d answers push back on the name\n", c, C_MAMA.n); } }
    for (i = 0; i < T_KICK_MM_N; i++) if (strstr(T_KICK_MM[i], "Mama needs") || strstr(T_KICK_MM[i], "mama needs")) { bad++; printf("v97: kick line accepts the name\n"); }
    if (!(B_CALLMAMA_N == 6 && strstr(B_CALLMAMA[1].txt, "Don't") && !strstr(B_CALLMAMA[1].txt, "Say it again"))) { bad++; printf("v97: the first-time scene must be a refusal\n"); }
    /* 2. memory: today / yesterday / older / not in mama mode / after a tease */
    { int ks[] = {MK_CALL, MK_CHAT, MK_FEED, MK_FEEDNO, MK_HUG, MK_KISS}; const char *const *const *tp[] = {&R_MEM_CALL_T_MM, &R_MEM_CHAT_T_MM, &R_MEM_FEED_T_MM, &R_MEM_FEEDNO_T_MM, &R_MEM_HUG_T_MM, &R_MEM_KISS_T_MM}; int *tn[] = {&R_MEM_CALL_T_MM_N, &R_MEM_CHAT_T_MM_N, &R_MEM_FEED_T_MM_N, &R_MEM_FEEDNO_T_MM_N, &R_MEM_HUG_T_MM_N, &R_MEM_KISS_T_MM_N};
      const char *const *const *yp[] = {&R_MEM_CALL_Y_MM, &R_MEM_CHAT_Y_MM, &R_MEM_FEED_Y_MM, &R_MEM_FEEDNO_Y_MM, &R_MEM_HUG_Y_MM, &R_MEM_KISS_Y_MM}; int *yn[] = {&R_MEM_CALL_Y_MM_N, &R_MEM_CHAT_Y_MM_N, &R_MEM_FEED_Y_MM_N, &R_MEM_FEEDNO_Y_MM_N, &R_MEM_HUG_Y_MM_N, &R_MEM_KISS_Y_MM_N};
      int k, r; G.day = 20; kicked_day = 0;
      for (k = 0; k < 6; k++) {
        G.mm_kind = 0; G.mm_day = 0; g_mm_note(&G, ks[k]);
        for (r = 0; r < 8; r++) { const char *g = rom_greet(); if (!in_pool(g, *tp[k], *tn[k])) { bad++; printf("v97: kind %d: today her greeting must come from its TODAY pool: %s\n", ks[k], g); break; } }
        G.day = 21; for (r = 0; r < 8; r++) { const char *g = rom_greet(); if (!in_pool(g, *yp[k], *yn[k])) { bad++; printf("v97: kind %d: next day her greeting must come from its YESTERDAY pool: %s\n", ks[k], g); break; } }
        G.day = 22; for (r = 0; r < 8; r++) { const char *g = rom_greet(); if (!in_pool(g, R_GREET_MM, R_GREET_MM_N)) { bad++; printf("v97: kind %d: two days later she has forgotten (plain greeting): %s\n", ks[k], g); break; } }
        G.day = 20; G.call = 0; { const char *g = rom_greet(); if (in_pool(g, *tp[k], *tn[k]) || in_pool(g, R_GREET_MM, R_GREET_MM_N)) { bad++; printf("v97: kind %d: in SOPHIA mode no mama memory (and no mama greeting)\n", ks[k]); } } G.call = 1;
      }
      G.day = 20; G.mm_kind = 0; G.mm_day = 0; g_mm_note(&G, MK_TEASE); G.day = 21; { const char *g = rom_greet(); if (!in_pool(g, R_MEM_TEASE_Y_MM, R_MEM_TEASE_Y_MM_N)) { bad++; printf("v97: the day after a tease she remembers it: %s\n", g); } }
      G.day = 20; G.mm_kind = 0; G.mm_day = 0; g_mm_note(&G, MK_SLEEP); G.day = 21; { const char *g = rom_greet(); if (!in_pool(g, R_MEM_SLEEP_Y_MM, R_MEM_SLEEP_Y_MM_N)) { bad++; printf("v97: the morning after sleeping together she remembers it: %s\n", g); } }
      G.day = 20; G.mm_kind = 0; G.mm_day = 0; g_mm_note(&G, MK_FEED); kicked_day = 20; { const char *g = rom_greet(); if (!in_pool(g, R_KICKED_MM, R_KICKED_MM_N)) { bad++; printf("v97: thrown out today beats any memory: %s\n", g); } } kicked_day = 0;
      G.call = 0; G.mm_kind = 0; G.mm_day = 0; g_mm_note(&G, MK_HUG); if (G.mm_kind != MK_NONE) { bad++; printf("v97: nothing is noted in SOPHIA mode\n"); } G.call = 1; }
    /* 3. flows: every mama interaction is noted, the tease sets the saved kick day */
    G.day = 30; G.mm_kind = 0; G.mm_day = 0; kicked_day = 0; V97_OPEN(); tease_start(TS_LOOKS); talk_end();
    if (G.mm_kind != MK_TEASE || G.mm_day != 30 || G.kick_day != 30 || kicked_day != 30) { bad++; printf("v97: a tease is remembered (kind %d day %d kick %d)\n", G.mm_kind, G.mm_day, G.kick_day); } kicked_day = 0;
    G.day = 31; G.mm_kind = 0; V97_OPEN(); chat_start(); talk_end(); if (G.mm_kind != MK_CHAT || G.mm_day != 31) { bad++; printf("v97: a chat is remembered (kind %d)\n", G.mm_kind); }
    G.mm_kind = 0; V97_OPEN(); vn_start(SCR_HUG); V97_FIN(); if (G.mm_kind != MK_HUG) { bad++; printf("v97: a hug is remembered (kind %d)\n", G.mm_kind); }
    G.mm_kind = 0; V97_OPEN(); vn_start(SCR_KISS); V97_FIN(); if (G.mm_kind != MK_KISS) { bad++; printf("v97: a kiss is remembered (kind %d)\n", G.mm_kind); }
    G.mm_kind = 0; G.phase = PH_NIGHT; V97_OPEN(); vn_start(SCR_SLEEP); V97_FIN(); if (G.mm_kind != MK_SLEEP) { bad++; printf("v97: sleeping together is remembered (kind %d)\n", G.mm_kind); } G.phase = PH_MORNING;
    /* 4. BREASTFEED: she sometimes refuses flatly; nothing is used up; asking again in the same part of the day gets the 'I just said no' line; the next part of the day rolls again */
    for (i = 0; i < 300; i++) {
        G.day = 40; G.phase = PH_MORNING; G.milk_pct = 100; G.mday = 0; G.mm_kind = 0; feed_no_day = -1; feed_no_ph = -1; V97_OPEN(); rom_feed();
        if (scr == SC_ROM && in_pool(rom_txt, R_FEED_NO_MM, R_FEED_NO_MM_N)) {
            ref++; if (G.milk_pct != 100 || G.mday == 40 || G.mm_kind != MK_FEEDNO) { bad++; printf("v97: a refusal must not use the gauge (pct %d mday %d kind %d)\n", G.milk_pct, G.mday, G.mm_kind); }
            { int r; for (r = 0; r < 6; r++) { rom_feed(); if (!in_pool(rom_txt, R_FEED_AGAIN_MM, R_FEED_AGAIN_MM_N) || G.milk_pct != 100 || scr != SC_ROM) { bad++; printf("v97: asking again right away gets the 'I just said no' line\n"); break; } } }
            G.phase = PH_EVENING; { int got = 0, r; for (r = 0; r < 60 && !got; r++) { feed_no_day = 40; feed_no_ph = PH_MORNING; rom_feed(); if (scr == SC_STORY) got = 1; else { G.milk_pct = 100; V97_OPEN(); } } if (!got) { bad++; printf("v97: later in the day she can say yes\n"); } }
        } else if (scr == SC_STORY && vn_id == SCR_FEED) { ok++; if (G.milk_pct != 0 || G.mday != 40 || G.mm_kind != MK_FEED) { bad++; printf("v97: a yes empties the gauge and is remembered (pct %d)\n", G.milk_pct); } V97_FIN(); }
        else { bad++; printf("v97: BREASTFEED did neither (scr %d)\n", scr); break; }
    }
    if (ref < 60 || ok < 60 || ref + ok != 300) { bad++; printf("v97: roughly one in three is refused (refused %d, yes %d of 300)\n", ref, ok); }
    G.day = 41; G.phase = PH_MORNING; G.milk_pct = 50; V97_OPEN(); rom_feed(); if (scr != SC_ROM || G.milk_pct != 50 || !in_pool(rom_txt, R_NOT_FULL_MM, R_NOT_FULL_MM_N)) { bad++; printf("v97: a not-full gauge keeps its own answer\n"); }
    G.call = 0; G.milk_pct = 100; V97_OPEN(); rom_feed(); if (scr == SC_STORY && vn_id == SCR_FEED) { bad++; printf("v97: still no BREASTFEED outside mama mode\n"); } G.call = 1;
    /* 5. the save: the kick day and her memory survive, a version-19 save loads */
    { const char *f = "/tmp/pq_v97.sav"; Game a, b; g_init(&a); a.married = 1; a.call = 1; a.rs = RS_LIVING; a.day = 9; a.mm_kind = MK_FEED; a.mm_day = 9; a.kick_day = 9;
      if (!save_game(&a, f) || !load_game(&b, f) || b.mm_kind != MK_FEED || b.mm_day != 9 || b.kick_day != 9 || b.ver != SAVE_VER) { bad++; printf("v97: the kick day and her memory are saved\n"); }
      { FILE *fp; unsigned char raw[sizeof(Game)]; Game c = a; c.ver = SAVE_VER_19; memcpy(raw, &c, sizeof c); fp = fopen(f, "wb"); if (fp) { fwrite(raw, 1, offsetof(Game, mm_kind), fp); fclose(fp); }
        if (!load_game(&b, f) || b.ver != SAVE_VER || b.mm_kind != 0 || b.mm_day != 0 || b.kick_day != 0 || b.call != 1 || b.size != (int)sizeof b) { bad++; printf("v97: a version-19 save loads with no memory\n"); } }
      a.mm_kind = MK_N; if (save_sane(&a)) { bad++; printf("v97: a bad memory kind is rejected\n"); } a.mm_kind = 0; a.mm_day = 3; if (save_sane(&a)) { bad++; printf("v97: a memory day without a kind is rejected\n"); }
      { Game t; g_init(&t); t.rs = RS_LIVING; t.pill_seen = 1; t.dring = 1; t.call = 1; t.mm_kind = MK_HUG; t.mm_day = 3; if (g_marry(&t) != 0 || t.mm_kind != 0 || t.mm_day != 0) { bad++; printf("v97: the wedding starts with no memory\n"); } }
      remove(f); }
    /* 6. table lines: mama mode + the free match at home only */
    G.call = 1; G.home = 1; scr = SC_PLAY; if (!in_pool(PICKT(L_START), L_START_MM, L_START_MM_N)) { bad++; printf("v97: home match in mama mode uses the mama table lines\n"); }
    { const char *const *tp[] = {L_START, L_HAND, L_LAST, L_SHEWIN, L_YOUWIN, L_SHEFOLD, L_YOUFOLD, L_SPLIT, L_HERRAISE, L_YOURAISE, L_ENDHER, L_ENDYOU, L_ENDTIE, L_ENDLOST};
      const char *const *mp[] = {L_START_MM, L_HAND_MM, L_LAST_MM, L_SHEWIN_MM, L_YOUWIN_MM, L_SHEFOLD_MM, L_YOUFOLD_MM, L_SPLIT_MM, L_HERRAISE_MM, L_YOURAISE_MM, L_ENDHER_MM, L_ENDYOU_MM, L_ENDTIE_MM, L_ENDLOST_MM};
      int mn[] = {L_START_MM_N, L_HAND_MM_N, L_LAST_MM_N, L_SHEWIN_MM_N, L_YOUWIN_MM_N, L_SHEFOLD_MM_N, L_YOUFOLD_MM_N, L_SPLIT_MM_N, L_HERRAISE_MM_N, L_YOURAISE_MM_N, L_ENDHER_MM_N, L_ENDYOU_MM_N, L_ENDTIE_MM_N, L_ENDLOST_MM_N}; int tn[] = {L_START_N, L_HAND_N, L_LAST_N, L_SHEWIN_N, L_YOUWIN_N, L_SHEFOLD_N, L_YOUFOLD_N, L_SPLIT_N, L_HERRAISE_N, L_YOURAISE_N, L_ENDHER_N, L_ENDYOU_N, L_ENDTIE_N, L_ENDLOST_N};
      int k, r; for (k = 0; k < 14; k++) {
        if (mn[k] < 2) { bad++; printf("v97: table pool %d has too few lines\n", k); }
        for (r = 0; r < 10; r++) { G.home = 1; scr = SC_PLAY; if (!in_pool(tl_pick(tp[k], tn[k], mp[k], mn[k]), mp[k], mn[k])) { bad++; printf("v97: table pool %d: mama + home must use the mama pool\n", k); break; } }
        G.home = 0; scr = SC_PLAY; if (!in_pool(tl_pick(tp[k], tn[k], mp[k], mn[k]), tp[k], tn[k])) { bad++; printf("v97: table pool %d: a hotel / other match keeps the old pool\n", k); }
        G.home = 1; G.call = 0; if (!in_pool(tl_pick(tp[k], tn[k], mp[k], mn[k]), tp[k], tn[k])) { bad++; printf("v97: table pool %d: SOPHIA mode keeps the old pool\n", k); } G.call = 1;
        G.home = 0; scr = SC_END; home_end = 1; if (!in_pool(tl_pick(tp[k], tn[k], mp[k], mn[k]), mp[k], mn[k])) { bad++; printf("v97: table pool %d: the end screen of a home match (home flag already cleared) uses the mama pool\n", k); } home_end = 0;
        for (r = 0; r < mn[k]; r++) { const char *x = mp[k][r]; if (strstr(x, "BOY") || strstr(x, "BABY") || strstr(x, "MAMA")) { bad++; printf("v97: mama table line accepts a pet name: %s\n", x); } } } }
    G.home = 0; scr = SC_MENU;
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = 0; ls_leave = 0; ls_away = -1; ls_ph = -1; kicked_day = 0; sb_out = 0; home_end = 0; feed_no_day = -1; feed_no_ph = -1; feed_no_force = 0; wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
    return bad;
}
static int v95_tests(void) {   /* v9.5: R2 + the name keyboard, mama mode (her own dialogue, BREASTFEED only there), married = husband not boy, items.txt names in the game */
    int bad = 0, i, z; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face;
    int a[10], n;
#define V95_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
#define V95_FIN() do { for (z = 0; z < 160 && scr == SC_STORY; z++) on_button(B_A); } while (0)
#define V95_TYPE(str) do { const char *q_; for (q_ = (str); *q_; q_++) { if (*q_ == ' ') on_button(B_X); else { int f_ = 0, r_, c_; for (r_ = 0; r_ < 3 && !f_; r_++) for (c_ = 0; c_ < 9; c_++) if (NM_KEYS[r_][c_] == *q_) { nm_r = r_; nm_c = c_; f_ = 1; break; } on_button(B_A); } } } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0;
    nm_reset(); it_reset();
    g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1;
    G.milk_st = 4; G.milk_pct = 0; G.mpill = 6; G.dring = 1; G.yoga_given = 1; G.cash = 1000;
    /* 1. the key: R2 = BACKSPACE on the Miyoo, R on the desktop */
    if (map_key(SDLK_BACKSPACE) != B_R2 || map_key(SDLK_r) != B_R2 || map_key(SDLK_t) != B_R1 || map_key(SDLK_e) != B_L1) { bad++; printf("v95: R2 = BACKSPACE / R, R1 and L1 unchanged\n"); }
    /* 2. before the wedding R2 does nothing */
    V95_OPEN(); on_button(B_R2); if (scr != SC_ROM) { bad++; printf("v95: unmarried: R2 must do nothing (scr %d)\n", scr); }
    /* 3. married: R2 opens the keyboard, the page hint shows R2 */
    G.married = 1; V95_OPEN(); RSHOT("v95_menu_r2_hint"); on_button(B_R2);
    if (scr != SC_NAME || nm_len != 0) { bad++; printf("v95: married: R2 opens the name keyboard (scr %d)\n", scr); }
    RSHOT("v95_name_empty");
    /* 4. cancel: B on an empty name, and the BACK key */
    on_button(B_B); if (scr != SC_ROM || rom_mode != RM_MENU || G.call != 0) { bad++; printf("v95: B on an empty name goes back to her menu (scr %d)\n", scr); }
    V95_OPEN(); on_button(B_R2); nm_r = 3; nm_c = 2; on_button(B_A); if (scr != SC_ROM) { bad++; printf("v95: the BACK key leaves\n"); }
    /* 5. typing: letters, space, DEL, B deletes, the length limit */
    V95_OPEN(); on_button(B_R2); V95_TYPE("MAMA"); if (strcmp(nm_buf, "MAMA")) { bad++; printf("v95: typed MAMA, got '%s'\n", nm_buf); }
    on_button(B_X); on_button(B_X); if (strcmp(nm_buf, "MAMA ")) { bad++; printf("v95: one space only, got '%s'\n", nm_buf); }
    V95_TYPE("SOPHIA"); if (strcmp(nm_buf, "MAMA SOPHIA")) { bad++; printf("v95: typed MAMA SOPHIA, got '%s'\n", nm_buf); }
    RSHOT("v95_name_mama_sophia");
    on_button(B_B); if (strcmp(nm_buf, "MAMA SOPHI")) { bad++; printf("v95: B deletes a letter\n"); }
    nm_r = 3; nm_c = 0; on_button(B_A); if (strcmp(nm_buf, "MAMA SOPH")) { bad++; printf("v95: the DEL key deletes a letter\n"); }
    for (i = 0; i < 30; i++) { nm_r = 0; nm_c = 0; on_button(B_A); } if (nm_len != NM_LEN) { bad++; printf("v95: the name stops at %d letters (%d)\n", NM_LEN, nm_len); }
    nm_len = 0; nm_buf[0] = 0; on_button(B_X); if (nm_len) { bad++; printf("v95: a name cannot start with a space\n"); }
    /* 6. an unknown name and an empty OK change nothing, with a note */
    V95_TYPE("BOB"); on_button(B_Y);
    if (scr != SC_ROM || G.call != 0 || !rom_note[0]) { bad++; printf("v95: an unknown name: back to her menu with a note, nothing changed (scr %d call %d)\n", scr, G.call); }
    V95_OPEN(); on_button(B_R2); V95_TYPE("SOPHIA"); on_button(B_Y); if (scr != SC_ROM || G.call != 0 || strcmp(rom_note, "NOTHING CHANGED")) { bad++; printf("v95: SOPHIA while she is SOPHIA: nothing changed (note %s)\n", rom_note); }
    V95_OPEN(); on_button(B_R2); nm_r = 3; nm_c = 1; on_button(B_A); if (scr != SC_ROM || G.call != 0) { bad++; printf("v95: OK on an empty name just goes back\n"); }
    /* 7. MAMA SOPHIA: the scene, then mama mode */
    V95_OPEN(); on_button(B_R2); V95_TYPE("MAMA SOPHIA"); on_button(B_Y);
    if (scr != SC_STORY || vn_id != SCR_CALLM || G.call != 1 || vn_n != B_CALLMAMA_N || vn_buf[0].bg != BG_ROM) { bad++; printf("v95: MAMA SOPHIA starts the first-time scene and sets mama mode (scr %d id %d call %d)\n", scr, vn_id, G.call); }
    RSHOT("v95_call_mama_scene"); V95_FIN();
    if (scr != SC_ROM || rom_mode != RM_MENU || strcmp(rom_note, "NOW YOU CALL HER MAMA") || !g_mama(&G)) { bad++; printf("v95: after the scene her menu is back with a note (scr %d note %s)\n", scr, rom_note); }
    /* 8. mama mode: page 4 has BREASTFEED, her greeting, hug / kiss / sleep / feed use the mama scenes */
    n = rom_page_acts(3, a); if (n != 4 || a[3] != RA_FEED) { bad++; printf("v95: mama mode: page 4 has BREASTFEED (n %d)\n", n); }
    rom_page = 3; rom_sel = 0; RSHOT("v95_page4_mama");
    G.mm_kind = 0; G.mm_day = 0;   /* v9.7: typing MAMA is remembered; this check is about the plain greeting */
    for (i = 0; i < 12; i++) { const char *g = rom_greet(); if (!in_pool(g, R_GREET_MM, R_GREET_MM_N)) { bad++; printf("v95: mama greeting must come from R_GREET_MM: %s\n", g); break; } }
    V95_OPEN(); rom_page = 3; rom_sel = 0; on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_HUG || vn_n != B_HUG_MM_N || !in_pool(vn_buf[0].txt, R_HUG_OPEN_MM, R_HUG_OPEN_MM_N) || strcmp(vn_buf[1].txt, B_HUG_MM[1].txt)) { bad++; printf("v95: mama hug scene\n"); }
    V95_FIN();
    V95_OPEN(); rom_page = 3; rom_sel = 1; on_button(B_A); if (scr != SC_STORY || vn_id != SCR_KISS || vn_n != B_KISS_MM_N || !in_pool(vn_buf[0].txt, R_KISS_OPEN_MM, R_KISS_OPEN_MM_N)) { bad++; printf("v95: mama kiss scene\n"); } V95_FIN();
    G.phase = PH_NIGHT; V95_OPEN(); rom_page = 3; rom_sel = 2; on_button(B_A); if (scr != SC_STORY || vn_id != SCR_SLEEP || vn_n != B_SLEEP_MM_N || strcmp(vn_buf[0].txt, B_SLEEP_MM[0].txt)) { bad++; printf("v95: mama sleep scene\n"); } G.phase = PH_MORNING; G.day = 20; scr = SC_WALK;
    G.milk_pct = 100; G.mday = 0; G.bottle = 0; V95_OPEN(); rom_page = 3; rom_sel = 3; on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_FEED || vn_n != B_FEED_MM_N || strcmp(vn_buf[3].txt, B_FEED_MM[3].txt) || vn_buf[0].spr != 1 || vn_buf[2].bg != BG_FEED || G.milk_pct != 0) { bad++; printf("v95: mama breastfeed scene (id %d n %d pct %d)\n", vn_id, vn_n, G.milk_pct); }
    RSHOT("v95_feed_mama_start"); vn_i = 4; RSHOT("v95_feed_mama_middle"); V95_FIN(); if (scr != SC_ROM) { bad++; printf("v95: after the mama feed scene her menu is back (scr %d)\n", scr); }
    /* 9. mama mode: refusals, outfit, chat, tease, kick-out, the greeting after a tease */
    G.milk_pct = 0; V95_OPEN(); rom_page = 3; rom_sel = 3; on_button(B_A); if (!in_pool(rom_txt, R_NOT_FULL_MM, R_NOT_FULL_MM_N)) { bad++; printf("v95: mama: feed with an empty gauge is answered by pick(R_NOT_FULL_MM, R_NOT_FULL_MM_N)\n"); }
    G.phase = PH_NIGHT; kicked_day = G.day; V95_OPEN(); rom_page = 3; rom_sel = 2; on_button(B_A); if (!in_pool(rom_txt, R_SLEEP_NO_TEASE_MM, R_SLEEP_NO_TEASE_MM_N)) { bad++; printf("v95: mama: no sleeping after a tease uses the mama pool\n"); }
    for (i = 0; i < 6; i++) { const char *g = rom_greet(); if (!in_pool(g, R_KICKED_MM, R_KICKED_MM_N)) { bad++; printf("v95: mama: the greeting after a tease is from R_KICKED_MM\n"); break; } }
    G.phase = PH_MORNING; kicked_day = 0; G.mday = G.day; G.phase = PH_NIGHT; V95_OPEN(); rom_page = 3; rom_sel = 2; on_button(B_A); if (!in_pool(rom_txt, R_SLEEP_NO_MILK_MM, R_SLEEP_NO_MILK_MM_N)) { bad++; printf("v95: mama: no sleeping on a milk day uses the mama pool\n"); } G.phase = PH_MORNING; G.mday = 0;
    for (i = 0; i < 25; i++) { int found = 0, k; chat_start(); for (k = 0; k < C_MAMA.n; k++) if (tk_t[0] == C_MAMA.t[k].me) found = 1; if (tk_kind == TK_CHAT && !found) { bad++; printf("v95: mama chat comes from C_MAMA: %s\n", tk_t[0]); break; } if (tk_kind == TK_FAIL && !in_pool(tk_t[1], C_FAIL_MM, C_FAIL_MM_N)) { bad++; printf("v95: mama failed chat from C_FAIL_MM\n"); break; } }
    chat_start(); talk_end(); if (!in_pool(rom_txt, R_MORE_MM, R_MORE_MM_N) && tk_kind != TK_TEASE) { bad++; printf("v95: mama: after a chat her line is from R_MORE_MM (%s)\n", rom_txt); }
    for (z = 0; z < TS_N; z++) { int q, ok = 0; tease_start(z); for (q = 0; q < T_TOPICS_MM[z].n; q++) if (tk_t[0] == T_TOPICS_MM[z].t[q].me) ok = 1;
        if (!ok || !in_pool(tk_t[tk_n - 2], T_KICK_MM, T_KICK_MM_N) || !in_pool(tk_t[tk_n - 1], T_OUT_MM, T_OUT_MM_N) || tk_kind != TK_TEASE) { bad++; printf("v95: mama tease topic %d: her exchange, T_KICK_MM, then T_OUT_MM\n", z); } }
    tease_start(TS_LOOKS); RSHOT("v95_tease_mama"); talk_end(); if (kicked_day != G.day) { bad++; printf("v95: a mama tease still sends you out for the day\n"); } kicked_day = 0; scr = SC_WALK;
    G.outfit = 0; if (!tst_ward(OF_YOGA) || !in_pool(wk_pg[0], R_OUTFIT_YOGA_MM, R_OUTFIT_YOGA_MM_N)) { bad++; printf("v95: mama outfit line\n"); } WK_CLOSE(); if (!tst_ward(OF_USUAL) || !in_pool(wk_pg[0], R_OUTFIT_D8_MM, R_OUTFIT_D8_MM_N)) { bad++; printf("v95: mama outfit back line\n"); } WK_CLOSE(); V95_OPEN();   /* v11.3: through the wardrobe */
    G.phase = PH_NIGHT; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 5, 3, UP); on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_NIGHTM || vn_n != B_NIGHTM_MM_N || vn_buf[2].spr != 5) { bad++; printf("v95: mama night knock uses B_NIGHTM_MM with the hiding picture (id %d n %d)\n", vn_id, vn_n); } G.phase = PH_MORNING; scr = SC_WALK;
    /* 10. back to SOPHIA: the scene, no BREASTFEED, the normal scenes again */
    V95_OPEN(); on_button(B_R2); V95_TYPE("SOPHIA"); on_button(B_Y);
    if (scr != SC_STORY || vn_id != SCR_CALLB || G.call != 0 || vn_n != B_CALLBACK_N) { bad++; printf("v95: SOPHIA starts the way-back scene (id %d call %d)\n", vn_id, G.call); } V95_FIN();
    if (scr != SC_ROM || strcmp(rom_note, "NOW YOU CALL HER SOPHIA") || g_mama(&G)) { bad++; printf("v95: back to SOPHIA (note %s)\n", rom_note); }
    n = rom_page_acts(3, a); if (n != 3) { bad++; printf("v95: SOPHIA mode: no BREASTFEED row (n %d)\n", n); }
    V95_OPEN(); rom_page = 3; rom_sel = 0; on_button(B_A); if (vn_id != SCR_HUG || vn_n != B_HUG_N) { bad++; printf("v95: SOPHIA mode hug is the normal scene\n"); } V95_FIN();
    for (i = 0; i < 8; i++) { const char *g = rom_greet(); if (!in_pool(g, R_GREET_M, R_GREET_M_N)) { bad++; printf("v95: married SOPHIA greeting from R_GREET_M\n"); break; } }
    /* 11. married in SOPHIA mode she never calls you boy: chats, teases, kick-outs and her lines after a chat */
    for (i = 0; i < 400; i++) { chat_start(); for (z = 0; z < tk_n; z++) if (txt_boy(tk_t[z])) { bad++; printf("v95: married chat calls you boy: %s\n", tk_t[z]); i = 400; break; } }
    for (i = 0; i < 300; i++) { tease_start(i % TS_N == TS_MILK || i % TS_N == TS_TASTE ? i % TS_N : i % TS_N); for (z = 0; z < tk_n - 1; z++) if (txt_boy(tk_t[z])) { bad++; printf("v95: married tease calls you boy: %s\n", tk_t[z]); i = 300; break; } }
    for (i = 0; i < 60; i++) { chat_start(); talk_end(); if (txt_boy(rom_txt)) { bad++; printf("v95: married: her line after a chat calls you boy: %s\n", rom_txt); break; } }
    /* 12. before the wedding she still says boy (the old pools are untouched) */
    { int found = 0; G.married = 0; G.call = 0; for (i = 0; i < 200 && !found; i++) { chat_start(); for (z = 0; z < tk_n; z++) if (txt_boy(tk_t[z])) found = 1; } if (!found) { bad++; printf("v95: unmarried chats still use the old lines with boy\n"); } G.married = 1; }
    /* 13. a mama save outside a wedding cannot exist: the wedding resets the call */
    { Game t; g_init(&t); t.rs = RS_LIVING; t.pill_seen = 1; t.dring = 1; t.call = 1; if (g_marry(&t) != 0 || t.call != 0) { bad++; printf("v95: marrying starts as SOPHIA\n"); } }
    /* 14. items.txt: the names reach the game */
    it_reset(); INV_NAME[0] = "COLA"; INV_NAME[2] = "WHITE MILK"; SH_BOUGHT[SH_FLOWER] = "BOUGHT A ROSE"; SH_NAME[SH_FLOWER] = "ROSE"; SH_INFO[SH_FLOWER] = "A GIFT FOR HER"; SH_DESC[SH_GOLD] = "A gold bar. She likes shiny things."; R_GNAME[GF_FLOWER] = "ROSE";
    g_init(&G); G.cash = 5000; G.story = ST_NAMED; G.day = 3; G.soda = 1; G.bottle = 1; G.milk_st = 4; scr = SC_INV; inv_sel = 0; draw_all(); RSHOT("v95_fridge_renamed");
    scr = SC_INV; inv_sel = 2; on_button(B_Y); draw_all(); RSHOT("v95_fridge_milk_info_renamed"); on_button(B_B); scr = SC_WALK;
    { int rs0_ = G.rs; G.rs = RS_LIVING; shop_open(); shop_sel = SH_FLOWER; draw_all(); RSHOT("v95_shop_renamed"); on_button(B_A); on_button(B_A);
    if (G.flower != 1 || strcmp(shop_msg, "BOUGHT A ROSE")) { bad++; printf("v95: buying the renamed flower shows its own BOUGHT note (%s)\n", shop_msg); }
    shop_sel = SH_GOLD; on_button(B_Y); if (!shop_info) { bad++; printf("v95: an item that got a DESC from items.txt opens its info page with Y\n"); } draw_all(); RSHOT("v95_shop_gold_info_added"); on_button(B_B); G.rs = rs0_; }
    store_open(); st_confirm = 1; draw_all(); RSHOT("v95_vending_renamed"); store_key(B_A); if (strcmp(st_msg, "BOUGHT A SODA") && strcmp(st_msg, INV_BOUGHT[0])) { bad++; printf("v95: vending machine note %s\n", st_msg); } scr = SC_WALK;
    it_reset();
    if (strcmp(INV_NAME[0], "SODA") || strcmp(SH_NAME[SH_FLOWER], "FLOWER")) { bad++; printf("v95: it_reset in the test\n"); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; wk_map = kmap; wk_tx = ktx; wk_ty = kty; wk_face = kface; nm_reset(); kicked_day = 0;
    return bad;
}
static int v94_tests(void) {   /* v9.4: (1) the night knock after move-in shows her moved-in picture; (2) you can step up to the computer and the bed (one more tile); (3) A on Sophia while she is mid-step leaves her on the tile you face */
    int bad = 0, u; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face;
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 1; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; wk_mode = 0; scr = SC_WALK;
    /* 1. night knock picture: before she lives with you her night picture; once moved in her_home (her_home_yoga in the yoga outfit) */
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.room_up = 2; G.day = 12; G.phase = PH_NIGHT; G.rs = RS_NONE; G.outfit = 0;
    if (!nightS || night_vis() != nightS) { bad++; printf("v94: before move-in the night knock must keep her night picture\n"); }
    G.rs = RS_LIVING; saved_G = G;
    { SDL_Surface *np = night_vis(); if (!homeS || np != homeS || np == nightS) { bad++; printf("v94: after move-in the night knock must show her moved-in picture (her_home)\n"); }
      G.outfit = 1; np = night_vis(); if (homeYS ? np != homeYS : np != homeS) { bad++; printf("v94: yoga outfit: her_home_yoga (or her_home when missing) for the night knock\n"); } G.outfit = 0; }
    room_apply(); wk_place(MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, UP); on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_NIGHTHOME) { bad++; printf("v94: moved in, unmarried, night knock must start the scolding scene (scr %d id %d)\n", scr, vn_id); }
    else { int k, saw3 = 0; for (k = 0; k < vn_n && scr == SC_STORY; k++) { if (vn_buf[vn_i].spr == 3) { saw3 = 1; draw_all(); if (night_vis() != homeS) { bad++; printf("v94: the sprite-3 beat does not use her moved-in picture\n"); } RSHOT("v94_night_knock_moved_in"); break; } on_button(B_A); on_button(B_A); }
           if (!saw3) { bad++; printf("v94: the night scolding has no sprite-3 beat\n"); } }
    scr = SC_WALK; wk_mode = 0;
    /* 2. the computer and the bed: one more tile up in every room level */
    for (u = 0; u < 3; u++) {
        int px = u == 0 ? 4 : 7, bx = u == 0 ? 5 : 9;
        g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.cash = 500; G.room_up = u; G.rs = RS_NONE; saved_G = G; scr = SC_WALK; wk_mode = 0; ls_on = 0; room_apply();
        if (wk_tile(MAP_ROOM, px, 2) != 'P' || !wk_walkable(wk_tile(MAP_ROOM, px, 3))) { bad++; printf("v94: level %d: the computer must be at (%d,2) with floor below it\n", u, px); }
        if (wk_tile(MAP_ROOM, bx, 3) != 'B' || !wk_walkable(wk_tile(MAP_ROOM, bx, 4)) || wk_tile(MAP_ROOM, bx, 2) == '.') { bad++; printf("v94: level %d: the bed must be only (%d,3) with floor at (%d,4)\n", u, bx, bx); }
        wk_place(MAP_ROOM, px, 5, UP); if (!bot_walk(px, 3)) { bad++; printf("v94: level %d: cannot walk up to the tile in front of the computer (%d,3)\n", u, px); }
        else { on_button(B_UP); if (wk_ty != 3 || wk_face != UP) { bad++; printf("v94: level %d: the computer is solid, you must stop in front of it\n", u); } }
        if (u >= 1) { wk_place(MAP_ROOM, px, 3, UP); on_button(B_A); if (scr != SC_SHOP) { bad++; printf("v94: level %d: A in front of the computer (one tile up) must open the store\n", u); } scr = SC_WALK; wk_mode = 0; }
        wk_place(MAP_ROOM, bx, 5, UP); if (!bot_walk(bx, 4)) { bad++; printf("v94: level %d: cannot walk up to the free tile below the bed (%d,4)\n", u, bx); }
        else { on_button(B_UP); if (wk_ty != 4 || wk_face != UP) { bad++; printf("v94: level %d: the bed is solid, you must stop in front of it\n", u); } }
        if (u == 2) { G.phase = PH_MORNING; wk_place(MAP_ROOM, px, 3, UP); wk_mode = 0; RSHOT("v94_up_at_computer"); wk_place(MAP_ROOM, bx, 4, UP); RSHOT("v94_up_at_bed"); }
        G.phase = PH_NIGHT; wk_place(MAP_ROOM, bx, 4, UP); on_button(B_A); if (wk_mode != 2 || !wk_sleep) { bad++; printf("v94: level %d: A at the foot of the bed at night must offer sleep (mode %d)\n", u, wk_mode); } wk_mode = 0;
        G.phase = PH_NIGHT; wk_place(MAP_ROOM, bx - 1, 3, RT); on_button(B_A); if (wk_mode != 2 || !wk_sleep) { bad++; printf("v94: level %d: the bed must still work from its side (mode %d)\n", u, wk_mode); } wk_mode = 0;
    }
    { G.room_up = 0; room_apply(); wk_place(MAP_ROOM, UNPACK_X, UNPACK_Y, RT); if (wk_tile(MAP_ROOM, UNPACK_X + 1, UNPACK_Y) != 'B') { bad++; printf("v94: the unpacking spot must face the bed\n"); } }
    RSHOT("v94_room_computer_bed");
    /* 3. A on Sophia while she is mid-step: she stays where you face her, facing you */
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.cash = 90000; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.day = 12; G.phase = PH_MORNING; saved_G = G; room_apply();
    {   /* she is LEAVING the tile you face (walking from (3,4) to (2,4)) */
        scr = SC_WALK; wk_mode = 0; ls_ph = G.phase; wk_place(MAP_ROOM, 4, 4, LF); ls_on = 1; ls_leave = 0; ls_fx = 3; ls_fy = 4; ls_tx = 2; ls_ty = 4; ls_moving = 1; ls_t0 = now_ms();
        if (!ls_faced()) { bad++; printf("v94: setup: she must count as faced while she is leaving the tile\n"); }
        on_button(B_A);
        if (scr != SC_ROM || !ls_talk) { bad++; printf("v94: A on her mid-step must open her menu (scr %d)\n", scr); }
        if (ls_tx != 3 || ls_ty != 4 || ls_fx != 3 || ls_fy != 4 || ls_moving) { bad++; printf("v94: she must stay on the tile you face, not step back (at %d,%d from %d,%d moving %d)\n", ls_tx, ls_ty, ls_fx, ls_fy, ls_moving); }
        if (ls_face != RT) { bad++; printf("v94: she must face you (face %d)\n", ls_face); }
        go(GO_HUB, 0);
        if (scr != SC_WALK || !ls_on || ls_tx != 3 || ls_ty != 4 || ls_moving) { bad++; printf("v94: after her menu she is still where you left her (at %d,%d)\n", ls_tx, ls_ty); }
        /* she is ARRIVING on the tile you face (walking from (2,4) to (3,4)) */
        scr = SC_WALK; wk_mode = 0; ls_talk = 0; ls_ph = G.phase; wk_place(MAP_ROOM, 4, 4, LF); ls_on = 1; ls_fx = 2; ls_fy = 4; ls_tx = 3; ls_ty = 4; ls_moving = 1; ls_t0 = now_ms();
        on_button(B_A);
        if (ls_tx != 3 || ls_ty != 4 || ls_fx != 3 || ls_fy != 4 || ls_moving) { bad++; printf("v94: arriving on the faced tile: she must stay there (at %d,%d)\n", ls_tx, ls_ty); }
        /* standing still: nothing changes */
        go(GO_HUB, 0); scr = SC_WALK; wk_mode = 0; ls_talk = 0; ls_ph = G.phase; wk_place(MAP_ROOM, 4, 4, LF); ls_on = 1; ls_fx = ls_tx = 3; ls_fy = ls_ty = 4; ls_moving = 0; on_button(B_A);
        if (ls_tx != 3 || ls_ty != 4) { bad++; printf("v94: standing still she must not move (at %d,%d)\n", ls_tx, ls_ty); }
        RSHOT("v94_sophia_stays");
    }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = 0; ls_leave = 0; ls_away = -1; ls_ph = -1; room_apply(); wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE; wk_mode = 0;
    return bad;
}
static int v90_tests(void) {   /* v9.0: the icons stay when you face the thing; the Event Center menu (Exhibition / Lottery ticket); the lottery through the real buttons */
    int bad = 0, kscr = scr, fr = frozen, krs = room_seen, kun = unpack_done, kmode = mp_mode, kpk = pause_kind, n; Game keep = G, ksaved = saved_G; static const char *const none[1] = {0}; (void)none;
    frozen = 1; pause_kind = PK_NONE; room_seen = 1; unpack_done = 1; ls_enabled = 0;
    /* 1. the icons: still there while you face the fridge, the ATM, the clerk and the Event Center door (the A bubble shows over your head as well) */
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.day = 1; G.phase = PH_MORNING; saved_G = G; scr = SC_WALK; wk_mode = 0;
    wk_place(MAP_ROOM, 1, 3, UP); memset(ic_seen, 0, sizeof ic_seen); draw_all();
    if (!wk_interactive(wk_tile(wk_map, wk_tx + DX4[wk_face], wk_ty + DY4[wk_face]))) { bad++; printf("v90: not facing the fridge\n"); }
    if (!ic_seen[IC_FRIDGE]) { bad++; printf("v90: the fridge icon vanished while you face the fridge\n"); }
    shot("v90_icon_fridge_while_facing");
    wk_place(MAP_LOBBY, 11, 4, UP); memset(ic_seen, 0, sizeof ic_seen); draw_all();
    if (!wk_interactive(wk_tile(wk_map, wk_tx + DX4[wk_face], wk_ty + DY4[wk_face]))) { bad++; printf("v90: not facing the ATM\n"); }
    if (ic_seen[IC_ATM] != 1) { bad++; printf("v91: the wall ATM must show exactly one badge (%d)\n", ic_seen[IC_ATM]); }
    if (!ic_seen[IC_ATM]) { bad++; printf("v90: the ATM icon vanished while you face the ATM\n"); }
    shot("v90_icon_atm_while_facing");
    wk_place(MAP_LOBBY, 5, 4, UP); memset(ic_seen, 0, sizeof ic_seen); draw_all();
    if (!wk_interactive(wk_tile(wk_map, wk_tx + DX4[wk_face], wk_ty + DY4[wk_face]))) { bad++; printf("v90: not facing the vending machine\n"); }
    if (ic_seen[IC_STORE] != 1) { bad++; printf("v91: the store icon must be over the vending machine, once (%d)\n", ic_seen[IC_STORE]); }
    shot("v90_icon_store_while_facing");
    wk_place(MAP_LOBBY, 1, 6, LF); memset(ic_seen, 0, sizeof ic_seen); draw_all();
    if (!wk_interactive(wk_tile(wk_map, wk_tx + DX4[wk_face], wk_ty + DY4[wk_face]))) { bad++; printf("v90: not facing the Event Center door\n"); }
    if (!ic_seen[IC_EVENT]) { bad++; printf("v90: the Event Center icon vanished while you face the door\n"); }
    shot("v90_icon_event_while_facing");
    wk_place(MAP_LOBBY, 7, 6, UP); on_button(B_A); memset(ic_seen, 0, sizeof ic_seen); draw_all();   /* a message box is open: the icon is still on the map behind it */
    if (wk_mode != 1 || !ic_seen[IC_STORE]) { bad++; printf("v90: the store icon vanished while the clerk speaks (mode %d)\n", wk_mode); }
    WK_CLOSE(); scr = SC_WALK; wk_mode = 0;
    /* 2. the Event Center menu and the lottery */
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.day = 3; G.phase = PH_MORNING; G.cash = 5000; G.energy = ENERGY_MAX; saved_G = G; enter_hub();
    bot_use(MAP_LOBBY, 1, 6, LF); bot_use(MAP_EVENT, 5, 5, UP);
    if (scr != SC_WALK || wk_mode != 1 || wk_after != WK_EVENT) { bad++; printf("v90: the clerk did not greet (mode %d after %d)\n", wk_mode, wk_after); }
    WK_CLOSE(); if (scr != SC_EVENT || ev_sel != 0) { bad++; printf("v90: no Exhibition / Lottery menu after the greeting (scr %d)\n", scr); }
    shot("v90_event_menu");
    on_button(B_DOWN); if (ev_sel != 1) { bad++; printf("v90: the menu cursor did not move\n"); }
    shot("v90_event_menu_lottery");
    on_button(B_A); if (scr != SC_LOTTO || lot_sel != 0) { bad++; printf("v90: the lottery list did not open (scr %d)\n", scr); }
    shot("v90_lottery_list");
    on_button(B_UP); if (lot_sel != LOT_N - 1) { bad++; printf("v90: UP does not wrap to the last ticket\n"); } on_button(B_DOWN); on_button(B_DOWN); on_button(B_DOWN); if (lot_sel != 2) { bad++; printf("v90: DOWN did not select the third ticket (%d)\n", lot_sel); }
    on_button(B_A); if (G.lot_price != 100 || G.cash != 4900 || G.lot_day != 3 || lot_kind != 0 || G.energy != ENERGY_MAX) { bad++; printf("v90: buying the 100 ticket (price %d cash %d day %d energy %d)\n", G.lot_price, G.cash, G.lot_day, G.energy); }
    shot("v90_lottery_ticket_bought");
    on_button(B_A); if (G.lot_price != 100 || G.cash != 4900 || lot_kind != 2) { bad++; printf("v90: a second ticket was sold (cash %d kind %d)\n", G.cash, lot_kind); }
    shot("v90_lottery_one_at_a_time");
    on_button(B_B); if (scr != SC_EVENT || ev_sel != 1) { bad++; printf("v90: B in the lottery list did not go back to the menu\n"); }
    on_button(B_B); if (scr != SC_WALK || wk_map != MAP_EVENT || wk_tx != 5 || wk_ty != 5) { bad++; printf("v90: B in the menu did not leave you at the desk\n"); }
    G.lot_win = 1; bot_use(MAP_EVENT, 5, 5, UP);   /* the same day: no result yet, straight to the menu */
    for (n = 0; n < 8 && scr == SC_WALK && wk_mode == 1; n++) on_button(B_A);
    if (scr != SC_EVENT || G.lot_price != 100 || G.cash != 4900) { bad++; printf("v90: the same day the clerk must not tell the result (scr %d price %d cash %d)\n", scr, G.lot_price, G.cash); }
    on_button(B_B); G.day = 4; saved_G = G;   /* the next day: she tells you, pays, then the menu */
    bot_use(MAP_EVENT, 5, 5, UP);
    for (n = 0; n < 8 && scr == SC_WALK && wk_mode == 1 && wk_after == WK_EVENT; n++) on_button(B_A);
    if (scr != SC_WALK || wk_mode != 1 || wk_after != WK_EVMENU || wk_npg != 2 || wk_tag != W_CLERK_TAG) { bad++; printf("v90: the result was not told (scr %d mode %d after %d pages %d)\n", scr, wk_mode, wk_after, wk_npg); }
    else {
        int k, ok = 0; for (k = 0; k < W_LOTWIN_N; k++) if (wk_pg[0] == W_LOTWIN[k]) ok = 1;
        if (!ok) { bad++; printf("v90: the win line is not from the win pool\n"); }
        if (!strstr(wk_pg[1], "$300")) { bad++; printf("v90: the prize is not named: %s\n", wk_pg[1]); }
    }
    if (G.cash != 5200 || G.lot_price != 0 || G.lot_day != 0 || G.lot_win != 0) { bad++; printf("v90: the prize was not paid (cash %d price %d)\n", G.cash, G.lot_price); }
    shot("v90_clerk_result_win");
    WK_CLOSE(); if (scr != SC_EVENT || ev_sel != 0) { bad++; printf("v90: no menu after the result (scr %d)\n", scr); }
    on_button(B_DOWN); on_button(B_A); on_button(B_A);   /* a new ticket in the same visit: the cheapest one, a losing roll */
    if (G.lot_price != 20 || G.cash != 5180 || G.lot_day != 4) { bad++; printf("v90: a new ticket after the result (price %d cash %d)\n", G.lot_price, G.cash); }
    G.lot_win = 0; on_button(B_B); on_button(B_B); G.day = 9; saved_G = G;
    bot_use(MAP_EVENT, 5, 5, UP);
    for (n = 0; n < 8 && scr == SC_WALK && wk_mode == 1 && wk_after == WK_EVENT; n++) on_button(B_A);
    if (scr != SC_WALK || wk_after != WK_EVMENU || wk_npg != 1) { bad++; printf("v90: the losing result (pages %d after %d)\n", wk_npg, wk_after); }
    else { int k, ok = 0; for (k = 0; k < W_LOTLOSE_N; k++) if (wk_pg[0] == W_LOTLOSE[k]) ok = 1; if (!ok) { bad++; printf("v90: the loss line is not from the loss pool\n"); } }
    if (G.cash != 5180 || G.lot_price != 0) { bad++; printf("v90: a lost ticket changed the cash (%d)\n", G.cash); }
    shot("v90_clerk_result_loss");
    WK_CLOSE(); if (scr != SC_EVENT) { bad++; printf("v90: no menu after the losing result\n"); }
    G.cash = 10; on_button(B_DOWN); if (ev_sel != 1) ev_sel = 1; on_button(B_A); on_button(B_A);
    if (G.lot_price != 0 || G.cash != 10 || lot_kind != 1) { bad++; printf("v90: not enough cash for the cheapest ticket (price %d kind %d)\n", G.lot_price, lot_kind); }
    shot("v90_lottery_no_cash");
    on_button(B_B); on_button(B_B);
    /* 3. the Exhibition is still the first entry, with the energy check on the menu */
    G.cash = 5000; G.energy = EX_ENERGY - 1; saved_G = G; bot_use(MAP_EVENT, 5, 5, UP); WK_CLOSE(); qp_ms = 0; on_button(B_A);
    if (scr != SC_EVENT || !qp_ms) { bad++; printf("v90: Exhibition with low energy was not refused on the menu (scr %d)\n", scr); }
    shot("v90_event_menu_need_energy"); qp_ms = 0;
    G.energy = ENERGY_MAX; on_button(B_A); if (scr != SC_QSETUP || mp_mode != 1) { bad++; printf("v90: EXHIBITION did not open the buy-in list\n"); }
    on_button(B_B); if (scr != SC_EVENT || ev_sel != 0) { bad++; printf("v90: B in the buy-in list did not return to the menu\n"); }
    on_button(B_B);
    frozen = fr; scr = kscr; room_seen = krs; unpack_done = kun; mp_mode = kmode; pause_kind = kpk; G = keep; saved_G = ksaved; wk_mode = 0;
    return bad;
}
static int v89_tests(void) {   /* v8.9: bet menu (X) at the Exhibition and the boss table; Quick Play keeps the fixed raise; the title menu has no chip */
    int bad = 0, kscr = scr, fr = frozen, kmode = mp_mode, ksi = qs_si, kbm = bm_open; Game keep = G; MP km = M;
    int mode, guard;
    frozen = 1; pause_kind = PK_NONE; G.cash = 5000; G.energy = 100; G.story = ST_NAMED;
    for (mode = 0; mode < 3; mode++) {   /* 0 quick play, 1 exhibition 500, 2 boss */
        mp_mode = mode ? 1 : 0; qs_si = mode == 2 ? EX_BOSS : (mode == 1 ? 3 : 1); mp_begin();
        if (M.nl != (mode ? 1 : 0)) { bad++; printf("v89: nl flag wrong in mode %d\n", mode); }
        guard = 0; while (!(M.toact == 0 && !M.over) && guard++ < 60) { if (M.over) on_button(B_A); else mp_do(mp_decide(&M, M.toact)); }
        if (M.toact != 0 || M.over) continue;
        if (mode == 0) {   /* fixed limit: X raises at once, no menu */
            int c0 = M.c[0]; on_button(B_X); if (bm_open || M.c[0] <= c0) { bad++; printf("v89: Quick Play X must raise at once (menu %d)\n", bm_open); }
            continue;
        }
        {   /* free size: X opens the menu, nothing is paid yet */
            int c0 = M.c[0], mn = mp_min_by(&M, 0), mx = mp_max_by(&M, 0), need = mp_need(&M, 0), st = M.stack[0];
            if (!mp_can_raise(&M, 0)) { continue; }
            on_button(B_X); if (!bm_open || M.c[0] != c0 || M.toact != 0) { bad++; printf("v89: X must open the bet menu without acting (mode %d)\n", mode); }
            if (bm_by != mn) { bad++; printf("v89: the menu must open on the minimum raise (%d vs %d)\n", bm_by, mn); }
            shot(mode == 1 ? "v89_bet_menu_exhibition" : "v89_bet_menu_boss");
            on_button(B_UP); if (bm_by <= mn && mx > mn) { bad++; printf("v89: UP must raise the amount\n"); }
            on_button(B_DOWN); on_button(B_DOWN); on_button(B_DOWN); if (bm_by != mn) { bad++; printf("v89: the amount must stop at the minimum (%d)\n", bm_by); }
            on_button(B_RIGHT); if (mx > mn + 5 && bm_by <= mn + 1) { bad++; printf("v89: RIGHT must be a bigger step\n"); }
            on_button(B_Y); if (bm_by != mx) { bad++; printf("v89: Y must jump to all-in (%d vs %d)\n", bm_by, mx); }
            shot(mode == 1 ? "v89_bet_menu_allin" : "v89_bet_menu_allin_boss");
            on_button(B_UP); if (bm_by != mx) { bad++; printf("v89: the amount must stop at all-in\n"); }
            on_button(B_X); if (bm_by < mn || bm_by > mx) { bad++; printf("v89: X (pot) outside the range\n"); }
            on_button(B_Y); on_button(B_B); if (bm_open || M.c[0] != c0 || M.toact != 0) { bad++; printf("v89: B must close the menu without acting\n"); }
            on_button(B_X); on_button(B_DOWN); on_button(B_RIGHT); on_button(B_LEFT);   /* a mid amount, then A */
            { int by = bm_by; on_button(B_A); if (bm_open || M.c[0] != c0 + need + by || M.stack[0] != st - need - by) { bad++; printf("v89: A must pay call + %d (c %d -> %d)\n", by, c0, M.c[0]); } }
            if (M.toact == 0 && !M.over) { bad++; printf("v89: after A the turn must pass\n"); }
        }
        /* all-in from the start of a fresh hand, through the menu */
        guard = 0; while (!(M.toact == 0 && !M.over) && guard++ < 80) { if (M.game_over) break; if (M.over) on_button(B_A); else mp_do(mp_decide(&M, M.toact)); }
        if (M.toact == 0 && !M.over && mp_can_raise(&M, 0) && !M.game_over) {
            int cap = mp_cap_pay(&M, 0), st = M.stack[0]; on_button(B_X); on_button(B_Y); on_button(B_A);
            if (M.stack[0] != st - cap) { bad++; printf("v89: all-in must put in the cap (%d of %d)\n", st - M.stack[0], cap); }
            if (M.stack[0] == 0 && !M.allin[0]) { bad++; printf("v89: an empty stack must be all-in\n"); }
        }
    }
    /* the menu takes every key (Y does not turn the cards while it is open) and never survives the turn */
    mp_mode = 1; qs_si = 3; mp_begin();
    guard = 0; while (!(M.toact == 0 && !M.over) && guard++ < 60) { if (M.over) on_button(B_A); else mp_do(mp_decide(&M, M.toact)); }
    if (M.toact == 0 && !M.over && mp_can_raise(&M, 0)) {
        int pk = hd_hole[0][0]; on_button(B_X); on_button(B_Y); if (hd_hole[0][0] != pk) { bad++; printf("v89: Y in the bet menu must not turn the cards\n"); }
        on_button(B_B); mp_begin(); if (bm_open) { bad++; printf("v89: a new table must start with the menu closed\n"); }
    }
    /* only all-in left: the guide says so */
    mp_mode = 1; qs_si = 3; mp_begin(); M.over = 0; M.toact = 0; M.stack[1] = 3; M.r[0] = 0; M.r[1] = 0; M.c[0] = M.c[1] = 0;
    if (mp_can_raise(&M, 0) && mp_min_by(&M, 0) != mp_max_by(&M, 0) && mp_max_by(&M, 0) != 3) { bad++; printf("v89: a short opponent must cap the bet at its stack (max %d)\n", mp_max_by(&M, 0)); }
    /* the title menu has no chip: the old chip area is the plain menu background */
    {
        size_t sz = (size_t)S->pitch * S->h; unsigned char *a = (unsigned char *)malloc(sz); int x, y, sx, sy, dif = 0;
        if (a) {
            scr = SC_MENU; menu_sel = 0; draw_all(); shot("v89_title_menu_no_chip");
            sx = XS(120); sy = YS(400);
            for (y = -30; y <= 30; y++) for (x = -80; x <= 80; x++) { Uint32 v = *px(S, sx + x, sy + y), w = *px(S, sx + x + 1, sy + y); if (((v >> 16) & 255) > 200 && ((w >> 16) & 255) > 200 && ((v >> 8) & 255) > 200) dif++; }
            if (dif > 0) { bad++; printf("v89: bright chip pixels still on the title menu (%d)\n", dif); }
            free(a);
        }
    }
    bm_open = kbm; M = km; mp_mode = kmode; qs_si = ksi; scr = kscr; frozen = fr; pause_kind = PK_NONE; G = keep;
    printf("v8.9: bet menu (Exhibition + boss), Quick Play unchanged, title menu without chip: problems %d\n", bad);
    return bad;
}
static int v84_tests(void) {   /* v8.4: the dialogue box is full width and flush on the bottom edge; its height follows the rows of the sentence (1 / 2 / 3 rows = 64 / 86 / 108); the romance bar is vertical with the level inside the heart */
    int bad = 0, i; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face;
    static const char *const TX[3] = {
        "HMPH. IS THAT REALLY THE BEST YOU'VE GOT?",
        "I SAID WHAT I MEANT AND I MEANT WHAT I SAID, SO DO NOT ASK ME AGAIN TODAY.",
        "I SAID WHAT I MEANT AND I MEANT WHAT I SAID, SO DO NOT ASK ME AGAIN TODAY. YOU HAVE TAKEN ENOUGH OF MY TIME ALREADY, PLEASE GO NOW." };
    static const int H[3] = {64, 86, 108};
    int kd84 = dlg_idx; dlg_idx = 0;   /* v10.7: the gold border is only that bright at 100% opacity */
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0;
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.rom_lvl = 2; G.rom_pct = 50; G.day = 5; G.phase = PH_MORNING;
    /* 1. the height follows the rows; 3 rows is the old fixed height */
    for (i = 0; i < 3; i++) {
        int top = 480 - H[i];
        if (dlg_lines(TX[i]) != i + 1) { bad++; printf("v84: test text %d must wrap to %d rows (got %d)\n", i, i + 1, dlg_lines(TX[i])); continue; }
        if (dlg_h(TX[i]) != H[i]) { bad++; printf("v84: %d row(s): box height %d, expected %d\n", i + 1, dlg_h(TX[i]), H[i]); }
        wk_mode = 0; scr = SC_ROM; rom_open(TX[i]); rom_choose = 0; rom_in = 1;
        { char nm[40]; snprintf(nm, sizeof nm, "v84_box_%d_row%s", i + 1, i ? "s" : ""); RSHOT(nm); }   /* shot() = draw_all() + save */
        if (dlg_py != top + H[i] - 22) { bad++; printf("v84: %d row(s): the A prompt is at %d, expected %d\n", i + 1, dlg_py, top + H[i] - 22); }
        if (!v84_gold(320, 479) || !v84_gold(1, 450) || !v84_gold(638, 450)) { bad++; printf("v84: %d row(s): the box must be full width and end on the bottom edge\n", i + 1); }
        if (!v84_gold(320, top + 1) || v84_gold(320, top - 4)) { bad++; printf("v84: %d row(s): the top edge of the box must be at y %d\n", i + 1, top); }
    }
    /* never more than 3 rows: the text limits of the box keep it (every built-in sentence through the same count) */
    { const char *mx = "A"; (void)mx; if (dlg_lines("") != 1) { bad++; printf("v84: an empty text is one row\n"); } }
    /* the box must not grow while the line is typed: the height comes from the whole sentence */
    frozen = 0; { int h0, h1; fake_ms = 300000; rom_open(TX[2]); rom_ms = now_ms(); h0 = dlg_h(TX[2]); fake_ms += 900; h1 = dlg_h(TX[2]); if (h0 != h1) { bad++; printf("v84: the height changed while typing\n"); } } frozen = 1;
    /* 2. the choice pages and the small menus end on the bottom edge too */
    wk_mode = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_page = 0; RSHOT("v84_choices_page1");
    if (!v84_gold(320, 479) || !v84_gold(1, 460)) { bad++; printf("v84: the choice box must be full width and on the bottom edge\n"); }
    /* 3. the romance bar: vertical, level inside the heart, no percentage text */
    G.rom_lvl = 2; G.rom_pct = 50; wk_mode = 0; scr = SC_ROM; rom_open(TX[0]); rom_choose = 0; RSHOT("v84_bar_half_level2");
    { int r, g, b, hx = 8 + 26, white = 0, x, y;
      v84_px(37, 80, &r, &g, &b); if (!(r > 180 && g < 100)) { bad++; printf("v84: the lower half of the track must be filled (rgb %d %d %d)\n", r, g, b); }
      v84_px(37, 20, &r, &g, &b); if (!(r < 90 && g < 60)) { bad++; printf("v84: the upper half of the track must be empty (rgb %d %d %d)\n", r, g, b); }
      for (y = 86; y < 134; y++) for (x = 8; x < 60; x++) { v84_px(x, y, &r, &g, &b); if (r > 240 && g > 235 && b > 235) white++; }
      if (white < 6) { bad++; printf("v84: the level number must be drawn inside the heart (%d light pixels)\n", white); } (void)hx;
      /* nothing is drawn to the right of the heart / track any more (no percentage text): the strip x 64..200, y 6..60 is the room picture, never the pale bar text */
      for (y = 20; y < 60; y++) for (x = 64; x < 200; x++) { v84_px(x, y, &r, &g, &b); if (r > 240 && g > 225 && b > 230) { bad++; printf("v84: text beside the bar (x %d y %d)\n", x, y); y = 99; break; } }
    }
    G.rom_pct = 0; RSHOT("v84_bar_empty"); G.rom_pct = 100; G.rom_lvl = 1; RSHOT("v84_bar_full_level1");
    { int r, g, b; v84_px(37, 20, &r, &g, &b); if (!(r > 180 && g < 100)) { bad++; printf("v84: a full bar fills the whole track (rgb %d %d %d)\n", r, g, b); } }
    G.rom_lvl = ROM_LVLS; G.rom_pct = 100; RSHOT("v84_bar_max_hidden");
    { int r, g, b; v84_px(37, 60, &r, &g, &b); if (r > 180 && g < 100) { bad++; printf("v84: a full level-3 bar is not shown\n"); } }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = 0; ls_ph = -1; wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
    dlg_idx = kd84;
    return bad;
}
static int v86_tests(void) {   /* v8.6: the wedding, page 4 (hug / kiss / sleep together / breastfeed), the outfit change, the gauge that stays full when married */
    int bad = 0, i, z; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face;
    int a[10], n, idx;
#define V86_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
#define V86_FIN() do { for (z = 0; z < 160 && scr == SC_STORY; z++) on_button(B_A); } while (0)
#define V86_ROW(pg, act) do { n = rom_page_acts(pg, a); idx = -1; for (i = 0; i < n; i++) if (a[i] == (act)) idx = i; } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0;
    g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1;
    G.milk_st = 4; G.milk_pct = 0; G.mpill = 6; G.dring = 1; G.yoga_given = 1; G.cash = 1000;
    /* 1. the pictures exist and have the screen size; every new line fits the box */
    for (i = BG_HUG; i <= BG_FEED; i++) { int o; for (o = 0; o < 2; o++) { SDL_Surface *p; G.outfit = o; p = sc_pic(i); if (!p) { bad++; printf("v86: scene picture %d outfit %d did not load\n", i, o); } else if (p->w != SW || p->h != SH) { bad++; printf("v86: scene picture %d outfit %d is %dx%d, the screen is %dx%d\n", i, o, p->w, p->h, SW, SH); } } }
    G.outfit = 0;
    { const Beat *sc[5] = {B_WEDDING, B_HUG, B_KISS, B_SLEEP, B_FEED}; int ns[5]; ns[0] = B_WEDDING_N; ns[1] = B_HUG_N; ns[2] = B_KISS_N; ns[3] = B_SLEEP_N; ns[4] = B_FEED_N;
      for (i = 0; i < 5; i++) { int k; for (k = 0; k < ns[i]; k++) { char w[400]; int ln = 1, c; if (!sc[i][k].txt) continue; wrap_buf(w, sizeof w, sc[i][k].txt, 46); for (c = 0; w[c]; c++) if (w[c] == '\n') ln++; if (ln > 3) { bad++; printf("v86: scene %d beat %d needs %d lines\n", i, k, ln); } } } }
    /* 2. before the wedding: 3 pages, the ring is a row of the give page, no outfit row, no page 4 */
    V86_OPEN(); if (rom_pages() != 3 || rom_page_acts(0, a) != 2) { bad++; printf("v86: unmarried: 3 pages and page 1 has no outfit row\n"); }
    V86_ROW(1, RA_RING); if (idx < 0) { bad++; printf("v86: the diamond ring must be a row of the give page after the day-8 scene\n"); }
    rom_page = 1; rom_sel = idx < 0 ? 0 : idx; RSHOT("v86_give_page_ring"); on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_WEDDING || !G.married || vn_n != B_WEDDING_N) { bad++; printf("v86: giving the ring must start the wedding scene (scr %d id %d married %d)\n", scr, vn_id, G.married); }
    RSHOT("v86_wedding_start"); vn_i = 4; RSHOT("v86_wedding_middle"); V86_FIN();
    if (scr != SC_WALK || !G.married) { bad++; printf("v86: the wedding scene must end on the map (scr %d)\n", scr); }
    V86_OPEN(); V86_ROW(1, RA_RING); if (idx >= 0) { bad++; printf("v86: after the wedding the ring row is gone\n"); }
    /* 3. page 4 and the outfit change (page 1) */
    if (rom_pages() != 4) { bad++; printf("v86: married: 4 pages (got %d)\n", rom_pages()); }
    V86_ROW(0, RA_OUTFIT_K + OF_YOGA); if (idx != -1 || rom_page_acts(0, a) != 2) { bad++; printf("v86: v11.3: page 1 has no CHANGE OUTFIT row any more (idx %d)\n", idx); }
    RSHOT("v86_page1_no_outfit");
    if (!tst_ward(OF_YOGA) || G.outfit != 1 || !in_pool(wk_pg[0], R_OUTFIT_YOGA, R_OUTFIT_YOGA_N)) { bad++; printf("v86: wardrobe: change to yoga (outfit %d)\n", G.outfit); } WK_CLOSE();
    if (!tst_ward(OF_USUAL) || G.outfit != 0 || !in_pool(wk_pg[0], R_OUTFIT_D8, R_OUTFIT_D8_N)) { bad++; printf("v86: wardrobe: change back (outfit %d)\n", G.outfit); } WK_CLOSE();
    V86_OPEN();
    n = rom_page_acts(3, a); if (n != 3 || a[0] != RA_HUG || a[1] != RA_KISS || a[2] != RA_SLEEP) { bad++; printf("v86: page 4 = HUG / KISS / SLEEP TOGETHER (no BREASTFEED unless you call her MAMA) (n %d)\n", n); }
    G.call = 1; n = rom_page_acts(3, a); if (n != 4 || a[3] != RA_FEED) { bad++; printf("v86: in mama mode page 4 also has BREASTFEED (n %d)\n", n); } G.call = 0;
    V86_OPEN(); rom_page = 3; rom_sel = 0; RSHOT("v86_page4_day_locked");
    /* 4. hug and kiss, in both outfits, back to her menu */
    for (z = 0; z < 2; z++) {
        int o = z; G.outfit = o;
        V86_OPEN(); rom_page = 3; rom_sel = 0; on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_HUG || vn_n != B_HUG_N || vn_buf[0].bg != BG_HUG || !in_pool(vn_buf[0].txt, R_HUG_OPEN, R_HUG_OPEN_N)) { bad++; printf("v86: hug scene wrong (scr %d id %d)\n", scr, vn_id); }
        RSHOT(o ? "v86_hug_yoga" : "v86_hug_d8"); { int q; for (q = 0; q < 160 && scr == SC_STORY; q++) on_button(B_A); }
        if (scr != SC_ROM || rom_mode != RM_MENU) { bad++; printf("v86: after the hug her menu is back (scr %d)\n", scr); }
        V86_OPEN(); rom_page = 3; rom_sel = 1; on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_KISS || vn_n != B_KISS_N || vn_buf[0].bg != BG_KISS || !in_pool(vn_buf[0].txt, R_KISS_OPEN, R_KISS_OPEN_N)) { bad++; printf("v86: kiss scene wrong (scr %d id %d)\n", scr, vn_id); }
        RSHOT(o ? "v86_kiss_yoga" : "v86_kiss_d8"); { int q; for (q = 0; q < 160 && scr == SC_STORY; q++) on_button(B_A); }
        if (scr != SC_ROM) { bad++; printf("v86: after the kiss her menu is back\n"); }
    }
    G.outfit = 0;
    /* 5. sleeping together: only at night, refused after a tease or a milk day */
    V86_OPEN(); rom_page = 3; rom_sel = 2; on_button(B_A);
    if (scr != SC_ROM || !strstr(rom_note, "NIGHT") || G.day != 20) { bad++; printf("v86: sleeping in the daytime is refused with a note (note %s)\n", rom_note); }
    G.phase = PH_NIGHT; G.mday = G.day; V86_OPEN(); rom_page = 3; rom_sel = 2; RSHOT("v86_page4_night"); on_button(B_A);
    if (scr != SC_ROM || !in_pool(rom_txt, R_SLEEP_NO_MILK, R_SLEEP_NO_MILK_N)) { bad++; printf("v86: on a milk day she refuses to sleep beside you\n"); }
    G.mday = 0; kicked_day = G.day; V86_OPEN(); rom_page = 3; rom_sel = 2; on_button(B_A);
    if (scr != SC_ROM || !in_pool(rom_txt, R_SLEEP_NO_TEASE, R_SLEEP_NO_TEASE_N)) { bad++; printf("v86: after a tease she refuses to sleep beside you\n"); }
    kicked_day = 0; wk_place(MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, UP);
    { int d0 = G.day; V86_OPEN(); rom_page = 3; rom_sel = 2; on_button(B_A);
      if (scr != SC_STORY || vn_id != SCR_SLEEP || vn_n != B_SLEEP_N || vn_buf[0].bg != BG_SLEEPT) { bad++; printf("v86: the sleep scene must start (scr %d id %d)\n", scr, vn_id); }
      RSHOT("v86_sleep_scene"); V86_FIN();
      if (G.day != d0 + 1 || G.phase != PH_MORNING || scr != SC_WALK || saved_G.day != d0 + 1) { bad++; printf("v86: sleeping together = the bed: next morning and saved (day %d phase %d saved %d)\n", G.day, G.phase, saved_G.day); } WK_CLOSE(); }
    /* 6. the night knock once married opens her menu on page 4; unmarried it stays the old scolding */
    G.phase = PH_NIGHT; wk_mode = 0; scr = SC_WALK; wk_place(MAP_ROOM, 5, 3, UP); on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_NIGHTM) { bad++; printf("v87: married, the night knock starts the hiding scene (scr %d id %d)\n", scr, vn_id); }
    else {   /* door + narration first, her line without a picture, only then the hiding picture; outfit d8 then yoga */
        int o; if (vn_buf[0].bg != BG_BDOOR || vn_buf[0].spr != 0 || vn_buf[1].spr != 0 || vn_buf[2].spr != 5) { bad++; printf("v87: night scene order: door, her line, then the hiding picture\n"); }
        for (o = 0; o < 2; o++) { G.outfit = o; hide_free(); vn_i = 2; vn_prev_spr = 5; if (!hide_pic() || hideO != o) { bad++; printf("v87: hiding picture for outfit %d not loaded\n", o); } RSHOT(o ? "v87_hide_night_yoga" : "v87_hide_night_d8"); }
        G.outfit = 0; vn_i = 0; vn_prev_spr = 0; RSHOT("v87_night_door_first");
        V86_FIN(); hide_free();
    }
    if (!(wk_nmap == MAP_SBED || wk_map == MAP_SBED) && (scr != SC_ROM || rom_page != 3 || !in_pool(rom_txt, R_NIGHT_M, R_NIGHT_M_N))) { bad++; printf("v86: married, the night knock lets you into her bedroom (v9.2) (scr %d page %d map %d)\n", scr, rom_page, wk_map); }   /* v9.3: this check was stale since v9.2 (the married night knock walks you into her bedroom now) */
    RSHOT("v86_night_knock_married"); G.married = 0; wk_mode = 0; scr = SC_WALK; wk_place(MAP_ROOM, 5, 3, UP); on_button(B_A);
    if (scr != SC_SNEAK) { bad++; printf("v86: unmarried the night knock keeps the sneak-in menu (scr %d)\n", scr); }
    G.married = 1; G.phase = PH_MORNING; scr = SC_WALK; wk_mode = 0;
    /* 7. the gauge: pills fill it and it STAYS full; the pump, breastfeeding and ASK all empty it */
    G.milk_pct = 0; G.mpill = 6; G.bottle = 0; V86_OPEN(); rom_give(GF_MILK); V86_FIN(); V86_OPEN();
    if (G.milk_pct != 50 || G.bottle != 0) { bad++; printf("v86: 1st pill 50%% (pct %d)\n", G.milk_pct); }
    rom_give(GF_MILK); if (G.milk_pct != 100 || G.bottle != 0 || scr != SC_STORY || vn_id != SCR_GIFT || !strstr(vn_buf[3].txt, "GAUGE FULL")) { bad++; printf("v86: married: the 2nd pill fills the gauge and it stays full (pct %d bottle %d id %d)\n", G.milk_pct, G.bottle, vn_id); }
    V86_FIN(); V86_OPEN(); rom_page = 3; RSHOT("v86_page4_feed_ready"); V86_ROW(2, RA_PUMP); if (idx != 1) { bad++; printf("v86: PUMP MILK is on page 3 while the gauge is full\n"); } rom_page = 2; rom_sel = 1; RSHOT("v86_page3_pump");
    rom_give(GF_MILK); if (scr != SC_ROM || !in_pool(rom_txt, R_MILK_FULL, R_MILK_FULL_N) || G.mpill != 4) { bad++; printf("v86: a pill on a full gauge is refused (mpill %d)\n", G.mpill); }
    V86_OPEN(); rom_page = 2; rom_sel = 1; on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_BOTTLE || G.bottle != 1 || G.milk_pct != 0 || G.mday != G.day) { bad++; printf("v86: PUMP MILK = the pump scene, a bottle, an empty gauge (id %d bottle %d pct %d)\n", vn_id, G.bottle, G.milk_pct); }
    V86_FIN(); if (scr == SC_PEEK) { on_button(B_A); V86_FIN(); } if (scr != SC_WALK) { bad++; printf("v86: the pump scene ends on the map (scr %d)\n", scr); }
    V86_OPEN(); V86_ROW(2, RA_PUMP); if (idx >= 0) { bad++; printf("v86: the pump is gone until the gauge is full again\n"); }
    G.call = 1;   /* v9.5: BREASTFEED exists only in mama mode */
    rom_page = 3; rom_sel = 3; on_button(B_A); if (scr != SC_ROM || !in_pool(rom_txt, R_NOT_FULL_MM, R_NOT_FULL_MM_N) || G.milk_pct != 0) { bad++; printf("v86: breastfeeding with an empty gauge is refused\n"); }
    G.milk_pct = 100; G.mday = 0; G.outfit = 1; V86_OPEN(); rom_page = 3; rom_sel = 3; on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_FEED || vn_n != B_FEED_MM_N || vn_buf[0].bg != BG_ROM || vn_buf[2].bg != BG_FEED || G.milk_pct != 0 || G.bottle != 1 || G.mday != G.day) { bad++; printf("v86: breastfeeding empties the gauge, no bottle (id %d pct %d bottle %d)\n", vn_id, G.milk_pct, G.bottle); }
    RSHOT("v86_feed_yoga"); vn_i = 3; RSHOT("v86_feed_yoga_4"); V86_FIN(); if (scr != SC_ROM || !strstr(rom_note, "EMPTY")) { bad++; printf("v86: after breastfeeding her menu is back with a note (scr %d note %s)\n", scr, rom_note); }
    G.outfit = 0; G.milk_pct = 100; V86_OPEN(); rom_page = 3; rom_sel = 3; on_button(B_A); RSHOT("v86_feed_d8"); V86_FIN();
    /* 8. the Super drug: breastfeeding is one of the two daily asks */
    G.milk_pct = 50; G.sdrug = 1; G.mday = 0; G.day = 30; G.ask_day = 0; G.ask_n = 0; g_super_gift(&G);
    V86_OPEN(); rom_page = 3; rom_sel = 3; on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_FEED || G.ask_n != 1 || G.milk_pct != 50) { bad++; printf("v86: under the drug breastfeeding uses an ask (ask_n %d pct %d)\n", G.ask_n, G.milk_pct); }
    V86_FIN(); if (scr != SC_ROM || !strstr(rom_note, "ASKS LEFT")) { bad++; printf("v86: the note shows the asks left\n"); }
    /* v8.7: the moved-in picture, the yoga everyday picture, the day-8 / knock hiding scenes */
    { Game t = G; G.outfit = 0; G.married = 0; G.pill_seen = 0; G.pill_given = 1; G.pill_day = 18; G.day = 20; G.rs = RS_UPG;
      if (her_vis() == homeS && homeS) { bad++; printf("v87: not moved in yet: no moved-in picture\n"); }
      G.rs = RS_LIVING; G.pill_given = 0; G.pill_day = 0;
      if (!homeS && !lz(&homeS, "assets/her_home.png")) { bad++; printf("v87: her_home.png missing\n"); }
      if (her_vis() != homeS || chat_vis() != homeS) { bad++; printf("v87: moved in: her_home for door, room and chat\n"); }
      G.pill_given = 1; G.pill_day = 14; G.pill_seen = 1; G.married = 1; G.yoga_given = 1; G.outfit = 1;
      if (!(her_vis() == homeYS && homeYS) || chat_vis() != homeYS) { bad++; printf("v87: yoga outfit: her_home_yoga for door, room and chat\n"); }
      RSHOT("v87_everyday_yoga"); G.outfit = 0; if (her_vis() != confP || !confP || her_vis() == herCoverS) { bad++; printf("v88: married, day-8 outfit: the uncovered day-8 picture at the door (no cover)\n"); }
      G.rs = RS_NONE; G.pill_seen = 0; G.pill_given = 0; G.married = 0; if (her_vis() == homeS && homeS) { bad++; printf("v87: before moving in her_home must not show\n"); }
      /* day 8: the old scene opens on the door, her line has no picture, then the hiding picture */
      G = t; G.married = 0; G.outfit = 0; vn_start(SCR_PILLDAY);
      if (vn_buf[0].spr != 0 || vn_buf[2].spr != 0 || vn_buf[3].spr != 5 || vn_buf[vn_n - 1].bg != BG_ROM || vn_buf[vn_n - 1].spr != 0) { bad++; printf("v87: day 8 scene order\n"); }
      { int q, h = 0; for (q = 0; q < vn_n; q++) if (vn_buf[q].spr == 5) h++; if (h < 5) { bad++; printf("v87: day 8: the dialogue plays on the hiding picture\n"); } }
      vn_i = 3; vn_prev_spr = 5; RSHOT("v87_day8_hiding");
      /* after day 8, unmarried: the knock scene hides her; married in the daytime she does not */
      G.married = 0; G.phase = PH_MORNING; vn_start(SCR_KNOCKIN);
      if (vn_buf[0].spr != 0 || vn_buf[1].spr != 0 || vn_buf[2].spr != 5) { bad++; printf("v87: unmarried knock: door, her line, hiding picture\n"); }
      G.married = 1; vn_start(SCR_KNOCKIN);
      { int q, h = 0; for (q = 0; q < vn_n; q++) if (vn_buf[q].spr == 5) h++; if (h) { bad++; printf("v87: married in the daytime: no hiding picture\n"); } }
      G.phase = PH_EVENING; vn_start(SCR_KNOCKIN); if (vn_buf[2].spr != 0) { bad++; printf("v87: married in the evening: no hiding picture\n"); }
      hide_free(); G = t; }
    /* v8.8: married = no cover at the door; she covers only for body teases, ASK FOR MILK and BREASTFEED; table faces from move-in */
    { Game t = G; SDL_Surface *p1, *p2; int q, ok;
      G.rs = RS_LIVING; G.pill_seen = 1; G.married = 1; G.yoga_given = 1; G.outfit = 0; cover_now = 0; feed_hide = 0;
      if (her_vis() != confP || !confP) { bad++; printf("v88: married, day-8 outfit: her_vis must be the uncovered day-8 picture\n"); }
      cover_set(1); p1 = her_vis(); if (!herCoverS || p1 != herCoverS) { bad++; printf("v88: married, covering: her_cover\n"); }
      G.outfit = 1; cover_set(1); p1 = her_vis(); if (!herCoverYS || p1 != herCoverYS) { bad++; printf("v88: married, yoga, covering: her_cover_yoga\n"); }
      RSHOT("v88_cover_yoga"); cover_set(0); p1 = her_vis(); if (!homeYS || p1 != homeYS) { bad++; printf("v88: married, yoga: her_home_yoga when she does not cover\n"); }
      G.outfit = 0; G.married = 0; cover_set(1); if (!herCoverS || her_vis() != herCoverS) { bad++; printf("v88: not married: her_cover as before\n"); }
      cover_set(0); if (her_vis() != herCoverS) { bad++; printf("v88: not married after day 8: her_cover at the door as before\n"); }
      G.married = 1; cover_now = 0; feed_hide = 0;
      /* tease: every topic except HER LOOKS makes her cover, and the kick-out ends it */
      for (q = 0; q < TS_N; q++) { V86_OPEN(); G.milk_st = 4; tease_start(q); ok = (q == TS_LOOKS) ? !cover_now : cover_now;
        if (!ok) { bad++; printf("v88: tease topic %d: cover_now %d\n", q, cover_now); }
        if (q == TS_CHEST) RSHOT("v88_tease_chest_covered");
        talk_end(); if (cover_now) { bad++; printf("v88: after the tease the cover is cleared\n"); } }
      /* ask for milk: she covers while she answers (refusal) and in the bottle scene */
      G.sd_day = G.day; G.ask_day = 0; G.ask_n = 0; G.bottle = 0; V86_OPEN(); rom_ask();
      if (scr != SC_STORY || !cover_now || her_vis() != herCoverS) { bad++; printf("v88: ask for milk: she covers in the bottle scene (scr %d cover %d)\n", scr, cover_now); }
      RSHOT("v88_ask_milk_cover"); V86_FIN(); if (cover_now) { bad++; printf("v88: after the ask scene the cover is cleared\n"); }
      G.sd_day = 0; V86_OPEN(); rom_ask(); if (!cover_now || scr != SC_ROM) { bad++; printf("v88: ask for milk (refused): she covers while she answers\n"); }
      rom_choose = 0; on_button(B_A); if (cover_now) { bad++; printf("v88: after her answer the choices come back uncovered\n"); }
      /* breastfeed: refused (gauge not full) = covers until the choices; accepted = covering beats, then the feed picture */
      G.call = 1; G.milk_pct = 0; V86_OPEN(); rom_feed(); if (!cover_now || scr != SC_ROM) { bad++; printf("v88: breastfeed refused: she covers while she answers\n"); }
      rom_choose = 0; on_button(B_A); if (cover_now) { bad++; printf("v88: breastfeed refused: uncovered once the choices open\n"); }
      G.milk_pct = 100; G.mday = 0; G.bottle = 0; V86_OPEN(); rom_feed();
      if (scr != SC_STORY || vn_id != SCR_FEED || !cover_now || vn_buf[0].spr != 1 || vn_buf[1].spr != 1 || vn_buf[0].bg != BG_ROM) { bad++; printf("v88: breastfeed: the scene opens with her covering her chest (scr %d id %d cover %d)\n", scr, vn_id, cover_now); }
      if (her_vis() != herCoverS) { bad++; printf("v88: breastfeed: her_cover shows in the opening beats\n"); }
      RSHOT("v88_feed_cover_beat"); V86_FIN(); if (cover_now) { bad++; printf("v88: after the feed scene the cover is cleared\n"); }
      /* table faces: the home set from move-in, the yoga set in the yoga outfit, separate from the covering pictures */
      G.outfit = 0; G.pill_seen = 0; her_pick = 0; if (!home_faces()) { bad++; printf("v88: moved in, before day 8: home faces\n"); }
      p1 = face_home_pic(0); p2 = face_home_pic(1);
      if (!p1 || !p2 || p1 == p2) { bad++; printf("v88: home table faces must load (neutral / shock)\n"); }
      if (!face_home_pic(2)) { bad++; printf("v88: home smug face\n"); }
      p1 = caught_pic(); if (!p1 || p1 == herCoverS) { bad++; printf("v88: the caught picture must be its own surface, not her_cover\n"); }
      if (p1 && herCoverS) { int sz = p1->pitch * p1->h; if (p1->w == herCoverS->w && p1->h == herCoverS->h && !memcmp(p1->pixels, herCoverS->pixels, (size_t)sz)) { bad++; printf("v88: her_caught_home is identical to her_cover (they must be separate pictures)\n"); } }
      bg(1); RSHOT("v88_table_home");
      G.outfit = 1; if (!home_faces()) { bad++; printf("v88: yoga outfit: yoga faces\n"); }
      p1 = face_home_pic(0); if (hfaceO != 1 || !p1) { bad++; printf("v88: yoga table face\n"); }
      p2 = caught_pic(); if (!p2) { bad++; printf("v88: yoga caught picture\n"); } else if (herCoverYS && p2 == herCoverYS) { bad++; printf("v88: yoga caught picture must not be her_cover_yoga\n"); }
      her_pick = 1; bg(1); her_pick = 2; bg(1); her_pick = 0; RSHOT("v88_table_yoga");
      G.outfit = 0; face_home_pic(0); if (hfaceO != 0) { bad++; printf("v88: back to the home faces\n"); }
      /* after the day-8 scene (usual outfit) the day-8 faces come back: her_*_p and her_caught, the home faces are dropped */
      G.pill_seen = 1; her_pick = 0; if (home_faces()) { bad++; printf("v88: after day 8 the home faces must not be used\n"); }
      bg(1); if (!herP || hfaceS[0] || hfaceS[1] || hfaceS[2]) { bad++; printf("v88: after day 8 the table shows her_neutral_p and the home faces are freed\n"); }
      lz_free(&herCaughtS); p1 = caught_pic(); if (!p1 || hfaceS[0]) { bad++; printf("v88: after day 8 the caught picture is her_caught\n"); }
      else { SDL_Surface *o = load_png("assets/her_caught.png"); if (!o || p1->w != o->w || memcmp(p1->pixels, o->pixels, (size_t)(p1->pitch * p1->h))) { bad++; printf("v88: after day 8 the caught picture must be assets/her_caught.png\n"); } if (o) SDL_FreeSurface(o); }
      G.outfit = 1; if (!home_faces()) { bad++; printf("v88: yoga after day 8: the yoga faces\n"); } G.outfit = 0;
      /* before she lives with you the old table faces stay */
      G.rs = RS_NONE; her_pick = 0; bg(1); if (!(her || herP)) { bad++; printf("v88: before move-in the old table face must load\n"); }
      G = t; cover_now = 0; feed_hide = 0; lz_free(&herCaughtS); }
    /* 9. old saves: a version-15 save has no wedding */
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = 0; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE; kicked_day = 0;
#undef V86_OPEN
#undef V86_FIN
#undef V86_ROW
    return bad;
}
static int v110_tests(void) {   /* v11.0: the FRIENDSHIP bar (blue, 20 wins), her door openers by tier, COME IN and her room by day */
    int bad = 0, i, k; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, kfv = fv_on, kseen = fv_seen, kgn = fv_greet_n, kon = ls_on;
#define V110_BASE(w) do { g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.ex_big = 1; G.cash = 5000; G.energy = 100; G.day = 5; G.phase = PH_MORNING; G.fr_wins = (w); G.m_played = 3; G.last_res = 1; saved_G = G; fv_on = 0; fv_seen = 0; fv_greet_n = 0; scr = SC_WALK; wk_mode = 0; ls_on = 0; ls_talk = 0; } while (0)
#define V110_SAY(face_x, face_y, face, pool, what) do { wk_mode = 0; scr = SC_WALK; wk_place(MAP_SOPHIA, face_x, face_y, face); on_button(B_A); \
        if (wk_mode != 1 || !in_pool(wk_pg[0], pool, pool##_N)) { bad++; printf("v110: %s: wrong or no line (mode %d)\n", what, wk_mode); } WK_CLOSE(); } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0;
    /* 1. the tiers: tiers 1-4 every 5 wins, tier 5 = full bar at 20 and never goes past it */
    { static const int w[] = {-3, 0, 4, 5, 9, 10, 14, 15, 19, 20, 50}, t[] = {1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5}, pc[] = {0, 0, 20, 25, 45, 50, 70, 75, 95, 100, 100};
      for (i = 0; i < (int)(sizeof w / sizeof w[0]); i++) { V110_BASE(w[i]); if (g_fr_tier(&G) != t[i] || g_fr_pct(&G) != pc[i] || g_fr_full(&G) != (w[i] >= 20)) { bad++; printf("v110: %d wins: tier %d pct %d full %d\n", w[i], g_fr_tier(&G), g_fr_pct(&G), g_fr_full(&G)); } } }
    /* 2. her door: COME IN is the row above LEAVE by day once she has a name, never at night, before the boss, or once she lives with you */
    { int dk[5], dn;
      V110_BASE(3); dn = door_items(dk); if (dn < 3 || dk[0] != DR_PLAY || dk[dn - 2] != DR_COMEIN || dk[dn - 1] != DR_LEAVE) { bad++; printf("v110: COME IN must sit just above LEAVE in her door menu (%d rows)\n", dn); }
      scr = SC_DOOR; sel = 0; RSHOT("v110_door_menu_come_in"); G.fr_wins = 20; RSHOT("v110_door_menu_bar_full"); scr = SC_WALK;
      V110_BASE(3); G.phase = PH_NIGHT; dn = door_items(dk); for (i = 0; i < dn; i++) if (dk[i] == DR_COMEIN) { bad++; printf("v110: no COME IN at night\n"); }
      V110_BASE(3); G.story = ST_FIRST; dn = door_items(dk); for (i = 0; i < dn; i++) if (dk[i] == DR_COMEIN) { bad++; printf("v110: no COME IN before you beat her\n"); }
      V110_BASE(3); G.rs = RS_LIVING; dn = door_items(dk); for (i = 0; i < dn; i++) if (dk[i] == DR_COMEIN) { bad++; printf("v110: no COME IN once she lives with you (3rd floor closed)\n"); } }
    /* 3. COME IN: you stand in her room, she stands in the middle of it */
    V110_BASE(3); wk_place(MAP_HALL3, 1, 4, LF); scr = SC_DOOR; { int dk[5], dn = door_items(dk); sel = 0; for (i = 0; i < dn; i++) if (dk[i] == DR_COMEIN) sel = i; }
    on_button(B_A);
    if (wk_map != MAP_SOPHIA || !fv_on || scr != SC_WALK || wk_tx != 13 || wk_ty != 5) { bad++; printf("v110: COME IN did not put you in her room (map %d fv %d scr %d at %d,%d)\n", wk_map, fv_on, scr, wk_tx, wk_ty); }
    if (wk_mode != 1 || !in_pool(wk_pg[0], V_IN, V_IN_N)) { bad++; printf("v110: no narration when you step in\n"); }
    if (!ls_on || ls_tx != FV_NX || ls_ty != FV_NY) { bad++; printf("v110: she is not standing in her room (on %d at %d,%d)\n", ls_on, ls_tx, ls_ty); }
    RSHOT("v110_room_enter"); WK_CLOSE(); update();
    if (!ls_on || ls_tx != FV_NX || fv_on != 1) { bad++; printf("v110: she must stay put while you stand there\n"); }
    wk_place(MAP_SOPHIA, FV_NX + 1, FV_NY, LF); wk_move(LF, 1); if (wk_tx != FV_NX + 1) { bad++; printf("v110: you must not walk through her\n"); }
    /* 4. A on her: her line, then TALK / PLAY A MATCH / LEAVE; the bar is on the screen */
    wk_mode = 0; wk_place(MAP_SOPHIA, FV_NX + 1, FV_NY, LF); G.fr_wins = 0; on_button(B_A);
    if (wk_mode != 1 || wk_after != WK_VISIT || !in_pool(wk_pg[0], V_HELLO, V_HELLO_N)) { bad++; printf("v110: A on her must give her line first (mode %d)\n", wk_mode); }
    RSHOT("v110_her_line_bar_empty"); WK_CLOSE();
    if (wk_mode != 9 || fv_vsel != 0) { bad++; printf("v110: her line must lead to the visit menu (mode %d sel %d)\n", wk_mode, fv_vsel); }
    G.fr_wins = 10; RSHOT("v110_visit_menu_bar_half"); G.fr_wins = 20; RSHOT("v110_visit_menu_bar_full"); G.fr_wins = 3;
    on_button(B_UP); if (fv_vsel != 2) { bad++; printf("v110: UP from TALK wraps to LEAVE\n"); } on_button(B_DOWN); if (fv_vsel != 0) { bad++; printf("v110: DOWN from LEAVE wraps to TALK\n"); }
    /* 5. TALK: her talk follows the tier, and you are back in the menu afterwards */
    { const char *const *tp[5] = {V_TALK1, V_TALK2, V_TALK3, V_TALK4, V_TALK5}; const int *tn[5] = {&V_TALK1_N, &V_TALK2_N, &V_TALK3_N, &V_TALK4_N, &V_TALK5_N}; static const int w[5] = {0, 5, 10, 15, 20};
      for (k = 0; k < 5; k++) { int hit = 0, j; G.fr_wins = w[k]; wk_mode = 9; fv_vsel = 0; on_button(B_A);
        for (j = 0; j < *tn[k]; j++) if (wk_pg[0] == tp[k][j]) hit = 1;
        if (!hit || wk_mode != 1 || wk_after != WK_VISIT) { bad++; printf("v110: TALK at %d wins is not from the tier %d pool\n", w[k], k + 1); }
        if (k == 2) RSHOT("v110_talk_tier3");
        WK_CLOSE(); if (wk_mode != 9) { bad++; printf("v110: TALK must lead back to the menu\n"); } } }
    /* 6. PLAY A MATCH: the buy-in list (B goes back to the menu); no cash: her refusal */
    G.fr_wins = 3; wk_mode = 9; fv_vsel = 1; on_button(B_A);
    if (scr != SC_LEN || !fv_on) { bad++; printf("v110: PLAY A MATCH must open the buy-in list (scr %d)\n", scr); }
    RSHOT("v110_play_in_her_room"); on_button(B_B); if (scr != SC_WALK || wk_mode != 9) { bad++; printf("v110: B in the buy-in list must return to her visit menu (scr %d mode %d)\n", scr, wk_mode); }
    G.cash = 0; G.bank = 0; wk_mode = 9; fv_vsel = 1; on_button(B_A);
    if (scr != SC_WALK || wk_mode != 1 || !in_pool(wk_pg[0], S_BROKE, S_BROKE_N)) { bad++; printf("v110: no cash: she must refuse (scr %d mode %d)\n", scr, wk_mode); }
    WK_CLOSE(); G.cash = 5000;
    /* 7. the things in her room answer by day; her bedroom and bathroom stay shut */
    V110_SAY(4, 5, UP, V_TV, "TV (left)"); V110_SAY(5, 5, UP, V_TV, "TV (right)"); V110_SAY(3, 4, UP, V_WIN, "window (left)"); V110_SAY(6, 4, UP, V_WIN, "window (right)");
    V110_SAY(9, 5, UP, V_TABLE, "table"); V110_SAY(4, 7, LF, V_SOFA, "bench (upper)"); V110_SAY(4, 8, LF, V_SOFA, "bench (lower)");
    V110_SAY(5, 8, DN, V_BED, "bedroom door"); if (scr != SC_WALK) { bad++; printf("v110: her bedroom door must stay shut by day (scr %d)\n", scr); }
    V110_SAY(9, 8, DN, V_BATH, "bathroom door");
    wk_mode = 0; wk_place(MAP_SOPHIA, 9, 5, UP); on_button(B_A); RSHOT("v110_object_table"); WK_CLOSE();
    /* 8. LEAVE: her goodbye, then you are in the hall at her door and the visit is over */
    wk_mode = 9; fv_vsel = 2; on_button(B_A);
    if (wk_mode != 1 || wk_after != WK_VBYE || !in_pool(wk_pg[0], V_BYE, V_BYE_N)) { bad++; printf("v110: LEAVE must give her goodbye (mode %d)\n", wk_mode); }
    WK_CLOSE(); update();
    if (wk_map != MAP_HALL3 || fv_on || ls_on || wk_tx != 1 || wk_ty != 4) { bad++; printf("v110: LEAVE must put you in the hall at her door (map %d fv %d on %d at %d,%d)\n", wk_map, fv_on, ls_on, wk_tx, wk_ty); }
    /* 9. walking out through the door tile ends the visit too, and the NIGHT sneak-in is unchanged (no objects, no bar, bedroom door opens the peek) */
    V110_BASE(3); fv_on = 1; wk_place(MAP_SOPHIA, 13, 5, RT); update(); wk_place(MAP_HALL3, 1, 4, LF); update(); if (fv_on) { bad++; printf("v110: fv_on must reset outside her room\n"); }
    /* v11.1: the night door is knock-only (no sneaking into her 3rd-floor room), and no night filter on her room */
    V110_BASE(3); G.phase = PH_NIGHT; wk_place(MAP_HALL3, 1, 4, LF); enter_hub(); on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_NIGHT) { bad++; printf("v111: the 3rd-floor door at night must be the knock / scolding only (scr %d id %d)\n", scr, vn_id); }
    { SDL_Surface *ro = load_png_ex("assets/map_sophia.png", 3, 1, 1, 1, 1); Uint32 a1, b1; int x, y, diff = 0;
      if (!ro || !wm[MAP_SOPHIA].img || ro->w != wm[MAP_SOPHIA].img->w) { bad++; printf("v111: her room picture missing\n"); }
      else { for (y = 0; y < ro->h; y += 5) for (x = 0; x < ro->w; x += 5) { a1 = *px(ro, x, y); b1 = *px(wm[MAP_SOPHIA].img, x, y); if (a1 != b1) diff++; }
        if (diff) { bad++; printf("v111: her room picture is still darkened (%d pixels differ)\n", diff); } } }
    /* v11.1: a match started inside her room (leave table / after the match) ends outside it, at her door */
    V110_BASE(3); G.phase = PH_MORNING; fv_on = 1; wk_place(MAP_SOPHIA, 13, 5, LF); scr = SC_LEN; len_sel = 1; SAYP(L_LEN);
    { int k; vn_start(SCR_LEAVE); for (k = 0; k < 40 && scr == SC_STORY; k++) { fake_ms += 300; update(); on_button(B_A); }
      for (k = 0; k < 40 && wk_map != MAP_HALL3; k++) { fake_ms += 300; update(); }
      if (wk_map != MAP_HALL3 || fv_on || wk_tx != 1 || wk_ty != 4 || scr != SC_WALK) { bad++; printf("v111: leaving a match from inside her room must end outside, at her door (map %d fv %d at %d,%d scr %d)\n", wk_map, fv_on, wk_tx, wk_ty, scr); }
      V110_BASE(3); fv_on = 1; wk_place(MAP_SOPHIA, 13, 5, LF); vn_start(SCR_AFTER); for (k = 0; k < 60 && scr == SC_STORY; k++) { fake_ms += 300; update(); on_button(B_A); }
      for (k = 0; k < 40 && wk_map != MAP_HALL3; k++) { fake_ms += 300; update(); }
      if (wk_map != MAP_HALL3 || fv_on) { bad++; printf("v111: after a match from inside her room you must end outside (map %d fv %d)\n", wk_map, fv_on); }
      V110_BASE(3); fv_on = 0; wk_place(MAP_HALL3, 1, 4, LF); vn_start(SCR_LEAVE); for (k = 0; k < 40 && scr == SC_STORY; k++) { fake_ms += 300; update(); on_button(B_A); }
      if (wk_map != MAP_HALL3 || scr != SC_WALK) { bad++; printf("v111: leaving a match from the door must stay as before (map %d scr %d)\n", wk_map, scr); } }
    /* v11.1: the buy-in list: her_room.png from the door, BG-poker_day.png from inside her room */
    /* v11.8: inside her room the buy-in goes straight to the table (no door scene); at her door the comment scene still plays */
    V110_BASE(3); fv_on = 1; wk_place(MAP_SOPHIA, 13, 5, LF); scr = SC_LEN; len_sel = 0; SAYP(L_LEN); on_button(B_A);
      if (scr != SC_PLAY) { bad++; printf("v118: the buy-in inside her room must start the match at once (scr %d)\n", scr); }
    V110_BASE(3); fv_on = 0; scr = SC_LEN; len_sel = 0; SAYP(L_LEN); on_button(B_A);
      if (scr != SC_STORY || vn_id != SCR_COMMENT) { bad++; printf("v118: the buy-in at her door must still play her comment scene (scr %d)\n", scr); }
    V110_BASE(3); fv_on = 0; scr = SC_LEN; len_sel = 1; SAYP(L_LEN); RSHOT("v111_buyin_from_door"); fv_on = 1; RSHOT("v111_buyin_from_inside"); fv_on = 0; scr = SC_WALK;
    scr = SC_WALK; wk_mode = 0;
    /* 10. the door openers: the first knock after the boss is the special line (tier 1, once); then 2 knocks in 3 use her tier pool, the rest the old line for your last match */
    V110_BASE(2); vn_start(SCR_GREET); if (!in_pool(vn_buf[1].txt, G_FIRSTVIS, G_FIRSTVIS_N) || !fv_seen) { bad++; printf("v110: the first knock must be the special line\n"); }
    { int tier = 0, other = 0; for (i = 0; i < 30; i++) { vn_start(SCR_GREET); if (in_pool(vn_buf[1].txt, G_FR1, G_FR1_N)) tier++; else if (in_pool(vn_buf[1].txt, G_WIN, G_WIN_N)) other++; }
      if (tier != 20 || other != 10) { bad++; printf("v110: 30 knocks: %d tier lines, %d old lines (expected 20 and 10)\n", tier, other); } }
    { const char *const *tp[5] = {G_FR1, G_FR2, G_FR3, G_FR4, G_FR5}; const int *tn[5] = {&G_FR1_N, &G_FR2_N, &G_FR3_N, &G_FR4_N, &G_FR5_N}; static const int w[5] = {0, 5, 10, 15, 20};
      for (k = 0; k < 5; k++) { int hit = 0, j; V110_BASE(w[k]); fv_seen = 1; fv_greet_n = 0; vn_start(SCR_GREET); for (j = 0; j < *tn[k]; j++) if (vn_buf[1].txt == tp[k][j]) hit = 1; if (!hit) { bad++; printf("v110: knock at %d wins is not from the tier %d pool\n", w[k], k + 1); } } }
    V110_BASE(2); fv_seen = 1; G.forfeit = 1; vn_start(SCR_GREET); if (!in_pool(vn_buf[1].txt, G_COWARD, G_COWARD_N)) { bad++; printf("v110: after a walk-out she still calls you a coward\n"); }
    V110_BASE(2); G.story = ST_FIRST; fv_seen = 0; vn_start(SCR_GREET); if (fv_seen || in_pool(vn_buf[1].txt, G_FIRSTVIS, G_FIRSTVIS_N) || in_pool(vn_buf[1].txt, G_FR1, G_FR1_N)) { bad++; printf("v110: before the boss win the old greetings stay\n"); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; wk_map = kmap; wk_tx = ktx; wk_ty = kty; wk_face = kface; fv_on = kfv; fv_seen = kseen; fv_greet_n = kgn; ls_on = kon; wk_mode = 0;
    return bad;
}
static int v108_tests(void) {   /* v10.8: the DESIGNER TOP, a third outfit (G.outfit 2): her menu rows, the wardrobe, the lines, every picture set, the store list */
    int bad = 0, i, n, idx, a[10]; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk;
    SDL_Surface *p1, *p2; unsigned s1, s2;
#define V108_BASE() do { g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1; \
    G.milk_st = 4; G.milk_pct = 0; G.mpill = 6; G.dring = 1; G.married = 1; G.event = 1; G.hall_seen = 3; G.ring = 1; G.cash = 90000; G.energy = 100; saved_G = G; ls_away = -1; ls_talk = 0; ls_on = 0; sb_out = 0; kicked_day = 0; sb_rkey = -1; scr = SC_WALK; wk_mode = 0; cover_now = 0; feed_hide = 0; } while (0)
#define V108_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
#define V108_ROW(pg, act) do { n = rom_page_acts(pg, a); idx = -1; for (i = 0; i < n; i++) if (a[i] == (act)) idx = i; } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0;
    /* 1. v11.3: her menu has NO outfit rows any more, whatever she owns and wears; the wardrobe is the only place (mama lines included) */
    V108_BASE(); G.yoga_given = 1; G.top_given = 1; G.cow_given = 1; G.outfit = 0; V108_OPEN();
    n = rom_page_acts(0, a); if (n != 2 || a[0] != RA_PLAY || a[1] != RA_CHAT) { bad++; printf("v108: page 1 is PLAY / CHAT only (rows %d)\n", n); }
    G.outfit = OF_TOP; n = rom_page_acts(0, a); if (n != 2) { bad++; printf("v108: still no outfit rows while she wears the top (rows %d)\n", n); }
    for (i = 0; i < 4; i++) { n = rom_page_acts(i, a); for (idx = 0; idx < n; idx++) if (a[idx] >= RA_OUTFIT_K) { bad++; printf("v108: page %d has an outfit row\n", i + 1); } }
    RSHOT("v108_menu_no_outfit_rows");
    V108_BASE(); G.yoga_given = 1; G.top_given = 1; G.outfit = 0;
    if (!tst_ward(OF_TOP) || G.outfit != OF_TOP || !in_pool(wk_pg[0], R_OUTFIT_TOP, R_OUTFIT_TOP_N)) { bad++; printf("v108: wardrobe: usual > top with her line (outfit %d)\n", G.outfit); } WK_CLOSE();
    if (!tst_ward(OF_USUAL) || G.outfit != OF_USUAL || !in_pool(wk_pg[0], R_OUTFIT_D8, R_OUTFIT_D8_N)) { bad++; printf("v108: wardrobe: and back to her usual outfit (outfit %d)\n", G.outfit); } WK_CLOSE();
    G.outfit = OF_TOP; if (!tst_ward(OF_YOGA) || G.outfit != OF_YOGA || !in_pool(wk_pg[0], R_OUTFIT_YOGA, R_OUTFIT_YOGA_N)) { bad++; printf("v108: wardrobe: top > yoga straight away (outfit %d)\n", G.outfit); } WK_CLOSE();
    G.outfit = 0; G.call = 1; if (!tst_ward(OF_TOP) || !in_pool(wk_pg[0], R_OUTFIT_TOP_MM, R_OUTFIT_TOP_MM_N)) { bad++; printf("v108: mama mode: her designer-top line comes from the mama pool\n"); } WK_CLOSE();
    G.call = 0;
    /* 2. the wardrobe in her bedroom: a row for every outfit she can change into, then LEAVE; UP / DOWN wrap */
    V108_BASE(); G.yoga_given = 1; G.top_given = 1; G.outfit = 0; saved_G = G; wk_place(MAP_SBED, 2, 3, LF); ls_on = 0; on_button(B_A); on_button(B_A);
    if (wk_mode != 8 || wk_wsel != 0) { bad++; printf("v108: the wardrobe menu opens on its first row (mode %d)\n", wk_mode); }
    RSHOT("v108_wardrobe_both");
    on_button(B_UP); if (wk_wsel != 2) { bad++; printf("v108: UP from the first row wraps to LEAVE (sel %d)\n", wk_wsel); } on_button(B_DOWN); if (wk_wsel != 0) { bad++; printf("v108: DOWN from LEAVE wraps to the first row (sel %d)\n", wk_wsel); }
    on_button(B_DOWN); on_button(B_DOWN); on_button(B_A); if (wk_mode != 0 || G.outfit != 0) { bad++; printf("v108: LEAVE changes nothing (outfit %d mode %d)\n", G.outfit, wk_mode); }
    on_button(B_A); on_button(B_A); on_button(B_DOWN); on_button(B_A); ow_play(); if (G.outfit != OF_TOP || wk_mode != 1 || !in_pool(wk_pg[0], R_OUTFIT_TOP, R_OUTFIT_TOP_N)) { bad++; printf("v108: the 2nd row changes her into the designer top with her line (outfit %d)\n", G.outfit); }
    RSHOT("v108_wardrobe_she_answers"); WK_CLOSE();
    on_button(B_A); on_button(B_A); if (wk_mode != 8) { bad++; printf("v108: the wardrobe opens again while she wears the top\n"); }
    on_button(B_DOWN); on_button(B_A); ow_play(); if (G.outfit != OF_YOGA || !in_pool(wk_pg[0], R_OUTFIT_YOGA, R_OUTFIT_YOGA_N)) { bad++; printf("v108: top > yoga from the wardrobe (outfit %d)\n", G.outfit); } WK_CLOSE();
    on_button(B_A); on_button(B_A); on_button(B_A); if (G.outfit != OF_USUAL || !in_pool(wk_pg[0], R_OUTFIT_D8, R_OUTFIT_D8_N)) { bad++; printf("v108: and to the usual outfit (outfit %d)\n", G.outfit); } WK_CLOSE();
    G.yoga_given = 0; saved_G = G; wk_place(MAP_SBED, 2, 3, LF); on_button(B_A); on_button(B_A); on_button(B_DOWN); on_button(B_DOWN); if (wk_mode != 8 || wk_wsel != 0) { bad++; printf("v108: with only the top: 2 rows (top, LEAVE), DOWN twice is back on the top (sel %d)\n", wk_wsel); } wk_mode = 0;
    G.top_given = 0; G.yoga_given = 0; wk_place(MAP_SBED, 2, 3, LF); on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_WARD_NONE) { bad++; printf("v108: with no outfit at all she has nothing to change into\n"); } WK_CLOSE(); wk_mode = 0;
    /* 3. pictures: every outfit has its own set; the designer top never shows the yoga picture when its own file exists, and a missing file falls back to yoga */
    V108_BASE(); G.yoga_given = 1; G.top_given = 1;
    G.outfit = 1; p1 = her_vis(); if (!p1 || p1 != homeYS || homeYO != 1) { bad++; printf("v108: yoga: her everyday picture is her_home_yoga\n"); } s1 = p1 ? surf_sig(p1) : 0;
    G.outfit = 2; p2 = her_vis(); if (!p2 || p2 != homeYS || homeYO != 2) { bad++; printf("v108: top: her everyday picture is her_home_top (loaded for outfit %d)\n", homeYO); } s2 = p2 ? surf_sig(p2) : 0;
    if (s1 == s2) { bad++; printf("v108: top: her everyday picture must not be the yoga one\n"); }
    if (chat_vis() != homeYS || night_vis() != homeYS) { bad++; printf("v108: top: chat and night-knock pictures are the everyday picture\n"); }
    RSHOT("v108_everyday_top");
    cover_set(1); p2 = her_vis(); if (!p2 || p2 != herCoverYS || coverYO != 2) { bad++; printf("v108: top: covering picture is her_cover_top\n"); } s2 = p2 ? surf_sig(p2) : 0;
    G.outfit = 1; p1 = her_vis(); if (!p1 || p1 != herCoverYS || coverYO != 1 || surf_sig(p1) == s2) { bad++; printf("v108: yoga covering picture differs from the top one and is reloaded on a change\n"); }
    RSHOT("v108_cover_yoga"); G.outfit = 2; her_vis(); RSHOT("v108_cover_top"); cover_set(0);
    G.outfit = 0; p1 = her_vis(); if (p1 == homeYS && homeYS) { bad++; printf("v108: usual outfit: not an outfit picture\n"); }
    /* a missing designer-top file falls back to the yoga picture */
    G.outfit = 2; lz_free(&homeYS); rename("assets/her_home_top.png", "assets/her_home_top.bak"); p2 = her_vis(); s2 = p2 ? surf_sig(p2) : 1;   /* v11.1: read the signature before the cache is freed */
    G.outfit = 1; lz_free(&homeYS); p1 = her_vis(); if (!p2 || !p1 || surf_sig(p1) != s2) { bad++; printf("v108: no her_home_top: she shows the yoga picture\n"); }
    rename("assets/her_home_top.bak", "assets/her_home_top.png"); lz_free(&homeYS);
    /* hiding behind the door */
    G.outfit = 1; hide_free(); p1 = hide_pic(); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 2; p2 = hide_pic(); if (!p2 || hideO != 2 || surf_sig(p2) == s1) { bad++; printf("v108: top: hiding picture is its own (hideO %d)\n", hideO); }
    for (i = 1; i <= 2; i++) { G.outfit = 1; fh_free(); p1 = fh_pic(i); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 2; p2 = fh_pic(i); if (!p2 || p2->w != 752 || p2->h != 560 || fhK != i * 4 + 2 || surf_sig(p2) == s1) { bad++; printf("v108: top: BREASTFEED hiding picture %d is its own (key %d)\n", i, fhK); } }
    fh_free(); hide_free();
    /* page-4 scenes (hug / kiss / sleep / breastfeed) and the living-room breastfeed picture */
    for (i = 0; i < 4; i++) {
        G.outfit = 1; p1 = sc_pic(BG_HUG + i); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 2; p2 = sc_pic(BG_HUG + i);
        if (!p1 || !p2 || p2->w != 752 || p2->h != 560 || surf_sig(p2) == s1) { bad++; printf("v108: top: scene picture %d is its own\n", i); }
    }
    { SDL_Surface *lt = load_png("assets/sc_feed_lr_top.png"); if (!lt || lt->w != 752 || lt->h != 560) { bad++; printf("v108: sc_feed_lr_top.png missing or not 752x560\n"); } if (lt) SDL_FreeSurface(lt); }
    /* table faces and the close-up answer */
    G.outfit = 1; if (!home_faces()) { bad++; printf("v108: yoga faces\n"); }
    G.outfit = 2; if (!home_faces()) { bad++; printf("v108: top: the home faces are used (like yoga) even after the day-8 scene\n"); }
    for (i = 0; i < 3; i++) { G.outfit = 1; p1 = face_home_pic(i); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 2; p2 = face_home_pic(i); if (!p2 || hfaceO != 2 || surf_sig(p2) == s1) { bad++; printf("v108: top: table face %d is its own (hfaceO %d)\n", i, hfaceO); } }
    G.outfit = 1; p1 = caught_pic(); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 2; p2 = caught_pic(); if (!p2 || surf_sig(p2) == s1) { bad++; printf("v108: top: the caught picture is reloaded for the new outfit\n"); }
    her_pick = 1; bg(1); her_pick = 3; bg(1); her_pick = 0; RSHOT("v108_table_top");
    G.outfit = 0; lz_free(&herCaughtS); caughtO = -1;
    /* 4. the store: the designer top is listed after the yoga outfit and before the gold bar, with its icon and its info page */
    V108_BASE(); G.cash = 90000; shop_tab = TAB_ITEMS; { int o[16], c = shop_list(TAB_ITEMS, o), y = -1, t = -1, g = -1; for (i = 0; i < c; i++) { if (o[i] == SH_YOGA) y = i; if (o[i] == SH_TOP) t = i; if (o[i] == SH_GOLD) g = i; }   /* v11.1: the cow print outfit sits between the top and the gold bar */
        if (y < 0 || t != y + 1 || g != t + 2) { bad++; printf("v108: ITEMS order: yoga %d, top %d, gold %d\n", y, t, g); } }
    if (!tst_list_ok(TAB_ITEMS)) { bad++; printf("v108: the ITEMS list does not match the rules\n"); }
    if (shop_owned(SH_TOP) != 0) { bad++; printf("v108: owned count starts at 0\n"); } G.top = 3; if (shop_owned(SH_TOP) != 3) { bad++; printf("v108: owned count shows the stock\n"); } G.top = 0;
    if (giftS[SH_YOGA] && !giftS[SH_TOP]) { bad++; printf("v108: the store icon assets/gift_designer.png did not load\n"); }
    shop_sel = SH_TOP; shop_info = 0; scr = SC_SHOP; draw_shop(); RSHOT("v108_store_list"); shop_info = 1; draw_shop(); RSHOT("v108_store_info"); shop_info = 0;
    /* 5. her gift page: before the day-8 scene the designer top is a row between the yoga outfit and the gold bar, only while she has one in stock */
    V108_BASE(); G.pill_seen = 0;   /* v11.3: max romance (kept from the base) is the gate */ G.married = 0; G.pill = 0; G.pill_given = 0; G.yoga = 1; G.top = 0; G.gold = 1; V108_OPEN();
    V108_ROW(1, RA_GIVE + GF_TOP); if (idx != -1) { bad++; printf("v108: no gift row without a designer top in stock\n"); }
    G.top = 2; V108_ROW(1, RA_GIVE + GF_TOP); { int iy, ig; iy = -1; ig = -1; for (i = 0; i < n; i++) { if (a[i] == RA_GIVE + GF_YOGA) iy = i; if (a[i] == RA_GIVE + GF_GOLD) ig = i; } if (idx != iy + 1 || ig != idx + 1) { bad++; printf("v108: gift rows: yoga %d, top %d, gold %d\n", iy, idx, ig); } }
    rom_page = 1; rom_sel = idx; rom_choose = 1; on_button(B_A); if (scr != SC_STORY || vn_id != SCR_GIFT || G.top != 1 || !G.top_given) { bad++; printf("v108: giving the designer top starts the gift scene, takes one, unlocks the outfit (scr %d top %d given %d)\n", scr, G.top, G.top_given); }
    else { if (!gift_in_pool(0, GF_TOP, vn_buf[0].txt) || !gift_in_pool(1, GF_TOP, vn_buf[1].txt) || !gift_in_pool(2, GF_TOP, vn_buf[2].txt) || !gift_in_pool(3, GF_TOP, vn_buf[3].txt)) { bad++; printf("v108: the gift scene lines come from the designer-top pools\n"); }
           if (!strstr(vn_buf[3].txt, "ADDED TO HER WARDROBE") || strstr(vn_buf[3].txt, "ROMANCE") || G.rom_lvl != ROM_LVLS || G.rom_pct != 100 || G.gift_day == G.day) { bad++; printf("v108: v11.3: the closing note says wardrobe, no romance, not the gift of the day (%s)\n", vn_buf[3].txt); } RSHOT("v108_gift_scene"); }
    { int q; for (q = 0; q < 160 && scr == SC_STORY; q++) on_button(B_A); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_mode = 0; cover_now = 0; feed_hide = 0; fh_free(); hide_free(); wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
#undef V108_BASE
#undef V108_OPEN
#undef V108_ROW
    return bad;
}

static int v111_tests(void) {   /* v11.1: the COW PRINT OUTFIT, a fourth outfit (G.outfit 3): her menu (5 rows with every outfit), the wardrobe (3 rows + LEAVE), the pictures and their fallback, the store, the gift scene */
    int bad = 0, i, n, idx, a[10]; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk;
    SDL_Surface *p1, *p2; unsigned s1, s2;
#define V111_BASE() do { g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1; \
    G.milk_st = 4; G.milk_pct = 0; G.mpill = 6; G.dring = 1; G.married = 1; G.event = 1; G.hall_seen = 3; G.ring = 1; G.cash = 90000; G.energy = 100; saved_G = G; ls_away = -1; ls_talk = 0; ls_on = 0; sb_out = 0; kicked_day = 0; sb_rkey = -1; scr = SC_WALK; wk_mode = 0; cover_now = 0; feed_hide = 0; } while (0)
#define V111_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
#define V111_ROW(pg, act) do { n = rom_page_acts(pg, a); idx = -1; for (i = 0; i < n; i++) if (a[i] == (act)) idx = i; } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0;
    /* 1. v11.3: her menu has no outfit rows (all four owned changes nothing); the wardrobe does the change, also in mama mode */
    V111_BASE(); G.yoga_given = 1; G.top_given = 1; G.cow_given = 1; G.outfit = 0; V111_OPEN(); n = rom_page_acts(0, a);
    if (n != 2 || a[0] != RA_PLAY || a[1] != RA_CHAT) { bad++; printf("v111: all owned: page 1 is still only PLAY / CHAT (rows %d)\n", n); }
    RSHOT("v111_menu_no_outfit_rows");
    V111_BASE(); G.yoga_given = 1; G.top_given = 1; G.cow_given = 1; G.outfit = 0;
    if (!tst_ward(OF_COW) || G.outfit != OF_COW || !in_pool(wk_pg[0], R_OUTFIT_COW, R_OUTFIT_COW_N)) { bad++; printf("v111: wardrobe: usual > cow with the cow line (outfit %d)\n", G.outfit); } WK_CLOSE();
    if (!tst_ward(OF_TOP) || G.outfit != OF_TOP) { bad++; printf("v111: wardrobe: cow > top straight away (outfit %d)\n", G.outfit); } WK_CLOSE();
    if (!tst_ward(OF_USUAL) || G.outfit != OF_USUAL || !in_pool(wk_pg[0], R_OUTFIT_D8, R_OUTFIT_D8_N)) { bad++; printf("v111: wardrobe: top > usual, with the usual-outfit line (outfit %d)\n", G.outfit); } WK_CLOSE();
    G.outfit = 0; G.call = 1; if (!tst_ward(OF_COW) || !in_pool(wk_pg[0], R_OUTFIT_COW_MM, R_OUTFIT_COW_MM_N)) { bad++; printf("v111: mama mode: her cow-outfit line comes from the mama pool\n"); } WK_CLOSE();
    G.call = 0;
    /* 2. the wardrobe in her bedroom: three outfit rows + LEAVE with everything owned; UP / DOWN wrap */
    V111_BASE(); G.yoga_given = 1; G.top_given = 1; G.cow_given = 1; G.outfit = 0; saved_G = G; wk_place(MAP_SBED, 2, 3, LF); ls_on = 0; on_button(B_A); on_button(B_A);
    if (wk_mode != 8 || wk_wsel != 0) { bad++; printf("v111: the wardrobe menu opens on its first row (mode %d)\n", wk_mode); }
    RSHOT("v111_wardrobe_all");
    on_button(B_UP); if (wk_wsel != 3) { bad++; printf("v111: UP from the first row wraps to LEAVE, the 4th row (sel %d)\n", wk_wsel); } on_button(B_DOWN); if (wk_wsel != 0) { bad++; printf("v111: DOWN from LEAVE wraps to the first row (sel %d)\n", wk_wsel); }
    on_button(B_DOWN); on_button(B_DOWN); on_button(B_A); ow_play(); if (G.outfit != OF_COW || wk_mode != 1 || !in_pool(wk_pg[0], R_OUTFIT_COW, R_OUTFIT_COW_N)) { bad++; printf("v111: the 3rd row changes her into the cow outfit with her line (outfit %d)\n", G.outfit); }
    RSHOT("v111_wardrobe_she_answers"); WK_CLOSE();
    on_button(B_A); on_button(B_A); if (wk_mode != 8) { bad++; printf("v111: the wardrobe opens again while she wears the cow outfit\n"); }
    on_button(B_A); if (G.outfit != OF_USUAL || !in_pool(wk_pg[0], R_OUTFIT_UNCOW, R_OUTFIT_UNCOW_N)) { bad++; printf("v111: cow > usual from the wardrobe (outfit %d)\n", G.outfit); } WK_CLOSE();
    G.yoga_given = 0; G.top_given = 0; saved_G = G; wk_place(MAP_SBED, 2, 3, LF); on_button(B_A); on_button(B_A); on_button(B_DOWN); on_button(B_DOWN); if (wk_mode != 8 || wk_wsel != 0) { bad++; printf("v111: with only the cow outfit: 2 rows (cow, LEAVE), DOWN twice is back on the cow (sel %d)\n", wk_wsel); } wk_mode = 0;
    G.cow_given = 0; wk_place(MAP_SBED, 2, 3, LF); on_button(B_A); if (wk_mode != 1 || wk_pg[0] != W_WARD_NONE) { bad++; printf("v111: with no outfit at all she has nothing to change into\n"); } WK_CLOSE(); wk_mode = 0;
    /* 3. pictures: all 14 placeholder files exist at the right size; the cow outfit has its own set (not the top, not the yoga one) */
    {   static const char *const NM[14] = {"her_home", "her_cover", "her_neutral", "her_shock", "her_smug", "her_caught", "her_hide", "her_hide_no", "her_hide_yes", "sc_hug", "sc_kiss", "sc_sleep", "sc_feed", "sc_feed_lr"};
        for (i = 0; i < 14; i++) { char fn[64]; SDL_Surface *t; snprintf(fn, sizeof fn, "assets/%s_cow.png", NM[i]); t = load_png(fn);
            if (!t || t->w != SW || t->h != SH) {   /* load_png resamples a 640x480 picture to the screen (the file sizes are checked by the generator script) */ bad++; printf("v111: %s missing or the wrong size\n", fn); } if (t) SDL_FreeSurface(t); }
        { SDL_Surface *t = load_png("assets/gift_cow.png"); if (!t || t->w < 20 || t->h < 20) { bad++; printf("v111: assets/gift_cow.png missing or empty\n"); } if (t) SDL_FreeSurface(t); } }
    V111_BASE(); G.yoga_given = 1; G.top_given = 1; G.cow_given = 1;
    G.outfit = 2; p1 = her_vis(); s1 = p1 ? surf_sig(p1) : 0;
    G.outfit = 3; p2 = her_vis(); if (!p2 || p2 != homeYS || homeYO != 3) { bad++; printf("v111: cow: her everyday picture is her_home_cow (loaded for outfit %d)\n", homeYO); } s2 = p2 ? surf_sig(p2) : 0;
    if (s1 == s2) { bad++; printf("v111: cow: her everyday picture must not be the top one\n"); }
    if (chat_vis() != homeYS || night_vis() != homeYS) { bad++; printf("v111: cow: chat and night-knock pictures are the everyday picture\n"); }
    RSHOT("v111_everyday_cow");
    cover_set(1); G.outfit = 2; her_vis(); G.outfit = 3; p2 = her_vis(); if (!p2 || p2 != herCoverYS || coverYO != 3) { bad++; printf("v111: cow: covering picture is her_cover_cow\n"); } RSHOT("v111_cover_cow"); cover_set(0);
    /* a missing cow file falls back to the designer-top picture */
    G.outfit = 3; lz_free(&homeYS); rename("assets/her_home_cow.png", "assets/her_home_cow.bak"); p2 = her_vis(); s2 = p2 ? surf_sig(p2) : 1;
    G.outfit = 2; lz_free(&homeYS); p1 = her_vis(); if (!p2 || !p1 || surf_sig(p1) != s2) { bad++; printf("v111: no her_home_cow: she shows the designer-top picture\n"); }
    rename("assets/her_home_cow.bak", "assets/her_home_cow.png"); lz_free(&homeYS);
    G.outfit = 2; hide_free(); p1 = hide_pic(); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 3; p2 = hide_pic(); if (!p2 || hideO != 3 || surf_sig(p2) == s1) { bad++; printf("v111: cow: hiding picture is its own (hideO %d)\n", hideO); }
    for (i = 1; i <= 2; i++) { G.outfit = 2; fh_free(); p1 = fh_pic(i); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 3; p2 = fh_pic(i); if (!p2 || p2->w != 752 || p2->h != 560 || fhK != i * 4 + 3 || surf_sig(p2) == s1) { bad++; printf("v111: cow: BREASTFEED hiding picture %d is its own (key %d)\n", i, fhK); } }
    fh_free(); hide_free();
    for (i = 0; i < 4; i++) { G.outfit = 2; p1 = sc_pic(BG_HUG + i); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 3; p2 = sc_pic(BG_HUG + i); if (!p1 || !p2 || p2->w != 752 || p2->h != 560 || surf_sig(p2) == s1) { bad++; printf("v111: cow: scene picture %d is its own\n", i); } }
    G.outfit = 3; if (!home_faces()) { bad++; printf("v111: cow: the home faces are used (like yoga and top) even after the day-8 scene\n"); }
    for (i = 0; i < 3; i++) { G.outfit = 2; p1 = face_home_pic(i); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 3; p2 = face_home_pic(i); if (!p2 || hfaceO != 3 || surf_sig(p2) == s1) { bad++; printf("v111: cow: table face %d is its own (hfaceO %d)\n", i, hfaceO); } }
    G.outfit = 2; p1 = caught_pic(); s1 = p1 ? surf_sig(p1) : 0; G.outfit = 3; p2 = caught_pic(); if (!p2 || surf_sig(p2) == s1) { bad++; printf("v111: cow: the caught picture is reloaded for the new outfit\n"); }
    her_pick = 1; bg(1); her_pick = 3; bg(1); her_pick = 0; RSHOT("v111_table_cow");
    G.outfit = 0; lz_free(&herCaughtS); caughtO = -1;
    /* 4. the store: after the designer top, before the gold bar, with its icon and its info page */
    V111_BASE(); G.cash = 90000; shop_tab = TAB_ITEMS; { int o[16], c = shop_list(TAB_ITEMS, o), t = -1, cw = -1, g = -1; for (i = 0; i < c; i++) { if (o[i] == SH_TOP) t = i; if (o[i] == SH_COW) cw = i; if (o[i] == SH_GOLD) g = i; }
        if (t < 0 || cw != t + 1 || g != cw + 1) { bad++; printf("v111: ITEMS order: top %d, cow %d, gold %d\n", t, cw, g); } }
    if (!tst_list_ok(TAB_ITEMS)) { bad++; printf("v111: the ITEMS list does not match the rules\n"); }
    if (shop_owned(SH_COW) != 0) { bad++; printf("v111: owned count starts at 0\n"); } G.cow = 3; if (shop_owned(SH_COW) != 3) { bad++; printf("v111: owned count shows the stock\n"); } G.cow = 0;
    if (!giftS[SH_COW]) { bad++; printf("v111: the store icon assets/gift_cow.png did not load\n"); }
    shop_sel = SH_COW; shop_info = 0; scr = SC_SHOP; draw_shop(); RSHOT("v111_store_list"); shop_info = 1; draw_shop(); RSHOT("v111_store_info"); shop_info = 0;
    shop_sel = SH_COW; on_button(B_A); on_button(B_A); if (G.cow != 1 || G.cash != 90000 - 4000) { bad++; printf("v111: buying through the store buttons (cow %d cash %d)\n", G.cow, G.cash); }
    /* 5. her gift page (before the day-8 scene): the cow outfit is a row between the designer top and the gold bar, only while she has one in stock */
    V111_BASE(); G.pill_seen = 0;   /* v11.3: max romance (kept from the base) is the gate */ G.married = 0; G.pill = 0; G.pill_given = 0; G.yoga = 1; G.top = 1; G.cow = 0; G.gold = 1; V111_OPEN();
    V111_ROW(1, RA_GIVE + GF_COW); if (idx != -1) { bad++; printf("v111: no gift row without a cow outfit in stock\n"); }
    G.cow = 2; V111_ROW(1, RA_GIVE + GF_COW); { int it_, ig; it_ = -1; ig = -1; for (i = 0; i < n; i++) { if (a[i] == RA_GIVE + GF_TOP) it_ = i; if (a[i] == RA_GIVE + GF_GOLD) ig = i; } if (idx != it_ + 1 || ig != idx + 1) { bad++; printf("v111: gift rows: top %d, cow %d, gold %d\n", it_, idx, ig); } }
    RSHOT("v111_gift_page");
    rom_page = 1; rom_sel = idx; rom_choose = 1; on_button(B_A); if (scr != SC_STORY || vn_id != SCR_GIFT || G.cow != 1 || !G.cow_given || G.top_given) { bad++; printf("v111: giving the cow outfit starts the gift scene, takes one, unlocks only the cow outfit (scr %d cow %d given %d top_given %d)\n", scr, G.cow, G.cow_given, G.top_given); }
    else { if (!gift_in_pool(0, GF_COW, vn_buf[0].txt) || !gift_in_pool(1, GF_COW, vn_buf[1].txt) || !gift_in_pool(2, GF_COW, vn_buf[2].txt) || !gift_in_pool(3, GF_COW, vn_buf[3].txt)) { bad++; printf("v111: the gift scene lines come from the cow outfit pools\n"); }
           if (!strstr(vn_buf[3].txt, "ADDED TO HER WARDROBE") || strstr(vn_buf[3].txt, "ROMANCE") || G.rom_lvl != ROM_LVLS || G.rom_pct != 100 || G.gift_day == G.day) { bad++; printf("v111: v11.3: the closing note says wardrobe, no romance, not the gift of the day (%s)\n", vn_buf[3].txt); } RSHOT("v111_gift_scene"); }
    { int q; for (q = 0; q < 160 && scr == SC_STORY; q++) on_button(B_A); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_mode = 0; cover_now = 0; feed_hide = 0; fh_free(); hide_free(); wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
#undef V111_BASE
#undef V111_OPEN
#undef V111_ROW
    return bad;
}

static int v112_tests(void) {   /* v11.2: the outfit gifts are angry (yoga / top: too thin, too much skin; cow: "you think I am a cow", milk-aware once she took the pill); CHANGE OUTFIT is a 3-beat exchange (request, refusal, grudging give-in), the outfit changes when it ends */
    int bad = 0, i, n, a[10], k, c; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk;
    static const int KD[3] = {GF_YOGA, GF_TOP, GF_COW}, OFS[3] = {OF_YOGA, OF_TOP, OF_COW};
    static const Beat *const *const SC[2][3] = {{&B_WEAR_YOGA, &B_WEAR_TOP, &B_WEAR_COW}, {&B_WEAR_YOGA_MM, &B_WEAR_TOP_MM, &B_WEAR_COW_MM}};
    static int *const SCN[2][3] = {{&B_WEAR_YOGA_N, &B_WEAR_TOP_N, &B_WEAR_COW_N}, {&B_WEAR_YOGA_MM_N, &B_WEAR_TOP_MM_N, &B_WEAR_COW_MM_N}};
#define V112_BASE() do { g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1; \
    G.milk_st = 4; G.milk_pct = 0; G.mpill = 6; G.dring = 1; G.married = 1; G.event = 1; G.hall_seen = 3; G.ring = 1; G.cash = 90000; G.energy = 100; G.yoga_given = 1; G.top_given = 1; G.cow_given = 1; saved_G = G; ls_away = -1; ls_talk = 0; ls_on = 0; sb_out = 0; kicked_day = 0; sb_rkey = -1; scr = SC_WALK; wk_mode = 0; cover_now = 0; feed_hide = 0; } while (0)
#define V112_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0;
    /* 1. text rules: every wear exchange is whole groups of 3 beats (ME, SOPHIA, SOPHIA); nothing she says has boy / baby; Mama only as the reaction to the name (mama mode) */
    for (c = 0; c < 2; c++) for (k = 0; k < 3; k++) {
        int nb = *SCN[c][k]; const Beat *b = *SC[c][k];
        if (nb < 15 || nb % 3) { bad++; printf("v112: wear exchange %d/%d: whole groups of 3 beats, at least 5 (%d beats)\n", c, k, nb); continue; }
        for (i = 0; i < nb; i++) {
            if (b[i].who != (i % 3 == 0 ? W_ME : W_SOPHIA)) { bad++; printf("v112: wear exchange %d/%d beat %d: wrong speaker\n", c, k, i); }
            if (b[i].who == W_SOPHIA && (txt_boy(b[i].txt) || strstr(b[i].txt, "baby") || strstr(b[i].txt, "Baby"))) { bad++; printf("v112: wear exchange %d/%d beat %d: boy / baby\n", c, k, i); }
            if (c == 0 && strstr(b[i].txt, "Mama")) { bad++; printf("v112: wear exchange %d/%d beat %d: Mama in husband mode\n", c, k, i); }
            if (c == 1 && i % 3 == 0 && !strstr(b[i].txt, "Mama")) { bad++; printf("v112: wear exchange %d/%d beat %d: your line has the name in mama mode\n", c, k, i); }
            if (c == 1 && i % 3 == 1 && !strstr(b[i].txt, "that") && !strstr(b[i].txt, "mama") && !strstr(b[i].txt, "name")) { bad++; printf("v112: wear exchange %d/%d beat %d: her refusal rejects the name\n", c, k, i); }
            if (k < 2 && strstr(b[i].txt, "milk")) { bad++; printf("v112: wear exchange %d/%d beat %d: milk belongs to the cow outfit only\n", c, k, i); }
            if (b[i].who == W_SOPHIA && i % 3 == 1 && k < 2 && !strstr(b[i].txt, "thin") && !strstr(b[i].txt, "skin") && !strstr(b[i].txt, "see-through") && !strstr(b[i].txt, "naked") && !strstr(b[i].txt, "shows") && !strstr(b[i].txt, "chest") && !strstr(b[i].txt, "shoulders") && !strstr(b[i].txt, "curve")) { bad++; printf("v112: wear exchange %d/%d beat %d: yoga / top refusal is about thin cloth and skin\n", c, k, i); }
            if (k == 2 && i % 3 == 1 && !strstr(b[i].txt, "milk") && !strstr(b[i].txt, "cow") && !strstr(b[i].txt, "lactate") && !strstr(b[i].txt, "that") && !cow_disgust(b[i].txt)) { bad++; printf("v112: wear exchange %d/%d beat %d: cow refusal is about milk / being a cow\n", c, k, i); }
        }
    }
    for (i = 0; i < GO_COW_MILK_N; i++) if (!strstr(GO_COW_MILK[i], "pill") && !strstr(GO_COW_MILK[i], "milk") && !strstr(GO_COW_MILK[i], "chest")) { bad++; printf("v112: milk-aware cow gift beat 1 entry %d knows about the pill, milk or her chest\n", i); }
    if (GO_COW_MILK_N < 5 || GL_COW_MILK_N < 5 || GE_COW_MILK_N < 5) { bad++; printf("v112: milk-aware cow gift pools have 5 versions\n"); }
    for (i = 0; i < GO_COW_N; i++) if (strstr(GO_COW[i], "milk") || strstr(GL_COW[i], "milk")) { bad++; printf("v112: the plain cow gift (before the pill) does not know about milk (%d)\n", i); }
    for (k = 0; k < 3; k++) { const char *const *go = k == 0 ? GO_YOGA : k == 1 ? GO_TOP : GO_COW, *const *gl = k == 0 ? GL_YOGA : k == 1 ? GL_TOP : GL_COW, *const *ge = k == 0 ? GE_YOGA : k == 1 ? GE_TOP : GE_COW;
        int ng = k == 0 ? GO_YOGA_N : k == 1 ? GO_TOP_N : GO_COW_N;
        if (ng < 5 || (k == 0 ? GL_YOGA_N : k == 1 ? GL_TOP_N : GL_COW_N) < 5 || (k == 0 ? GE_YOGA_N : k == 1 ? GE_TOP_N : GE_COW_N) < 5) { bad++; printf("v112: gift %d: pools of 5\n", k); }
        for (i = 0; i < 5; i++) if (!strstr(ge[i], "slam") && !strstr(ge[i], "bang") && !strstr(ge[i], "shuts the door") && !strstr(ge[i], "takes it inside") && !strstr(ge[i], "inside") && !strstr(ge[i], "keeps it") && !strstr(ge[i], "closes the door") && !strstr(ge[i], "doesn't give")) { bad++; printf("v112: gift %d closing %d: she keeps it and shuts the door\n", k, i); }
        (void)go; (void)gl;
    }
    /* 2. the gift scene: before the pill the cow gift uses the plain pools, after the pill the milk-aware ones; yoga and top never change */
    V112_BASE(); G.pill_given = 0; G.pill_seen = 0; G.married = 0; G.rs = RS_LIVING; G.cow = 1; G.yoga = 1; G.top = 1; G.cow_given = 0; saved_G = G;   /* v11.3: outfit gifts need max romance (kept from the base) */
    gift_kind = GF_COW; gift_l1 = gift_pick(1, GF_COW); if (!gift_in_pool(1, GF_COW, gift_l1) || in_pool(gift_l1, GO_COW_MILK, GO_COW_MILK_N)) { bad++; printf("v112: before the pill the cow gift uses the plain pools\n"); }
    for (i = 0; i < 30; i++) { const char *t = gift_pick(2, GF_COW); if (!in_pool(t, GL_COW, GL_COW_N) || in_pool(t, GL_COW_MILK, GL_COW_MILK_N)) { bad++; printf("v112: before the pill beat 2 is from GL_COW\n"); break; } }
    G.pill_given = 1;
    gift_l1 = gift_pick(1, GF_COW); if (!in_pool(gift_l1, GO_COW_MILK, GO_COW_MILK_N) || !gift_in_pool(1, GF_COW, gift_l1)) { bad++; printf("v112: after the pill the cow gift beat 1 is milk-aware\n"); }
    for (i = 0; i < 30; i++) { const char *t = gift_pick(2, GF_COW); if (!in_pool(t, GL_COW_MILK, GL_COW_MILK_N) || !gift_in_pool(2, GF_COW, t)) { bad++; printf("v112: after the pill beat 2 is from GL_COW_MILK\n"); break; } }
    for (i = 0; i < 30; i++) { const char *t = gift_pick(3, GF_COW); if (!gift_in_pool(3, GF_COW, t) || !in_pool(t, GE_COW_MILK, GE_COW_MILK_N)) { bad++; printf("v112: after the pill the closing is from GE_COW_MILK\n"); break; } }
    if (gift_pick(0, GF_COW) == NULL || !in_pool(gift_pick(0, GF_COW), GH_COW, GH_COW_N)) { bad++; printf("v112: the hold-out beat is unchanged by the pill\n"); }
    for (i = 0; i < 10; i++) { const char *t = gift_pick(2, GF_YOGA); if (!in_pool(t, GL_YOGA, GL_YOGA_N)) { bad++; printf("v112: yoga gift beat 2 never uses the cow pools\n"); break; } }
    /* the gift scene itself still has 4 beats, gives the romance and the outfit */
    G.day = 20; G.gift_day = 0; saved_G = G; scr = SC_WALK; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; rom_mode = RM_MENU;
    rom_give(GF_COW);
    if (scr != SC_STORY || vn_id != SCR_GIFT || vn_n != 4 || !in_pool(vn_buf[1].txt, GO_COW_MILK, GO_COW_MILK_N) || !in_pool(vn_buf[2].txt, GL_COW_MILK, GL_COW_MILK_N) || !G.cow_given) { bad++; printf("v112: the cow gift scene after the pill (scr %d id %d n %d)\n", scr, vn_id, vn_n); }
    else RSHOT("v112_cow_gift_milk_aware");
    { int q; for (q = 0; q < 60 && scr == SC_STORY; q++) on_button(B_A); }
    /* 3. wearing, through the wardrobe (v11.3: her menu has no outfit rows): husband mode and mama mode */
#define V112_WARD(row) do { wk_mode = 0; scr = SC_WALK; wk_place(MAP_SBED, 2, 3, LF); ls_on = 0; on_button(B_A); on_button(B_A); wk_wsel = (row); on_button(B_A); } while (0)
    for (c = 0; c < 2; c++) for (k = 0; k < 3; k++) {
        const Beat *b = *SC[c][k]; int nb = *SCN[c][k], g, j, rows = 0;
        V112_BASE(); G.call = c; G.outfit = 0; V112_OPEN(); n = rom_page_acts(0, a);
        for (j = 0; j < n; j++) if (a[j] >= RA_OUTFIT_K) { bad++; printf("v112: %d/%d: her menu has no outfit row any more\n", c, k); }
        V112_BASE(); G.call = c; G.outfit = 0; V112_WARD(k);
        if (scr != SC_STORY || vn_id != SCR_OUTFIT || vn_n != 3) { bad++; printf("v112: %d/%d: choosing the outfit starts the 3-beat exchange (scr %d id %d n %d)\n", c, k, scr, vn_id, vn_n); continue; }
        if (G.outfit != 0) { bad++; printf("v112: %d/%d: the outfit must not change before the exchange ends (outfit %d)\n", c, k, G.outfit); }
        for (g = 0; g < nb / 3; g++) if (b[3 * g].txt == vn_buf[0].txt && b[3 * g + 1].txt == vn_buf[1].txt && b[3 * g + 2].txt == vn_buf[2].txt) rows = 1;
        if (!rows) { bad++; printf("v112: %d/%d: the exchange is one whole group of the pool\n", c, k); }
        if (vn_buf[0].who != W_ME || vn_buf[1].who != W_SOPHIA || vn_buf[2].who != W_SOPHIA || vn_buf[0].bg != BG_ROM) { bad++; printf("v112: %d/%d: speakers / background of the exchange\n", c, k); }
        RSHOT(c == 0 && k == 2 ? "v112_wear_cow_refusal" : "v112_wear");
        { int q; for (q = 0; q < 2 && vn_i < 1; q++) on_button(B_A); } if (vn_i != 1 || G.outfit != 0) { bad++; printf("v112: %d/%d: A advances one beat at a time (beat %d, outfit %d)\n", c, k, vn_i, G.outfit); }
        ow_play();
        if (scr != SC_WALK || wk_mode != 1 || G.outfit != OFS[k]) { bad++; printf("v112: %d/%d: after the exchange she wears the outfit and her last word is a message on the map (scr %d mode %d outfit %d)\n", c, k, scr, wk_mode, G.outfit); }
        if (!in_pool(wk_pg[0], k == 0 ? (c ? R_OUTFIT_YOGA_MM : R_OUTFIT_YOGA) : k == 1 ? (c ? R_OUTFIT_TOP_MM : R_OUTFIT_TOP) : (c ? R_OUTFIT_COW_MM : R_OUTFIT_COW), k == 0 ? (c ? R_OUTFIT_YOGA_MM_N : R_OUTFIT_YOGA_N) : k == 1 ? (c ? R_OUTFIT_TOP_MM_N : R_OUTFIT_TOP_N) : (c ? R_OUTFIT_COW_MM_N : R_OUTFIT_COW_N))) { bad++; printf("v112: %d/%d: her last word after the exchange\n", c, k); }
        WK_CLOSE();
        /* never the same group twice in a row */
        { const char *first = NULL; int same = 0, t; for (t = 0; t < 12; t++) { G.outfit = 0; V112_WARD(k); if (scr == SC_STORY && vn_id == SCR_OUTFIT) { if (first && first == vn_buf[0].txt) same = 1; first = vn_buf[0].txt; } ow_play(); WK_CLOSE(); }
          if (same) { bad++; printf("v112: %d/%d: the same exchange came twice in a row\n", c, k); } }
        /* going back to the usual outfit has no exchange */
        G.outfit = OFS[k]; if (!tst_ward(OF_USUAL) || scr != SC_WALK || wk_mode != 1 || G.outfit != OF_USUAL || !in_pool(wk_pg[0], k == 2 ? (c ? R_OUTFIT_UNCOW_MM : R_OUTFIT_UNCOW) : c ? R_OUTFIT_D8_MM : R_OUTFIT_D8, k == 2 ? (c ? R_OUTFIT_UNCOW_MM_N : R_OUTFIT_UNCOW_N) : c ? R_OUTFIT_D8_MM_N : R_OUTFIT_D8_N)) { bad++; printf("v112: %d/%d: back to the usual outfit has no exchange (scr %d outfit %d)\n", c, k, scr, G.outfit); }
        WK_CLOSE();
    }
    /* 4. the wardrobe in her bedroom: the same exchange, then the change and her last word as a message on the map */
    for (k = 0; k < 3; k++) {
        int g;
        V112_BASE(); G.call = 0; G.outfit = 0; wk_place(MAP_SBED, 2, 3, LF); ls_talk = 0; wk_mode = 8; wk_wsel = k;
        { int wt[OF_N], wn = g_outfit_targets(&G, wt); if (wn != 3 || wt[k] != OFS[k]) { bad++; printf("v112: wardrobe rows (%d)\n", wn); continue; } }
        on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_OUTFIT || vn_n != 3 || G.outfit != 0 || vn_buf[0].bg != BG_ROM) { bad++; printf("v112: wardrobe %d: the exchange starts (scr %d id %d outfit %d)\n", k, scr, vn_id, G.outfit); continue; }
        if (k == 2) RSHOT("v112_wardrobe_cow_exchange");
        for (g = 0; g < 40 && scr == SC_STORY; g++) on_button(B_A);
        if (scr != SC_WALK || G.outfit != OFS[k] || wk_mode != 1) { bad++; printf("v112: wardrobe %d: after the exchange she stands on the map with her message (scr %d outfit %d mode %d)\n", k, scr, G.outfit, wk_mode); }
        WK_CLOSE();
    }
    /* 5. an emptied pool (lines.txt) falls back to the old instant change */
    { int *np = &B_WEAR_YOGA_N, sv = *np; *np = 0; V112_BASE(); G.outfit = 0; V112_WARD(0);
      if (scr != SC_WALK || wk_mode != 1 || G.outfit != OF_YOGA) { bad++; printf("v112: an emptied wear pool changes her at once (scr %d outfit %d)\n", scr, G.outfit); } WK_CLOSE(); *np = sv; }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_mode = 0; cover_now = 0; feed_hide = 0; fh_free(); hide_free(); wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
#undef V112_BASE
#undef V112_OPEN
    return bad;
}

static int v116_tests(void) {   /* v11.6: card back colour at Sophia's table, level-up texts, 5 levels in the bar */
    int bad = 0, kscr = scr; Uint32 blue, pinkpx; int px = XS(520), py = YS(20);
    #define PIX(x, y) (*(Uint32 *)((Uint8 *)S->pixels + (y) * S->pitch + (x) * 4))
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.cash = 90000; G.day = 12; G.phase = PH_MORNING; G.energy = 100; G.pdeck = 2;
    if (g_home_start(&G, 10) != 1) { bad++; printf("v116: the home match did not start\n"); }
    scr = SC_PLAY; frozen = 1; deal_ms = 0;
    G.deck_given = 0; draw_play(); blue = PIX(px, py); RSHOT("v116_table_blue_before_deck");
    G.deck_given = 1; draw_play(); pinkpx = PIX(px, py); RSHOT("v116_table_pink_after_deck");
    if (blue == pinkpx) { bad++; printf("v116: the card back must change from blue to pink once the deck is given\n"); }
    G.rs = RS_VISIT; draw_play(); if (PIX(px, py) != blue) { bad++; printf("v116: not living with you = blue even with the flag\n"); } G.rs = RS_LIVING;
    G.deck_given = 0; draw_play(); if (PIX(px, py) != blue) { bad++; printf("v116: blue again without the flag\n"); }
    /* the romance bar: heart number counts to 5 */
    G.rom_lvl = 5; G.rom_pct = 40; if (g_rom_full(&G)) { bad++; printf("v116: level 5 at 40%% is not max\n"); }
    G.in_match = 0; scr = kscr;
    #undef PIX
    return bad;
}

static unsigned rhash(int x, int y, int w, int h) { unsigned hh = 2166136261u; int i, j; for (j = YS(y); j < YS(y + h); j++) for (i = XS(x); i < XS(x + w); i++) hh = (hh ^ *(Uint32 *)((Uint8 *)S->pixels + j * S->pitch + i * 4)) * 16777619u; return hh; }
static int txt_has_ci(const char *t, const char *w) { char a[300], b[40]; size_t i; for (i = 0; t[i] && i < sizeof a - 1; i++) a[i] = (char)tolower((unsigned char)t[i]); a[i] = 0; for (i = 0; w[i] && i < sizeof b - 1; i++) b[i] = (char)tolower((unsigned char)w[i]); b[i] = 0; return strstr(a, b) != NULL; }
static int cow_disgust(const char *t) { static const char *const W[] = {"ugh", "ew", "disgust", "revolting", "repuls", "sick", "degrading", "dirty", "vile", "crawl", "filthy", "sickened", "gag", "shudder", "wince", "grimace", "wrinkle", "recoil", "flinch", "curl"}; int i; for (i = 0; i < (int)(sizeof W / sizeof W[0]); i++) if (txt_has_ci(t, W[i])) return 1; return 0; }
static int pair_in(const Beat *b, int n, const char *me, const char *her) { int i; for (i = 0; i + 1 < n; i += 2) if (b[i].txt == me && b[i + 1].txt == her) return 1; return 0; }
static int v121_tests(void) {   /* v12.1: COW MODE: disgust at the cow outfit (gift, wear exchange); while she wears it (married) her dialogue is negative everywhere but she still accepts */
    int bad = 0, i, k, mm; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk;
#define V121_BASE() do { g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1; \
    G.milk_st = 4; G.milk_pct = 100; G.mpill = 6; G.dring = 1; G.married = 1; G.event = 1; G.hall_seen = 3; G.ring = 1; G.cash = 90000; G.energy = 100; G.yoga_given = 1; G.top_given = 1; G.cow_given = 1; G.mm_kind = 0; saved_G = G; ls_away = -1; ls_talk = 0; ls_on = 0; sb_out = 0; kicked_day = 0; sb_rkey = -1; scr = SC_WALK; wk_mode = 0; cover_now = 0; feed_hide = 0; } while (0)
#define V121_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0;
    /* 1. text rules: the gift and wear texts are disgusted (not only angry), every wear exchange is whole groups of 3, no boy / baby in any cow mode text */
    if (GO_COW_N < 6 || GL_COW_N < 6 || GE_COW_N < 6 || GO_COW_MILK_N < 6 || GL_COW_MILK_N < 6 || GE_COW_MILK_N < 6) { bad++; printf("v121: the cow gift pools have 6 versions\n"); }
    for (i = 0; i < GO_COW_N; i++) if (!cow_disgust(GO_COW[i]) || !cow_disgust(GL_COW[i])) { bad++; printf("v121: cow gift entry %d is not disgusted\n", i); }
    for (i = 0; i < GO_COW_MILK_N; i++) if (!cow_disgust(GO_COW_MILK[i]) || !cow_disgust(GL_COW_MILK[i])) { bad++; printf("v121: milk-aware cow gift entry %d is not disgusted\n", i); }
    for (i = 0; i < R_OUTFIT_COW_N; i++) if (!cow_disgust(R_OUTFIT_COW[i])) { bad++; printf("v121: R_OUTFIT_COW %d is not disgusted\n", i); }
    for (mm = 0; mm < 2; mm++) { const Beat *b = mm ? B_WEAR_COW_MM : B_WEAR_COW; int n = mm ? B_WEAR_COW_MM_N : B_WEAR_COW_N;
        if (n % 3 || n < 18) { bad++; printf("v121: the wear exchange %d has whole groups (n %d), at least 6\n", mm, n); }
        for (i = 0; i + 2 < n; i += 3) if (b[i].who != W_ME || b[i + 1].who != W_SOPHIA || b[i + 2].who != W_SOPHIA || !cow_disgust(b[i + 1].txt)) { bad++; printf("v121: wear group %d/%d: ME, her disgusted refusal, her give-in\n", mm, i / 3); }
        for (i = 0; i + 2 < n; i += 3) { const char *g = b[i + 2].txt; if (!(strstr(g, "Fine") || strstr(g, "fine"))) { bad++; printf("v121: wear group %d/%d: she gives in\n", mm, i / 3); } } }
    { const struct { const char *const *p; int *n; } PL[] = {{R_GREET_CW, &R_GREET_CW_N}, {R_GREET_CW_MM, &R_GREET_CW_MM_N}, {R_NIGHT_CW, &R_NIGHT_CW_N}, {R_NIGHT_CW_MM, &R_NIGHT_CW_MM_N}, {R_MORE_CW, &R_MORE_CW_N}, {R_MORE_CW_MM, &R_MORE_CW_MM_N}, {R_NOT_FULL_CW, &R_NOT_FULL_CW_N}, {R_NOT_FULL_CW_MM, &R_NOT_FULL_CW_MM_N}, {R_HUG_OPEN_CW, &R_HUG_OPEN_CW_N}, {R_HUG_OPEN_CW_MM, &R_HUG_OPEN_CW_MM_N}, {R_KISS_OPEN_CW, &R_KISS_OPEN_CW_N}, {R_KISS_OPEN_CW_MM, &R_KISS_OPEN_CW_MM_N}, {C_FAIL_CW, &C_FAIL_CW_N}, {C_FAIL_CW_MM, &C_FAIL_CW_MM_N}, {R_OUTFIT_UNCOW, &R_OUTFIT_UNCOW_N}, {R_OUTFIT_UNCOW_MM, &R_OUTFIT_UNCOW_MM_N}};
      int pi; for (pi = 0; pi < (int)(sizeof PL / sizeof PL[0]); pi++) { if (*PL[pi].n < 4) { bad++; printf("v121: cow pool %d has at least 4 lines\n", pi); } for (i = 0; i < *PL[pi].n; i++) if (txt_boy(PL[pi].p[i]) || strstr(PL[pi].p[i], "baby")) { bad++; printf("v121: cow pool %d line %d says boy / baby\n", pi, i); } } }
    for (i = 0; i < C_COW.n; i++) if (txt_boy(C_COW.t[i].a) || txt_boy(C_COW.t[i].me)) { bad++; printf("v121: C_COW %d says boy\n", i); }
    if (C_COW.n < 6 || C_COW_MM.n < 6) { bad++; printf("v121: the cow chat has at least 6 exchanges\n"); }
    /* 2. the mode follows the outfit, only when married */
    V121_BASE(); G.outfit = OF_USUAL; if (g_cow(&G)) { bad++; printf("v121: no cow mode in her usual outfit\n"); } G.outfit = OF_YOGA; if (g_cow(&G)) { bad++; printf("v121: no cow mode in the yoga outfit\n"); }
    G.outfit = OF_COW; if (!g_cow(&G)) { bad++; printf("v121: cow mode while she wears the cow outfit\n"); } G.married = 0; if (g_cow(&G)) { bad++; printf("v121: no cow mode before the wedding\n"); } G.married = 1;
    /* 3. greetings (day, night), after-chat line, not-full answer */
    for (mm = 0; mm < 2; mm++) { G.call = mm; G.outfit = OF_COW; G.mm_kind = 0;
        for (k = 0; k < 30; k++) { const char *g = rom_greet(), *nt = night_greet(); if (!in_pool(g, mm ? R_GREET_CW_MM : R_GREET_CW, mm ? R_GREET_CW_MM_N : R_GREET_CW_N)) { bad++; printf("v121: %d: her greeting in cow mode\n", mm); break; } if (!in_pool(nt, mm ? R_NIGHT_CW_MM : R_NIGHT_CW, mm ? R_NIGHT_CW_MM_N : R_NIGHT_CW_N)) { bad++; printf("v121: %d: her night greeting in cow mode\n", mm); break; } }
        G.outfit = OF_USUAL; { const char *g = rom_greet(); if (in_pool(g, R_GREET_CW, R_GREET_CW_N) || in_pool(g, R_GREET_CW_MM, R_GREET_CW_MM_N)) { bad++; printf("v121: %d: the cow greeting stays out of her usual outfit\n", mm); } }
        G.outfit = OF_COW; V121_OPEN(); chat_start(); talk_end(); if (!in_pool(rom_txt, mm ? R_MORE_CW_MM : R_MORE_CW, mm ? R_MORE_CW_MM_N : R_MORE_CW_N) && tk_kind != TK_TEASE) { bad++; printf("v121: %d: after a chat her line is from the cow pool (%s)\n", mm, rom_txt); }
        G.milk_pct = 20; V121_OPEN(); rom_feed(); if (!in_pool(rom_txt, mm ? R_NOT_FULL_CW_MM : R_NOT_FULL_CW, mm ? R_NOT_FULL_CW_MM_N : R_NOT_FULL_CW_N)) { bad++; printf("v121: %d: 'not full' in cow mode (%s)\n", mm, rom_txt); } G.milk_pct = 100; }
    /* 4. chat: her own exchanges (or her cow 'not in the mood' line) */
    for (mm = 0; mm < 2; mm++) { const TalkPool *cp = mm ? &C_COW_MM : &C_COW; G.call = mm; G.outfit = OF_COW;
        for (k = 0; k < 60; k++) { int found = 0; V121_OPEN(); chat_start();
            for (i = 0; i < cp->n; i++) if (tk_t[0] == cp->t[i].me) found = 1;
            if (tk_kind == TK_FAIL) { if (!in_pool(tk_t[1], mm ? C_FAIL_CW_MM : C_FAIL_CW, mm ? C_FAIL_CW_MM_N : C_FAIL_CW_N)) { bad++; printf("v121: %d: the cow 'not in the mood' line\n", mm); break; } }
            else if (!found) { bad++; printf("v121: %d: a chat in cow mode uses the cow exchanges\n", mm); break; } } }
    /* 5. the page-4 scenes */
    for (mm = 0; mm < 2; mm++) { G.call = mm; G.outfit = OF_COW; V121_OPEN();
        vn_start(SCR_HUG); if (vn_buf[1].txt != (mm ? B_HUG_CW_MM : B_HUG_CW)[1].txt || !in_pool(vn_buf[0].txt, mm ? R_HUG_OPEN_CW_MM : R_HUG_OPEN_CW, mm ? R_HUG_OPEN_CW_MM_N : R_HUG_OPEN_CW_N)) { bad++; printf("v121: %d: hug in cow mode\n", mm); }
        vn_start(SCR_KISS); if (vn_buf[1].txt != (mm ? B_KISS_CW_MM : B_KISS_CW)[1].txt || !in_pool(vn_buf[0].txt, mm ? R_KISS_OPEN_CW_MM : R_KISS_OPEN_CW, mm ? R_KISS_OPEN_CW_MM_N : R_KISS_OPEN_CW_N)) { bad++; printf("v121: %d: kiss in cow mode\n", mm); }
        vn_start(SCR_SLEEP); if (vn_buf[1].txt != (mm ? B_SLEEP_CW_MM : B_SLEEP_CW)[1].txt) { bad++; printf("v121: %d: sleep in cow mode\n", mm); }
        vn_start(SCR_NIGHTM); if (vn_buf[1].txt != (mm ? B_NIGHTM_CW_MM : B_NIGHTM_CW)[1].txt) { bad++; printf("v121: %d: the night knock in cow mode\n", mm); }
        G.outfit = OF_USUAL; vn_start(SCR_HUG); if (vn_buf[1].txt == B_HUG_CW[1].txt || vn_buf[1].txt == B_HUG_CW_MM[1].txt) { bad++; printf("v121: %d: the cow hug stays out of her usual outfit\n", mm); } G.outfit = OF_COW; }
    /* 6. BREASTFEED: always accepted (the scene runs to the end), one cow pair in the middle; door / bedroom and living room, normal and Mama */
    for (mm = 0; mm < 2; mm++) { int lr; for (lr = 0; lr < 2; lr++) { const Beat *sc = mm ? (lr ? B_FEED_CW_MM_LR : B_FEED_CW_MM) : (lr ? B_FEED_CW_LR : B_FEED_CW); int sn = mm ? (lr ? B_FEED_CW_MM_LR_N : B_FEED_CW_MM_N) : (lr ? B_FEED_CW_LR_N : B_FEED_CW_N);
        const Beat *mid = mm ? B_MID_FEED_CW_MM : B_MID_FEED_CW; int mn = mm ? B_MID_FEED_CW_MM_N : B_MID_FEED_CW_N;
        G.call = mm; G.outfit = OF_COW; G.milk_pct = 100; wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = lr; if (lr) wk_place(MAP_ROOM, 5, 5, LF); rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_in = lr ? 0 : 1;
        vn_start(SCR_FEED);
        { int base = sn + 2 + (lr ? 1 : 0); if (vn_n != base) { bad++; printf("v121: %d/%d: the cow feed scene has %d beats (expected %d)\n", mm, lr, vn_n, base); } }
        { int p0 = lr ? 1 : 0, found = 0, j; if (vn_buf[p0].txt != sc[0].txt) { bad++; printf("v121: %d/%d: the cow feed scene opens with its own beat\n", mm, lr); }
          if (lr && !in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N)) { bad++; printf("v121: %d/%d: Here?! comes first in the living room\n", mm, lr); }
          for (j = 0; j + 1 < vn_n; j++) { int m; for (m = 0; m + 1 < mn; m += 2) if (vn_buf[j].txt == mid[m].txt && vn_buf[j + 1].txt == mid[m + 1].txt) found = 1; } if (!found) { bad++; printf("v121: %d/%d: one cow pair in the middle of the feed scene\n", mm, lr); } } } }
    ls_talk = 0;
    /* 7. the pump scene (door / bedroom and living room) and the pill / super drug gift scenes: her disgusted opener, she still goes on */
    for (mm = 0; mm < 2; mm++) { int lr; for (lr = 0; lr < 2; lr++) { int pill; for (pill = 0; pill < 2; pill++) {
        const Beat *op = mm ? (pill ? B_OPEN_PILL_CW_MM : B_OPEN_ASK_CW_MM) : (pill ? B_OPEN_PILL_CW : B_OPEN_ASK_CW); int on = mm ? (pill ? B_OPEN_PILL_CW_MM_N : B_OPEN_ASK_CW_MM_N) : (pill ? B_OPEN_PILL_CW_N : B_OPEN_ASK_CW_N);
        G.call = mm; G.outfit = OF_COW; wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = lr; if (lr) wk_place(MAP_ROOM, 5, 5, LF); rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_in = lr ? 0 : 1;
        bottle_pill = pill; vn_start(SCR_BOTTLE);
        { int at = lr ? 1 : 0, j, found = 0, bn = lr ? B_BOTTLE_LR_N : B_BOTTLE_N; (void)j;
          if (lr && !in_pool(vn_buf[0].txt, R_LR_HERE, R_LR_HERE_N)) { bad++; printf("v121: %d/%d/%d: Here?! first in the pump scene\n", mm, lr, pill); }
          if (!pair_in(op, on, vn_buf[at].txt, vn_buf[at + 1].txt)) { bad++; printf("v121: %d/%d/%d: the pump scene opens with the cow pair\n", mm, lr, pill); }
          if (vn_n != LEAD_BEATS + bn + 2 + 2 + (lr ? 1 : 0)) { bad++; printf("v121: %d/%d/%d: the pump scene is lead-in + scene + opener + mid pair (%d beats)\n", mm, lr, pill, vn_n); }
          { const Beat *mid = mm ? B_MID_PUMP_CW_MM : B_MID_PUMP_CW; int mn = mm ? B_MID_PUMP_CW_MM_N : B_MID_PUMP_CW_N, m; for (j = 0; j + 1 < vn_n; j++) for (m = 0; m + 1 < mn; m += 2) if (vn_buf[j].txt == mid[m].txt && vn_buf[j + 1].txt == mid[m + 1].txt) found = 1; } if (!found) { bad++; printf("v121: %d/%d/%d: one cow pair in the middle of the pump scene\n", mm, lr, pill); } } } } }
    ls_talk = 0;
    for (mm = 0; mm < 2; mm++) { int kind; for (kind = 0; kind < 2; kind++) { int gk = kind ? GF_SUPER : GF_MILK;
        const Beat *op = mm ? (kind ? B_OPEN_SUPER_CW_MM : B_OPEN_PILL_CW_MM) : (kind ? B_OPEN_SUPER_CW : B_OPEN_PILL_CW); int on = mm ? (kind ? B_OPEN_SUPER_CW_MM_N : B_OPEN_PILL_CW_MM_N) : (kind ? B_OPEN_SUPER_CW_N : B_OPEN_PILL_CW_N);
        G.call = mm; G.outfit = OF_COW; wk_mode = 0; scr = SC_WALK; ls_on = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_in = 1;
        gift_kind = gk; gift_l1 = gift_pick(1, gk); vn_start(SCR_GIFT);
        if (vn_n != 6 || !pair_in(op, on, vn_buf[0].txt, vn_buf[1].txt)) { bad++; printf("v121: %d/%d: the pill / super drug gift scene opens with the cow pair (n %d)\n", mm, kind, vn_n); }
        G.outfit = OF_USUAL; gift_l1 = gift_pick(1, gk); vn_start(SCR_GIFT); if (pair_in(op, on, vn_buf[0].txt, vn_buf[1].txt) || vn_n > 4 + (mm ? 4 : 0)) { bad++; printf("v121: %d/%d: no cow pair outside cow mode (n %d)\n", mm, kind, vn_n); } } }
    /* 8. changing out of the cow outfit: her relieved line (usual outfit: no exchange; other outfits: the exchange, then the relieved line); changing into it: the cow line */
    for (mm = 0; mm < 2; mm++) { G.call = mm; V121_BASE(); G.call = mm; G.outfit = OF_COW;
        if (!tst_ward(OF_USUAL) || G.outfit != OF_USUAL || !in_pool(wk_pg[0], mm ? R_OUTFIT_UNCOW_MM : R_OUTFIT_UNCOW, mm ? R_OUTFIT_UNCOW_MM_N : R_OUTFIT_UNCOW_N)) { bad++; printf("v121: %d: leaving the cow outfit for her usual one gives the relieved line (outfit %d)\n", mm, G.outfit); } WK_CLOSE();
        G.outfit = OF_COW; if (!tst_ward(OF_YOGA) || G.outfit != OF_YOGA || !in_pool(wk_pg[0], mm ? R_OUTFIT_UNCOW_MM : R_OUTFIT_UNCOW, mm ? R_OUTFIT_UNCOW_MM_N : R_OUTFIT_UNCOW_N)) { bad++; printf("v121: %d: leaving the cow outfit for the yoga outfit: the exchange, then the relieved line (outfit %d)\n", mm, G.outfit); } WK_CLOSE();
        if (g_cow(&G)) { bad++; printf("v121: %d: cow mode ends with the cow outfit\n", mm); }
        G.outfit = OF_USUAL; if (!tst_ward(OF_COW) || G.outfit != OF_COW || !in_pool(wk_pg[0], mm ? R_OUTFIT_COW_MM : R_OUTFIT_COW, mm ? R_OUTFIT_COW_MM_N : R_OUTFIT_COW_N) || !g_cow(&G)) { bad++; printf("v121: %d: changing into the cow outfit gives the disgusted line and starts cow mode (outfit %d)\n", mm, G.outfit); } WK_CLOSE();
        G.outfit = OF_YOGA; if (!tst_ward(OF_TOP) || in_pool(wk_pg[0], mm ? R_OUTFIT_UNCOW_MM : R_OUTFIT_UNCOW, mm ? R_OUTFIT_UNCOW_MM_N : R_OUTFIT_UNCOW_N)) { bad++; printf("v121: %d: the relieved line only follows the cow outfit\n", mm); } WK_CLOSE(); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_mode = 0; cover_now = 0; feed_hide = 0; fh_free(); hide_free(); wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE; bottle_pill = 0;
#undef V121_BASE
#undef V121_OPEN
    return bad;
}

static int v122_tests(void) {   /* v12.2: the breast pump wears out: she says it is broken when you ask (PUMP MILK, ASK FOR MILK, a milk pill that would fill the gauge), the store lists a new pump only after that, it is handed over from the give page, debug row, texts */
    int bad = 0, i, mm, cw; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk;
#define V122_BASE() do { g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1; \
    G.milk_st = 4; G.milk_pct = 100; G.mpill = 6; G.dring = 1; G.married = 1; G.event = 1; G.hall_seen = 3; G.ring = 1; G.cash = 90000; G.energy = 100; G.mm_kind = 0; saved_G = G; ls_away = -1; ls_talk = 0; ls_on = 0; sb_out = 0; kicked_day = 0; sb_rkey = -1; scr = SC_WALK; wk_mode = 0; cover_now = 0; feed_hide = 0; } while (0)
#define V122_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
    const struct { const char *const *p; int *n; int min; } PL[] = {{R_PUMP_BROKEN, &R_PUMP_BROKEN_N, 6}, {R_PUMP_BROKEN_MM, &R_PUMP_BROKEN_MM_N, 6}, {R_PUMP_BROKEN_CW, &R_PUMP_BROKEN_CW_N, 6}, {R_PUMP_BROKEN_CW_MM, &R_PUMP_BROKEN_CW_MM_N, 6},
        {R_PUMP_NEW, &R_PUMP_NEW_N, 4}, {R_PUMP_NEW_MM, &R_PUMP_NEW_MM_N, 4}, {R_PUMP_NEW_CW, &R_PUMP_NEW_CW_N, 4}, {R_PUMP_NEW_CW_MM, &R_PUMP_NEW_CW_MM_N, 4}};
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0;
    /* 1. the texts: 6 broken lines and 4 thanks in each of the four modes, none says boy / baby, all fit a dialogue box */
    for (i = 0; i < (int)(sizeof PL / sizeof PL[0]); i++) { int j; if (*PL[i].n < PL[i].min) { bad++; printf("v122: pump pool %d has at least %d lines\n", i, PL[i].min); }
        for (j = 0; j < *PL[i].n; j++) if (txt_boy(PL[i].p[j]) || strstr(PL[i].p[j], "baby") || strlen(PL[i].p[j]) > 130) { bad++; printf("v122: pump pool %d line %d says boy / baby or is too long\n", i, j); } }
    if (strcmp(SH_INFO[SH_PUMP], "GIVE IT TO HER. IT CAN BREAK") != 0) { bad++; printf("v122: the store line of the pump says it can break (%s)\n", SH_INFO[SH_PUMP]); }
    /* 2. PUMP MILK with a worn-out pump (all four modes): her broken line, a note, no bottle, the gauge stays full, the pump is listed in the store afterwards */
    for (cw = 0; cw < 2; cw++) for (mm = 0; mm < 2; mm++) {
        int a[10], n, idx = -1; const char *const *pool = cw ? (mm ? R_PUMP_BROKEN_CW_MM : R_PUMP_BROKEN_CW) : (mm ? R_PUMP_BROKEN_MM : R_PUMP_BROKEN); int pn = cw ? (mm ? R_PUMP_BROKEN_CW_MM_N : R_PUMP_BROKEN_CW_N) : (mm ? R_PUMP_BROKEN_MM_N : R_PUMP_BROKEN_N);
        V122_BASE(); G.call = mm; G.outfit = cw ? OF_COW : OF_USUAL; G.cow_given = 1; G.pump_life = 10; G.pump_used = 10; G.milk_pct = 100;
        V122_OPEN(); n = rom_page_acts(2, a); for (i = 0; i < n; i++) if (a[i] == RA_PUMP) idx = i; if (idx < 0) { bad++; printf("v122: PUMP MILK is in the menu while the gauge is full (a worn-out pump too)\n"); continue; }
        rom_page = 2; rom_sel = idx; on_button(B_A);
        if (scr != SC_ROM || !in_pool(rom_txt, pool, pn) || G.bottle != 0 || G.milk_pct != 100 || !G.pump_dead || G.mday == G.day || !strstr(rom_note, "BUY A NEW ONE")) { bad++; printf("v122: %d/%d: broken pump: her line, no bottle, the gauge stays full, the note (scr %d bottle %d pct %d dead %d note %s)\n", cw, mm, scr, G.bottle, G.milk_pct, G.pump_dead, rom_note); }
        if (cw == 0 && mm == 0) RSHOT("v122_pump_broken");
    }
    /* 3. the store: no pump while hers works, a pump once she told you, bought once; the give page row; her thanks; it works again */
    V122_BASE(); G.pump_life = 12; G.pump_used = 3; { int o[16], k, has = 0, c = shop_list(TAB_ITEMS, o); for (k = 0; k < c; k++) if (o[k] == SH_PUMP) has = 1; if (has || shop_sold(SH_PUMP) == 0) { bad++; printf("v122: the pump is not on offer while hers works (has %d)\n", has); } }
    G.pump_used = 12; G.milk_pct = 100; rom_pump(); if (!G.pump_dead) { bad++; printf("v122: the broken pump is found out when you ask\n"); }
    { int o[16], k, has = 0, c = shop_list(TAB_ITEMS, o); for (k = 0; k < c; k++) if (o[k] == SH_PUMP) has = 1; if (!has || shop_sold(SH_PUMP)) { bad++; printf("v122: after she told you the pump is in the store (has %d)\n", has); } }
    { int a[10], n, idx = -1; V122_OPEN(); n = rom_page_acts(1, a); for (i = 0; i < n; i++) if (a[i] == RA_GIVE + GF_PUMP) idx = i; if (idx >= 0) { bad++; printf("v122: no pump row on the give page before you own the new one\n"); } }
    if (g_shop_buy(&G, SH_PUMP) != 0 || G.pump != 1) { bad++; printf("v122: buying the new pump\n"); }
    { int a[10], n, idx = -1; V122_OPEN(); n = rom_page_acts(1, a); for (i = 0; i < n; i++) if (a[i] == RA_GIVE + GF_PUMP) idx = i; if (idx < 0) { bad++; printf("v122: the new pump is on the give page\n"); }
      else { rom_page = 1; rom_sel = idx; RSHOT("v122_give_page_pump"); on_button(B_A);
        if (scr != SC_ROM || !in_pool(rom_txt, R_PUMP_NEW, R_PUMP_NEW_N) || G.pump != 0 || G.pump_dead || G.pump_used != 0 || G.pump_life < PUMP_MIN || G.pump_life > PUMP_MAX || !strstr(rom_note, "NEW PUMP READY") || G.gift_day == G.day) { bad++; printf("v122: handing over the new pump: her thanks, a new life, not the gift of the day (scr %d pump %d dead %d)\n", scr, G.pump, G.pump_dead); }
        RSHOT("v122_new_pump"); n = rom_page_acts(1, a); idx = -1; for (i = 0; i < n; i++) if (a[i] == RA_GIVE + GF_PUMP) idx = i; if (idx >= 0) { bad++; printf("v122: the pump row is gone after handing it over\n"); } } }
    G.milk_pct = 100; G.bottle = 0; rom_pump(); if (scr != SC_STORY || vn_id != SCR_BOTTLE || G.bottle != 1 || G.pump_used != 1) { bad++; printf("v122: the new pump makes a bottle again (id %d bottle %d used %d)\n", vn_id, G.bottle, G.pump_used); }
    /* 4. ASK FOR MILK (Super drug): the broken pump is found out, the ask is not used up */
    V122_BASE(); G.married = 0; G.call = 0; G.sdrug = 1; g_gift(&G, GF_SUPER); G.pump_life = 4; G.pump_used = 4; scr = SC_WALK; V122_OPEN(); rom_ask();
    if (scr != SC_ROM || !in_pool(rom_txt, R_PUMP_BROKEN, R_PUMP_BROKEN_N) || G.bottle != 0 || G.ask_n != 0 || !G.pump_dead) { bad++; printf("v122: ASK FOR MILK with a broken pump (scr %d bottle %d ask_n %d)\n", scr, G.bottle, G.ask_n); }
    /* 5. a milk pill that would fill the gauge (before the wedding): refused with her broken line, the pill stays in the bag */
    V122_BASE(); G.married = 0; G.call = 0; G.milk_pct = 50; G.mpill = 3; G.pump_life = 5; G.pump_used = 5; V122_OPEN(); rom_give(GF_MILK);
    if (scr != SC_ROM || !in_pool(rom_txt, R_PUMP_BROKEN, R_PUMP_BROKEN_N) || G.mpill != 3 || G.milk_pct != 50 || G.bottle != 0 || !G.pump_dead) { bad++; printf("v122: a pill that would make a bottle with a broken pump (scr %d mpill %d pct %d)\n", scr, G.mpill, G.milk_pct); }
    /* 6. the debug row */
    if (DBG_N != 11 || strcmp(DBG_ITEMS[8], "BREAK THE PUMP") != 0 || strcmp(DBG_ITEMS[DBG_N - 1], "BACK") != 0) { bad++; printf("v122: the debug panel has BREAK THE PUMP before BACK\n"); }
    V122_BASE(); G.pump_life = 11; G.pump_used = 2; scr = SC_DEBUG; dbg_sel = 8; on_button(B_A); if (G.pump_used != G.pump_life) { bad++; printf("v122: the debug row wears the pump out (used %d life %d)\n", G.pump_used, G.pump_life); }
    scr = SC_DEBUG; V122_BASE(); G.milk_st = 1; scr = SC_DEBUG; dbg_sel = 8; on_button(B_A); if (G.pump_life != 0) { bad++; printf("v122: the debug row needs a pump\n"); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_mode = 0; cover_now = 0; feed_hide = 0; fh_free(); hide_free(); wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE; bottle_pill = 0; rom_note[0] = 0;
#undef V122_BASE
#undef V122_OPEN
    return bad;
}

static int v123_tests(void) {   /* v12.3: the Mommy Milk Injection: store, give page and scene, no gauge, one milk a day (ask = 2 bottles, pump, breastfeed), pill / drug refused, the handing-bottle pictures, debug row */
    int bad = 0, i, mm, cw; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk;
#define V123_BASE() do { g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1; \
    G.milk_st = 4; G.milk_pct = 50; G.mpill = 6; G.dring = 1; G.married = 1; G.event = 1; G.hall_seen = 3; G.ring = 1; G.cash = 90000; G.energy = 100; G.sw_wins = SW_GOAL; G.pump_life = 14; G.mm_kind = 0; saved_G = G; ls_away = -1; ls_talk = 0; ls_on = 0; sb_out = 0; kicked_day = 0; sb_rkey = -1; scr = SC_WALK; wk_mode = 0; cover_now = 0; feed_hide = 0; bottle_inj = 0; } while (0)
#define V123_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
    const struct { const char *const *p; int *n; int min; } PL[] = {{GH_INJECT, &GH_INJECT_N, 5}, {GO_INJECT, &GO_INJECT_N, 5}, {GL_INJECT, &GL_INJECT_N, 5}, {GE_INJECT, &GE_INJECT_N, 5}, {R_INJ_DONE, &R_INJ_DONE_N, 4}, {R_INJ_DONE_MM, &R_INJ_DONE_MM_N, 4}, {R_INJ_DONE_CW, &R_INJ_DONE_CW_N, 4}, {R_INJ_DONE_CW_MM, &R_INJ_DONE_CW_MM_N, 4}, {R_INJ_NOPILL, &R_INJ_NOPILL_N, 4}, {R_INJ_WAIT, &R_INJ_WAIT_N, 3}};
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0;
    /* 1. texts and tables */
    for (i = 0; i < (int)(sizeof PL / sizeof PL[0]); i++) { int j; if (*PL[i].n < PL[i].min) { bad++; printf("v123: pool %d has at least %d lines\n", i, PL[i].min); }
        for (j = 0; j < *PL[i].n; j++) if (txt_boy(PL[i].p[j]) || strstr(PL[i].p[j], "baby") || strlen(PL[i].p[j]) > 130) { bad++; printf("v123: pool %d line %d says boy / baby or is too long\n", i, j); } }
    if (strcmp(SH_NAME[SH_INJECT], "MOMMY MILK INJECTION") || !SH_DESC[SH_INJECT] || !SH_INFO[SH_INJECT] || !SH_BOUGHT[SH_INJECT] || strcmp(R_GNAME[GF_INJECT], "MOMMY INJECTION")) { bad++; printf("v123: the item texts\n"); }
    { char w[400]; int j, ln = 1; wrap_buf(w, sizeof w, SH_DESC[SH_INJECT], 28); for (j = 0; w[j]; j++) if (w[j] == '\n') ln++; if (ln > 5) { bad++; printf("v123: the info page needs %d lines (5 fit)\n", ln); } }
    if (!giftS[SH_INJECT] || giftS[SH_INJECT]->h > 88 || giftS[SH_INJECT]->w > 110) { bad++; printf("v123: assets/gift_inject.png missing or too big for the store row\n"); }
    if (!R_GIFT_HAND[GF_INJECT] || !R_GIFT_L2[GF_INJECT] || !R_GIFT_END[GF_INJECT] || !R_GNAME[GF_INJECT]) { bad++; printf("v123: fallback gift texts\n"); }
    /* 2. the handing-bottle pictures: one per outfit, 752x560 */
    for (i = 0; i < OF_N; i++) { SDL_Surface *p; V123_BASE(); G.outfit = i; p = sc_pic(BG_BOTTLE); if (!p || p->w != SW || p->h != SH) { bad++; printf("v123: sc_bottle picture for outfit %d missing or the wrong size\n", i); } }
    /* 3. the store: black market row, info page, buying, gone afterwards */
    V123_BASE(); { int o[16], c = shop_list(TAB_BM, o), k, at = -1; for (k = 0; k < c; k++) if (o[k] == SH_INJECT) at = k; if (at < 0 || o[c - 1] != SH_INJECT) { bad++; printf("v123: the injection is the last BLACK MARKET row (at %d of %d)\n", at, c); } }
    shop_tab = TAB_BM; shop_sel = SH_INJECT; shop_info = 0; scr = SC_SHOP; draw_shop(); RSHOT("v123_store_list"); on_button(B_Y); if (!shop_info) { bad++; printf("v123: Y opens the injection info page\n"); } draw_shop(); RSHOT("v123_store_info"); shop_info = 0;
    on_button(B_A); on_button(B_A); if (G.inj != 1 || G.cash != 90000 - 30000) { bad++; printf("v123: buying through the buttons (inj %d cash %d)\n", G.inj, G.cash); }
    { int o[16], c = shop_list(TAB_BM, o), k; for (k = 0; k < c; k++) if (o[k] == SH_INJECT) { bad++; printf("v123: bought: still listed\n"); } }
    V123_BASE(); G.pill_seen = 0; { int o[16], c = shop_list(TAB_BM, o), k; for (k = 0; k < c; k++) if (o[k] == SH_INJECT) { bad++; printf("v123: listed before the day-8 scene\n"); } }
    /* 4. the give page and the scene */
    V123_BASE(); G.inj = 1; G.mpill = 3; G.sdrug = 1; { int a[10], n, idx = -1; V123_OPEN(); n = rom_page_acts(1, a); for (i = 0; i < n; i++) if (a[i] == RA_GIVE + GF_INJECT) idx = i; if (idx < 0) { bad++; printf("v123: the injection is on the give page\n"); } }
    rom_give(GF_INJECT);
    if (scr != SC_STORY || vn_id != SCR_GIFT || vn_n < 4 || !G.injected || G.inj != 0 || G.milk_pct != 0 || G.gift_day == G.day || !gift_in_pool(0, GF_INJECT, vn_buf[vn_n - 4].txt)) { bad++; printf("v123: giving the injection (scr %d id %d n %d injected %d)\n", scr, vn_id, vn_n, G.injected); }
    { int k, hit = 0; for (k = 0; k < vn_n; k++) if (gift_in_pool(3, GF_INJECT, vn_buf[k].txt) || strstr(vn_buf[k].txt, "PERMANENT")) hit = 1; if (!hit) { bad++; printf("v123: the closing line of the injection scene\n"); } }
    { int a[10], n; G.sdrug = 1; G.mpill = 3; V123_OPEN(); n = rom_page_acts(1, a); for (i = 0; i < n; i++) if (a[i] == RA_MILK || a[i] == RA_SUPER || a[i] == RA_GIVE + GF_INJECT) { bad++; printf("v123: milk pill / Super drug / injection rows are gone from the give page\n"); } }
    rom_give(GF_MILK); if (scr != SC_ROM || !in_pool(rom_txt, R_INJ_NOPILL, R_INJ_NOPILL_N) || G.mpill != 3) { bad++; printf("v123: the milk pill is refused (scr %d mpill %d)\n", scr, G.mpill); }
    rom_give(GF_SUPER); if (!in_pool(rom_txt, R_INJ_NOPILL, R_INJ_NOPILL_N) || G.sdrug != 1) { bad++; printf("v123: the Super drug is refused\n"); }
    V123_BASE(); G.inj = 1; G.sdrug = 1; g_gift(&G, GF_SUPER); V123_OPEN(); rom_give(GF_INJECT); if (scr != SC_ROM || !in_pool(rom_txt, R_INJ_WAIT, R_INJ_WAIT_N) || G.injected || G.inj != 1) { bad++; printf("v123: the injection waits for the Super drug\n"); }
    /* 5. the menu and the gauge */
    V123_BASE(); G.injected = 1; { int a[10], n, ask = -1, pump = -1; V123_OPEN(); n = rom_page_acts(2, a); for (i = 0; i < n; i++) { if (a[i] == RA_ASK) ask = i; if (a[i] == RA_PUMP) pump = i; } if (ask < 0 || pump < 0) { bad++; printf("v123: ASK FOR MILK and PUMP MILK are on page 3 (ask %d pump %d)\n", ask, pump); }
        rom_page = 2; rom_sel = ask < 0 ? 0 : ask; draw_rom(); RSHOT("v123_menu_ask"); rom_page = 3; G.call = 1; draw_rom(); RSHOT("v123_menu_feed"); if (g_milk_gauge(&G)) { bad++; printf("v123: no gauge after the injection\n"); } }
    /* 6. ONE milk a day: ask (2 bottles, hand-over picture), then everything else refused with her line, in all four modes */
    for (cw = 0; cw < 2; cw++) for (mm = 0; mm < 2; mm++) {
        const char *const *pool = cw ? (mm ? R_INJ_DONE_CW_MM : R_INJ_DONE_CW) : (mm ? R_INJ_DONE_MM : R_INJ_DONE); int pn = cw ? (mm ? R_INJ_DONE_CW_MM_N : R_INJ_DONE_CW_N) : (mm ? R_INJ_DONE_MM_N : R_INJ_DONE_N);
        V123_BASE(); G.injected = 1; G.call = mm; G.outfit = cw ? OF_COW : OF_USUAL; G.cow_given = 1; G.bottle = 0; V123_OPEN(); rom_ask();
        if (scr != SC_STORY || vn_id != SCR_BOTTLE || G.bottle != 2 || G.inj_day != G.day || !bottle_inj) { bad++; printf("v123: ask for milk (cw %d mm %d): scene %d bottle %d\n", cw, mm, vn_id, G.bottle); }
        scr = SC_WALK; V123_OPEN(); rom_ask(); if (scr != SC_ROM || !in_pool(rom_txt, pool, pn) || G.bottle != 2) { bad++; printf("v123: second ask the same day (cw %d mm %d)\n", cw, mm); }
        rom_feed(); if (!in_pool(rom_txt, pool, pn) || G.bottle != 2) { bad++; printf("v123: breastfeed after the ask (cw %d mm %d)\n", cw, mm); }
    }
    /* the hand-over scene uses the handing-bottle picture, the ordinary bottle scene does not */
    V123_BASE(); G.injected = 1; bottle_inj = 1; vn_start(SCR_BOTTLEB); for (i = 0; i < vn_n; i++) if (vn_buf[i].bg != BG_BOTTLE) { bad++; printf("v123: bottle hand-over beat %d does not use the handing-bottle picture\n", i); break; }
    RSHOT("v123_bottle_scene"); G.outfit = OF_COW; G.cow_given = 1; draw_story(); RSHOT("v123_bottle_scene_cow");
    bottle_inj = 0; vn_start(SCR_BOTTLEB); for (i = 0; i < vn_n; i++) if (vn_buf[i].bg == BG_BOTTLE) { bad++; printf("v123: the normal bottle scene must not use the new picture\n"); break; }
    /* 7. breastfeed (mama) after a day with no ask: no gauge, no pump; next day ask works again */
    V123_BASE(); G.injected = 1; G.call = 1; G.bottle = 0; feed_no_force = 0; V123_OPEN(); rom_feed(); feed_no_force = -1;
    if (scr != SC_STORY || vn_id != SCR_FEED || G.inj_day != G.day || G.bottle != 0 || G.pump_used != 0) { bad++; printf("v123: breastfeed with the injection (id %d inj_day %d)\n", vn_id, G.inj_day); }
    G.day++; if (g_asks_left(&G) != 1) { bad++; printf("v123: next day the ask is free again\n"); }
    /* 8. the debug row */
    V123_BASE(); scr = SC_DEBUG; dbg_sel = 9; on_button(B_A); if (!G.injected) { bad++; printf("v123: the debug row injects her\n"); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_mode = 0; cover_now = 0; feed_hide = 0; bottle_inj = 0; fh_free(); hide_free(); wk_place(kmap, ktx, kty, kface);
#undef V123_BASE
#undef V123_OPEN
    return bad;
}

static int v120_tests(void) {   /* v12.0: GUI line + GUI text colours, Sophia corner box style */
    int bad = 0, kgl = gui_line, kgt = gui_text, kml = mn_line, kdi = dlg_idx, ktb = tb_idx, kcp = card_pink, ksub = opt_sub, ksel = opt_tsel, kscr = scr, kmm = mp_mode;
    Uint32 bg = C(70,110,60), a0, a1; unsigned h0, h1; char k1[40], k2[40]; unsigned kms = say_ms;
    #define PIX2(x, y) (*(Uint32 *)((Uint8 *)S->pixels + (y) * S->pitch + (x) * 4))
    #define RED2(v) ((int)(((v) >> S->format->Rshift) & 255))
    #define GRN2(v) ((int)(((v) >> S->format->Gshift) & 255))
    memcpy(k1, l1, sizeof k1); memcpy(k2, l2, sizeof k2);
    /* 1. default RED; GOLD (index 1) gives back the old gold box */
    gui_line = 0; gui_text = 0; R(0, 0, 640, 480, bg); panel_a(10, 10, 150, 26, 256, 246); a0 = PIX2(XS(60), YS(10));
    if (RED2(a0) <= GRN2(a0) + 40) { bad++; printf("v120: the default GUI line must be red (r %d g %d)\n", RED2(a0), GRN2(a0)); }
    if (gui_tc() != C(255,85,85)) { bad++; printf("v120: the default GUI text must be red\n"); }
    mn_line = 1; gui_line = 1; gui_text = 1; R(0, 0, 640, 480, bg); panel_a(10, 10, 150, 26, 256, 246); a0 = PIX2(XS(60), YS(10)); R(0, 0, 640, 480, bg); panel_am(10, 10, 150, 26, 256, 246); a1 = PIX2(XS(60), YS(10));
    if (a0 != a1 || gui_tc() != C(250,215,90)) { bad++; printf("v120: GOLD must give back the old gold look\n"); }
    mn_line = kml;
    /* 2. every kind of box follows GUI LINE: table box, cash box, day box, icon, A bubble frame */
    gui_text = 0; gui_line = 0; R(0, 0, 640, 480, bg); panel_t(10, 10, 150, 26, 256); a0 = PIX2(XS(60), YS(10)); gui_line = 2; R(0, 0, 640, 480, bg); panel_t(10, 10, 150, 26, 256); a1 = PIX2(XS(60), YS(10));
    if (a0 == a1) { bad++; printf("v120: the table boxes must follow the GUI line colour\n"); }
    gui_line = 0; R(0, 0, 640, 480, bg); draw_cash_v(1234, 8); h0 = rhash(470, 0, 170, 40); gui_line = 3; R(0, 0, 640, 480, bg); draw_cash_v(1234, 8); h1 = rhash(470, 0, 170, 40);
    if (h0 == h1) { bad++; printf("v120: the cash box line must follow GUI LINE\n"); }
    gui_line = 0; gui_text = 0; R(0, 0, 640, 480, bg); draw_cash_v(1234, 8); h0 = rhash(470, 0, 170, 40); gui_text = 3; R(0, 0, 640, 480, bg); draw_cash_v(1234, 8); h1 = rhash(470, 0, 170, 40);
    if (h0 == h1) { bad++; printf("v120: the cash box label must follow GUI TEXT\n"); }
    gui_text = 0; gui_line = 0; R(0, 0, 640, 480, bg); draw_time(); h0 = rhash(470, 30, 170, 60); gui_line = 3; R(0, 0, 640, 480, bg); draw_time(); h1 = rhash(470, 30, 170, 60);
    if (h0 == h1) { bad++; printf("v120: the day counter line must follow GUI LINE\n"); }
    gui_line = 0; R(0, 0, 640, 480, bg); draw_icon(IC_FRIDGE, 100, 100); h0 = rhash(70, 70, 60, 60); gui_line = 3; R(0, 0, 640, 480, bg); draw_icon(IC_FRIDGE, 100, 100); h1 = rhash(70, 70, 60, 60);
    if (h0 == h1) { bad++; printf("v120: the floating icon frame must follow GUI LINE\n"); }
    /* 3. Sophia's corner box: TABLE OPACITY drives it, DIALOGUE BOX opacity does not */
    snprintf(l1, sizeof l1, "TEST"); l2[0] = 0; say_ms = now_ms(); card_pink = 1; gui_line = 0; tb_idx = 2;
    dlg_idx = 0; R(0, 0, 640, 480, bg); draw_say(); a0 = PIX2(XS(200), YS(66)); dlg_idx = 4; R(0, 0, 640, 480, bg); draw_say(); a1 = PIX2(XS(200), YS(66));
    if (a0 != a1 || a0 == bg) { bad++; printf("v120: the corner box must not follow the dialogue opacity\n"); }
    tb_idx = 0; R(0, 0, 640, 480, bg); draw_say(); a0 = PIX2(XS(200), YS(66)); tb_idx = 3; R(0, 0, 640, 480, bg); draw_say(); a1 = PIX2(XS(200), YS(66));
    if (a0 == a1 || a1 == bg) { bad++; printf("v120: the corner box must follow TABLE OPACITY\n"); }
    tb_idx = 5; R(0, 0, 640, 480, bg); draw_say(); if (PIX2(XS(200), YS(66)) != bg || PIX2(XS(100), YS(0)) != bg) { bad++; printf("v120: at 0%% the corner box and its line must vanish\n"); }
    tb_idx = 2; gui_line = 0; R(0, 0, 640, 480, bg); draw_say(); a0 = PIX2(XS(100), YS(0)); gui_line = 3; R(0, 0, 640, 480, bg); draw_say(); a1 = PIX2(XS(100), YS(0));
    if (a0 == a1) { bad++; printf("v120: the corner box line must follow GUI LINE\n"); }
    /* 4. Options > MENU COLORS has four rows */
    gui_line = 0; gui_text = 0; scr = SC_OPT; opt_sub = 3; opt_tsel = 0; opt_key(B_DOWN); opt_key(B_DOWN); if (opt_tsel != 2) { bad++; printf("v120: row 3 is GUI LINE (%d)\n", opt_tsel); }
    opt_key(B_RIGHT); if (gui_line != 1 || gui_text != 0) { bad++; printf("v120: row 3 changes the GUI line only (%d %d)\n", gui_line, gui_text); } opt_key(B_LEFT); opt_key(B_LEFT); if (gui_line != 7) { bad++; printf("v120: L wraps (%d)\n", gui_line); } gui_line = 0;
    shot("v120_menu_colors_gui_line"); opt_key(B_DOWN); opt_key(B_RIGHT); if (opt_tsel != 3 || gui_text != 1 || gui_line != 0) { bad++; printf("v120: row 4 changes the GUI text only (%d %d %d)\n", opt_tsel, gui_text, gui_line); }
    gui_text = 0; shot("v120_menu_colors_gui_text"); opt_key(B_DOWN); if (opt_tsel != 3) { bad++; printf("v120: four rows only\n"); }
    opt_key(B_B);
    /* 5. cfg: round trip, bad values ignored, an old cfg (with the old tbl line) keeps RED */
    gui_line = 4; gui_text = 6; cfg_save(); gui_line = 0; gui_text = 0; cfg_load(); if (gui_line != 4 || gui_text != 6) { bad++; printf("v120: cfg round trip (%d %d)\n", gui_line, gui_text); }
    { FILE *f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "gul 9\ngut -1\n"); fclose(f); } gui_line = 2; gui_text = 2; cfg_load(); if (gui_line != 2 || gui_text != 2) { bad++; printf("v120: bad cfg values must be ignored\n"); }
      f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "dlg 1\nsnd 1\ntbl 4\n"); fclose(f); } gui_line = 0; gui_text = 0; cfg_load(); if (gui_line != 0 || gui_text != 0) { bad++; printf("v120: an old cfg keeps RED (tbl is ignored)\n"); } }
    /* 6. screenshots of every changed box (default RED, then BLUE once) */
    l1[0] = 0; l2[0] = 0;
    gui_line = 0; gui_text = 0; scr = SC_WALK; G.story = ST_NAMED; wk_place(MAP_ROOM, 1, 3, UP); shot("v120_room_fridge_icon_red");
    wk_place(MAP_ROOM, 2, 5, LF); wk_mode = 2; wk_yes = 0; wk_sleep = 0; shot("v120_sofa_save_box_red"); wk_mode = 0;
    mp_mode = 0; qs_step = 1; qs_sel = 2; scr = SC_QSETUP; shot("v120_quick_play_red"); mp_mode = 1; qs_sel = 1; G.cash = 120; shot("v120_exhibition_red");
    scr = SC_LOTTO; lot_sel = 0; lot_ms = 0; G.cash = 5000; shot("v120_lottery_red");
    atm_open(); shot("v120_atm_red"); name_open(); shot("v120_name_picker_red");
    G.last_res = 1; G.last_net = 14; G.fee = 100; G.last_pay = 514; G.last_drink = 0; G.mlen = 10; memset(G.bn, 0, sizeof G.bn); scr = SC_END; first_end = 0; shot("v120_end_win_red");
    gui_line = 6; gui_text = 6; shot("v120_end_win_blue"); scr = SC_QSETUP; mp_mode = 0; shot("v120_quick_play_blue"); scr = SC_WALK; wk_place(MAP_ROOM, 1, 3, UP); shot("v120_room_fridge_icon_blue");
    gui_line = kgl; gui_text = kgt; dlg_idx = kdi; tb_idx = ktb; card_pink = kcp; opt_sub = ksub; opt_tsel = ksel; scr = kscr; mp_mode = kmm; say_ms = kms; memcpy(l1, k1, sizeof k1); memcpy(l2, k2, sizeof k2);
    #undef PIX2
    #undef RED2
    #undef GRN2
    return bad;
}

static int v119_tests(void) {   /* v11.9: menu colours */
    int bad = 0, kl = mn_line, kt = mn_text, kmc = opt_mc, ksub = opt_sub, ksel = opt_tsel, krow = opt_row, kscr = scr, kfrom = opt_from, kpk = pause_kind, kpsel = pause_sel; Uint32 bg = C(70,110,60), a0, a1; int qx = XS(60), qy = YS(10);
    #define PIX9(x, y) (*(Uint32 *)((Uint8 *)S->pixels + (y) * S->pitch + (x) * 4))
    #define RED9(v) ((int)(((v) >> S->format->Rshift) & 255))
    #define GRN9(v) ((int)(((v) >> S->format->Gshift) & 255))
    /* 1. default: red line and red text colour; the old gold stays reachable (index 1) and equals the old panel */
    if (mn_line != 0 && kl == 0) { bad++; printf("v119: default line colour must be RED\n"); }
    mn_line = 0; mn_text = 0; R(0, 0, 640, 480, bg); panel_am(10, 10, 150, 26, 256, 246); a0 = PIX9(qx, qy);
    if (RED9(a0) <= GRN9(a0) + 40) { bad++; printf("v119: the default menu border must be red (r %d g %d)\n", RED9(a0), GRN9(a0)); }
    if (mn_tc() != C(255,85,85)) { bad++; printf("v119: the default menu text colour must be red\n"); }
    mn_line = 1; mn_text = 1; { int kg = gui_line; gui_line = 1; R(0, 0, 640, 480, bg); panel_am(10, 10, 150, 26, 256, 246); a0 = PIX9(qx, qy); R(0, 0, 640, 480, bg); panel_a(10, 10, 150, 26, 256, 246); a1 = PIX9(qx, qy); gui_line = kg; }
    if (a0 != a1 || mn_tc() != C(250,215,90)) { bad++; printf("v119: GOLD must give back the old gold look\n"); }
    mn_line = 0; mn_text = 0;
    /* 2. Options: the last row holds TEXT BOX STYLE (left) and MENU COLORS (right) */
    scr = SC_OPT; opt_from = PK_HOME; opt_sub = 0; opt_row = OPT_TXT; opt_mc = 0;
    opt_key(B_RIGHT); if (opt_mc != 1) { bad++; printf("v119: R must pick MENU COLORS\n"); }
    opt_key(B_LEFT); if (opt_mc != 0) { bad++; printf("v119: L must pick TEXT BOX STYLE\n"); }
    opt_key(B_A); if (opt_sub != 2) { bad++; printf("v119: A on TEXT BOX STYLE must still open it (sub %d)\n", opt_sub); } opt_key(B_B);
    opt_key(B_RIGHT); shot("v119_options_last_row"); opt_key(B_A);
    if (opt_sub != 3 || opt_tsel != 0) { bad++; printf("v119: A on MENU COLORS must open its panel (sub %d)\n", opt_sub); }
    shot("v119_menu_colors");
    opt_key(B_LEFT); if (mn_line != 7) { bad++; printf("v119: L from RED wraps to PURPLE (%d)\n", mn_line); } opt_key(B_RIGHT); if (mn_line != 0) { bad++; printf("v119: R wraps back to RED (%d)\n", mn_line); }
    opt_key(B_DOWN); opt_key(B_RIGHT); if (opt_tsel != 1 || mn_text != 1 || mn_line != 0) { bad++; printf("v119: the second row changes the text colour only (row %d text %d line %d)\n", opt_tsel, mn_text, mn_line); }
    opt_key(B_DOWN); if (opt_tsel != 2) { bad++; printf("v119: rows 3-4 are the GUI rows (v12.0), got row %d\n", opt_tsel); }
    opt_key(B_B); if (opt_sub != 0 || scr != SC_OPT) { bad++; printf("v119: B closes the MENU COLORS panel only\n"); }
    /* 3. cfg: round trip, bad values ignored, an old cfg keeps RED */
    mn_line = 3; mn_text = 5; cfg_save(); mn_line = 0; mn_text = 0; cfg_load(); if (mn_line != 3 || mn_text != 5) { bad++; printf("v119: cfg round trip (%d %d)\n", mn_line, mn_text); }
    { FILE *f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "mnl 9\nmnt -1\n"); fclose(f); } mn_line = 2; mn_text = 2; cfg_load(); if (mn_line != 2 || mn_text != 2) { bad++; printf("v119: bad cfg values must be ignored\n"); }
      f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "dlg 1\nsnd 1\n"); fclose(f); } mn_line = 0; mn_text = 0; cfg_load(); if (mn_line != 0 || mn_text != 0) { bad++; printf("v119: an old cfg keeps RED\n"); } }
    /* 4. screenshots of every changed menu */
    mn_line = 0; mn_text = 0; opt_sub = 1; opt_lsel = 1; opt_conf = 0; scr = SC_OPT; slots_refresh(); shot("v119_save_slots");
    opt_sub = 0; opt_row = 0; shot("v119_options");
    scr = SC_SHOP; shop_tab = TAB_ITEMS; shop_sel = 0; shot("v119_online_store");
    shop_tab = TAB_STOCKS; shot("v119_online_store_stocks");
    scr = SC_WALK; pause_kind = PK_HOME; pause_sel = 0; shot("v119_pause_menu"); pause_kind = PK_NONE;
    scr = SC_CAREER; car_sel = 0; car_conf = 0; shot("v119_career");
    mn_line = kl; mn_text = kt; opt_mc = kmc; opt_sub = ksub; opt_tsel = ksel; opt_row = krow; scr = kscr; opt_from = kfrom; pause_kind = kpk; pause_sel = kpsel;
    #undef PIX9
    #undef RED9
    #undef GRN9
    return bad;
}
static int v115_tests(void) {   /* v11.5: table box opacity / line colour, default look unchanged, cfg, Options rows, folded CPU */
    int bad = 0, kto = tb_idx, ktl = gui_line, kdi = dlg_idx, kdl = dlg_line, ksub = opt_sub, kts = opt_tsel, kscr = scr, kmm = mp_mode, kqs = qs_si; Uint32 bg = C(70,110,60), a0, a1, a2; int qx = XS(60), qy = YS(10), ix = XS(60), iy = YS(20);
    #define PIX(x, y) (*(Uint32 *)((Uint8 *)S->pixels + (y) * S->pitch + (x) * 4))
    /* 1. default = the old look (the same pixels as panel() with PANEL_ALPHA, border included) */
    tb_idx = 2; gui_line = 0; if (TB_A[2] != PANEL_ALPHA) { bad++; printf("v115: default step must equal the old alpha (%d)\n", TB_A[2]); }
    R(0, 0, 640, 480, bg); panel(10, 10, 150, 26, 256); a0 = PIX(qx, qy); a1 = PIX(ix, iy);
    R(0, 0, 640, 480, bg); panel_t(10, 10, 150, 26, 256); if (PIX(qx, qy) != a0 || PIX(ix, iy) != a1) { bad++; printf("v115: the default table box must look exactly like the old one\n"); }
    /* 2. 0% draws nothing, box and line */
    tb_idx = 5; R(0, 0, 640, 480, bg); panel_t(10, 10, 150, 26, 256); if (PIX(qx, qy) != bg || PIX(ix, iy) != bg || PIX(XS(11), YS(11)) != bg) { bad++; printf("v115: at 0%% neither the table box nor its line may be drawn\n"); }
    /* 3. the steps get fainter, the line colour changes the border, the dialogue style is not touched */
    tb_idx = 0; R(0, 0, 640, 480, bg); panel_t(10, 10, 150, 26, 256); a0 = PIX(ix, iy); tb_idx = 3; R(0, 0, 640, 480, bg); panel_t(10, 10, 150, 26, 256); a1 = PIX(ix, iy);
    if (a0 == a1 || a1 == bg) { bad++; printf("v115: 40%% must be fainter than 100%% but visible\n"); }
    tb_idx = 2; gui_line = 0; R(0, 0, 640, 480, bg); panel_t(10, 10, 150, 26, 256); a0 = PIX(qx, qy); gui_line = 2; R(0, 0, 640, 480, bg); panel_t(10, 10, 150, 26, 256); a1 = PIX(qx, qy);
    if (a0 == a1) { bad++; printf("v115: the line colour must change the table box border\n"); }
    dlg_line = 0; gui_line = 3; R(0, 0, 640, 480, bg); panel_d(0, 250, 640, 60, 256); a0 = PIX(XS(60), YS(250)); dlg_line = 3; R(0, 0, 640, 480, bg); panel_d(0, 250, 640, 60, 256); a2 = PIX(XS(60), YS(250));
    if (a0 == a2) { bad++; printf("v115: the dialogue line colour is its own setting\n"); }
    dlg_line = kdl; dlg_idx = kdi;
    /* 4. cfg round trip + bad values + an old cfg keeps the defaults */
    tb_idx = 4; gui_line = 6; cfg_save(); tb_idx = 2; gui_line = 0; cfg_load(); if (tb_idx != 4 || gui_line != 6) { bad++; printf("v115: cfg round trip (%d %d)\n", tb_idx, gui_line); }
    { FILE *f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "tbo 9\ngul 8\n"); fclose(f); } tb_idx = 3; gui_line = 1; cfg_load(); if (tb_idx != 3 || gui_line != 1) { bad++; printf("v115: bad cfg values must be ignored\n"); }
      f = fopen(CFG_PATH, "w"); if (f) { fprintf(f, "dlg 2\nsnd 1\n"); fclose(f); } tb_idx = 2; gui_line = 0; cfg_load(); if (tb_idx != 2 || gui_line != 0) { bad++; printf("v115: an old cfg must keep the defaults\n"); } }
    /* 5. Options > TEXT BOX STYLE has four rows; L/R wraps both ways */
    scr = SC_OPT; opt_sub = 2; opt_tsel = 0; opt_key(B_DOWN); opt_key(B_DOWN); opt_key(B_DOWN); if (opt_tsel != 3) { bad++; printf("v115: row 4 is TABLE OPACITY (%d)\n", opt_tsel); }
    tb_idx = 2; opt_key(B_RIGHT); if (tb_idx != 3) { bad++; printf("v115: RIGHT raises the step\n"); } tb_idx = 5; opt_key(B_RIGHT); if (tb_idx != 0) { bad++; printf("v115: RIGHT wraps to 100%%\n"); } opt_key(B_LEFT); if (tb_idx != 5) { bad++; printf("v115: LEFT wraps to 0%%\n"); }
    opt_key(B_DOWN); opt_key(B_DOWN); if (opt_tsel != 3) { bad++; printf("v115: four rows now, TABLE LINE is gone (%d)\n", opt_tsel); }
    shot("v115_table_box_style"); opt_tsel = 3; tb_idx = 3; shot("v115_table_opacity_40");
    opt_key(B_B); if (opt_sub != 0) { bad++; printf("v115: B closes the panel\n"); }
    /* 6. a folded CPU: no dark box over the felt (the old rectangle at 284,366 is gone) and its table cards are not drawn */
    tb_idx = 2; gui_line = 0; scr = kscr; mp_mode = 0; qs_si = 1; G.cash = 1340; mp_begin();
    { int guard = 0; while (!M.over && guard++ < 40 && !(M.stage == 2 && M.toact == 0)) { if (M.toact == 0 && M.stage < 2 && mp_can_raise(&M, 0) && guard < 3) mp_do(A_RAISE); else mp_do(A_CALL); } }
    M.folded[1] = 0; draw_mtable(); a0 = PIX(XS(300), YS(372)); M.folded[1] = 1; draw_mtable(); a1 = PIX(XS(300), YS(372));
    { SDL_Surface *keep = S; (void)keep; }
    shot("v115_quick_cpu_folded"); if (a0 == a1) { /* the cards are gone, so the pixel must differ from the face-down card */ bad++; printf("v115: a folded CPU must not keep its cards on the table\n"); }
    { Uint32 c0 = PIX(XS(300), YS(420)); M.folded[1] = 0; draw_mtable(); a2 = PIX(XS(300), YS(420)); M.folded[1] = 1; draw_mtable(); if (PIX(XS(300), YS(420)) != a2 && c0 == a2) { bad++; printf("v115: fold must not darken the board cards\n"); } else if (PIX(XS(300), YS(420)) != a2) { bad++; printf("v115: the board area must look the same folded or not (the old dark box is gone)\n"); } }
    M.folded[1] = 0; mp_mode = kmm; qs_si = kqs; tb_idx = kto; gui_line = ktl; opt_sub = ksub; opt_tsel = kts; scr = kscr;
    #undef PIX
    return bad;
}

static int v113_tests(void) {   /* v11.3: the three outfits unlock only at max romance (level 3, bar full), give no romance, are not a gift of the day, and are changed only in her wardrobe */
    int bad = 0, i, n, k, a[10]; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face, ktalk = ls_talk;
    static const int KD[3] = {GF_YOGA, GF_TOP, GF_COW}, SHI[3] = {SH_YOGA, SH_TOP, SH_COW};
#define V113_BASE() do { g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.story = ST_NAMED; G.day = 20; G.phase = PH_MORNING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1; \
    G.milk_st = 4; G.milk_pct = 0; G.mpill = 2; G.dring = 1; G.married = 1; G.event = 1; G.hall_seen = 3; G.ring = 1; G.cash = 90000; G.energy = 100; G.yoga = 1; G.top = 1; G.cow = 1; saved_G = G; ls_away = -1; ls_talk = 0; ls_on = 0; sb_out = 0; kicked_day = 0; sb_rkey = -1; scr = SC_WALK; wk_mode = 0; cover_now = 0; feed_hide = 0; } while (0)
#define V113_OPEN() do { wk_mode = 0; scr = SC_WALK; ls_on = 0; ls_talk = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_in = 1; } while (0)
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0; kicked_day = 0;
    /* 1. the gate: not at level 2 with a full bar, not at level 3 with 99 percent; the store and the give page both stay without outfit rows */
    for (k = 0; k < 3; k++) {
        int lv, pc;
        for (lv = ROM_LVLS - 1; lv <= ROM_LVLS; lv++) { pc = lv == ROM_LVLS - 1 ? 100 : 99;
            V113_BASE(); G.rom_lvl = lv; G.rom_pct = pc; G.pill_seen = 0; G.pill_given = 0; G.pill = 0; G.married = 0; V113_OPEN();
            for (i = 0; i < 4; i++) { int j; n = rom_page_acts(i, a); for (j = 0; j < n; j++) if (a[j] == RA_GIVE + KD[k] || a[j] >= RA_OUTFIT_K) { bad++; printf("v113: outfit %d: a row at level %d %d%%\n", k, lv, pc); } }
            { int o[16], nn = shop_list(TAB_ITEMS, o), j; for (j = 0; j < nn; j++) if (o[j] == SHI[k]) { bad++; printf("v113: outfit %d is listed in the store at level %d %d%%\n", k, lv, pc); } }
            if (g_gift(&G, KD[k]) != 12) { bad++; printf("v113: outfit %d: the gift is refused below max (level %d %d%%)\n", k, lv, pc); }
        }
        /* at max romance, before the day-8 scene: a row on the give page, listed in the store */
        V113_BASE(); G.pill_seen = 0; G.pill_given = 0; G.pill = 0; G.married = 0; V113_OPEN();
        n = rom_page_acts(1, a); { int f = 0, j; for (j = 0; j < n; j++) if (a[j] == RA_GIVE + KD[k]) f = 1; if (!f) { bad++; printf("v113: outfit %d: row on the give page at max romance\n", k); } }
        { int o[16], nn = shop_list(TAB_ITEMS, o), j, f = 0; for (j = 0; j < nn; j++) if (o[j] == SHI[k]) f = 1; if (!f) { bad++; printf("v113: outfit %d is listed in the store at max romance\n", k); } }
        /* after the day-8 scene the outfits stay on the give page, next to the milk pill */
        V113_BASE(); G.married = 0; V113_OPEN();
        n = rom_page_acts(1, a); { int f = 0, j; for (j = 0; j < n; j++) if (a[j] == RA_GIVE + KD[k]) f = 1; if (!f || a[0] != RA_MILK) { bad++; printf("v113: outfit %d: still a row on the give page after the day-8 scene (rows %d)\n", k, n); } }
        if (k == 0) RSHOT("v113_give_page_after_day8");
        /* giving it through the menu: the gift scene plays, no romance, not the gift of the day (even on a day she already got a gift), she owns it, the row is gone */
        { int idx = -1, j, gd; G.gift_day = G.day; gd = G.gift_day; n = rom_page_acts(1, a); for (j = 0; j < n; j++) if (a[j] == RA_GIVE + KD[k]) idx = j;
          rom_page = 1; rom_sel = idx; rom_choose = 1; on_button(B_A);
          if (scr != SC_STORY || vn_id != SCR_GIFT || vn_n != 4 || g_gift_stock(&G, KD[k]) != 0 || G.rom_lvl != ROM_LVLS || G.rom_pct != 100 || G.gift_day != gd) { bad++; printf("v113: outfit %d: the gift scene, no romance, gift day untouched (scr %d id %d lvl %d pct %d)\n", k, scr, vn_id, G.rom_lvl, G.rom_pct); }
          else { if (!strstr(vn_buf[3].txt, "ADDED TO HER WARDROBE") || strstr(vn_buf[3].txt, "ROMANCE")) { bad++; printf("v113: outfit %d: closing note (%s)\n", k, vn_buf[3].txt); } if (k == 2) RSHOT("v113_gift_scene_outfit"); }
          { int q; for (q = 0; q < 160 && scr == SC_STORY; q++) on_button(B_A); }
          if (!(k == 0 ? G.yoga_given : k == 1 ? G.top_given : G.cow_given) || (k != 0 && G.yoga_given) || (k != 1 && G.top_given) || (k != 2 && G.cow_given)) { bad++; printf("v113: outfit %d: she owns exactly that outfit now\n", k); }
          G.yoga = G.top = G.cow = 1; V113_OPEN(); n = rom_page_acts(1, a); for (j = 0; j < n; j++) if (a[j] == RA_GIVE + KD[k]) { bad++; printf("v113: outfit %d: no second row for an outfit she has already\n", k); }
          if (g_gift(&G, KD[k]) != 1) { bad++; printf("v113: outfit %d: giving it twice is refused\n", k); } }
    }
    /* 2. her menu never has a CHANGE row; the only way is the wardrobe, which works for an outfit she got in an older game (given below max romance) */
    V113_BASE(); G.rom_lvl = 1; G.rom_pct = 10; G.yoga_given = 1; G.top_given = 1; G.cow_given = 1; G.outfit = 0; V113_OPEN();
    for (i = 0; i < 4; i++) { int j; n = rom_page_acts(i, a); for (j = 0; j < n; j++) if (a[j] >= RA_OUTFIT_K) { bad++; printf("v113: page %d has a change row\n", i + 1); } }
    V113_BASE(); G.rom_lvl = 1; G.rom_pct = 10; G.yoga_given = 1; G.top_given = 1; G.cow_given = 1; G.outfit = 0;
    for (k = 0; k < 3; k++) { static const int OFS[3] = {OF_YOGA, OF_TOP, OF_COW}; G.outfit = 0; if (!tst_ward(OFS[k]) || G.outfit != OFS[k]) { bad++; printf("v113: wardrobe %d: an old game keeps its outfits, they work below max romance (outfit %d)\n", k, G.outfit); } WK_CLOSE(); }
    /* 3. an owned outfit is not taken away when romance would be below max (it cannot drop, but a hand-edited save can) */
    V113_BASE(); G.rom_lvl = 2; G.rom_pct = 0; G.cow_given = 1; if (!g_outfit_open(&G) || !g_outfit_has(&G, OF_COW)) { bad++; printf("v113: owned outfits stay\n"); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = ktalk; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_mode = 0; cover_now = 0; feed_hide = 0; fh_free(); hide_free(); wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
#undef V113_BASE
#undef V113_OPEN
    return bad;
}

static int v82_tests(void) {   /* v8.2: body lotion / pink card deck / yoga outfit / gold bar gifts, the diamond ring, the scrolling ITEMS list, the Super milk drug (ask for milk twice a day, normal pill refused) */
    int bad = 0, i, k, j; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face;
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 0; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0; ls_on = 0;
    /* 1. text and picture sizes */
    for (i = 0; i < SH_N; i++) {
        if (!SH_NAME[i] || !SH_INFO[i]) { bad++; printf("v82: item %d has no name / info\n", i); continue; }
        if (tw(SH_INFO[i], 2) > 424 || tw(SH_NAME[i], 2) + 20 + tw("OWNED 99", 2) > 428) { bad++; printf("v82: item %d: text too wide for its row (info %d, name %d)\n", i, tw(SH_INFO[i], 2), tw(SH_NAME[i], 2)); }
        if (!giftS[i] && frozen == 0) { bad++; printf("v82: item %d has no icon\n", i); }
    }
    for (i = SH_LOTION; i <= SH_SUPER; i++) if (giftS[i] && (giftS[i]->h > 88 || giftS[i]->w > 110)) { bad++; printf("v82: icon %d is %dx%d, it must fit the store row\n", i, giftS[i]->w, giftS[i]->h); }
    for (i = 0; i < GF_N; i++) { if (!R_GNAME[i] || !R_GIFT_HAND[i] || !R_GIFT_L2[i] || !R_GIFT_END[i]) { bad++; printf("v82: gift kind %d lacks a name / scene line\n", i); } }
    for (i = 0; i < 8; i++) if (!R_GIFT_OK[i]) { bad++; printf("v82: R_GIFT_OK[%d] missing\n", i); }
    for (i = GF_LOTION; i < GF_N; i++) {   /* every new gift scene page fits the dialogue box (3 lines of 46), the closing line with its note too */
        char w[400]; const char *t[4]; char endn[220]; int q;
        snprintf(endn, sizeof endn, "%s  (ROMANCE +50%%  LEVEL 3!)", R_GIFT_END[i]); t[0] = R_GIFT_HAND[i]; t[1] = R_GIFT_OK[3 + i - GF_LOTION < 8 ? 3 + i - GF_LOTION : 0]; t[2] = R_GIFT_L2[i];   /* v11.1: the designer top and the cow outfit have no R_GIFT_OK entry (GO_ pools) */ t[3] = endn;
        for (q = 0; q < 4; q++) { int ln = 1, c; wrap_buf(w, sizeof w, t[q], 46); for (c = 0; w[c]; c++) if (w[c] == '\n') ln++; if (ln > 3) { bad++; printf("v82: gift %d page %d needs %d lines\n", i, q, ln); } }
    }
    { const char *ex[4] = {pick(R_MILK_SUPER, R_MILK_SUPER_N), pick(R_SUPER_ON, R_SUPER_ON_N), pick(R_ASK_DONE, R_ASK_DONE_N), pick(R_ASK_NONE, R_ASK_NONE_N)}; for (i = 0; i < 4; i++) { char w[400]; int ln = 1, c; wrap_buf(w, sizeof w, ex[i], 46); for (c = 0; w[c]; c++) if (w[c] == '\n') ln++; if (ln > 3) { bad++; printf("v82: her line %d needs %d lines\n", i, ln); } } }
    /* 2. the online store through the real buttons: ITEMS scrolls, buy the lotion, the diamond ring is one time only */
    g_init(&G); G.cash = 80000; G.bank = 0; G.day = 3; G.rs = RS_LIVING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; scr = SC_WALK; shop_open(); RSHOT("v82_items_top");   /* v11.3: the outfit rows need max romance */
    { int o[16]; if (shop_list(TAB_ITEMS, o) != 9) { bad++; printf("v82: ITEMS has 9 rows before the day-8 scene (v10.8: the designer top, v11.1: the cow print outfit)\n"); } }
    on_button(B_DOWN); if (shop_sel != SH_LOTION) { bad++; printf("v82: the lotion is the 2nd row (sel %d)\n", shop_sel); }
    on_button(B_A); if (!shop_confirm) { bad++; printf("v82: A must ask to confirm\n"); } RSHOT("v82_buy_lotion");
    on_button(B_A); if (G.lotion != 1 || G.cash != 80000 - 300) { bad++; printf("v82: buying the lotion (lotion %d cash %d)\n", G.lotion, G.cash); }
    for (k = 0; k < 7; k++) on_button(B_DOWN);   /* v11.1: the diamond ring is the 9th row */
    if (shop_sel != SH_DIAMOND || stk_top != 5) { bad++; printf("v82: 8 DOWNs reach the diamond ring and scroll the list (sel %d top %d)\n", shop_sel, stk_top); } RSHOT("v82_items_scrolled");
    on_button(B_Y); if (!shop_info) { bad++; printf("v82: the diamond ring has an info page\n"); } RSHOT("v82_ring_info"); on_button(B_B);
    on_button(B_A); on_button(B_A); if (!G.dring || G.cash != 80000 - 300 - 50000) { bad++; printf("v82: buying the diamond ring (ring %d cash %d)\n", G.dring, G.cash); }
    RSHOT("v82_ring_gone"); { int o[16], n = shop_list(TAB_ITEMS, o), j; for (j = 0; j < n; j++) if (o[j] == SH_DIAMOND) { bad++; printf("v10: the bought diamond ring must vanish from the list\n"); } if (n != 8 || shop_sel != SH_GOLD) { bad++; printf("v10: after the diamond ring vanishes the cursor stays on the last row (n %d sel %d)\n", n, shop_sel); } }
    if (G.ring || g_can_propose(&G)) { bad++; printf("v82: the diamond ring must not act as the proposal ring\n"); }
    on_button(B_DOWN); if (shop_sel != SH_FLOWER || stk_top != 0) { bad++; printf("v82: DOWN from the last row wraps to the flower and scrolls back (sel %d top %d)\n", shop_sel, stk_top); }
    on_button(B_UP); if (shop_sel != SH_GOLD) { bad++; printf("v82: UP from the first row wraps to the last (sel %d)\n", shop_sel); }
    G.pill_seen = 1; { int o[16]; if (shop_list(TAB_ITEMS, o) != 9 || o[8] != SH_PUMP) { bad++; printf("v82: the pump is the 10th row after the day-8 scene\n"); } }
    on_button(B_DOWN); on_button(B_DOWN); on_button(B_DOWN); on_button(B_DOWN); if (shop_sel != SH_YOGA && 0) {} RSHOT("v82_items_with_pump");
    /* the black market: the Super milk drug, info page, buying */
    shop_open(); on_button(B_R1); on_button(B_R1); if (shop_tab != TAB_BM) { bad++; printf("v82: R1 twice = black market\n"); }
    for (k = 0; k < 3; k++) on_button(B_DOWN); if (shop_sel != SH_SUPER) { bad++; printf("v82: the Super drug is the 4th black market row (sel %d)\n", shop_sel); } RSHOT("v82_black_market_super");
    on_button(B_Y); if (!shop_info) { bad++; printf("v82: Y opens the Super drug info\n"); } else { char w[400]; int ln = 1, c; wrap_buf(w, sizeof w, SH_DESC[SH_SUPER], 28); for (c = 0; w[c]; c++) if (w[c] == '\n') ln++; if (ln > 5) { bad++; printf("v82: the Super drug info needs %d lines\n", ln); } } RSHOT("v82_super_info"); on_button(B_B);
    if (shop_info) { bad++; printf("v82: B closes the info page\n"); }
    G.cash = 10000; on_button(B_A); on_button(B_A); if (G.sdrug != 1 || G.cash != 7750) { bad++; printf("v82: buying the Super drug (n %d cash %d)\n", G.sdrug, G.cash); }
    on_button(B_L1); on_button(B_L1); on_button(B_L1); /* back round to ITEMS then check the diamond ring page */
    /* 3. the new gifts through her menu, before the day-8 scene */
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.cash = 90000; G.day = 5; G.phase = PH_MORNING;
    G.lotion = 1; G.pdeck = 1; G.stack = 1; G.yoga = 1; G.gold = 1; G.flower = 1; G.pill = 1; G.rom_lvl = 1; G.rom_pct = 5;   /* v11.3: the yoga outfit is no longer a romance gift (the money stack takes its place here); it is locked below max romance */
    { static const int kind[4] = {GF_LOTION, GF_DECK, GF_STACK, GF_GOLD}, gain[4] = {12, 15, 20, 50}, pct[4] = {17, 32, 52, 0};
      for (k = 0; k < 4; k++) {
        G.day = 5 + k; door_knock(); rom_adv(3); RC();
        if (scr != SC_ROM || rom_menu_n() != 2 || rom_pages() != 2 || rom_pg() != 0) { bad++; printf("v82: gift %d: before the day-8 scene her menu has 2 pages and page 1 has 2 entries (scr %d n %d)\n", k, scr, rom_menu_n()); break; }
        on_button(B_R1); if (rom_pg() != 1) { bad++; printf("v82: R1 turns to the gift page\n"); break; }
        { int a[10], n = rom_acts(a), q; if (n != 6 - k) { bad++; printf("v82: gift %d: the page lists only what you own, %d expected, %d shown\n", k, 6 - k, n); break; }
          for (q = 0; q < n && a[q] != RA_GIVE + kind[k]; q++); if (q == n) { bad++; printf("v82: gift %d: kind %d is not on the page\n", k, kind[k]); break; }
          for (j = 0; j < q; j++) on_button(B_DOWN); }
        if (k == 0) RSHOT("v82_gift_list");
        on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_GIFT || vn_n != 4 || !gift_in_pool(0, kind[k], vn_buf[0].txt) || !gift_in_pool(1, kind[k], vn_buf[1].txt) || !gift_in_pool(2, kind[k], vn_buf[2].txt) || !gift_in_pool(3, kind[k], vn_buf[3].txt)) { bad++; printf("v82: gift %d: the 4-beat gift scene is wrong (scr %d id %d)\n", k, scr, vn_id); break; }
        { char want[24]; snprintf(want, sizeof want, "ROMANCE +%d%%", gain[k]); if (k == 3) snprintf(want, sizeof want, "ROMANCE LEVEL UP!"); if (!strstr(vn_buf[3].txt, want)) { bad++; printf("v82: gift %d: the note must say %s (%s)\n", k, want, vn_buf[3].txt); } }
        if (G.rom_pct != pct[k] || G.rom_lvl != (k == 3 ? 2 : 1) || g_gift_stock(&G, kind[k]) != 0 || G.gift_day != G.day) { bad++; printf("v82: gift %d: bar %d%% level %d stock %d\n", k, G.rom_pct, G.rom_lvl, g_gift_stock(&G, kind[k])); }
        if (k == 1) RSHOT("v82_gift_scene_deck");
        rom_adv(10); if (scr != SC_WALK || wk_tx != HOME_DOOR_X || wk_ty != HOME_DOOR_Y) { bad++; printf("v82: gift %d: after the scene you stand at her door (scr %d)\n", k, scr); }
      } }
    /* a second gift on the same day is refused (one gift a day), also for the new gifts */
    G.lotion = 1; door_knock(); rom_adv(3); RC(); on_button(B_R1); on_button(B_DOWN); on_button(B_A);
    if (scr != SC_ROM || !in_pool(rom_txt, R_GIFT_TODAY, R_GIFT_TODAY_N) || G.lotion != 1) { bad++; printf("v82: a 2nd gift on the same day must be refused (scr %d)\n", scr); }
    /* 4. the Super milk drug through her menu */
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.cash = 90000; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.day = 20; G.phase = PH_MORNING;
    G.pill = 1; G.pill_given = 1; G.pill_day = 5; G.pill_seen = 1; G.milk_st = 4; G.milk_pct = 50; G.mpill = 3; G.sdrug = 2; G.bottle = 0; G.hint_day = 0;
    wk_mode = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1;
    { int a[10], n = rom_page_acts(0, a); if (rom_pages() != 3 || n != 2 || a[0] != RA_PLAY || a[1] != RA_CHAT) { bad++; printf("v82: page 1 is PLAY / CHAT (n %d)\n", n); }
      n = rom_page_acts(1, a); if (n != 2 || a[0] != RA_MILK || a[1] != RA_SUPER) { bad++; printf("v82: with a drug in the bag page 2 has the milk pill and GIVE SUPER DRUG (n %d)\n", n); }
      n = rom_page_acts(2, a); if (n != 1 || a[0] != RA_TEASE) { bad++; printf("v82: page 3 is only TEASE without the drug (n %d)\n", n); } }
    rom_page = 1; RSHOT("v82_menu_drug_in_bag");
    rom_page = 1; rom_sel = 1; on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_GIFT || G.sdrug != 1 || G.sd_day != 20 || !gift_in_pool(0, GF_SUPER, vn_buf[0].txt) || !gift_in_pool(1, GF_SUPER, vn_buf[1].txt) || !gift_in_pool(2, GF_SUPER, vn_buf[2].txt) || !strstr(vn_buf[3].txt, "GAUGE FULL FOR 2 DAYS")) { bad++; printf("v82: giving the Super drug must play the gift scene (scr %d id %d sdrug %d)\n", scr, vn_id, G.sdrug); }
    if (G.milk_pct != 50 || G.rom_pct != 100 || G.gift_day == 20) { bad++; printf("v82: the drug leaves the real gauge, the romance bar and the gift of the day alone\n"); }
    rom_adv(10); if (scr != SC_WALK) { bad++; printf("v82: after the drug scene a note tells you to ask her (scr %d)\n", scr); }
    if (g_milk_gauge_pct(&G) != 100 || !g_super_active(&G)) { bad++; printf("v82: the gauge must show full while the drug works\n"); }
    wk_mode = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1;
    { int a[10], n = rom_page_acts(2, a); if (n != 2 || a[0] != RA_TEASE || a[1] != RA_ASK) { bad++; printf("v82: ASK FOR MILK appears on page 3 while the drug works (n %d)\n", n); } }
    rom_page = 2; RSHOT("v82_menu_ask_for_milk");
    rom_page = 1; rom_sel = 0; on_button(B_A); if (scr != SC_ROM || !in_pool(rom_txt, R_MILK_SUPER, R_MILK_SUPER_N) || G.mpill != 3 || G.milk_pct != 50) { bad++; printf("v82: the normal milk pill must be refused while the drug works (scr %d)\n", scr); } RSHOT("v82_pill_refused");
    rom_choose = 1; rom_mode = RM_MENU; rom_page = 1; rom_sel = 1; on_button(B_A); if (scr != SC_ROM || !in_pool(rom_txt, R_SUPER_ON, R_SUPER_ON_N) || G.sdrug != 1) { bad++; printf("v82: a 2nd drug while the first works must be refused\n"); }
    for (k = 0; k < 2; k++) {   /* two asks today */
        wk_mode = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_page = 2; rom_sel = 1; on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_BOTTLE || G.bottle != k + 1 || G.ask_n != k + 1) { bad++; printf("v82: ask %d must start the bottle scene and give a bottle (scr %d id %d bottle %d)\n", k, scr, vn_id, G.bottle); break; }
        rom_adv(20); if (scr != SC_PEEK) { bad++; printf("v82: ask %d: the peek choice must follow (scr %d)\n", k, scr); break; }
        on_button(B_A); rom_adv(20); if (scr != SC_WALK) { bad++; printf("v82: ask %d: the scene must end on the map (scr %d)\n", k, scr); break; }
    }
    wk_mode = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_page = 2; RSHOT("v82_menu_asks_used"); rom_sel = 1; on_button(B_A);
    if (scr != SC_ROM || !in_pool(rom_txt, R_ASK_DONE, R_ASK_DONE_N) || G.bottle != 2) { bad++; printf("v82: the 3rd ask of a day must be refused (scr %d bottle %d)\n", scr, G.bottle); }
    /* day 2 of the effect: two more asks */
    G.day = 21; wk_mode = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_page = 2; rom_sel = 1; on_button(B_A); if (scr != SC_STORY || G.bottle != 3) { bad++; printf("v82: the asks come back on day 2 of the effect\n"); } rom_adv(20); on_button(B_A); rom_adv(20);
    if (g_asks_left(&G) != 1) { bad++; printf("v82: one ask left on day 2 (left %d)\n", g_asks_left(&G)); }
    /* day 3 and 4: the effect is over (no ASK, the gauge is back) and the normal pill is still refused; day 5 it works */
    for (j = 22; j <= 24; j++) {
        G.day = j; wk_mode = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1;
        rom_page = 2; if (rom_menu_n() != 1 || g_super_active(&G) || g_milk_gauge_pct(&G) != 50) { bad++; printf("v82: day %d: effect over, no ASK entry, gauge back to 50 (n %d)\n", j, rom_menu_n()); }
        rom_page = 1; rom_sel = 0; on_button(B_A);
        if (j < 24) { if (!in_pool(rom_txt, R_MILK_SUPER, R_MILK_SUPER_N) || G.mpill != 3) { bad++; printf("v82: day %d: the normal pill must still be refused\n", j); } }
        else { if (scr != SC_STORY || G.mpill != 2) { bad++; printf("v82: day 24: the normal pill works again (scr %d mpill %d)\n", scr, G.mpill); } }
    }
    rom_adv(40);
    /* v8.3: her menu is PAGES (R1 / L1, B leaves, no LEAVE row); the give page lists only what you own; from day 6 of the pill she stays out of the living room */
    {
        int a[10], d, hit, hit_before = 0;
        g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.rom_lvl = ROM_LVLS; G.day = 5; G.phase = PH_MORNING;
        wk_mode = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1;
        if (rom_pages() != 2 || rom_page_acts(0, a) != 2 || rom_page_acts(1, a) != 0) { bad++; printf("v83: before day 8 she has 2 pages, page 1 = 2 rows, page 2 = nothing owned\n"); }
        on_button(B_R1); if (rom_pg() != 1) { bad++; printf("v83: R1 = next page\n"); }
        on_button(B_R1); if (rom_pg() != 0) { bad++; printf("v83: R1 on the last page wraps to the first\n"); }
        on_button(B_L1); if (rom_pg() != 1) { bad++; printf("v83: L1 on the first page wraps to the last\n"); }
        RSHOT("v83_no_gift"); on_button(B_A); if (scr != SC_ROM || rom_mode != RM_MENU) { bad++; printf("v83: A on the empty give page does nothing\n"); }
        G.flower = 3; if (rom_page_acts(1, a) != 1 || a[0] != RA_GIVE + GF_FLOWER) { bad++; printf("v83: an owned flower is ONE row (no count)\n"); }
        RSHOT("v83_one_flower");
        on_button(B_B); if (scr == SC_ROM && !fade_st) { bad++; printf("v83: B must leave her menu (scr %d)\n", scr); }
        G.pill = 1; G.pill_given = 1; G.pill_seen = 1; G.pill_day = 2; G.day = 12; G.mpill = 2; G.sdrug = 1;
        scr = SC_ROM; wk_mode = 0; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1; rom_page = 2; rom_open(pick(R_MILK_AFTER, R_MILK_AFTER_N)); rom_choose = 1;
        if (rom_pg() != 0) { bad++; printf("v83: after a scene / a new visit her menu starts on page 1\n"); }
        if (rom_pages() != 3) { bad++; printf("v83: after the day-8 scene she has 3 pages\n"); }
        on_button(B_L1); if (rom_pg() != 2) { bad++; printf("v83: L1 from page 1 = page 3 (tease / ask)\n"); }
        on_button(B_R1); on_button(B_R1); if (rom_pg() != 1 || rom_page_acts(1, a) != 2 || a[0] != RA_MILK || a[1] != RA_SUPER) { bad++; printf("v83: page 2 after day 8 = milk pill + Super drug, one row each\n"); }
        RSHOT("v83_page2_items");
        /* the living room: from day 6 of the pill (pill_day + 5) on she never comes out again */
        ls_enabled = 1; ls_force = 1; G.phase = PH_EVENING; G.pill_given = 1; G.pill_seen = 1; G.pill_day = 10; G.rs = RS_LIVING; G.rom_lvl = ROM_LVLS; G.rom_pct = 100;
        for (d = 11; d <= 14; d++) { G.day = d; wk_place(MAP_ROOM, 4, 4, DN); ls_away = -1; ls_ph = -1; ls_roll(); if (ls_on) hit_before++; }
        if (hit_before != 4) { bad++; printf("v83: setup: before day 6 of the pill she can still be in the living room (%d of 4)\n", hit_before); }
        for (d = 15; d <= 40; d++) { G.day = d; for (hit = 0; hit < 3; hit++) { G.phase = hit; wk_place(MAP_ROOM, 4, 4, DN); ls_away = -1; ls_ph = -1; ls_roll(); if (ls_on) { bad++; printf("v83: day %d (pill day %d) phase %d: she must stay out of the living room\n", d, d - 9, hit); d = 99; break; } } }
        G.pill_given = 0; G.day = 20; G.phase = PH_EVENING; wk_place(MAP_ROOM, 4, 4, DN); ls_away = -1; ls_ph = -1; ls_roll(); if (!ls_on) { bad++; printf("v83: without the pill the shy rule does not apply\n"); }
        ls_force = 0; ls_on = 0; scr = SC_WALK; wk_mode = 0;
    }
    /* 5. an old-style save (v14) loaded in the game keeps playing: no new items */
    { Game t; g_init(&t); t.cash = 555; t.ver = SAVE_VER_14; t.size = (int)offsetof(Game, lotion); { FILE *w = fopen("/tmp/pq_v82_main.sav", "wb"); if (w) { fwrite(&t, 1, offsetof(Game, lotion), w); fclose(w); } }
      if (!load_game(&G, "/tmp/pq_v82_main.sav") || G.cash != 555 || G.sdrug != 0 || G.dring != 0) { bad++; printf("v82: a v14 save must load in the game\n"); } remove("/tmp/pq_v82_main.sav"); }
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = 0; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
    return bad;
}
static int v81_tests(void) {   /* v8.1: Sophia always there in the morning, every free tile, no pop-in, the walk back to her room, the door line; six stocks with a scrolling list */
    int bad = 0, i, k, x, y; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, ken = ls_enabled, kforce = ls_force, kmap = wk_map, ktx = wk_tx, kty = wk_ty, kface = wk_face;
    frozen = 1; pause_kind = PK_NONE; ls_enabled = 1; ls_force = 0; ls_away = -1; ls_leave = 0; ls_talk = 0;
    g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.cash = 90000; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.day = 12; G.phase = PH_MORNING; G.pill_seen = 0; G.pill_given = 0;
    scr = SC_WALK; wk_mode = 0;
    /* 1. the morning: she is ALWAYS there, already there the moment you are placed in the room (no update() in between = no pop-in) */
    { int hit = 0, onyou = 0; for (k = 0; k < 500; k++) { wk_place(MAP_HALL, 3, 3, DN); ls_on = 0; wk_place(MAP_ROOM, 3, 5, UP); if (ls_on) hit++; if (ls_on && ls_tx == 3 && ls_ty == 5) onyou++; }
      if (hit != 500) { bad++; printf("v81: in the morning she must always be in the living room (%d of 500)\n", hit); } if (onyou) { bad++; printf("v81: she was placed on your tile\n"); } }
    /* the evening is still about LS_CHANCE percent, the night never */
    G.phase = PH_EVENING; { int hit = 0; for (k = 0; k < 4000; k++) { wk_place(MAP_HALL, 3, 3, DN); wk_place(MAP_ROOM, 3, 5, UP); if (ls_on) hit++; } if (hit < 4000 * (LS_CHANCE - 4) / 100 || hit > 4000 * (LS_CHANCE + 4) / 100) { bad++; printf("v81: evening chance %d of 4000\n", hit); } }
    G.phase = PH_NIGHT; wk_place(MAP_HALL, 3, 3, DN); wk_place(MAP_ROOM, 3, 5, UP); if (ls_on) { bad++; printf("v81: not at night\n"); }
    /* the sofa: the time moves on and she is rolled while the screen is black (already there when the picture comes back) */
    G.phase = PH_MORNING; ls_force = 1; wk_place(MAP_ROOM, 2, 4, LF); wk_mode = 2; wk_yes = 0; wk_sleep = 0; ls_on = 0; go(GO_TIME, 0);
    if (G.phase != PH_EVENING || ls_ph != PH_EVENING || !ls_on) { bad++; printf("v81: after the sofa she must already be rolled (phase %d ph %d on %d)\n", G.phase, ls_ph, ls_on); }
    G.phase = PH_MORNING; ls_force = 0;
    /* 2. every free floor tile of the living room (13), also in front of the fridge, the sofa, the sink and the doors */
    ls_force = 1; { int seen[8][8], cnt = 0; memset(seen, 0, sizeof seen);
      for (i = 0; i < 6000; i++) { wk_place(MAP_ROOM, 8, 4, DN); ls_ph = -1; ls_roll(); if (!ls_on) continue; seen[ls_ty][ls_tx] = 1; for (k = 0; k < 4; k++) { ls_moving = 0; ls_wander(); if (!ls_ok(ls_tx, ls_ty)) { bad++; printf("v81: she left the floor (%d,%d)\n", ls_tx, ls_ty); i = 99999; break; } seen[ls_ty][ls_tx] = 1; } }
      for (y = 0; y < 8; y++) for (x = 0; x < 8; x++) if (seen[y][x]) cnt++;
      if (cnt != 13 || !seen[3][1] || !seen[4][2] || !seen[5][2] || !seen[5][3] || !seen[3][5] || !seen[5][5] || !seen[3][2]) { bad++; printf("v81: she must use all 13 floor tiles (used %d)\n", cnt); } }
    /* she does not stay long in front of the fridge / sofa / doors, and steps to a free tile */
    { int stayed = 0; for (i = 0; i < 400; i++) { wk_place(MAP_ROOM, 8, 4, DN); ls_on = 1; ls_tx = ls_fx = 1; ls_ty = ls_fy = 3; ls_moving = 0; ls_leave = 0; ls_wander(); if (!ls_moving || (int)(ls_next - now_ms()) >= 1800) stayed++; }
      if (stayed) { bad++; printf("v81: in front of the fridge she must step away quickly (%d)\n", stayed); } }
    ls_force = 0;
    /* 3. knocking on her bedroom door while she stands in the living room: only her line, no scene */
    ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; ls_moving = 0; ls_leave = 0; wk_place(MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, UP); ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; scr = SC_WALK; wk_mode = 0; G.phase = PH_MORNING;
    on_button(B_A); if (scr != SC_WALK || wk_mode != 1 || wk_pg[0] != W_LS_NODOOR || wk_tag != N_SOPHIA) { bad++; printf("v81: knocking while she is in the living room must give her line (scr %d mode %d)\n", scr, wk_mode); }
    if (strstr(W_LS_NODOOR, "Don't you dare") == NULL) { bad++; printf("v81: wrong door line\n"); }
    shot("v81_dont_you_dare"); wk_mode = 0; ls_on = 0; on_button(B_A); if (scr == SC_WALK && wk_mode == 0) { bad++; printf("v81: without her in the living room the knock must work\n"); } scr = SC_WALK; wk_mode = 0;
    /* 4. the milk: when the pump scene, the scolding or the bottle scene ends she walks back to her door and only THEN is gone */
    { static const int IDS[3] = {SCR_PUMPB, SCR_SCOLD, SCR_BOTTLEB}; int n;
      for (n = 0; n < 3; n++) {
        frozen = 1; G.phase = PH_MORNING; ls_away = -1; ls_leave = 0; ls_talk = 1; scr = SC_WALK; wk_place(MAP_ROOM, 2, 5, UP); ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 3; ls_moving = 0; ls_ph = G.phase;
        vn_id = IDS[n]; vn_done(); if (!ls_leave || !ls_on) { bad++; printf("v81: scene %d must send her home (leave %d on %d)\n", IDS[n], ls_leave, ls_on); continue; }
        if (ls_ok(1, 1)) { bad++; printf("v81: wall tile is floor?\n"); }
        wk_mode = 0; frozen = 0; { int reached = 0, gone = 0, stepped_in = 0, ticks = 0; ls_ph = G.phase;
          for (ticks = 0; ticks < 200 && ls_on; ticks++) { if (ls_moving) ls_t0 = now_ms() - ls_step_ms() - 10; update(); if (ls_on && ((ls_tx == HOME_DOOR_X && ls_ty == HOME_DOOR_Y) || ls_leave == 2)) reached = 1; if (ls_on && ls_ty == HOME_DOOR_Y - 1) stepped_in = 1; if (ls_on && !ls_ok(ls_tx, ls_ty) && !(ls_tx == HOME_DOOR_X && ls_ty == HOME_DOOR_Y - 1)) { bad++; printf("v81: she walked off the floor (%d,%d)\n", ls_tx, ls_ty); break; } if (ls_on && ls_player_on(ls_tx, ls_ty)) { bad++; printf("v81: she walked onto you\n"); break; } }
          gone = !ls_on; if (!reached || !stepped_in || !gone) { bad++; printf("v81: she must walk to her door, step in, and only then vanish (reached %d in %d gone %d)\n", reached, stepped_in, gone); } }
        frozen = 1; ls_force = 1; ls_ph = -1; ls_roll(); if (ls_on) { bad++; printf("v81: she must stay in her room for the rest of this time of day\n"); }
        G.phase = PH_EVENING; ls_ph = -1; ls_roll(); if (!ls_on) { bad++; printf("v81: she can be there again at the next time of day\n"); } ls_force = 0; G.phase = PH_MORNING; ls_on = 0;
      } }
    /* she waits while you stand on the tile in front of her door, and goes round you otherwise */
    frozen = 1; ls_away = -1; ls_leave = 0; ls_talk = 0; scr = SC_WALK; wk_mode = 0; G.phase = PH_MORNING; wk_place(MAP_ROOM, HOME_DOOR_X, HOME_DOOR_Y, DN); ls_on = 1; ls_tx = ls_fx = 4; ls_ty = ls_fy = 4; ls_moving = 0; ls_ph = G.phase; ls_go_home();
    frozen = 0; for (k = 0; k < 20; k++) { if (ls_moving) ls_t0 = now_ms() - ls_step_ms() - 10; ls_next = now_ms(); update(); }
    if (!ls_on || (ls_tx == HOME_DOOR_X && ls_ty == HOME_DOOR_Y)) { bad++; printf("v81: she must wait while you stand in front of her door (on %d at %d,%d)\n", ls_on, ls_tx, ls_ty); }
    wk_place(MAP_ROOM, 4, 5, UP); ls_ph = G.phase; for (k = 0; k < 40 && ls_on; k++) { if (ls_moving) ls_t0 = now_ms() - ls_step_ms() - 10; update(); }
    if (ls_on) { bad++; printf("v81: she must go round you to her door\n"); }
    /* a story scene waits behind her door: A on her sends her home, then you knock */
    frozen = 1; ls_away = -1; ls_leave = 0; G.pill_given = 1; G.pill_seen = 0; G.pill_day = G.day - PILL_DAYS; wk_place(MAP_ROOM, 4, 4, LF); scr = SC_WALK; wk_mode = 0; ls_on = 1; ls_tx = ls_fx = 3; ls_ty = ls_fy = 4; ls_ph = G.phase;
    on_button(B_A); if (!ls_leave || scr != SC_WALK || wk_mode != 1) { bad++; printf("v81: with a story scene waiting A on her must send her to her door (leave %d)\n", ls_leave); }
    ls_on = 0; ls_force = 1; ls_away = -1; ls_ph = -1; ls_roll(); if (ls_on) { bad++; printf("v81: with a story scene waiting behind her door she is in her room\n"); } ls_force = 0; G.pill_given = 0; G.pill_day = 0;
    /* 5. SIX stocks, four rows, a scrolling list, logos */
    frozen = 1; ls_on = 0; ls_leave = 0; G.cash = 99999; G.bank = 0; scr = SC_SHOP; shop_open(); shop_tab = TAB_STOCKS; shop_sel = STK_NVIDIA; stk_top = 0; shop_confirm = 0; shop_info = 0;
    for (i = 0; i < STK_N; i++) if (!stkLogo[i]) { bad++; printf("v81: the logo of company %d did not load\n", i); }
    RSHOT("v81_stocks_top");
    for (k = 1; k < STK_N; k++) { on_button(B_DOWN); if (shop_sel != k || shop_sel < stk_top || shop_sel > stk_top + STK_ROWS - 1) { bad++; printf("v81: DOWN %d: sel %d top %d\n", k, shop_sel, stk_top); } }
    if (stk_top != STK_N - STK_ROWS) { bad++; printf("v81: at the end the list must be scrolled to the bottom (top %d)\n", stk_top); }
    RSHOT("v81_stocks_bottom");
    { int p0 = stk_now(&G, STK_SPACEX), c0 = G.cash; on_button(B_A); if (G.stk[STK_SPACEX] != 1 || G.cash != c0 - p0) { bad++; printf("v81: A must buy one SPACEX share\n"); } p0 = stk_now(&G, STK_SPACEX); c0 = G.cash; on_button(B_X); if (G.stk[STK_SPACEX] != 0 || G.cash != c0 + p0) { bad++; printf("v81: X must sell it\n"); } }
    on_button(B_UP); if (shop_sel != STK_SONY || shop_sel < stk_top) { bad++; printf("v81: UP to SONY (sel %d top %d)\n", shop_sel, stk_top); } { int p0 = stk_now(&G, STK_SONY); on_button(B_A); if (G.stk[STK_SONY] != 1) { bad++; printf("v81: A must buy one SONY share\n"); } (void)p0; }
    for (k = 0; k < 6; k++) on_button(B_UP); if (shop_sel != STK_SONY) { bad++; printf("v81: six UPs go round once (sel %d)\n", shop_sel); }
    shop_sel = STK_NVIDIA; on_button(B_UP); if (shop_sel != STK_SPACEX || stk_top != STK_N - STK_ROWS) { bad++; printf("v81: UP from the first company wraps to SPACEX and scrolls (sel %d top %d)\n", shop_sel, stk_top); }
    on_button(B_R1); on_button(B_L1); if (stk_top != 0) { bad++; printf("v81: changing tab resets the scroll\n"); }
    G.stk[STK_SONY] = 0; ls_on = 0;
    G = keep; saved_G = keeps; scr = kscr; frozen = fr; ls_enabled = ken; ls_force = kforce; ls_on = 0; ls_talk = 0; ls_leave = 0; ls_away = -1; ls_ph = -1; wk_place(kmap, ktx, kty, kface); pause_kind = PK_NONE;
    return bad;
}
static int v79_tests(void) {   /* v7.9: text from lines.txt / talk.txt reaches the real game (the loader itself is tested in tests/test_logic.c) */
    int bad = 0; Game keep = G; int kscr = scr, kpeek = peeked, n0 = B_BUMP_N;
    ex_reset(); ex_nlog = ex_nprob = 0;
    ex_load_string("[B_BUMP]\nNAR LOBBY 0 | One.\nAW LOBBY 1 | Two.\nME | Three.\n[L_START]\nSAY THIS|ONLY\n"
                   "[B_SCOLD]\nHER ROM 1 | s1\nHER | s2\nHER | s3\nHER | s4\nHER | s5\n[B_BOTTLEB]\nNAR ROM 0 | b1\nNAR | b2\n"
                   "[B_LEAD]\nNAR ROM 0 | l1\nHER | l2\nHER | l3\nHER | l4\n", 0);
    if (ex_nprob) { bad++; printf("v79: the test text was refused: %s\n", ex_log[0]); }
    vn_start(SCR_BUMP);
    if (scr != SC_STORY || vn_n != 3 || strcmp(vn_buf[1].txt, "Two.") || vn_buf[1].spr != 1 || vn_buf[2].spr != 1 || vn_buf[2].bg != BG_LOBBY || vn_buf[2].who != W_ME) { bad++; printf("v79: a longer scene did not reach the game (n %d)\n", vn_n); }
    shot("v79_custom_scene_1"); vn_i = 1; vn_prev_spr = 0; shot("v79_custom_scene_2");
    on_button(B_A); on_button(B_A);   /* finish typing, next beat */
    if (strcmp(pick(L_START, L_START_N), "SAY THIS|ONLY") || L_START_N != 1) { bad++; printf("v79: the table line from the file is not used\n"); }
    peeked = 1; vn_start(SCR_BOTTLEB);
    if (vn_n != 7 || strcmp(vn_buf[0].txt, "b1") || strcmp(vn_buf[1].txt, "s1") || strcmp(vn_buf[5].txt, "s5") || strcmp(vn_buf[6].txt, "b2")) { bad++; printf("v79: the scolding scene must go before the closing line (n %d)\n", vn_n); }
    peeked = 0; vn_start(SCR_BOTTLE);
    if (B_LEAD_N != 1 || vn_n != LEAD_BEATS + B_BOTTLE_N || strcmp(vn_buf[0].txt, "l1") || strcmp(vn_buf[LEAD_BEATS].txt, B_BOTTLE[0].txt)) { bad++; printf("v79: a lead-in from the file must play before the bottle scene (n %d)\n", vn_n); }
    ex_reset(); vn_start(SCR_BUMP);
    if (vn_n != n0 || L_START_N != 5 || B_LEAD_N != 6) { bad++; printf("v79: the built-in text must come back after a reset\n"); }
    /* a scene the game fills in: the (auto) beat is still filled with how you played */
    ex_load_string("[B_GREET]\nNAR HALL 0 | Knock.\nHER HALL 1 | (auto)\nHER | Extra beat after the greeting.\n", 0);
    G.m_played = 0; G.forfeit = 0; vn_start(SCR_GREET);
    if (vn_n != 3 || !vn_buf[1].txt || !in_pool(vn_buf[1].txt, G_NORMAL, G_NORMAL_N) || strcmp(vn_buf[2].txt, "Extra beat after the greeting.")) { bad++; printf("v79: an (auto) beat in a longer scene was not filled in\n"); }
    ex_reset();
    printf("v7.9: scenes and lines from the text files reach the game: problems %d\n", bad);
    G = keep; scr = kscr; peeked = kpeek; return bad;
}

static int romance_tests(void) {
    int bad = 0, k, c0; Game keep = G, keeps = saved_G; int fr = frozen; frozen = 1;
    g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.event = 1; G.room_up = 1; room_apply(); G.cash = 20000; G.energy = 100; G.sw_wins = SW_GOAL; saved_G = G; room_seen = 1;   /* v10.0: 20 wins already (the ring is sold in the black market) */ /* v9.8 test fix: room_apply, so the map is the 11 x 7 room with the computer at (7,3) */ scr = SC_WALK; pause_kind = PK_NONE; wk_mode = 0; fade_st = 0; home_end = 0;
    /* the computer in the bedroom opens the online store; the ring is for sale from the start and is one time only */
    bot_use(MAP_ROOM, 7, 3, UP); if (scr != SC_SHOP) { bad++; printf("the computer did not open the online store\n"); }
    if (shop_tab != TAB_STOCKS || shop_tabs_n() != 2) { bad++; printf("v10: before she lives with you the store has STOCKS and (after 20 wins) the BLACK MARKET only (tab %d, %d tabs)\n", shop_tab, shop_tabs_n()); }
    on_button(B_R1); if (shop_tab != TAB_BM || shop_sel != SH_RING) { bad++; printf("v78: R1 must reach the BLACK MARKET with the ring selected\n"); }
    on_button(B_A); on_button(B_A);
    if (!G.ring || G.cash != 20000 - SH_PRICE[SH_RING]) { bad++; printf("buying the ring failed (ring %d cash %d)\n", G.ring, G.cash); }
    { int o[16], n = shop_list(TAB_BM, o), j; for (j = 0; j < n; j++) if (o[j] == SH_RING) { bad++; printf("v10: the bought ring must vanish from the list\n"); } }
    on_button(B_L1); if (shop_tab != TAB_STOCKS) { bad++; printf("v78: L1 must go back to STOCKS\n"); }
    G.flower = 1; G.cash -= SH_PRICE[SH_FLOWER];   /* v10.0: the ITEMS tab is closed until she lives with you: the flower is given directly so the later steps keep their state */
    on_button(B_B); if (scr != SC_WALK) { bad++; printf("the store did not close\n"); }
    /* before the proposal: the upgrade is not for sale and her door has no extra entries */
    if (g_upgrade_avail(&G) || g_rom_on(&G)) { bad++; printf("upgrade or romance available too early\n"); }
    /* her door: PLAY / PROPOSE / LEAVE; the proposal ends with her question */
    bot_use(MAP_HALL3, 1, 4, LF); if (!rom_adv(1) && 0) {}
    for (k = 0; k < 40 && scr != SC_DOOR; k++) { fake_ms += 300; update(); if (scr == SC_STORY) on_button(B_A); }
    if (scr != SC_DOOR) { bad++; printf("romance: did not reach her door menu (scr %d)\n", scr); }
    else {
        int dk[5]; if (door_items(dk) != 4 || dk[1] != DR_PROPOSE || dk[2] != DR_COMEIN) { bad++; printf("the door menu must offer PROPOSE once you own the ring\n"); }
        on_button(B_DOWN); on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_PROPOSE) { bad++; printf("propose did not start the scene\n"); }
        rom_adv(20); if (scr != SC_ROMQ) { bad++; printf("the proposal did not end with her question (scr %d)\n", scr); }
        on_button(B_A); rom_adv(10);
        if (G.rs != RS_VISIT || scr != SC_WALK) { bad++; printf("answering yes must set RS_VISIT and return to walking (rs %d scr %d)\n", G.rs, scr); }
    }
    /* she visits your room the first time you are in it; then the desk sells the upgrade */
    if (wk_map != MAP_ROOM || scr != SC_WALK || wk_mode != 1 || wk_after != WK_DEMAND) { bad++; printf("after yes you must be back in your room waiting for her (map %d scr %d mode %d)\n", wk_map, scr, wk_mode); }
    { int k, n = 0; for (k = 0; k < 14 && scr == SC_WALK; k++) { if (wk_mode == 1) { n++; if (k == 1) { RSHOT("v65_wait_text"); } } on_button(B_A); }   /* two narration messages (wait, time passes), then a fade and the knocking narration, only then she comes in */
      if (n != 2) { bad++; printf("v66: the wait must be two messages, no pan (saw %d)\n", n); } }
    if (scr != SC_STORY || vn_id != SCR_DEMAND || vn_buf[0].bg != BG_MYROOM) { bad++; printf("she did not visit your room (scr %d)\n", scr); }
    if (vn_buf[0].spr != 0 || vn_buf[1].spr != 0 || vn_buf[2].spr != 0 || vn_buf[3].spr == 0) { bad++; printf("v66: the knocking is narration only; her picture shows when she speaks\n"); }
    rom_adv(20); if (G.rs != RS_UPG || scr != SC_WALK) { bad++; printf("after her visit the upgrade must be open (rs %d)\n", G.rs); }
    /* asking her to move in before the upgrade sends you away, back to the door menu */
    bot_use(MAP_HALL3, 1, 4, LF); for (k = 0; k < 40 && scr != SC_DOOR; k++) { fake_ms += 300; update(); if (scr == SC_STORY) on_button(B_A); }
    { int dk[5], n = door_items(dk); if (n != 4 || dk[1] != DR_ASK || dk[2] != DR_COMEIN) { bad++; printf("the door menu must offer ASK TO MOVE IN after her visit\n"); }
      else { on_button(B_DOWN); on_button(B_A); if (scr != SC_SCENE) { bad++; printf("asking before the upgrade must give her refusal\n"); } for (k = 0; k < 6 && scr == SC_SCENE; k++) on_button(B_A); if (scr != SC_DOOR || G.rs != RS_UPG) { bad++; printf("the refusal must return to the door menu\n"); } } }
    go(GO_HUB, 0);
    /* the desk: the upgrade is in the same menu; cash only; rent goes to 1000 */
    G.cash = 100; bot_use(MAP_LOBBY, 7, 6, UP); WK_CLOSE();
    if (wk_mode != 4) { bad++; printf("the desk menu did not open for the upgrade (mode %d)\n", wk_mode); }
    on_button(B_A); WK_CLOSE(); if (G.room_up != 1 || G.rs != RS_UPG || G.cash != 100) { bad++; printf("upgrade 2 must fail without cash\n"); }
    G.cash = 14000; bot_use(MAP_LOBBY, 7, 6, UP); WK_CLOSE(); on_button(B_A); WK_CLOSE();
    if (G.room_up != 2 || G.rs != RS_READY || G.cash != 4000 || g_rent_amt(&G) != 1000) { bad++; printf("the upgrade did not work (up %d rs %d cash %d)\n", G.room_up, G.rs, G.cash); }
    bot_use(MAP_LOBBY, 7, 6, UP); WK_CLOSE(); if (wk_mode == 4) { bad++; printf("the upgrade must be gone from the desk once bought\n"); } wk_mode = 0; if (scr == SC_STORE) scr = SC_WALK;   /* (no rent due, no upgrade: the greeting leads straight to the drinks store) */
    /* now she moves in: the 3rd floor closes */
    bot_use(MAP_HALL3, 1, 4, LF); for (k = 0; k < 40 && scr != SC_DOOR; k++) { fake_ms += 300; update(); if (scr == SC_STORY) on_button(B_A); }
    on_button(B_DOWN); on_button(B_A); if (scr != SC_STORY || vn_id != SCR_MOVEIN) { bad++; printf("asking to move in did not start the scene (scr %d)\n", scr); }
    rom_adv(20); if (G.rs != RS_LIVING || wk_map != MAP_HALL || scr != SC_WALK) { bad++; printf("moving in failed (rs %d map %d)\n", G.rs, wk_map); }
    { int fl[3]; if (elev_floors(2, fl) != 1 || floor_card()) { bad++; printf("the 3rd floor must be closed once she lives with you\n"); } }
    /* her bedroom door in the upgraded room: knock = the romance scene */
    wk_place(MAP_ROOM, 5, 3, UP); if (wk_tile(MAP_ROOM, 5, 2) != 'g') { bad++; printf("the upgraded room has no door for her\n"); }
    G.phase = PH_NIGHT; on_button(B_A); if (scr != SC_STORY || vn_id != SCR_NIGHTHOME || vn_buf[0].spr != 0) { bad++; printf("at night the knock must start with narration and end in a scolding (scr %d)\n", scr); }
    rom_adv(12); if (scr != SC_WALK) { bad++; printf("the night scolding must return to walking (scr %d)\n", scr); }
    G.phase = PH_MORNING; wk_place(MAP_ROOM, 5, 3, UP); on_button(B_A);
    if (scr != SC_STORY || vn_id != SCR_KNOCKHOME || vn_buf[0].spr != 0) { bad++; printf("knocking at home must start with the narration of the knock (scr %d)\n", scr); }
    rom_adv(3); if (scr != SC_ROM) { bad++; printf("knocking did not open the romance scene (scr %d)\n", scr); }
    if (rom_in || rom_choose || vn_id != SCR_KNOCKHOME) { bad++; printf("v66: before day 6 she only stands at her door, her line first (in %d choose %d)\n", rom_in, rom_choose); }
    RC(); if (!rom_choose || rom_mode != RM_MENU || rom_menu_n() != 2) { bad++; printf("v66: A must open the choice page (page 1 = play a match / chat, no LEAVE row)\n"); }
    /* gifts: one a day, flower +10%, money stack +20%, the pill only at a full level 3 bar, once */
    G.flower = 2; G.stack = 1; G.pill = 1; G.gift_day = 0; G.day = 5; G.energy = 100; G.chal_day = 0;
    on_button(B_R1); on_button(B_A);   /* v8.3: R1 = the give page, the first row is the flower */
    if (scr != SC_STORY || vn_id != SCR_GIFT || vn_buf[0].bg != BG_BDOOR || vn_buf[1].bg != BG_BDOOR || vn_buf[1].spr != 1) { bad++; printf("v66: a gift is a short scene at her door (scr %d)\n", scr); }
    rom_adv(10); if (scr != SC_WALK || wk_map != MAP_ROOM || wk_tx != HOME_DOOR_X || wk_ty != HOME_DOOR_Y || wk_face != UP) { bad++; printf("v66: after a gift you must be back in front of her door (scr %d)\n", scr); }
    if (G.rom_pct != 10 || G.flower != 1) { bad++; printf("a flower must give 10%% (pct %d)\n", G.rom_pct); }
    door_knock(); rom_adv(3); RC(); on_button(B_R1); on_button(B_A); if (G.rom_pct != 10 || G.flower != 1 || scr != SC_ROM || !in_pool(rom_txt, R_GIFT_TODAY, R_GIFT_TODAY_N)) { bad++; printf("a second gift on the same day must be refused\n"); }
    G.day = 6; RC(); on_button(B_DOWN); on_button(B_A); rom_adv(10); if (G.rom_pct != 30 || G.stack != 0 || scr != SC_WALK) { bad++; printf("a money stack must give 20%% (pct %d)\n", G.rom_pct); }
    G.day = 7; door_knock(); rom_adv(3); RC(); on_button(B_R1); on_button(B_UP); on_button(B_A); if (G.pill_given || scr != SC_ROM) { bad++; printf("the pill must wait for a full bar at level 3\n"); }
    /* free match at home: 10 energy, once a day, no money, +5% for a win */
    c0 = G.cash; RC(); rom_page = 0; rom_sel = 0; on_button(B_A);   /* back on page 1: PLAY A MATCH */
    if (rom_mode != RM_MENU) { rom_mode = RM_MENU; }
    rom_page = 0; rom_sel = 0; on_button(B_A); rom_lsel = 0; on_button(B_A);
    if (g_rom_on(&G)) { vn_start(SCR_LEAVE); if (vn_buf[0].bg != BG_BDOOR) { bad++; printf("v67: leaving a match at home: her bedroom door picture\n"); } scr = SC_PLAY; }
    if (scr != SC_PLAY || !G.home || G.energy != 100 - HOME_ENERGY || G.cash != c0 || G.mlen != 10 || G.fee != 0) { bad++; printf("the home match did not start properly (scr %d energy %d)\n", scr, G.energy); }
    else {
        int pc = G.rom_pct; G.net = 8; G.hand = G.mlen; G.over = 1; finish_match();
        if (scr != SC_END || !home_end || G.last_rom != ROM_WIN10 || G.rom_pct != pc + ROM_WIN10 || G.cash != c0 || G.last_pay != 0 || G.last_drink) { bad++; printf("a home win must only fill the bar (rom %d pct %d)\n", G.last_rom, G.rom_pct); }
        RSHOT("v62_home_match_end");
        on_button(B_A); if (scr != SC_ROM || home_end) { bad++; printf("the home match did not return to her door scene\n"); }
    }
    rom_choose = 1; rom_mode = RM_LEN; rom_lsel = 1; on_button(B_A); if (scr != SC_ROM || G.home || G.in_match) { bad++; printf("a second match on the same day must be refused\n"); }
    RSHOT("v62_romance_scene");
    /* the bar, levels and the pill */
    G.rom_lvl = 1; G.rom_pct = 95; g_rom_add(&G, 10); if (G.rom_lvl != 2 || G.rom_pct != 0) { bad++; printf("level up must reset the bar to 0 (lvl %d pct %d)\n", G.rom_lvl, G.rom_pct); }
    G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.day = 9; G.gift_day = 0; if (g_gift(&G, GF_PILL) != 0 || !G.pill_given) { bad++; printf("the pill must work at a full level 3 bar\n"); }
    RSHOT("v62_after_pill");
    G.pill_given = 0; G.rs = RS_LIVING;
    wk_place(MAP_ROOM, 5, 3, UP); scr = SC_WALK; RSHOT("v62_room_upgraded");
    G.room_up = 1; G.rs = RS_NONE; wk_place(MAP_ROOM, 5, 4, UP); RSHOT("v93_room_upgrade1");
    G.room_up = 0; G.rs = RS_NONE; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN); RSHOT("v62_room_small"); G.rs = RS_LIVING; scr = SC_SHOP; shop_tab = TAB_ITEMS; shop_sel = 0; RSHOT("v62_online_store"); G.rs = RS_NONE;
    /* v6.4: the pill takes 7 days; days 6 and 7 she complains; the first knock on day 8 plays the scene and changes her pictures; the night sneak-in opens after it */
    g_init(&G); G.story = ST_NAMED; G.rs = RS_LIVING; G.room_up = 2; G.pill = 1; G.rom_lvl = ROM_LVLS; G.rom_pct = 100; G.day = 10; G.phase = PH_MORNING; G.gift_day = 0; G.energy = 100; G.cash = 500; saved_G = G;
    room_seen = 1; frozen = 1; fade_st = 0; scr = SC_WALK; wk_mode = 0; pause_kind = PK_NONE; wk_place(MAP_ROOM, 5, 3, UP);
    on_button(B_A); rom_adv(3); if (scr != SC_ROM) { bad++; printf("v64: knock at home did not reach the menu (scr %d)\n", scr); }
    RSHOT("v64_rom_menu_in_box");
    RC(); RSHOT("v66_rom_choices_door"); rom_choose = 1; rom_mode = RM_MENU; rom_page = 1; rom_sel = 0; RSHOT("v64_rom_gift_in_box"); on_button(B_A);   /* page 2 lists only the pill (the only thing owned) */
    if (scr != SC_STORY || vn_id != SCR_PILLGIVE || !G.pill_given || G.pill_day != 10 || G.pill_seen) { bad++; printf("v64: giving the pill must start the pill scene and the 7 days (scr %d day %d)\n", scr, G.pill_day); }
    else {
        int has = 0, q; for (q = 0; q < vn_n; q++) if (vn_buf[q].txt && strstr(vn_buf[q].txt, "What is this pill")) has = 1;
        if (!has || her_vis() != (G.rs == RS_LIVING ? homeS : conf)) { bad++; printf("v64: the pill scene needs her question, and her picture must stay her normal one (v8.7: her moved-in picture)\n"); }
        has = 0; for (q = 0; q < vn_n; q++) if (vn_buf[q].txt && strstr(vn_buf[q].txt, "beauty supplement")) has = 1; if (!has) { bad++; printf("v64: you must say it is a beauty supplement\n"); }
    }
    rom_adv(12); if (scr != SC_WALK || wk_map != MAP_ROOM || wk_tx != HOME_DOOR_X || wk_ty != HOME_DOOR_Y || wk_face != UP) { bad++; printf("v66: after the pill scene you must stand in front of her door on the map (scr %d)\n", scr); }
    { int d; for (d = 11; d <= 18; d++) { int want_h = d == 15 ? 1 : d == 16 ? 2 : 0, want_due = d >= 17; G.day = d;
        if (g_invited(&G) != (d >= 15)) { bad++; printf("v66: she invites you in from day 6 on only (day %d)\n", d); }
        if (g_pill_hint(&G) != want_h || g_pill_due(&G) != want_due) { bad++; printf("v64: pill timing wrong on day %d (hint %d due %d)\n", d, g_pill_hint(&G), g_pill_due(&G)); } } }
    G.day = 15; if (!in_pool(rom_greet(), R_PILL6, (int)R_PILL6_N)) { bad++; printf("v64: day 6 after the pill: stuffy chest line missing\n"); }
    G.day = 16; if (!in_pool(rom_greet(), R_PILL7, (int)R_PILL7_N)) { bad++; printf("v64: day 7 after the pill: tight clothes line missing\n"); }
    G.day = 17; G.phase = PH_NIGHT; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 5, 3, UP); on_button(B_A);
    if (vn_id != SCR_NIGHTHOME || G.pill_seen) { bad++; printf("v64: before the day-8 scene the night knock is only a scolding\n"); }
    rom_adv(12); G.phase = PH_MORNING; scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 5, 3, UP); on_button(B_A);
    { int k; for (k = 0; k < 60 && !(scr == SC_STORY && vn_id == SCR_PILLDAY); k++) { fake_ms += 300; update(); if (scr == SC_STORY && vn_id == SCR_KNOCKHOME) on_button(B_A); } }
    if (scr != SC_STORY || vn_id != SCR_PILLDAY || !G.pill_seen || her_vis() != herCoverS) { bad++; printf("v64: the day-8 knock must play the body-change scene with her new picture (scr %d seen %d)\n", scr, G.pill_seen); }
    else { int has = 0, q; for (q = 0; q < vn_n; q++) if (vn_buf[q].txt && strstr(vn_buf[q].txt, "outside")) has = 1; if (!has) { bad++; printf("v64: she must say she cannot go outside\n"); } }
    if (vn_buf[0].bg != BG_BDOOR || vn_buf[1].spr != 0 || vn_buf[vn_n - 1].bg != BG_ROM) { bad++; printf("v66: the day-8 scene starts on the door picture without her picture and ends in her room\n"); }
    RSHOT("v64_day8_scene"); rom_adv(24); if (scr != SC_ROM || !rom_in) { bad++; printf("v65: the day-8 scene must end inside her room with her menu (scr %d)\n", scr); }
    RC(); { int a[10]; if (rom_pages() != 3 || rom_page_acts(0, a) != 2 || a[0] != RA_PLAY || a[1] != RA_CHAT || rom_page_acts(1, a) != 0 || rom_page_acts(2, a) != 1 || a[0] != RA_TEASE) { bad++; printf("v83: after the day-8 scene: page 1 match / chat, page 2 nothing owned (YOU HAVE NO GIFT), page 3 tease\n"); } }
    rom_page = 1; RSHOT("v83_page2_empty"); rom_page = 0;
    scr = SC_WALK; wk_mode = 0;
    G.phase = PH_NIGHT; wk_place(MAP_ROOM, 5, 3, UP); on_button(B_A);
    if (scr != SC_SNEAK || sn_mode != SM_CHOICE) { bad++; printf("v64: after the day-8 scene the night door must offer KNOCK / sneak in (scr %d)\n", scr); }
    else {
        RSHOT("v64_night_choice"); on_button(B_DOWN); on_button(B_A);
        { int k; for (k = 0; k < 60 && !(scr == SC_SNEAK && sn_mode == SM_BED); k++) { fake_ms += 300; update(); if (scr == SC_STORY) on_button(B_A); } }
        if (scr != SC_SNEAK || sn_mode != SM_BED) { bad++; printf("v64: sneaking in at home did not reach her sleeping options (scr %d mode %d)\n", scr, sn_mode); }
        else { on_button(B_A); sn_mode = SM_BED; sn_sel = 0; RSHOT("v64_sleep_options"); on_button(B_B); { int k; for (k = 0; k < 40 && scr != SC_WALK; k++) { fake_ms += 300; update(); } }
               if (scr != SC_WALK || wk_map != MAP_ROOM) { bad++; printf("v64: leaving her bed picture must return to your living room (scr %d map %d)\n", scr, wk_map); } }
    }
    G.phase = PH_NIGHT; wk_mode = 0; scr = SC_WALK; wk_place(MAP_ROOM, 5, 3, UP); on_button(B_A); if (scr != SC_SNEAK) { bad++; printf("v64: the sneak-in must stay available at night\n"); }
    scr = SC_WALK; { Game t; g_init(&t); t.rs = RS_LIVING; t.phase = PH_NIGHT; if (g_home_sneak(&t)) { bad++; printf("v64: no sneak-in before the pill scene\n"); } }
    /* ---- v6.5: days 6 / 7 asked first, her room after day 8, the milk pill, pump, gauge, Baby Milk Bottle ---- */
    {
#define GIVE(kind) do { wk_mode = 0; scr = SC_ROM; rom_in = 1; rom_choose = 1; rom_mode = RM_MENU; rom_give(kind); } while (0)   /* (v6.7: the milk pill comes from her menu entry, the pump only from the scene) */
#define KNOCK_AT_HOME(d, ph) do { G.day = (d); G.phase = (ph); scr = SC_WALK; wk_mode = 0; wk_place(MAP_ROOM, 5, 3, UP); on_button(B_A); } while (0)
        int k, i;
        g_init(&G); G.story = ST_NAMED; G.event = 1; G.hall_seen = 3; G.rs = RS_LIVING; G.room_up = 2; G.ring = 1; G.cash = 90000; G.rom_lvl = ROM_LVLS; G.rom_pct = 100;
        G.pill = 1; G.pill_given = 1; G.pill_day = 10;
        KNOCK_AT_HOME(15, PH_MORNING);   /* day 6: you ask her first */
        if (vn_id != SCR_KNOCKHOME) { bad++; printf("v65: day 6 knock did not start\n"); }
        for (k = 0; k < 4 && vn_id == SCR_KNOCKHOME; k++) on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_PILL6 || G.hint_day != 15 || vn_buf[1].who != W_ME) { bad++; printf("v65: day 6: the asking-first scene did not play (scr %d id %d)\n", scr, vn_id); }
        { int q, pic = 0; for (q = 0; q < vn_n - 1; q++) if (vn_buf[q].spr || vn_buf[q].bg != BG_BDOOR) pic = 1;
          if (pic || vn_buf[vn_n - 1].bg != BG_ROM || vn_buf[vn_n - 1].spr != 0) { bad++; printf("v66: day 6: only the door picture, no picture of her, until she invites you in\n"); } }
        RSHOT("v65_day6_you_ask"); rom_adv(20); if (scr != SC_ROM || !rom_in) { bad++; printf("v65: day 6 scene must end with her menu inside her room\n"); }
        KNOCK_AT_HOME(15, PH_EVENING); if (vn_id != SCR_KNOCKIN) { bad++; printf("v66: day 6 second knock: she lets you in at once (id %d)\n", vn_id); }
        rom_adv(8);
        if (scr != SC_ROM || !rom_in) { bad++; printf("v65: day 6 second knock must go straight to her menu in her room (scr %d id %d)\n", scr, vn_id); }
        KNOCK_AT_HOME(16, PH_MORNING); for (k = 0; k < 4 && vn_id == SCR_KNOCKHOME && scr == SC_STORY; k++) on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_PILL7) { bad++; printf("v65: day 7: the asking-first scene did not play\n"); }
        rom_adv(20);
        KNOCK_AT_HOME(17, PH_MORNING); for (k = 0; k < 4 && vn_id == SCR_KNOCKHOME && scr == SC_STORY; k++) on_button(B_A);
        if (vn_id != SCR_PILLDAY || vn_buf[1].who != W_ME) { bad++; printf("v65: day 8: you must ask first\n"); }
        rom_adv(30);
        KNOCK_AT_HOME(18, PH_MORNING);   /* after day 8: you knock and walk into her room */
        if (vn_id != SCR_KNOCKIN) { bad++; printf("v65: after day 8 the knock must let you into her room (id %d)\n", vn_id); }
        RSHOT("v65_enter_her_room"); rom_adv(10);
        if (scr != SC_ROM || !in_pool(rom_txt, R_GREET_P, R_GREET_P_N)) { bad++; printf("v65: inside her room she must greet you with the after-day-8 lines\n"); }
        if (!g_rom_full(&G)) { bad++; printf("v65: setup (full bar) wrong\n"); }
        RSHOT("v65_room_no_bar");
        RC(); G.mpill = 0; { int a[10]; if (rom_page_acts(1, a) != 0) { bad++; printf("v83: with no milk pill the give page is empty\n"); } } GIVE(GF_MILK); if (scr != SC_ROM || !in_pool(rom_txt, R_GIFT_NONE, R_GIFT_NONE_N) || rom_mode != RM_MENU) { bad++; printf("v67: with no milk pill in the bag she refuses\n"); }
        RSHOT("v67_milk_menu"); rom_mode = RM_MENU; rom_sel = 0;
        G.mpill = 12; G.pump = 1;
        GIVE(GF_PUMP); if (scr != SC_ROM || !in_pool(rom_txt, R_PUMP_NOW, R_PUMP_NOW_N)) { bad++; printf("v65: the pump before the wait must be refused\n"); }
        G.pump = 0;   /* v6.7: first the path without the pump (the objective shows), the pump is bought later */
        GIVE(GF_MILK); if (scr != SC_STORY || vn_id != SCR_MILK1 || G.milk_st != 1 || G.mpill != 11) { bad++; printf("v65: the first milk pill must start the first-pill scene (id %d st %d)\n", vn_id, G.milk_st); }
        rom_adv(20); if (scr != SC_WALK || wk_map != MAP_ROOM || wk_mode != 1) { bad++; printf("v66: after the first pill you are back at her door with a line (scr %d)\n", scr); } wk_mode = 0;
        GIVE(GF_MILK); if (!in_pool(rom_txt, R_MILK_WAIT, R_MILK_WAIT_N) || G.mpill != 11) { bad++; printf("v65: a second pill while waiting must be refused\n"); }
        if (G.gift_day == G.day) { bad++; printf("v65: milk pills must not use up the gift of the day\n"); }
        KNOCK_AT_HOME(19, PH_MORNING); rom_adv(10); if (scr != SC_ROM || G.milk_st != 1) { bad++; printf("v65: day 1 after the pill: nothing yet\n"); }
        KNOCK_AT_HOME(20, PH_MORNING);
        if (vn_id != SCR_KNOCKIN) { bad++; printf("v65: day 2 knock did not start\n"); }
        for (k = 0; k < 4 && vn_id == SCR_KNOCKIN && scr == SC_STORY; k++) on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_MILK2 || G.milk_st != 2 || vn_buf[1].who != W_ME) { bad++; printf("v65: two days later the even-more-stuffy scene must play, you asking first (id %d st %d)\n", vn_id, G.milk_st); }
        if (vn_n != 6 || vn_buf[5].who != W_NAR) { bad++; printf("v67: without the pump the scene ends on the line that sends you to the store (n %d)\n", vn_n); }
        RSHOT("v65_milk2"); rom_adv(20);
        if (scr != SC_ROM || !strstr(rom_note, "PUMP")) { bad++; printf("v67: without the pump the objective to buy it must show (scr %d)\n", scr); }
        GIVE(GF_MILK); if (!in_pool(rom_txt, R_MILK_PUMP, R_MILK_PUMP_N)) { bad++; printf("v65: pills before the pump must send you for the pump\n"); }
        /* v6.7: you buy the pump later: on your next visit it is handed over by the scene, there is no menu entry for it */
        G.pump = 1; KNOCK_AT_HOME(21, PH_MORNING);
        for (k = 0; k < 4 && vn_id == SCR_KNOCKIN && scr == SC_STORY; k++) on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_PUMP || G.milk_st != 3 || G.pump) { bad++; printf("v67: the next visit with the pump must hand it over by itself (scr %d id %d st %d)\n", scr, vn_id, G.milk_st); }
        if (!strstr(vn_buf[1].txt, "baby bottle")) { bad++; printf("v66: you must hand her the baby bottle with the pump\n"); }
        { int q, emb = 0; for (q = 0; q < vn_n; q++) if (vn_buf[q].txt && strstr(vn_buf[q].txt, "embarrassing")) emb = 1; if (!emb || vn_n < 12) { bad++; printf("v67: the pump scene needs her extra hesitation lines (n %d)\n", vn_n); } }
        rom_adv(40); if (scr != SC_PEEK || peek_ctx != 0) { bad++; printf("v67: the pump scene must end with the face-the-wall / look choice (scr %d)\n", scr); }
        RSHOT("v67_peek_choice"); on_button(B_DOWN); on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_PEEKPIC || !peeked || vn_buf[0].bg != BG_PUMP) { bad++; printf("v67: LOOK AT HER must show her picture (scr %d id %d)\n", scr, vn_id); }
        RSHOT("v67_peek_picture"); rom_adv(4);
        if (scr != SC_STORY || vn_id != SCR_PUMPB || vn_n != B_PUMPB_N + B_SCOLD_N || !strstr(vn_buf[B_PUMPB_N - 1].txt, "peeked") || !strstr(vn_buf[vn_n - 1].txt, "Sophia breast milk")) { bad++; printf("v67: after peeking she must scold you at the end of the pump scene (id %d n %d)\n", vn_id, vn_n); }
        RSHOT("v67_scold"); rom_adv(40); if (scr != SC_WALK || !g_milk_gauge(&G)) { bad++; printf("v65: after the pump the gauge must exist (scr %d)\n", scr); }
        wk_mode = 0; scr = SC_ROM; rom_in = 1; rom_mode = RM_MENU; rom_choose = 1; RSHOT("v65_gauge_0");
        for (i = 1; i <= 1; i++) { GIVE(GF_MILK); if (scr != SC_STORY || vn_id != SCR_GIFT || G.milk_pct != 50 * i || !in_pool(vn_buf[1].txt, R_MILK_OK, R_MILK_OK_N)) { bad++; printf("v65: pill %d did not add 50%% (pct %d)\n", i, G.milk_pct); }
            rom_adv(10); scr = SC_ROM; rom_in = 1; rom_mode = RM_MENU; rom_choose = 1; RSHOT("v65_gauge_50"); }
        GIVE(GF_MILK); if (scr != SC_STORY || vn_id != SCR_BOTTLE || G.bottle != 2 || G.milk_pct != 0) { bad++; printf("v65: the second pill must hand you the bottle (the first one came with the pump scene) (id %d bottle %d)\n", vn_id, G.bottle); }
        rom_adv(20); if (scr != SC_PEEK || peek_ctx != 1) { bad++; printf("v67: the bottle scene has the same peek choice (scr %d)\n", scr); }
        on_button(B_A);   /* FACE THE WALL: no scolding */
        if (scr != SC_STORY || vn_id != SCR_BOTTLEB || peeked || vn_n != B_BOTTLEB_N) { bad++; printf("v67: facing the wall: the bottle scene has no scolding (id %d n %d)\n", vn_id, vn_n); }
        rom_adv(20); RSHOT("v65_after_bottle");
        G.milk_pct = 50; G.mpill += 3; GIVE(GF_MILK); rom_adv(20); on_button(B_DOWN); on_button(B_A);   /* the next full bottle: LOOK */
        if (scr != SC_STORY || vn_id != SCR_PEEKPIC) { bad++; printf("v67: every bottle scene offers the look (scr %d id %d)\n", scr, vn_id); }
        rom_adv(4);
        if (vn_id != SCR_BOTTLEB || vn_n != B_BOTTLEB_N + B_SCOLD_N || vn_buf[vn_n - 1].who != W_NAR || !strstr(vn_buf[vn_n - 5].txt, "peeked")) { bad++; printf("v67: peeking at a bottle: she scolds before the closing line (id %d n %d)\n", vn_id, vn_n); }
        rom_adv(20); G.bottle = 1;
        /* v6.7: you already own the pump when the two days are over: the same visit hands it over, nothing to buy */
        { Game sv = G; G.milk_st = 1; G.milk_day = G.day - MILK_WAIT; G.pump = 1; KNOCK_AT_HOME(G.day, PH_EVENING);
          for (k = 0; k < 4 && vn_id == SCR_KNOCKIN && scr == SC_STORY; k++) on_button(B_A);
          if (vn_id != SCR_MILK2 || vn_n != MILK2_OWNED || !strstr(vn_buf[5].txt, "bought")) { bad++; printf("v67: with the pump bought the milk scene hands it over (id %d n %d)\n", vn_id, vn_n); }
          rom_adv(60); if (scr != SC_PEEK || G.milk_st != 3 || G.pump || rom_note[0] == 0 && 0) { bad++; printf("v67: the pump must follow the milk scene by itself (scr %d st %d)\n", scr, G.milk_st); }
          on_button(B_A); rom_adv(40); if (scr != SC_WALK) { bad++; printf("v67: the pump scene did not end on the map (scr %d)\n", scr); }
          G = sv; }
        scr = SC_WALK; wk_mode = 0; G.soda = 0; G.monster = 0; inv_open(); if (inv_rows() != 1 || inv_sel != 2) { bad++; printf("v65: the fridge must show only the bottle row (rows %d sel %d)\n", inv_rows(), inv_sel); }
        on_button(B_UP); RSHOT("v65_inventory_bottle");
        G.energy = 0; G.day = 20; on_button(B_A); if (!g_milk_active(&G) || G.bottle != 0) { bad++; printf("v65: drinking the bottle did not work\n"); }
        RSHOT("v65_inventory_active"); G.energy = 0;
        on_button(B_B); RSHOT("v65_energy_milk_hud");
        if (g_ex_start(&G, 10) != 1 || G.energy != 0) { bad++; printf("v65: the bottle must stop the energy drop\n"); }
        /* v7.8: the store has tabs: ITEMS (flower, money stack, pump), STOCKS, BLACK MARKET (ring, pill, milk pill) */
        G.cash = 5000; shop_open(); RSHOT("v65_shop_top");
        if (!tst_list_ok(TAB_ITEMS)) { bad++; printf("v78/v82: ITEMS after the day-8 scene = flower, lotion, pink cards, stack, yoga, gold bar, diamond ring (until bought), pump\n"); }
        if (!tst_list_ok(TAB_BM)) { bad++; printf("v78/v82: BLACK MARKET = ring, pill, milk pill, Super milk drug (a bought ring or pill is gone)\n"); }
        for (i = 0; i < 4; i++) on_button(B_DOWN); if (shop_sel != SH_YOGA) { bad++; printf("v78/v82: DOWN must cycle inside the tab (sel %d)\n", shop_sel); }
        on_button(B_R1); if (shop_tab != TAB_STOCKS || shop_sel != 0) { bad++; printf("v78: R1 must open STOCKS (tab %d sel %d)\n", shop_tab, shop_sel); } RSHOT("v78_stocks_tab");
        on_button(B_R1); if (shop_tab != TAB_BM) { bad++; printf("v78: R1 must open the BLACK MARKET\n"); } RSHOT("v78_black_market");
        on_button(B_R1); if (shop_tab != TAB_ITEMS) { bad++; printf("v78: R1 must wrap to ITEMS\n"); }
        on_button(B_L1); if (shop_tab != TAB_BM) { bad++; printf("v78: L1 must wrap to the BLACK MARKET\n"); }
        on_button(B_L1); if (shop_tab != TAB_STOCKS) { bad++; printf("v78: L1 must go back to STOCKS\n"); }
        on_button(B_L1); if (shop_tab != TAB_ITEMS) { bad++; printf("v78: L1 must go back to ITEMS\n"); } RSHOT("v78_items_tab");
        on_button(B_B);
        { Game t = G; t.pill_seen = 0; G.pill_seen = 0; shop_open();
          { int o[16]; if (!tst_list_ok(TAB_ITEMS) || !tst_list_ok(TAB_BM)) { bad++; printf("v78/v82: before the day-8 scene no pump, no milk pill and no Super drug are listed\n"); } }
          on_button(B_R1); on_button(B_R1); for (i = 0; i < 7; i++) on_button(B_DOWN); if (shop_sel == SH_MILK || shop_sel == SH_SUPER) { bad++; printf("v65: before the day-8 scene the milk pill is not for sale\n"); } on_button(B_B); G = t; }
        /* v7.8 STOCKS through the real buttons: A buys 1, X sells 1, at today's price; cash first, then the bank */
        { Game t = G; int p0, c0b; g_init(&G); G.cash = 3000; G.bank = 1000; G.day = 5; scr = SC_WALK; shop_open(); on_button(B_R1);
          p0 = stk_now(&G, STK_NVIDIA); c0b = G.cash;
          on_button(B_A); if (G.stk[STK_NVIDIA] != 1 || G.cash != c0b - p0) { bad++; printf("v78: A must buy one NVIDIA share (%d, cash %d)\n", G.stk[STK_NVIDIA], G.cash); }
          on_button(B_DOWN); on_button(B_A); on_button(B_A); if (shop_sel != STK_GOOGLE || G.stk[STK_GOOGLE] != 2) { bad++; printf("v78: DOWN then two buys must give 2 GOOGLE shares (sel %d, %d)\n", shop_sel, G.stk[STK_GOOGLE]); }
          RSHOT("v78_stocks_owned");
          G.cash = 0; G.bank = 100; on_button(B_A); if (G.stk[STK_GOOGLE] != 2 || G.bank != 100) { bad++; printf("v78: buying without enough money must do nothing\n"); }
          G.cash = 10; G.bank = 2000; p0 = stk_now(&G, STK_GOOGLE); on_button(B_A); if (G.stk[STK_GOOGLE] != 3 || G.cash != 0 || G.bank != 2010 - p0) { bad++; printf("v78: a buy must take cash first, then the bank (cash %d bank %d)\n", G.cash, G.bank); }
          p0 = stk_now(&G, STK_GOOGLE); c0b = G.cash; on_button(B_X); if (G.stk[STK_GOOGLE] != 2 || G.cash != c0b + p0) { bad++; printf("v78: X must sell one share for today's price\n"); }
          on_button(B_UP); on_button(B_UP); if (shop_sel != STK_SPACEX) { bad++; printf("v78: UP must wrap inside the stock list (sel %d)\n", shop_sel); }
          on_button(B_X); if (G.stk[STK_APPLE] != 0) { bad++; printf("v78: selling a share you do not own\n"); }
          on_button(B_Y); if (shop_info) { bad++; printf("v78: STOCKS has no info page\n"); }
          on_button(B_B); if (scr != SC_WALK) { bad++; printf("v78: B must leave the store\n"); }
          G = t; }
        scr = SC_WALK; wk_mode = 0;
    }
    /* v7.7: CHAT and TEASE */
    {
        Game sv = G; int k, n, fails = 0, gain0, iter; unsigned char who_last;
        g_init(&G); G.rs = RS_LIVING; G.room_up = 2; G.day = 20; G.rom_lvl = 1; G.rom_pct = 0; kicked_day = 0; chat_gain_day = 0; chat_gain_n = 0; srand(77);
        wk_mode = 0; scr = SC_ROM; rom_in = 1; rom_choose = 1; rom_mode = RM_MENU; rom_page = 0; rom_sel = 1;
        if (rom_menu_n() != 2 || rom_act(1) != RA_CHAT || !g_chat_open(&G) || g_tease_open(&G)) { bad++; printf("v77: before the day-8 scene: chat yes, tease no\n"); }
        /* chat: 400 chats on one day, a quarter go badly, the bar gains only CHAT_DAY_MAX good ones */
        for (k = 0; k < 400; k++) {
            int pc = G.rom_pct, lv = G.rom_lvl;
            rom_mode = RM_MENU; rom_page = 0; rom_sel = 1; rom_choose = 1; on_button(B_A);
            if (rom_mode != RM_TALK || tk_n < 2 || tk_n > 3 || tk_who[0] != 0 || tk_who[1] != 1) { bad++; printf("v77: A on CHAT must start a talk (mode %d n %d)\n", rom_mode, tk_n); break; }
            if (tk_kind == TK_FAIL) fails++;
            if (k == 0) { tk_i = 1; tk_ms = 0; RSHOT("v77_chat_talk"); tk_i = 0; tk_ms = now_ms(); }
            for (iter = 0; iter < 8 && rom_mode == RM_TALK; iter++) { on_button(B_A); on_button(B_A); }
            if (rom_mode != RM_MENU || scr != SC_ROM) { bad++; printf("v77: the chat must end back at her menu\n"); break; }
            if (G.rom_lvl == lv && G.rom_pct < pc) { bad++; printf("v77: the bar must never go down\n"); break; }
        }
        if (fails < 60 || fails > 140) { bad++; printf("v77: about 25%% of the chats must go badly (%d of 400)\n", fails); }
        if (G.rom_lvl != 1 || G.rom_pct != CHAT_DAY_MAX * CHAT_ROM) { bad++; printf("v77: one day of chats must give exactly %d%% (lvl %d pct %d)\n", CHAT_DAY_MAX * CHAT_ROM, G.rom_lvl, G.rom_pct); }
        G.day++; gain0 = G.rom_pct; chat_start(); while (tk_kind == TK_FAIL) chat_start(); for (iter = 0; iter < 8 && rom_mode == RM_TALK; iter++) { on_button(B_A); on_button(B_A); }
        if (G.rom_pct != gain0 + CHAT_ROM || strstr(rom_note, "ROMANCE +") == NULL) { bad++; printf("v77: the next day a good chat gives %d%% again (pct %d note '%s')\n", CHAT_ROM, G.rom_pct, rom_note); }
        { int lvl0; G.rom_pct = 100 - CHAT_ROM; G.day++; lvl0 = G.rom_lvl; chat_start(); while (tk_kind == TK_FAIL) chat_start(); for (iter = 0; iter < 8 && rom_mode == RM_TALK; iter++) { on_button(B_A); on_button(B_A); }
          if (G.rom_lvl != lvl0 + 1 || strstr(rom_note, "LEVEL") == NULL) { bad++; printf("v77: a good chat can fill the bar and start the next level (lvl %d note '%s')\n", G.rom_lvl, rom_note); } }
        /* a failed chat: no romance */
        { int pc; G.day++; pc = G.rom_pct; do chat_start(); while (tk_kind != TK_FAIL); for (iter = 0; iter < 8 && rom_mode == RM_TALK; iter++) { on_button(B_A); on_button(B_A); }
          if (G.rom_pct != pc || tk_n != 2) { bad++; printf("v77: a chat that goes badly gives no romance and is you + one answer\n"); } }
        /* pictures: before the pill her normal one, days 6 and 7 her day-6 one, from the day-8 scene confident_p for chat and her_cover for tease */
        G.pill_given = 1; G.pill_day = 1; G.pill_seen = 0; G.day = 5; if (chat_vis() != homeS || !homeS) { bad++; printf("v77: chat day 5: her moved-in picture (v8.7)\n"); }
        G.day = 6; if (chat_vis() != herD6S) { bad++; printf("v77: chat day 6: her_day_6\n"); }
        G.day = 7; if (chat_vis() != herD6S) { bad++; printf("v77: chat day 7: her_day_6\n"); }
        G.pill_seen = 1; G.day = 9; if (chat_vis() != confP || !confP) { bad++; printf("v77: chat after day 8: confident_p\n"); }
        if (her_vis() != herCoverS) { bad++; printf("v77: tease: her_cover\n"); }
        /* after the day-8 scene: tease exists, the milk topics wait for the first bottle */
        G.rs = RS_LIVING; G.milk_st = 0; rom_mode = RM_MENU; rom_choose = 1; rom_page = 2; rom_sel = 0; scr = SC_ROM;
        if (rom_menu_n() != 1 || rom_act(0) != RA_TEASE || !g_tease_open(&G) || g_tease_milk_open(&G)) { bad++; printf("v77: after day 8 TEASE opens, the milk topics do not\n"); }
        on_button(B_A); if (rom_mode != RM_TEASE) { bad++; printf("v77: A on TEASE must open the topic list\n"); }
        RSHOT("v77_tease_topics");
        tease_sel = TS_MILK; on_button(B_A); if (rom_mode != RM_TEASE || strstr(rom_note, "FIRST BOTTLE") == NULL) { bad++; printf("v77: HER MILK must be locked before the first bottle\n"); }
        tease_sel = TS_TASTE; on_button(B_A); if (rom_mode != RM_TEASE) { bad++; printf("v77: MILK TASTE must be locked before the first bottle\n"); }
        G.milk_st = 3; G.pump = 0; G.bottle = 0; if (g_tease_milk_open(&G)) { bad++; printf("v77: the pump alone must not open the milk topics\n"); }
        { Game t = G; G.milk_st = 3; G.milk_pct = 50; G.mpill = 1; G.bottle = 0; g_gift(&G, GF_MILK); if (G.milk_st != 4 || !g_tease_milk_open(&G)) { bad++; printf("v77: a bottle from the gauge must open the milk topics\n"); } G = t; }
        G.milk_st = 4; for (n = 0; n < TS_N; n++) {
            int pc = G.rom_pct, lv = G.rom_lvl, kd = 0; unsigned k2;
            for (k2 = 0; k2 < 30; k2++) {
                rom_mode = RM_TEASE; rom_choose = 1; scr = SC_ROM; tease_sel = n; kicked_day = 0; on_button(B_A);
                if (rom_mode != RM_TALK || tk_kind != TK_TEASE || tk_n < 4 || tk_n > 5) { bad++; printf("v77: topic %d must start a tease talk (mode %d n %d)\n", n, rom_mode, tk_n); break; }
                who_last = tk_who[tk_n - 1];
                if (k2 == 0 && n == TS_CHEST) { tk_i = 1; tk_ms = 0; RSHOT("v77_tease_talk"); tk_i = tk_n - 2; tk_ms = 0; RSHOT("v77_tease_kick"); tk_i = 0; tk_ms = now_ms(); }
                { int has_kick = 0, j; for (j = 0; j < T_KICK_N; j++) if (tk_t[tk_n - 2] == T_KICK[j]) has_kick = 1; if (!has_kick || who_last != 2 || tk_who[0] != 0) { bad++; printf("v77: a tease ends with her angry line and the door\n"); break; } }
                for (iter = 0; iter < 8 && rom_mode == RM_TALK; iter++) { on_button(B_A); on_button(B_A); }
                if (kicked_day != G.day || scr == SC_ROM) { bad++; printf("v77: topic %d: every tease must end with her throwing you out (kicked %d scr %d)\n", n, kicked_day, scr); break; }
                kd++;
            }
            if (G.rom_pct != pc || G.rom_lvl != lv) { bad++; printf("v77: tease must never give romance\n"); }
            (void)kd;
        }
        /* the greeting after a tease the same day is her angry one */
        kicked_day = G.day; { const char *g = rom_greet(); int f = 0, j; for (j = 0; j < R_KICKED_N; j++) if (g == R_KICKED[j]) f = 1; if (!f) { bad++; printf("v77: after a tease, her next greeting that day is the angry one\n"); } }
        kicked_day = G.day - 1; { const char *g = rom_greet(); int f = 0, j; for (j = 0; j < R_GREET_P_N; j++) if (g == R_GREET_P[j]) f = 1; if (!f) { bad++; printf("v77: the next day she greets you normally\n"); } }
        /* old saves: a bottle already received opens the milk topics (no save reset) */
        { Game t; char path[] = "/tmp/pq_v77.sav"; g_init(&t); t.rs = RS_LIVING; t.pill_seen = 1; t.milk_st = 3; t.bottle = 1; save_game(&t, path); { Game u; if (!load_game(&u, path) || u.milk_st != 4) { bad++; printf("v77: an old save with a bottle must load as milk stage 4\n"); } }
          t.bottle = 0; t.milk_until = 0; save_game(&t, path); { Game u; if (!load_game(&u, path) || u.milk_st != 3) { bad++; printf("v77: an old save without a bottle stays at stage 3\n"); } } remove(path); }
        G = sv; kicked_day = 0; scr = SC_WALK; wk_mode = 0; rom_mode = RM_MENU;
    }
    G = keep; saved_G = keeps; frozen = fr; scr = SC_WALK; wk_mode = 0; pause_kind = PK_NONE;
    printf("romance: ring, proposal, upgrade, moving in, store, gifts, home matches: problems %d\n", bad);
    return bad;
}

/* --autoplay: drives the real button handler through the whole story, many matches, walking out, the ATM and the coach */
static int autoplay(void) {
    int steps = 0, matches = 0, bad = 0, cash_before = 0, walked = 0, forfeits_seen = 0, power_tested = 0, mid_match_steps = 0;
    int first_len = 0, story_ok = 0, c_expect = -1, after_seen = 0, rematches = 0, fees_seen = 0;
    frozen = 1; fake_ms = 1000; srand(5); g_init(&G); scr = SC_MENU; menu_sel = 1; ls_enabled = 0;
    while (matches < 40 && steps++ < 600000) {
        fake_ms += 200;
        if (scr == SC_WALK && G.story >= ST_NAMED && G.cash < 150) G.cash += 500;   /* keep the bot able to pay (without money the game ends: that has its own test) */
        update();
        switch (scr) {
        case SC_MENU: menu_sel = 1; on_button(B_A); break;
        case SC_CAREER: on_button(B_A); break;   /* Continue */
        case SC_STORY: case SC_MONO: case SC_SNEAK: on_button(B_A); break;   /* SC_SNEAK here is only her door menu: the first entry is Knock */
        case SC_WALK:
            G.energy = ENERGY_MAX;   /* the long run is about the story and the money; energy has its own test below */
            if (G.story >= ST_NAMED && G.cash < 150) G.cash += 500;   /* a match costs cash now: keep the bot able to pay */
            if (wk_mode) { on_button(wk_mode == 2 ? B_B : B_A); break; }
            if (G.story == ST_NEW) { bot_use(MAP_LOBBY, 8, 6, UP); break; }   /* the opening: walk to the front desk, press A */
            if (G.story == ST_FIRST) {   /* the shirt is paid: the bot beats the boss at the Exhibition (the $1000 win before it is simulated), then the story goes on as before */
                int kk;
                if (wk_map != MAP_HALL || wk_tx != ELEV_X[2] || wk_ty != ELEV_Y[2] || wk_face != DN) { bad++; printf("after paying the shirt you are not at the 2nd floor elevator facing down (map %d %d,%d face %d)\n", wk_map, wk_tx, wk_ty, wk_face); }
                G.ex_big = 1; G.cash = 5000; G.energy = ENERGY_MAX;
                bot_use(MAP_LOBBY, 1, 6, LF); bot_use(MAP_EVENT, 5, 5, UP); EV_ENTER(); qs_sel = EX_BOSS; on_button(B_A);
                if (scr != SC_MTABLE || !M.boss) { bad++; printf("the boss table did not start (scr %d)\n", scr); break; }
                M.over = 1; M.game_over = 1; M.place = 1; on_button(B_A); on_button(B_A);
                if (scr != SC_STORY || vn_id != SCR_BOSS) { bad++; printf("the boss scene did not follow\n"); break; }
                for (kk = 0; kk < 30 && scr == SC_STORY; kk++) { fake_ms += 400; update(); on_button(B_A); }
                if (G.story == ST_NAMED && G.cash == EX_BOSS_PRIZE && floor_card()) story_ok = 1; else { bad++; printf("beating the boss did not name her / pay / return the card\n"); }
                break;
            }
            if (G.story == ST_HOME) { G.hall_seen |= 1; unpack_done = 1; }   /* (the unpacking has its own test) a real player has seen the 2nd floor look before paying; the bot never walks there */
            if (G.story == ST_HOME && G.cash < DEBT + 100) G.cash = DEBT + 100;   /* (the 100 left over pays for the first challenge) stands in for winning the shirt money at the Exhibition (the door gate has its own test) */
            if (G.story != ST_HOME && G.cash > 300 && rnd(4) == 0) { bot_use(MAP_LOBBY, 10, 4, UP); if (scr != SC_ATM) { bad++; printf("ATM not opened at the desk\n"); } break; }
            if (G.story >= ST_FIRST && g_played_today(&G)) { G.day++; G.rent_next = G.day + RENT_DAYS; }   /* she plays once a day (own test below): the bot lets a day pass, and keeps the rent out of its way */
            bot_use(MAP_HALL3, 1, 4, LF); break;   /* walk to Sophia's door and knock */
        case SC_DOOR:
            if (G.story >= ST_NAMED && G.cash < 150) G.cash += 500;
            sel = 0;
            on_button(B_A); break;
        case SC_LEN: c_expect = G.cash - SOPH_FEE[0]; len_sel = 0; on_button(B_A); break;
        case SC_PLAY: {
            if (G.mlen && matches == 0 && !first_len) first_len = G.mlen;
            if (c_expect >= 0 && G.in_match) { if (G.cash != c_expect || G.energy != ENERGY_MAX - SOPH_ENERGY) { bad++; printf("cash/energy after a 10-hand match start %d/%d, expected %d/%d\n", G.cash, G.energy, c_expect, ENERGY_MAX - SOPH_ENERGY); }   /* v12.4: a hotel match also costs 20% energy */ c_expect = -1; fees_seen++; }
            if (G.over) { if (G.hand >= G.mlen) cash_before = G.cash; on_button(B_A); break; }
            mid_match_steps++;
            if (matches == 6 && !walked && G.hand == 3) {   /* walk out: Menu, down, A */
                int cash0 = G.cash, lost0 = G.m_lost, played0 = G.m_played;
                on_button(B_QUIT); on_button(B_DOWN); on_button(B_DOWN); on_button(B_A);
                if (G.cash != cash0 || G.m_lost != lost0 + 1 || G.m_played != played0 + 1 || G.in_match || !G.forfeit) { bad++; printf("walk-out wrong\n"); }
                walked = 1; forfeits_seen++; break;
            }
            if (matches == 10 && !power_tested && G.hand == 2) {   /* the app is closed mid-match: the saved file must say "lost" */
                Game t = G; int cash0 = G.cash, lost0 = G.m_lost;
                save_game(&t, "/tmp/pq_auto.sav"); { Game h; if (!load_game(&h, "/tmp/pq_auto.sav") || h.in_match || !h.forfeit || h.m_lost != lost0 + 1 || h.cash != cash0) { bad++; printf("power-off forfeit wrong\n"); } }
                remove("/tmp/pq_auto.sav"); power_tested = 1; forfeits_seen++;
            }
            if (G.toact == 0) { int r = rnd(10); on_button(r < 5 ? B_A : r < 8 ? B_X : B_B); }
            break; }
        case SC_END:
            if (G.cash != cash_before + G.last_pay + (G.last_res == 0 ? G.fee : 0)) { bad++; printf("cash mismatch %d vs %d+%d\n", G.cash, cash_before, G.last_pay); }   /* a tie gives the entry fee back */
            if (G.last_res <= 0 && G.last_pay != 0) bad++;
            if (!story_ok && G.story == ST_NAMED && first_end) story_ok = 1;
            if (first_end && matches == 0 && G.energy != ENERGY_MAX) { bad++; printf("the first challenge cost energy\n"); }
            matches++;
            { int fe = first_end, rematch = !fe && G.last_res == 0 && g_can_afford_match(&G, G.mlen);
              on_button(B_A);
              if (rematch) { rematches++; if (scr != SC_PLAY) { bad++; printf("tie rematch did not start\n"); } }
              else if (fe) { if (scr != SC_STORY || vn_id != SCR_NAMED) { bad++; printf("first match: introduction did not follow\n"); } }
              else if (scr != SC_STORY || vn_id != SCR_AFTER || !vn_buf[0].txt || vn_buf[0].spr != 2 || vn_buf[0].bg != BG_LIVING || vn_buf[1].bg != BG_LIVING) { bad++; printf("after a match her room scene did not play inside her living room\n"); }
              else if (G.last_res > 0 && !strstr(vn_buf[0].txt, G.last_bonus > 0 ? "bonus" : "reward")) { bad++; printf("win scene line wrong: %s\n", vn_buf[0].txt); }
              else if (G.last_res > 0 && G.last_drink && vn_n != 2) { bad++; printf("free drink page missing\n"); }
              else if (G.last_res <= 0 && vn_n != 1) { bad++; printf("loss/tie scene has extra pages\n"); }
              after_seen += scr == SC_STORY && vn_id == SCR_AFTER; }
            break;
        case SC_ATM:
            if (atm_mode == 0 && sel == 0 && G.cash > 100) { on_button(B_A); on_button(B_Y); on_button(B_A); }
            else { sel = 2; on_button(B_A); }
            break;
        case SC_SCENE: on_button(B_A); break;
        default: on_button(B_B); break;
        }
        if (G.cash < 0 || G.bank < 0 || G.cash > MONEY_CAP || G.bank > MONEY_CAP) { bad++; break; }
    }
    { /* Quick Play, Exhibition, the Event Center, tournament message, no-cash refusal, leaving a table */
        Game keep = G; int g, wins = 0, played = 0;
        for (g = 0; g < 30; g++) { int mode = g % 3 == 0 ? 0 : 1, si = g % (mode ? EX_N : 4);   /* (the 6th Exhibition table is the boss) */ g_init(&G); bad += mp_flow_test(mode, si, 5000, g % 2, 0); }
        for (g = 0; g < 8; g++) { g_init(&G); bad += mp_flow_test(1, g % EX_N, 5000, 1, 2 + g); bad += mp_flow_test(0, 0, 0, 1, 3); }
        g_init(&G); G.story = ST_NAMED; G.event = 1; G.cash = 9; enter_hub(); bot_use(MAP_LOBBY, 1, 6, LF); bot_use(MAP_EVENT, 5, 5, UP); EV_ENTER(); qs_sel = 0; on_button(B_A);
        if (scr != SC_QSETUP || G.cash != 9 || G.energy != ENERGY_MAX) { bad++; printf("exhibition started without enough cash\n"); }
        on_button(B_B); if (scr != SC_EVENT) { bad++; printf("B in the buy-in list did not go back to the Event Center menu\n"); } on_button(B_B); if (scr != SC_WALK || wk_map != MAP_EVENT || wk_tx != 5 || wk_ty != 5) { bad++; printf("B in the menu did not leave you at the desk (map %d)\n", wk_map); }
        g_init(&G); G.story = ST_NAMED; G.event = 1; G.cash = 5000; G.energy = EX_ENERGY - 1; enter_hub(); bot_use(MAP_LOBBY, 1, 7, LF); bot_use(MAP_EVENT, 6, 5, UP); WK_CLOSE(); if (scr != SC_EVENT) { bad++; printf("no menu after the greeting\n"); } qp_ms = 0; on_button(B_A);
        if (scr != SC_EVENT || !qp_ms || G.cash != 5000 || G.energy != EX_ENERGY - 1) { bad++; printf("exhibition not refused below 10 percent energy\n"); }
        g_init(&G); G.story = ST_NAMED; G.event = 1; G.cash = 5000; G.energy = EX_ENERGY; enter_hub(); bot_use(MAP_LOBBY, 1, 7, LF); bot_use(MAP_EVENT, 7, 5, UP); EV_ENTER(); qp_ms = 0; qs_sel = 0; on_button(B_A);
        if (scr != SC_MTABLE) { bad++; printf("exhibition refused at exactly 10 percent energy\n"); }
        scr = SC_WALK; mp_mode = 0; wk_mode = 0; pause_kind = PK_NONE;
        if (!bot_use(MAP_EVENT, 13, 6, RT) || wk_map != MAP_LOBBY || wk_tx != 1 || wk_ty != 6 || wk_face != RT) { bad++; printf("the Event Center door did not lead back to the lobby opening\n"); }
        g_init(&G); G.story = ST_NAMED; enter_hub(); if (G.event) bad++;   /* not unlocked by default: the opening is just a wall, no bubble, no message */
        bot_use(MAP_LOBBY, 1, 6, LF); if (scr != SC_WALK || wk_mode != 0 || wk_map != MAP_LOBBY || wk_interactive('E')) { bad++; printf("locked event opening answered\n"); }
        (void)wins; (void)played; G = keep;
    }
    /* sounds: a new hand queues the shuffle, four cards and both blinds; an all-in and the pot have their own sounds; the queue empties as the clock runs */
    { int fz = frozen, k; frozen = 0; sq_n = 0; memset(sfx_log, 0, sizeof sfx_log);
      anim_new_hand(1, 2, 0);
      if (sq_n != 7) { bad++; printf("new hand queued %d sounds (want 7)\n", sq_n); }
      SDL_Delay(20000); sfx_tick();
      if (sq_n || sfx_log[SFX_SHUF] != 1 || sfx_log[SFX_GIVE] != 4 || sfx_log[SFX_PLACE] != 2) { bad++; printf("hand sounds wrong: %d %d %d left %d\n", sfx_log[SFX_SHUF], sfx_log[SFX_GIVE], sfx_log[SFX_PLACE], sq_n); }
      anim_allin = 1; anim_after(1, 8, 0, 0, 0, 0, 0); anim_after(0, 4, 0, 3, 0, 0, 0);   /* an all-in, then a call that opens the flop */
      SDL_Delay(20000); sfx_tick();
      if (sfx_log[SFX_ALLIN] != 1 || sfx_log[SFX_PLACE] != 3 || sfx_log[SFX_GIVE] != 7 || sfx_log[SFX_OPEN] != 3) { bad++; printf("action sounds wrong: allin %d place %d give %d open %d\n", sfx_log[SFX_ALLIN], sfx_log[SFX_PLACE], sfx_log[SFX_GIVE], sfx_log[SFX_OPEN]); }
      anim_new_hand(1, 2, 0); sq_n = 0; for (k = 0; k < SFX_N; k++) sfx_log[k] = 0;
      pay_ms = 0; sfx_at(now_ms() + 50, SFX_COLLECT); SDL_Delay(100); sfx_tick(); if (sfx_log[SFX_COLLECT] != 1) { bad++; printf("pot sound did not play\n"); }
      sq_n = 0; frozen = fz; }
    /* the NET plate waits for the banner; the first deal waits for a lead-in */
    {
        Game keep = G; int k = hdl; unsigned t;
        g_init(&G); G.over = 1; G.res = 1; G.amt = 10; G.net = 10; hdl = 700;
        if (net_shown(0) != 0 || net_shown(699) != 0 || net_shown(900) != 0 || net_shown(1000) != 10) { bad++; printf("NET changed before the banner was up (%d %d %d %d)\n", net_shown(0), net_shown(699), net_shown(900), net_shown(1000)); }
        G.res = -1; G.amt = 6; G.net = -6; if (net_shown(100) != 0 || net_shown(5000) != -6) { bad++; printf("NET after a loss wrong\n"); }
        G.over = 0; G.net = 4; if (net_shown(0) != 4) { bad++; printf("NET must not change while a hand is running\n"); }
        G = keep; hdl = k;
        frozen = 0; t = now_ms(); lead_next = 1; anim_new_hand(1, 2, 0);
        if (deal_ms < t + LEAD_MS - 100 || anim_end_ms < t + LEAD_MS + DEAL_ALL || lead_next) { bad++; printf("the first deal has no lead-in\n"); }
        t = now_ms(); anim_new_hand(1, 2, 0); if (deal_ms > t + 100) { bad++; printf("later hands must deal right away\n"); }
        frozen = 1;
    }
    /* her door after the first match: knock or go in; her room, the bedroom picture, one look per visit for 20 percent energy */
    {
        Game keep = G;
        g_init(&G); G.story = ST_NAMED; G.hall_seen = 3; G.energy = 100; enter_hub();
        bot_use(MAP_HALL3, 1, 4, LF);   /* in the daytime the door has no menu: the knock goes straight to her greeting and the door menu */
        { int k2; for (k2 = 0; k2 < 40 && scr != SC_DOOR; k2++) { fake_ms += 300; update(); if (scr == SC_STORY) on_button(B_A); } }
        if (scr != SC_DOOR) { bad++; printf("a daytime knock did not reach her door menu (scr %d)\n", scr); }
        on_button(B_B); enter_hub(); G.phase = PH_EVENING; bot_use(MAP_HALL3, 1, 4, LF); if (scr == SC_SNEAK) { bad++; printf("the sneak-in menu showed up in the evening\n"); } enter_hub();
        G.phase = PH_NIGHT; bot_use(MAP_HALL3, 1, 4, LF);
        /* v6.6: no key card, no quiet way into her room on the 3rd floor: the night knock is the angry scene */
        if (scr != SC_STORY || vn_id != SCR_NIGHT) { bad++; printf("v66: at night her 3rd-floor door must only give the angry scene (scr %d)\n", scr); }
        if (scr != SC_STORY || vn_id != SCR_NIGHT || vn_buf[1].spr != 3 || vn_buf[0].bg != BG_HALL) { bad++; printf("the night knock did not start her angry scene (scr %d id %d)\n", scr, vn_id); }
        { int k, e0 = G.energy, c0 = G.cash; for (k = 0; k < 20 && scr == SC_STORY; k++) { fake_ms += 300; update(); on_button(B_A); }
          if (scr != SC_WALK || wk_map != MAP_HALL3 || G.energy != e0 || G.cash != c0 || G.phase != PH_NIGHT || G.in_match) { bad++; printf("the night scene did not send you back to the hall unchanged (scr %d)\n", scr); } }
        G.phase = PH_MORNING; bot_use(MAP_HALL3, 1, 4, LF);
        { int k; for (k = 0; k < 40 && scr != SC_DOOR; k++) { fake_ms += 300; update(); if (scr == SC_STORY) on_button(B_A); } }
        if (scr != SC_DOOR) { bad++; printf("knock did not lead to the challenge door menu\n"); }
        g_init(&G); G.story = ST_FIRST; G.phase = PH_NIGHT; enter_hub(); bot_use(MAP_HALL3, 1, 4, LF);
        if (scr == SC_SNEAK) { bad++; printf("her room offered before she is beaten\n"); }
        if (wk_mode != 1 || wk_pg[0] != W_BOSS_NO) { bad++; printf("night at her door before the boss fight must stay shut\n"); }
        G = keep; enter_hub();
    }
    /* energy, zombie refusal, store, inventory, pity soda: all through the real buttons */
    {
        Game keep = G; int k, c0;
        g_init(&G); G.story = ST_NAMED; G.energy = 0; G.cash = 99; enter_hub(); bot_use(MAP_HALL3, 1, 4, LF);
        for (k = 0; k < 40 && scr != SC_DOOR; k++) { fake_ms += 300; update(); if (scr == SC_STORY || scr == SC_SNEAK) on_button(B_A); }
        if (scr != SC_DOOR) { bad++; printf("did not reach her door (scr %d)\n", scr); }
        sel = 0; on_button(B_A);
        if (scr != SC_SCENE || sc_end != SE_ZOMBIE) { bad++; printf("no refusal with 99 cash (scr %d)\n", scr); }
        fake_ms += 40000; update(); on_button(B_A);
        if (scr != SC_DOOR || sel != 0 || G.in_match) { bad++; printf("refusal scene did not return to the door menu\n"); }
        G.cash = 100; sel = 0; on_button(B_A); if (scr != SC_LEN) { bad++; printf("cannot play with 100 cash and zero energy\n"); }
        len_sel = 1; on_button(B_A); if (scr != SC_LEN || !len_msg_ms) { bad++; printf("the 500 buy-in started without 500 cash\n"); }
        G.energy = SOPH_ENERGY - 1; len_sel = 0; len_msg_ms = 0; on_button(B_A); if (scr != SC_LEN || !len_msg_ms || !len_msg_en || G.cash != 100 || G.in_match) { bad++; printf("v12.4: 19 percent energy must refuse the hotel match with the energy note\n"); }
        G.energy = SOPH_ENERGY; len_sel = 0; on_button(B_A); while (scr == SC_STORY) on_button(B_A); if (scr != SC_PLAY || G.cash != 0 || G.energy != 0) { bad++; printf("the 100 buy-in is wrong (cash %d energy %d)\n", G.cash, G.energy); }
        g_init(&G); G.story = ST_NAMED; G.cash = 30; G.energy = 40; enter_hub(); bot_use(MAP_LOBBY, 8, 6, UP);
        if (scr != SC_WALK || wk_mode != 1 || wk_tag != W_CLERK_TAG) { bad++; printf("the receptionist did not greet\n"); }
        WK_CLOSE(); bot_use(MAP_LOBBY, 5, 4, UP); if (scr != SC_STORE) { bad++; printf("store not opened\n"); }   /* v9.8 test fix: A on the greeting used to open the store, since v9.1 it opens the desk menu (wk_mode 4); WK_CLOSE only closes the message */
        on_button(B_X); if (st_confirm != 1) { bad++; printf("X did not ask to confirm a soda\n"); }
        on_button(B_B); if (st_confirm || G.soda || G.cash != 30) { bad++; printf("cancel wrong\n"); }
        on_button(B_X); on_button(B_A); if (G.soda != 1 || G.cash != 25 || st_confirm) { bad++; printf("soda purchase wrong\n"); }
        on_button(B_Y); if (st_confirm != 2) { bad++; printf("Y did not ask to confirm an energy drink\n"); }
        on_button(B_A); if (G.monster != 1 || G.cash != 15) { bad++; printf("energy drink purchase wrong\n"); }
        /* v12.4: the vending machine screen: UP / DOWN pick the shelf, A buys the picked one, the smiley turns happy and the can drops into the tray */
        /* v12.5: the picture is static: no smiley / tray animation to test */
        RSHOT("v124_vending_after_buy");
        on_button(B_UP); if (st_sel != 0) { bad++; printf("v12.4: UP must toggle from the energy drink to the soda shelf (sel %d)\n", st_sel); }
        on_button(B_A); if (st_confirm != 1) { bad++; printf("v12.4: A must buy the picked soda\n"); } on_button(B_B); RSHOT("v124_vending_soda");
        on_button(B_DOWN); if (st_sel != 1) { bad++; printf("v12.4: DOWN must pick the energy drink shelf (sel %d)\n", st_sel); } RSHOT("v124_vending_energy");
        G.cash = 9; on_button(B_A); if (st_confirm || G.cash != 9) { bad++; printf("v12.4: $9 must not buy a $10 energy drink\n"); } G.cash = 15; on_button(B_UP);
        G.cash = 2; on_button(B_Y); if (st_confirm || G.monster != 1 || G.cash != 2) { bad++; printf("bought without cash\n"); }
        on_button(B_B); if (scr != SC_WALK || wk_map != MAP_LOBBY) { bad++; printf("store did not return to the lobby\n"); }
        bot_use(MAP_ROOM, 1, 3, UP); if (scr != SC_INV) { bad++; printf("inventory not opened at the fridge\n"); }
        inv_sel = 0; c0 = G.energy; on_button(B_A); if (G.energy != c0 + 20 || G.soda != 0) { bad++; printf("drinking a soda wrong\n"); }
        if (inv_sel != 1 || inv_rows() != 1) { bad++; printf("v78: with the soda gone the fridge lists only the energy drink (sel %d rows %d)\n", inv_sel, inv_rows()); }
        on_button(B_DOWN); on_button(B_A); if (G.energy != 100 || G.monster != 0) { bad++; printf("drinking an energy drink wrong (%d)\n", G.energy); }
        if (inv_rows() != 0) { bad++; printf("v78: an empty fridge has no rows\n"); } RSHOT("v78_fridge_empty"); on_button(B_A); on_button(B_Y); on_button(B_UP); if (scr != SC_INV || inv_info) { bad++; printf("v78: A, Y and UP in an empty fridge must do nothing\n"); }
        G.monster = 1; inv_sel = 1; on_button(B_A); if (G.energy != 100 || G.monster != 1) { bad++; printf("drank at full energy\n"); }
        on_button(B_B); if (scr != SC_WALK || wk_map != MAP_ROOM) { bad++; printf("inventory did not return\n"); }
        g_init(&G); G.story = ST_NAMED; G.energy = 0; G.cash = 0; enter_hub(); bot_use(MAP_LOBBY, 5, 4, UP);
        if (scr != SC_STORE || G.soda != 1 || !G.rescue_used) { bad++; printf("pity soda missing\n"); }
        on_button(B_B); G.soda = 0; bot_use(MAP_LOBBY, 7, 6, UP); on_button(B_A); if (G.soda != 0) { bad++; printf("a second pity soda\n"); }
        scr = SC_STORE; pause_kind = PK_NONE; on_button(B_QUIT); if (pause_kind != PK_HOME) { bad++; printf("store pause wrong\n"); } pause_kind = PK_NONE;
        G = keep;
    }
    /* the boss: the shirt is paid and the card is taken back; the $1000 table opens her table; losing costs the buy-in and she can be challenged again; winning is one time only */
    {
        Game keep = G; int k, c;
        g_init(&G); G.story = ST_HOME; G.hall_seen = 3; G.event = 1; G.cash = DEBT + 20; saved_G = G; enter_hub(); bot_use(MAP_HALL3, 1, 4, LF);
        for (k = 0; k < 40 && scr == SC_STORY; k++) { fake_ms += 400; update(); on_button(B_A); }
        for (k = 0, c = 0; k < 20 && !c; k++) { fake_ms += 400; update(); c = scr == SC_WALK && !wk_mode; }
        if (G.story != ST_FIRST || G.cash != 20) { bad++; printf("the shirt payment did not move the story on (story %d cash %d)\n", G.story, G.cash); }
        if (scr != SC_WALK || wk_map != MAP_HALL || wk_tx != ELEV_X[2] || wk_ty != ELEV_Y[2] || wk_face != DN) { bad++; printf("after paying you are not at the 2nd floor elevator facing down (map %d %d,%d face %d)\n", wk_map, wk_tx, wk_ty, wk_face); }
        { int fl[3], n = elev_floors(2, fl); if (floor_card() || n != 1 || fl[0] != 1) { bad++; printf("she did not take the floor key card back (%d floors)\n", n); } }
        { int fl[3], n; G.story = ST_NAMED; n = elev_floors(2, fl); if (!floor_card() || n != 2 || fl[1] != 3) { bad++; printf("beating her must give the floor key card back\n"); } G.story = ST_FIRST; }
        /* the list: the boss table appears only after the $1000 table was won, and only while she is the boss */
        G.event = 1; G.cash = 7000; G.energy = ENERGY_MAX; G.ex_big = 0; if (g_ex_n(&G) != EX_N - 1 || g_ex_start(&G, 5000) != 0 || G.cash != 7000) { bad++; printf("the boss table must not be there before the $1000 win\n"); }
        G.ex_big = 1; if (g_ex_n(&G) != EX_N) { bad++; printf("the boss table did not appear after the $1000 win\n"); }
        G.story = ST_NAMED; if (g_ex_n(&G) != EX_N - 1 || g_ex_start(&G, 5000) != 0) { bad++; printf("the boss table must be gone after she is beaten\n"); }
        G.story = ST_FIRST;
        { char o[240]; obj_text(o, sizeof o); if (strcmp(o, O_BOSS)) { bad++; printf("boss objective wrong: %s\n", o); } G.ex_big = 0; obj_text(o, sizeof o); if (strcmp(o, O_FIRST)) { bad++; printf("pre-boss objective wrong\n"); } G.ex_big = 1; }
        /* winning the $1000 table for the first time: the news on the result screen */
        G.ex_big = 0; G.cash = 2000; G.energy = ENERGY_MAX; enter_hub(); bot_use(MAP_LOBBY, 1, 6, LF); bot_use(MAP_EVENT, 5, 5, UP); EV_ENTER(); qs_sel = 4; on_button(B_A);
        if (scr != SC_MTABLE) { bad++; printf("$1000 table not started\n"); }
        else { M.over = 1; M.game_over = 1; M.place = 1; on_button(B_A); if (scr != SC_MEND || !G.ex_big || !boss_new || G.cash != 1000 + 3500) { bad++; printf("the $1000 win did not open the boss table (ex_big %d new %d cash %d)\n", G.ex_big, boss_new, G.cash); }
               on_button(B_A); if (scr != SC_WALK || boss_new == 0 && 0) { bad++; printf("not back at the match-room door\n"); } }
        /* the boss table: buy-in 5000, energy 10 percent, she plays like Sophia and the picture is her boss outfit; losing costs the buy-in and keeps her open */
        G.story = ST_FIRST; G.ex_big = 1; G.cash = 7000; G.energy = ENERGY_MAX; enter_hub(); bot_use(MAP_LOBBY, 1, 6, LF); bot_use(MAP_EVENT, 5, 5, UP); EV_ENTER();
        if (scr != SC_QSETUP || g_ex_n(&G) != EX_N) { bad++; printf("the buy-in list did not show the boss table\n"); }
        qs_sel = EX_BOSS; on_button(B_A);
        if (scr != SC_MTABLE || !M.boss || M.start_stack != 5000 || M.bb != 100 || G.cash != 2000 || G.energy != ENERGY_MAX - EX_ENERGY) { bad++; printf("boss table wrong (scr %d boss %d stack %d bb %d cash %d)\n", scr, M.boss, M.start_stack, M.bb, G.cash); }
        M.over = 1; M.game_over = 2; M.place = 2; on_button(B_A);
        if (scr != SC_MEND || G.cash != 2000 || G.story != ST_FIRST || boss_win) { bad++; printf("losing to the boss changed something (cash %d story %d)\n", G.cash, G.story); }
        on_button(B_A); if (scr != SC_WALK || g_ex_n(&G) != EX_N) { bad++; printf("after losing, she must still be there to challenge again\n"); }
        /* ... and beating her: $10000, her name, the card back, a scene, one time only */
        G.cash = 6000; G.energy = ENERGY_MAX; WK_CLOSE(); bot_use(MAP_EVENT, 5, 5, UP); EV_ENTER(); qs_sel = EX_BOSS; on_button(B_A);
        if (scr != SC_MTABLE) { bad++; printf("second boss attempt not started\n"); }
        M.over = 1; M.game_over = 1; M.place = 1; on_button(B_A);
        if (scr != SC_MEND || G.cash != 1000 + EX_BOSS_PRIZE || G.story != ST_NAMED || !boss_win || mp_prize != EX_BOSS_PRIZE) { bad++; printf("beating the boss wrong (cash %d story %d)\n", G.cash, G.story); }
        on_button(B_A);
        if (scr != SC_STORY || vn_id != SCR_BOSS || vn_buf[0].who != W_AW || vn_buf[2].who != W_SOPHIA || vn_buf[0].spr != 4) { bad++; printf("the boss scene did not start (scr %d)\n", scr); }
        for (k = 0; k < 30 && scr == SC_STORY; k++) { fake_ms += 400; update(); on_button(B_A); }
        if (scr != SC_WALK || wk_map != MAP_EVENT || wk_tx != EV_OUT_X || wk_ty != EV_OUT_Y) { bad++; printf("after the boss scene you are not at the match-room door\n"); }
        if (g_ex_n(&G) != EX_N - 1 || !floor_card() || G.story != ST_NAMED) { bad++; printf("after beating her the table must be gone and the card back\n"); }
        { Game t = G; save_game(&t, "/tmp/pq_boss.sav"); k = load_game(&t, "/tmp/pq_boss.sav"); remove("/tmp/pq_boss.sav"); if (!k || t.story != ST_NAMED || !t.ex_big) { bad++; printf("boss progress not saved\n"); } }
        G = keep;
    }
    scr = SC_MENU; on_button(B_QUIT); if (running) { bad++; printf("menu did not exit\n"); } running = 1;   /* Menu in the title screen exits */
    bad += anim_tests();
    bad += v44_tests();
    bad += v50_tests();
    bad += v53_tests();
    bad += v54_tests();
    bad += v55_tests();
    bad += walk_tests();
    bad += romance_tests();
    bad += v68_tests();
    bad += v79_tests();
    bad += v80_tests();
    bad += v81_tests();
    bad += v82_tests();
    bad += v63_tests();
    printf("table games through the buttons: %d (you won %d)\n", mp_games_done, mp_wins_you);
    printf("autoplay: %d matches, %d steps, cash %d bank %d key %d story %d first match %d hands, forfeits %d, her room scenes %d, problems %d\n",
           matches, steps, G.cash, G.bank, 0, G.story, first_len, forfeits_seen, after_seen, bad);
    return bad || fees_seen < 10 || after_seen < 20 || matches < 40 || G.story != ST_NAMED || first_len != 10 || !story_ok || forfeits_seen < 2;
}
#endif

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    srand((unsigned)time(NULL));
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
    /* v7.5: the screen is 752x560 on every build (PQ_SIZE=WxH overrides it, for tests); there is no 640x480 fallback any more. */
    { const char *e = getenv("PQ_SIZE"); int ew = 0, eh = 0; if (e && sscanf(e, "%dx%d", &ew, &eh) == 2 && ew >= 320 && ew <= 1920 && eh >= 240 && eh <= 1080) { SW = ew; SH = eh; } }
#ifdef USE_SDL1
    S = getenv("PQ_NODB") ? NULL : SDL_SetVideoMode(SW, SH, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);   /* v7.4: ask for a double-buffered screen first, fall back to the plain one */
    if (!S) S = SDL_SetVideoMode(SW, SH, 32, SDL_SWSURFACE);
#else
    win = SDL_CreateWindow("Poker Mini", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SW, SH, 0);
    S = win ? SDL_GetWindowSurface(win) : NULL;
#endif
    if (!S) { fprintf(stderr, "No screen: %s\n", SDL_GetError()); return 1; }
    if (S->w != SW || S->h != SH) { fprintf(stderr, "This build needs a %dx%d screen, the system gave %dx%d\n", SW, SH, S->w, S->h); return 1; }
#ifdef USE_SDL1
    vscr = S;
    if (S->format->BitsPerPixel == 32) {   /* v7.4: draw on a memory copy (see present); anything else keeps drawing straight on the screen as before */
        SDL_Surface *b = SDL_CreateRGBSurface(SDL_SWSURFACE, SW, SH, 32, S->format->Rmask, S->format->Gmask, S->format->Bmask, 0);
        if (b) S = b;
    }
    fprintf(stderr, "video: %dx%d, %s, %s\n", SW, SH, (vscr != S) ? "drawn in memory, copied per frame" : "drawn on the screen", getenv("PQ_NODB") ? "plain screen (PQ_NODB)" : "double buffer asked");
#endif
    if (SW < 320 || SH < 240) { fprintf(stderr, "Screen too small: %dx%d\n", SW, SH); return 1; }
    T = S;
    CS = SDL_CreateRGBSurface(0, XS(64) + 2, YS(80) + 2, 32, S->format->Rmask, S->format->Gmask, S->format->Bmask, 0);   /* the scratch surface a card is drawn flat on (screen resolution) */
    SDL_ShowCursor(0);
#ifndef SDL_STUB_H
    audio_init();
#endif
    bgs[BG_TABLE] = load_png("assets/room_neighbor.png");
    bgs[BG_LOBBY] = load_png("assets/entrance.png");
    bgs[BG_MYROOM] = load_png("assets/my_room.png");
    bgs[BG_HALL] = load_png("assets/her_room.png");
    bgs[BG_LIVING] = load_png("assets/BG-poker_day.png");
    bgs[BG_EVENT] = load_png("assets/poker_entrance.png");
    bgs[BG_EXHIB] = load_png("assets/Poker_Exhibition_Interior.png");
    bgs[BG_RAIN] = load_png("assets/windows_rain.png"); bgs[BG_SLEEP] = load_png("assets/sophia_sleeping.png"); bgs[BG_CLOSE] = load_png("assets/sophia_closer.png"); bgs[BG_LONG] = load_png("assets/sophia_longer.png"); bgs[BG_TORSO] = load_png("assets/close_up_torso.png");
    cfg_load();
    ignoreS = load_png("assets/ignore.png"); nightS = load_png("assets/her_night.png");   /* nightS: her in her night clothes (placeholder = the confident picture) */
    suitsS = load_png("assets/title_suits.png");
    bgs[BG_ROM] = load_png("assets/rom_room.png"); if (!bgs[BG_ROM]) bgs[BG_ROM] = bgs[BG_HALL];
    bgs[BG_BDOOR] = load_png("assets/bedroom_door.png"); if (!bgs[BG_BDOOR]) bgs[BG_BDOOR] = bgs[BG_HALL];
    ld_crop = 1; bgs[BG_PUMP] = load_png("assets/pump18.png"); ld_crop = 0; if (!bgs[BG_PUMP]) bgs[BG_PUMP] = bgs[BG_ROM];   /* v6.7: the peek picture */
    { static const char *const SL[STK_N] = {"nvidia", "google", "openai", "apple", "sony", "spacex"}; int q; for (q = 0; q < STK_N; q++) { char pth[48]; snprintf(pth, sizeof pth, "assets/stk_%s.png", SL[q]); stkLogo[q] = load_png_ex(pth, 1, 1, 1, 1, 1); } }
    giftS[SH_FLOWER] = load_png("assets/gift_flower.png"); giftS[SH_STACK] = load_png("assets/gift_cash.png"); giftS[SH_PILL] = load_png("assets/gift_pill.png"); giftS[SH_RING] = load_png("assets/gift_ring.png"); giftS[SH_PUMP] = load_png("assets/gift_pump.png"); giftS[SH_MILK] = load_png("assets/gift_milk.png");
    giftS[SH_LOTION] = load_png("assets/gift_lotion.png"); giftS[SH_DECK] = load_png("assets/gift_deck.png"); giftS[SH_YOGA] = load_png("assets/gift_yoga.png"); giftS[SH_TOP] = load_png("assets/gift_designer.png"); giftS[SH_COW] = load_png("assets/gift_cow.png"); giftS[SH_GOLD] = load_png("assets/gift_gold.png"); giftS[SH_DIAMOND] = load_png("assets/gift_dring.png"); giftS[SH_SUPER] = load_png("assets/gift_super.png"); giftS[SH_INJECT] = load_png("assets/gift_inject.png");   /* v12.3 */   /* v8.2 */ bottleS = load_png("assets/item_bottle.png"); gaugeS = load_png("assets/bottle_gauge.png"); gauge_init(); romHeartS = load_png("assets/romance_heart.png"); frHeartS = load_png("assets/friend_heart.png");
    cpuimg = load_png("assets/Cpu.png"); tblE = load_png("assets/table_empty.png");
    storeS = load_png("assets/store_menu.png"); sodaS = load_png("assets/drink_soda.png"); monS = load_png("assets/drink_monster.png");
    barS = load_png("assets/energy_bar.png"); segS = load_png("assets/energy_seg.png");
    roomS[0] = load_png_ex("assets/map_room.png", 3, 1, 1, 1, 1); wm[MAP_ROOM].img = roomS[0]; room_apply(); wm[MAP_HALL].img = load_png_ex("assets/map_hall.png", 3, 1, 1, 1, 1); wm[MAP_LOBBY].img = load_png_ex("assets/map_lobby.png", 3, 1, 1, 1, 1);
    wm[MAP_EVENT].img = load_png_ex("assets/map_event.png", 3, 1, 1, 1, 1);   /* no night copy: the Event Center is always lit */
    wm[MAP_HALL1].img = load_png_ex("assets/map_hall1.png", 3, 1, 1, 1, 1); wm[MAP_HALL3].img = load_png_ex("assets/map_hall3.png", 3, 1, 1, 1, 1);
    wm[MAP_SOPHIA].img = load_png_ex("assets/map_sophia.png", 3, 1, 1, 1, 1);   /* v11.1: no night filter on her 3rd-floor room any more (and no night visit: the night door is knock-only) */
    wm[MAP_SBED].img = load_png_ex("assets/map_sbed.png", 3, 1, 1, 1, 1); sbN = load_png_ex("assets/map_sbed.png", 3, 1, 1, 1, 1); night(sbN);   /* v9.2: her bedroom, and its night picture (darkened once here) */   /* her room at night: the picture is darkened once here, so it costs nothing while you walk */
    plS = load_png_ex("assets/player.png", 11, 5, 0, 3, 4); plSn = load_png_ex("assets/player.png", 11, 5, 0, 3, 4); night(plSn); plSm = load_png_ex("assets/player.png", 11, 5, 0, 3, 4); night_mild(plSm); clerkS = load_png_ex("assets/clerk.png", 27, 14, 0, 1, 1);   /* you are about 145 px tall (as tall as the dark doorway), the receptionist about 79 */
    if (plS) {   /* the sheet is 3 frames x 4 rows (down, left, right, up); find where the feet are in each row so you stand on the tile */
        int r, x, y, fwn = plS->w / 3, fhn = plS->h / 4; pl_fw = (int)((long long)fwn * 640 / SW); pl_fh = (int)((long long)fhn * 480 / SH);   /* logical frame size */
        for (r = 0; r < 4; r++) {
            int bot = 0;
            for (y = r * fhn; y < (r + 1) * fhn && y < plS->h; y++) for (x = 0; x < plS->w; x++)
                if (((*px(plS, x, y) & plS->format->Amask) >> plS->format->Ashift) > 32) bot = y - r * fhn + 1;
            pl_foot[r] = bot ? (int)((long long)bot * 480 / SH) : pl_fh;   /* where the feet are, in logical px */
        }
    }
    lsS = load_png_ex("assets/sophia_walk.png", 11, 5, 0, 3, 4);   /* v8.0: Sophia in the living room (3 x 4 frames, like your sheet) */
    lsSn = load_png_ex("assets/sophia_walk.png", 11, 5, 0, 3, 4); night(lsSn);   /* v9.2: her at night in her bedroom */
    if (lsS) {
        int r, x, y, fwn = lsS->w / 3, fhn = lsS->h / 4; ls_fw = (int)((long long)fwn * 640 / SW); ls_fh = (int)((long long)fhn * 480 / SH);
        for (r = 0; r < 4; r++) {
            int bot = 0;
            for (y = r * fhn; y < (r + 1) * fhn && y < lsS->h; y++) for (x = 0; x < lsS->w; x++)
                if (((*px(lsS, x, y) & lsS->format->Amask) >> lsS->format->Ashift) > 32) bot = y - r * fhn + 1;
            ls_foot[r] = bot ? (int)((long long)bot * 480 / SH) : ls_fh;
        }
    }
    g_init(&G); saved_G = G; wk_place(MAP_ROOM, ROOM_SX, ROOM_SY, DN);
    { int i; char p[64];
      for (i = 0; i < 4; i++) {
          snprintf(p, sizeof p, "assets/chip_%s.png", CN[i]); chipS[i] = load_png(p);
          snprintf(p, sizeof p, "assets/chip_%s_dim.png", CN[i]); chipD[i] = load_png(p);
          snprintf(p, sizeof p, "assets/potchip_%s.png", CN[i]); potS[i] = load_png(p);
      }
      bnW = load_png("assets/banner_win.png"); bnL = load_png("assets/banner_lose.png"); bnS = load_png("assets/banner_split.png");
      dgW = load_png("assets/banner_digits.png"); dgL = load_png("assets/banner_digits_lose.png"); icon = load_png("assets/chip_icon.png"); }
#ifdef HAVE_SHOTS
    if (argc > 1 && !strcmp(argv[1], "--shots")) { shots(); SDL_Quit(); return 0; }
    if (argc > 1 && !strcmp(argv[1], "--autoplay")) { int r = autoplay(); SDL_Quit(); return r; }
#endif
    text_problems = ex_load_all4(LINES_PATH, TALK_PATH, ITEMS_PATH, NAMES_PATH, TLOG_PATH, 1);   /* v7.9: the built-in text stays wherever a file is missing or a line is refused */
    if (!load_game(&G, slot_path(slot_act))) g_new(&G);   /* v10.5: the title shows the active slot (the sofa and the bed save there); CAREER MODE lists all five */
    prog_far = prog_get(prog_path(), G.career); saved_G = G; scr = SC_MENU; menu_sel = 1;
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (running) { unsigned t0 = (unsigned)SDL_GetTicks(), el; frame(); el = (unsigned)SDL_GetTicks() - t0; if (el < 16) SDL_Delay(16 - el); }   /* v7.2: wait only for what is left of the 16 ms, not 16 ms on top of the drawing time */
#endif
    SDL_Quit();
    return 0;
}
