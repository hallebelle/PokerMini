/* Poker Mini - multi-player engine for Quick Play and the Exhibition (no SDL).
   2 or 4 players, stacks that carry over from hand to hand, all-ins with side pots,
   blinds that rise every 10 hands, CPU opponents. Seat 0 is you. Sophia's engine in logic.h is separate and unchanged.
   Quick Play is fixed limit. v8.9: the Exhibition (and its boss) set m->nl = 1 = free bet size: any raise from the minimum up to all-in
   (see mp_cap_pay / mp_min_by / mp_act_by). */
#ifndef PQ_MULTI_H
#define PQ_MULTI_H
#include "logic.h"

#define MP_MAX 4
typedef struct {
    int n, start_stack, start_bb, bb, level, hands, button;
    int stack[MP_MAX], alive[MP_MAX], style[MP_MAX];
    int deck[52];                       /* seat i holds deck[2i], deck[2i+1]; the board is deck[8..12] */
    int folded[MP_MAX], allin[MP_MAX], pending[MP_MAX], c[MP_MAX], r[MP_MAX];   /* committed this hand / this round */
    int stage, nbets, toact, over, showdown, sb_seat, bb_seat;
    int sc[MP_MAX], won[MP_MAX];        /* showdown scores, chips received this hand */
    int game_over, place;               /* game_over: 0 playing, 1 you won it all, 2 you are out; place = your finishing place */
    int nl;                             /* v8.9: 1 = free bet size (Exhibition + boss), 0 = fixed limit (Quick Play) */
    int last_inc;                       /* v8.9: size of the biggest raise increment this round (the next raise is at least this much) */
    int boss;                           /* 1 = the Exhibition boss (Sophia): her seat decides like Sophia at her own table, not like a CPU */
} MP;

static const int MP_START_BB[4] = {4, 10, 20, 40};          /* Quick Play: big blind for the 100 / 250 / 500 / 1000 stacks */
static const int QP_STACK[4] = {100, 250, 500, 1000};      /* Quick Play starting chips */
static const int EX_START_BB[EX_N] = {2, 2, 4, 20, 40, 100};     /* Exhibition: big blind for the 10 / 50 / 100 / 500 / 1000 / 5000 (boss) buy-ins (the 10 game is only 5 big blinds deep; the boss game is 50 big blinds deep = a long match) */
static const int MP_MULT[10] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
static inline int mp_bb_at(int start_bb, int level) { return start_bb * MP_MULT[level < 9 ? level : 9]; }

static inline int mp_alive_count(const MP *m) { int i, k = 0; for (i = 0; i < m->n; i++) k += m->alive[i]; return k; }
static inline int mp_next_alive(const MP *m, int s) { int k, t; for (k = 1; k <= m->n; k++) { t = (s + k) % m->n; if (m->alive[t]) return t; } return s; }
static inline int mp_active(const MP *m, int i) { return m->alive[i] && !m->folded[i] && !m->allin[i]; }
static inline int mp_nonfolded(const MP *m) { int i, k = 0; for (i = 0; i < m->n; i++) k += m->alive[i] && !m->folded[i]; return k; }
static inline int mp_pot(const MP *m) { int i, t = 0; for (i = 0; i < m->n; i++) t += m->c[i]; return t; }
static inline int mp_maxr(const MP *m) { int i, t = 0; for (i = 0; i < m->n; i++) if (m->r[i] > t) t = m->r[i]; return t; }
static inline int mp_need(const MP *m, int p) { return mp_maxr(m) - m->r[p]; }           /* chips to stay in (may exceed your stack) */
static inline int mp_pay_call(const MP *m, int p) { int n = mp_need(m, p); return n < m->stack[p] ? n : m->stack[p]; }
static inline int mp_size(const MP *m) { return m->stage < 2 ? m->bb : 2 * m->bb; }
/* v8.9 free bet size. The most chips seat p can put in right now: its own stack, but never more than the richest other live seat can match
   (chips nobody can call stay in the stack, so no chips are ever returned). */
static inline int mp_cap_pay(const MP *m, int p) {
    int i, cap = 0, pay;
    for (i = 0; i < m->n; i++) if (i != p && m->alive[i] && !m->folded[i] && m->r[i] + m->stack[i] > cap) cap = m->r[i] + m->stack[i];
    pay = cap - m->r[p]; if (pay > m->stack[p]) pay = m->stack[p];
    return pay < 0 ? 0 : pay;
}
/* smallest legal raise INCREMENT (on top of the call): the fixed size, or the last raise if that was bigger; all-in may be smaller */
static inline int mp_min_by(const MP *m, int p) {
    int mn = mp_size(m), mx = mp_cap_pay(m, p) - mp_need(m, p);
    if (m->last_inc > mn) mn = m->last_inc;
    return mn > mx ? mx : mn;
}
static inline int mp_max_by(const MP *m, int p) { int mx = mp_cap_pay(m, p) - mp_need(m, p); return mx < 0 ? 0 : mx; }
static inline int mp_can_raise(const MP *m, int p) {
    int i, others = 0;
    for (i = 0; i < m->n; i++) if (i != p && mp_active(m, i)) others++;
    if (m->nl) return others > 0 && mp_max_by(m, p) > 0;   /* free size: anything up to all-in (after the 4th bet only all-in) */
    return m->nbets < MAX_BETS && others > 0 && m->stack[p] >= mp_need(m, p) + mp_size(m);   /* a short stack can only call (all-in) or fold */
}
static inline void mp_commit(MP *m, int p, int pay) {
    if (pay > m->stack[p]) pay = m->stack[p];
    m->stack[p] -= pay; m->c[p] += pay; m->r[p] += pay; if (m->stack[p] == 0) m->allin[p] = 1;
}
static inline void mp_init(MP *m, int n, int stack, int start_bb) {
    int i; memset(m, 0, sizeof *m);
    m->n = n; m->start_stack = stack; m->start_bb = start_bb; m->bb = start_bb;
    for (i = 0; i < n; i++) { m->stack[i] = stack; m->alive[i] = 1; m->style[i] = rnd(3); }
    m->button = rnd(n);
}
/* set who has to act at the start of a betting round. returns 1 if somebody has to act, 0 if the board just runs out */
static inline int mp_round_setup(MP *m, int first) {
    int i, k, act = 0, p = -1;
    for (i = 0; i < m->n; i++) { m->pending[i] = 0; if (mp_active(m, i)) { act++; p = i; } }
    if (act >= 2) { for (i = 0; i < m->n; i++) m->pending[i] = mp_active(m, i); }
    else if (act == 1 && mp_need(m, p) > 0) m->pending[p] = 1;
    m->toact = -1;
    for (k = 0; k < m->n; k++) { int t = (first + k) % m->n; if (m->pending[t]) { m->toact = t; break; } }
    return m->toact >= 0;
}
static inline int mp_finish(MP *m);
static inline int mp_next_street(MP *m) {
    for (;;) {
        int i;
        m->stage++;
        if (m->stage >= 4) return mp_finish(m);
        for (i = 0; i < m->n; i++) m->r[i] = 0;
        m->nbets = 0; m->last_inc = 0;
        if (mp_round_setup(m, mp_next_alive(m, m->button))) return 0;   /* postflop: first live seat after the button (heads-up: the non-button) */
    }
}
static inline void mp_start_hand(MP *m) {
    int i, na = mp_alive_count(m), sb, bb;
    if (m->hands > 0) m->button = mp_next_alive(m, m->button);
    m->hands++; m->level = (m->hands - 1) / 10; m->bb = mp_bb_at(m->start_bb, m->level);
    for (i = 0; i < 52; i++) m->deck[i] = i;
    for (i = 51; i > 0; i--) { int j = rnd(i + 1), t = m->deck[i]; m->deck[i] = m->deck[j]; m->deck[j] = t; }
    for (i = 0; i < m->n; i++) { m->folded[i] = !m->alive[i]; m->allin[i] = 0; m->pending[i] = 0; m->c[i] = m->r[i] = 0; m->sc[i] = m->won[i] = 0; }
    m->stage = 0; m->nbets = 1; m->last_inc = 0; m->over = 0; m->showdown = 0;
    if (na == 2) { sb = m->button; bb = mp_next_alive(m, sb); } else { sb = mp_next_alive(m, m->button); bb = mp_next_alive(m, sb); }
    m->sb_seat = sb; m->bb_seat = bb;
    mp_commit(m, sb, m->bb / 2); mp_commit(m, bb, m->bb);
    if (!mp_round_setup(m, na == 2 ? sb : mp_next_alive(m, bb))) mp_next_street(m);   /* everybody all-in from the blinds: run it out */
}
/* split the pot(s). returns 1 (the hand is over) */
static inline int mp_finish(MP *m) {
    int i, nf = mp_nonfolded(m), total = mp_pot(m), prev = 0, paid = 0, game;
    m->over = 1;
    for (i = 0; i < m->n; i++) m->won[i] = 0;
    if (nf == 1) { for (i = 0; i < m->n; i++) if (m->alive[i] && !m->folded[i]) m->won[i] = total; }
    else {
        m->showdown = 1; m->stage = 4;
        for (i = 0; i < m->n; i++) if (m->alive[i] && !m->folded[i]) {
            int a[7]; a[0] = m->deck[2*i]; a[1] = m->deck[2*i+1]; memcpy(a + 2, m->deck + 8, 5 * sizeof(int)); m->sc[i] = best7(a);
        }
        for (;;) {   /* one slice of the pot per all-in level */
            int level = 0, slice = 0, nw = 0, best = -1, w[MP_MAX], k, share, rem, t;
            for (i = 0; i < m->n; i++) if (m->alive[i] && !m->folded[i] && m->c[i] > prev && (level == 0 || m->c[i] < level)) level = m->c[i];
            if (!level) break;
            for (i = 0; i < m->n; i++) slice += (m->c[i] < level ? m->c[i] : level) - (m->c[i] < prev ? m->c[i] : prev);
            for (i = 0; i < m->n; i++) if (m->alive[i] && !m->folded[i] && m->c[i] >= level && m->sc[i] > best) best = m->sc[i];
            for (i = 0; i < m->n; i++) if (m->alive[i] && !m->folded[i] && m->c[i] >= level && m->sc[i] == best) w[nw++] = i;
            share = slice / nw; rem = slice % nw;
            for (k = 0, t = m->button; k < m->n && rem > 0; k++) {   /* odd chips go to the first winner after the button */
                t = (m->button + 1 + k) % m->n;
                for (i = 0; i < nw; i++) if (w[i] == t && rem > 0) { m->won[t]++; rem--; }
            }
            for (i = 0; i < nw; i++) m->won[w[i]] += share;
            prev = level;
        }
        for (i = 0; i < m->n; i++) paid += m->won[i];
        if (paid < total) {   /* chips of a folded player above every live bet (cannot normally happen): best live hand takes them */
            int bi = -1; for (i = 0; i < m->n; i++) if (m->alive[i] && !m->folded[i] && (bi < 0 || m->sc[i] > m->sc[bi])) bi = i;
            m->won[bi] += total - paid;
        }
    }
    for (i = 0; i < m->n; i++) { m->stack[i] += m->won[i]; if (m->alive[i] && m->stack[i] == 0) m->alive[i] = 0; }
    game = mp_alive_count(m);
    if (!m->alive[0]) { m->game_over = 2; m->place = game + 1; }
    else if (game == 1) { m->game_over = 1; m->place = 1; }
    return 1;
}
/* apply an action for m->toact. returns 1 when the hand just ended */
static inline int mp_act_by(MP *m, int a, int by) {   /* by = raise increment on top of the call (free bet size only; 0 = the minimum) */
    int p = m->toact, need, k;
    if (m->over || p < 0) return 0;
    need = mp_need(m, p); m->pending[p] = 0;
    if (a == A_FOLD && need > 0) m->folded[p] = 1;
    else if (a == A_RAISE && mp_can_raise(m, p)) {
        if (m->nl) {
            int mx = mp_max_by(m, p), mn = mp_min_by(m, p);
            if (by < mn) by = mn;
            if (by > mx) by = mx;
            if (m->nbets >= MAX_BETS) by = mx;   /* the 4-bet cap: from here on a raise is all-in */
            if (by > m->last_inc) m->last_inc = by;
        } else by = mp_size(m);
        mp_commit(m, p, need + by); m->nbets++;
        for (k = 0; k < m->n; k++) if (k != p && mp_active(m, k)) m->pending[k] = 1;
    } else mp_commit(m, p, need);
    if (mp_nonfolded(m) == 1) return mp_finish(m);
    for (k = 1; k <= m->n; k++) { int t = (p + k) % m->n; if (m->pending[t]) { m->toact = t; return 0; } }
    return mp_next_street(m);
}

static inline int mp_act(MP *m, int a) { return mp_act_by(m, a, 0); }

/* ---------- CPU opponents ---------- */
/* equity (win chance, ties shared) of 'hole' against nopp random hands */
static inline float equity_n(const int *hole, const int *board, int nb, int samples, int nopp) {
    int used[52] = {0}, unk[52], nu = 0, i, s, need = 2 * nopp + (5 - nb); float w = 0;
    used[hole[0]] = used[hole[1]] = 1; for (i = 0; i < nb; i++) used[board[i]] = 1;
    for (i = 0; i < 52; i++) if (!used[i]) unk[nu++] = i;
    for (s = 0; s < samples; s++) {
        int a[7], b[7], bd[5], sa, win = 1, ties = 0, k;
        for (i = 0; i < need; i++) { int j = i + rnd(nu - i), t = unk[i]; unk[i] = unk[j]; unk[j] = t; }
        for (i = 0; i < nb; i++) bd[i] = board[i];
        for (i = nb; i < 5; i++) bd[i] = unk[2 * nopp + (i - nb)];
        a[0] = hole[0]; a[1] = hole[1]; memcpy(a + 2, bd, sizeof bd); sa = best7(a);
        for (k = 0; k < nopp && win; k++) {
            int sb; b[0] = unk[2*k]; b[1] = unk[2*k+1]; memcpy(b + 2, bd, sizeof bd); sb = best7(b);
            if (sb > sa) win = 0; else if (sb == sa) ties++;
        }
        if (win) w += 1.0f / (float)(ties + 1);
    }
    return w / (float)samples;
}
static inline float pre_equity_n(const int *hole, int nopp) {
    static float cache[4][338]; static int init = 0;
    int r0 = hole[0] % 13, r1 = hole[1] % 13, hi = r0 > r1 ? r0 : r1, lo = r0 > r1 ? r1 : r0, su = hole[0] / 13 == hole[1] / 13, k;
    if (!init) { int i, j; for (i = 0; i < 4; i++) for (j = 0; j < 338; j++) cache[i][j] = -1.0f; init = 1; }
    k = (hi * 13 + lo) + su * 169;
    if (cache[nopp][k] < 0) { int h[2], b[1] = {0}; h[0] = hi; h[1] = su ? lo : 13 + lo; cache[nopp][k] = equity_n(h, b, 0, 700, nopp); }
    return cache[nopp][k];
}
/* the boss seat: the same brain as Sophia at her own table (ai_decide_with in logic.h: little noise, 400 samples, honest pot odds, bluffs), heads-up only, no tilt */
static inline int mp_decide_boss(const MP *m, int p) {
    static const float A[3] = {0.00f, 0.03f, 0.06f}, B[3] = {0.08f, 0.12f, 0.16f}, T[3] = {0.02f, 0.00f, 0.01f};
    int hole[2], board[5], nb = m->stage == 0 ? 0 : m->stage + 2, i, need = mp_need(m, p), pay = mp_pay_call(m, p), pot = mp_pot(m), aggr, st = m->style[p] % 3;
    float e, odds;
    hole[0] = m->deck[2*p]; hole[1] = m->deck[2*p+1];
    for (i = 0; i < nb; i++) board[i] = m->deck[8+i];
    e = nb == 0 ? pre_equity_n(hole, 1) : equity_n(hole, board, nb, AI_SAMPLES, 1);
    aggr = m->nbets - (m->stage == 0 ? 1 : 0);
    if (need > 0 && aggr > 0) e -= 0.045f * (float)aggr;
    if (need > 0 && m->nl) {   /* v8.9 free bet size: a big bet means a stronger hand than a random one, so a big bet (or shove) needs more equity to call */
        float f = (float)need / (float)(pot - need > 1 ? pot - need : 1); e -= 0.05f * (f > 2.0f ? 2.0f : f);
        if (pay * 2 >= m->stack[p]) e -= 0.04f;
    }
    e += (frnd() - 0.5f) * AI_NOISE;
    if (need == 0) {
        if (e > 0.88f && frnd() < 0.20f) return A_CALL;                                                   /* occasional slow-play */
        if (mp_can_raise(m, p) && e > 0.60f - A[st]) return A_RAISE;
        if (mp_can_raise(m, p) && m->stage > 0 && e < 0.50f && frnd() < B[st]) return A_RAISE;            /* bluff */
        return A_CALL;
    }
    odds = (float)pay / (float)(pot + pay);
    if (mp_can_raise(m, p) && e > 0.70f - A[st] && !(e < 0.82f && frnd() < 0.15f)) return A_RAISE;
    if (mp_can_raise(m, p) && e > 0.40f && frnd() < B[st] * 0.4f) return A_RAISE;                         /* semi-bluff raise */
    if (e > odds + AI_CALL_MARGIN + T[st]) return A_CALL;
    return A_FOLD;
}
/* a CPU seat decides. Clearly weaker than Sophia: noisier, simpler, no tilt. Uses only what that seat can see. */
static inline int mp_decide(const MP *m, int p) {
    if (m->boss && p == 1) return mp_decide_boss(m, p);
    static const float A[3] = {0.00f, 0.07f, 0.03f}, B[3] = {0.05f, 0.10f, 0.14f}, T[3] = {0.02f, 0.00f, -0.03f};
    int hole[2], board[5], nb = m->stage == 0 ? 0 : m->stage + 2, i, nopp = 0, need = mp_need(m, p), pay = mp_pay_call(m, p), pot = mp_pot(m), aggr, st = m->style[p] % 3;
    float e, fair, s, odds;
    hole[0] = m->deck[2*p]; hole[1] = m->deck[2*p+1];
    for (i = 0; i < nb; i++) board[i] = m->deck[8+i];
    for (i = 0; i < m->n; i++) if (i != p && m->alive[i] && !m->folded[i]) nopp++;
    if (nopp < 1) return A_CALL;
    e = nb == 0 ? pre_equity_n(hole, nopp) : equity_n(hole, board, nb, nopp >= 3 ? 300 : 400, nopp);
    aggr = m->nbets - (m->stage == 0 ? 1 : 0);
    if (need > 0 && aggr > 0) e -= 0.04f * (float)aggr;
    if (need > 0 && m->nl) {   /* v8.9 free bet size: a big bet means a stronger hand than a random one, so a big bet (or shove) needs more equity to call */
        float f = (float)need / (float)(pot - need > 1 ? pot - need : 1); e -= 0.05f * (f > 2.0f ? 2.0f : f);
        if (pay * 2 >= m->stack[p]) e -= 0.04f;
    }
    e += (frnd() - 0.5f) * 0.13f;   /* v13.25: was 0.20 */
    fair = 1.0f / (float)(nopp + 1); s = (e - fair) / (1.0f - fair);
    if (need == 0) {
        if (mp_can_raise(m, p) && s > 0.45f - A[st]) return A_RAISE;
        if (mp_can_raise(m, p) && m->stage > 0 && s < 0.10f && frnd() < B[st]) return A_RAISE;
        return A_CALL;
    }
    odds = (float)pay / (float)(pot + pay);
    if (mp_can_raise(m, p) && s > 0.55f - A[st] && frnd() > 0.15f) return A_RAISE;
    if (e > odds + 0.06f + T[st]) return A_CALL;   /* v13.25: was 0.05 */
    return A_FOLD;
}
/* v8.9: the same decision plus HOW MUCH when it raises (free bet size only; *by stays 0 otherwise = the fixed size).
   Sizes: half pot / three quarters / pot, now and then all-in; anything that would commit about 60 percent of what is left becomes all-in;
   after the 4th bet a raise is all-in only, and the CPU does that only half of the time (else it just calls). */
static inline int mp_decide_by(const MP *m, int p, int *by) {
    int a = mp_decide(m, p); *by = 0;
    if (a == A_RAISE && m->nl) {
        int mx = mp_max_by(m, p), mn = mp_min_by(m, p), pot = mp_pot(m) + mp_need(m, p), tgt; float r = frnd();
        if (m->nbets >= MAX_BETS) { if (frnd() < 0.5f) { *by = mx; return A_RAISE; } return A_CALL; }
        tgt = r < 0.35f ? pot / 2 : r < 0.75f ? pot * 3 / 4 : pot;
        if (frnd() < 0.08f) tgt = mx;
        if (tgt < mn) tgt = mn;
        if (tgt > mx) tgt = mx;
        if (tgt * 10 >= mx * 6) tgt = mx;
        *by = tgt;
    }
    return a;
}
#endif
