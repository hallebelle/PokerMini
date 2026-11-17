/* Poker Mini - 8 ball engine for the Event Center (v12.6, no SDL).
   Everything is in NATIVE screen pixels of the 752x560 table picture (assets/pool_table.png); the game scales to the real screen.
   Heads-up standard 8 ball, no called pockets (bar rules), ball in hand after a foul. Seat 0 = you (you break), seat 1 = the CPU.
   Fixed time step, equal-mass ball collisions, rolling friction, circular pocket capture. Deterministic from the seed.
   All tuning numbers are in the block "TUNING" below. */
#ifndef PQ_POOL_H
#define PQ_POOL_H
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* ---------- TUNING ---------- */
#define PL_R 10.0f                /* ball radius (v12.8: was 8.5, so the number on the ball can be read; 11 is the next step if it is still too small) */
#define PL_X0 44.0f               /* cushion inner edges (from the picture) */
#define PL_X1 706.5f
#define PL_Y0 46.5f
#define PL_Y1 373.5f
#define PL_DT (1.0f / 480.0f)     /* one physics step (seconds); the game runs 8 per frame */
/* v12.7: the cloth. 1 px = about 3.36 mm (an 8 ft table, 2.25 inch balls), so g = 2919 px/s^2. */
#define PL_G 2919.0f
#define PL_MU_S 0.20f             /* sliding friction cloth / ball (while the ball skids: draw, follow, a fresh cue ball) */
#define PL_FRIC 52.0f             /* rolling resistance: constant deceleration (px / s^2) */
#define PL_DRAG 0.35f             /* ... plus a little loss proportional to speed (per second) */
#define PL_STOP 5.0f              /* a rolling ball below this speed (px / s) stops */
#define PL_ZSPIN 34.0f            /* side spin dies away at this rate (rad / s^2) */
#define PL_E_BALL 0.96f           /* ball-ball restitution */
#define PL_MU_BB 0.055f           /* ball-ball friction: throw and spin transfer */
#define PL_E_RAIL0 0.86f          /* cushion restitution at a crawl ... */
#define PL_E_RAILK 0.00018f       /* ... minus this per px/s of impact speed ... */
#define PL_E_RAILMIN 0.58f        /* ... never below this */
#define PL_MU_RAIL 0.10f          /* cushion friction: side spin changes the bounce angle */
#define PL_RAIL_H 0.4f            /* height of the cushion nose above the ball centre (in ball radii): 0.4 keeps a rolling ball rolling after the bounce */
#define PL_VMIN 70.0f             /* cue ball speed at the weakest shot */
#define PL_VMAX 1500.0f           /* ... and at full power */
#define PL_VBREAK 2600.0f         /* ... and at full power on the break shot */
#define PL_SPIN_MAX 0.6f          /* the cue tip may hit this far from the centre (in ball radii) */
#define PL_SQUIRT 0.045f          /* the cue ball leaves this many radians off the aim per ball radius of side english (away from the english) */
#define PL_BREAK_X 209.0f         /* head spot (cue ball at the break) */
#define PL_FOOT_X 541.0f          /* foot spot (the apex of the rack) */
#define PL_CY 210.0f              /* the middle line */

#define PL_NP 6
static const float PL_PX[PL_NP] = {38.0f, 375.25f, 712.5f, 38.0f, 375.25f, 712.5f};      /* pocket centres for the capture test (a little inside the painted holes) */
static const float PL_PY[PL_NP] = {40.0f, 38.0f, 40.0f, 380.0f, 382.0f, 380.0f};
static const float PL_PR[PL_NP] = {30.0f, 25.0f, 30.0f, 30.0f, 25.0f, 30.0f};            /* capture radius: corner pockets are roomier than the side ones */

enum { PH_AIM, PH_ROLL, PH_OVER };          /* waiting for a shot / balls moving / game finished */
enum { PW_NONE, PW_8BALL, PW_EARLY8, PW_SCRATCH8, PW_FOUL8 };   /* how the game was decided: legal 8 / 8 too early / scratch on the 8 / foul on the 8 shot */
enum { FL_NONE, FL_SCRATCH, FL_NOHIT, FL_WRONG, FL_NORAIL };      /* the foul of the last shot */

typedef struct {
    float x[16], y[16], vx[16], vy[16];
    float wx[16], wy[16], wz[16];   /* v12.7: spin (rad / s). x, y, z = right, down, into the table (right handed). Rolling: vx = -R*wy, vy = R*wx. wz > 0 turns clockwise on the screen */
    float q[16][4];             /* orientation of every ball (w, x, y, z) for the picture; only advanced while track = 1 (the game sets it, the CPU's trial shots do not) */
    int track;
    float ev_hit_v, ev_rail_v;  /* hardest ball / cushion impact since the caller last cleared them (px / s) */
    int in[16];                 /* 1 = on the table, 0 = potted (ball 0 = cue ball, 1-7 solids, 8 the eight, 9-15 stripes) */
    int turn, phase, winner, wreason;   /* whose shot, phase, winner (-1 none), PW_* */
    int grp[2];                 /* 0 = solids, 1 = stripes, -1 = open table */
    int bih, brk;               /* ball in hand for the player to move / the break shot is still to be played */
    int foul, last_turn;        /* FL_* of the last shot and who shot it */
    /* the shot that is rolling */
    int first_hit, rail_after, nrail, shooter, pot_n, pot_list[16], scratch, pocketed_8, pot_pocket[16];
    int own_left0;              /* balls of the shooter's group on the table when the shot was taken (8 is legal only when this was 0) */
    int ev_hit, ev_rail, ev_pot;   /* events since the caller last cleared them (for the sounds) */
    unsigned rng; int steps; float maxv;
} Pool;

static inline unsigned pl_rand(Pool *g) { g->rng ^= g->rng << 13; g->rng ^= g->rng >> 17; g->rng ^= g->rng << 5; return g->rng; }
static inline float pl_rf(Pool *g) { return (pl_rand(g) & 0xFFFFFF) / 16777216.0f; }
static inline float pl_gauss(Pool *g) { float s = 0; int i; for (i = 0; i < 6; i++) s += pl_rf(g); return (s - 3.0f) * 1.41421f; }   /* about N(0,1) */
static inline int pl_group_of(int id) { return id >= 1 && id <= 7 ? 0 : id >= 9 ? 1 : -1; }
static inline int pl_left(const Pool *g, int grp) { int i, n = 0; for (i = 1; i < 16; i++) if (g->in[i] && pl_group_of(i) == grp) n++; return n; }
static inline int pl_ball_moves(const Pool *g, int i) { return g->vx[i] != 0 || g->vy[i] != 0 || g->wx[i] != 0 || g->wy[i] != 0; }
static inline int pl_all_still(const Pool *g) { int i; for (i = 0; i < 16; i++) if (g->in[i] && pl_ball_moves(g, i)) return 0; return 1; }

/* ---------- orientation (for the picture only) ---------- */
static inline void pl_qmul(const float *a, const float *b, float *o) {
    o[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
    o[1] = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2];
    o[2] = a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
    o[3] = a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
}

static void pl_rack(Pool *g) {
    int rest[14], nr = 0, i, r, k = 0, ia = -1, ib = -1;
    float dx = 1.7320508f * PL_R + 0.35f;
    for (i = 1; i <= 7; i++) rest[nr++] = i;
    for (i = 9; i <= 15; i++) rest[nr++] = i;
    for (i = nr - 1; i > 0; i--) { int j = (int)(pl_rand(g) % (unsigned)(i + 1)), t = rest[i]; rest[i] = rest[j]; rest[j] = t; }
    for (i = 0; i < nr; i++) if (rest[i] <= 7) { ia = i; break; }
    { int t = rest[0]; rest[0] = rest[ia]; rest[ia] = t; }
    for (i = 1; i < nr; i++) if (rest[i] >= 9) { ib = i; break; }
    { int t = rest[1]; rest[1] = rest[ib]; rest[ib] = t; }
    if (pl_rand(g) & 1) { int t = rest[0]; rest[0] = rest[1]; rest[1] = t; }   /* the solid and the stripe sit in the two back corners, side chosen at random */
    for (r = 0; r < 5; r++) for (i = 0; i <= r; i++) {
        int id;
        float x = PL_FOOT_X + dx * r, y = PL_CY + (i - r * 0.5f) * (2.0f * PL_R + 0.3f);
        if (r == 2 && i == 1) id = 8;
        else if (r == 4 && i == 0) id = rest[0];
        else if (r == 4 && i == 4) id = rest[1];
        else id = rest[2 + k++];
        g->x[id] = x; g->y[id] = y;
    }
}

/* a ball at rest shows its number to the viewer (body x axis towards the viewer, the stripe axis up the screen), then a little random turn so no two look alike */
static void pl_orient_init(Pool *g, int i) {
    float base[4] = {0.70710678f, 0, 0.70710678f, 0}, az, ax, ay, t1[4], t2[4], t3[4];
    float rz = (pl_rf(g) - 0.5f) * 0.8f, rx = (pl_rf(g) - 0.5f) * 0.5f, ry = (pl_rf(g) - 0.5f) * 0.5f;
    if (i == 0) { rz = (pl_rf(g) - 0.5f) * 6.2f; rx = (pl_rf(g) - 0.5f) * 3.0f; ry = (pl_rf(g) - 0.5f) * 3.0f; }   /* the cue ball: any way up */
    az = rz * 0.5f; ax = rx * 0.5f; ay = ry * 0.5f;
    { float qz[4] = {cosf(az), 0, 0, sinf(az)}, qx[4] = {cosf(ax), sinf(ax), 0, 0}, qy[4] = {cosf(ay), 0, sinf(ay), 0};
      pl_qmul(qz, qx, t1); pl_qmul(t1, qy, t2); pl_qmul(t2, base, t3); }
    memcpy(g->q[i], t3, sizeof t3);
}
static inline void pl_new(Pool *g, unsigned seed) {
    int i; memset(g, 0, sizeof *g);
    g->rng = seed ? seed : 2463534242u;
    for (i = 0; i < 16; i++) g->in[i] = 1;
    g->x[0] = PL_BREAK_X; g->y[0] = PL_CY;
    pl_rack(g);
    for (i = 0; i < 16; i++) pl_orient_init(g, i);
    g->turn = 0; g->phase = PH_AIM; g->winner = -1; g->grp[0] = g->grp[1] = -1; g->brk = 1; g->bih = 0; g->foul = FL_NONE; g->last_turn = -1; g->first_hit = -1;
}

/* is the point (x, y) a legal spot for the cue ball (inside the cushions, clear of every other ball)? */
static inline int pl_spot_ok(const Pool *g, float x, float y) {
    int i;
    if (x < PL_X0 + PL_R || x > PL_X1 - PL_R || y < PL_Y0 + PL_R || y > PL_Y1 - PL_R) return 0;
    for (i = 0; i < PL_NP; i++) { float dx = x - PL_PX[i], dy = y - PL_PY[i]; if (dx * dx + dy * dy < PL_PR[i] * PL_PR[i]) return 0; }   /* v12.7: not in a pocket mouth */
    for (i = 1; i < 16; i++) if (g->in[i]) { float dx = g->x[i] - x, dy = g->y[i] - y; if (dx * dx + dy * dy < 4 * PL_R * PL_R + 0.5f) return 0; }
    return 1;
}

/* take the shot: angle in radians (0 = right, y grows downwards), power 0..1.
   v12.7: (sa, sb) = where the cue tip hits the ball, in ball radii: sa > 0 = right of the centre (as the shooter looks along the shot), sb > 0 = above the centre.
   sb > 0 = follow (top spin), sb < 0 = draw (back spin), sa = side english. The point is kept inside a circle of PL_SPIN_MAX. */
static inline void pl_shoot_sp(Pool *g, float ang, float power, float sa, float sb) {
    float v, dx, dy, top, side, r, vmax; int i;
    if (power < 0) power = 0;
    if (power > 1) power = 1;
    r = sqrtf(sa * sa + sb * sb); if (r > PL_SPIN_MAX) { sa *= PL_SPIN_MAX / r; sb *= PL_SPIN_MAX / r; }
    vmax = g->brk ? PL_VBREAK : PL_VMAX;
    v = PL_VMIN + (vmax - PL_VMIN) * power;
    ang -= PL_SQUIRT * sa;   /* right english squirts the ball a little to the left */
    dx = cosf(ang); dy = sinf(ang);
    g->vx[0] = dx * v; g->vy[0] = dy * v;
    top = 2.5f * v * sb / PL_R; side = -2.5f * v * sa / PL_R;   /* (5 v b) / (2 R^2) with b = sb * R; a strike to the right turns the ball counter-clockwise */
    g->wx[0] = top * dy; g->wy[0] = -top * dx; g->wz[0] = side;
    g->phase = PH_ROLL; g->shooter = g->turn; g->first_hit = -1; g->rail_after = 0; g->nrail = 0; g->pot_n = 0; g->scratch = 0; g->pocketed_8 = 0; g->steps = 0;
    g->own_left0 = g->grp[g->turn] >= 0 ? pl_left(g, g->grp[g->turn]) : 15; g->bih = 0;
    g->ev_hit_v = g->ev_rail_v = 0;
    for (i = 0; i < 16; i++) g->pot_pocket[i] = -1;
}
static inline void pl_shoot(Pool *g, float ang, float power) { pl_shoot_sp(g, ang, power, 0, 0); }

/* decide the shot once everything has stopped */
static void pl_resolve(Pool *g) {
    int me = g->shooter, foul = FL_NONE, i, cont = 0, own_pot = 0, assign = -1, was_break = g->brk;
    int grp = g->grp[me], legal_first;
    g->brk = 0;
    /* the first ball the cue ball touched must be legal */
    if (g->first_hit < 0) foul = FL_NOHIT;
    else {
        if (grp < 0) legal_first = g->first_hit != 8;
        else if (g->own_left0 == 0) legal_first = g->first_hit == 8;
        else legal_first = pl_group_of(g->first_hit) == grp;
        if (!legal_first) foul = FL_WRONG;
    }
    if (foul == FL_NONE && !g->pot_n && !g->rail_after) foul = FL_NORAIL;   /* after the contact something must hit a cushion or drop */
    if (g->scratch) foul = FL_SCRATCH;
    /* what dropped */
    for (i = 0; i < g->pot_n; i++) {
        int id = g->pot_list[i], gp = pl_group_of(id);
        if (id == 8) continue;
        if (grp >= 0 ? gp == grp : 1) own_pot++;
        if (grp < 0 && assign < 0) assign = gp;
    }
    if (g->pocketed_8) {
        if (was_break) {   /* the 8 on the break: spot it again, no win and no loss */
            g->in[8] = 1; g->x[8] = PL_FOOT_X; g->y[8] = PL_CY; g->vx[8] = g->vy[8] = 0; g->wx[8] = g->wy[8] = g->wz[8] = 0;
            { int j; for (j = 0; j < 16; j++) if (j != 8 && g->in[j] && j != 0) { float dx = g->x[j] - g->x[8], dy = g->y[j] - g->y[8]; if (dx * dx + dy * dy < 4 * PL_R * PL_R) { g->x[8] += 2.0f * PL_R + 0.5f; j = -1; } } }
            g->pocketed_8 = 0;
        } else {
            g->phase = PH_OVER;
            if (grp < 0 || g->own_left0 > 0) { g->winner = !me; g->wreason = PW_EARLY8; }
            else if (g->scratch) { g->winner = !me; g->wreason = PW_SCRATCH8; }
            else if (foul != FL_NONE) { g->winner = !me; g->wreason = PW_FOUL8; }
            else { g->winner = me; g->wreason = PW_8BALL; }
            g->foul = foul; g->last_turn = me;
            return;
        }
    }
    /* the groups are decided by the first legal pot on an open table */
    if (foul == FL_NONE && grp < 0 && assign >= 0 && !was_break) { g->grp[me] = assign; g->grp[!me] = 1 - assign; grp = assign; }
    else if (foul == FL_NONE && grp < 0 && assign >= 0 && was_break) { /* the break never decides the groups: the table stays open */ }
    /* does the shooter go on? */
    if (foul == FL_NONE) {
        if (was_break) cont = g->pot_n > 0 && !g->scratch;
        else cont = own_pot > 0;
    }
    g->foul = foul; g->last_turn = me;
    if (foul != FL_NONE) { g->turn = !me; g->bih = 1; }
    else if (!cont) { g->turn = !me; g->bih = 0; }
    else { g->turn = me; g->bih = 0; }
    if (g->scratch) { g->in[0] = 1; g->vx[0] = g->vy[0] = 0; g->wx[0] = g->wy[0] = g->wz[0] = 0; g->x[0] = PL_BREAK_X; g->y[0] = PL_CY; }   /* the cue ball is back on the table (ball in hand: the player moves it) */
    if (g->bih && !pl_spot_ok(g, g->x[0], g->y[0])) {   /* find a free spot near the head spot */
        float tx, ty; int ok = 0, k;
        for (k = 0; k < 400 && !ok; k++) { tx = PL_X0 + 60 + (k % 20) * 20.0f; ty = PL_Y0 + 30 + (k / 20) * 14.0f; if (pl_spot_ok(g, tx, ty)) { g->x[0] = tx; g->y[0] = ty; ok = 1; } }
    }
    g->phase = PH_AIM;
}

static inline void pl_potted(Pool *g, int i, int pk) {
    g->in[i] = 0; g->vx[i] = g->vy[i] = 0; g->wx[i] = g->wy[i] = g->wz[i] = 0; g->ev_pot++;
    if (i == 0) { g->scratch = 1; return; }
    if (g->pot_n < 16) { g->pot_list[g->pot_n] = i; g->pot_pocket[g->pot_n] = pk; g->pot_n++; }
    if (i == 8) g->pocketed_8 = 1;
}

/* the cloth under one ball for one step: while the contact point skids (slip u = v + w x r) sliding friction drags the ball towards a clean roll
   (and turns the spin the same way); once it rolls only the small rolling resistance is left. Side spin dies away on its own. */
static inline void pl_cloth(Pool *g, int i, float dt) {
    float vx = g->vx[i], vy = g->vy[i], wx = g->wx[i], wy = g->wy[i], wz = g->wz[i];
    float ux = vx + PL_R * wy, uy = vy - PL_R * wx, s = sqrtf(ux * ux + uy * uy);
    if (s > 0.6f) {
        float a = 3.5f * PL_MU_S * PL_G * dt, ix = ux / s, iy = uy / s;
        int lock = 0;
        if (a >= s) { a = s; lock = 1; }
        vx -= 0.2857143f * a * ix; vy -= 0.2857143f * a * iy;
        wy -= 0.7142857f / PL_R * a * ix; wx += 0.7142857f / PL_R * a * iy;
        if (lock) { wy = -vx / PL_R; wx = vy / PL_R; }
    } else {
        float sp = sqrtf(vx * vx + vy * vy), ns = sp - (PL_FRIC + PL_DRAG * sp) * dt;
        if (ns < PL_STOP) { vx = vy = 0; wx = wy = 0; }
        else { float k = ns / sp; vx *= k; vy *= k; wy = -vx / PL_R; wx = vy / PL_R; }
    }
    if (wz != 0) { float d = PL_ZSPIN * dt; if (wz > d) wz -= d; else if (wz < -d) wz += d; else wz = 0; }
    if (vx == 0 && vy == 0 && wx == 0 && wy == 0) wz = 0;
    g->vx[i] = vx; g->vy[i] = vy; g->wx[i] = wx; g->wy[i] = wy; g->wz[i] = wz;
}
static inline void pl_turn(Pool *g, int i, float dt) {   /* advance the picture orientation */
    float wx = g->wx[i], wy = g->wy[i], wz = g->wz[i], *q = g->q[i], dw, dx, dy, dz, n;
    if (wx == 0 && wy == 0 && wz == 0) return;
    dw = -(wx * q[1] + wy * q[2] + wz * q[3]);
    dx = wx * q[0] + (wy * q[3] - wz * q[2]);
    dy = wy * q[0] + (wz * q[1] - wx * q[3]);
    dz = wz * q[0] + (wx * q[2] - wy * q[1]);
    q[0] += 0.5f * dt * dw; q[1] += 0.5f * dt * dx; q[2] += 0.5f * dt * dy; q[3] += 0.5f * dt * dz;
    n = 1.0f / sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    q[0] *= n; q[1] *= n; q[2] *= n; q[3] *= n;
}
/* a cushion at the ball: (nx, ny) = the unit normal pointing into the table. The nose touches the ball PL_RAIL_H * R above its centre, so the contact point's
   speed along the normal includes the spin; the bounce is judged there (energy can only go down). Friction along the cushion works on the side spin. */
static inline void pl_cushion(Pool *g, int i, float nx, float ny) {
    float vn = g->vx[i] * nx + g->vy[i] * ny, h = PL_RAIL_H * PL_R, wn = g->wx[i] * ny - g->wy[i] * nx, un = vn + h * wn;
    float e, jn, tx = -ny, ty = nx, vt, ut, jt;
    if (un >= 0) return;
    e = PL_E_RAIL0 - PL_E_RAILK * (-un); if (e < PL_E_RAILMIN) e = PL_E_RAILMIN;
    jn = (1.0f + e) * (-un) / 1.4f;
    g->vx[i] += jn * nx; g->vy[i] += jn * ny;
    g->wx[i] += jn * ny / PL_R; g->wy[i] -= jn * nx / PL_R;
    vt = g->vx[i] * tx + g->vy[i] * ty; ut = vt - PL_R * g->wz[i];
    if (ut != 0) { jt = PL_MU_RAIL * jn; if (jt > fabsf(ut) / 3.5f) jt = fabsf(ut) / 3.5f; if (ut > 0) jt = -jt; g->vx[i] += jt * tx; g->vy[i] += jt * ty; g->wz[i] -= 2.5f * jt / PL_R; }
    if (-un > g->ev_rail_v) g->ev_rail_v = -un;
}

/* one physics step; returns 1 while something still moves */
static int pl_step(Pool *g, float dt) {
    int i, j, moving = 0;
    float rr = 2.0f * PL_R;
    for (i = 0; i < 16; i++) {
        if (!g->in[i]) continue;
        if (!pl_ball_moves(g, i)) continue;
        g->x[i] += g->vx[i] * dt; g->y[i] += g->vy[i] * dt;
        pl_cloth(g, i, dt);
        if (g->track) pl_turn(g, i, dt);
    }
    /* pockets: a ball drops when it is inside the mouth and either heading into the pocket or slow; one sliding past along the cushion does not */
    for (i = 0; i < 16; i++) if (g->in[i]) {
        int p; for (p = 0; p < PL_NP; p++) {
            float dx = g->x[i] - PL_PX[p], dy = g->y[i] - PL_PY[p], d2 = dx * dx + dy * dy;
            if (d2 < PL_PR[p] * PL_PR[p]) {
                float sp2 = g->vx[i] * g->vx[i] + g->vy[i] * g->vy[i]; int ok = d2 < 0.36f * PL_PR[p] * PL_PR[p] || sp2 < 625.0f;
                if (!ok) { float c = -(g->vx[i] * dx + g->vy[i] * dy) / (sqrtf(sp2) * sqrtf(d2) + 1e-6f), sp = sqrtf(sp2), thr = (p == 1 || p == 4 ? 0.66f + 0.28f * (sp > 900.0f ? 1.0f : sp / 900.0f) : 0.30f + 0.20f * (sp > 900.0f ? 1.0f : sp / 900.0f)); ok = c > thr; }   /* side pockets (the cushion runs right past their mouth): the faster the ball, the straighter it must head at the pocket; corners are kind */
                if (ok) { pl_potted(g, i, p); break; }
            }
        }
    }
    /* cushions */
    for (i = 0; i < 16; i++) if (g->in[i]) {
        int hit = 0;
        if (g->x[i] < PL_X0 + PL_R) { g->x[i] = PL_X0 + PL_R + (PL_X0 + PL_R - g->x[i]) * 0.5f; pl_cushion(g, i, 1, 0); hit = 1; }
        else if (g->x[i] > PL_X1 - PL_R) { g->x[i] = PL_X1 - PL_R - (g->x[i] - (PL_X1 - PL_R)) * 0.5f; pl_cushion(g, i, -1, 0); hit = 1; }
        if (g->y[i] < PL_Y0 + PL_R) { g->y[i] = PL_Y0 + PL_R + (PL_Y0 + PL_R - g->y[i]) * 0.5f; pl_cushion(g, i, 0, 1); hit = 1; }
        else if (g->y[i] > PL_Y1 - PL_R) { g->y[i] = PL_Y1 - PL_R - (g->y[i] - (PL_Y1 - PL_R)) * 0.5f; pl_cushion(g, i, 0, -1); hit = 1; }
        if (hit) { g->nrail++; g->ev_rail++; if (g->first_hit >= 0) g->rail_after = 1; }
    }
    /* ball against ball */
    for (i = 0; i < 16; i++) if (g->in[i]) for (j = i + 1; j < 16; j++) if (g->in[j]) {
        float dx = g->x[j] - g->x[i], dy = g->y[j] - g->y[i], d2 = dx * dx + dy * dy;
        if (d2 < rr * rr && d2 > 1e-6f) {
            float rvx = g->vx[j] - g->vx[i], rvy = g->vy[j] - g->vy[i], rdv = dx * rvx + dy * rvy;
            if (rdv < 0) {   /* approaching: find the exact moment of contact (tau seconds ago) so the contact normal is exact, then bounce and move on from there */
                float a2 = rvx * rvx + rvy * rvy, c = d2 - rr * rr, disc = rdv * rdv - a2 * c, tau = 0, nx, ny, rv, jj, cx, cy;
                if (a2 > 1e-6f && disc >= 0) tau = (rdv + sqrtf(disc)) / a2;
                if (tau < 0 || tau > dt * 2.0f) tau = 0;
                cx = dx - rvx * tau; cy = dy - rvy * tau;   /* the centre offset at the moment of contact (length 2R) */
                { float cl = sqrtf(cx * cx + cy * cy); if (cl < 1e-4f) { cl = sqrtf(d2); cx = dx; cy = dy; } nx = cx / cl; ny = cy / cl; }
                g->x[i] -= g->vx[i] * tau; g->y[i] -= g->vy[i] * tau; g->x[j] -= g->vx[j] * tau; g->y[j] -= g->vy[j] * tau;
                rv = (g->vx[i] - g->vx[j]) * nx + (g->vy[i] - g->vy[j]) * ny; jj = (1.0f + PL_E_BALL) * rv * 0.5f;
                g->vx[i] -= jj * nx; g->vy[i] -= jj * ny; g->vx[j] += jj * nx; g->vy[j] += jj * ny;
                {   /* v12.7: friction between the two surfaces: the cut and the side spin throw the object ball a little and pass some spin on */
                    float tx = -ny, ty = nx, ut = (g->vx[i] - g->vx[j]) * tx + (g->vy[i] - g->vy[j]) * ty + PL_R * (g->wz[i] + g->wz[j]), jt = PL_MU_BB * jj;
                    if (jt > fabsf(ut) / 7.0f) jt = fabsf(ut) / 7.0f; if (ut > 0) jt = -jt;
                    g->vx[i] += jt * tx; g->vy[i] += jt * ty; g->vx[j] -= jt * tx; g->vy[j] -= jt * ty;
                    g->wz[i] += 2.5f * jt / PL_R; g->wz[j] += 2.5f * jt / PL_R;
                }
                if (rv > g->ev_hit_v) g->ev_hit_v = rv;
                g->x[i] += g->vx[i] * tau; g->y[i] += g->vy[i] * tau; g->x[j] += g->vx[j] * tau; g->y[j] += g->vy[j] * tau;
                { float ex = g->x[j] - g->x[i], ey = g->y[j] - g->y[i], el = sqrtf(ex * ex + ey * ey);   /* never leave them overlapping */
                  if (el < rr && el > 1e-6f) { float ov = (rr - el) * 0.5f; g->x[i] -= ex / el * ov; g->y[i] -= ey / el * ov; g->x[j] += ex / el * ov; g->y[j] += ey / el * ov; } }
                g->ev_hit++;
                if (g->first_hit < 0 && g->shooter >= 0) { if (i == 0) g->first_hit = j; else if (j == 0) g->first_hit = i; }
            } else {   /* overlapping but not approaching: just push apart */
                float d = sqrtf(d2), ov = (rr - d) * 0.5f; g->x[i] -= dx / d * ov; g->y[i] -= dy / d * ov; g->x[j] += dx / d * ov; g->y[j] += dy / d * ov;
            }
        }
    }
    for (i = 0; i < 16; i++) if (g->in[i] && pl_ball_moves(g, i)) moving = 1;
    return moving;
}

/* advance the game by n steps (the caller decides how many per frame); resolves the shot when everything has stopped */
static inline void pl_run(Pool *g, int n) {
    int k;
    if (g->phase != PH_ROLL) return;
    for (k = 0; k < n; k++) {
        g->steps++;
        if (!pl_step(g, PL_DT)) { pl_resolve(g); return; }
        if (g->steps > 480 * 40) { int i; for (i = 0; i < 16; i++) g->vx[i] = g->vy[i] = 0; pl_resolve(g); return; }   /* never longer than 40 seconds */
    }
}

/* ---------- aiming help: where does the cue ball first touch something? ---------- */
typedef struct { int hit; float t, cx, cy; int wall; } PlRay;   /* hit = ball id, -1 = a cushion; t = distance the cue ball travels; (cx, cy) = the cue ball centre at the contact; wall (only when hit = -1): 1 = a left / right cushion, 2 = a top / bottom cushion, 3 = a corner (both) */
/* v12.8: trace a cue-ball-sized circle from any point (x, y) in direction ang; ball 0 (the cue ball) never blocks it, every other ball on the table does */
static inline PlRay pl_ray_from(const Pool *g, float x0, float y0, float ang) {
    PlRay r; float dx = cosf(ang), dy = sinf(ang), best = 1e9f; int i;
    r.hit = -1; r.t = 0; r.cx = x0; r.cy = y0; r.wall = 0;
    for (i = 1; i < 16; i++) if (g->in[i]) {
        float ox = g->x[i] - x0, oy = g->y[i] - y0, b = ox * dx + oy * dy, c = ox * ox + oy * oy - 4 * PL_R * PL_R, disc = b * b - c;
        if (b > 0 && disc >= 0) { float t = b - sqrtf(disc); if (t >= 0 && t < best) { best = t; r.hit = i; } }
    }
    {   /* the cushions (for the cue ball centre) */
        float tx = 1e9f, ty = 1e9f, tw, t;
        if (dx > 1e-6f) { t = (PL_X1 - PL_R - x0) / dx; if (t < tx) tx = t; } else if (dx < -1e-6f) { t = (PL_X0 + PL_R - x0) / dx; if (t < tx) tx = t; }
        if (dy > 1e-6f) { t = (PL_Y1 - PL_R - y0) / dy; if (t < ty) ty = t; } else if (dy < -1e-6f) { t = (PL_Y0 + PL_R - y0) / dy; if (t < ty) ty = t; }
        tw = tx < ty ? tx : ty;
        if (r.hit < 0 || tw < best) { r.hit = -1; best = tw; r.wall = (tx < ty - 1e-3f) ? 1 : (ty < tx - 1e-3f) ? 2 : 3; }
    }
    if (best < 0) best = 0;
    r.t = best; r.cx = x0 + dx * best; r.cy = y0 + dy * best;
    return r;
}
static inline PlRay pl_ray(const Pool *g, float ang) { return pl_ray_from(g, g->x[0], g->y[0], ang); }

/* ---------- v13.0: the simulated guide ----------
   The guide no longer draws a straight line and a mirror bounce. It runs the real engine (pl_step, the same code the shot uses) on a copy of the table for this
   aim, power and tip position, and records where the balls really go: the cue ball up to the first ball it touches (with the real bend after a cushion), then the
   cue ball and the object ball right after the touch. The copy is thrown away, the real table is never changed. */
#define PL_GD_N 320                /* samples in the cue ball line before the touch */
#define PL_GD_M 64                 /* samples in each line after the touch */
#define PL_GD_GAP 4.0f             /* one sample every 4 px of travel */
#define PL_GD_REB 220.0f           /* after the first cushion the cue ball line is at most this long (px) */
#define PL_GD_CUE 60.0f            /* the cue ball line after it touched a ball */
#define PL_GD_OBJ 200.0f           /* the object ball line (ends at its first cushion / ball, or in the pocket) */
#define PL_GD_STEPS (480 * 3)      /* at most 3 seconds of table time */
typedef struct {
    int hit;                       /* the ball the cue ball touches first (1-15), -1 = none on this line */
    float cx, cy;                  /* cue ball centre at the touch */
    int rails;                     /* cushions the cue ball meets before the touch (the line ends at the second one) */
    int scratch, pot;              /* 1 = the cue ball drops in a pocket on this line / the object ball drops in a pocket */
    int n0, bend;                  /* the cue ball line: n0 samples; bend = sample at the first cushion (-1 = none) */
    float x0[PL_GD_N], y0[PL_GD_N];
    int n1, n2;                    /* after the touch: the cue ball line and the object ball line */
    float x1[PL_GD_M], y1[PL_GD_M], x2[PL_GD_M], y2[PL_GD_M];
} PlGuide;
static inline void pl_gd_add(float *x, float *y, int *n, int cap, float px, float py, float *len, int force) {
    float dx, dy, d;
    if (*n == 0) { x[0] = px; y[0] = py; *n = 1; return; }
    dx = px - x[*n - 1]; dy = py - y[*n - 1]; d = sqrtf(dx * dx + dy * dy);
    if ((d >= PL_GD_GAP || (force && d > 0.01f)) && *n < cap) { x[*n] = px; y[*n] = py; (*n)++; *len += d; }
}
static void pl_guide(const Pool *g, float ang, float power, float sa, float sb, PlGuide *o) {
    Pool t = *g; float ox[16], oy[16], len0 = 0, lenBend = 0, len1 = 0, len2 = 0, v1x = 0, v1y = 0, v2x = 0, v2y = 0, dummy = 0;
    int s, ob = -1, c_done = 0, o_done = 0;
    memset(o, 0, sizeof *o); o->hit = -1; o->bend = -1;
    t.track = 0; pl_shoot_sp(&t, ang, power, sa, sb);
    pl_gd_add(o->x0, o->y0, &o->n0, PL_GD_N, t.x[0], t.y[0], &len0, 0);
    for (s = 0; s < PL_GD_STEPS; s++) {
        float px = t.x[0], py = t.y[0], vx = t.vx[0], vy = t.vy[0]; int nr0 = t.nrail, moving;
        memcpy(ox, t.x, sizeof ox); memcpy(oy, t.y, sizeof oy);
        moving = pl_step(&t, PL_DT);
        if (ob < 0) {   /* the cue ball is still on its way to the first ball */
            if (!t.in[0]) { o->scratch = 1; pl_gd_add(o->x0, o->y0, &o->n0, PL_GD_N, t.x[0], t.y[0], &len0, 1); break; }
            if (t.first_hit >= 0) {
                float sp = sqrtf(vx * vx + vy * vy), dx = sp > 1e-3f ? vx / sp : 0, dy = sp > 1e-3f ? vy / sp : 0, bx, by, b, disc, tt = -1, cx = px, cy = py;
                ob = t.first_hit; o->hit = ob;
                bx = ox[ob] - px; by = oy[ob] - py; b = bx * dx + by * dy; disc = 4.0f * PL_R * PL_R - (bx * bx + by * by - b * b);   /* where the two circles first touch along the line the cue ball was travelling */
                if (sp > 1e-3f && disc >= 0) tt = b - sqrtf(disc);
                if (tt >= 0 && tt <= 2.0f * sp * PL_DT + 0.5f) { cx = px + dx * tt; cy = py + dy * tt; }
                o->cx = cx; o->cy = cy;
                pl_gd_add(o->x0, o->y0, &o->n0, PL_GD_N, cx, cy, &len0, 1);
                pl_gd_add(o->x1, o->y1, &o->n1, PL_GD_M, cx, cy, &len1, 0);
                pl_gd_add(o->x2, o->y2, &o->n2, PL_GD_M, ox[ob], oy[ob], &len2, 0);
                v1x = t.vx[0]; v1y = t.vy[0]; v2x = t.vx[ob]; v2y = t.vy[ob];
                continue;
            }
            pl_gd_add(o->x0, o->y0, &o->n0, PL_GD_N, t.x[0], t.y[0], &len0, t.nrail > nr0);
            if (t.nrail > nr0) { o->rails++; if (o->rails == 1) { o->bend = o->n0 - 1; lenBend = len0; } else break; }
            if (o->rails >= 1 && len0 - lenBend > PL_GD_REB) break;
            if (!moving) break;
        } else {        /* after the touch: the first bit of both balls */
            if (!c_done) {
                float sp = sqrtf(t.vx[0] * t.vx[0] + t.vy[0] * t.vy[0]), sp0 = sqrtf(v1x * v1x + v1y * v1y);
                if (!t.in[0]) { c_done = 1; o->scratch = 1; }
                else {
                    if (sp > 20.0f && sp0 > 20.0f && (t.vx[0] * v1x + t.vy[0] * v1y) < 0.94f * sp * sp0) c_done = 1;   /* a sudden turn = a cushion or another ball */
                    pl_gd_add(o->x1, o->y1, &o->n1, PL_GD_M, t.x[0], t.y[0], &len1, c_done);
                    if (len1 >= PL_GD_CUE || sp < 8.0f) c_done = 1;
                    v1x = t.vx[0]; v1y = t.vy[0]; }
            }
            if (!o_done) {
                float sp = sqrtf(t.vx[ob] * t.vx[ob] + t.vy[ob] * t.vy[ob]), sp0 = sqrtf(v2x * v2x + v2y * v2y);
                if (!t.in[ob]) { o_done = 1; o->pot = 1; }
                else {
                    if (sp > 20.0f && sp0 > 20.0f && (t.vx[ob] * v2x + t.vy[ob] * v2y) < 0.94f * sp * sp0) o_done = 1;
                    pl_gd_add(o->x2, o->y2, &o->n2, PL_GD_M, t.x[ob], t.y[ob], &len2, o_done);
                    if (len2 >= PL_GD_OBJ || sp < 8.0f) o_done = 1;
                    v2x = t.vx[ob]; v2y = t.vy[ob]; }
            }
            if ((c_done && o_done) || !moving) break;
        }
    }
    (void)dummy;
}
/* v12.8: the direction after a straight bounce off the wall(s) a ray hit (angle in = angle out; spin and speed are ignored) */
static inline float pl_rebound_angle(float ang, int wall) {
    float dx = cosf(ang), dy = sinf(ang);
    if (wall & 1) dx = -dx;
    if (wall & 2) dy = -dy;
    return atan2f(dy, dx);
}

/* ---------- the CPU player ---------- */
/* skill 0..4 (the five tables): aim error in degrees, power error in percent */
static const float PL_AIM_ERR[5] = {2.6f, 1.7f, 1.05f, 0.6f, 0.32f};
static const float PL_POW_ERR[5] = {0.09f, 0.065f, 0.045f, 0.03f, 0.02f};

static inline float pl_seg_dist(float px, float py, float ax, float ay, float bx, float by) {   /* distance of a point to the segment a-b */
    float vx = bx - ax, vy = by - ay, l2 = vx * vx + vy * vy, t = l2 > 0 ? ((px - ax) * vx + (py - ay) * vy) / l2 : 0, qx, qy;
    if (t < 0) t = 0; if (t > 1) t = 1; qx = ax + vx * t; qy = ay + vy * t;
    return sqrtf((px - qx) * (px - qx) + (py - qy) * (py - qy));
}
static int pl_path_clear(const Pool *g, float ax, float ay, float bx, float by, int skip1, int skip2, float need) {
    int i; for (i = 0; i < 16; i++) if (g->in[i] && i != skip1 && i != skip2 && pl_seg_dist(g->x[i], g->y[i], ax, ay, bx, by) < need) return 0;
    return 1;
}
/* the balls the player to move may hit first */
static inline int pl_legal_target(const Pool *g, int id) {
    int grp = g->grp[g->turn];
    if (id < 1 || !g->in[id]) return 0;
    if (grp < 0) return id != 8;
    if (pl_left(g, grp) == 0) return id == 8;
    return pl_group_of(id) == grp;
}

typedef struct { float ang, score; int ball, pocket; float dist; } PlCand;
/* every sensible straight shot from the cue ball position (cx, cy): ball -> pocket, sorted best first; returns the count (at most max) */
static int pl_candidates(const Pool *g, float cx, float cy, PlCand *out, int max) {
    int n = 0, t, p, i, j;
    Pool tmp = *g; tmp.x[0] = cx; tmp.y[0] = cy; tmp.in[0] = 1;
    for (t = 1; t < 16; t++) {
        if (!pl_legal_target(g, t)) continue;
        for (p = 0; p < PL_NP; p++) {
            float tx = g->x[t], ty = g->y[t], px = PL_PX[p], py = PL_PY[p], ex = px - tx, ey = py - ty, el = sqrtf(ex * ex + ey * ey), gx, gy, dx, dy, dl, cut, sc;
            /* aim the pocket point a little deeper than the centre so the ball drops */
            if (el < 1) continue;
            ex /= el; ey /= el; gx = tx - ex * 2 * PL_R; gy = ty - ey * 2 * PL_R;
            if (gx < PL_X0 + PL_R || gx > PL_X1 - PL_R || gy < PL_Y0 + PL_R || gy > PL_Y1 - PL_R) continue;
            dx = gx - cx; dy = gy - cy; dl = sqrtf(dx * dx + dy * dy); if (dl < 2) continue;
            cut = (dx * ex + dy * ey) / dl; if (cut < 0.25f) continue;   /* too thin a cut (more than about 75 degrees) */
            if (!pl_path_clear(&tmp, cx, cy, gx, gy, 0, t, 2 * PL_R - 0.5f)) continue;
            if (!pl_path_clear(&tmp, tx, ty, px, py, 0, t, 2 * PL_R - 0.5f)) continue;
            sc = dl * 0.6f + el * 0.9f + (1.0f - cut) * 420.0f;
            if (n < max) { out[n].ang = atan2f(dy, dx); out[n].score = sc; out[n].ball = t; out[n].pocket = p; out[n].dist = dl + el; n++; }
            else { int w = 0; for (i = 1; i < n; i++) if (out[i].score > out[w].score) w = i; if (sc < out[w].score) { out[w].ang = atan2f(dy, dx); out[w].score = sc; out[w].ball = t; out[w].pocket = p; out[w].dist = dl + el; } }
        }
    }
    for (i = 0; i < n; i++) for (j = i + 1; j < n; j++) if (out[j].score < out[i].score) { PlCand c = out[i]; out[i] = out[j]; out[j] = c; }
    return n;
}

/* run a whole shot on a copy and say how good it was: +big for a legal pot, minus for a foul; (sa, sb) = the tip point (see pl_shoot_sp).
   v12.7: a shot that goes on and leaves the cue ball with another good straight pot earns a little extra, so follow and draw have a point. */
static float pl_eval_sp(const Pool *g, float ang, float power, float sa, float sb) {
    Pool t = *g; int k = 0, i; float sc = 0; int me = g->turn;
    t.track = 0; pl_shoot_sp(&t, ang, power, sa, sb);
    while (t.phase == PH_ROLL && k++ < 9600) { t.steps += 2; if (!pl_step(&t, PL_DT * 2)) { pl_resolve(&t); break; } }
    if (t.phase == PH_ROLL) pl_resolve(&t);
    if (t.phase == PH_OVER) return t.winner == me ? 1000.0f : -1000.0f;
    if (t.foul != FL_NONE) sc -= 200.0f + (t.foul == FL_SCRATCH ? 60.0f : 0);
    for (i = 0; i < t.pot_n; i++) { int id = t.pot_list[i]; if (id == 8) continue; sc += pl_group_of(id) == g->grp[me] || g->grp[me] < 0 ? 100.0f : -90.0f; }
    if (t.turn == me && t.foul == FL_NONE) {
        PlCand c[2]; int n = pl_candidates(&t, t.x[0], t.y[0], c, 2);
        sc += 30.0f;
        sc += n > 0 ? 8.0f + 18.0f * (1.0f - (c[0].score > 600.0f ? 600.0f : c[0].score) / 600.0f) : -12.0f;   /* position: is there a good next shot? */
    }
    return sc;
}

/* The CPU's shot, worked out a few trial shots at a time so the game never freezes (v12.7): pl_ai_begin, then pl_ai_work until it says done, then read out_*.
   skill 0..4. Spin: the lowest tables only hit the centre, the middle ones also try follow / draw, the best also side english. */
#define PL_AI_MAXT 112
typedef struct { int n, i, phase, skill, done; float ang[PL_AI_MAXT], pw[PL_AI_MAXT], sa[PL_AI_MAXT], sb[PL_AI_MAXT], off[PL_AI_MAXT];
    float best, ba, bp, bsa, bsb, out_ang, out_pow, out_sa, out_sb; } PlAI;
static const float PL_AI_SPA[5] = {0.0f, 0.0f, 0.0f, 0.42f, -0.42f};
static const float PL_AI_SPB[5] = {0.0f, 0.42f, -0.42f, 0.0f, 0.0f};
static const int PL_AI_NSP[5] = {1, 1, 3, 3, 5};

static void pl_ai_finish(PlAI *ai, Pool *g) {
    int sk = ai->skill;
    ai->out_ang = ai->ba + pl_gauss(g) * PL_AIM_ERR[sk] * 0.0174533f;
    ai->out_pow = ai->bp * (1.0f + pl_gauss(g) * PL_POW_ERR[sk]);
    if (ai->out_pow > 1) ai->out_pow = 1; if (ai->out_pow < 0.05f) ai->out_pow = 0.05f;
    ai->out_sa = ai->bsa * (1.0f + pl_gauss(g) * 0.06f); ai->out_sb = ai->bsb * (1.0f + pl_gauss(g) * 0.06f);
    ai->done = 1;
}
static void pl_ai_begin(PlAI *ai, Pool *g, int skill, int spin_on) {
    PlCand cand[8]; int n, i, k, s, nc, ns;
    static const float pw[4] = {0.30f, 0.48f, 0.66f, 0.85f};
    memset(ai, 0, sizeof *ai);
    if (skill < 0) skill = 0; if (skill > 4) skill = 4;
    ai->skill = skill; ai->best = -1e9f; ai->bp = 0.5f;
    if (g->brk) {   /* the break: straight at the head ball, hard */
        ai->ba = atan2f(PL_CY - g->y[0], PL_FOOT_X - g->x[0]); ai->bp = 0.96f; pl_ai_finish(ai, g); return;
    }
    n = pl_candidates(g, g->x[0], g->y[0], cand, 8);
    nc = n < (skill == 4 ? 4 : 5) ? n : (skill == 4 ? 4 : 5); ns = spin_on ? PL_AI_NSP[skill] : 1;
    for (i = 0; i < nc; i++) for (k = 0; k < 4; k++) for (s = 0; s < ns && ai->n < PL_AI_MAXT; s++) {
        int t = ai->n++; ai->ang[t] = cand[i].ang; ai->pw[t] = pw[k]; ai->sa[t] = PL_AI_SPA[s]; ai->sb[t] = PL_AI_SPB[s]; ai->off[t] = -cand[i].score * 0.05f;
    }
}
/* nothing good among the straight shots: a soft shot at the nearest legal ball, a few angles tried */
static void pl_ai_fallback(PlAI *ai, const Pool *g) {
    int tgt = -1, i, k; float bd = 1e9f;
    for (i = 1; i < 16; i++) if (pl_legal_target(g, i)) { float dx = g->x[i] - g->x[0], dy = g->y[i] - g->y[0], d = dx * dx + dy * dy; if (d < bd) { bd = d; tgt = i; } }
    ai->phase = 1; ai->n = 0; ai->i = 0;
    if (tgt >= 0) { float a0 = atan2f(g->y[tgt] - g->y[0], g->x[tgt] - g->x[0]); for (k = 0; k < 7; k++) { int t = ai->n++; ai->ang[t] = a0 + (k - 3) * 0.025f; ai->pw[t] = 0.3f; ai->sa[t] = ai->sb[t] = 0; ai->off[t] = 0; } }
}
/* run at most maxn trial shots; returns 1 when the answer is ready (ai->out_ang / out_pow / out_sa / out_sb) */
static int pl_ai_work(PlAI *ai, Pool *g, int maxn) {
    int done_n = 0;
    while (!ai->done && done_n < maxn) {
        if (ai->i < ai->n) {
            int t = ai->i++; float s = pl_eval_sp(g, ai->ang[t], ai->pw[t], ai->sa[t], ai->sb[t]) + ai->off[t]; done_n++;
            if (s > ai->best) { ai->best = s; ai->ba = ai->ang[t]; ai->bp = ai->pw[t]; ai->bsa = ai->sa[t]; ai->bsb = ai->sb[t]; }
        } else if (ai->phase == 0 && ai->best < -150.0f) pl_ai_fallback(ai, g);
        else pl_ai_finish(ai, g);
    }
    return ai->done;
}
/* the whole answer at once. spin_on 0 = centre ball only (the old behaviour, what the older tests use) */
static void pl_ai_shot_sp(Pool *g, int skill, float *ang_out, float *pow_out, float *sa_out, float *sb_out, int spin_on) {
    PlAI *ai = (PlAI *)malloc(sizeof(PlAI));
    pl_ai_begin(ai, g, skill, spin_on); while (!pl_ai_work(ai, g, 64)) { }
    *ang_out = ai->out_ang; *pow_out = ai->out_pow; if (sa_out) *sa_out = ai->out_sa; if (sb_out) *sb_out = ai->out_sb;
    free(ai);
}
static void pl_ai_shot(Pool *g, int skill, float *ang_out, float *pow_out) { pl_ai_shot_sp(g, skill, ang_out, pow_out, 0, 0, 0); }

/* ball in hand for the CPU: the best spot among a grid (judged by the best straight shot from there) */
static void pl_ai_place(Pool *g) {
    int ix, iy, n; float bx = g->x[0], by = g->y[0], bs = 1e9f; PlCand c[2];
    for (iy = 0; iy < 11; iy++) for (ix = 0; ix < 24; ix++) {
        float x = PL_X0 + 24 + ix * 27.0f, y = PL_Y0 + 20 + iy * 28.0f;
        if (!pl_spot_ok(g, x, y)) continue;
        n = pl_candidates(g, x, y, c, 2);
        if (n > 0 && c[0].score < bs) { bs = c[0].score; bx = x; by = y; }
    }
    if (bs > 1e8f) { bx = PL_BREAK_X; by = PL_CY; if (!pl_spot_ok(g, bx, by)) { int k; for (k = 0; k < 400; k++) { float x = PL_X0 + 60 + (k % 20) * 20.0f, y = PL_Y0 + 30 + (k / 20) * 14.0f; if (pl_spot_ok(g, x, y)) { bx = x; by = y; break; } } } }
    g->x[0] = bx; g->y[0] = by; g->bih = 0;
}
#endif
