/* Poker Mini - 8 ball engine for the Event Center (v12.6, no SDL).
   Everything is in NATIVE screen pixels of the 752x560 table picture (assets/pool_table.png); the game scales to the real screen.
   Heads-up standard 8 ball, no called pockets (bar rules), ball in hand after a foul. Seat 0 = you (you break), seat 1 = the CPU.
   Fixed time step, equal-mass ball collisions, rolling friction, circular pocket capture. Deterministic from the seed.
   All tuning numbers are in the block "TUNING" below. */
#ifndef PQ_POOL_H
#define PQ_POOL_H
#include <math.h>
#include <string.h>

/* ---------- TUNING ---------- */
#define PL_R 8.5f                 /* ball radius */
#define PL_X0 44.0f               /* cushion inner edges (from the picture) */
#define PL_X1 706.5f
#define PL_Y0 46.5f
#define PL_Y1 373.5f
#define PL_DT (1.0f / 480.0f)     /* one physics step (seconds); the game runs 8 per frame */
#define PL_DRAG 0.62f             /* velocity loss per second (proportional) */
#define PL_FRIC 52.0f             /* constant deceleration (px / s^2) */
#define PL_STOP 5.0f              /* below this speed (px / s) a ball stops */
#define PL_E_BALL 0.96f           /* ball-ball restitution */
#define PL_E_RAIL 0.74f           /* cushion restitution */
#define PL_VMIN 70.0f             /* cue ball speed at the weakest shot */
#define PL_VMAX 1500.0f           /* ... and at full power */
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
static inline int pl_all_still(const Pool *g) { int i; for (i = 0; i < 16; i++) if (g->in[i] && (g->vx[i] != 0 || g->vy[i] != 0)) return 0; return 1; }

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

static inline void pl_new(Pool *g, unsigned seed) {
    int i; memset(g, 0, sizeof *g);
    g->rng = seed ? seed : 2463534242u;
    for (i = 0; i < 16; i++) g->in[i] = 1;
    g->x[0] = PL_BREAK_X; g->y[0] = PL_CY;
    pl_rack(g);
    g->turn = 0; g->phase = PH_AIM; g->winner = -1; g->grp[0] = g->grp[1] = -1; g->brk = 1; g->bih = 0; g->foul = FL_NONE; g->last_turn = -1; g->first_hit = -1;
}

/* is the point (x, y) a legal spot for the cue ball (inside the cushions, clear of every other ball)? */
static inline int pl_spot_ok(const Pool *g, float x, float y) {
    int i;
    if (x < PL_X0 + PL_R || x > PL_X1 - PL_R || y < PL_Y0 + PL_R || y > PL_Y1 - PL_R) return 0;
    for (i = 1; i < 16; i++) if (g->in[i]) { float dx = g->x[i] - x, dy = g->y[i] - y; if (dx * dx + dy * dy < 4 * PL_R * PL_R + 0.5f) return 0; }
    return 1;
}

/* take the shot: angle in radians (0 = right, y grows downwards), power 0..1 */
static inline void pl_shoot(Pool *g, float ang, float power) {
    float v; int i;
    if (power < 0) power = 0;
    if (power > 1) power = 1;
    v = PL_VMIN + (PL_VMAX - PL_VMIN) * power;
    g->vx[0] = cosf(ang) * v; g->vy[0] = sinf(ang) * v;
    g->phase = PH_ROLL; g->shooter = g->turn; g->first_hit = -1; g->rail_after = 0; g->nrail = 0; g->pot_n = 0; g->scratch = 0; g->pocketed_8 = 0; g->steps = 0;
    g->own_left0 = g->grp[g->turn] >= 0 ? pl_left(g, g->grp[g->turn]) : 15; g->bih = 0;
    for (i = 0; i < 16; i++) g->pot_pocket[i] = -1;
}

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
            g->in[8] = 1; g->x[8] = PL_FOOT_X; g->y[8] = PL_CY; g->vx[8] = g->vy[8] = 0;
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
    if (g->scratch) { g->in[0] = 1; g->vx[0] = g->vy[0] = 0; g->x[0] = PL_BREAK_X; g->y[0] = PL_CY; }   /* the cue ball is back on the table (ball in hand: the player moves it) */
    if (g->bih && !pl_spot_ok(g, g->x[0], g->y[0])) {   /* find a free spot near the head spot */
        float tx, ty; int ok = 0, k;
        for (k = 0; k < 400 && !ok; k++) { tx = PL_X0 + 60 + (k % 20) * 20.0f; ty = PL_Y0 + 30 + (k / 20) * 14.0f; if (pl_spot_ok(g, tx, ty)) { g->x[0] = tx; g->y[0] = ty; ok = 1; } }
    }
    g->phase = PH_AIM;
}

static inline void pl_potted(Pool *g, int i, int pk) {
    g->in[i] = 0; g->vx[i] = g->vy[i] = 0; g->ev_pot++;
    if (i == 0) { g->scratch = 1; return; }
    if (g->pot_n < 16) { g->pot_list[g->pot_n] = i; g->pot_pocket[g->pot_n] = pk; g->pot_n++; }
    if (i == 8) g->pocketed_8 = 1;
}

/* one physics step; returns 1 while something still moves */
static int pl_step(Pool *g, float dt) {
    int i, j, moving = 0;
    float rr = 2.0f * PL_R;
    for (i = 0; i < 16; i++) {
        float sp, k;
        if (!g->in[i]) continue;
        g->x[i] += g->vx[i] * dt; g->y[i] += g->vy[i] * dt;
        sp = sqrtf(g->vx[i] * g->vx[i] + g->vy[i] * g->vy[i]);
        if (sp > 0) {
            float ns = sp - (PL_FRIC + PL_DRAG * sp) * dt;
            if (ns < PL_STOP) { g->vx[i] = g->vy[i] = 0; } else { k = ns / sp; g->vx[i] *= k; g->vy[i] *= k; }
        }
    }
    /* pockets */
    for (i = 0; i < 16; i++) if (g->in[i]) {
        int p; for (p = 0; p < PL_NP; p++) { float dx = g->x[i] - PL_PX[p], dy = g->y[i] - PL_PY[p]; if (dx * dx + dy * dy < PL_PR[p] * PL_PR[p]) { pl_potted(g, i, p); break; } }
    }
    /* cushions */
    for (i = 0; i < 16; i++) if (g->in[i]) {
        int hit = 0;
        if (g->x[i] < PL_X0 + PL_R) { g->x[i] = PL_X0 + PL_R + (PL_X0 + PL_R - g->x[i]) * 0.5f; if (g->vx[i] < 0) g->vx[i] = -g->vx[i] * PL_E_RAIL; g->vy[i] *= 0.985f; hit = 1; }
        else if (g->x[i] > PL_X1 - PL_R) { g->x[i] = PL_X1 - PL_R - (g->x[i] - (PL_X1 - PL_R)) * 0.5f; if (g->vx[i] > 0) g->vx[i] = -g->vx[i] * PL_E_RAIL; g->vy[i] *= 0.985f; hit = 1; }
        if (g->y[i] < PL_Y0 + PL_R) { g->y[i] = PL_Y0 + PL_R + (PL_Y0 + PL_R - g->y[i]) * 0.5f; if (g->vy[i] < 0) g->vy[i] = -g->vy[i] * PL_E_RAIL; g->vx[i] *= 0.985f; hit = 1; }
        else if (g->y[i] > PL_Y1 - PL_R) { g->y[i] = PL_Y1 - PL_R - (g->y[i] - (PL_Y1 - PL_R)) * 0.5f; if (g->vy[i] > 0) g->vy[i] = -g->vy[i] * PL_E_RAIL; g->vx[i] *= 0.985f; hit = 1; }
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
    for (i = 0; i < 16; i++) if (g->in[i] && (g->vx[i] != 0 || g->vy[i] != 0)) moving = 1;
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
typedef struct { int hit; float t, cx, cy; } PlRay;   /* hit = ball id, -1 = a cushion; t = distance the cue ball travels; (cx, cy) = the cue ball centre at the contact */
static inline PlRay pl_ray(const Pool *g, float ang) {
    PlRay r; float dx = cosf(ang), dy = sinf(ang), best = 1e9f; int i;
    r.hit = -1; r.t = 0; r.cx = g->x[0]; r.cy = g->y[0];
    for (i = 1; i < 16; i++) if (g->in[i]) {
        float ox = g->x[i] - g->x[0], oy = g->y[i] - g->y[0], b = ox * dx + oy * dy, c = ox * ox + oy * oy - 4 * PL_R * PL_R, disc = b * b - c;
        if (b > 0 && disc >= 0) { float t = b - sqrtf(disc); if (t >= 0 && t < best) { best = t; r.hit = i; } }
    }
    {   /* the cushions (for the cue ball centre) */
        float tw = 1e9f, t;
        if (dx > 1e-6f) { t = (PL_X1 - PL_R - g->x[0]) / dx; if (t < tw) tw = t; } else if (dx < -1e-6f) { t = (PL_X0 + PL_R - g->x[0]) / dx; if (t < tw) tw = t; }
        if (dy > 1e-6f) { t = (PL_Y1 - PL_R - g->y[0]) / dy; if (t < tw) tw = t; } else if (dy < -1e-6f) { t = (PL_Y0 + PL_R - g->y[0]) / dy; if (t < tw) tw = t; }
        if (r.hit < 0 || tw < best) { r.hit = -1; best = tw; }
    }
    if (best < 0) best = 0;
    r.t = best; r.cx = g->x[0] + dx * best; r.cy = g->y[0] + dy * best;
    return r;
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

/* run a whole shot on a copy and say how good it was: +big for a legal pot, minus for a foul */
static float pl_eval(const Pool *g, float ang, float power) {
    Pool t = *g; int k = 0, i; float sc = 0; int me = g->turn;
    pl_shoot(&t, ang, power);
    while (t.phase == PH_ROLL && k++ < 9600) { t.steps += 2; if (!pl_step(&t, PL_DT * 2)) { pl_resolve(&t); break; } }
    if (t.phase == PH_ROLL) pl_resolve(&t);
    if (t.phase == PH_OVER) return t.winner == me ? 1000.0f : -1000.0f;
    if (t.foul != FL_NONE) sc -= 200.0f + (t.foul == FL_SCRATCH ? 60.0f : 0);
    for (i = 0; i < t.pot_n; i++) { int id = t.pot_list[i]; if (id == 8) continue; sc += pl_group_of(id) == g->grp[me] || g->grp[me] < 0 ? 100.0f : -90.0f; }
    if (t.turn == me && t.foul == FL_NONE) sc += 30.0f;
    return sc;
}

/* the CPU's shot. skill 0..4. Fills the angle and power it will shoot with (errors already added). */
static void pl_ai_shot(Pool *g, int skill, float *ang_out, float *pow_out) {
    PlCand cand[8]; int n, i, k, bi = -1; float bs = -1e9f, ba = 0, bp = 0.5f;
    static const float pw[4] = {0.30f, 0.48f, 0.66f, 0.85f};
    if (skill < 0) skill = 0; if (skill > 4) skill = 4;
    if (g->brk) {   /* the break: straight at the head ball, hard */
        float dx = PL_FOOT_X - g->x[0], dy = PL_CY - g->y[0]; ba = atan2f(dy, dx); bp = 0.96f;
    } else {
        n = pl_candidates(g, g->x[0], g->y[0], cand, 8);
        for (i = 0; i < n && i < 5; i++) for (k = 0; k < 4; k++) {
            float s = pl_eval(g, cand[i].ang, pw[k]) - cand[i].score * 0.05f;
            if (s > bs) { bs = s; ba = cand[i].ang; bp = pw[k]; bi = i; }
        }
        if (bi < 0 || bs < -150.0f) {   /* nothing good: a soft shot at the nearest legal ball, a few angles tried */
            int tgt = -1; float bd = 1e9f, sbest = -1e9f, sa = 0;
            for (i = 1; i < 16; i++) if (pl_legal_target(g, i)) { float dx = g->x[i] - g->x[0], dy = g->y[i] - g->y[0], d = dx * dx + dy * dy; if (d < bd) { bd = d; tgt = i; } }
            if (tgt >= 0) {
                float a0 = atan2f(g->y[tgt] - g->y[0], g->x[tgt] - g->x[0]);
                for (k = 0; k < 7; k++) { float a = a0 + (k - 3) * 0.025f, s = pl_eval(g, a, 0.3f); if (s > sbest) { sbest = s; sa = a; } }
                if (bi < 0 || sbest > bs) { ba = sa; bp = 0.3f; }
            }
        }
    }
    *ang_out = ba + pl_gauss(g) * PL_AIM_ERR[skill] * 0.0174533f;
    *pow_out = bp * (1.0f + pl_gauss(g) * PL_POW_ERR[skill]);
    if (*pow_out > 1) *pow_out = 1; if (*pow_out < 0.05f) *pow_out = 0.05f;
}

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
