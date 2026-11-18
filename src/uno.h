/* Poker Mini v13.7 - UNO with Sophia: the rules, the deck and Sophia's play. No SDL (like pool.h): the tests run it without a screen.
   Card = colour * 13 + k, colour 0 Yellow, 1 Red, 2 Blue, 3 Green; k 0..9 = the digit, 10 = +2, 11 = Skip, 12 = Reverse.
   52 = Wild, 53 = Wild +4. 108 cards: per colour one 0, two of 1..9, two Skip, two Reverse, two +2 (25 x 4 = 100), 4 Wild, 4 Wild +4.
   RULES (v1, two players, one round to an empty hand, free: no cash, no energy):
   - Reverse acts as Skip with two players. No stacking of +2 / +4. +2: the opponent draws 2 and loses the turn. Skip: the opponent loses the turn.
   - Wild: the player names the colour. Wild +4 is always legal (no challenge): the opponent draws 4 and loses the turn.
   - First card: flipped from the deck. Wild +4 goes back and the deck is shuffled again. Official behaviour for the rest: +2 = the first player draws 2 and loses the turn,
     Skip = the first player loses the turn, Reverse = (two players) the first player loses the turn, plain Wild = the first player names the colour.
   - Draw: you draw ONE card; a drawn card that is playable may be played (only that card) or kept. Empty deck: the discard pile (except the top card) is shuffled back.
   - UNO: call it (Y) while you hold two cards or right after you play down to one. A player left with one card who has not called is "exposed": the opponent can catch it
     (Sophia catches you at the start of her turn, you catch her with Y at the start of yours) and the catched player draws 2. */
#ifndef PQ_UNO_H
#define PQ_UNO_H
#include <stdlib.h>
#include <string.h>

#define UNO_DECK 108
#define UNO_MAXH 108
#define UNO_WILD 52
#define UNO_WILD4 53
enum { UK_PLUS2 = 10, UK_SKIP = 11, UK_REV = 12 };
enum { UE_NONE, UE_PLAY, UE_DRAW };   /* what the last move of a player was */

typedef struct {
    int draw[UNO_DECK], nd;               /* draw pile (top = draw[nd - 1]) */
    int disc[UNO_DECK], ndisc;            /* discard pile (top = disc[ndisc - 1]) */
    int hand[2][UNO_MAXH], nh[2];         /* 0 = you, 1 = Sophia */
    int color;                            /* the active colour (-1 = a first Wild still waits for its colour) */
    int turn, over, winner;
    int drawn;                            /* index in hand[turn] of a card that was just drawn and may be played, else -1 */
    int called[2];                        /* UNO called and not used up yet */
    int exposed;                          /* the player left with one card who did not call (-1 = nobody) */
    int forget_pct;                       /* Sophia forgets to call her own UNO this often (percent) */
    int catch_pct;                        /* Sophia catches your missing UNO this often (percent) */
    /* the last move, for the screen (cleared by uno_ev_clear) */
    int ev_who, ev_kind, ev_card, ev_color, ev_drew, ev_pen, ev_skip, ev_catch, ev_uno, ev_win, ev_kept;
    int first_kind;                       /* what the first card did (see uno_start): 0 none, 1 +2, 2 skip, 3 reverse, 4 wild */
} Uno;

static inline int uno_col(int c) { return c >= 0 && c < 52 ? c / 13 : -1; }
static inline int uno_val(int c) { return c >= 0 && c < 52 ? c % 13 : -1; }
static inline int uno_wild(int c) { return c == UNO_WILD || c == UNO_WILD4; }
static inline int uno_acts(int c) { return uno_wild(c) || (c < 52 && c % 13 >= 10); }
static inline int uno_pts(int c) { return c < 52 ? (c % 13 <= 9 ? c % 13 : 20) : 50; }   /* (not used for scoring in v1) */
/* the picture of a card in assets/uno_cards.png: 0 = back, 1 wild, 2..5 wild coloured, 6 wild +4, 7..10 wild +4 coloured, 11 blank, 12 + 13 * colour + (1..9 -> 0..8, 0 -> 9, +2, skip, reverse -> 10, 11, 12) */
static inline int uno_pic(int c, int chosen) {
    if (c == UNO_WILD) return chosen >= 0 ? 2 + chosen : 1;
    if (c == UNO_WILD4) return chosen >= 0 ? 7 + chosen : 6;
    { int k = c % 13; return 12 + 13 * (c / 13) + (k == 0 ? 9 : k <= 9 ? k - 1 : k); }
}

static inline void uno_shuffle_arr(int *a, int n) { int i; for (i = n - 1; i > 0; i--) { int j = rand() % (i + 1), t = a[i]; a[i] = a[j]; a[j] = t; } }
static inline void uno_fill(Uno *u) {   /* the full 108-card deck, shuffled */
    int c, k, n = 0, i;
    for (c = 0; c < 4; c++) for (k = 0; k <= 12; k++) { u->draw[n++] = c * 13 + k; if (k > 0) u->draw[n++] = c * 13 + k; }
    for (i = 0; i < 4; i++) { u->draw[n++] = UNO_WILD; u->draw[n++] = UNO_WILD4; }
    u->nd = n; uno_shuffle_arr(u->draw, u->nd);
}
static inline int uno_total(const Uno *u) { return u->nd + u->ndisc + u->nh[0] + u->nh[1]; }
static inline void uno_ev_clear(Uno *u) { u->ev_who = -1; u->ev_kind = UE_NONE; u->ev_card = -1; u->ev_color = -1; u->ev_drew = u->ev_pen = u->ev_skip = u->ev_catch = u->ev_uno = u->ev_win = u->ev_kept = 0; }

/* sort one hand: colours in order, then the value; the wilds last */
static inline int uno_key(int c) { return c >= 52 ? 1000 + c : (c / 13) * 20 + (c % 13 >= 1 && c % 13 <= 9 ? c % 13 : c % 13 == 0 ? 0 : 10 + c % 13); }
static inline void uno_sort(Uno *u, int p) {
    int i, j, *h = u->hand[p];
    for (i = 1; i < u->nh[p]; i++) { int v = h[i]; for (j = i - 1; j >= 0 && uno_key(h[j]) > uno_key(v); j--) h[j + 1] = h[j]; h[j + 1] = v; }
}
/* take one card off the draw pile (the discard pile is shuffled back when it is empty); -1 = no card left anywhere */
static inline int uno_pull(Uno *u) {
    if (u->nd == 0 && u->ndisc > 1) {
        int top = u->disc[u->ndisc - 1], i;
        for (i = 0; i < u->ndisc - 1; i++) u->draw[u->nd++] = u->disc[i];
        u->disc[0] = top; u->ndisc = 1; uno_shuffle_arr(u->draw, u->nd);
    }
    return u->nd > 0 ? u->draw[--u->nd] : -1;
}
static inline int uno_give(Uno *u, int p, int n) {   /* p draws n cards; returns how many it really got */
    int got = 0, c;
    while (got < n && u->nh[p] < UNO_MAXH && (c = uno_pull(u)) >= 0) { u->hand[p][u->nh[p]++] = c; got++; }
    uno_sort(u, p); return got;
}

/* the match: 7 cards each, first card flipped (see the rules above). first = who plays first (0 you, 1 Sophia) */
static inline void uno_start(Uno *u, int first) {
    int c;
    memset(u, 0, sizeof *u); u->forget_pct = 8; u->catch_pct = 85; u->exposed = -1; u->drawn = -1; u->winner = -1; uno_ev_clear(u);
    uno_fill(u);
    u->nh[0] = u->nh[1] = 0; uno_give(u, 0, 7); uno_give(u, 1, 7);
    for (;;) {
        c = uno_pull(u);
        if (c == UNO_WILD4) { u->draw[u->nd++] = c; uno_shuffle_arr(u->draw, u->nd); continue; }   /* back into the deck */
        break;
    }
    u->disc[u->ndisc++] = c; u->turn = first; u->color = uno_wild(c) ? -1 : uno_col(c);
    if (c == UNO_WILD) u->first_kind = 4;
    else if (uno_val(c) == UK_PLUS2) { u->first_kind = 1; uno_give(u, first, 2); u->turn = 1 - first; }
    else if (uno_val(c) == UK_SKIP) { u->first_kind = 2; u->turn = 1 - first; }
    else if (uno_val(c) == UK_REV) { u->first_kind = 3; u->turn = 1 - first; }
}
static inline int uno_top(const Uno *u) { return u->disc[u->ndisc - 1]; }

/* may card c be played now? */
static inline int uno_legal_card(const Uno *u, int c) {
    int t = uno_top(u);
    if (uno_wild(c)) return 1;
    if (uno_col(c) == u->color) return 1;
    if (!uno_wild(t) && uno_val(c) == uno_val(t)) return 1;
    return 0;
}
/* may player p play the card at index i of its hand right now? (after a draw only the drawn card) */
static inline int uno_can_play(const Uno *u, int p, int i) {
    if (u->over || p != u->turn || i < 0 || i >= u->nh[p] || u->color < 0) return 0;
    if (u->drawn >= 0 && i != u->drawn) return 0;
    return uno_legal_card(u, u->hand[p][i]);
}
static inline int uno_any_legal(const Uno *u, int p) { int i; for (i = 0; i < u->nh[p]; i++) if (uno_can_play(u, p, i)) return 1; return 0; }

static inline void uno_set_color(Uno *u, int c) { if (c >= 0 && c < 4) u->color = c; }

/* play the card at index i; color = the colour named for a wild. Returns 1 when played, 0 when illegal. */
static inline int uno_play(Uno *u, int p, int i, int color) {
    int c, q = 1 - p, k, n;
    if (!uno_can_play(u, p, i)) return 0;
    if (uno_wild(u->hand[p][i]) && (color < 0 || color > 3)) return 0;
    c = u->hand[p][i];
    for (k = i; k < u->nh[p] - 1; k++) u->hand[p][k] = u->hand[p][k + 1];
    u->nh[p]--; u->disc[u->ndisc++] = c; u->drawn = -1;
    u->color = uno_wild(c) ? color : uno_col(c);
    u->ev_who = p; u->ev_kind = UE_PLAY; u->ev_card = c; u->ev_color = u->color;
    uno_sort(u, p);
    if (u->nh[p] == 0) { u->over = 1; u->winner = p; u->ev_win = 1; u->called[p] = 0; return 1; }
    if (u->nh[p] == 1) { if (u->called[p]) { u->ev_uno = 1; u->called[p] = 0; } else u->exposed = p; }
    else u->called[p] = 0;
    if (u->exposed == q) u->exposed = -1;   /* the opponent moved on: no catching any more */
    u->turn = q;
    if (c == UNO_WILD4) { n = uno_give(u, q, 4); u->ev_pen = n; u->ev_skip = 1; u->turn = p; }
    else if (!uno_wild(c) && uno_val(c) == UK_PLUS2) { n = uno_give(u, q, 2); u->ev_pen = n; u->ev_skip = 1; u->turn = p; }
    else if (!uno_wild(c) && (uno_val(c) == UK_SKIP || uno_val(c) == UK_REV)) { u->ev_skip = 1; u->turn = p; }
    return 1;
}
/* draw one card (the turn's player). Returns 1 = the card may be played (u->drawn is its index at the end of the hand: play it or uno_keep),
   0 = it cannot be played and the turn has passed, -1 = nothing left to draw (the turn passes). */
static inline int uno_draw(Uno *u, int p) {
    int c;
    if (u->over || p != u->turn || u->drawn >= 0 || u->color < 0) return -1;
    u->ev_who = p; u->ev_kind = UE_DRAW; u->called[p] = 0;
    if (u->exposed == 1 - p) u->exposed = -1;   /* (the opponent moved on) */
    c = u->nh[p] < UNO_MAXH ? uno_pull(u) : -1;
    if (c < 0) { u->ev_drew = 0; u->turn = 1 - p; return -1; }
    u->hand[p][u->nh[p]++] = c; u->ev_drew = 1; u->ev_card = c;
    if (uno_legal_card(u, c)) { u->drawn = u->nh[p] - 1; return 1; }
    uno_sort(u, p); u->turn = 1 - p; return 0;
}
/* keep the drawn (playable) card and pass the turn */
static inline void uno_keep(Uno *u, int p) {
    if (u->over || p != u->turn || u->drawn < 0) return;
    u->drawn = -1; u->ev_kept = 1; uno_sort(u, p); u->turn = 1 - p;
}
/* the UNO button of player p: while holding two cards (before playing) it arms the call; holding one card that is exposed it is too late for p itself.
   Catching: the other player pressed it while p is exposed. Returns 1 = armed / caught, 0 = nothing happened. */
static inline int uno_call(Uno *u, int p) {
    if (u->over) return 0;
    if (u->exposed == 1 - p) {   /* catch the opponent: it draws 2 */
        int q = 1 - p; uno_give(u, q, 2); u->exposed = -1; u->ev_catch = 1; u->ev_who = p; return 1;
    }
    if (u->exposed == p) { u->exposed = -1; u->ev_uno = 1; return 1; }   /* still in time before the opponent catches: calling now clears it */
    if (u->nh[p] <= 2 && !u->called[p]) { u->called[p] = 1; return 1; }
    return 0;
}

/* ---------- Sophia (player 1; the tests use the same code for both sides) ---------- */
static inline int uno_count_col(const Uno *u, int p, int col, int skip) {   /* cards of a colour in p's hand, not counting index skip */
    int i, n = 0; for (i = 0; i < u->nh[p]; i++) if (i != skip && !uno_wild(u->hand[p][i]) && uno_col(u->hand[p][i]) == col) n++; return n;
}
static inline int uno_best_color(const Uno *u, int p, int skip) {   /* the colour p holds most of (random among ties; random if none) */
    int c, best = -1, bn = -1, ties = 0;
    for (c = 0; c < 4; c++) { int n = uno_count_col(u, p, c, skip); if (n > bn) { bn = n; best = c; ties = 1; } else if (n == bn && rand() % ++ties == 0) best = c; }
    return bn <= 0 ? rand() % 4 : best;
}
/* how much she likes playing hand card i right now (higher = better) */
static inline int uno_score(const Uno *u, int p, int i) {
    int c = u->hand[p][i], q = 1 - p, s = 0, opp = u->nh[q], mine = u->nh[p];
    if (uno_wild(c)) {
        s = c == UNO_WILD4 ? -6 : -10;   /* the wilds are saved */
        if (mine <= 2 || opp <= 2) s += 14; else if (uno_count_col(u, p, u->color, i) == 0) s += 6;
        if (mine == 1) s += 40;
    } else {
        s = 10;
        if (uno_val(c) >= 10) { s += opp <= 2 ? 12 : opp <= 3 ? 4 : -3; }   /* action cards: used against you when you are nearly out */
        s += uno_count_col(u, p, uno_col(c), i) * 2;   /* the colour she holds most of */
        if (uno_val(c) <= 9) s += uno_val(c) / 3;   /* higher numbers first */
        if (mine == 1) s += 40;
    }
    return s + rand() % 4;   /* a little noise: beatable, not perfect */
}
/* Sophia's (or a test player's) UNO catch at the start of her turn: you stood on one card without calling */
static inline int uno_ai_catch(Uno *u, int p) {
    if (u->over || u->turn != p || u->exposed != 1 - p) return 0;
    if (rand() % 100 < u->catch_pct) return uno_call(u, p);
    u->exposed = -1; return 0;   /* she did not notice: you got away with it */
}
/* a whole turn: she plays the best legal card, or draws one (and plays it when it is playable). Returns 1 when she played. */
static inline int uno_ai_move(Uno *u, int p) {
    int i, best = -1, bs = -9999, col;
    if (u->over || u->turn != p) return 0;
    for (i = 0; i < u->nh[p]; i++) if (uno_can_play(u, p, i)) { int s = uno_score(u, p, i); if (s > bs) { bs = s; best = i; } }
    if (best < 0) {
        int r = uno_draw(u, p);
        if (r != 1) return 0;
        best = u->drawn;
    }
    col = uno_wild(u->hand[p][best]) ? uno_best_color(u, p, best) : -1;
    if (u->nh[p] == 2 && rand() % 100 >= u->forget_pct) u->called[p] = 1;   /* she calls her own UNO (mostly) */
    return uno_play(u, p, best, col);
}
#endif
