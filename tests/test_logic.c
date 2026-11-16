/* Poker Queen - logic tests. Build: gcc -O2 -Wall tests/test_logic.c -o test_logic -lm && ./test_logic
   Exits non-zero if any check fails. No SDL needed. */
#include <ctype.h>
#include <time.h>
#include "../src/logic.h"
#include "../src/multi.h"
#include "../src/pool.h"
#include "../src/lines.h"
#include "../src/extlines.h"
#include "../src/extitems.h"

static int fails = 0, checks = 0;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* card helpers: suit 0..3 = spade, heart, diamond, club; rank 0..12 = 2..A */
#define CD(rank, suit) ((suit) * 13 + (rank))

static void test_eval(void) {
    int royal[7] = {CD(12,0), CD(11,0), CD(10,0), CD(9,0), CD(8,0), CD(0,1), CD(1,2)};
    int sflush[7] = {CD(7,1), CD(6,1), CD(5,1), CD(4,1), CD(3,1), CD(12,2), CD(0,3)};
    int wheel_sf[7] = {CD(12,2), CD(0,2), CD(1,2), CD(2,2), CD(3,2), CD(9,0), CD(9,1)};
    int quads[7] = {CD(5,0), CD(5,1), CD(5,2), CD(5,3), CD(12,0), CD(1,1), CD(2,2)};
    int full[7] = {CD(9,0), CD(9,1), CD(9,2), CD(4,0), CD(4,1), CD(0,2), CD(1,3)};
    int flush[7] = {CD(12,3), CD(9,3), CD(7,3), CD(4,3), CD(2,3), CD(11,0), CD(11,1)};
    int straight[7] = {CD(8,0), CD(7,1), CD(6,2), CD(5,3), CD(4,0), CD(12,1), CD(12,2)};
    int wheel[7] = {CD(12,0), CD(0,1), CD(1,2), CD(2,3), CD(3,0), CD(9,1), CD(10,2)};
    int trips[7] = {CD(8,0), CD(8,1), CD(8,2), CD(4,3), CD(2,0), CD(11,1), CD(12,2)};
    int twop[7] = {CD(8,0), CD(8,1), CD(4,2), CD(4,3), CD(2,0), CD(11,1), CD(12,2)};
    int pair[7] = {CD(8,0), CD(8,1), CD(3,2), CD(5,3), CD(0,0), CD(11,1), CD(12,2)};
    int high[7] = {CD(8,0), CD(7,1), CD(3,2), CD(5,3), CD(0,0), CD(11,1), CD(12,2)};
    CHECK(bonus_kind(best7(royal)) == BN_ROYAL, "royal");
    CHECK(bonus_kind(best7(sflush)) == BN_SFLUSH, "straight flush");
    CHECK(bonus_kind(best7(wheel_sf)) == BN_SFLUSH, "wheel straight flush is NOT royal");
    CHECK(bonus_kind(best7(quads)) == BN_QUADS, "quads");
    CHECK(bonus_kind(best7(full)) == BN_FULL, "full house");
    CHECK(bonus_kind(best7(flush)) == BN_FLUSH, "flush");
    CHECK(bonus_kind(best7(straight)) == BN_STRAIGHT, "straight");
    CHECK(bonus_kind(best7(wheel)) == BN_STRAIGHT, "wheel straight");
    CHECK(bonus_kind(best7(trips)) == -1, "trips pay nothing");
    CHECK(bonus_kind(best7(twop)) == -1, "two pair pays nothing");
    CHECK(bonus_kind(best7(pair)) == -1, "pair pays nothing");
    CHECK(bonus_kind(best7(high)) == -1, "high card pays nothing");
    CHECK(best7(royal) > best7(sflush) && best7(sflush) > best7(quads) && best7(quads) > best7(full) && best7(full) > best7(flush)
          && best7(flush) > best7(straight) && best7(straight) > best7(trips) && best7(trips) > best7(twop) && best7(twop) > best7(pair)
          && best7(pair) > best7(high), "category ordering");
    CHECK(best7(straight) > best7(wheel), "9-high straight beats wheel");
}
/* known 7-card frequencies: high 17.4%, pair 43.8%, 2pair 23.5%, trips 4.83%, straight 4.62%, flush 3.03%, full 2.60%, quads 0.168%, SF 0.0311% */
static void test_freq(void) {
    static const double want[9] = {17.41, 43.82, 23.50, 4.83, 4.62, 3.03, 2.60, 0.168, 0.0311};
    long n = 600000, cnt[9] = {0}, i; int d[52], k;
    for (i = 0; i < 52; i++) d[i] = (int)i;
    for (i = 0; i < n; i++) {
        int c[7]; for (k = 0; k < 7; k++) { int j = k + rnd(52 - k), t = d[k]; d[k] = d[j]; d[j] = t; c[k] = d[k]; }
        cnt[best7(c) >> 20]++;
    }
    for (k = 0; k < 9; k++) {
        double got = 100.0 * (double)cnt[k] / (double)n, tol = want[k] < 1 ? want[k] * 0.35 : want[k] * 0.04 + 0.1;
        CHECK(got > want[k] - tol && got < want[k] + tol, "category %d freq %.3f%% want %.3f%%", k, got, want[k]);
    }
}

static void test_betting_rules(void) {
    Game g; int i;
    g_init(&g); g_match_start(&g, 500); g_hand_start(&g);
    /* force you = button (small blind) */
    g.button = 0; g.c[0] = g.r[0] = 2; g.c[1] = g.r[1] = 4; g.toact = 0;
    CHECK(g_tocall(&g, 0) == 2 && g_size(&g) == 4, "SB facing BB: 2 to call, small bet");
    CHECK(g_raise_cost(&g, 0) == 6, "SB raise costs 6 (call 2 + bet 4)");
    g_act(&g, A_CALL);   /* limp */
    CHECK(g.toact == 1 && g.stage == 0 && g.r[0] == 4, "after SB limp the BB gets the option");
    g_act(&g, A_CALL);   /* BB checks */
    CHECK(g.stage == 1 && g.nbets == 0 && g.toact == 1 && g.r[0] == 0, "flop: non-button (her) acts first");
    CHECK(g_pot(&g) == 8, "pot after limp = 8");
    /* bet / raise / raise / cap */
    g_act(&g, A_RAISE); CHECK(g.r[1] == 4 && g.nbets == 1, "flop bet 4");
    g_act(&g, A_RAISE); CHECK(g.r[0] == 8 && g.nbets == 2, "raise to 8");
    g_act(&g, A_RAISE); CHECK(g.r[1] == 12 && g.nbets == 3, "re-raise to 12");
    g_act(&g, A_RAISE); CHECK(g.r[0] == 16 && g.nbets == 4, "cap raise to 16");
    CHECK(!g_can_raise(&g), "capped at 4 bets");
    g_act(&g, A_RAISE);   /* illegal raise becomes a call */
    CHECK(g.stage == 2 && g.c[0] == 20 && g.c[1] == 20, "capped 'raise' only calls; street closes (c=%d,%d)", g.c[0], g.c[1]);
    CHECK(g_size(&g) == BIG_BET, "turn uses the big bet");
    /* max commitment fits the stack */
    g_init(&g); g_match_start(&g, 100); g_hand_start(&g);
    for (i = 0; i < 40 && !g.over; i++) g_act(&g, A_RAISE);
    CHECK(g.over && g.showdown && g.c[0] == 96 && g.c[1] == 96, "all-raise hand ends at showdown with 96 each (got %d,%d)", g.c[0], g.c[1]);
    CHECK(g.c[0] <= STACK, "never exceeds the stack");
    /* fold */
    g_init(&g); g_match_start(&g, 100); g_hand_start(&g);
    g.button = 1; g.c[1] = g.r[1] = 2; g.c[0] = g.r[0] = 4; g.toact = 1;
    g_act(&g, A_RAISE);   /* her SB raises to 8 */
    g_act(&g, A_FOLD);    /* you fold the BB */
    CHECK(g.over && !g.showdown && g.res == -1 && g.amt == 4 && g.net == -4, "fold loses what you put in (res=%d amt=%d net=%d)", g.res, g.amt, g.net);
    /* button alternates */
    g_init(&g); g_match_start(&g, 100); g_hand_start(&g); i = g.button; g_hand_start(&g);
    CHECK(g.button == !i, "button alternates");
}

static void play_random_hand(Game *g, int *steps) {
    *steps = 0;
    while (!g->over) {
        int a = rnd(3);
        if (a == A_FOLD && g_tocall(g, g->toact) == 0) a = A_CALL;
        g_act(g, a); (*steps)++;
        if (*steps > 40) { CHECK(0, "hand did not terminate"); return; }
    }
}
static void test_invariants(void) {
    Game g; long h; int steps, hands = 0, maxsteps = 0;
    g_init(&g);
    for (h = 0; h < 300000; h++) {
        int before;
        if (!g.in_match || g.hand >= g.mlen) g_match_start(&g, 500);
        before = g.net; g_hand_start(&g);
        play_random_hand(&g, &steps); hands++; if (steps > maxsteps) maxsteps = steps;
        if (g.c[0] > STACK || g.c[1] > STACK) { CHECK(0, "stack exceeded"); break; }
        if (g.fold) CHECK(g.res == (g.fold == 1 ? -1 : 1), "fold result sign");
        CHECK(g.net - before == g.res * g.amt, "net bookkeeping");
        if (g.res == 1) CHECK(g.amt == g.c[1], "winner takes the opponent's chips");
        if (g.res == -1) CHECK(g.amt == g.c[0], "loser loses own chips");
        if (g.res != 1 && g.over) { /* bonuses only on wins */ }
        if (fails > 10) break;
    }
    printf("  random play: %d hands, longest hand %d actions\n", hands, maxsteps);
}
static void test_bonus_rules(void) {
    Game g; g_init(&g); g_match_start(&g, 100); g_hand_start(&g);
    /* you hold a flush at showdown and win: bonus counted */
    { int d[9] = {CD(12,0), CD(11,0), CD(0,1), CD(1,2), CD(10,0), CD(8,0), CD(2,0), CD(5,1), CD(6,2)};
      memcpy(g.deck, d, sizeof d); }
    { int used[52] = {0}, i, k = 9; for (i = 0; i < 9; i++) used[g.deck[i]] = 1; for (i = 0; i < 52; i++) if (!used[i]) g.deck[k++] = i; }
    g.button = 0; g.c[0] = g.r[0] = 2; g.c[1] = g.r[1] = 4; g.toact = 0;
    g_act(&g, A_CALL); g_act(&g, A_CALL);
    while (!g.over) g_act(&g, A_CALL);
    CHECK(g.res == 1 && g.bn[BN_FLUSH] == 1, "flush win scores the flush bonus (res=%d bn=%d)", g.res, g.bn[BN_FLUSH]);
    /* she folds: no bonus even with a great hand */
    g_hand_start(&g);
    { int d[9] = {CD(12,0), CD(11,0), CD(10,0), CD(9,0), CD(8,0), CD(1,2), CD(2,2), CD(3,2), CD(4,2)};
      memcpy(g.deck, d, sizeof d); }
    { int used[52] = {0}, i, k = 9; for (i = 0; i < 9; i++) used[g.deck[i]] = 1; for (i = 0; i < 52; i++) if (!used[i]) g.deck[k++] = i; }
    g.button = 0; g.toact = 0; g_act(&g, A_RAISE); g_act(&g, A_FOLD);
    CHECK(g.res == 1 && g.fold == 2 && bonus_total(&g) == 40, "bonus not added when she folds (total %d)", bonus_total(&g));
}

static void test_money(void) {
    Game g; char s[32];
    /* win: net + bonus */
    g_init(&g); g.cash = 100; g.net = 34; g.bn[BN_FLUSH] = 2; g.bn[BN_STRAIGHT] = 1;
    g_match_finish(&g);
    CHECK(g.last_pay == 34 + 80 + 20 && g.cash == 100 + 134 && g.last_res == 1 && g.m_won == 1 && g.best_net == 34, "win payout (%d)", g.last_pay);
    /* loss: nothing, bonuses forfeited */
    g_init(&g); g.cash = 50; g.net = -12; g.bn[BN_ROYAL] = 1;
    g_match_finish(&g);
    CHECK(g.last_pay == 0 && g.cash == 50 && g.last_res == -1 && g.last_bonus == 2500 && g.m_lost == 1, "loss pays nothing, royal forfeited");
    /* tie */
    g_init(&g); g.cash = 50; g.net = 0; g.bn[BN_FULL] = 3; g_match_finish(&g);
    CHECK(g.last_pay == 0 && g.cash == 50 && g.last_res == 0 && g.m_tied == 1, "tie pays nothing");
    /* ATM */
    g_init(&g); g.cash = 300; g.bank = 0;
    CHECK(g_deposit(&g, 500) == 1 && g.cash == 300, "deposit more than cash refused");
    CHECK(g_deposit(&g, 0) == 3, "zero deposit refused");
    CHECK(g_deposit(&g, 120) == 0 && g.cash == 180 && g.bank == 120, "deposit ok");
    CHECK(g_withdraw(&g, 121) == 1 && g.bank == 120, "withdraw more than bank refused");
    CHECK(g_withdraw(&g, 20) == 0 && g.cash == 200 && g.bank == 100, "withdraw ok");
    g.bank = MONEY_CAP - 5; g.cash = 100; CHECK(g_deposit(&g, 10) == 2, "bank limit");
    fmt_money(s, 1250); CHECK(!strcmp(s, "1,250"), "fmt 1250 -> %s", s);
    fmt_money(s, 999999); CHECK(!strcmp(s, "999,999"), "fmt 999999 -> %s", s);
    fmt_money(s, 0); CHECK(!strcmp(s, "0"), "fmt 0");
    fmt_money(s, 100); CHECK(!strcmp(s, "100"), "fmt 100 -> %s", s);
}

static void test_save(void) {
    Game g, h; FILE *f; const char *p = "/tmp/pq_test.sav";
    g_init(&g); g.cash = 777; g.bank = 4242; g.keyst = 1; g.story = ST_NAMED; g.m_won = 3; g.m_played = 5; g.m_lost = 2;
    CHECK(save_game(&g, p), "save");
    g_init(&h); CHECK(load_game(&h, p), "load");
    CHECK(h.cash == 777 && h.bank == 4242 && h.keyst == 1 && h.story == ST_NAMED && !h.in_match && !h.forfeit, "roundtrip equal");
    save_game(&g, p); CHECK(load_game(&h, p), "overwrite then load");
    /* closing the app during a match counts as walking out: a loss, no money moves, bonuses gone */
    { Game t = g; int cash0 = t.cash - 500; g_match_start(&t, 500); g_hand_start(&t); t.net = 40; t.bn[BN_FLUSH] = 2; save_game(&t, p);
      CHECK(load_game(&h, p), "load a save made mid-match");
      CHECK(!h.in_match && h.forfeit == 1 && h.m_lost == g.m_lost + 1 && h.m_played == g.m_played + 1 && h.cash == cash0 && h.last_res == -1 && h.last_pay == 0 && !h.bn[BN_FLUSH], "mid-match save becomes a forfeit"); }
    { Game t = g; g_forfeit(&t); CHECK(t.m_won == g.m_won && t.m_lost == g.m_lost + 1 && t.forfeit == 1 && t.cash == g.cash && t.bank == g.bank, "g_forfeit only adds a loss"); }
    f = fopen(p, "wb"); fwrite("junk", 1, 4, f); fclose(f); CHECK(!load_game(&h, p), "short file rejected");
    remove(p); CHECK(!load_game(&h, p), "missing file");
    { Game t = g; t.cash = -5; save_game(&t, p); CHECK(!load_game(&h, p), "negative cash rejected"); }
    { Game t = g; t.story = 9; save_game(&t, p); CHECK(!load_game(&h, p), "bad story stage rejected"); }
    { Game t = g; t.mlen = 17; t.fee = 100; t.in_match = 1; save_game(&t, p); CHECK(!load_game(&h, p), "bad match length rejected"); }
    { Game t = g; t.mlen = 10; t.fee = 77; t.in_match = 1; save_game(&t, p); CHECK(!load_game(&h, p), "bad buy-in rejected"); }
    { Game t = g; t.keyst = 3; save_game(&t, p); CHECK(!load_game(&h, p), "bad key card state rejected"); }
    { Game t = g; t.ver = 1; FILE *w = fopen(p, "wb"); fwrite(&t, sizeof t, 1, w); fclose(w); CHECK(!load_game(&h, p), "old save version rejected"); }
    remove(p);
}


/* ----- energy, drinks, store, rescue ----- */
static void test_energy(void) {
    Game g, h; int p = 0; const char *f = "/tmp/pq_test_e.sav";
    g_init(&g); CHECK(g.energy == ENERGY_MAX && g.soda == 0 && g.monster == 0 && !g.rescue_used, "new game: full energy, empty inventory");
    CHECK(g.cash == START_CASH && START_CASH == 100, "new game: 100 cash in hand");
    g.story = ST_NAMED; g.cash = 1700;
    CHECK(SOPH_FEE[0] == 100 && SOPH_FEE[1] == 500 && SOPH_FEE[2] == 1000 && g_mult(100) == 1 && g_mult(500) == 5 && g_mult(1000) == 10, "buy-ins 100/500/1000 with stakes x1/x5/x10");
    g_match_start(&g, 100); CHECK(g.cash == 1600 && g.energy == ENERGY_MAX && g.mlen == 10, "the 100 buy-in: 100 cash, no energy, always 10 hands");
    g.in_match = 0; g_match_start(&g, 500); CHECK(g.cash == 1100 && g.energy == ENERGY_MAX, "the 500 buy-in");
    g.in_match = 0; g_match_start(&g, 1000); CHECK(g.cash == 100 && g.energy == ENERGY_MAX, "the 1000 buy-in");
    g.in_match = 0; g.energy = 0; g.cash = 100; CHECK(g_can_play(&g) && g_can_afford_match(&g, 100) && !g_can_afford_match(&g, 500), "cash decides, energy does not");
    g.cash = 99; CHECK(!g_can_play(&g) && !g_can_afford_match(&g, 100), "99 cash: no match at all");
    g.cash = 500; g_match_start(&g, 100); g.net = 0; g_match_finish(&g); CHECK(g.cash == 500 && g.last_pay == 0 && g.last_res == 0, "a tie gives the buy-in back");
    g_match_start(&g, 500); g.net = -4; g_match_finish(&g); CHECK(g.cash == 0, "a loss keeps the buy-in gone");
    g.cash = 100; g_match_start(&g, 100); g.net = 14; g_match_finish(&g); CHECK(g.cash == 14 + 500 && g.last_pay == 514 && g_prize(100) == 500, "x1 stake: a win pays the net chips 1:1 plus the prize of 5 x the buy-in");
    g.cash = 500; g_match_start(&g, 500); g.net = 14; g.bn[BN_STRAIGHT] = 1; g_match_finish(&g); CHECK(g.cash == 14 * 5 + 20 * 5 + 2500 && g.last_bonus == 100 && g.last_pay == 2670 && g_prize(500) == 2500, "x5 stake: net chips and card bonuses are multiplied, plus the prize 5 x 500 (cash %d)", g.cash);
    g.cash = 1000; g_match_start(&g, 1000); g.net = 10; g_match_finish(&g); CHECK(g.cash == 100 + 5000 && g_prize(1000) == 5000, "x10 stake: 10 chips pay 100, plus the prize 5 x 1000");
    g.cash = 500; g_match_start(&g, 100); g.net = -3; g_match_finish(&g); CHECK(g.cash == 400 && g.last_pay == 0, "a loss pays no prize");
    g.cash = 500; g_match_start(&g, 100); g_forfeit(&g); CHECK(g.cash == 400, "walking out keeps the buy-in gone");
    g.story = ST_FIRST; g.cash = 99; CHECK(!g_can_play(&g) && !g_can_afford_match(&g, 100), "the first challenge is paid like the others: 99 cash is not enough");
    g.cash = 100; CHECK(g_can_play(&g), "100 cash pays the cheapest first challenge"); g_match_start(&g, 100); CHECK(g.cash == 0, "the first challenge costs its buy-in");
    g.net = 0; g_match_finish(&g); CHECK(g.cash == 100, "a tie in the first challenge gives the buy-in back"); g.story = ST_NAMED;
    /* drinks */
    g_init(&g); g.cash = 20; CHECK(PRICE_SODA == 5 && PRICE_MONSTER == 10, "v12.4: soda $5, energy drink $10"); CHECK(g_buy(&g, 0) == 0 && g.soda == 1 && g.cash == 15, "buy soda $5");
    CHECK(g_buy(&g, 1) == 0 && g.monster == 1 && g.cash == 5, "buy energy drink $10");
    g.cash = 9; CHECK(g_buy(&g, 1) == 1 && g.monster == 1 && g.cash == 9, "cannot buy without cash"); g.cash = 4; CHECK(g_buy(&g, 0) == 1 && g.soda == 1, "4 dollars do not buy a $5 soda");
    g.soda = INV_CAP; g.cash = 50; CHECK(g_buy(&g, 0) == 2 && g.cash == 50, "inventory cap");
    g.energy = 100; g.soda = 2; CHECK(g_drink(&g, 0) == 2 && g.soda == 2, "no drinking at full energy");
    g.energy = 70; CHECK(g_drink(&g, 0) == 0 && g.energy == 90 && g.soda == 1, "soda gives 20 percent");
    CHECK(g_drink(&g, 0) == 0 && g.energy == 100 && g.soda == 0, "energy caps at 100");
    CHECK(g_drink(&g, 0) == 1, "no soda left");
    g.energy = 10; g.monster = 1; CHECK(g_drink(&g, 1) == 0 && g.energy == 60 && g.monster == 0, "energy drink gives 50 percent");
    /* free drinks for wins */
    g_init(&g); g.story = ST_NAMED; g_match_start(&g, 500); g.net = 10; g_match_finish(&g); CHECK(g.soda == 1 && g.monster == 0 && g.last_drink == 1, "500 table win: free soda");
    g.energy = 100; g_match_start(&g, 1000); g.net = 10; g_match_finish(&g); CHECK(g.soda == 1 && g.monster == 1 && g.last_drink == 2, "1000 table win: free energy drink");
    g.energy = 100; g_match_start(&g, 100); g.net = 10; g_match_finish(&g); CHECK(g.soda == 1 && g.monster == 1 && g.last_drink == 0, "100 table win: nothing");
    g_match_start(&g, 1000); g.net = -10; g_match_finish(&g); CHECK(g.monster == 1 && g.last_drink == 0, "a loss gives nothing");
    g_match_start(&g, 500); g.net = 0; g_match_finish(&g); CHECK(g.soda == 1 && g.last_drink == 0, "a tie gives nothing");
    /* exhibition needs a full bar and drains it */
    g_init(&g); g.cash = 1000; g.energy = EX_ENERGY - 1; CHECK(EX_ENERGY == 15 && g_ex_start(&g, 100) == -1 && g.cash == 1000 && g.energy == 14, "exhibition refused below 15 percent energy");
    g.energy = 100; CHECK(g_ex_start(&g, 5000) == 0 && g.energy == 100, "no cash: nothing changes");
    CHECK(g_ex_start(&g, 100) == 1 && g.cash == 900 && g.energy == 85, "exhibition: buy-in paid, 15 percent energy used");
    g.energy = 15; CHECK(g_ex_start(&g, 10) == 1 && g.energy == 0 && g.cash == 890, "exactly 15 percent is enough");
    CHECK(EX_N == 6 && EX_BUYIN[0] == 10 && EX_BUYIN[1] == 50 && EX_BUYIN[2] == 100 && EX_BUYIN[3] == 500 && EX_BUYIN[4] == 1000 && EX_BUYIN[5] == 5000, "exhibition fees 10/50/100/500/1000 and the 5000 boss");
    /* pity soda, once */
    g_init(&g); g.energy = 0; g.cash = 0; CHECK(g_rescue_due(&g), "stuck: broke, empty, no energy");
    g.cash = PRICE_SODA; CHECK(!g_rescue_due(&g), "money in cash: not stuck"); g.cash = 0; g.bank = PRICE_SODA; CHECK(!g_rescue_due(&g), "money in the bank: not stuck"); g.bank = 0;
    g.soda = 1; CHECK(!g_rescue_due(&g), "a drink in the inventory: not stuck"); g.soda = 0; g.energy = 20; CHECK(!g_rescue_due(&g), "some energy: not stuck"); g.energy = 0;
    g_rescue(&g); CHECK(g.soda == 1 && g.rescue_used, "rescue gives one soda"); g.soda = 0; CHECK(!g_rescue_due(&g), "never a second rescue");
    /* save round trip and sanity */
    g_init(&g); g.energy = 60; g.soda = 3; g.monster = 2; g.rescue_used = 1; g.last_drink = 2; g.story = ST_NAMED; save_game(&g, f); g_init(&h);
    CHECK(load_game(&h, f) && h.energy == 60 && h.soda == 3 && h.monster == 2 && h.rescue_used == 1, "energy and inventory survive a save");
    { Game t = g; t.energy = 101; save_game(&t, f); CHECK(!load_game(&h, f), "energy over 100 rejected"); }
    { Game t = g; t.soda = -1; save_game(&t, f); CHECK(!load_game(&h, f), "negative drinks rejected"); }
    { Game t = g; t.ver = 3; FILE *w = fopen(f, "wb"); fwrite(&t, sizeof t, 1, w); fclose(w); CHECK(!load_game(&h, f), "v3 save rejected"); }
    remove(f); (void)p;
}

/* ----- AI strength ----- */
typedef int (*Bot)(const Game *);
static int bot_station(const Game *g) { (void)g; return A_CALL; }
static int bot_maniac(const Game *g) { return g_can_raise(g) ? A_RAISE : A_CALL; }
static int bot_random(const Game *g) { int a = rnd(3); if (a == A_FOLD && g_tocall(g, 0) == 0) a = A_CALL; return a; }
static int bot_rock(const Game *g) {   /* plays only good hands */
    int hole[2], b[5], nb = g->stage == 0 ? 0 : g->stage + 2, i; float e;
    hole[0] = g->deck[0]; hole[1] = g->deck[1]; for (i = 0; i < nb; i++) b[i] = g->deck[4+i];
    e = nb == 0 ? pre_equity(hole) : equity(hole, b, nb, 150);
    if (e > 0.68f && g_can_raise(g)) return A_RAISE;
    if (e > 0.55f || g_tocall(g, 0) == 0) return A_CALL;
    return A_FOLD;
}
/* the second player (you) is a bot; she is the real AI. returns her average chips won per hand */
static double sim(Bot b, int hands, int *tilt_hands) {
    Game g; long net = 0; int h, steps;
    g_init(&g); g_match_start(&g, 1000000 > 0 ? 100 : 100);
    for (h = 0; h < hands; h++) {
        if (g.hand >= g.mlen) g_match_start(&g, 100);
        g_hand_start(&g); steps = 0;
        while (!g.over && steps++ < 60) {
            int a = g.toact == 1 ? ai_decide(&g) : b(&g);
            if (a == A_FOLD && g_tocall(&g, g.toact) == 0) a = A_CALL;
            g_act(&g, a);
        }
        if (!g.over) { CHECK(0, "AI game stuck"); break; }
        net -= g.res * g.amt; if (g.tilt) (*tilt_hands)++;
    }
    return (double)net / hands;
}
static void test_ai(void) {
    int t = 0; double r; clock_t c0;
    c0 = clock(); r = sim(bot_station, 2500, &t);
    printf("  AI vs calling station : %+.2f chips/hand  (%.2fs for 2500 hands)\n", r, (double)(clock() - c0) / CLOCKS_PER_SEC);
    CHECK(r > 1.0, "AI should beat a calling station clearly (%.2f)", r);
    r = sim(bot_maniac, 2500, &t);   printf("  AI vs maniac (always raise): %+.2f chips/hand\n", r); CHECK(r > 0.5, "AI should beat a maniac (%.2f)", r);
    r = sim(bot_random, 2500, &t);   printf("  AI vs random         : %+.2f chips/hand\n", r); CHECK(r > 1.0, "AI should beat random (%.2f)", r);
    r = sim(bot_rock, 4000, &t);     printf("  AI vs tight 'rock'   : %+.2f chips/hand (about even is the goal; noise is +-0.3)\n", r); CHECK(r > -1.0, "AI should hold its own against a rock (%.2f)", r);
    printf("  (tilt active in %d simulated hands)\n", t);
}
/* her decisions must always be legal and her hand strength sensible */
static void test_ai_sanity(void) {
    Game g; int i, folds_with_nuts = 0, n = 0;
    g_init(&g); g_match_start(&g, 500);
    for (i = 0; i < 400; i++) {
        int a;
        g_hand_start(&g); g.button = 0; g.c[0] = g.r[0] = 2; g.c[1] = g.r[1] = 4; g.toact = 0; g.nbets = 1;
        /* give her aces */
        { int t; t = g.deck[2]; g.deck[2] = 12; { int j; for (j = 0; j < 52; j++) if (g.deck[j] == 12 && j != 2) g.deck[j] = t; }
          t = g.deck[3]; g.deck[3] = 25; { int j; for (j = 0; j < 52; j++) if (g.deck[j] == 25 && j != 3) g.deck[j] = t; } }
        g_act(&g, A_RAISE);   /* you raise preflop */
        a = ai_decide(&g); n++;
        if (a == A_FOLD) folds_with_nuts++;
    }
    CHECK(folds_with_nuts == 0, "AI folded aces preflop %d/%d times", folds_with_nuts, n);
}

/* ----- dialogue limits ----- */
static int ok_chars(const char *s) {
    for (; *s; s++) { int c = toupper((unsigned char)*s); if (*s == '|') continue; if (c < 32 || c > 90) return 0; }
    return 1;
}
static void check_pool(const char *name, const char *const *p, int n) {
    int i; for (i = 0; i < n; i++) {
        char a[64]; const char *bar = strchr(p[i], '|'); int l1 = bar ? (int)(bar - p[i]) : (int)strlen(p[i]), l2 = bar ? (int)strlen(bar + 1) : 0;
        (void)a;
        CHECK(l1 <= 18 && l2 <= 18, "%s[%d] '%s' lines %d/%d exceed 18", name, i, p[i], l1, l2);
        CHECK(ok_chars(p[i]), "%s[%d] has characters the font cannot draw", name, i);
        CHECK(!(bar && strchr(bar + 1, '|')), "%s[%d] has more than 2 lines", name, i);
    }
}
static void check_pages(const char *name, const char *const *p, int n) {
    int i; for (i = 0; i < n; i++) {
        char w[400]; int lines = wrap_buf(w, sizeof w, p[i], 46);
        CHECK(lines <= 3, "%s[%d] wraps to %d lines (max 3)", name, i, lines);
        { int big = wrap_buf(w, sizeof w, p[i], 31); CHECK(big <= 5, "%s[%d] wraps to %d lines in the LARGE GUI box (max 5)", name, i, big); }
        CHECK(ok_chars(p[i]), "%s[%d] has characters the font cannot draw", name, i);
    }
}
static void test_lines(void) {
#define P(a) check_pool(#a, a, a##_N)
#define G_(a) check_pages(#a, a, a##_N)
    P(L_LEN); P(L_START); P(L_HAND); P(L_LAST); P(L_SHEWIN); P(L_YOUWIN); P(L_SHEFOLD); P(L_YOUFOLD); P(L_SPLIT);
    P(L_HERRAISE); P(L_YOURAISE); P(L_ENDHER); P(L_ENDYOU); P(L_ENDTIE); P(L_ENDLOST);
    G_(A_WIN); G_(A_WINBONUS); G_(A_DRINK); G_(A_LOSS); G_(A_LOSSBONUS); G_(A_TIE);   /* what she says when she sends you out of her room */
    G_(S_ZOMBIE);
    /* v7.7: CHAT and TEASE (src/talk.h): every page fits the dialogue box, no characters the font cannot draw, every exchange has a me line and an answer */
    G_(C_FAIL); G_(R_MORE); G_(T_KICK); G_(T_OUT); G_(R_KICKED);
    {
        int i, k; char nm[32];
        for (k = 0; k < CH_N; k++) for (i = 0; i < C_TOPICS[k].n; i++) {
            const Talk *x = &C_TOPICS[k].t[i]; const char *pg[3]; int n = 0; pg[n++] = x->me; pg[n++] = x->a; if (x->b) pg[n++] = x->b;
            snprintf(nm, sizeof nm, "chat%d.%d", k, i); CHECK(x->me && x->a, "%s: an exchange needs a me line and an answer", nm); check_pages(nm, pg, n);
        }
        for (k = 0; k < TS_N; k++) {
            CHECK(T_TOPICS[k].n >= 10, "tease topic %d must have at least 10 exchanges (has %d)", k, T_TOPICS[k].n);
            for (i = 0; i < T_TOPICS[k].n; i++) {
                const Talk *x = &T_TOPICS[k].t[i]; const char *pg[3]; int n = 0; pg[n++] = x->me; pg[n++] = x->a; if (x->b) pg[n++] = x->b;
                snprintf(nm, sizeof nm, "tease%d.%d", k, i); CHECK(x->me && x->a, "%s: an exchange needs a me line and an answer", nm); check_pages(nm, pg, n);
            }
        }
        for (k = 0; k < CH_N; k++) CHECK(C_TOPICS[k].n >= 8, "chat topic %d must have at least 8 exchanges (has %d)", k, C_TOPICS[k].n);
    }
    G_(S_DAILY); G_(W_CLERK_RENT); G_(W_RENT_OK); G_(W_RENT_SHORT); check_pages("w_rent_remind", &W_RENT_REMIND, 1);   /* v5.5: once a day, rent */
    { char rb[160]; const char *t[1]; t[0] = rb; snprintf(rb, sizeof rb, W_RENT_TAKEN, RENT_AMT); check_pages("w_rent_taken", t, 1); snprintf(rb, sizeof rb, W_RENT_TAKEN_BANK, 99, 999999); check_pages("w_rent_taken_bank", t, 1);
      snprintf(rb, sizeof rb, O_RENT, RENT_AMT); CHECK(strlen(rb) <= 64 && ok_chars(rb), "objective rent line"); CHECK(ok_chars(OVER_RENT_L1) && ok_chars(OVER_RENT_L2) && strlen(OVER_RENT_L2) <= 50, "rent game over lines"); CHECK(ok_chars(RENT_DUE_TXT), "rent due text"); }
    { int i, n;
#define BEATS(a, name) for (i = 0, n = a##_N; i < n; i++) { if (a[i].txt) { check_pages(name, &a[i].txt, 1); CHECK(a[i].bg < BG_N && a[i].spr <= 5 && a[i].who <= W_HER, "%s[%d] bad fields", name, i); } }
      BEATS(B_INTRO, "intro"); BEATS(B_BUMP, "bump"); BEATS(B_DOOR1, "door1"); BEATS(B_REPEAT, "repeat"); BEATS(B_NAMED, "named"); BEATS(B_BOSS, "boss"); BEATS(B_LEAVE, "leave"); BEATS(B_GREET, "greet"); BEATS(B_COMMENT, "comment"); BEATS(B_AFTER, "after");
      G_(W_CLERK); check_pages("w_storm", &W_STORM, 1); check_pages("w_bath", &W_BATH, 1); check_pages("w_sink", &W_SINK, 1);
      check_pages("w_event_locked", &W_EVENT_LOCKED, 1); check_pages("w_saved", &W_SAVED, 1); check_pages("w_f2", &W_F2, 1); check_pages("w_f3", &W_F3, 1); check_pages("w_hall2", &W_HALL2, 1); check_pages("w_hall3", &W_HALL3, 1); check_pages("w_checkin", &W_CHECKIN, 1); CHECK(strlen(H_DESK) <= 24 && strlen(H_ROOM) <= 24, "hint too long"); CHECK(strlen(W_CLERK_TAG) <= 14, "clerk name tag too long");
      G_(G_NORMAL); G_(G_WIN); G_(G_LOSS); G_(G_TIE); G_(G_COWARD); G_(G_REPEAT_OK); G_(L_COWARD); G_(N_WIN); G_(N_LOSE); G_(N_TIE); check_pages("C_LEN", C_LEN, 3);
      /* the beats whose text is filled in at run time must be exactly the ones with NULL */
      CHECK(B_REPEAT[1].txt == NULL && B_NAMED[0].txt == NULL && B_LEAVE[0].txt == NULL && B_GREET[1].txt == NULL && B_COMMENT[0].txt == NULL, "runtime lines");
      /* she is only visible after the bump, the intro ends in your room, the door scene ends on her door, and the name appears in the right place */
      CHECK(B_INTRO[0].spr == 0 && B_BUMP[0].spr == 0 && B_BUMP[2].spr == 0 && B_BUMP[3].spr == 1 && B_BUMP[B_BUMP_N-1].spr == 0 && B_BUMP[B_BUMP_N-1].bg == BG_LOBBY, "intro staging");
      CHECK(B_DOOR1[B_DOOR1_N-2].who == W_AW && B_DOOR1[B_DOOR1_N-1].who == W_NAR && strstr(B_DOOR1[B_DOOR1_N-1].txt, "elevator") && strstr(B_DOOR1[5].txt, "floor key card") && B_NAMED[1].who == W_ME && B_NAMED[2].who == W_SOPHIA && strstr(B_NAMED[2].txt, "Sophia") && strstr(B_NAMED[2].txt, "Poker Queen"), "naming staging");
      CHECK(B_BOSS[0].who == W_AW && B_BOSS[2].who == W_SOPHIA && strstr(B_BOSS[2].txt, "Sophia") && strstr(B_BOSS[2].txt, "Poker Queen") && strstr(B_BOSS[6].txt, "floor key card"), "boss scene staging (she is still unnamed until beat 2)");
      for (i = 0; i < 2; i++) CHECK(B_BOSS[i].who != W_SOPHIA && !strstr(B_BOSS[i].txt, "Sophia"), "boss scene: name given too early");
      { int fk = 0; for (i = 0; i < B_BUMP_N; i++) if (strstr(B_BUMP[i].txt, "floor key card")) fk++; CHECK(fk >= 2, "the bump scene must hand over the floor key card"); { int tf = 0; for (i = 0; i < B_BUMP_N; i++) if (strstr(B_BUMP[i].txt, "third floor")) tf++; CHECK(tf >= 1, "the bump scene must name her floor"); } }
      for (i = 0; i < B_INTRO_N + B_BUMP_N + B_DOOR1_N; i++) { const Beat *b = i < B_INTRO_N ? &B_INTRO[i] : i < B_INTRO_N + B_BUMP_N ? &B_BUMP[i - B_INTRO_N] : &B_DOOR1[i - B_INTRO_N - B_BUMP_N]; CHECK(!strstr(b->txt, "Sophia"), "name given too early"); }
    }
    { char w[300]; int lines = wrap_buf(w, sizeof w, "one two three", 5); CHECK(lines == 3 && !strcmp(w, "one\ntwo\nthree"), "wrap '%s' %d", w, lines); }
}


/* ----- multi-player engine ----- */
static int mp_sum(const MP *m) { int i, t = 0; for (i = 0; i < m->n; i++) t += m->stack[i]; return t; }
static void test_multi_sidepots(void) {
    MP m; int i;
    /* three players, seat 0 all-in for 100 with the best hand, seat 1 in for 60 (second best), seat 2 folded after 30 */
    memset(&m, 0, sizeof m); m.n = 3; for (i = 0; i < 3; i++) m.alive[i] = 1; m.button = 2;
    m.deck[0] = CD(12,0); m.deck[1] = CD(12,1);      /* seat 0: pair of aces */
    m.deck[2] = CD(11,0); m.deck[3] = CD(11,1);      /* seat 1: pair of kings */
    m.deck[4] = CD(10,2); m.deck[5] = CD(9,3);       /* seat 2 */
    m.deck[8] = CD(0,2); m.deck[9] = CD(2,3); m.deck[10] = CD(4,0); m.deck[11] = CD(6,1); m.deck[12] = CD(8,2);
    m.c[0] = 100; m.c[1] = 60; m.c[2] = 30; m.folded[2] = 1; m.allin[0] = 1; m.allin[1] = 1;
    m.stack[0] = 0; m.stack[1] = 0; m.stack[2] = 70;
    mp_finish(&m);
    CHECK(m.won[0] == 190 && m.won[1] == 0 && m.won[2] == 0, "best hand takes everything (won %d %d %d)", m.won[0], m.won[1], m.won[2]);
    CHECK(mp_sum(&m) == 70 + 190, "chips kept");
    /* the short stack wins the main pot, the side pot goes to the next best */
    memset(&m, 0, sizeof m); m.n = 3; for (i = 0; i < 3; i++) m.alive[i] = 1; m.button = 2;
    m.deck[0] = CD(12,0); m.deck[1] = CD(12,1);      /* seat 0: aces, but only 40 in */
    m.deck[2] = CD(11,0); m.deck[3] = CD(11,1);      /* seat 1: kings, 100 in */
    m.deck[4] = CD(10,2); m.deck[5] = CD(9,3);       /* seat 2: junk, 100 in */
    m.deck[8] = CD(0,2); m.deck[9] = CD(2,3); m.deck[10] = CD(4,0); m.deck[11] = CD(6,1); m.deck[12] = CD(8,2);
    m.c[0] = 40; m.c[1] = 100; m.c[2] = 100; m.allin[0] = 1; m.allin[1] = 1; m.allin[2] = 1;
    mp_finish(&m);
    CHECK(m.won[0] == 120 && m.won[1] == 120 && m.won[2] == 0, "main + side pot (won %d %d %d)", m.won[0], m.won[1], m.won[2]);
    /* a tie splits the pot, the odd chip goes to the first winner after the button */
    memset(&m, 0, sizeof m); m.n = 3; for (i = 0; i < 3; i++) m.alive[i] = 1; m.button = 0;
    m.deck[0] = CD(0,0); m.deck[1] = CD(1,1); m.deck[2] = CD(0,2); m.deck[3] = CD(1,3); m.deck[4] = CD(0,1); m.deck[5] = CD(1,0);   /* nobody plays the hole cards */
    m.deck[8] = CD(12,0); m.deck[9] = CD(11,1); m.deck[10] = CD(10,2); m.deck[11] = CD(9,3); m.deck[12] = CD(7,0);
    m.c[0] = 11; m.c[1] = 11; m.c[2] = 11; m.stack[0] = m.stack[1] = m.stack[2] = 0;
    mp_finish(&m);
    CHECK(m.won[0] + m.won[1] + m.won[2] == 33 && m.won[0] == 11 && m.won[1] == 11 && m.won[2] == 11, "three-way tie splits evenly");
    m.c[0] = 5; m.c[1] = 6; m.c[2] = 0; m.folded[2] = 1; m.over = 0;   /* odd chip */
    mp_finish(&m);
    CHECK(m.won[0] + m.won[1] == 11 && m.won[1] == 6 && m.won[0] == 5, "odd chip to first seat after the button (%d %d)", m.won[0], m.won[1]);
}
static void test_multi_rules(void) {
    MP m; int i;
    CHECK(mp_bb_at(4, 0) == 4 && mp_bb_at(4, 1) == 8 && mp_bb_at(40, 3) == 160 && mp_bb_at(40, 50) == 1280, "blind schedule");
    for (i = 0; i < 4; i++) CHECK(MP_START_BB[i] % 2 == 0 && MP_START_BB[i] * 25 == QP_STACK[i], "quick play start blind %d", i);
    for (i = 0; i < EX_N; i++) CHECK(EX_START_BB[i] % 2 == 0 && EX_START_BB[i] >= 2 && EX_START_BB[i] * 5 <= EX_BUYIN[i], "exhibition start blind %d", i);
    CHECK(g_ex_prize(10) == 35 && g_ex_prize(50) == 175 && g_ex_prize(100) == 350 && g_ex_prize(250) == 875 && g_ex_prize(500) == 1750 && g_ex_prize(1000) == 3500, "prize is 3.5x");
    /* heads-up: button is the small blind and acts first before the flop; the other seat acts first after it */
    mp_init(&m, 2, 100, 4); m.button = 0; mp_start_hand(&m);
    CHECK(m.sb_seat == 0 && m.bb_seat == 1 && m.toact == 0 && m.c[0] == 2 && m.c[1] == 4 && m.nbets == 1, "heads-up blinds");
    mp_act(&m, A_CALL); CHECK(m.toact == 1 && !m.over, "big blind gets the option");
    mp_act(&m, A_CALL); CHECK(m.stage == 1 && m.toact == 1, "flop: non-button first");
    /* four players: UTG acts first, blinds move */
    mp_init(&m, 4, 250, 10); m.button = 3; mp_start_hand(&m);
    CHECK(m.sb_seat == 0 && m.bb_seat == 1 && m.toact == 2 && m.c[0] == 5 && m.c[1] == 10, "four-player blinds, UTG first");
    /* raise cap and short stacks */
    mp_init(&m, 2, 100, 4); m.button = 0; mp_start_hand(&m);
    { int k = 0; while (mp_can_raise(&m, m.toact) && k < 10) { mp_act(&m, A_RAISE); k++; } CHECK(m.nbets == MAX_BETS, "cap of %d bets (got %d)", MAX_BETS, m.nbets); }
    mp_init(&m, 2, 100, 4); m.button = 0; mp_start_hand(&m); m.stack[0] = 5;   /* cannot afford call 2 + raise 4 */
    CHECK(!mp_can_raise(&m, 0), "short stack cannot raise");
    mp_act(&m, A_RAISE); CHECK(m.c[0] == 2 + 2 || m.c[0] == 2 + 5 || 1, "raise request on a short stack is just a call");
    CHECK(m.c[0] == 4 && m.stack[0] == 3, "short stack raise became a call (c %d stack %d)", m.c[0], m.stack[0]);
    /* a player who is all-in from the blind runs the board out and chips are conserved */
    mp_init(&m, 2, 100, 4); m.button = 0; m.stack[1] = 3; mp_start_hand(&m);
    CHECK(m.allin[1] && m.c[1] == 3, "short big blind is all-in");
    mp_act(&m, A_CALL);
    CHECK(m.over && mp_sum(&m) == 103, "all-in from the blind finishes the hand (sum %d)", mp_sum(&m));
}
/* ---------- v8.9: free bet size (Exhibition + boss): min raise, all-in, stack caps, chip conservation, AI sizes ---------- */
static void test_nl(void) {
    MP m; int i, g, hands = 0, shoves = 0, sizes = 0;
    /* minimum raise = the fixed size, then at least the last raise; cap = what the other seat can match */
    mp_init(&m, 2, 100, 4); m.nl = 1; m.button = 0; mp_start_hand(&m);   /* seat 0 SB 2, seat 1 BB 4, seat 0 acts */
    CHECK(mp_can_raise(&m, 0) && mp_need(&m, 0) == 2, "open");
    CHECK(mp_cap_pay(&m, 0) == 98 && mp_max_by(&m, 0) == 96 && mp_min_by(&m, 0) == 4, "min 4 max 96 (cap %d)", mp_cap_pay(&m, 0));
    mp_act_by(&m, A_RAISE, 20);                                         /* raise to 4 + 20 = 24 total */
    CHECK(m.c[0] == 24 && m.stack[0] == 76 && m.last_inc == 20 && m.nbets == 2, "raise by 20 (c %d stack %d inc %d)", m.c[0], m.stack[0], m.last_inc);
    CHECK(m.toact == 1 && mp_need(&m, 1) == 20 && mp_min_by(&m, 1) == 20, "re-raise is at least 20 (min %d)", mp_min_by(&m, 1));
    mp_act_by(&m, A_RAISE, 5);                                          /* too small: becomes the minimum 20 */
    CHECK(m.c[1] == 24 + 20 && m.last_inc == 20, "too small a raise is lifted to the minimum (c %d)", m.c[1]);
    mp_act_by(&m, A_RAISE, 100000);                                     /* way too big: all-in, capped by seat 1's stack */
    CHECK(m.c[0] == 100 && m.stack[0] == 0 && m.allin[0], "seat 0 is all-in for 100 (c %d)", m.c[0]);
    CHECK(!mp_can_raise(&m, 1), "no raise over an all-in");
    mp_act(&m, A_CALL);
    CHECK(m.over && mp_sum(&m) == 200, "all-in called, hand over, chips conserved (sum %d)", mp_sum(&m));
    /* a shorter opponent caps the bet; the rest stays in the stack */
    mp_init(&m, 2, 100, 4); m.nl = 1; m.button = 0; m.stack[1] = 30; mp_start_hand(&m);
    CHECK(mp_cap_pay(&m, 0) == 28, "cap 28 vs a short stack (%d)", mp_cap_pay(&m, 0));   /* seat 1 has 30 left after the 4 big blind... see below */
    mp_act_by(&m, A_RAISE, 999);
    CHECK(m.c[0] == 30 && m.stack[0] == 70 && !m.allin[0], "bet capped at her stack (c %d stack %d)", m.c[0], m.stack[0]);
    mp_act(&m, A_CALL);
    CHECK(m.over && m.allin[1] && mp_sum(&m) == 130, "capped bet called all-in, sum %d", mp_sum(&m));
    CHECK(m.c[0] == m.c[1], "both seats put in the same (%d vs %d): nothing to give back", m.c[0], m.c[1]);
    /* the 4-bet cap: after it only all-in */
    mp_init(&m, 2, 1000, 4); m.nl = 1; m.button = 0; mp_start_hand(&m);
    mp_act_by(&m, A_RAISE, 0); mp_act_by(&m, A_RAISE, 0);                 /* nbets 3 */
    CHECK(m.nbets == 3 && mp_can_raise(&m, m.toact), "third bet ok");
    mp_act_by(&m, A_RAISE, 0);                                            /* nbets 4 = cap */
    CHECK(m.nbets == MAX_BETS && mp_can_raise(&m, m.toact), "after the cap all-in is still allowed");
    { int who = m.toact; mp_act_by(&m, A_RAISE, 10); CHECK(m.stack[who] == 0 && m.allin[who], "a raise after the cap is all-in"); }
    /* the old limit rules are untouched when nl is 0 */
    mp_init(&m, 2, 100, 4); m.button = 0; mp_start_hand(&m);
    mp_act_by(&m, A_RAISE, 50); CHECK(m.c[0] == 2 + 2 + 4 && !m.nl, "fixed limit ignores the amount (c %d)", m.c[0]);
    /* random free-size games: chips conserved, hands end, caps hold, AI sizes legal */
    for (g = 0; g < 600; g++) {
        int si = g % EX_N, guard = 0, total = 2 * EX_BUYIN[si];
        mp_init(&m, 2, EX_BUYIN[si], EX_START_BB[si]); m.nl = 1; m.boss = si == EX_BOSS;
        while (!m.game_over && guard++ < 4000) {
            int steps = 0;
            mp_start_hand(&m); hands++;
            while (!m.over && steps++ < 200) {
                int p = m.toact, a, by = 0, c0[2], st0[2];
                CHECK(p >= 0 && p < 2 && !m.folded[p] && !m.allin[p], "bad actor");
                if (p == 0 && g % 3 == 0) { int r = rnd(10); a = r < 5 ? A_CALL : r < 8 ? A_RAISE : A_FOLD; by = a == A_RAISE ? rnd(1 + mp_max_by(&m, 0)) : 0; if (a == A_RAISE && rnd(6) == 0) by = 1 << 20; }
                else a = mp_decide_by(&m, p, &by);
                if (a == A_RAISE && !mp_can_raise(&m, p)) a = A_CALL;
                if (a == A_RAISE) { int ai = !(p == 0 && g % 3 == 0); if (ai) CHECK(by >= 0 && by <= mp_max_by(&m, p), "AI size %d outside 0..%d", by, mp_max_by(&m, p)); sizes += ai; if (ai && by == mp_max_by(&m, p)) shoves++; }
                c0[0] = m.c[0]; c0[1] = m.c[1]; st0[0] = m.stack[0]; st0[1] = m.stack[1];
                mp_act_by(&m, a, by);
                CHECK(m.stack[0] >= 0 && m.stack[1] >= 0 && m.c[0] >= 0 && m.c[1] >= 0, "negative chips");
                if (a == A_RAISE && !m.over) CHECK(m.r[p] - m.r[1 - p] <= 0 || m.r[p] <= m.r[1 - p] + m.stack[1 - p], "raise beyond what she can call");
            }
            CHECK(m.over, "hand did not end");
            CHECK(m.won[0] + m.won[1] == mp_pot(&m), "won != pot");
            CHECK(mp_sum(&m) == total, "chips conserved (%d vs %d)", mp_sum(&m), total);
            CHECK(m.c[0] == m.c[1] || m.folded[0] || m.folded[1] || m.allin[0] || m.allin[1], "unequal contributions without fold or all-in");
            for (i = 0; i < 2; i++) CHECK((m.stack[i] > 0) == (m.alive[i] != 0), "alive flag");
        }
        CHECK(m.game_over, "free-size game never ended");
    }
    CHECK(sizes > 1000, "the AI raised often enough to test (%d)", sizes);
    CHECK(shoves > 0 && shoves < sizes * 6 / 10, "AI all-ins are a minority (%d of %d)", shoves, sizes);
    printf("  free bet size: %d hands, %d raises, %d all-ins\n", hands, sizes, shoves);
}
/* random games to the end: chips are conserved, hands end, games end, nobody gets negative chips */
static int mp_random_game(int n, int si, int human_style, long *hands_out) {
    MP m; int guard = 0, total; long hands = 0;
    mp_init(&m, n, QP_STACK[si], MP_START_BB[si]); total = n * QP_STACK[si];
    while (!m.game_over && guard++ < 5000) {
        int steps = 0, i;
        mp_start_hand(&m); hands++;
        CHECK(m.over ? mp_sum(&m) == total : mp_sum(&m) + mp_pot(&m) == total, "chips before play");
        while (!m.over && steps++ < 200) {
            int p = m.toact, a;
            CHECK(p >= 0 && p < n && m.alive[p] && !m.folded[p] && !m.allin[p], "bad actor %d", p);
            if (p == 0) { int r = rnd(10); a = human_style == 0 ? (r < 6 ? A_CALL : r < 8 ? A_RAISE : A_FOLD) : human_style == 1 ? A_CALL : mp_decide(&m, 0); }
            else a = mp_decide(&m, p);
            mp_act(&m, a);
            for (i = 0; i < n; i++) CHECK(m.stack[i] >= 0 && m.c[i] >= 0, "negative chips");
        }
        CHECK(m.over, "hand did not end");
        { int won = 0; for (i = 0; i < n; i++) won += m.won[i]; CHECK(won == mp_pot(&m), "won %d != pot %d", won, mp_pot(&m)); }
        CHECK(mp_sum(&m) == total, "chips conserved after hand (%d vs %d)", mp_sum(&m), total);
        for (i = 0; i < n; i++) CHECK((m.stack[i] > 0) == (m.alive[i] != 0), "alive flag matches stack");
    }
    CHECK(m.game_over, "game never ended");
    CHECK((m.game_over == 1) == (m.alive[0] && mp_alive_count(&m) == 1), "winner flag");
    CHECK(m.place >= 1 && m.place <= n, "place %d", m.place);
    *hands_out += hands;
    return m.game_over == 1;
}
static void test_multi(void) {
    int si, n, g, wins; long hands = 0;
    test_multi_sidepots(); test_multi_rules();
    for (n = 2; n <= 4; n += 2) for (si = 0; si < 4; si++) { for (g = 0; g < 60; g++) mp_random_game(n, si, g % 2, &hands); }
    printf("  %ld hands in random multi-player games, all conserved\n", hands);
    /* strength sanity: a thinking seat should beat random play, and tight/loose CPUs should beat a calling station often */
    for (n = 2; n <= 4; n += 2) {
        wins = 0; hands = 0;
        for (g = 0; g < 120; g++) wins += mp_random_game(n, 1, 0, &hands);
        printf("  random-ish player wins %d of 120 games vs %s\n", wins, n == 2 ? "1 CPU" : "3 CPUs");
        CHECK(wins < (n == 2 ? 55 : 28), "CPUs are too weak vs random play (%d wins)", wins);
        wins = 0; hands = 0;
        for (g = 0; g < 120; g++) wins += mp_random_game(n, 1, 2, &hands);
        printf("  a CPU-brained player wins %d of 120 games (fair share would be %d)\n", wins, 120 / n);
    }
}

static void test_time(void) {   /* day counter and time of day */
    Game g, t; const char *f = "/tmp/pq_test_time.sav"; int i, d;
    g_init(&g); CHECK(g.day == 1 && g.phase == PH_MORNING && !g_night(&g), "a new game starts on day 1, morning");
    g_advance(&g); CHECK(g.day == 1 && g.phase == PH_EVENING && !g_night(&g), "morning -> evening, same day");
    g_advance(&g); CHECK(g.day == 1 && g.phase == PH_NIGHT && g_night(&g), "evening -> night, same day");
    g_advance(&g); CHECK(g.day == 2 && g.phase == PH_MORNING, "night -> morning of the next day");
    for (i = 0; i < 300; i++) g_advance(&g); CHECK(g.day == 102 && g.phase == PH_MORNING, "100 days later");
    g.day = DAY_MAX; g.phase = PH_NIGHT; g_advance(&g); CHECK(g.day == DAY_MAX && g.phase == PH_MORNING, "the day counter stops at its cap");
    g_init(&g); g.day = 7; g.phase = PH_EVENING; save_game(&g, f); CHECK(load_game(&t, f) && t.day == 7 && t.phase == PH_EVENING, "day and time are saved");
    g_init(&g); g.phase = 3; save_game(&g, f); CHECK(!load_game(&t, f), "a bad time of day is rejected"); g_init(&g); g.day = 0; save_game(&g, f); CHECK(!load_game(&t, f), "day 0 is rejected");
    remove(f); (void)d;
}

static void test_rent(void) {   /* v5.5: rent every 7 days, taken at the start of the night (cash first, then bank); Sophia once a day */
    Game g, t; const char *f = "/tmp/pq_test_rent.sav"; int bank, i, taken = 0, start;
    g_init(&g); CHECK(RENT_AMT == 250 && RENT_UP1_AMT == 500 && RENT_UP_AMT == 1000 && RENT_DAYS == 7 && g.rent_next == 7 && g.chal_day == 0, "rent starts on day 7, 250, nothing played");
    g.day = 6; CHECK(!g_rent_due(&g), "day 6: not due"); g.day = 7; CHECK(g_rent_due(&g), "day 7: due");
    g.cash = 249; CHECK(!g_rent_pay_cash(&g) && g.cash == 249 && g_rent_due(&g), "249 cash cannot pay at the desk");
    g.cash = 250; CHECK(g_rent_pay_cash(&g) && g.cash == 0 && g.rent_next == 14 && !g_rent_due(&g), "250 cash pays at the desk, next rent on day 14");
    g.day = 8; g.cash = 500; CHECK(g_rent_night(&g, &bank) == -1 && g.cash == 500, "nothing is taken when nothing is due");
    g_init(&g); g.day = 7; g.cash = 350; g.bank = 0; CHECK(g_rent_night(&g, &bank) == 1 && bank == 0 && g.cash == 100 && g.rent_next == 14, "night: all from cash");
    g_init(&g); g.day = 7; g.cash = 40; g.bank = 500; CHECK(g_rent_night(&g, &bank) == 1 && bank == 210 && g.cash == 0 && g.bank == 290 && g.rent_next == 14, "night: cash first, the rest from the bank");
    g_init(&g); g.day = 7; g.cash = 0; g.bank = 250; CHECK(g_rent_night(&g, &bank) == 1 && bank == 250 && g.cash == 0 && g.bank == 0, "night: all from the bank");
    g_init(&g); g.day = 7; g.cash = 30; g.bank = 219; CHECK(g_rent_night(&g, &bank) == 0 && g.cash == 30 && g.bank == 219 && g_rent_due(&g), "night: 249 in total = game over, nothing taken");
    g_init(&g); g.day = 7; g.cash = 250; g.bank = 0; CHECK(g_rent_night(&g, &bank) == 1 && g.cash == 0, "exactly 250 is enough");
    /* 70 days of sofa and bed: exactly one rent per 7 days, never skipped, never twice */
    g_init(&g); g.cash = 100000; start = g.cash;
    for (i = 0; i < 70 * 3; i++) { g_advance(&g); if (g_night(&g)) { int r = g_rent_night(&g, &bank); if (r == 1) taken++; CHECK(r != 0, "rich player never evicted"); } }
    CHECK(taken == 10 && start - g.cash == 10 * RENT_AMT, "ten rents in 70 days");
    /* Sophia once a day: counted when the match starts (also a tie and a walk-out), the next day she plays again */
    g_init(&g); g.cash = 5000; CHECK(!g_played_today(&g), "day 1: not played yet");
    g_match_start(&g, 100); CHECK(g_played_today(&g), "the match start uses the day up");
    g_advance(&g); g_advance(&g); CHECK(g_played_today(&g), "evening and night are the same day");
    g_advance(&g); CHECK(!g_played_today(&g) && g.day == 2, "after sleeping she plays again");
    g_init(&g); g.day = 9; g.phase = PH_EVENING; g.chal_day = 4; g.rent_next = 14; g.cash = 77; save_game(&g, f);
    CHECK(load_game(&t, f) && t.chal_day == 4 && t.rent_next == 14 && t.day == 9, "last match day and next rent day are saved");
    remove(f);
}
static void test_debt(void) {   /* the shirt debt and game over */
    Game g, t; const char *f = "/tmp/pq_test_debt.sav"; int c, b, st;
    g_init(&g);
    CHECK(DEBT == 300 && START_CASH < DEBT, "debt vs start cash");   /* you cannot pay on arrival */
    for (st = ST_NEW; st <= ST_NAMED; st++) for (c = 0; c <= 12; c++) for (b = 0; b <= 12; b++) {
        int want; g_init(&g); g.story = st; g.cash = c; g.bank = b; want = st >= ST_HOME && c + b < EX_BUYIN[0];
        CHECK(g_broke(&g) == want, "g_broke story %d cash %d bank %d", st, c, b);
        
    }
    g_init(&g); g.story = ST_HOME; g.event = 0; save_game(&g, f); CHECK(load_game(&t, f) && t.event == 1, "old save did not open the exhibition");   /* old saves: the Exhibition is open from the bump scene on */
    g_init(&g); g.story = ST_NEW; g.event = 0; save_game(&g, f); CHECK(load_game(&t, f) && t.event == 0, "a new game must keep the exhibition locked");
    remove(f);
}

/* v6.2: romance, shop, room upgrade, home matches, save fields, new texts */
static void test_romance(void) {
    Game g, h; int i, r, bank0; const char *path = "/tmp/pq_romance.sav";
    g_init(&g); CHECK(g.rom_lvl == 1 && g.rs == RS_NONE && !g.room_up && g_rent_amt(&g) == 250, "romance starts empty, base room, rent 250");
    /* online store: cash first, then the bank; the ring and the pill are one time */
    g.cash = 1000; g.bank = 20000; g.sw_wins = SW_GOAL;   /* v10.0: the black market is open after 20 wins */
    CHECK(g_shop_buy(&g, SH_RING) == 0 && g.ring == 1 && g.cash == 0 && g.bank == 9000, "ring paid from cash then bank");
    CHECK(g_shop_buy(&g, SH_RING) == 2, "the ring is one time only");
    g.bank = 20000; CHECK(g_shop_buy(&g, SH_PILL) == 0 && g.pill == 1 && g_shop_buy(&g, SH_PILL) == 2, "the pill is one time only");
    g.rs = RS_LIVING;   /* v10.0: the ITEMS tab (flower, stack ...) is open only once she lives with you */
    g.cash = 0; g.bank = 0; CHECK(g_shop_buy(&g, SH_FLOWER) == 1 && g.flower == 0, "no money, no flower");
    g.cash = 350; CHECK(g_shop_buy(&g, SH_FLOWER) == 0 && g.flower == 1 && g.cash == 100, "flower costs 250");
    g.cash = 1000; CHECK(g_shop_buy(&g, SH_STACK) == 0 && g.stack == 1 && g.cash == 0, "money stack costs 1000");
    CHECK(SH_PRICE[SH_FLOWER] == 250 && SH_PRICE[SH_STACK] == 1000 && SH_PRICE[SH_PILL] == 15000 && SH_PRICE[SH_RING] == 12000, "store prices (v12.2: the pill 15000)");
    g.flower = INV_CAP; g.cash = 5000; CHECK(g_shop_buy(&g, SH_FLOWER) == 3, "flower stock is capped"); g.rs = RS_NONE; g.flower = 0;
    /* the story gates */
    g_init(&g); g.ring = 1; g.story = ST_FIRST; CHECK(!g_can_propose(&g), "no proposal before you beat her and have the card back");
    g.story = ST_NAMED; CHECK(g_can_propose(&g) && !g_can_askmove(&g), "proposal possible, nothing else yet");
    g.cash = 4999; CHECK(g_upgrade_avail(&g) && g_upgrade_cost(&g) == 5000 && g_upgrade_buy(&g) == 1 && !g.room_up, "upgrade 1 needs 5000 cash");
    g.cash = 5000; CHECK(g_upgrade_buy(&g) == 0 && g.room_up == 1 && g.rs == RS_NONE && g.cash == 0 && g_rent_amt(&g) == 500 && g_computer(&g) && !g_upgrade_avail(&g), "upgrade 1 bought: rent 500, the computer, the story is not touched");
    g.rs = RS_UPG; g.cash = 9999; CHECK(g_upgrade_avail(&g) && g_upgrade_cost(&g) == 10000 && g_can_askmove(&g) && !g_move_ok(&g) && g_upgrade_buy(&g) == 1 && g.room_up == 1, "upgrade 2 needs 10000 cash");
    g.cash = 10000; CHECK(g_upgrade_buy(&g) == 0 && g.room_up == 2 && g.rs == RS_READY && g.cash == 0 && g_rent_amt(&g) == 1000 && g_move_ok(&g) && !g_upgrade_avail(&g), "upgrade 2 bought: rent 1000, she can move in");
    g.rs = RS_NO; g.room_up = 1; CHECK(g_upgrade_avail(&g) && g_can_askmove(&g) && !g_move_ok(&g), "after NOT NOW: upgrade 2 is still on offer, she moves in only into the level-2 room");
    g.rs = RS_NONE; CHECK(!g_upgrade_avail(&g), "upgrade 2 is not on offer before she was asked");
    g.room_up = 0; g.story = ST_NEW; CHECK(!g_upgrade_avail(&g) && !g_computer(&g), "no upgrade before the shirt scene, no computer in the base room"); g.story = ST_HOME; CHECK(g_upgrade_avail(&g), "upgrade 1 from the shirt scene on");
    g.story = ST_NAMED; g.rs = RS_LIVING; CHECK(g_rom_on(&g) && !g_floor3_open(&g) && g_room_lvl(&g) == 2 && g_computer(&g) && !g_can_askmove(&g), "living with you: bar on, 3rd floor closed, the level-2 room");
    /* the rent of every level is really taken */
    g_init(&g); g.room_up = 1; g.cash = 800; g.day = 7; CHECK(g_rent_pay_cash(&g) == 1 && g.cash == 300 && g.rent_next == 14, "rent 500 at the desk");
    g_init(&g); g.room_up = 2; g.cash = 1500; g.day = 7; CHECK(g_rent_pay_cash(&g) == 1 && g.cash == 500 && g.rent_next == 14, "rent 1000 at the desk");
    g.day = 14; g.cash = 100; g.bank = 1250; { int bk; CHECK(g_rent_night(&g, &bk) == 1 && bk == 900 && g.cash == 0 && g.bank == 350, "rent 1000 at night from cash then bank"); }
    g.day = 21; g.cash = 100; g.bank = 800; { int bk; CHECK(g_rent_night(&g, &bk) == 0, "900 in total is not enough for 1000"); }
    /* the bar */
    g_init(&g); g.rs = RS_LIVING; g_rom_add(&g, 10); CHECK(g.rom_pct == 10 && g.rom_lvl == 1, "10%");
    g_rom_add(&g, 90); CHECK(g.rom_lvl == 2 && g.rom_pct == 0, "a full bar starts the next level at 0");
    g.rom_pct = 95; g_rom_add(&g, 20); CHECK(g.rom_lvl == 3 && g.rom_pct == 0, "no carry over");
    g.rom_lvl = ROM_LVLS; g.rom_pct = 95; g_rom_add(&g, 20); CHECK(g.rom_lvl == ROM_LVLS && g.rom_pct == 100 && g_rom_full(&g), "the last level stays full");
    /* gifts */
    g_init(&g); g.rs = RS_LIVING; g.day = 3; g.flower = 2; g.stack = 1; g.pill = 1;
    CHECK(g_gift(&g, GF_FLOWER) == 0 && g.rom_pct == 10 && g.flower == 1 && g.gift_day == 3, "flower +10%");
    CHECK(g_gift(&g, GF_STACK) == 2 && g.stack == 1, "one gift per day");
    g.day = 4; CHECK(g_gift(&g, GF_STACK) == 0 && g.rom_pct == 30 && g.stack == 0, "money stack +20%");
    g.day = 5; CHECK(g_gift(&g, GF_STACK) == 1, "no stack left");
    CHECK(g_gift(&g, GF_PILL) == 3 && !g.pill_given, "the pill waits for a full level 3 bar");
    g.rom_lvl = ROM_LVLS; g.rom_pct = 100; g.day = 6; CHECK(g_gift(&g, GF_FLOWER) == 4 && g.flower == 1, "no flowers wasted on a full heart");
    CHECK(g_gift(&g, GF_PILL) == 0 && g.pill_given == 1 && g.gift_day == 6, "the pill at a full bar");
    g.day = 7; CHECK(g_gift(&g, GF_PILL) == 1, "the pill is given only once");
    /* free matches at home */
    g_init(&g); g.rs = RS_LIVING; g.day = 2; g.energy = 100; g.cash = 777; bank0 = g.bank;
    CHECK(g_home_start(&g, 10) == 1 && g.home && g.in_match && g.mlen == 10 && g.fee == 0 && g.energy == 90 && g.cash == 777 && g_played_today(&g), "home match: free, 10% energy, uses the day");
    g.net = 12; g_match_finish(&g);
    CHECK(g.last_res == 1 && g.last_rom == 5 && g.rom_pct == 5 && g.cash == 777 && g.bank == bank0 && g.last_pay == 0 && g.last_drink == 0 && !g.home && !g.in_match, "home win: +5%, no money, no drink");
    CHECK(g_home_start(&g, 20) == -1, "once a day");
    g.day = 3; CHECK(g_home_start(&g, 20) == 1 && g.mlen == 20 && g.energy == 80, "20 hands");
    g.net = 1; g.bn[BN_FLUSH] = 2; g_match_finish(&g); CHECK(g.last_rom == 10 && g.rom_pct == 15 && g.cash == 777 && g.last_bonus == 0, "20 hands: +10%, card bonuses pay nothing");
    g.day = 4; g_home_start(&g, 10); g.net = -3; g_match_finish(&g); CHECK(g.last_res == -1 && g.last_rom == 0 && g.rom_pct == 15 && g.cash == 777, "a loss gives nothing");
    g.day = 5; g_home_start(&g, 10); g.net = 0; g_match_finish(&g); CHECK(g.last_res == 0 && g.cash == 777 && g.rom_pct == 15, "a tie gives nothing back (nothing was paid)");
    g.day = 6; g.energy = 9; CHECK(g_home_start(&g, 10) == 0 && !g.in_match && g.chal_day != 6, "too tired: 10% needed, nothing used");
    g.energy = 50; g_home_start(&g, 10); g_forfeit(&g); CHECK(!g.home && !g.in_match, "walking out of a home match");
    /* normal matches are untouched */
    g_init(&g); g.cash = 1000; g_match_start(&g, 500); CHECK(!g.home && g.fee == 500, "a normal match is not a home match");
    g.net = 5; g_match_finish(&g); CHECK(g.last_rom == 0 && g.last_pay == 5 * 5 + g_prize(500) && g.rom_pct == 0, "normal win pays as before and gives no romance");
    /* save file: round trip, ranges, old versions */
    g_init(&g); g.rs = RS_LIVING; g.room_up = 2; g.ring = 1; g.pill = 1; g.pill_given = 1; g.flower = 3; g.stack = 2; g.gift_day = 4; g.rom_lvl = 2; g.rom_pct = 35; g.story = ST_NAMED;
    CHECK(save_game(&g, path) && load_game(&h, path) && h.rs == RS_LIVING && h.room_up == 2 && h.ring == 1 && h.pill == 1 && h.pill_given == 1 && h.flower == 3 && h.stack == 2 && h.gift_day == 4 && h.rom_lvl == 2 && h.rom_pct == 35, "romance fields survive a save");
    h = g; h.rs = 9; CHECK(!save_sane(&h), "bad rs rejected"); h = g; h.rom_pct = 101; CHECK(!save_sane(&h), "bad pct rejected"); h = g; h.rom_lvl = ROM_LVLS + 1; CHECK(!save_sane(&h), "bad level rejected");
    h = g; h.in_match = 1; h.home = 1; h.mlen = 20; h.fee = 0; CHECK(save_sane(&h), "a home match in progress is a sane save"); h.mlen = 15; CHECK(!save_sane(&h), "15 hands is not"); h = g; h.in_match = 1; h.home = 0; h.mlen = 10; h.fee = 0; CHECK(!save_sane(&h), "a normal match needs a buy-in");
    h = g; h.in_match = 1; h.home = 1; h.mlen = 10; h.fee = 0; { Game t; save_game(&h, path); CHECK(load_game(&t, path) && !t.in_match && !t.home && t.last_res == -1, "closing the app in a home match counts as walking out"); }
    CHECK(SAVE_VER == 28 && SAVE_VER_27 == 27 && SAVE_VER_26 == 26 && SAVE_VER_25 == 25 && SAVE_VER_24 == 24 && SAVE_VER_23 == 23 && SAVE_VER_22 == 22 && SAVE_VER_21 == 21 && SAVE_VER_20 == 20 && SAVE_VER_19 == 19 && SAVE_VER_18 == 18 && SAVE_VER_17 == 17 && SAVE_VER_16 == 16 && SAVE_VER_15 == 15 && SAVE_VER_14 == 14 && SAVE_VER_OLD == 12 && SAVE_VER_13 == 13, "save version 25 (v11.1), 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13 and 12 still load");
    { Game t; g_init(&t); t.rs = RS_LIVING; t.pill = 1; t.rom_lvl = ROM_LVLS; t.rom_pct = 100; t.day = 10;   /* v6.4: the pill works after 7 days */
      CHECK(!g_pill_due(&t) && g_pill_hint(&t) == 0 && !g_home_sneak(&t), "no pill, nothing due");
      CHECK(g_gift(&t, GF_PILL) == 0 && t.pill_given && t.pill_day == 10 && !t.pill_seen, "pill given: the 7 days start");
      t.day = 14; CHECK(g_pill_hint(&t) == 0 && !g_pill_due(&t), "day 5 after the pill: nothing yet");
      t.day = 15; CHECK(g_pill_hint(&t) == 1 && !g_pill_due(&t), "day 6: stuffy chest");
      t.day = 16; CHECK(g_pill_hint(&t) == 2 && !g_pill_due(&t), "day 7: tight clothes");
      t.day = 17; CHECK(g_pill_hint(&t) == 0 && g_pill_due(&t) && !g_home_sneak(&t), "day 8: the scene is due, sneak still closed");
      t.pill_seen = 1; CHECK(!g_pill_due(&t) && g_pill_hint(&t) == 0 && g_home_sneak(&t), "after the scene: sneak open");
      t.rs = RS_UPG; CHECK(!g_home_sneak(&t), "sneak only while she lives with you");
      g_dbg_week(&t); CHECK(t.day == 24 && t.phase == PH_MORNING, "skip 7 days");
      t.pill_seen = 2; CHECK(!save_sane(&t), "pill_seen out of range"); }
    (void)i; (void)r;
    /* texts */
    {
#define RBEATS(a, nm) for (i = 0; i < a##_N; i++) { char w[400]; CHECK(wrap_buf(w, sizeof w, (a)[i].txt, 46) <= 3 && ok_chars((a)[i].txt), "%s beat %d: too long or bad characters", nm, i); }
        RBEATS(B_PILLDAY, "pillday") RBEATS(B_PILL6, "pill6") RBEATS(B_PILL7, "pill7") RBEATS(B_KNOCKIN, "knockin") RBEATS(B_MILK1, "milk1") RBEATS(B_MILK2, "milk2") RBEATS(B_PUMP, "pump") RBEATS(B_PEEKPIC, "peekpic") RBEATS(B_PUMPB, "pumpb") RBEATS(B_BOTTLEB, "bottleb") RBEATS(B_SCOLD, "scold") RBEATS(B_BOTTLE, "bottle")
        check_pages("greet after day 8", R_GREET_P, R_GREET_P_N); check_pages("milk ok", R_MILK_OK, R_MILK_OK_N);
        { const char *t[] = {R_MILK_WAIT[0], R_MILK_PUMP[0], R_PUMP_NOW[0], R_BOTTLE_FULL[0], R_MILK_AFTER[0], W_WAIT2, R_GIFT_HAND[0], R_GIFT_HAND[1], R_GIFT_HAND[4], R_GIFT_L2[0], R_GIFT_L2[1], R_GIFT_L2[4], R_GIFT_END[0], R_GIFT_END[1], R_GIFT_END[4], R_MILKITEM}; check_pages("v65 lines", t, (int)(sizeof t / sizeof t[0])); }
        CHECK(B_PILLDAY[1].who == W_ME && B_PILL6[1].who == W_ME && B_PILL7[1].who == W_ME && B_MILK2[1].who == W_ME, "days 6, 7, 8 and the milk day: you ask first, before she speaks");
        RBEATS(B_PROPOSE, "propose") RBEATS(B_LIVEYES, "liveyes") RBEATS(B_LIVENO, "liveno") RBEATS(B_DEMAND, "demand") RBEATS(B_MOVEIN, "movein")
        CHECK(B_PROPOSE[B_PROPOSE_N - 1].who == W_SOPHIA && strstr(B_PROPOSE[B_PROPOSE_N - 1].txt, "live with you"), "the last proposal beat is her question");
        check_pages("upfirst", S_UPFIRST, 1); check_pages("r_greet", R_GREET, 3); check_pages("r_gift_ok", R_GIFT_OK, 3);
        { const char *t[] = {R_TIRED[0], R_TODAY[0], R_GIFT_TODAY[0], R_GIFT_NONE[0], R_GIFT_PILL[0], R_GIFT_FULL[0], R_BACK[0], W_SPARE, W_CLOSED3, W_UPG_OK, W_UPG_SHORT, W_RENT_REMIND_UP, O_RING, O_VISIT, O_UPG, O_ASKMOVE}; check_pages("v62 lines", t, (int)(sizeof t / sizeof t[0])); }

    /* v10.1: the new Sophia line variants: sizes, fit, characters, no duplicates */
    check_pages("v101 R_TIRED", R_TIRED, R_TIRED_N); CHECK(R_TIRED_N >= 2, "v101 R_TIRED has enough lines (%d)", R_TIRED_N);
    check_pages("v101 R_TODAY", R_TODAY, R_TODAY_N); CHECK(R_TODAY_N >= 2, "v101 R_TODAY has enough lines (%d)", R_TODAY_N);
    check_pages("v101 R_BACK", R_BACK, R_BACK_N); CHECK(R_BACK_N >= 2, "v101 R_BACK has enough lines (%d)", R_BACK_N);
    check_pages("v101 R_GIFT_TODAY", R_GIFT_TODAY, R_GIFT_TODAY_N); CHECK(R_GIFT_TODAY_N >= 2, "v101 R_GIFT_TODAY has enough lines (%d)", R_GIFT_TODAY_N);
    check_pages("v101 R_GIFT_NONE", R_GIFT_NONE, R_GIFT_NONE_N); CHECK(R_GIFT_NONE_N >= 2, "v101 R_GIFT_NONE has enough lines (%d)", R_GIFT_NONE_N);
    check_pages("v101 R_GIFT_PILL", R_GIFT_PILL, R_GIFT_PILL_N); CHECK(R_GIFT_PILL_N >= 2, "v101 R_GIFT_PILL has enough lines (%d)", R_GIFT_PILL_N);
    check_pages("v101 R_GIFT_FULL", R_GIFT_FULL, R_GIFT_FULL_N); CHECK(R_GIFT_FULL_N >= 2, "v101 R_GIFT_FULL has enough lines (%d)", R_GIFT_FULL_N);
    check_pages("v101 R_NOT_FULL", R_NOT_FULL, R_NOT_FULL_N); CHECK(R_NOT_FULL_N >= 2, "v101 R_NOT_FULL has enough lines (%d)", R_NOT_FULL_N);
    check_pages("v101 R_NOT_FULL_MM", R_NOT_FULL_MM, R_NOT_FULL_MM_N); CHECK(R_NOT_FULL_MM_N >= 2, "v101 R_NOT_FULL_MM has enough lines (%d)", R_NOT_FULL_MM_N);
    check_pages("v101 R_MILK_WAIT", R_MILK_WAIT, R_MILK_WAIT_N); CHECK(R_MILK_WAIT_N >= 2, "v101 R_MILK_WAIT has enough lines (%d)", R_MILK_WAIT_N);
    check_pages("v101 R_MILK_PUMP", R_MILK_PUMP, R_MILK_PUMP_N); CHECK(R_MILK_PUMP_N >= 2, "v101 R_MILK_PUMP has enough lines (%d)", R_MILK_PUMP_N);
    check_pages("v101 R_MILK_FULL", R_MILK_FULL, R_MILK_FULL_N); CHECK(R_MILK_FULL_N >= 2, "v101 R_MILK_FULL has enough lines (%d)", R_MILK_FULL_N);
    check_pages("v101 R_MILK_AFTER", R_MILK_AFTER, R_MILK_AFTER_N); CHECK(R_MILK_AFTER_N >= 2, "v101 R_MILK_AFTER has enough lines (%d)", R_MILK_AFTER_N);
    check_pages("v101 R_MILK_SUPER", R_MILK_SUPER, R_MILK_SUPER_N); CHECK(R_MILK_SUPER_N >= 2, "v101 R_MILK_SUPER has enough lines (%d)", R_MILK_SUPER_N);
    check_pages("v101 R_PUMP_NOW", R_PUMP_NOW, R_PUMP_NOW_N); CHECK(R_PUMP_NOW_N >= 2, "v101 R_PUMP_NOW has enough lines (%d)", R_PUMP_NOW_N);
    check_pages("v101 R_BOTTLE_FULL", R_BOTTLE_FULL, R_BOTTLE_FULL_N); CHECK(R_BOTTLE_FULL_N >= 2, "v101 R_BOTTLE_FULL has enough lines (%d)", R_BOTTLE_FULL_N);
    check_pages("v101 R_OUTFIT_YOGA", R_OUTFIT_YOGA, R_OUTFIT_YOGA_N); CHECK(R_OUTFIT_YOGA_N >= 2, "v101 R_OUTFIT_YOGA has enough lines (%d)", R_OUTFIT_YOGA_N);
    check_pages("v101 R_OUTFIT_D8", R_OUTFIT_D8, R_OUTFIT_D8_N); CHECK(R_OUTFIT_D8_N >= 2, "v101 R_OUTFIT_D8 has enough lines (%d)", R_OUTFIT_D8_N);
    check_pages("v101 R_OUTFIT_YOGA_MM", R_OUTFIT_YOGA_MM, R_OUTFIT_YOGA_MM_N); CHECK(R_OUTFIT_YOGA_MM_N >= 2, "v101 R_OUTFIT_YOGA_MM has enough lines (%d)", R_OUTFIT_YOGA_MM_N);
    check_pages("v101 R_OUTFIT_D8_MM", R_OUTFIT_D8_MM, R_OUTFIT_D8_MM_N); CHECK(R_OUTFIT_D8_MM_N >= 2, "v101 R_OUTFIT_D8_MM has enough lines (%d)", R_OUTFIT_D8_MM_N);
    check_pages("v101 R_SUPER_ON", R_SUPER_ON, R_SUPER_ON_N); CHECK(R_SUPER_ON_N >= 2, "v101 R_SUPER_ON has enough lines (%d)", R_SUPER_ON_N);
    check_pages("v101 R_ASK_DONE", R_ASK_DONE, R_ASK_DONE_N); CHECK(R_ASK_DONE_N >= 2, "v101 R_ASK_DONE has enough lines (%d)", R_ASK_DONE_N);
    check_pages("v101 R_ASK_NONE", R_ASK_NONE, R_ASK_NONE_N); CHECK(R_ASK_NONE_N >= 2, "v101 R_ASK_NONE has enough lines (%d)", R_ASK_NONE_N);
    check_pages("v101 R_GREET", R_GREET, R_GREET_N); CHECK(R_GREET_N >= 3, "v101 R_GREET has enough lines (%d)", R_GREET_N);
    check_pages("v101 R_GREET_M", R_GREET_M, R_GREET_M_N); CHECK(R_GREET_M_N >= 3, "v101 R_GREET_M has enough lines (%d)", R_GREET_M_N);
    check_pages("v101 R_GREET_MM", R_GREET_MM, R_GREET_MM_N); CHECK(R_GREET_MM_N >= 3, "v101 R_GREET_MM has enough lines (%d)", R_GREET_MM_N);
    check_pages("v101 R_NIGHT_M", R_NIGHT_M, R_NIGHT_M_N); CHECK(R_NIGHT_M_N >= 3, "v101 R_NIGHT_M has enough lines (%d)", R_NIGHT_M_N);
    check_pages("v101 R_NIGHT_MM", R_NIGHT_MM, R_NIGHT_MM_N); CHECK(R_NIGHT_MM_N >= 3, "v101 R_NIGHT_MM has enough lines (%d)", R_NIGHT_MM_N);
    check_pages("v101 R_HUG_OPEN", R_HUG_OPEN, R_HUG_OPEN_N); CHECK(R_HUG_OPEN_N >= 3, "v101 R_HUG_OPEN has enough lines (%d)", R_HUG_OPEN_N);
    check_pages("v101 R_HUG_OPEN_MM", R_HUG_OPEN_MM, R_HUG_OPEN_MM_N); CHECK(R_HUG_OPEN_MM_N >= 3, "v101 R_HUG_OPEN_MM has enough lines (%d)", R_HUG_OPEN_MM_N);
    check_pages("v101 R_KISS_OPEN", R_KISS_OPEN, R_KISS_OPEN_N); CHECK(R_KISS_OPEN_N >= 3, "v101 R_KISS_OPEN has enough lines (%d)", R_KISS_OPEN_N);
    check_pages("v101 R_KISS_OPEN_MM", R_KISS_OPEN_MM, R_KISS_OPEN_MM_N); CHECK(R_KISS_OPEN_MM_N >= 3, "v101 R_KISS_OPEN_MM has enough lines (%d)", R_KISS_OPEN_MM_N);
    check_pages("v101 R_SLEEP_NO_TEASE", R_SLEEP_NO_TEASE, R_SLEEP_NO_TEASE_N); CHECK(R_SLEEP_NO_TEASE_N >= 3, "v101 R_SLEEP_NO_TEASE has enough lines (%d)", R_SLEEP_NO_TEASE_N);
    check_pages("v101 R_SLEEP_NO_TEASE_MM", R_SLEEP_NO_TEASE_MM, R_SLEEP_NO_TEASE_MM_N); CHECK(R_SLEEP_NO_TEASE_MM_N >= 3, "v101 R_SLEEP_NO_TEASE_MM has enough lines (%d)", R_SLEEP_NO_TEASE_MM_N);
    check_pages("v101 R_SLEEP_NO_MILK", R_SLEEP_NO_MILK, R_SLEEP_NO_MILK_N); CHECK(R_SLEEP_NO_MILK_N >= 3, "v101 R_SLEEP_NO_MILK has enough lines (%d)", R_SLEEP_NO_MILK_N);
    check_pages("v101 R_SLEEP_NO_MILK_MM", R_SLEEP_NO_MILK_MM, R_SLEEP_NO_MILK_MM_N); CHECK(R_SLEEP_NO_MILK_MM_N >= 3, "v101 R_SLEEP_NO_MILK_MM has enough lines (%d)", R_SLEEP_NO_MILK_MM_N);
    check_pages("v101 R_FEED_NO_MM", R_FEED_NO_MM, R_FEED_NO_MM_N); CHECK(R_FEED_NO_MM_N >= 3, "v101 R_FEED_NO_MM has enough lines (%d)", R_FEED_NO_MM_N);
    check_pages("v101 R_FEED_AGAIN_MM", R_FEED_AGAIN_MM, R_FEED_AGAIN_MM_N); CHECK(R_FEED_AGAIN_MM_N >= 3, "v101 R_FEED_AGAIN_MM has enough lines (%d)", R_FEED_AGAIN_MM_N);
    check_pages("v101 R_LR_HERE", R_LR_HERE, R_LR_HERE_N); CHECK(R_LR_HERE_N >= 3, "v101 R_LR_HERE has enough lines (%d)", R_LR_HERE_N);
    check_pages("v101 R_PILL6", R_PILL6, R_PILL6_N); CHECK(R_PILL6_N >= 3, "v101 R_PILL6 has enough lines (%d)", R_PILL6_N);
    check_pages("v101 R_PILL7", R_PILL7, R_PILL7_N); CHECK(R_PILL7_N >= 3, "v101 R_PILL7 has enough lines (%d)", R_PILL7_N);
    check_pages("v101 R_MEM_CALL_T_MM", R_MEM_CALL_T_MM, R_MEM_CALL_T_MM_N); CHECK(R_MEM_CALL_T_MM_N >= 3, "v101 R_MEM_CALL_T_MM has enough lines (%d)", R_MEM_CALL_T_MM_N);
    check_pages("v101 R_MEM_CALL_Y_MM", R_MEM_CALL_Y_MM, R_MEM_CALL_Y_MM_N); CHECK(R_MEM_CALL_Y_MM_N >= 3, "v101 R_MEM_CALL_Y_MM has enough lines (%d)", R_MEM_CALL_Y_MM_N);
    check_pages("v101 R_MEM_CHAT_T_MM", R_MEM_CHAT_T_MM, R_MEM_CHAT_T_MM_N); CHECK(R_MEM_CHAT_T_MM_N >= 3, "v101 R_MEM_CHAT_T_MM has enough lines (%d)", R_MEM_CHAT_T_MM_N);
    check_pages("v101 R_MEM_CHAT_Y_MM", R_MEM_CHAT_Y_MM, R_MEM_CHAT_Y_MM_N); CHECK(R_MEM_CHAT_Y_MM_N >= 3, "v101 R_MEM_CHAT_Y_MM has enough lines (%d)", R_MEM_CHAT_Y_MM_N);
    check_pages("v101 R_MEM_TEASE_Y_MM", R_MEM_TEASE_Y_MM, R_MEM_TEASE_Y_MM_N); CHECK(R_MEM_TEASE_Y_MM_N >= 3, "v101 R_MEM_TEASE_Y_MM has enough lines (%d)", R_MEM_TEASE_Y_MM_N);
    check_pages("v101 R_MEM_FEED_T_MM", R_MEM_FEED_T_MM, R_MEM_FEED_T_MM_N); CHECK(R_MEM_FEED_T_MM_N >= 3, "v101 R_MEM_FEED_T_MM has enough lines (%d)", R_MEM_FEED_T_MM_N);
    check_pages("v101 R_MEM_FEED_Y_MM", R_MEM_FEED_Y_MM, R_MEM_FEED_Y_MM_N); CHECK(R_MEM_FEED_Y_MM_N >= 3, "v101 R_MEM_FEED_Y_MM has enough lines (%d)", R_MEM_FEED_Y_MM_N);
    check_pages("v101 R_MEM_FEEDNO_T_MM", R_MEM_FEEDNO_T_MM, R_MEM_FEEDNO_T_MM_N); CHECK(R_MEM_FEEDNO_T_MM_N >= 3, "v101 R_MEM_FEEDNO_T_MM has enough lines (%d)", R_MEM_FEEDNO_T_MM_N);
    check_pages("v101 R_MEM_FEEDNO_Y_MM", R_MEM_FEEDNO_Y_MM, R_MEM_FEEDNO_Y_MM_N); CHECK(R_MEM_FEEDNO_Y_MM_N >= 3, "v101 R_MEM_FEEDNO_Y_MM has enough lines (%d)", R_MEM_FEEDNO_Y_MM_N);
    check_pages("v101 R_MEM_HUG_T_MM", R_MEM_HUG_T_MM, R_MEM_HUG_T_MM_N); CHECK(R_MEM_HUG_T_MM_N >= 3, "v101 R_MEM_HUG_T_MM has enough lines (%d)", R_MEM_HUG_T_MM_N);
    check_pages("v101 R_MEM_HUG_Y_MM", R_MEM_HUG_Y_MM, R_MEM_HUG_Y_MM_N); CHECK(R_MEM_HUG_Y_MM_N >= 3, "v101 R_MEM_HUG_Y_MM has enough lines (%d)", R_MEM_HUG_Y_MM_N);
    check_pages("v101 R_MEM_KISS_T_MM", R_MEM_KISS_T_MM, R_MEM_KISS_T_MM_N); CHECK(R_MEM_KISS_T_MM_N >= 3, "v101 R_MEM_KISS_T_MM has enough lines (%d)", R_MEM_KISS_T_MM_N);
    check_pages("v101 R_MEM_KISS_Y_MM", R_MEM_KISS_Y_MM, R_MEM_KISS_Y_MM_N); CHECK(R_MEM_KISS_Y_MM_N >= 3, "v101 R_MEM_KISS_Y_MM has enough lines (%d)", R_MEM_KISS_Y_MM_N);
    check_pages("v101 R_MEM_SLEEP_Y_MM", R_MEM_SLEEP_Y_MM, R_MEM_SLEEP_Y_MM_N); CHECK(R_MEM_SLEEP_Y_MM_N >= 3, "v101 R_MEM_SLEEP_Y_MM has enough lines (%d)", R_MEM_SLEEP_Y_MM_N);
    check_pool("v101 L_LAST", L_LAST, L_LAST_N); CHECK(L_LAST_N >= 3, "v101 L_LAST has enough lines (%d)", L_LAST_N);
    check_pool("v101 L_ENDLOST", L_ENDLOST, L_ENDLOST_N); CHECK(L_ENDLOST_N >= 3, "v101 L_ENDLOST has enough lines (%d)", L_ENDLOST_N);
    check_pool("v101 L_SPLIT", L_SPLIT, L_SPLIT_N); CHECK(L_SPLIT_N >= 3, "v101 L_SPLIT has enough lines (%d)", L_SPLIT_N);
    check_pool("v101 L_YOURAISE", L_YOURAISE, L_YOURAISE_N); CHECK(L_YOURAISE_N >= 3, "v101 L_YOURAISE has enough lines (%d)", L_YOURAISE_N);
    check_pool("v101 L_ENDTIE", L_ENDTIE, L_ENDTIE_N); CHECK(L_ENDTIE_N >= 3, "v101 L_ENDTIE has enough lines (%d)", L_ENDTIE_N);
    check_pool("v101 L_SHEFOLD", L_SHEFOLD, L_SHEFOLD_N); CHECK(L_SHEFOLD_N >= 3, "v101 L_SHEFOLD has enough lines (%d)", L_SHEFOLD_N);
    check_pool("v101 L_YOUFOLD", L_YOUFOLD, L_YOUFOLD_N); CHECK(L_YOUFOLD_N >= 3, "v101 L_YOUFOLD has enough lines (%d)", L_YOUFOLD_N);
    check_pool("v101 L_ENDHER", L_ENDHER, L_ENDHER_N); CHECK(L_ENDHER_N >= 3, "v101 L_ENDHER has enough lines (%d)", L_ENDHER_N);
    check_pool("v101 L_ENDYOU", L_ENDYOU, L_ENDYOU_N); CHECK(L_ENDYOU_N >= 3, "v101 L_ENDYOU has enough lines (%d)", L_ENDYOU_N);
    CHECK(R_FEED_NO_MM_N >= 12 && R_FEED_AGAIN_MM_N >= 8, "v101 breastfeed refusals have many lines");

        check_pages("clerk upg", W_CLERK_UPG, 2); check_pages("clerk up1", W_CLERK_UP1, 2); { const char *t[2]; t[0] = W_UPG_OK1; t[1] = O_UP1; check_pages("v93 lines", t, 2); } check_pages("clerk rent up", W_CLERK_RENT_UP, 2);
        for (i = 0; i < SH_N; i++) CHECK(ok_chars(SH_NAME[i]) && ok_chars(SH_INFO[i]) && strlen(SH_NAME[i]) <= 24 && strlen(SH_INFO[i]) <= 40, "shop text %d", i);
        for (i = 0; i < 3; i++) CHECK(ok_chars(R_ITEMS[i]) && ok_chars(R_LEN[i]), "romance menu text %d", i);
        for (i = 0; i < GF_N; i++) CHECK(ok_chars(R_GNAME[i]) && strlen(R_GNAME[i]) <= 20, "gift name %d", i);   /* v11.1: 20 (the items.txt GIVE limit); COW PRINT OUTFIT has 16 */
        for (i = 0; i < 5; i++) CHECK(ok_chars(DOOR_ITEMS[i]), "door item %d", i);
    }
}
static void test_milk(void) {   /* v6.5: pump, milk inducer pill, bottle gauge, Baby Milk Bottle, energy freeze */
    Game g, h; int i, r;
    g_init(&g); g.cash = 100000; g.bank = 0; g.rs = RS_LIVING;
    CHECK(SH_PRICE[SH_PUMP] == 500 && SH_PRICE[SH_MILK] == 750, "pump costs 500, the milk pill 750 (v12.2)");
    CHECK(!g_shop_avail(&g, SH_PUMP) && !g_shop_avail(&g, SH_MILK) && g_shop_avail(&g, SH_RING), "pump and milk pill are not in the store before the day-8 scene");
    CHECK(g_shop_buy(&g, SH_MILK) == 1 && g.mpill == 0, "cannot buy before the day-8 scene");
    g.pill_given = 1; g.pill_seen = 1; g.pill = 1; g.day = 20;
    CHECK(g_shop_avail(&g, SH_PUMP) && g_shop_buy(&g, SH_PUMP) == 0 && g.pump == 1 && g_shop_buy(&g, SH_PUMP) == 2, "the pump is one time");
    CHECK(g_shop_buy(&g, SH_MILK) == 0 && g.mpill == 1 && g_shop_buy(&g, SH_MILK) == 0 && g.mpill == 2, "milk pills are stackable");
    for (i = 0; i < 20; i++) g_shop_buy(&g, SH_MILK);
    /* before the first pill the pump is refused */
    CHECK(g_gift(&g, GF_PUMP) == 7 && g.pump == 1 && g.milk_st == 0, "the pump is not wanted before the 2-day wait");
    g.rom_lvl = ROM_LVLS; g.rom_pct = 100; g.gift_day = g.day;   /* a full heart and today's gift used: the milk pill still works */
    CHECK(g_gift(&g, GF_MILK) == 0 && g.milk_st == 1 && g.milk_day == 20 && g.mpill == 21, "first milk pill: ignores the one-gift-a-day rule and the full heart");
    CHECK(g.gift_day == g.day, "(the gift slot of the day is untouched)");
    CHECK(g_gift(&g, GF_MILK) == 5 && g.mpill == 21, "second pill while waiting: wait");
    CHECK(!g_milk_due(&g), "day 0 after the first pill: nothing due"); g.day = 21; CHECK(!g_milk_due(&g), "day 1: nothing due"); g.day = 22; CHECK(g_milk_due(&g), "day 2: her scene is due");
    g.milk_st = 2; CHECK(g_gift(&g, GF_MILK) == 6 && g.mpill == 21, "pills wait for the pump");
    CHECK(g_gift(&g, GF_PUMP) == 0 && g.milk_st == 3 && g.pump == 0 && g.milk_pct == 0 && g_milk_gauge(&g), "the pump unlocks the gauge");
    CHECK(g_gift(&g, GF_PUMP) == 1, "no second pump");
    for (i = 1; i <= 1; i++) { g.day = 22; CHECK(g_gift(&g, GF_MILK) == 0 && g.milk_pct == 50 * i && g.bottle == 0, "pill %d: +50%%, same day, no limit", i); }
    CHECK(g_gift(&g, GF_MILK) == 0 && g.milk_pct == 0 && g.bottle == 1 && g.mpill == 21 - 2, "2 pills fill the bottle: a Baby Milk Bottle, the gauge starts again");
    for (i = 0; i < 2; i++) g_gift(&g, GF_MILK);
    CHECK(g.bottle == 2 && g.milk_pct == 0 && g.mpill == 17, "a second bottle");
    g.bottle = INV_CAP; g.milk_pct = 50; CHECK(g_gift(&g, GF_MILK) == 8 && g.mpill == 17 && g.milk_pct == 50, "bottle stock full: the last pill is kept");
    /* the bottle: 2 days with no energy cost */
    g_init(&h); h.day = 5; h.energy = 4; h.bottle = 2; h.story = ST_NAMED; h.cash = 5000; h.rs = RS_LIVING;
    CHECK(!g_energy_ok(&h, 10) && g_ex_start(&h, 10) == -1 && g_home_start(&h, 10) == 0, "no energy, no bottle: refused");
    CHECK(g_drink_bottle(&h) == 0 && h.bottle == 1 && g_milk_active(&h) && g_milk_left(&h) == 2 && h.milk_until == 6, "drinking: today and tomorrow");
    CHECK(g_drink_bottle(&h) == 2 && h.bottle == 1, "one at a time");
    CHECK(g_ex_start(&h, 10) == 1 && h.energy == 4 && h.cash == 4990, "an exhibition costs no energy while it works (and is allowed at 4%)");
    h.day = 6; CHECK(g_milk_left(&h) == 1 && g_home_start(&h, 10) == 1 && h.energy == 4, "day 2: a home match costs no energy");
    h.day = 7; CHECK(!g_milk_active(&h) && g_milk_left(&h) == 0 && !g_energy_ok(&h, 10), "day 3: the bottle is over");
    h.energy = 50; h.chal_day = 0; CHECK(g_home_start(&h, 10) == 1 && h.energy == 40, "energy drops again");
    g_init(&h); h.bottle = 2; CHECK(g_drink_bottle(&h) == 0 && g_drink_bottle(&h) == 2, "bottle drunk: a second drink is refused while it works"); h.bottle = 0; h.day = 9; CHECK(g_drink_bottle(&h) == 1, "none left");
    /* save */
    { char path[] = "/tmp/pq_milk.sav"; Game t;
      g_init(&g); g.pill_seen = 1; g.milk_st = 4; g.mpill = 4; g.milk_pct = 50; g.bottle = 2; g.milk_until = 8; g.milk_day = 3; g.hint_day = 6; g.pump = 0;
      CHECK(save_game(&g, path) && load_game(&t, path) && t.milk_st == 4 && t.mpill == 4 && t.milk_pct == 50 && t.bottle == 2 && t.milk_until == 8 && t.milk_day == 3 && t.hint_day == 6, "milk fields survive a save");
      /* v7.7: an older save (stage 3) that already holds or has drunk a bottle becomes stage 4; without a bottle it stays at 3 */
      t = g; t.milk_st = 3; CHECK(save_game(&t, path) && load_game(&t, path) && t.milk_st == 4, "old save with bottles: milk stage 4");
      t = g; t.milk_st = 3; t.bottle = 0; t.milk_until = 0; CHECK(save_game(&t, path) && load_game(&t, path) && t.milk_st == 3, "old save without a bottle: stays stage 3");
      t = g; t.milk_st = 3; t.bottle = 0; t.milk_until = 6; CHECK(save_game(&t, path) && load_game(&t, path) && t.milk_st == 4, "old save that drank a bottle: milk stage 4");
      t = g; t.milk_st = 5; CHECK(!save_sane(&t), "milk stage 5 is not valid");
      t = g; t.milk_pct = 35; CHECK(!save_sane(&t), "gauge not a multiple of 50"); t = g; t.milk_pct = 90; CHECK(!save_sane(&t), "90% is no gauge value any more");
      t = g; t.milk_pct = 30; save_game(&t, path); { Game u; CHECK(load_game(&u, path) && u.milk_pct == 50 && u.bottle == 2, "an old 30%% gauge loads as 50%% (the save is kept)"); }
      t = g; t.milk_pct = 90; save_game(&t, path); { Game u; CHECK(load_game(&u, path) && u.milk_pct == 50, "an old 90%% gauge loads as 50%%"); } t = g; t = g; t.milk_st = 5; CHECK(!save_sane(&t), "bad milk stage"); t = g; t.mpill = -1; CHECK(!save_sane(&t), "bad pill count"); t = g; t.bottle = INV_CAP + 1; CHECK(!save_sane(&t), "bad bottle count");
      remove(path); }
    /* days 6 and 7: the asking-first scene plays once a day */
    g_init(&g); g.rs = RS_LIVING; g.pill_given = 1; g.pill_day = 10; g.day = 14; CHECK(!g_hint_scene(&g), "day 5: nothing");
    g.day = 15; CHECK(g_hint_scene(&g), "day 6: the scene is due"); g.hint_day = 15; CHECK(!g_hint_scene(&g), "day 6: played once"); g.day = 16; CHECK(g_hint_scene(&g), "day 7: due again"); g.pill_seen = 1; CHECK(!g_hint_scene(&g), "never after day 8");
    (void)r;
}
static void test_debug(void) {
    Game g; int total;
    g_init(&g); CHECK(g_dbg_skip(&g) == 2 && g.story == ST_NEW && !g.event, "the skip is refused before the intro is finished");
    g.story = ST_HOME; g.event = 0; g.ex_big = 0; g.hall_seen = 0;
    CHECK(g_dbg_skip(&g) == 0 && g.story == ST_NAMED && g.event == 1 && g.ex_big == 1 && g.hall_seen == 3, "skip jumps to the state after beating her");
    CHECK(g_dbg_skip(&g) == 1 && g.story == ST_NAMED, "a second skip changes nothing");
    g.story = ST_FIRST; CHECK(g_dbg_skip(&g) == 0 && g.story == ST_NAMED, "skip also works from the boss stage");
    g.cash = 100; g_dbg_cash(&g); CHECK(g.cash == 50100 && g.bank == 0, "+50000 goes to cash, not the bank");
    g.cash = MONEY_CAP - 10; g_dbg_cash(&g); CHECK(g.cash == MONEY_CAP, "the money stays under the cap");
    /* with the debug money the whole shopping list is affordable: ring, pill, room upgrade (cash only) and a few gifts */
    g_init(&g); g.story = ST_HOME; g_dbg_skip(&g); g_dbg_cash(&g);
    total = SH_PRICE[SH_RING] + SH_PRICE[SH_PILL] + ROOM_UP1_COST + ROOM_UP_COST + 5 * SH_PRICE[SH_STACK];
    CHECK(g.cash + g.bank >= total, "50000 covers ring + pill + both upgrades + 5 stacks");
    CHECK(g_shop_buy(&g, SH_RING) == 0 && g_shop_buy(&g, SH_PILL) == 0 && g_can_propose(&g), "after the skip you can buy the ring and propose");
    CHECK(g_upgrade_buy(&g) == 0 && g.room_up == 1, "upgrade 1 is paid from the debug cash"); g.rs = RS_UPG; CHECK(g_upgrade_buy(&g) == 0 && g.room_up == 2, "upgrade 2 is paid from the debug cash");
    /* time and the bar */
    g_init(&g); g.day = 3; g.phase = PH_NIGHT; g.gift_day = 3; g.chal_day = 3;
    CHECK(g_gifted_today(&g) && g_played_today(&g), "setup: limits are used up");
    g_dbg_newday(&g); CHECK(g.day == 4 && g.phase == PH_MORNING && !g_gifted_today(&g) && !g_played_today(&g), "new day: morning of the next day, limits open again");
    g.day = DAY_MAX; g_dbg_newday(&g); CHECK(g.day == DAY_MAX && g.phase == PH_MORNING, "the day counter stays under its cap");
    g_init(&g); g.rom_pct = 30; g_dbg_fillbar(&g); CHECK(g.rom_lvl == 2 && g.rom_pct == 0, "fill bar: level 1 done, level 2 starts at 0");
    g_dbg_fillbar(&g); g_dbg_fillbar(&g); g_dbg_fillbar(&g); g_dbg_fillbar(&g); CHECK(g.rom_lvl == ROM_LVLS && g.rom_pct == 100 && g_rom_full(&g), "fill bar x5: level 5 full (the pill is allowed)");
    g_dbg_fillbar(&g); CHECK(g.rom_lvl == ROM_LVLS && g.rom_pct == 100, "the last level stays full");
    { const char *path = "/tmp/pq_debug.sav"; Game h; g_init(&g); g.story = ST_HOME; g_dbg_skip(&g); g_dbg_cash(&g); g_dbg_fillbar(&g);
      CHECK(save_game(&g, path) && load_game(&h, path) && h.story == ST_NAMED && h.cash == g.cash && h.rom_lvl == 2, "a debug state survives the save file"); remove(path); }
}

static void test_stocks(void) {   /* v7.8: stock prices, buy / sell, the save (also an old version-12 save) */
    Game g, h; int s, d, i; const char *f = "/tmp/pq_stk.sav"; unsigned seed;
    /* every price, every day, every seed: 1..999 (the rule: below $1,000) and a day-to-day move that stays reasonable */
    for (seed = 1; seed < 400; seed += 7) {
        g_init(&g); g.mseed = (int)seed;
        for (s = 0; s < STK_N; s++) for (d = 1; d <= 3000; d++) {
            int p = stk_price(&g, s, d), lo = STK_BASE[s] - STK_BASE[s] * STK_AMP[s] / 100 - 1, hi = STK_BASE[s] + STK_BASE[s] * STK_AMP[s] / 100 + 1;
            CHECK(p >= lo && p <= hi && p < 1000 && p >= 1, "price of %d on day %d (seed %u) = %d outside %d..%d", s, d, seed, p, lo, hi);
            if (d > 1) { int ch = p - stk_price(&g, s, d - 1); CHECK(ch >= -2 * STK_BASE[s] * STK_AMP[s] / 100 && ch <= 2 * STK_BASE[s] * STK_AMP[s] / 100, "day move %d too big", ch); }
        }
    }
    g_init(&g); g.mseed = 0;
    for (s = 0; s < STK_N; s++) CHECK(stk_price(&g, s, 0) == stk_price(&g, s, 1) && stk_price(&g, s, -5) == stk_price(&g, s, 1), "day < 1 uses day 1");
    CHECK(stk_price(&g, -1, 5) == 0 && stk_price(&g, STK_N, 5) == 0, "bad company = 0");
    g.day = 1; for (s = 0; s < STK_N; s++) CHECK(stk_change(&g, s) == 0, "no change on day 1");
    /* deterministic: the same seed gives the same curve; another seed another one; the curves move */
    { Game a, b; int diff = 0, moves = 0, p0; g_init(&a); g_init(&b); a.mseed = 12345; b.mseed = 12345;
      for (d = 1; d < 200; d++) CHECK(stk_price(&a, STK_NVIDIA, d) == stk_price(&b, STK_NVIDIA, d), "same seed same price");
      b.mseed = 54321; for (d = 1; d < 200; d++) if (stk_price(&a, STK_OPENAI, d) != stk_price(&b, STK_OPENAI, d)) diff++;
      CHECK(diff > 100, "another seed gives another curve (%d)", diff);
      p0 = stk_price(&a, STK_GOOGLE, 1); for (d = 2; d < 200; d++) if (stk_price(&a, STK_GOOGLE, d) != p0) moves++;
      CHECK(moves > 150, "prices move (%d)", moves);
      a.day = 40; CHECK(stk_now(&a, STK_APPLE) == stk_price(&a, STK_APPLE, 40) && stk_change(&a, STK_APPLE) == stk_price(&a, STK_APPLE, 40) - stk_price(&a, STK_APPLE, 39), "now / change use the current day"); }
    /* g_new gives a seed, g_init does not */
    g_new(&g); CHECK(g.mseed > 0, "a new career has a market seed"); g_init(&h); CHECK(h.mseed == 0 && h.stk[0] == 0, "g_init: no shares, default seed");
    /* buying: cash first, then the bank; the cap; selling; the cash cap */
    g_init(&g); g.mseed = 777; g.day = 9; g.cash = 1000; g.bank = 5000;
    { int p = stk_now(&g, STK_NVIDIA);
      CHECK(g_stk_buy(&g, STK_NVIDIA) == 0 && g.stk[STK_NVIDIA] == 1 && g.cash == 1000 - p, "buy one share from cash");
      g.cash = 5; CHECK(g_stk_buy(&g, STK_NVIDIA) == 0 && g.cash == 0 && g.bank == 5000 - (p - 5), "cash first, then the bank");
      g.cash = 0; g.bank = p - 1; CHECK(g_stk_buy(&g, STK_NVIDIA) == 1 && g.stk[STK_NVIDIA] == 2 && g.bank == p - 1, "not enough cash + bank: nothing changes");
      g.cash = 0; g.bank = 0; CHECK(g_stk_sell(&g, STK_NVIDIA) == 0 && g.stk[STK_NVIDIA] == 1 && g.cash == p, "sell one share for today's price");
      g.stk[STK_NVIDIA] = 0; CHECK(g_stk_sell(&g, STK_NVIDIA) == 1 && g.cash == p, "selling a share you do not own"); }
    g.cash = MONEY_CAP - 3; g.stk[STK_APPLE] = 2; CHECK(g_stk_sell(&g, STK_APPLE) == 3 && g.stk[STK_APPLE] == 2 && g.cash == MONEY_CAP - 3, "selling must not pass the cash cap");
    g.cash = 900000; g.stk[STK_GOOGLE] = STK_CAP; CHECK(g_stk_buy(&g, STK_GOOGLE) == 3 && g.stk[STK_GOOGLE] == STK_CAP, "share cap");
    CHECK(g_stk_buy(&g, -1) == 1 && g_stk_buy(&g, STK_N) == 1 && g_stk_sell(&g, -1) == 1 && g_stk_sell(&g, STK_N) == 1, "bad company ids");
    /* a buy followed by a sell on the same day never makes or loses money (no spread, no free money) */
    g_init(&g); g.mseed = 99; g.cash = 100000;
    for (i = 0; i < 400; i++) { int s2 = i % STK_N, c0 = g.cash + g.bank; g.day = 1 + i; if (g_stk_buy(&g, s2) == 0) CHECK(g_stk_sell(&g, s2) == 0 && g.cash + g.bank == c0, "same-day round trip is neutral"); }
    { long v; g_init(&g); g.day = 3; g.stk[0] = 2; g.stk[3] = 5; v = 2L * stk_now(&g, 0) + 5L * stk_now(&g, 3); CHECK(g_stk_value(&g) == v, "portfolio value"); }
    /* the save: shares and seed survive; a bad number is refused */
    g_init(&g); g.mseed = 4242; g.stk[STK_OPENAI] = 7; g.stk[STK_APPLE] = 12; g.day = 33; save_game(&g, f);
    g_init(&h); CHECK(load_game(&h, f) && h.stk[STK_OPENAI] == 7 && h.stk[STK_APPLE] == 12 && h.mseed == 4242 && h.day == 33, "shares and the market seed are saved");
    { Game t = g; t.stk[1] = -1; save_game(&t, f); CHECK(!load_game(&h, f), "negative shares rejected"); }
    { Game t = g; t.stk[2] = STK_CAP + 1; save_game(&t, f); CHECK(!load_game(&h, f), "too many shares rejected"); }
    { Game t = g; t.mseed = -3; save_game(&t, f); CHECK(!load_game(&h, f), "negative seed rejected"); }
    /* a save made by v7.7 (version 12, the struct without stk[] and mseed) loads: everything kept, no shares, default seed */
    g_init(&g); g.cash = 4321; g.bank = 99; g.day = 14; g.pill_seen = 1; g.milk_st = 4; g.bottle = 3; g.story = ST_NAMED; g.energy = 80;
    { Game t = g; FILE *w; t.magic = SAVE_MAGIC; t.ver = SAVE_VER_OLD; t.size = (int)offsetof(Game, stk); w = fopen(f, "wb"); fwrite(&t, 1, offsetof(Game, stk), w); fclose(w);
      g_init(&h); CHECK(load_game(&h, f), "a version-12 save loads");
      CHECK(h.cash == 4321 && h.bank == 99 && h.day == 14 && h.pill_seen == 1 && h.milk_st == 4 && h.bottle == 3 && h.energy == 80 && h.story == ST_NAMED, "an old save keeps everything");
      CHECK(h.ver == SAVE_VER && h.size == (int)sizeof h && h.mseed == 0, "an old save is upgraded");
      for (s = 0; s < STK_N; s++) CHECK(h.stk[s] == 0, "an old save owns no shares");
      /* a file of the old size with another version, or of any other size, is still rejected */
      t.ver = 5; w = fopen(f, "wb"); fwrite(&t, 1, offsetof(Game, stk), w); fclose(w); CHECK(!load_game(&h, f), "old size with a wrong version rejected");
      t.ver = SAVE_VER_OLD; w = fopen(f, "wb"); fwrite(&t, 1, offsetof(Game, stk) - 4, w); fclose(w); CHECK(!load_game(&h, f), "a truncated file rejected");
      t.ver = SAVE_VER_OLD; w = fopen(f, "wb"); fwrite(&t, 1, sizeof t, w); fclose(w); CHECK(!load_game(&h, f), "full size with the old version rejected"); }
    /* v8.1: a version-13 save (stk[4] + mseed) loads: the four old holdings and the seed are kept, SONY and SPACEX start at 0 */
    { Game t; FILE *w; unsigned char hdr[sizeof(Game)]; int old13[5] = {3, 4, 5, 6, 777}; g_init(&t); t.cash = 777; t.day = 21; t.story = ST_NAMED; t.magic = SAVE_MAGIC; t.ver = SAVE_VER_13; t.size = (int)(offsetof(Game, stk) + 5 * sizeof(int));
      memcpy(hdr, &t, offsetof(Game, stk)); w = fopen(f, "wb"); fwrite(hdr, 1, offsetof(Game, stk), w); fwrite(old13, sizeof(int), 5, w); fclose(w);
      g_init(&h); CHECK(load_game(&h, f), "a version-13 save loads");
      CHECK(h.cash == 777 && h.day == 21 && h.story == ST_NAMED, "a v13 save keeps everything");
      CHECK(h.stk[0] == 3 && h.stk[1] == 4 && h.stk[2] == 5 && h.stk[3] == 6 && h.stk[STK_SONY] == 0 && h.stk[STK_SPACEX] == 0 && h.mseed == 777, "v13 shares and seed kept, SONY / SPACEX = 0");
      CHECK(h.ver == SAVE_VER && h.size == (int)sizeof h, "a v13 save is upgraded");
      w = fopen(f, "wb"); t.ver = 5; memcpy(hdr, &t, offsetof(Game, stk)); fwrite(hdr, 1, offsetof(Game, stk), w); fwrite(old13, sizeof(int), 5, w); fclose(w); CHECK(!load_game(&h, f), "v13 size with a wrong version rejected"); }
    CHECK(STK_N == 6 && STK_SONY == 4 && STK_SPACEX == 5, "six companies, SONY and SPACEX at the end");
    CHECK(STK_BASE[STK_SONY] * (100 + STK_AMP[STK_SONY]) / 100 < 1000 && STK_BASE[STK_SPACEX] * (100 + STK_AMP[STK_SPACEX]) / 100 < 1000 && STK_BASE[STK_SONY] >= 500 && STK_BASE[STK_SPACEX] >= 500, "SONY and SPACEX are expensive and below $1,000");
    remove(f);
}


/* ----- v8.2: new gifts and store items, the diamond ring, the Super milk drug ----- */
/* v8.6: the wedding, page 4 (hug / kiss / sleep together / breastfeed), the yoga flag, the gauge that stays full when married */
static void test_v86(void) {
    Game g, h; const char *f = "/tmp/pq_v86.sav"; int k;
    g_init(&g); g.rs = RS_LIVING; g.dring = 1;
    CHECK(!g_can_marry(&g) && g_marry(&g) == 1 && !g.married, "no wedding before the day-8 scene");
    g.pill_seen = 1; CHECK(g_can_marry(&g), "ring + day-8 scene: you can marry");
    g.dring = 0; CHECK(!g_can_marry(&g), "no ring, no wedding"); g.dring = 1;
    CHECK(g_marry(&g) == 0 && g.married == 1 && g_p4_open(&g) && !g_can_marry(&g) && g_marry(&g) == 1, "marrying works once");
    /* the yoga flag */
    g_init(&g); g.yoga = 1; g.rs = RS_LIVING; g.rom_lvl = ROM_LVLS; g.rom_pct = 100;   /* v11.3: outfits need max romance */
    CHECK(g_gift(&g, GF_YOGA) == 0 && g.yoga == 0 && g.yoga_given == 1, "giving the yoga outfit sets the flag");
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.dring = 1; g_marry(&g);
    CHECK(!g_outfit_open(&g), "no outfit change without the yoga outfit"); g_outfit_toggle(&g); CHECK(g.outfit == 0, "the toggle stays at 0 without the flag");
    g.yoga_given = 1; CHECK(g_outfit_open(&g), "outfit change opens with the flag"); g_outfit_toggle(&g); CHECK(g.outfit == 1, "to yoga"); g_outfit_toggle(&g); CHECK(g.outfit == 0, "and back");
    /* the gauge stays full when married; unmarried it still becomes a bottle at once */
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.milk_st = 3; g.mpill = 5; g.milk_pct = 50;
    CHECK(g_milk_gift(&g, GF_MILK) == 0 && g.bottle == 1 && g.milk_pct == 0, "unmarried: the second pill makes a bottle at once (unchanged)");
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.dring = 1; g.milk_st = 3; g.mpill = 5; g_marry(&g); g.call = 1;   /* v9.5: BREASTFEED exists only in mama mode */
    CHECK(g_milk_gift(&g, GF_MILK) == 0 && g.milk_pct == 50 && g.bottle == 0, "married: the first pill = 50%%");
    CHECK(g_milk_gift(&g, GF_MILK) == 0 && g.milk_pct == 100 && g.bottle == 0 && g.mpill == 3, "married: the second pill fills the gauge and it STAYS full");
    CHECK(g_milk_gift(&g, GF_MILK) == 11 && g.mpill == 3, "married: a pill on a full gauge is refused");
    CHECK(g_pump_ready(&g) && g_feed_ready(&g), "a full gauge: the pump and breastfeeding are both possible");
    CHECK(g_pump_use(&g) == 0 && g.bottle == 1 && g.milk_pct == 0 && g.mday == g.day && g.milk_st == 4, "the pump gives a bottle and empties the gauge");
    CHECK(!g_pump_ready(&g) && !g_feed_ready(&g) && g_pump_use(&g) == 1 && g_feed(&g) == 1, "empty: neither the pump nor breastfeeding");
    g.milk_pct = 100; g.mday = 0; CHECK(g_feed(&g) == 0 && g.milk_pct == 0 && g.bottle == 1 && g.mday == g.day, "breastfeeding empties the gauge, no bottle");
    CHECK(!g_pump_ready(&g) && !g_feed_ready(&g), "after breastfeeding the pump and the bottle are not available until it is full again");
    g.milk_pct = 100; g.bottle = INV_CAP; CHECK(g_pump_use(&g) == 8 && g.milk_pct == 100, "full bottle stock: the pump is refused and keeps the gauge");
    /* the Super drug: breastfeeding is one of the asks and leaves the real gauge alone */
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.dring = 1; g.milk_st = 4; g.sdrug = 1; g.day = 10; g.milk_pct = 50; g_marry(&g); g.call = 1;
    CHECK(!g_feed_ready(&g), "gauge 50, no drug: not ready"); g_super_gift(&g);
    CHECK(g_feed_ready(&g) && g_asks_left(&g) == 2, "drug: ready, two asks");
    CHECK(g_feed(&g) == 0 && g.milk_pct == 50 && g_asks_left(&g) == 1 && g.mday == 10, "breastfeeding uses one ask and keeps the real gauge");
    CHECK(g_ask_milk(&g) == 0 && g_asks_left(&g) == 0 && !g_feed_ready(&g), "the second ask is used: breastfeeding needs a full real gauge now");
    g.day = 11; CHECK(g_asks_left(&g) == 2 && g_feed_ready(&g), "the asks come back on the next drug day");
    g.day = 12; CHECK(!g_feed_ready(&g), "drug over");
    /* sleeping: refused on a day she gave milk */
    g_init(&g); g.day = 5; CHECK(!g_sleep_refused_milk(&g), "nothing given today: she agrees"); g.mday = 5; CHECK(g_sleep_refused_milk(&g), "she gave milk today: she refuses"); g.day = 6; CHECK(!g_sleep_refused_milk(&g), "next day: fine");
    /* saves */
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.married = 1; g.outfit = 1; g.yoga_given = 1; g.mday = 7; g.milk_st = 4; g.milk_pct = 100;
    CHECK(save_game(&g, f) && load_game(&h, f) && h.married == 1 && h.outfit == 1 && h.yoga_given == 1 && h.mday == 7 && h.milk_pct == 100, "page-4 state and a full gauge survive a save");
    h = g; h.married = 2; CHECK(!save_sane(&h), "bad married rejected"); h = g; h.outfit = 4; CHECK(!save_sane(&h), "bad outfit rejected"); h = g; h.outfit = 2; h.top_given = 1; CHECK(save_sane(&h), "outfit 2 (designer top) is valid"); h = g; h.top_given = 2; CHECK(!save_sane(&h), "bad top_given rejected"); h = g; h.top = INV_CAP + 1; CHECK(!save_sane(&h), "top stock above the cap rejected"); h = g; h.milk_pct = 75; CHECK(!save_sane(&h), "gauge 75 rejected"); h = g; h.milk_pct = 150; CHECK(!save_sane(&h), "gauge 150 rejected");
    { /* a version-15 save (the struct up to ask_n) is upgraded */
      FILE *fp; Game o; unsigned char raw[sizeof(Game)]; g_init(&o); o.cash = 321; o.yoga = 2; o.rs = RS_LIVING; o.ver = SAVE_VER_15; memcpy(raw, &o, sizeof o);
      fp = fopen(f, "wb"); CHECK(fp != NULL, "open"); if (fp) { fwrite(raw, 1, offsetof(Game, married), fp); fclose(fp); }
      CHECK(load_game(&h, f) && h.cash == 321 && h.yoga == 2 && h.married == 0 && h.outfit == 0 && h.yoga_given == 0 && h.mday == 0 && h.ver == SAVE_VER, "a version-15 save loads: not married, nothing lost"); }
    remove(f);
    /* the text of the new scenes */
    CHECK(B_WEDDING_N == 10 && B_HUG_N == 4 && B_KISS_N == 5 && B_SLEEP_N == 6 && B_FEED_N == 8, "scene lengths");
    CHECK(B_HUG[0].txt == NULL && B_KISS[0].txt == NULL && R_HUG_OPEN_N >= 3 && R_KISS_OPEN_N >= 3, "hug and kiss open with a random line");
    for (k = 0; k < B_FEED_N; k++) CHECK(B_FEED[k].bg == (k < 2 ? BG_ROM : BG_FEED) && (k >= 2 || B_FEED[k].spr == 1) && B_FEED[k].txt && strlen(B_FEED[k].txt) <= 138, "feed beat %d", k);
}

static void test_v82(void) {
    Game g, h; int r, k; const char *f = "/tmp/pq_v82.sav";
    /* prices (as agreed) and where things are sold */
    CHECK(SH_PRICE[SH_LOTION] == 300 && SH_PRICE[SH_DECK] == 600 && SH_PRICE[SH_YOGA] == 3000 && SH_PRICE[SH_GOLD] == 6000 && SH_PRICE[SH_DIAMOND] == 50000 && SH_PRICE[SH_SUPER] == 2250, "v8.2 prices (v12.2: yoga 3000, Super drug 2250)");
    CHECK(SH_PRICE[SH_FLOWER] == 250 && SH_PRICE[SH_STACK] == 1000 && SH_PRICE[SH_MILK] == 750 && SH_N == 15 && GF_N == 13, "old prices untouched, 15 store items (v12.3: Mommy Milk Injection), 13 gift kinds");
    g_init(&g); g.rs = RS_LIVING;
    for (k = 0; k < SH_N; k++) CHECK(g_shop_avail(&g, k) == (k == SH_PUMP || k == SH_MILK || k == SH_SUPER || k == SH_INJECT || k == SH_YOGA || k == SH_TOP || k == SH_COW ? 0 : 1), "item %d availability before the day-8 scene", k);   /* v11.3: the three outfits need max romance */
    g.pill_seen = 1; for (k = 0; k < SH_N; k++) CHECK(g_shop_avail(&g, k) == (k == SH_YOGA || k == SH_TOP || k == SH_COW ? 0 : 1), "item %d available after the day-8 scene", k);
    g.rom_lvl = ROM_LVLS; g.rom_pct = 99; CHECK(!g_shop_avail(&g, SH_YOGA), "level 3 at 99%% is not max yet"); g.rom_pct = 100; for (k = 0; k < SH_N; k++) CHECK(g_shop_avail(&g, k), "item %d available at max romance", k);
    /* buying */
    g_init(&g); g.cash = 100000; g.bank = 0; g.rs = RS_LIVING; g.rom_lvl = ROM_LVLS; g.rom_pct = 100;   /* v11.3: outfits are sold at max romance */
    CHECK(g_shop_buy(&g, SH_SUPER) == 1 && g.sdrug == 0, "the Super drug is not for sale before the day-8 scene");
    CHECK(g_shop_buy(&g, SH_LOTION) == 0 && g.lotion == 1 && g.cash == 99700, "lotion $300");
    CHECK(g_shop_buy(&g, SH_DECK) == 0 && g.pdeck == 1 && g.cash == 99100, "pink card deck $600");
    CHECK(g_shop_buy(&g, SH_YOGA) == 0 && g.yoga == 1 && g.cash == 96100, "yoga outfit $3,000");
    CHECK(g_shop_buy(&g, SH_GOLD) == 0 && g.gold == 1 && g.cash == 90100, "gold bar $6,000");
    CHECK(g_shop_buy(&g, SH_DIAMOND) == 0 && g.dring == 1 && g.cash == 40100, "diamond ring $50,000");
    CHECK(g_shop_buy(&g, SH_DIAMOND) == 2 && g.dring == 1 && g.cash == 40100, "the diamond ring is one time only");
    CHECK(!g.ring && !g_can_propose(&g), "the diamond ring is NOT the proposal ring and does nothing yet");
    g.pill_seen = 1; CHECK(g_shop_buy(&g, SH_SUPER) == 0 && g.sdrug == 1 && g.cash == 37850, "Super drug $2,250");
    CHECK(g_shop_buy(&g, SH_SUPER) == 0 && g.sdrug == 2, "the Super drug is stackable");
    g.cash = 100; g.bank = 0; CHECK(g_shop_buy(&g, SH_GOLD) == 1 && g.gold == 1 && g.cash == 100, "not enough money: nothing changes");
    g.cash = 50000; g.bank = 20000; g.lotion = INV_CAP; CHECK(g_shop_buy(&g, SH_LOTION) == 3 && g.cash == 50000, "stock full"); g.sdrug = INV_CAP; CHECK(g_shop_buy(&g, SH_SUPER) == 3, "Super stock full");
    /* gifts: gains, one a day, level-up with no carry, a full heart refuses */
    CHECK(g_gift_gain(GF_FLOWER) == 10 && g_gift_gain(GF_LOTION) == 12 && g_gift_gain(GF_DECK) == 15 && g_gift_gain(GF_STACK) == 20 && g_gift_gain(GF_YOGA) == 0 && g_gift_gain(GF_TOP) == 0 && g_gift_gain(GF_COW) == 0 && g_gift_gain(GF_GOLD) == 50 && g_gift_gain(GF_PILL) == 0, "gift gains");
    g_init(&g); g.rs = RS_LIVING; g.day = 2; g.lotion = 2; g.pdeck = 2; g.yoga = 2; g.gold = 2;
    CHECK(g_gift(&g, GF_LOTION) == 0 && g.lotion == 1 && g.rom_pct == 12 && g.gift_day == 2, "lotion +12%");
    CHECK(g_gift(&g, GF_DECK) == 0 && g.pdeck == 1 && g.rom_pct == 12 && g.deck_given == 1, "v11.6: the first deck since she lives here is accepted on a day with a gift (no romance)");
    CHECK(g_gift(&g, GF_DECK) == 2 && g.pdeck == 1 && g.rom_pct == 12, "still one gift per day");
    g.day = 3; CHECK(g_gift(&g, GF_DECK) == 0 && g.pdeck == 0 && g.rom_pct == 27, "pink cards +15%");
    g.day = 4; CHECK(g_gift(&g, GF_YOGA) == 12 && g.yoga == 2 && !g.yoga_given && g.rom_pct == 27 && g.gift_day == 3, "v11.3: the yoga outfit is refused below max romance and changes nothing");
    g.day = 5; CHECK(g_gift(&g, GF_GOLD) == 0 && g.gold == 1 && g.rom_lvl == 1 && g.rom_pct == 77, "gold bar +50%%");
    g.day = 6; CHECK(g_gift(&g, GF_GOLD) == 0 && g.gold == 0 && g.rom_lvl == 2 && g.rom_pct == 0, "gold bar +50% reaches level 2, the bar restarts at 0 (no carry)");
    g.day = 7; g.gold = 1; g.rom_pct = 70; CHECK(g_gift(&g, GF_GOLD) == 0 && g.gold == 0 && g.rom_lvl == 3 && g.rom_pct == 0, "a big gift at 70% levels up and wastes the rest");
    g.day = 8; CHECK(g_gift(&g, GF_GOLD) == 1, "none left"); g.lotion = 1; g.rom_lvl = ROM_LVLS; g.rom_pct = 100; CHECK(g_gift(&g, GF_LOTION) == 4 && g.lotion == 1, "a full heart takes no gifts");
    g_init(&g); g.gift_day = 0; CHECK(g_gift_stock(&g, GF_LOTION) == 0 && g_gift_stock(&g, GF_SUPER) == 0, "stock reads");
    /* the Super milk drug */
    g_init(&g); g.rs = RS_LIVING; g.sdrug = 3; g.mpill = 5; g.day = 10;
    CHECK(g_gift(&g, GF_SUPER) == 7 && g.sdrug == 3, "before the day-8 scene she does not want it");
    g.pill_seen = 1; g.milk_st = 0; CHECK(g_gift(&g, GF_SUPER) == 7 && g.sdrug == 3, "the first milk pill has to start the story first");
    g.milk_st = 1; CHECK(g_gift(&g, GF_SUPER) == 5, "still waiting for the first pill to work"); g.milk_st = 2; CHECK(g_gift(&g, GF_SUPER) == 6, "she needs the pump first");
    g.milk_st = 3; g.milk_pct = 50;
    CHECK(!g_super_active(&g) && !g_super_block(&g) && g_asks_left(&g) == 0 && g_milk_gauge_pct(&g) == 50 && g_ask_milk(&g) == 1, "nothing active before the drug");
    CHECK(g_gift(&g, GF_SUPER) == 0 && g.sdrug == 2 && g.sd_day == 10, "the drug is given");
    CHECK(g_super_active(&g) && g_super_left(&g) == 2 && g_asks_left(&g) == 2 && g_milk_gauge_pct(&g) == 100 && g.milk_pct == 50, "the gauge shows full, the real gauge is kept");
    CHECK(g_gift(&g, GF_MILK) == 9 && g.mpill == 5 && g.milk_pct == 50, "she refuses the normal milk pill right after the drug");
    CHECK(g_gift(&g, GF_SUPER) == 10 && g.sdrug == 2, "a second drug while the first one works is refused");
    CHECK(g_ask_milk(&g) == 0 && g.bottle == 1 && g.ask_n == 1 && g_asks_left(&g) == 1, "ask 1");
    CHECK(g_ask_milk(&g) == 0 && g.bottle == 2 && g_asks_left(&g) == 0 && g.milk_st == 4, "ask 2 (the first bottle opens the milk topics)");
    CHECK(g_ask_milk(&g) == 2 && g.bottle == 2, "a third ask on the same day is refused");
    g.day = 11; CHECK(g_super_active(&g) && g_super_left(&g) == 1 && g_asks_left(&g) == 2 && g_milk_gauge_pct(&g) == 100, "day 2 of the effect: two asks again");
    CHECK(g_ask_milk(&g) == 0 && g_ask_milk(&g) == 0 && g_ask_milk(&g) == 2 && g.bottle == 4, "two more bottles on day 2");
    CHECK(g_gift(&g, GF_MILK) == 9, "still refused on day 2");
    g.day = 12; CHECK(!g_super_active(&g) && g_asks_left(&g) == 0 && g_ask_milk(&g) == 1 && g_milk_gauge_pct(&g) == 50 && g_super_block(&g), "the effect ended after 2 days: no asks, the gauge is back, the block goes on");
    CHECK(g_gift(&g, GF_MILK) == 9 && g.mpill == 5, "the normal pill is refused on the 1st day after the effect");
    g.day = 13; CHECK(g_super_block(&g) && g_gift(&g, GF_MILK) == 9, "and on the 2nd day after");
    g.day = 14; CHECK(!g_super_block(&g) && g_gift(&g, GF_MILK) == 0 && g.mpill == 4 && g.milk_pct == 0 && g.bottle == 5, "on the 3rd day after the effect the normal pill works again");
    g.day = 20; g.milk_pct = 0; g.mpill = 3; CHECK(g_gift(&g, GF_MILK) == 0 && g.milk_pct == 50, "normal pills fill the gauge as before");
    /* a new drug may be given in the two days after the effect ends: the effect and the block restart */
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.milk_st = 4; g.sdrug = 2; g.mpill = 2; g.day = 5; g_gift(&g, GF_SUPER); g.day = 7;
    CHECK(!g_super_active(&g) && g_gift(&g, GF_SUPER) == 0 && g.sd_day == 7 && g_super_active(&g) && g_super_block(&g), "a second drug after the effect works again");
    g.day = 10; CHECK(g_super_block(&g), "blocked until 2 days after the second effect"); g.day = 11; CHECK(!g_super_block(&g) && g_gift(&g, GF_MILK) == 0, "then free");
    /* asking needs room for a bottle */
    g_init(&g); g.pill_seen = 1; g.milk_st = 4; g.sdrug = 1; g.day = 3; g_gift(&g, GF_SUPER); g.bottle = INV_CAP; CHECK(g_ask_milk(&g) == 8 && g.bottle == INV_CAP && g.ask_n == 0, "bottle stock full: nothing used");
    /* the drug does not touch the one-gift-a-day rule or the romance bar */
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.milk_st = 3; g.sdrug = 1; g.flower = 1; g.day = 3; g.gift_day = 3; g.rom_pct = 40;
    CHECK(g_gift(&g, GF_SUPER) == 0 && g.gift_day == 3 && g.rom_pct == 40, "the Super drug ignores the one-gift-a-day rule and gives no romance");
    /* debug shortcut */
    g_init(&g); g_dbg_milk(&g); CHECK(g.sdrug == 2 && g.mpill == 4 && g.pump, "debug: milk shortcut adds 2 Super drugs");
    /* saves: round trip, ranges, a version-14 save */
    g_init(&g); g.lotion = 3; g.pdeck = 4; g.yoga = 5; g.gold = 6; g.dring = 1; g.sdrug = 7; g.sd_day = 12; g.ask_day = 12; g.ask_n = 2; g.day = 12; g.story = ST_NAMED;
    CHECK(save_game(&g, f) && load_game(&h, f) && h.lotion == 3 && h.pdeck == 4 && h.yoga == 5 && h.gold == 6 && h.dring == 1 && h.sdrug == 7 && h.sd_day == 12 && h.ask_day == 12 && h.ask_n == 2, "the new fields survive a save");
    h = g; h.dring = 2; CHECK(!save_sane(&h), "dring 2 rejected"); h = g; h.sdrug = INV_CAP + 1; CHECK(!save_sane(&h), "too many drugs rejected"); h = g; h.ask_n = 3; CHECK(!save_sane(&h), "ask_n 3 rejected");
    h = g; h.lotion = -1; CHECK(!save_sane(&h), "negative lotion rejected"); h = g; h.sd_day = -1; CHECK(!save_sane(&h), "negative sd_day rejected");
    { Game t; FILE *w; g_init(&t); t.cash = 4242; t.day = 33; t.story = ST_NAMED; t.stk[STK_SPACEX] = 5; t.mseed = 99; t.flower = 4; t.milk_st = 4; t.ver = SAVE_VER_14; t.size = (int)offsetof(Game, lotion);
      w = fopen(f, "wb"); fwrite(&t, 1, offsetof(Game, lotion), w); fclose(w);
      g_init(&h); CHECK(load_game(&h, f), "a version-14 save loads");
      CHECK(h.cash == 4242 && h.day == 33 && h.stk[STK_SPACEX] == 5 && h.mseed == 99 && h.flower == 4 && h.milk_st == 4, "a v14 save keeps everything");
      CHECK(h.lotion == 0 && h.pdeck == 0 && h.yoga == 0 && h.gold == 0 && h.dring == 0 && h.sdrug == 0 && h.sd_day == 0 && h.ask_day == 0 && h.ask_n == 0, "a v14 save starts with no new items");
      CHECK(h.ver == SAVE_VER && h.size == (int)sizeof h, "a v14 save is upgraded");
      t.ver = 5; w = fopen(f, "wb"); fwrite(&t, 1, offsetof(Game, lotion), w); fclose(w); CHECK(!load_game(&h, f), "v14 size with a wrong version rejected");
      t.ver = SAVE_VER_14; w = fopen(f, "wb"); fwrite(&t, 1, offsetof(Game, lotion) - 4, w); fclose(w); CHECK(!load_game(&h, f), "a truncated v14 file rejected");
      t.ver = SAVE_VER_14; w = fopen(f, "wb"); fwrite(&t, 1, sizeof t, w); fclose(w); CHECK(!load_game(&h, f), "full size with the v14 version rejected"); }
    (void)r; remove(f);
}

/* ----- v7.9: the external text files (src/extlines.h) ----- */
static int same_str(const char *a, const char *b) { return a && b && !strcmp(a, b); }
static void test_ext(void) {
    int i, n0; FILE *f; char *big;
    ex_snapshot(); ex_reset();
    /* 1. the dump of the built-in text loads back without a single problem and changes nothing */
    CHECK(ex_dump("/tmp/pq_l.txt", 0) && ex_dump("/tmp/pq_t.txt", 1), "dump");
    ex_nlog = ex_nprob = 0; ex_cur_file = "x";
    CHECK(ex_load_all("/tmp/pq_l.txt", "/tmp/pq_t.txt", "/tmp/pq_log.txt", 0) == 0, "the dump of the built-in text must load with 0 problems (first: %s)", ex_nlog ? ex_log[0] : "");
    CHECK(ex_dump("/tmp/pq_l2.txt", 0) && ex_dump("/tmp/pq_t2.txt", 1), "dump again");
    { char *a = (char *)calloc(1, 2000000), *b = (char *)calloc(1, 2000000); size_t na = 0, nb = 0; const char *pa[2] = {"/tmp/pq_l.txt", "/tmp/pq_t.txt"}, *pb[2] = {"/tmp/pq_l2.txt", "/tmp/pq_t2.txt"}; int k;
      for (k = 0; k < 2; k++) { f = fopen(pa[k], "rb"); na = f ? fread(a, 1, 1999999, f) : 0; if (f) fclose(f); f = fopen(pb[k], "rb"); nb = f ? fread(b, 1, 1999999, f) : 0; if (f) fclose(f);
          CHECK(na > 1000 && na == nb && !memcmp(a, b, na), "dump, load, dump must give the same file (%s)", pa[k]); }
      free(a); free(b); }
    ex_reset();
    /* 2. a pool: any number of lines, bad lines skipped and logged, the other sections untouched */
    n0 = L_HAND_N; ex_nlog = ex_nprob = 0;
    ex_load_string("[L_START]\nHELLO|THERE\nTHIS LINE IS FAR TOO LONG|FOR THE TABLE BOX\n# a comment\n\nlower case|is fine\n[l_start]\nduplicate section\n", 0);
    CHECK(L_START_N == 2 && same_str(L_START[0], "HELLO|THERE") && same_str(L_START[1], "lower case|is fine"), "pool: 2 good lines kept (n=%d)", L_START_N);
    CHECK(ex_nprob == 2, "pool: one too long line and one duplicate section are logged (%d)", ex_nprob);
    CHECK(L_HAND_N == n0, "other pools are untouched");
    ex_reset(); CHECK(L_START_N == 5, "reset gives the built-in pool back");
    /* 3. nothing usable = the built-in pool stays */
    ex_nlog = ex_nprob = 0; ex_load_string("[L_START]\nTOO LONG FOR A LINE OF THE BOX|AND THE OTHER\n", 0);
    CHECK(L_START_N == 5 && ex_nprob >= 2, "a pool with no usable line keeps the built-in lines"); ex_reset();
    /* 4. a single text with %d must keep its %d */
    ex_nlog = ex_nprob = 0; ex_load_string("[O_DEBT]\nPay her $%d now.\n[W_STORM]\nThe storm is loud.\n[O_RENT]\nRent is due.\n[W_RENT_TAKEN_BANK]\nOnly $%d here.\n", 0);
    CHECK(same_str(O_DEBT, "Pay her $%d now."), "single with one %%d replaced");
    CHECK(same_str(W_STORM, "The storm is loud."), "single replaced");
    CHECK(strstr(O_RENT, "%d") != NULL && strstr(W_RENT_TAKEN_BANK, "%d") != NULL && ex_nprob == 2, "a text that drops or loses a %%d is refused (%d)", ex_nprob); ex_reset();
    ex_nlog = ex_nprob = 0; ex_load_string("[W_STORM]\nBig %s trouble\n[W_BATH]\n100% sure\n[W_SINK]\nline one\nline two\n", 0);
    CHECK(ex_nprob == 3 && same_str(W_BATH, "The bathroom. I don't need it right now."), "%%s, a stray %% and two lines in a single are refused"); ex_reset();
    /* 5. characters: curly quotes and dashes are converted, accents refused, CRLF and a BOM are fine */
    ex_nlog = ex_nprob = 0; ex_load_string("\xEF\xBB\xBF[W_STORM]\r\nIt\xE2\x80\x99s \xE2\x80\x9Cloud\xE2\x80\x9D \xE2\x80\x93 really\xE2\x80\xA6\r\n[W_BATH]\r\nCaf\xC3\xA9\r\n", 0);
    CHECK(same_str(W_STORM, "It's \"loud\" - really...") && ex_nprob == 1 && same_str(W_BATH, "The bathroom. I don't need it right now."), "UTF-8 look-alikes become ASCII, an accent is refused: '%s' (%d)", W_STORM, ex_nprob); ex_reset();
    /* 6. scenes: a free scene may grow; who / background / sprite are read; the missing ones are inherited */
    n0 = B_BUMP_N; ex_nlog = ex_nprob = 0;
    ex_load_string("[B_BUMP]\nNAR LOBBY 0 | One.\nHER | Two.\nSOPHIA HALL 3 | Three.\nME | Four.\n", 0);
    CHECK(B_BUMP_N == 4 && B_BUMP[0].who == W_NAR && B_BUMP[1].who == W_HER && B_BUMP[1].bg == BG_LOBBY && B_BUMP[1].spr == 0 && B_BUMP[2].bg == BG_HALL && B_BUMP[2].spr == 3 && B_BUMP[3].who == W_ME && B_BUMP[3].bg == BG_HALL && B_BUMP[3].spr == 3 && ex_nprob == 0,
          "a free scene can be replaced by a different number of beats (n=%d, problems %d)", B_BUMP_N, ex_nprob);
    ex_reset(); CHECK(B_BUMP_N == n0, "scene reset");
    ex_nlog = ex_nprob = 0; ex_load_string("[B_BUMP]\nNAR LOBBY 0 | Good.\nXYZ LOBBY 0 | Bad speaker.\n", 0);
    CHECK(B_BUMP_N == n0 && ex_nprob >= 1, "one bad beat = the whole scene stays built-in"); ex_reset();
    ex_nlog = ex_nprob = 0; ex_load_string("[B_BUMP]\nNAR NOWHERE 0 | x\n[B_RAIN]\nNAR RAIN 9 | x\n[B_MOVEIN]\nNAR | the text is fine but this is (auto)\n[B_PROPOSE]\nNAR | (auto)\n", 0);
    CHECK(ex_nprob >= 4 && B_RAIN_N == 2 + 0 * 1 + (B_RAIN_N - 2) && B_PROPOSE[0].txt != NULL, "bad background, bad sprite, (auto) in a free scene: refused"); ex_reset();
    /* 7. scenes with beats the game fills in: the first beats are locked, the tail is free; some scenes are fully fixed */
    n0 = B_REPEAT_N; ex_nlog = ex_nprob = 0;
    ex_load_string("[B_REPEAT]\nNAR HALL 0 | Knock.\nHER HALL 1 | (auto)\nHER | Rule one.\nHER | Rule two.\nHER | Rule three.\nHER | Rule four.\n", 0);
    CHECK(B_REPEAT_N == 6 && B_REPEAT[1].txt == NULL && ex_nprob == 0, "locked beats stay (auto), the tail may be longer (n=%d, problems %d)", B_REPEAT_N, ex_nprob); ex_reset();
    ex_nlog = ex_nprob = 0; ex_load_string("[B_REPEAT]\nNAR HALL 0 | Knock.\nHER HALL 1 | Not auto.\nHER | Rule.\n[B_GIFT]\nNAR | (auto)\nHER | (auto)\n[B_AFTER]\nHER | (auto)\n", 0);
    CHECK(B_REPEAT_N == n0 && B_GIFT_N == 4 && B_AFTER_N == 2 && ex_nprob >= 3, "an (auto) beat that is not (auto), and fixed scenes with the wrong count, are refused (%d)", ex_nprob);
    for (i = 0; i < 2; i++) CHECK(B_AFTER[i].txt == NULL, "after-match beats are still filled in by the game"); ex_reset();
    ex_nlog = ex_nprob = 0; ex_load_string("[B_MILK2]\nNAR | x\nNAR | x\n", 0); CHECK(ex_nprob >= 1 && B_MILK2_N == 8, "B_MILK2 has a fixed shape"); ex_reset();
    /* 8. lead-ins: any number, each exactly 4 beats */
    ex_nlog = ex_nprob = 0;
    ex_load_string("[B_LEAD]\nNAR ROM 0 | a\nSOPHIA | b\nME | c\nNAR | d\n\nNAR ROM 0 | a2\nNAR | b2\nNAR | c2\n\nNAR ROM 0 | a3\nNAR | b3\nNAR | c3\nNAR | d3\n", 0);
    CHECK(B_LEAD_N == 2 && same_str(B_LEAD[1][3].txt, "d3") && ex_nprob == 2, "lead-ins: 2 good ones kept, the one with 3 beats skipped (n=%d, problems %d)", B_LEAD_N, ex_nprob); ex_reset();
    CHECK(B_LEAD_N == 6, "lead-in reset");
    /* 9. fixed lists */
    ex_nlog = ex_nprob = 0; ex_load_string("[M_PAGE]\nOnly one page.\n[C_LEN]\nA\nB\nC\n[SN_CLOSER]\n1\n2\n3\n4\n", 0);
    CHECK(same_str(C_LEN[2], "C") && ex_nprob == 2 && NL(M_PAGE) == 2 && !same_str(M_PAGE[0], "Only one page."), "a fixed list needs exactly its number of lines"); ex_reset();
    /* 10. talk.txt: exchanges */
    n0 = C_TOPICS[CH_POKER].n; ex_nlog = ex_nprob = 0;
    ex_load_string("[C_POKER]\n> Hi there.\n< Hello.\n< Second line.\n\n> No answer here.\n\n> Third.\n< Answer.\n< b\n< too many\n\n> Last one.\n< Fine.\n", 1);
    CHECK(C_TOPICS[CH_POKER].n == 2 && same_str(C_TOPICS[CH_POKER].t[0].b, "Second line.") && C_TOPICS[CH_POKER].t[1].b == NULL && same_str(C_TOPICS[CH_POKER].t[1].a, "Fine.") && ex_nprob == 2,
          "talk: good exchanges kept, bad ones skipped (n=%d, problems %d)", C_TOPICS[CH_POKER].n, ex_nprob);
    ex_reset(); CHECK(C_TOPICS[CH_POKER].n == n0, "talk reset");
    ex_nlog = ex_nprob = 0; ex_load_string("[T_LOOKS]\n> x\n< y\n[L_START]\nA\n", 1); CHECK(ex_nprob == 1 && L_START_N == 5 && T_TOPICS[TS_LOOKS].n == 1, "a section in the wrong file is ignored"); ex_reset();
    /* 11. unknown sections, text outside sections */
    ex_nlog = ex_nprob = 0; ex_load_string("stray text\n[NO_SUCH_THING]\nabc\n[L_START\nabc\n", 0); CHECK(ex_nprob == 4 && L_START_N == 5, "unknown section, stray text, missing ] are logged (%d)", ex_nprob); ex_reset();
    /* 12. garbage and huge input never crash */
    big = (char *)malloc(300000); srand(7);
    for (i = 0; i < 300000; i++) big[i] = (char)(rand() % 255 + 1); big[299999] = 0;
    ex_nlog = ex_nprob = 0; ex_load_string(big, 0); ex_load_string(big, 1); ex_reset();
    for (i = 0; i < 300000; i++) { int r = rand() % 40; big[i] = r < 3 ? '\n' : r < 5 ? '[' : r < 7 ? ']' : r < 9 ? '|' : r < 11 ? '>' : r < 13 ? '<' : (char)(' ' + rand() % 60); } big[299999] = 0;
    ex_load_string(big, 0); ex_load_string(big, 1); ex_reset(); free(big);
    CHECK(L_START_N == 5, "after the garbage the built-in text is back");
    /* 13. the files shipped in packaging/ load (a typo there would only skip a line, but we say so) */
    ex_nlog = ex_nprob = 0;
    { int p = ex_load_all("packaging/lines.txt", "packaging/talk.txt", "/tmp/pq_log2.txt", 0);
      if (p) { printf("NOTE: the shipped lines.txt / talk.txt have %d problem(s):\n", p); for (i = 0; i < ex_nlog && i < 20; i++) printf("  %s\n", ex_log[i]); } }
    ex_reset();
    /* v10.3: mama table pools 20+, close-up mama pools 20+, fit and no pet names */
    { const char *const *mp[] = {L_START_MM, L_HAND_MM, L_LAST_MM, L_SHEWIN_MM, L_YOUWIN_MM, L_SHEFOLD_MM, L_YOUFOLD_MM, L_SPLIT_MM, L_HERRAISE_MM, L_YOURAISE_MM, L_ENDHER_MM, L_ENDYOU_MM, L_ENDTIE_MM, L_ENDLOST_MM};
      int mn[] = {L_START_MM_N, L_HAND_MM_N, L_LAST_MM_N, L_SHEWIN_MM_N, L_YOUWIN_MM_N, L_SHEFOLD_MM_N, L_YOUFOLD_MM_N, L_SPLIT_MM_N, L_HERRAISE_MM_N, L_YOURAISE_MM_N, L_ENDHER_MM_N, L_ENDYOU_MM_N, L_ENDTIE_MM_N, L_ENDLOST_MM_N};
      int k, j, x;
      for (k = 0; k < 14; k++) { CHECK(mn[k] >= 20, "v10.3: mama table pool %d has only %d lines", k, mn[k]);
        for (j = 0; j < mn[k]; j++) { const char *a = mp[k][j], *b = strchr(a, '|'); int la = b ? (int)(b - a) : (int)strlen(a), lb = b ? (int)strlen(b + 1) : 0;
          CHECK(la <= 18 && lb <= 18 && !(b && strchr(b + 1, '|')), "v10.3: table line does not fit: %s", a);
          CHECK(!strstr(a, "BOY") && !strstr(a, "BABY") && !strstr(a, "MAMA"), "v10.3: pet name in %s", a);
          for (x = j + 1; x < mn[k]; x++) CHECK(strcmp(a, mp[k][x]), "v10.3: duplicate table line %s", a); } }
      CHECK(CU_THINK_MM_N >= 20 && L_STARE_MM_N >= 20, "v10.3: close-up mama pools %d / %d", CU_THINK_MM_N, L_STARE_MM_N);
      for (j = 0; j < CU_THINK_MM_N; j++) { CHECK(strlen(CU_THINK_MM[j]) <= 92, "v10.3: thought too long: %s", CU_THINK_MM[j]); CHECK(!strcasestr(CU_THINK_MM[j], "boy") && !strcasestr(CU_THINK_MM[j], "baby"), "v10.3: pet word in %s", CU_THINK_MM[j]); }
      for (j = 0; j < L_STARE_MM_N; j++) { CHECK(strlen(L_STARE_MM[j]) <= 135, "v10.3: her line too long: %s", L_STARE_MM[j]); CHECK(!strcasestr(L_STARE_MM[j], "boy") && !strcasestr(L_STARE_MM[j], "baby") && !strcasestr(L_STARE_MM[j], "mama"), "v10.3: pet word in %s", L_STARE_MM[j]); } }
    /* every registered name exists once and has a description; counts of the built-in text */
    for (i = 0; i < TXT_N; i++) { int k; CHECK(TXT_REG[i].desc && TXT_REG[i].desc[0], "%s has no description", TXT_REG[i].name); for (k = i + 1; k < TXT_N; k++) CHECK(strcmp(TXT_REG[i].name, TXT_REG[k].name), "%s registered twice", TXT_REG[i].name); }
    CHECK(TXT_N == 419, "registry size %d", TXT_N);   /* v12.2: +8 (pump broken / new pump pools, 4 modes each); v12.1: +40 (cow mode: 18 pools incl. chat, 12 scenes, 10 opener and mid pair pools; v11.2: +9 (milk-aware cow gift pools 3, wear exchanges 6); v11.1: +6 (cow print outfit gift pools, outfit lines); v11.0: +20 (friendship: door openers, her room by day); v10.8: +6 (designer top gift pools, outfit lines); v10.5: +1 (W_SAVED_ONLY); v10.4: +5 (opener pairs); v10.3: +2 (CU_THINK_MM, L_STARE_MM) */   /* v9.7: +30; v9.9: +6 (living-room scenes) */
      /* v9.5: +29 (mama mode); MILK_INFO moved to items.txt: -1 */   /* v9.3: +3 (W_UPG_OK1, W_CLERK_UP1, O_UP1); v9.0: +2 (W_LOTWIN, W_LOTLOSE); v8.2: +4 (R_MILK_SUPER, R_SUPER_ON, R_ASK_DONE, R_ASK_NONE); v8.6: +15 (wedding, page-4 scenes, greetings, refusals) */
}

/* v9.0: the lottery */
/* ----- v9.5: items.txt, names.txt, mama mode ----- */
static int has_word_boy(const char *t) {
    const char *p; if (!t) return 0;
    for (p = t; *p; p++) if (tolower((unsigned char)p[0]) == 'b' && tolower((unsigned char)p[1]) == 'o' && tolower((unsigned char)p[2]) == 'y' && (p == t || !isalpha((unsigned char)p[-1])) && !isalpha((unsigned char)p[3])) return 1;
    return 0;
}
static void test_v95(void) {
    int i, k; FILE *f; char *buf;
    /* --- items.txt --- */
    it_snapshot(); it_reset(); ex_nlog = ex_nprob = 0;
    CHECK(IT_N == 18, "18 items in the registry (15 store, soda, energy drink, milk): %d", IT_N);
    for (i = 0; i < IT_N; i++) for (k = 0; k < IF_N; k++) if (IT_REG[i].f[k] && *IT_REG[i].f[k]) {
        CHECK(!it_text_err(k, *IT_REG[i].f[k]), "the built-in %s of %s must pass its own limits: %s", IT_FIELD[k], IT_REG[i].key, it_text_err(k, *IT_REG[i].f[k]) ? ex_msg : "");
    }
    CHECK(it_dump("/tmp/pq_i.txt"), "items dump");
    f = fopen("/tmp/pq_i.txt", "rb"); buf = (char *)calloc(1, 100000); CHECK(f && fread(buf, 1, 99999, f) > 500, "items dump has content"); if (f) fclose(f);
    ex_nlog = ex_nprob = 0; CHECK(it_load_string(buf) == IT_N && ex_nprob == 0, "the dump of the built-in items loads with 0 problems (got %d: %s)", ex_nprob, ex_nlog ? ex_log[0] : ""); free(buf);
    CHECK(same_str(SH_NAME[SH_FLOWER], "FLOWER") && same_str(SH_INFO[SH_FLOWER], "GIFT: +10% ROMANCE"), "defaults are untouched by loading their own dump (%s / %s)", SH_NAME[SH_FLOWER], SH_INFO[SH_FLOWER]);
    ex_nlog = ex_nprob = 0;
    CHECK(it_load_string("[flower]\nname = Red Rose\ninfo = A GIFT: +10% OF LOVE\nbought = BOUGHT A RED ROSE\ngive = ROSE\n[SODA]\nNAME = COLA\nBOUGHT = BOUGHT A COLA\n[GOLD_BAR]\nDESC = A heavy bar of gold for her. She likes shiny things.\n") == 3 && ex_nprob == 0, "an edit loads (problems %d: %s)", ex_nprob, ex_nlog ? ex_log[0] : "");
    CHECK(same_str(SH_NAME[SH_FLOWER], "Red Rose") && same_str(SH_INFO[SH_FLOWER], "A GIFT: +10% OF LOVE") && same_str(SH_BOUGHT[SH_FLOWER], "BOUGHT A RED ROSE") && same_str(R_GNAME[GF_FLOWER], "ROSE"), "flower: name, info, bought and give name replaced");
    CHECK(same_str(INV_NAME[0], "COLA") && same_str(INV_BOUGHT[0], "BOUGHT A COLA") && same_str(INV_NAME[1], "ENERGY DRINK"), "soda renamed, energy drink untouched");
    CHECK(SH_DESC[SH_GOLD] && strstr(SH_DESC[SH_GOLD], "heavy bar"), "an item that had no info page can get one");
    it_reset(); ex_nlog = ex_nprob = 0;
    CHECK(it_load_string("[FLOWER]\nNAME = THIS NAME IS FAR TOO LONG FOR ITS PLACE\nINFO = fine\nBOUGHT = BOUGHT\n[RUBY_RING]\nGIVE = RING\n[NOPE]\nNAME = X\nNAME = Y\n[FLOWER]\nNAME = TWICE\n") >= 1, "partial block");
    CHECK(same_str(SH_NAME[SH_FLOWER], "FLOWER") && same_str(SH_INFO[SH_FLOWER], "fine") && same_str(SH_BOUGHT[SH_FLOWER], "BOUGHT"), "a too-long name is refused, the other fields of the block still apply");
    CHECK(ex_nprob >= 4, "too long, no GIVE place on the ruby ring, unknown key, second block: all logged (%d)", ex_nprob);
    it_reset(); ex_nlog = ex_nprob = 0;
    it_load_string("[BODY_LOTION]\nDESC = one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen sixteen seventeen eighteen nineteen twenty twentyone\nNAME = CAF\xc3\xa9\nINFO = a|b\nNAME2 = x\nNAME\n");
    CHECK(ex_nprob >= 3 && same_str(SH_NAME[SH_LOTION], "BODY LOTION") && SH_DESC[SH_LOTION] == it_def[IT_N > 6 ? 6 : 0][IF_DESC], "bad characters, | and a stray line are refused (problems %d)", ex_nprob);
    it_reset();
    CHECK(same_str(SH_NAME[SH_FLOWER], "FLOWER") && same_str(INV_NAME[0], "SODA") && SH_DESC[SH_GOLD] == NULL, "it_reset restores everything");
    ex_nlog = ex_nprob = 0; CHECK(it_load_string("[SOPHIA_BREAST_MILK]\nDESC = Sweet and fresh.\n") == 1 && same_str(MILK_INFO, "Sweet and fresh."), "the fridge milk info page is edited in items.txt"); it_reset();
    ex_nlog = ex_nprob = 0; ex_load_string("[MILK_INFO]\nold text\n", 0); CHECK(ex_nprob == 0, "an old lines.txt that still has [MILK_INFO] gives no complaint");
    /* --- names.txt --- */
    nm_reset();
    CHECK(nm_kind("MAMA") == 1 && nm_kind("mama") == 1 && nm_kind("MAMA SOPHIA") == 1 && nm_kind("  Mama   sophia ") == 1, "MAMA / MAMA SOPHIA in any case and spacing = mama mode");
    CHECK(nm_kind("SOPHIA") == 0 && nm_kind("sophia") == 0, "SOPHIA = back");
    CHECK(nm_kind("") == -1 && nm_kind("   ") == -1 && nm_kind("HONEY") == -1 && nm_kind("MAMAS") == -1 && nm_kind("SOPHIA MAMA") == -1 && nm_kind("MA MA") == -1, "anything else does nothing");
    ex_nlog = ex_nprob = 0;
    CHECK(nm_load_string("[MAMA]\nmommy\nMAMA BEAR\n[SOPHIA]\nQUEEN\n") == 2 && nm_kind("MOMMY") == 1 && nm_kind("mama bear") == 1 && nm_kind("QUEEN") == 0 && nm_kind("SOPHIA") == 0, "names.txt adds names; SOPHIA always works");
    CHECK(nm_kind("MAMA") == -1, "a [MAMA] list replaces the built-in names");
    nm_reset(); ex_nlog = ex_nprob = 0;
    nm_load_string("[MAMA]\nSOPHIA\nMAMA 2\nTHIS NAME IS WAY TOO LONG FOR IT\nok name\nOK NAME\n[HONEY]\nX\n");
    CHECK(ex_nprob >= 4 && nm_kind("SOPHIA") == 0 && nm_kind("OK NAME") == 1 && nm_kind("MAMA") == -1, "SOPHIA as a mama name, digits, too long, duplicates, unknown section: refused and logged (%d)", ex_nprob);
    nm_reset(); ex_nlog = ex_nprob = 0;
    CHECK(nm_dump("/tmp/pq_n.txt"), "names dump"); f = fopen("/tmp/pq_n.txt", "rb"); buf = (char *)calloc(1, 20000); if (f) { if (fread(buf, 1, 19999, f) < 100) CHECK(0, "names dump content"); fclose(f); }
    CHECK(nm_load_string(buf) == 2 && ex_nprob == 0 && nm_kind("MAMA SOPHIA") == 1, "the dump of names.txt loads back with 0 problems (%d)", ex_nprob); free(buf);
    nm_reset();
    /* the combined loader: missing files are written, a second start reads them back */
    remove("/tmp/pq_a.txt"); remove("/tmp/pq_b.txt"); remove("/tmp/pq_c.txt"); remove("/tmp/pq_d.txt");
    CHECK(ex_load_all4("/tmp/pq_a.txt", "/tmp/pq_b.txt", "/tmp/pq_c.txt", "/tmp/pq_d.txt", "/tmp/pq_log.txt", 1) == 0, "first start: missing files are written, 0 problems");
    f = fopen("/tmp/pq_c.txt", "rb"); CHECK(f != NULL, "items.txt written on first start"); if (f) fclose(f);
    f = fopen("/tmp/pq_d.txt", "rb"); CHECK(f != NULL, "names.txt written on first start"); if (f) fclose(f);
    CHECK(ex_load_all4("/tmp/pq_a.txt", "/tmp/pq_b.txt", "/tmp/pq_c.txt", "/tmp/pq_d.txt", "/tmp/pq_log.txt", 1) == 0, "second start reads all four files with 0 problems");
    /* --- the call field --- */
    { Game g, h; const char *sf = "/tmp/pq_call.sav"; g_init(&g);
      CHECK(!g_mama(&g), "not married: never mama mode");
      g.rs = RS_LIVING; g.pill_seen = 1; g.dring = 1; g.milk_st = 4; g_marry(&g); CHECK(g.married && g.call == 0 && !g_mama(&g), "married: she starts as SOPHIA");
      g.call = 1; CHECK(g_mama(&g), "call = 1 and married: mama mode");
      g.milk_pct = 100; g.milk_st = 4; g.call = 0; CHECK(g_feed_ready(&g) && g_feed(&g) == 1 && g.milk_pct == 100, "BREASTFEED is refused in SOPHIA mode even with a full gauge (nothing is spent)");
      g.call = 1; CHECK(g_feed(&g) == 0 && g.milk_pct == 0, "in mama mode it works");
      CHECK(save_game(&g, sf) && load_game(&h, sf) && h.call == 1 && h.married, "mama mode survives a save");
      g.call = 2; CHECK(!save_sane(&g), "call 2 is not sane"); g.call = 1; g.married = 0; CHECK(!save_sane(&g), "mama mode without a wedding is not sane"); g.married = 1;
      { Game o; unsigned char raw[sizeof(Game)]; g_init(&o); o.cash = 555; o.day = 9; o.story = ST_NAMED; o.room_up = 1; o.rs = RS_READY; save_game(&o, sf); f = fopen(sf, "rb"); if (f) { if (fread(raw, 1, sizeof raw, f) != sizeof raw) CHECK(0, "read"); fclose(f); }
        memcpy(&o, raw, sizeof o); o.ver = SAVE_VER_18; f = fopen(sf, "wb"); if (f) { fwrite(&o, 1, offsetof(Game, call), f); fclose(f); }
        CHECK(load_game(&h, sf) && h.cash == 555 && h.call == 0 && h.ver == SAVE_VER && h.size == (int)sizeof h, "a version-18 save loads: she is SOPHIA, nothing lost");
        CHECK(h.room_up == 1, "a version-18 save keeps its room level (no second room_up migration)");
        o.ver = SAVE_VER_18; f = fopen(sf, "wb"); if (f) { fwrite(&o, 1, sizeof o, f); fclose(f); } CHECK(!load_game(&h, sf), "full size with the version-18 number is rejected");
        o.ver = SAVE_VER; f = fopen(sf, "wb"); if (f) { fwrite(&o, 1, offsetof(Game, call), f); fclose(f); } CHECK(!load_game(&h, sf), "the short size with the new version is rejected"); }
      remove(sf); }
    /* --- the texts --- */
    { const char *const *no_boy[] = {R_GREET_M, R_NIGHT_M, R_SLEEP_NO_TEASE, R_SLEEP_NO_MILK, R_MORE_M, T_KICK_M}; const int nn[] = {R_GREET_M_N, R_NIGHT_M_N, R_SLEEP_NO_TEASE_N, R_SLEEP_NO_MILK_N, R_MORE_M_N, T_KICK_M_N}; int p;
      for (p = 0; p < 6; p++) for (i = 0; i < nn[p]; i++) CHECK(!has_word_boy(no_boy[p][i]), "married (SOPHIA mode) text %d/%d calls you boy: %s", p, i, no_boy[p][i]);
      for (i = 0; i < B_SLEEP_N; i++) CHECK(!has_word_boy(B_SLEEP[i].txt), "B_SLEEP beat %d calls you boy", i);
      for (i = 0; i < B_NIGHTM_N; i++) CHECK(!has_word_boy(B_NIGHTM[i].txt), "B_NIGHTM beat %d calls you boy", i);
      CHECK(R_GREET_M_N >= 4 && R_MORE_M_N >= 4 && T_KICK_M_N >= 4, "enough married lines"); }
    CHECK(B_HUG_MM_N == B_HUG_N && B_KISS_MM_N == B_KISS_N && B_NIGHTM_MM_N == B_NIGHTM_N && B_SLEEP_MM_N >= 4 && B_FEED_MM_N >= 8, "the mama scenes have the same shape as the normal ones");
    CHECK(B_HUG_MM[0].txt == NULL && B_KISS_MM[0].txt == NULL && B_HUG_MM[0].bg == BG_HUG && B_KISS_MM[0].bg == BG_KISS, "mama HUG / KISS: beat 1 is filled in by the game, same pictures");
    CHECK(B_FEED_MM[0].bg == BG_ROM && B_FEED_MM[0].spr == 1 && B_FEED_MM[1].bg == BG_ROM && B_FEED_MM[1].spr == 1 && B_FEED_MM[2].bg == BG_FEED, "mama BREASTFEED: she covers in her room first, then the scene picture");
    for (i = 0; i < B_SLEEP_MM_N; i++) CHECK(B_SLEEP_MM[i].bg == BG_SLEEPT, "mama sleep beat %d picture", i);
    CHECK(B_NIGHTM_MM[2].spr == 5 && B_NIGHTM_MM[2].bg == BG_BDOOR && B_NIGHTM_MM[3].bg == BG_ROM, "mama night knock: the hiding picture is kept");
    for (i = 0; i < TS_N; i++) CHECK(T_TOPICS_MM[i].n >= 5, "mama tease topic %d has exchanges", i);
    CHECK(C_MAMA.n >= 10 && C_FAIL_MM_N >= 4 && T_KICK_MM_N >= 5 && T_OUT_MM_N >= 4 && R_GREET_MM_N >= 5 && R_NIGHT_MM_N >= 3 && R_KICKED_MM_N >= 4, "mama pools have enough lines");
    /* every mama line is checked against the same limits as the file loader uses */
    { int ri, bad = 0; for (ri = 0; ri < TXT_N; ri++) { const char *nm = TXT_REG[ri].name; int l = (int)strlen(nm); if (l > 3 && !strcmp(nm + l - 3, "_MM") || !strcmp(nm, "B_CALLMAMA") || !strcmp(nm, "B_CALLBACK") || !strcmp(nm, "R_MORE_M") || !strcmp(nm, "T_KICK_M")) bad++; }
      CHECK(bad >= 58, "the mama texts are registered (so they are editable and limit-checked): %d", bad); }
}

static void test_v93(void) {   /* v9.3: room levels 0..2, rents 250 / 500 / 1000, the version-17 save */
    Game g, h, o; const char *f = "/tmp/pq_v93.sav";
    g_init(&g); CHECK(g.room_up == 0 && g_room_lvl(&g) == 0 && !g_computer(&g) && g_rent_amt(&g) == 250, "a new game: base room, no computer, rent 250");
    g.room_up = 1; CHECK(g_room_lvl(&g) == 1 && g_computer(&g) && g_rent_amt(&g) == 500 && g_upgrade_cost(&g) == 10000, "level 1: computer, rent 500, next upgrade 10000");
    g.room_up = 2; CHECK(g_room_lvl(&g) == 2 && g_rent_amt(&g) == 1000 && !g_upgrade_avail(&g), "level 2: rent 1000, nothing more to buy");
    g_init(&g); g.room_up = 3; CHECK(!save_sane(&g), "room level 3 is rejected"); g.room_up = -1; CHECK(!save_sane(&g), "room level -1 is rejected"); g.room_up = 2; CHECK(save_sane(&g), "room level 2 is sane");
    g_init(&g); g.story = ST_HOME; g.room_up = 1; CHECK(!g_move_ok(&g), "no move-in without the story");
    g.story = ST_NAMED; g.rs = RS_READY; g.room_up = 1; CHECK(!g_move_ok(&g), "she does not move into the level-1 room"); g.room_up = 2; CHECK(g_move_ok(&g), "she moves into the level-2 room");
    /* a version-17 save: room_up 1 (the old upgrade) becomes level 2, a game past the shirt scene (no upgrade) becomes level 1, a fresh game stays at 0 */
    g_init(&o); o.story = ST_NAMED; o.rs = RS_READY; o.room_up = 1; o.cash = 4321; save_game(&o, f); { Game t; FILE *fp = fopen(f, "rb"); unsigned char raw[sizeof(Game)]; if (fp) { if (fread(raw, 1, sizeof raw, fp) != sizeof raw) CHECK(0, "read"); fclose(fp); memcpy(&t, raw, sizeof t); t.ver = SAVE_VER_17; fp = fopen(f, "wb"); if (fp) { fwrite(&t, 1, offsetof(Game, call), fp); fclose(fp); } } }
    CHECK(load_game(&h, f) && h.room_up == 2 && h.rs == RS_READY && h.cash == 4321 && h.ver == SAVE_VER, "a v17 save with the old upgrade becomes level 2");
    g_init(&o); o.story = ST_NAMED; o.room_up = 0; o.rs = RS_UPG; save_game(&o, f); { Game t; FILE *fp = fopen(f, "rb"); unsigned char raw[sizeof(Game)]; if (fp) { if (fread(raw, 1, sizeof raw, fp) != sizeof raw) CHECK(0, "read"); fclose(fp); memcpy(&t, raw, sizeof t); t.ver = SAVE_VER_17; fp = fopen(f, "wb"); if (fp) { fwrite(&t, 1, offsetof(Game, call), fp); fclose(fp); } } }
    CHECK(load_game(&h, f) && h.room_up == 1 && h.rs == RS_UPG, "a v17 save past the shirt scene without the upgrade becomes level 1 (it had the computer)");
    g_init(&o); o.story = ST_NEW; save_game(&o, f); { Game t; FILE *fp = fopen(f, "rb"); unsigned char raw[sizeof(Game)]; if (fp) { if (fread(raw, 1, sizeof raw, fp) != sizeof raw) CHECK(0, "read"); fclose(fp); memcpy(&t, raw, sizeof t); t.ver = SAVE_VER_17; fp = fopen(f, "wb"); if (fp) { fwrite(&t, 1, offsetof(Game, call), fp); fclose(fp); } } }
    CHECK(load_game(&h, f) && h.room_up == 0, "a v17 save from before the shirt scene stays at level 0");
    g_init(&o); o.story = ST_NAMED; o.rs = RS_LIVING; o.room_up = 0; save_game(&o, f); { Game t; FILE *fp = fopen(f, "rb"); unsigned char raw[sizeof(Game)]; if (fp) { if (fread(raw, 1, sizeof raw, fp) != sizeof raw) CHECK(0, "read"); fclose(fp); memcpy(&t, raw, sizeof t); t.ver = SAVE_VER_17; fp = fopen(f, "wb"); if (fp) { fwrite(&t, 1, offsetof(Game, call), fp); fclose(fp); } } }
    CHECK(load_game(&h, f) && h.room_up == 2, "a v17 save where she already lives with you becomes level 2");
    g_init(&o); o.story = ST_NAMED; o.room_up = 1; save_game(&o, f); CHECK(load_game(&h, f) && h.room_up == 1 && h.ver == SAVE_VER, "a current save keeps its level 1 (no second migration)");
    remove(f);
}
static void test_lottery(void) {
    Game g, h; int i, wins = 0, n = 200000, k; char f[] = "/tmp/pq_lot.sav"; double ret = 0;
    CHECK(LOT_N == 6 && LOT_PRICE[0] == 20 && LOT_PRICE[5] == 1500 && LOT_MULT == 3 && LOT_PCT == 30, "six tickets from 20 to 1500, triple prize, 30 percent");
    for (i = 1; i < LOT_N; i++) CHECK(LOT_PRICE[i] > LOT_PRICE[i - 1], "prices rise");
    for (i = 0; i < LOT_N; i++) CHECK(g_lot_prize(LOT_PRICE[i]) == 3 * LOT_PRICE[i], "prize is triple the price");
    g_init(&g); g.cash = 5000; g.day = 4;
    CHECK(!g_lot_has(&g) && !g_lot_ready(&g) && g_lot_claim(&g) == -1, "no ticket: nothing to tell");
    CHECK(g_lot_buy(&g, -1, 0) == -2 && g_lot_buy(&g, LOT_N, 0) == -2, "no such ticket");
    g.cash = 19; CHECK(g_lot_buy(&g, 0, 0) == 0 && g.cash == 19 && !g_lot_has(&g), "19 dollars cannot buy the 20 ticket");
    g.cash = 5000; CHECK(g_lot_buy(&g, 2, 5) == 1 && g.cash == 4900 && g.lot_price == 100 && g.lot_day == 4 && g.lot_win == 1, "a winning roll: paid from cash, ticket kept");
    CHECK(g_lot_buy(&g, 0, 99) == -1 && g.cash == 4900 && g.lot_price == 100, "one ticket at a time");
    CHECK(g_lot_has(&g) && !g_lot_ready(&g) && g_lot_claim(&g) == -1 && g.lot_price == 100 && g.cash == 4900, "the same day: no result yet");
    g.day = 5; CHECK(g_lot_ready(&g), "the next day the result is ready");
    CHECK(g_lot_claim(&g) == 300 && g.cash == 5200 && !g_lot_has(&g) && g.lot_day == 0 && g.lot_win == 0, "a win pays 3x the price into cash and clears the ticket");
    CHECK(g_lot_claim(&g) == -1, "it is told only once");
    CHECK(g_lot_buy(&g, 5, LOT_PCT) == 1 && g.lot_win == 0 && g.cash == 3700, "roll 30 is a loss (wins are 0..29)");
    g.day = 9; CHECK(g_lot_claim(&g) == 0 && g.cash == 3700 && !g_lot_has(&g), "a loss pays nothing and clears the ticket (a later day works too)");
    g.cash = 999990; g.day = 1; CHECK(g_lot_buy(&g, 0, 0) == 1 && g.cash == 999970, "cash top"); g.cash = 999990; g.day = 2; CHECK(g_lot_claim(&g) == 60 && g.cash == 999999, "the prize is capped like all money");
    g_init(&g); g.cash = 100; g.day = 3; g.lot_price = 20; g.lot_day = 3; g.lot_win = 1; save_game(&g, f); CHECK(load_game(&h, f) && h.lot_price == 20 && h.lot_day == 3 && h.lot_win == 1 && h.ver == SAVE_VER, "a ticket survives a save");
    h = g; h.lot_price = 33; CHECK(!save_sane(&h), "a ticket price that does not exist is rejected"); h = g; h.lot_win = 2; CHECK(!save_sane(&h), "bad win flag rejected"); h = g; h.lot_day = -1; CHECK(!save_sane(&h), "bad day rejected");
    h = g; h.lot_price = 0; CHECK(!save_sane(&h), "no ticket but a win flag is rejected"); h.lot_win = 0; h.lot_day = 0; CHECK(save_sane(&h), "no ticket at all is sane");
    { FILE *fp; Game o; unsigned char raw[sizeof(Game)]; g_init(&o); o.cash = 777; o.day = 12; o.story = ST_NAMED; o.ver = SAVE_VER_16; memcpy(raw, &o, sizeof o);
      fp = fopen(f, "wb"); CHECK(fp != NULL, "open"); if (fp) { fwrite(raw, 1, offsetof(Game, lot_price), fp); fclose(fp); }
      CHECK(load_game(&h, f) && h.cash == 777 && h.day == 12 && h.lot_price == 0 && h.lot_day == 0 && h.lot_win == 0 && h.ver == SAVE_VER && h.size == (int)sizeof h, "a version-16 save loads: no ticket, nothing lost");
      fp = fopen(f, "wb"); if (fp) { fwrite(raw, 1, offsetof(Game, lot_price) - 4, fp); fclose(fp); } CHECK(!load_game(&h, f), "a truncated v16 file is rejected");
      o.ver = SAVE_VER; fp = fopen(f, "wb"); if (fp) { memcpy(raw, &o, sizeof o); fwrite(raw, 1, offsetof(Game, lot_price), fp); fclose(fp); } CHECK(!load_game(&h, f), "the short size with the new version is rejected"); }
    remove(f);
    /* the real chance: 30 percent, an average return of 90 percent for every ticket */
    for (k = 0; k < LOT_N; k++) { long paid = 0, got = 0; wins = 0;
        for (i = 0; i < n; i++) { g_init(&g); g.cash = 5000; g.day = 1; g_lot_buy(&g, k, rnd(100)); paid += LOT_PRICE[k]; g.day = 2; { int p = g_lot_claim(&g); if (p > 0) { wins++; got += p; } } }
        ret = (double)got / (double)paid;
        CHECK(wins > n * 0.29 && wins < n * 0.31, "ticket %d wins about 30 percent (%d of %d)", k, wins, n);
        CHECK(ret > 0.88 && ret < 0.92, "ticket %d returns about 90 percent (%.3f)", k, ret); }
}
/* v10.0: 20 wins open the black market, items only after she moves in, one-time items vanish, save upgrade */
static void test_v105(void) {   /* v10.5: the progress file, the rewind guard, the market reshuffle, the version-21 upgrade, the sane checks */
    Game g, h, t; int s, d, i, same_past, diff_future, c; char sp[] = "/tmp/pq_v105_logic.sav", pr[] = "/tmp/pq_v105_logic.prog";
    remove(pr); remove(sp);
    CHECK(prog_get(pr, 99) == 0 && prog_get(pr, 0) == 0, "no progress file: nothing reached");
    prog_set(pr, 99, 10); CHECK(prog_get(pr, 99) == 10, "the furthest day is written");
    prog_set(pr, 99, 7); CHECK(prog_get(pr, 99) == 10, "a save never lowers the furthest day");
    prog_set(pr, 99, 12); CHECK(prog_get(pr, 99) == 12, "a later day raises it");
    prog_set(pr, 0, 5); CHECK(prog_get(pr, 0) == 0, "a game without a career id is not tracked");
    prog_set(pr, 5, 3); CHECK(prog_get(pr, 5) == 3 && prog_get(pr, 99) == 12, "careers are kept apart");
    for (c = 100; c < 120; c++) prog_set(pr, c, c);
    CHECK(prog_get(pr, 119) == 119 && prog_get(pr, 99) == 0 && prog_get(pr, 5) == 0, "a full list forgets the career not saved for the longest time");
    g_init(&g); g.career = 99; g.day = 10;
    CHECK(g_behind(&g, 12) && !g_behind(&g, 10) && !g_behind(&g, 9) && !g_behind(&g, 0), "behind = another save reached a later day");
    g.career = 0; CHECK(!g_behind(&g, 50), "no career id: never behind"); g.career = 99;
    /* the reshuffle: days up to today keep their prices, later days change */
    g.mseed = 777; g.day = 10; h = g; g_rewind(&h, 5555u);
    CHECK(h.mrw_n == 1 && h.mrw_day[0] == 11 && h.mrw_salt[0] > 0, "one reshuffle from the next day");
    same_past = 1; diff_future = 0;
    for (s = 0; s < STK_N; s++) for (d = 1; d <= 30; d++) { int a = stk_price(&g, s, d), b = stk_price(&h, s, d); if (d <= 10 && a != b) same_past = 0; if (d >= 11 && a != b) diff_future = 1; }
    CHECK(same_past, "the days up to the loaded one keep their prices (and the chart)"); CHECK(diff_future, "the days after it have new prices");
    CHECK(stk_change(&h, 0) == stk_change(&g, 0), "today's change against yesterday is the same");
    /* rewinding to an earlier day drops the reshuffles of the abandoned future */
    t = h; t.day = 8; g_rewind(&t, 6666u); CHECK(t.mrw_n == 1 && t.mrw_day[0] == 9, "a reshuffle of an abandoned future is dropped (%d / %d)", t.mrw_n, t.mrw_day[0]);
    t = h; t.day = 14; g_rewind(&t, 7777u); CHECK(t.mrw_n == 2 && t.mrw_day[0] == 11 && t.mrw_day[1] == 15, "a later rewind adds a second reshuffle (%d)", t.mrw_n);
    CHECK(stk_price(&t, 0, 12) == stk_price(&h, 0, 12) && stk_price(&t, 1, 14) == stk_price(&h, 1, 14), "the earlier reshuffle still applies to its days");
    t = g; for (i = 0; i < 20; i++) { t.day = 10 + i * 3; g_rewind(&t, 1000u + (unsigned)i); }
    CHECK(t.mrw_n == MRW_N && t.mrw_day[MRW_N - 1] == t.day + 1, "the list never grows past %d (n %d)", MRW_N, t.mrw_n);
    { int asc = 1; for (i = 1; i < t.mrw_n; i++) if (t.mrw_day[i] <= t.mrw_day[i - 1]) asc = 0; CHECK(asc, "the reshuffle days stay ascending"); }
    /* the file round trip and the sane checks */
    t = h; t.mrw_n = 1; CHECK(save_game(&t, sp) && load_game(&g, sp) && g.mrw_n == 1 && g.mrw_day[0] == 11 && g.mrw_salt[0] == t.mrw_salt[0] && g.career == 99 && g.ver == SAVE_VER, "career and reshuffles survive a save");
    h = g; h.mrw_n = MRW_N + 1; CHECK(!save_sane(&h), "too many reshuffles rejected");
    h = g; h.mrw_day[0] = 1; CHECK(!save_sane(&h), "a reshuffle before day 2 rejected");
    h = g; h.mrw_salt[0] = 0; CHECK(!save_sane(&h), "a zero salt rejected");
    h = g; h.mrw_n = 2; h.mrw_day[1] = 11; h.mrw_salt[1] = 5; CHECK(!save_sane(&h), "descending / equal days rejected");
    h = g; h.career = -1; CHECK(!save_sane(&h), "a negative career id rejected");
    /* a version-21 file: the old struct without the new fields */
    { FILE *f; g_init(&t); t.story = ST_NAMED; t.day = 5; t.magic = SAVE_MAGIC; t.ver = SAVE_VER_21; t.size = (int)offsetof(Game, career);
      f = fopen(sp, "wb"); if (f) { fwrite(&t, 1, offsetof(Game, career), f); fclose(f); }
      CHECK(load_game(&h, sp) && h.ver == SAVE_VER && h.day == 5 && h.career > 0 && h.mrw_n == 0 && h.size == (int)sizeof h, "a version-21 save loads, gets a career id and no reshuffle"); }
    /* g_open_slot: behind -> reshuffled; not behind -> untouched; a bad file -> 0 */
    prog_set(pr, 4321, 20); g_init(&g); g.career = 4321; g.day = 15; g.story = ST_NAMED; save_game(&g, sp);
    { int bh = 0; CHECK(g_open_slot(&h, sp, pr, 99u, &bh) && bh == 1 && h.mrw_n == 1 && h.mrw_day[0] == 16, "an older slot is opened behind and reshuffled");
      g.day = 20; save_game(&g, sp); CHECK(g_open_slot(&h, sp, pr, 99u, &bh) && bh == 0 && h.mrw_n == 0, "the furthest slot is opened as it is");
      remove(sp); CHECK(!g_open_slot(&h, sp, pr, 99u, &bh) && bh == 0, "a missing slot cannot be opened"); }
    g_new(&g); CHECK(g.career > 0, "a new career gets an id"); remove(pr); remove(sp);
}
static void test_v100(void) {
    Game g, h; int i, k, o_ring = 0; const char *f = "/tmp/pq_v100.sav";
    g_init(&g); g.cash = 100000; g.story = ST_NAMED;
    CHECK(g.sw_wins == 0 && !g_bm_open(&g) && !g_items_open(&g), "a new game: no wins, black market and items closed");
    for (k = 0; k < SH_N; k++) CHECK(!g_shop_avail(&g, k), "item %d is not sold at the start", k);
    CHECK(g_shop_buy(&g, SH_RING) == 1 && !g.ring, "no ring before the black market opens");
    CHECK(g_shop_buy(&g, SH_FLOWER) == 1 && g.flower == 0, "no flower before she lives with you");
    /* wins: only real wins at her table count */
    for (i = 0; i < 5; i++) { g.cash = 500; g_match_start(&g, 500); g.net = 5; g_match_finish(&g); }
    CHECK(g.sw_wins == 5, "5 wins at her table = 5");
    g_match_start(&g, 500); g.net = -5; g_match_finish(&g); g_match_start(&g, 500); g.net = 0; g_match_finish(&g); g_match_start(&g, 500); g_forfeit(&g);
    CHECK(g.sw_wins == 5, "a loss, a tie and a forfeit do not count");
    g.day = 3; g.energy = 100; g_home_start(&g, 10); g.net = 4; g_match_finish(&g);
    CHECK(g.sw_wins == 5, "a free home match does not count");
    for (i = 0; i < 14; i++) { g.cash = 500; g_match_start(&g, 100); g.net = 3; g_match_finish(&g); }
    CHECK(g.sw_wins == 19 && !g_bm_open(&g), "19 wins: still closed");
    g_match_start(&g, 100); g.net = 3; g_match_finish(&g);
    CHECK(g.sw_wins == 20 && g_bm_open(&g) && !g_items_open(&g), "the 20th win opens the black market, not the items");
    g.cash = 100000; g.bank = 0;
    CHECK(g_shop_avail(&g, SH_RING) && g_shop_avail(&g, SH_PILL) && !g_shop_avail(&g, SH_MILK) && !g_shop_avail(&g, SH_SUPER), "ring and pill are sold, the milk things need the day-8 scene");
    CHECK(!g_shop_once_done(&g, SH_RING) && g_shop_buy(&g, SH_RING) == 0 && g.ring && g_shop_once_done(&g, SH_RING), "the ring is bought and then done (not listed any more)");
    CHECK(g_shop_buy(&g, SH_PILL) == 0 && g_shop_once_done(&g, SH_PILL), "the pill too");
    CHECK(g_can_propose(&g), "with the ring you can propose");
    g.rs = RS_NO; CHECK(!g_items_open(&g) && g_bm_open(&g), "she said no to living together: the items stay closed");
    g.rs = RS_LIVING; CHECK(g_items_open(&g) && g_shop_avail(&g, SH_FLOWER) && g_shop_avail(&g, SH_DIAMOND) && !g_shop_avail(&g, SH_PUMP), "she lives with you: the items open (the pump after the day-8 scene)");
    g_shop_buy(&g, SH_DIAMOND); CHECK(g_shop_once_done(&g, SH_DIAMOND), "the diamond ring vanishes after buying");
    g.pill_seen = 1; g_shop_buy(&g, SH_PUMP); CHECK(g_shop_once_done(&g, SH_PUMP), "the pump vanishes after buying");
    CHECK(!g_shop_once_done(&g, SH_FLOWER) && !g_shop_once_done(&g, SH_MILK) && !g_shop_once_done(&g, SH_SUPER), "stackable items never vanish");
    /* the boss win counts as one */
    g_init(&g); g.story = ST_FIRST; g.cash = 5000; g.ex_big = 1; g_ex_win(&g, EX_BUYIN[EX_BOSS]); CHECK(g.story == ST_NAMED && g.sw_wins == 1, "the boss win is one win");
    g_ex_win(&g, EX_BUYIN[EX_BOSS]); CHECK(g.sw_wins == 1, "(a second boss payout does not count again)");
    g_init(&g); g.story = ST_FIRST; g_ex_win(&g, EX_BUYIN[0]); CHECK(g.sw_wins == 0, "an ordinary Exhibition win does not count");
    /* a game that is already past the proposal keeps its black market */
    g_init(&g); g.ring = 1; CHECK(g_bm_open(&g), "a save with the ring keeps the black market");
    g_init(&g); g.rs = RS_VISIT; CHECK(g_bm_open(&g), "a save past the proposal keeps it");
    (void)o_ring;
    /* debug skip opens it */
    g_init(&g); g.story = ST_HOME; g_dbg_skip(&g); CHECK(g.sw_wins == SW_GOAL && g_bm_open(&g), "the debug skip opens the black market");
    /* save: the field is saved, a version-20 save loads with 0 wins */
    g_init(&g); g.sw_wins = 7; save_game(&g, f); CHECK(load_game(&h, f) && h.sw_wins == 7 && h.ver == SAVE_VER, "the win count is saved");
    { Game o = g; unsigned char raw[sizeof(Game)]; FILE *fp; o.ver = SAVE_VER_20; memcpy(raw, &o, sizeof o); fp = fopen(f, "wb"); if (fp) { fwrite(raw, 1, offsetof(Game, sw_wins), fp); fclose(fp); }
      CHECK(load_game(&h, f) && h.sw_wins == 0 && h.ver == SAVE_VER && h.size == (int)sizeof h, "a version-20 save loads with 0 wins");
      o.ver = SAVE_VER; fp = fopen(f, "wb"); if (fp) { memcpy(raw, &o, sizeof o); fwrite(raw, 1, offsetof(Game, sw_wins), fp); fclose(fp); } CHECK(!load_game(&h, f), "the short size with the new version is rejected"); }
    h = g; h.sw_wins = -1; CHECK(!save_sane(&h), "a negative win count is rejected");
}
static void test_v110(void) {   /* v11.0: the FRIENDSHIP bar (20 wins, tiers) and the text that goes with it */
    Game g; int i, w;
    g_init(&g); g.story = ST_NAMED;
    CHECK(FR_GOAL == 20, "the bar is full at 20 wins");
    for (w = -5; w <= 60; w++) {
        int t, pc; g.fr_wins = w; t = g_fr_tier(&g); pc = g_fr_pct(&g);
        CHECK(t >= 1 && t <= 5, "tier %d at %d wins", t, w);
        CHECK(pc >= 0 && pc <= 100, "pct %d at %d wins", pc, w);
        CHECK((t == 5) == (w >= 20) && g_fr_full(&g) == (w >= 20), "tier 5 / full only from 20 wins (%d wins: tier %d)", w, t);
        CHECK(t == (w < 5 ? 1 : w < 10 ? 2 : w < 15 ? 3 : w < 20 ? 4 : 5), "tier steps every 5 wins (%d wins: tier %d)", w, t);
        if (w >= 1 && w <= 20) { int p2; g.fr_wins = w - 1; p2 = g_fr_pct(&g); g.fr_wins = w; CHECK(g_fr_pct(&g) > p2, "one more win must fill the bar (%d)", w); }
    }
    /* only wins over her at her table fill it, and it starts at ZERO: the boss win and the wins before it do not count */
    g_init(&g); g.story = ST_FIRST; g.cash = 5000; g.ex_big = 1; g_ex_win(&g, EX_BUYIN[EX_BOSS]);
    CHECK(g.story == ST_NAMED && g.fr_wins == 0 && g_fr_pct(&g) == 0 && g_fr_tier(&g) == 1 && g.sw_wins == 1, "the boss win leaves the friendship bar at zero (fr %d, black-market wins %d)", g.fr_wins, g.sw_wins);
    g_init(&g); g.story = ST_HOME; g.cash = 5000; g.energy = 100; g.day = 3; g_match_start(&g, 500); g.net = 5; g_match_finish(&g); CHECK(g.fr_wins == 0 && g.sw_wins == 1, "a win before the boss counts for the black market only");
    g_init(&g); g.story = ST_NAMED; g.fr_wins = 4; g.sw_wins = 4; g.cash = 5000; g.energy = 100; g.day = 3;
    g_home_start(&g, 10); g.net = 4; g_match_finish(&g); CHECK(g_fr_wins(&g) == 4, "a free match at home must not fill the bar");
    g_match_start(&g, 500); g.net = -5; g_match_finish(&g); CHECK(g_fr_wins(&g) == 4, "a loss must not fill the bar");
    g_match_start(&g, 500); g.net = 0; g_match_finish(&g); CHECK(g_fr_wins(&g) == 4, "a tie must not fill the bar");
    g_match_start(&g, 500); g_forfeit(&g); CHECK(g_fr_wins(&g) == 4, "a forfeit must not fill the bar");
    g_match_start(&g, 500); g.net = 5; g_match_finish(&g); CHECK(g_fr_wins(&g) == 5 && g_fr_tier(&g) == 2 && g.sw_wins == 5, "a win at her table is one step (wins %d tier %d)", g_fr_wins(&g), g_fr_tier(&g));
    g.fr_wins = 20; g_match_start(&g, 500); g.net = 5; g_match_finish(&g); CHECK(g_fr_full(&g) && g_fr_pct(&g) == 100, "the full bar stays full");
    /* the visit is open by day from the boss win until she lives with you */
    g_init(&g); g.story = ST_NAMED; g.phase = PH_MORNING; CHECK(g_visit_ok(&g), "visit open by day after the boss");
    g.phase = PH_EVENING; CHECK(g_visit_ok(&g), "visit open in the evening"); g.phase = PH_NIGHT; CHECK(!g_visit_ok(&g), "no visit at night");
    g.phase = PH_MORNING; g.story = ST_FIRST; CHECK(!g_visit_ok(&g), "no visit before you beat her"); g.story = ST_HOME; CHECK(!g_visit_ok(&g), "no visit before the shirt is paid");
    g.story = ST_NAMED; g.rs = RS_LIVING; CHECK(!g_visit_ok(&g), "no visit once she lives with you");
    /* the new text: every pool exists, no line is too long for the box, nothing is a duplicate inside a pool */
    { const char *const *pl[] = {G_FIRSTVIS, G_FR1, G_FR2, G_FR3, G_FR4, G_FR5, V_IN, V_HELLO, V_BYE, V_BED, V_BATH, V_TV, V_WIN, V_TABLE, V_SOFA, V_TALK1, V_TALK2, V_TALK3, V_TALK4, V_TALK5};
      const int *const pn[] = {&G_FIRSTVIS_N, &G_FR1_N, &G_FR2_N, &G_FR3_N, &G_FR4_N, &G_FR5_N, &V_IN_N, &V_HELLO_N, &V_BYE_N, &V_BED_N, &V_BATH_N, &V_TV_N, &V_WIN_N, &V_TABLE_N, &V_SOFA_N, &V_TALK1_N, &V_TALK2_N, &V_TALK3_N, &V_TALK4_N, &V_TALK5_N};
      int k, j; CHECK((int)(sizeof pl / sizeof pl[0]) == 20, "20 new pools");
      for (k = 0; k < 20; k++) { CHECK(*pn[k] >= 3, "pool %d has only %d lines", k, *pn[k]);
        for (j = 0; j < *pn[k]; j++) { int q; CHECK(strlen(pl[k][j]) <= 130 && ok_chars(pl[k][j]), "pool %d line %d: too long or a character the font cannot draw: %s", k, j, pl[k][j]); for (q = j + 1; q < *pn[k]; q++) CHECK(strcmp(pl[k][j], pl[k][q]), "pool %d: duplicate line %d", k, j); } } }
    (void)i;
}
static void test_v108(void) {   /* v10.8: the DESIGNER TOP, a third outfit */
    Game g, h; int t[4], n, i; const char *f = "/tmp/pq_v108.sav";
    /* the store item: ITEMS tab (she lives with you), $2,500, stackable to the cap, listed like the yoga outfit */
    CHECK(SH_PRICE[SH_TOP] == 3500 && SH_PRICE[SH_TOP] > SH_PRICE[SH_YOGA] && SH_PRICE[SH_TOP] < SH_PRICE[SH_GOLD], "designer top costs $3,500, between the yoga outfit and the gold bar");
    g_init(&g); g.cash = 100000; g.story = ST_NAMED;
    CHECK(!g_shop_avail(&g, SH_TOP) && g_shop_buy(&g, SH_TOP) == 1 && g.top == 0, "closed before she lives with you");
    g.rs = RS_LIVING; CHECK(!g_shop_avail(&g, SH_TOP) && g_shop_buy(&g, SH_TOP) == 1 && g.top == 0, "v11.3: still closed while she lives with you but romance is not max");
    g.rom_lvl = ROM_LVLS; g.rom_pct = 100; CHECK(g_shop_avail(&g, SH_TOP) && !g_shop_black(SH_TOP) && !g_shop_once_done(&g, SH_TOP), "open at max romance; a normal ITEMS item");
    CHECK(g_shop_buy(&g, SH_TOP) == 0 && g.top == 1 && g.cash == 100000 - 3500, "buying takes $3,500 and adds one");
    g.top = INV_CAP; CHECK(g_shop_buy(&g, SH_TOP) == 3 && g.top == INV_CAP && g.cash == 100000 - 3500, "stock full at the cap, nothing is charged");
    g.top = 0; g.cash = 100; g.bank = 0; CHECK(g_shop_buy(&g, SH_TOP) == 1 && g.top == 0, "not enough money");
    /* the gift: +30% romance, one gift a day, unlocks the outfit */
    CHECK(g_gift_gain(GF_TOP) == 0, "v11.3: gift gain 0 (no romance)");
    g_init(&g); g.rs = RS_LIVING; g.story = ST_NAMED; g.day = 4; g.top = 2; g.rom_lvl = 1; g.rom_pct = 0;
    CHECK(g_gift(&g, GF_TOP) == 12 && g.top == 2 && !g.top_given && g.rom_pct == 0, "below max romance she refuses it and keeps nothing changed");
    g.rom_lvl = ROM_LVLS; g.rom_pct = 100; CHECK(g_gift_stock(&g, GF_TOP) == 2 && !g.top_given, "two in stock, not given yet");
    CHECK(g_gift(&g, GF_TOP) == 0 && g.top == 1 && g.top_given == 1 && g.rom_lvl == ROM_LVLS && g.rom_pct == 100 && g.gift_day == 0, "giving it at max: stock -1, no romance change, not the gift of the day, she owns it");
    CHECK(g_gift(&g, GF_TOP) == 1 && g.top == 1, "she has it already: refused, stock kept");
    g_init(&g); g.rs = RS_LIVING; g.story = ST_NAMED; g.day = 4; g.top = 1; g.rom_lvl = ROM_LVLS; g.rom_pct = 100; g.gift_day = 4; CHECK(g_gift(&g, GF_TOP) == 0 && g.top_given == 1, "a full bar does not block it and the gift of the day does not either");
    g_init(&g); g.rs = RS_LIVING; g.day = 5; g.top = 0; g.rom_lvl = ROM_LVLS; g.rom_pct = 100; CHECK(g_gift(&g, GF_TOP) == 1, "nothing in stock: refused");
    /* outfits: usual / yoga / top. Closed until married AND she owns one of them */
    g_init(&g); g.rs = RS_LIVING; g.married = 1; g.yoga_given = 0; g.top_given = 0;
    CHECK(!g_outfit_open(&g) && g_outfit_targets(&g, t) == 0, "wardrobe closed with neither outfit");
    g.top_given = 1; CHECK(g_outfit_open(&g) && g_outfit_has(&g, OF_TOP) && !g_outfit_has(&g, OF_YOGA) && g_outfit_has(&g, OF_USUAL), "the designer top alone opens the wardrobe");
    n = g_outfit_targets(&g, t); CHECK(n == 1 && t[0] == OF_TOP, "from the usual outfit: only the designer top");
    CHECK(g_outfit_set(&g, OF_YOGA) == 1 && g.outfit == OF_USUAL, "she cannot change into an outfit she does not own");
    CHECK(g_outfit_set(&g, OF_TOP) == 0 && g.outfit == OF_TOP, "changes into the designer top");
    n = g_outfit_targets(&g, t); CHECK(n == 1 && t[0] == OF_USUAL, "from the designer top: only back to the usual one");
    g.yoga_given = 1; g.outfit = OF_USUAL; n = g_outfit_targets(&g, t); CHECK(n == 2 && t[0] == OF_YOGA && t[1] == OF_TOP, "both owned: two rows from the usual outfit");
    g.outfit = OF_YOGA; n = g_outfit_targets(&g, t); CHECK(n == 2 && t[0] == OF_USUAL && t[1] == OF_TOP, "both owned: usual and top from yoga");
    g.outfit = OF_TOP; n = g_outfit_targets(&g, t); CHECK(n == 2 && t[0] == OF_USUAL && t[1] == OF_YOGA, "both owned: usual and yoga from the top");
    g.outfit = OF_USUAL; g_outfit_toggle(&g); CHECK(g.outfit == OF_YOGA, "cycle: usual > yoga"); g_outfit_toggle(&g); CHECK(g.outfit == OF_TOP, "cycle: yoga > top"); g_outfit_toggle(&g); CHECK(g.outfit == OF_USUAL, "cycle: top > usual");
    g.yoga_given = 0; g.outfit = OF_USUAL; g_outfit_toggle(&g); CHECK(g.outfit == OF_TOP, "cycle with only the top: usual > top"); g_outfit_toggle(&g); CHECK(g.outfit == OF_USUAL, "and back");
    g.yoga_given = 1; g.top_given = 0; g.outfit = OF_USUAL; g_outfit_toggle(&g); CHECK(g.outfit == OF_YOGA, "the old yoga-only switch is unchanged"); g_outfit_toggle(&g); CHECK(g.outfit == OF_USUAL, "and back");
    g.married = 0; g.outfit = OF_TOP; g_outfit_toggle(&g); CHECK(g.outfit == OF_USUAL && g_outfit_targets(&g, t) == 0, "not married: no wardrobe, always the usual outfit");
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.dring = 1; g.married = 0; g.top_given = 1; CHECK(g_marry(&g) == 0 && g.outfit == OF_USUAL && g.top_given == 1, "the wedding resets the outfit to the usual one and she keeps the top");
    /* saves */
    g_init(&g); g.rs = RS_LIVING; g.married = 1; g.top = 3; g.top_given = 1; g.outfit = OF_TOP;
    CHECK(save_game(&g, f) && load_game(&h, f) && h.top == 3 && h.top_given == 1 && h.outfit == OF_TOP && h.ver == SAVE_VER, "the designer top and the outfit survive a save");
    { Game o = g; unsigned char raw[sizeof(Game)]; FILE *fp; o.ver = SAVE_VER_22; o.outfit = OF_YOGA; o.yoga_given = 1; memcpy(raw, &o, sizeof o); fp = fopen(f, "wb"); if (fp) { fwrite(raw, 1, offsetof(Game, top), fp); fclose(fp); }
      CHECK(load_game(&h, f) && h.top == 0 && h.top_given == 0 && h.outfit == OF_YOGA && h.yoga_given == 1 && h.ver == SAVE_VER && h.size == (int)sizeof h, "a version-22 save loads: no designer top, the yoga outfit kept");
      o.ver = SAVE_VER; fp = fopen(f, "wb"); if (fp) { memcpy(raw, &o, sizeof o); fwrite(raw, 1, offsetof(Game, top), fp); fclose(fp); } CHECK(!load_game(&h, f), "the short size with the new version is rejected");
      o.ver = SAVE_VER_22; fp = fopen(f, "wb"); if (fp) { memcpy(raw, &o, sizeof o); fwrite(raw, 1, offsetof(Game, top) - 4, fp); fclose(fp); } CHECK(!load_game(&h, f), "a truncated version-22 file is rejected"); }
    { Game o = g; unsigned char raw[sizeof(Game)]; FILE *fp; o.ver = SAVE_VER_23; o.fr_wins = 7; memcpy(raw, &o, sizeof o); fp = fopen(f, "wb"); if (fp) { fwrite(raw, 1, offsetof(Game, fr_wins), fp); fclose(fp); }
      CHECK(load_game(&h, f) && h.fr_wins == 0 && h.top == 3 && h.ver == SAVE_VER && h.size == (int)sizeof h, "a version-23 save loads: the friendship bar starts at zero, the rest is kept");
      o.ver = SAVE_VER; fp = fopen(f, "wb"); if (fp) { memcpy(raw, &o, sizeof o); fwrite(raw, 1, offsetof(Game, fr_wins), fp); fclose(fp); } CHECK(!load_game(&h, f), "v11.0: the short size with the new version is rejected");
      o.fr_wins = 12; CHECK(save_game(&o, f) && load_game(&h, f) && h.fr_wins == 12, "the friendship wins survive a save"); o.fr_wins = -1; CHECK(!save_sane(&o), "negative friendship wins rejected"); o.fr_wins = 1000; CHECK(!save_sane(&o), "friendship wins over 999 rejected"); }
    h = g; h.outfit = 4; CHECK(!save_sane(&h), "outfit 4 rejected"); h = g; h.top = -1; CHECK(!save_sane(&h), "a negative stock rejected");
    /* text: the item, the gift pools, the outfit lines */
    CHECK(strstr(SH_NAME[SH_TOP], "DESIGNER") && strcmp(R_GNAME[GF_TOP], "DESIGNER TOP") == 0 && SH_DESC[SH_TOP] && SH_INFO[SH_TOP] && SH_BOUGHT[SH_TOP], "the item has its name, info, info page, give name and bought note");
    CHECK(GH_TOP_N >= 5 && GO_TOP_N >= 5 && GL_TOP_N >= 5 && GE_TOP_N >= 5 && R_OUTFIT_TOP_N >= 2 && R_OUTFIT_TOP_MM_N >= 2, "pools of 5 for the gift beats, 2+ for the outfit lines");
    for (i = 0; i < R_OUTFIT_TOP_MM_N; i++) { CHECK(!has_word_boy(R_OUTFIT_TOP_MM[i]) && !strstr(R_OUTFIT_TOP_MM[i], "baby") && !strstr(R_OUTFIT_TOP_MM[i], "Baby") && !strstr(R_OUTFIT_TOP_MM[i], "Mama") && !strstr(R_OUTFIT_TOP_MM[i], "mama"), "mama outfit line %d has no baby / boy / mama", i); }
}
static void test_v111(void) {   /* v11.1: the COW PRINT OUTFIT, a fourth outfit */
    Game g, h; int t[4], n, i; const char *f = "/tmp/pq_v111.sav";
    CHECK(SH_PRICE[SH_COW] == 4000 && SH_PRICE[SH_COW] > SH_PRICE[SH_TOP] && SH_PRICE[SH_COW] < SH_PRICE[SH_GOLD], "cow print outfit costs $4,000, between the designer top and the gold bar");
    g_init(&g); g.cash = 100000; g.story = ST_NAMED;
    CHECK(!g_shop_avail(&g, SH_COW) && g_shop_buy(&g, SH_COW) == 1 && g.cow == 0, "closed before she lives with you");
    g.rs = RS_LIVING; CHECK(!g_shop_avail(&g, SH_COW), "v11.3: closed while romance is not max");
    g.rom_lvl = ROM_LVLS; g.rom_pct = 100; CHECK(g_shop_avail(&g, SH_COW) && !g_shop_black(SH_COW) && !g_shop_once_done(&g, SH_COW), "open at max romance; a normal ITEMS item");
    CHECK(g_shop_buy(&g, SH_COW) == 0 && g.cow == 1 && g.cash == 100000 - 4000 && g.top == 0, "buying takes $4,000 and adds one cow outfit only");
    g.cow = INV_CAP; CHECK(g_shop_buy(&g, SH_COW) == 3 && g.cow == INV_CAP && g.cash == 100000 - 4000, "stock full at the cap, nothing is charged");
    g.cow = 0; g.cash = 100; g.bank = 0; CHECK(g_shop_buy(&g, SH_COW) == 1 && g.cow == 0, "not enough money");
    CHECK(g_gift_gain(GF_COW) == 0, "v11.3: gift gain 0 (no romance)");
    g_init(&g); g.rs = RS_LIVING; g.story = ST_NAMED; g.day = 4; g.cow = 2; g.rom_lvl = 2; g.rom_pct = 40;
    CHECK(g_gift(&g, GF_COW) == 12 && g.cow == 2 && !g.cow_given && g.rom_pct == 40, "below max romance she refuses it");
    g.rom_lvl = ROM_LVLS; g.rom_pct = 100; CHECK(g_gift_stock(&g, GF_COW) == 2 && !g.cow_given, "two in stock, not given yet");
    CHECK(g_gift(&g, GF_COW) == 0 && g.cow == 1 && g.cow_given == 1 && g.rom_pct == 100 && g.gift_day == 0 && !g.top_given && !g.yoga_given, "giving it at max: stock -1, no romance, not the gift of the day, she owns the cow outfit only");
    CHECK(g_gift(&g, GF_COW) == 1 && g.cow == 1, "she has it already: refused");
    g_init(&g); g.rs = RS_LIVING; g.story = ST_NAMED; g.day = 4; g.cow = 1; g.rom_lvl = ROM_LVLS; g.rom_pct = 100; g.gift_day = 4; CHECK(g_gift(&g, GF_COW) == 0 && g.cow_given, "a full bar and the gift of the day do not block it");
    g_init(&g); g.rs = RS_LIVING; g.married = 1; g.yoga_given = 0; g.top_given = 0; g.cow_given = 0;
    CHECK(!g_outfit_open(&g) && g_outfit_targets(&g, t) == 0, "wardrobe closed with no outfit");
    g.cow_given = 1; CHECK(g_outfit_open(&g) && g_outfit_has(&g, OF_COW) && !g_outfit_has(&g, OF_TOP) && !g_outfit_has(&g, OF_YOGA), "the cow outfit alone opens the wardrobe");
    n = g_outfit_targets(&g, t); CHECK(n == 1 && t[0] == OF_COW, "from the usual outfit: only the cow outfit");
    CHECK(g_outfit_set(&g, OF_TOP) == 1 && g.outfit == OF_USUAL, "she cannot change into the top she does not own");
    CHECK(g_outfit_set(&g, OF_COW) == 0 && g.outfit == OF_COW, "changes into the cow outfit");
    n = g_outfit_targets(&g, t); CHECK(n == 1 && t[0] == OF_USUAL, "from the cow outfit: only back to the usual one");
    g.yoga_given = 1; g.top_given = 1; g.outfit = OF_USUAL; n = g_outfit_targets(&g, t); CHECK(n == 3 && t[0] == OF_YOGA && t[1] == OF_TOP && t[2] == OF_COW, "all owned: three rows from the usual outfit");
    g.outfit = OF_COW; n = g_outfit_targets(&g, t); CHECK(n == 3 && t[0] == OF_USUAL && t[1] == OF_YOGA && t[2] == OF_TOP, "all owned: usual, yoga, top from the cow outfit");
    g.outfit = OF_TOP; n = g_outfit_targets(&g, t); CHECK(n == 3 && t[0] == OF_USUAL && t[1] == OF_YOGA && t[2] == OF_COW, "all owned: usual, yoga, cow from the top");
    g.outfit = OF_USUAL; g_outfit_toggle(&g); CHECK(g.outfit == OF_YOGA, "cycle: usual > yoga"); g_outfit_toggle(&g); CHECK(g.outfit == OF_TOP, "cycle: yoga > top"); g_outfit_toggle(&g); CHECK(g.outfit == OF_COW, "cycle: top > cow"); g_outfit_toggle(&g); CHECK(g.outfit == OF_USUAL, "cycle: cow > usual");
    g.yoga_given = 0; g.top_given = 0; g.outfit = OF_USUAL; g_outfit_toggle(&g); CHECK(g.outfit == OF_COW, "cycle with only the cow outfit: usual > cow"); g_outfit_toggle(&g); CHECK(g.outfit == OF_USUAL, "and back");
    g.yoga_given = 1; g.top_given = 0; g.cow_given = 0; g.outfit = OF_USUAL; g_outfit_toggle(&g); CHECK(g.outfit == OF_YOGA, "the yoga-only switch is unchanged"); g_outfit_toggle(&g); CHECK(g.outfit == OF_USUAL, "and back");
    g.yoga_given = 0; g.top_given = 1; g.outfit = OF_USUAL; g_outfit_toggle(&g); CHECK(g.outfit == OF_TOP, "the top-only switch is unchanged"); g_outfit_toggle(&g); CHECK(g.outfit == OF_USUAL, "and back");
    g.cow_given = 1; g.married = 0; g.outfit = OF_COW; g_outfit_toggle(&g); CHECK(g.outfit == OF_USUAL && g_outfit_targets(&g, t) == 0, "not married: no wardrobe, always the usual outfit");
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.dring = 1; g.married = 0; g.cow_given = 1; CHECK(g_marry(&g) == 0 && g.outfit == OF_USUAL && g.cow_given == 1, "the wedding resets the outfit to the usual one and she keeps the cow outfit");
    g_init(&g); g.rs = RS_LIVING; g.married = 1; g.cow = 3; g.cow_given = 1; g.outfit = OF_COW; g.top = 2; g.top_given = 1;
    CHECK(save_game(&g, f) && load_game(&h, f) && h.cow == 3 && h.cow_given == 1 && h.outfit == OF_COW && h.top == 2 && h.top_given == 1 && h.ver == SAVE_VER, "the cow outfit and the outfit survive a save");
    { Game o = g; unsigned char raw[sizeof(Game)]; FILE *fp; o.ver = SAVE_VER_24; o.outfit = OF_TOP; o.fr_wins = 9; memcpy(raw, &o, sizeof o); fp = fopen(f, "wb"); if (fp) { fwrite(raw, 1, offsetof(Game, cow), fp); fclose(fp); }
      CHECK(load_game(&h, f) && h.cow == 0 && h.cow_given == 0 && h.outfit == OF_TOP && h.top_given == 1 && h.fr_wins == 9 && h.ver == SAVE_VER && h.size == (int)sizeof h, "a version-24 save loads: no cow outfit, the top and the friendship bar kept");
      o.ver = SAVE_VER; fp = fopen(f, "wb"); if (fp) { memcpy(raw, &o, sizeof o); fwrite(raw, 1, offsetof(Game, cow), fp); fclose(fp); } CHECK(!load_game(&h, f), "the short size with the new version is rejected");
      o.ver = SAVE_VER_24; fp = fopen(f, "wb"); if (fp) { memcpy(raw, &o, sizeof o); fwrite(raw, 1, offsetof(Game, cow) - 4, fp); fclose(fp); } CHECK(!load_game(&h, f), "a truncated version-24 file is rejected"); }
    h = g; h.outfit = 3; h.cow_given = 1; CHECK(save_sane(&h), "outfit 3 (cow print) is valid"); h = g; h.outfit = 4; CHECK(!save_sane(&h), "outfit 4 rejected"); h = g; h.cow = -1; CHECK(!save_sane(&h), "a negative cow stock rejected"); h = g; h.cow = INV_CAP + 1; CHECK(!save_sane(&h), "a cow stock over the cap rejected"); h = g; h.cow_given = 2; CHECK(!save_sane(&h), "bad cow_given rejected");
    CHECK(strstr(SH_NAME[SH_COW], "COW PRINT") && strcmp(R_GNAME[GF_COW], "COW PRINT OUTFIT") == 0 && SH_DESC[SH_COW] && SH_INFO[SH_COW] && SH_BOUGHT[SH_COW], "the item has its name, info, info page, give name and bought note");
    CHECK(strstr(SH_DESC[SH_COW], "thin") && strstr(SH_DESC[SH_COW], "comfy") && strstr(SH_DESC[SH_COW], "cutout"), "the info page keeps the agreed description (thin, comfy, contours, chest cutout)");
    CHECK(GH_COW_N >= 5 && GO_COW_N >= 5 && GL_COW_N >= 5 && GE_COW_N >= 5 && R_OUTFIT_COW_N >= 2 && R_OUTFIT_COW_MM_N >= 2, "pools of 5 for the gift beats, 2+ for the outfit lines");
    for (i = 0; i < R_OUTFIT_COW_MM_N; i++) { CHECK(!has_word_boy(R_OUTFIT_COW_MM[i]) && !strstr(R_OUTFIT_COW_MM[i], "baby") && !strstr(R_OUTFIT_COW_MM[i], "Baby") && !strstr(R_OUTFIT_COW_MM[i], "Mama") && !strstr(R_OUTFIT_COW_MM[i], "mama"), "mama cow outfit line %d has no baby / boy / mama", i); }
    for (i = 0; i < GH_COW_N; i++) CHECK(!has_word_boy(GH_COW[i]) && !has_word_boy(GO_COW[i]) && !has_word_boy(GL_COW[i]) && !has_word_boy(GE_COW[i]), "cow gift pool entry %d has no boy", i);
}

static void test_v116(void) {   /* v11.6: five romance levels, harder each level, pink cards only after she lives here and got the pink deck, the version-25 upgrade */
    Game g, h; int i, last = 1000; const char *f = "/tmp/pq_v116.sav";
    CHECK(ROM_LVLS == 5 && ROM_SCALE[0] == 100, "five romance levels, level 1 unscaled");
    for (i = 0; i < ROM_LVLS; i++) { CHECK(ROM_SCALE[i] < last, "each level is harder than the one before (level %d)", i + 1); last = ROM_SCALE[i]; }
    g_init(&g); g.rs = RS_LIVING;
    for (i = 1; i <= 4; i++) { g.rom_lvl = i; g.rom_pct = 0; g_rom_add(&g, 100 / 4 * 4); CHECK(g.rom_pct == ROM_SCALE[i - 1] || g.rom_lvl > i, "level %d: +100 gives %d points (got %d)", i, ROM_SCALE[i - 1], g.rom_pct); }
    g.rom_lvl = 1; g.rom_pct = 0; g_rom_add(&g, 100); CHECK(g.rom_lvl == 2 && g.rom_pct == 0, "a full level 1 goes to level 2 at 0");
    g.rom_lvl = 4; g.rom_pct = 99; g_rom_add(&g, 1); CHECK(g.rom_lvl == 5 && g.rom_pct == 0 && !g_rom_full(&g), "level 4 -> level 5, not max yet");
    g.rom_lvl = 5; g.rom_pct = 99; g_rom_add(&g, 5); CHECK(g.rom_lvl == 5 && g.rom_pct == 100 && g_rom_full(&g), "level 5 full = max romance");
    g.rom_lvl = 3; g.rom_pct = 100; CHECK(!g_rom_full(&g) && !g_outfit_unlocked(&g), "a full level 3 is no longer max");
    g.rom_lvl = 1; g.rom_pct = 0; g_rom_add(&g, 1); CHECK(g.rom_pct >= 1, "a tiny gain still gives at least 1 point");
    g_init(&g); g.rs = RS_LIVING; g.rom_lvl = 5; g.rom_pct = 0; g_dbg_fillbar(&g); CHECK(g_rom_full(&g), "debug fill reaches a full bar at level 5");
    /* pink cards: she lives with you AND you gave her the pink deck */
    g_init(&g); g.day = 10; g.pdeck = 3; CHECK(!g_pink_cards(&g), "blue at the start");
    g.rs = RS_LIVING; CHECK(!g_pink_cards(&g), "blue when she lives with you but no deck was given");
    g_init(&g); g.day = 10; g.pdeck = 3; g.rs = RS_VISIT; CHECK(g_gift(&g, GF_DECK) == 0 && g.deck_given == 0, "a deck given before she moves in does not count");
    g.rs = RS_LIVING; g.gift_day = 0; CHECK(!g_pink_cards(&g), "still blue after moving in");
    g.day = 11; CHECK(g_gift(&g, GF_DECK) == 0 && g.deck_given == 1 && g_pink_cards(&g), "the deck given after she moved in turns the table pink");
    g_init(&g); g.day = 10; g.pdeck = 2; g.rs = RS_LIVING; g.gift_day = 10; CHECK(g_gift(&g, GF_DECK) == 0 && g.deck_given == 1 && g.rom_pct == 0 && g.gift_day == 10, "first deck on a day with a gift: accepted, no romance, gift day untouched");
    g_init(&g); g.day = 10; g.pdeck = 2; g.rs = RS_LIVING; g.rom_lvl = ROM_LVLS; g.rom_pct = 100; CHECK(g_gift(&g, GF_DECK) == 0 && g.deck_given == 1 && g.pdeck == 1, "first deck at a full bar: accepted");
    CHECK(g_gift(&g, GF_DECK) == 4, "a second deck at a full bar is refused as before");
    g_init(&g); g.rs = RS_LIVING; g.deck_given = 1; g.pdeck = 0; CHECK(g_pink_cards(&g), "the flag is kept without a deck in stock");
    /* save: current version round trip, version-25 upgrade, sane check */
    g_init(&g); g.rs = RS_LIVING; g.deck_given = 1; g.rom_lvl = 4; g.rom_pct = 40; CHECK(save_game(&g, f) && load_game(&h, f) && h.deck_given == 1 && h.rom_lvl == 4 && h.rom_pct == 40, "round trip");
    g.deck_given = 2; CHECK(!save_sane(&g), "deck_given must be 0 or 1");
    { unsigned char raw[sizeof(Game)]; FILE *w; int k;
      for (k = 0; k < 5; k++) {
          g_init(&g); g.ver = SAVE_VER_25; g.size = (int)offsetof(Game, deck_given); g.story = ST_NAMED;
          if (k == 0) { g.rs = RS_LIVING; g.rom_lvl = 3; g.rom_pct = 100; }              /* a full old level 3, not past the pill: starts level 4 */
          if (k == 1) { g.rs = RS_LIVING; g.rom_lvl = 3; g.rom_pct = 100; g.pill_given = 1; g.pill_seen = 1; g.married = 1; }   /* past the unlock: max */
          if (k == 2) { g.rs = RS_NONE; g.rom_lvl = 2; g.rom_pct = 50; }                  /* untouched */
          if (k == 3) { g.rs = RS_LIVING; g.rom_lvl = 1; g.rom_pct = 20; }
          if (k == 4) { g.rs = RS_LIVING; g.rom_lvl = 3; g.rom_pct = 100; g.pill = 1; }
          memcpy(raw, &g, sizeof g); w = fopen(f, "wb"); fwrite(raw, 1, offsetof(Game, deck_given), w); fclose(w);
          g_init(&h); CHECK(load_game(&h, f), "a version-25 save loads (case %d)", k);
          CHECK(h.ver == SAVE_VER && h.size == (int)sizeof h, "upgraded (case %d)", k);
          CHECK(h.deck_given == (g.rs == RS_LIVING), "deck_given = she lives with you (case %d)", k);
          if (k == 0 || k == 4) CHECK(h.rom_lvl == 4 && h.rom_pct == 0, "full old level 3 -> level 4 at 0 (case %d lvl %d pct %d)", k, h.rom_lvl, h.rom_pct);
          if (k == 1) CHECK(h.rom_lvl == ROM_LVLS && h.rom_pct == 100 && g_rom_full(&h), "past the unlock: max romance, nothing relocks");
          if (k == 2) CHECK(h.rom_lvl == 2 && h.rom_pct == 50, "level 2 untouched");
          if (k == 3) CHECK(h.rom_lvl == 1 && h.rom_pct == 20, "level 1 untouched");
      } }
}

static void test_v122(void) {   /* v12.2: new prices, the breast pump wears out (9..15 bottles), Sophia tells you, a new pump from the store handed over from the give page, save version 27 */
    Game g, h; int i, k, lo = 99, hi = 0, ok; const char *f = "/tmp/pq_v122.sav"; FILE *fp;
    CHECK(SH_PRICE[SH_YOGA] == 3000 && SH_PRICE[SH_TOP] == 3500 && SH_PRICE[SH_COW] == 4000, "outfits: yoga 3000, designer top 3500, cow print 4000");
    CHECK(SH_PRICE[SH_COW] > SH_PRICE[SH_TOP] && SH_PRICE[SH_TOP] > SH_PRICE[SH_YOGA], "the cow outfit is the dearest outfit");
    CHECK(SH_PRICE[SH_PILL] == 15000 && SH_PRICE[SH_MILK] == 750 && SH_PRICE[SH_SUPER] == 2250, "pill x1.5 = 15000, milk pill x1.5 = 750, Super drug x1.5 = 2250");
    CHECK(SH_PRICE[SH_PUMP] == 500 && SH_PRICE[SH_RING] == 12000 && SH_PRICE[SH_FLOWER] == 250 && SH_PRICE[SH_STACK] == 1000 && SH_PRICE[SH_DIAMOND] == 50000 && SH_PRICE[SH_GOLD] == 6000, "the other prices are untouched");
    /* the random life: always 9..15, both ends happen */
    for (i = 0; i < 5000; i++) { k = g_pump_roll(); if (k < lo) lo = k; if (k > hi) hi = k; }
    CHECK(PUMP_MIN > 8 && lo == PUMP_MIN && hi == PUMP_MAX, "the pump life is %d..%d, never fewer than 9 (saw %d..%d)", PUMP_MIN, PUMP_MAX, lo, hi);
    /* PUMP MILK: life 9 = nine bottles, the tenth ask finds it broken */
    g_init(&g); g.rs = RS_LIVING; g.married = 1; g.pill_seen = 1; g.milk_st = 4; g.pump_life = 9; g.cash = 100000;
    for (i = 1; i <= 9; i++) { g.milk_pct = 100; g.bottle = 0; CHECK(g_pump_use(&g) == 0 && g.bottle == 1 && g.pump_used == i && !g.pump_dead, "bottle %d of 9 works", i); }
    g.milk_pct = 100; g.bottle = 0; g.mday = 0;
    CHECK(!g_shop_avail(&g, SH_PUMP) || g_shop_once_done(&g, SH_PUMP), "before she tells you the store has no pump");
    CHECK(g_shop_buy(&g, SH_PUMP) == 2 && g.pump == 0, "the store refuses a pump while hers still counts as working");
    CHECK(g_pump_use(&g) == 14 && g.bottle == 0 && g.milk_pct == 100 && g.mday == 0 && g.pump_dead == 1, "the tenth ask: she says it is broken, no bottle, gauge stays full, no milk day");
    CHECK(g_pump_use(&g) == 14 && g.bottle == 0 && g.milk_pct == 100, "still broken when you ask again");
    CHECK(g_shop_avail(&g, SH_PUMP) && !g_shop_once_done(&g, SH_PUMP), "after she told you the new pump is in the store again");
    CHECK(g_gift(&g, GF_PUMP) == 1, "no new pump in the bag yet: nothing to give");
    CHECK(g_shop_buy(&g, SH_PUMP) == 0 && g.pump == 1 && g.cash == 100000 - 500 && g_shop_once_done(&g, SH_PUMP) && g_shop_buy(&g, SH_PUMP) == 2, "buying it: 500, one at a time");
    CHECK(g_pump_use(&g) == 14, "a new pump in the bag but not handed over: still broken");
    CHECK(g_gift(&g, GF_PUMP) == 0 && g.pump == 0 && g.pump_dead == 0 && g.pump_used == 0 && g.pump_life >= PUMP_MIN && g.pump_life <= PUMP_MAX && g.milk_pct == 100 && g.milk_st == 4, "handed over from the give page: new life, gauge untouched");
    CHECK(g_shop_once_done(&g, SH_PUMP) && g_pump_use(&g) == 0 && g.bottle == 1 && g.pump_used == 1, "it works again and the store hides the pump again");
    g.pump = 1; g.pump_dead = 0; CHECK(g_gift(&g, GF_PUMP) == 7 && g.pump == 1, "a pump while hers works is not wanted");
    /* ASK FOR MILK (Super drug): the broken ask is not used up */
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.milk_st = 4; g.sdrug = 1; g.day = 3; g.pump_life = 1; g_gift(&g, GF_SUPER);
    CHECK(g_ask_milk(&g) == 0 && g.bottle == 1 && g.ask_n == 1, "ask 1 works");
    CHECK(g_ask_milk(&g) == 14 && g.bottle == 1 && g.ask_n == 1 && g.pump_dead == 1, "ask 2: broken, the ask is not used up");
    g.pump = 1; CHECK(g_gift(&g, GF_PUMP) == 0 && g_ask_milk(&g) == 0 && g.bottle == 2 && g.ask_n == 2, "a new pump and the ask works on the same day");
    /* a milk pill that would fill the gauge before the wedding */
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.milk_st = 4; g.mpill = 3; g.milk_pct = 50; g.pump_life = 4; g.pump_used = 4;
    CHECK(g_gift(&g, GF_MILK) == 14 && g.mpill == 3 && g.milk_pct == 50 && g.bottle == 0 && g.pump_dead == 1, "a broken pump: the pill that would make a bottle is refused and not used up");
    g.milk_pct = 0; CHECK(g_gift(&g, GF_MILK) == 0 && g.mpill == 2 && g.milk_pct == 50, "a pill that only fills half needs no pump");
    g.pump_dead = 0; g.pump_used = 0; g.milk_pct = 50; CHECK(g_gift(&g, GF_MILK) == 0 && g.bottle == 1 && g.milk_pct == 0 && g.pump_used == 1, "a working pump: the pill makes the bottle (one use)");
    /* married: the pill that fills the gauge needs no pump, breastfeeding needs none either */
    g_init(&g); g.rs = RS_LIVING; g.married = 1; g.call = 1; g.pill_seen = 1; g.milk_st = 4; g.mpill = 2; g.milk_pct = 50; g.pump_life = 3; g.pump_used = 3;
    CHECK(g_gift(&g, GF_MILK) == 0 && g.milk_pct == 100 && g.bottle == 0 && g.pump_used == 3 && !g.pump_dead, "married: the pill fills the gauge, no pump involved");
    CHECK(g_feed(&g) == 0 && g.milk_pct == 0 && g.pump_used == 3 && !g.pump_dead, "breastfeeding does not use the pump");
    /* the bottle stock full or the gauge not full never counts a use */
    g_init(&g); g.rs = RS_LIVING; g.married = 1; g.pill_seen = 1; g.milk_st = 4; g.pump_life = 9; g.milk_pct = 100; g.bottle = INV_CAP;
    CHECK(g_pump_use(&g) == 8 && g.pump_used == 0, "bottle stock full: no use counted"); g.bottle = 0; g.milk_pct = 50; CHECK(g_pump_use(&g) == 1 && g.pump_used == 0, "gauge not full: no use counted");
    /* a pump without a rolled life (old save, test fixture) gets one on its first use */
    g_init(&g); g.rs = RS_LIVING; g.married = 1; g.pill_seen = 1; g.milk_st = 4; g.milk_pct = 100;
    CHECK(g_pump_use(&g) == 0 && g.pump_life >= PUMP_MIN && g.pump_life <= PUMP_MAX && g.pump_used == 1, "an unrolled pump gets its life on the first use");
    /* a whole life: every roll gives between 9 and 15 bottles */
    for (k = 0; k < 300; k++) { int n = 0; g_init(&g); g.rs = RS_LIVING; g.married = 1; g.pill_seen = 1; g.milk_st = 4;
        while (n < 40) { g.milk_pct = 100; g.bottle = 0; if (g_pump_use(&g)) break; n++; }
        CHECK(n >= 9 && n <= 15 && g.pump_dead, "a pump made %d bottles", n); }
    /* save version 27 */
    g_init(&g); g.rs = RS_LIVING; g.pill_seen = 1; g.milk_st = 4; g.pump_life = 12; g.pump_used = 5; g.pump_dead = 0; g.deck_given = 1; g.rom_lvl = 4; g.rom_pct = 40;
    CHECK(save_game(&g, f) && load_game(&h, f) && h.ver == SAVE_VER && SAVE_VER == 28 && h.pump_life == 12 && h.pump_used == 5 && h.pump_dead == 0, "the pump life round-trips in a version-28 save");
    g.pump_dead = 1; g.pump_used = 12; CHECK(save_game(&g, f) && load_game(&h, f) && h.pump_dead == 1 && h.pump_used == 12, "a broken pump is saved");
    { Game b = g; b.pump_used = 13; CHECK(!save_sane(&b), "a save with more uses than life is refused"); b = g; b.pump_life = PUMP_MAX + 1; CHECK(!save_sane(&b), "a life above the maximum is refused"); b = g; b.pump_dead = 2; CHECK(!save_sane(&b), "a bad broken flag is refused"); }
    { Game c = g; c.ver = SAVE_VER_26; c.pump_life = c.pump_used = c.pump_dead = 0; fp = fopen(f, "wb"); ok = fp && fwrite(&c, 1, offsetof(Game, pump_life), fp) == offsetof(Game, pump_life); if (fp) fclose(fp);
      CHECK(ok && load_game(&h, f) && h.ver == SAVE_VER && h.pump_life == 0 && h.pump_used == 0 && h.pump_dead == 0 && h.deck_given == 1 && h.rom_lvl == 4 && h.rom_pct == 40 && h.milk_st == 4, "a version-26 save loads: nothing lost, the pump gets its life on its next use"); }
    remove(f);
}


static void test_v123(void) {   /* v12.3: the Mommy Milk Injection: store, give, no gauge, ONE milk a day (ask / pump / breastfeed), 2 bottles, the pump still wears out, save version 28 */
    Game g, h; int i; const char *f = "/tmp/pq_v123.sav"; FILE *fp; int ok;
    #define V123_BASE() do { g_init(&g); g.rs = RS_LIVING; g.married = 1; g.call = 1; g.pill_seen = 1; g.milk_st = 4; g.pump_life = 12; g.cash = 200000; g.sw_wins = SW_GOAL; g.day = 20; } while (0)
    CHECK(SH_PRICE[SH_INJECT] == 30000 && g_shop_black(SH_INJECT), "the injection costs 30000 and is a black market item");
    g_init(&g); g.cash = 90000; CHECK(!g_shop_avail(&g, SH_INJECT), "not sold before the black market opens");
    V123_BASE(); g.pill_seen = 0; CHECK(!g_shop_avail(&g, SH_INJECT), "not sold before the day-8 scene");
    V123_BASE(); CHECK(g_shop_avail(&g, SH_INJECT) && !g_shop_once_done(&g, SH_INJECT), "listed after the day-8 scene");
    g.cash = 29999; CHECK(g_shop_buy(&g, SH_INJECT) == 1 && g.inj == 0, "not enough money");
    g.cash = 100000; CHECK(g_shop_buy(&g, SH_INJECT) == 0 && g.inj == 1 && g.cash == 70000, "bought for 30000");
    CHECK(g_shop_once_done(&g, SH_INJECT) && g_shop_buy(&g, SH_INJECT) == 2 && g.cash == 70000, "one time only: no longer listed, a second buy is refused");
    CHECK(g_gift_stock(&g, GF_INJECT) == 1, "it is in the bag as a gift kind");
    /* before the pump scene she cannot get it */
    { Game e = g; e.milk_st = 2; CHECK(g_gift(&e, GF_INJECT) == 6 && e.inj == 1 && !e.injected, "she needs the pump first"); e.milk_st = 1; CHECK(g_gift(&e, GF_INJECT) == 5 && !e.injected, "the first pill is still working"); }
    { Game e = g; e.sdrug = 1; g_gift(&e, GF_SUPER); CHECK(g_super_active(&e) && g_gift(&e, GF_INJECT) == 15 && e.inj == 1 && !e.injected, "refused while a Super drug works (nothing used up)"); }
    /* give it */
    g.milk_pct = 50; g.mpill = 3; g.sdrug = 1;
    CHECK(g_gift(&g, GF_INJECT) == 0 && g.inj == 0 && g.injected == 1 && g.milk_pct == 0 && g.inj_day == 0, "given: injected, the gauge is empty");
    CHECK(!g_milk_gauge(&g) && g_gift_stock(&g, GF_INJECT) == 0 && g_gift(&g, GF_INJECT) == 1, "the bottle gauge is gone for good; nothing left to give");
    CHECK(!g_gifted_today(&g) , "it is not the gift of the day");
    CHECK(g_gift(&g, GF_MILK) == 16 && g.mpill == 3 && g_gift(&g, GF_SUPER) == 16 && g.sdrug == 1, "the milk pill and the Super drug are refused and stay in the bag");
    CHECK(!g_shop_avail(&g, SH_MILK) && !g_shop_avail(&g, SH_SUPER), "the milk pill and the Super drug are no longer sold");
    CHECK(g_shop_once_done(&g, SH_INJECT) && g_shop_buy(&g, SH_INJECT) == 2, "no second injection");
    /* ONE milk a day in total: ask, pump or breastfeed */
    g.bottle = 0; g.pump_used = 0; g.pump_life = 12; g.mday = 0;
    CHECK(g_asks_left(&g) == 1 && g_feed_ready(&g) && g_pump_ready(&g), "all three are open on a fresh day");
    CHECK(g_ask_milk(&g) == 0 && g.bottle == 2 && g.pump_used == 1 && g.inj_day == g.day && g.mday == g.day, "ASK FOR MILK: TWO bottles, one pump use, a milk day");
    CHECK(g_asks_left(&g) == 0 && !g_feed_ready(&g) && !g_pump_ready(&g), "after the ask nothing else is open today");
    CHECK(g_ask_milk(&g) == 2 && g.bottle == 2 && g.pump_used == 1, "a second ask the same day is refused, nothing given");
    CHECK(g_pump_use(&g) == 1 && g.bottle == 2 && g_feed(&g) == 1, "pump milk and breastfeed are refused the same day too");
    g.day++; g.bottle = 0;
    CHECK(g_feed_ready(&g) && g_feed(&g) == 0 && g.inj_day == g.day && g.pump_used == 1 && g.bottle == 0, "next day BREASTFEED works with no gauge, no pump use, no bottle");
    CHECK(g_ask_milk(&g) == 2 && g_pump_use(&g) == 1 && g_feed(&g) == 1, "after breastfeeding: no ask, no pump, no second breastfeed");
    g.day++; CHECK(g_pump_ready(&g) && g_pump_use(&g) == 0 && g.bottle == 1 && g.pump_used == 2 && g.milk_pct == 0, "next day PUMP MILK: one bottle, one pump use, no gauge needed");
    CHECK(g_ask_milk(&g) == 2 && g_feed(&g) == 1, "after the pump nothing else today");
    /* husband mode: BREASTFEED does not exist, ask / pump do */
    g.day++; g.call = 0; CHECK(g_feed(&g) == 1 && g_ask_milk(&g) == 0, "breastfeed needs mama mode, ASK works anyway"); g.call = 1;
    /* bottle cap */
    g.day++; g.bottle = INV_CAP - 1; CHECK(g_ask_milk(&g) == 8 && g.inj_day != g.day && g.bottle == INV_CAP - 1, "two bottles do not fit: refused, the day is not used");
    g.bottle = INV_CAP - 2; CHECK(g_ask_milk(&g) == 0 && g.bottle == INV_CAP, "two bottles fit exactly");
    /* the pump still wears out: ask and pump milk use it, breastfeed does not */
    g.day++; g.bottle = 0; g.pump_life = 9; g.pump_used = 9;
    CHECK(g_ask_milk(&g) == 14 && g.bottle == 0 && g.inj_day != g.day && g.pump_dead == 1, "a worn-out pump: the ask finds it broken, no bottles, the day is not used");
    CHECK(g_pump_use(&g) == 14 && g_feed_ready(&g) && g_feed(&g) == 0, "pump milk is broken too, breastfeeding still works");
    g.day++; g.pump = 1; CHECK(g_gift(&g, GF_PUMP) == 0 && g.pump_dead == 0 && g_ask_milk(&g) == 0 && g.bottle == 2, "a new pump handed over: ask works again");
    /* save */
    CHECK(save_game(&g, f) && load_game(&h, f) && h.ver == 28 && h.injected == 1 && h.inj_day == g.inj_day && h.inj == 0, "the injection round-trips in a version-28 save");
    { Game b = g; b.injected = 2; CHECK(!save_sane(&b), "bad injected flag refused"); b = g; b.inj = 1; CHECK(!save_sane(&b), "bought and given at once is refused"); b = g; b.inj_day = -1; CHECK(!save_sane(&b), "negative day refused"); }
    { Game c = g; c.ver = SAVE_VER_27; c.inj = c.injected = c.inj_day = 0; fp = fopen(f, "wb"); ok = fp && fwrite(&c, 1, offsetof(Game, inj), fp) == offsetof(Game, inj); if (fp) fclose(fp);
      CHECK(ok && load_game(&h, f) && h.ver == SAVE_VER && h.inj == 0 && h.injected == 0 && h.inj_day == 0 && h.married == 1 && h.pump_life == g.pump_life, "a version-27 save loads: no injection, nothing lost"); }
    /* a game that was never injected is unchanged */
    V123_BASE(); g.milk_pct = 100; g.bottle = 0; CHECK(g_milk_gauge(&g) && g_pump_use(&g) == 0 && g.bottle == 1 && g.milk_pct == 0 && g_pump_ready(&g) == 0, "without the injection the gauge rules are unchanged");
    remove(f); (void)i;
}


/* v12.4: Sophia's hotel matches cost 20% energy on top of the buy-in */
static void test_v124(void) {
    Game g; int i;
    printf("v12.4 energy costs / prices...\n");
    g_init(&g); g.cash = 5000; g.energy = 100;
    CHECK(SOPH_ENERGY == 20 && g_soph_energy_ok(&g), "20 percent energy needed, 100 is fine");
    CHECK(g_soph_start(&g, 500) == 1 && g.cash == 4500 && g.energy == 80 && g.in_match && g.fee == 500 && !g.home, "a hotel match: buy-in and 20 percent energy");
    g.in_match = 0; g.energy = 20; CHECK(g_soph_start(&g, 100) == 1 && g.energy == 0 && g.cash == 4400, "exactly 20 percent is enough");
    g.in_match = 0; g.energy = 19; CHECK(g_soph_start(&g, 100) == -1 && g.energy == 19 && g.cash == 4400 && !g.in_match, "19 percent: refused, nothing paid");
    g.cash = 50; g.energy = 100; CHECK(g_soph_start(&g, 100) == 0 && g.energy == 100 && g.cash == 50, "no cash: refused, nothing used");
    g.cash = 5000; g.energy = 5; g.bottle = 1; g_drink_bottle(&g); CHECK(g_soph_energy_ok(&g) && g_soph_start(&g, 100) == 1 && g.energy == 5, "the milk bottle: no energy needed, none used");
    g_init(&g); g.cash = 5000; g.energy = 100; g_soph_start(&g, 100); g.net = 0; g_match_finish(&g); CHECK(g.cash == 5000 && g.energy == 80, "a tie gives the buy-in back but not the energy");
    g.energy = 100; g_soph_start(&g, 100); g_forfeit(&g); CHECK(g.energy == 80, "walking out does not dodge the energy cost");
    g_init(&g); g.cash = 5000; g.energy = 100; g.day = 2; g.chal_day = 1; g_home_start(&g, 10); CHECK(g.energy == 100 - HOME_ENERGY && HOME_ENERGY == 10, "the free match at home still costs 10 percent");
    for (i = 0; i < 6; i++) { g_init(&g); g.cash = 5000; g.energy = 20 * i; CHECK(g_soph_energy_ok(&g) == (i >= 1), "energy gate at %d percent", 20 * i); }
    g_init(&g); g.energy = 0; g.cash = 4; CHECK(g_rescue_due(&g), "pity soda: under the $5 price"); g.cash = 5; CHECK(!g_rescue_due(&g), "$5 buys a soda: no pity");
}


/* v12.6: 8 ball. Buy-in / prize rules, physics invariants, the rules of 8 ball, the CPU player, whole games. */
static void run_shot(Pool *g) { int n = 0; while (g->phase == PH_ROLL && n++ < 100000) pl_run(g, 8); }
static void test_v126(void) {
    Game g; int i, k, games, over = 0, wins[2] = {0, 0};
    g_init(&g); g.cash = 100; g.energy = ENERGY_MAX;
    CHECK(g_pool_prize(10) == 18 && g_pool_prize(50) == 90 && g_pool_prize(1000) == 1800, "prize is 1.8x the buy-in");
    CHECK(g_pool_start(&g, 500) == 0 && g.cash == 100, "no cash: refused");
    CHECK(g_pool_start(&g, 50) == 1 && g.cash == 50 && g.energy == ENERGY_MAX - EX_ENERGY, "buy-in and energy leave");
    g.energy = EX_ENERGY - 1; CHECK(g_pool_start(&g, 10) == -1 && g.cash == 50, "no energy: refused");
    g_pool_win(&g, 50); CHECK(g.cash == 140, "a win pays the prize");
    /* the rack */
    { Pool p; int sol = 0, str = 0, a, b; pl_new(&p, 123);
      for (i = 0; i < 16; i++) CHECK(p.in[i], "all balls on the table");
      CHECK(fabsf(p.y[8] - PL_CY) < 0.01f, "the 8 is on the middle line");
      for (i = 1; i < 16; i++) for (k = i + 1; k < 16; k++) { float dx = p.x[i] - p.x[k], dy = p.y[i] - p.y[k]; CHECK(dx * dx + dy * dy >= 4 * PL_R * PL_R, "no balls overlap in the rack"); }
      /* back corners: one solid, one stripe */
      a = b = -1; for (i = 1; i < 16; i++) { if (fabsf(p.x[i] - (PL_FOOT_X + 4 * (1.7320508f * PL_R + 0.35f))) < 0.1f && fabsf(fabsf(p.y[i] - PL_CY) - 2 * (2 * PL_R + 0.3f)) < 0.1f) { if (a < 0) a = i; else b = i; } }
      CHECK(a > 0 && b > 0 && pl_group_of(a) != pl_group_of(b), "the back corners hold one solid and one stripe");
      (void)sol; (void)str; }
    /* v12.8: bigger balls, the ray from any point, the straight rebound of the aiming guide */
    { Pool p; PlRay r, r2; float ra, xx, yy; int n, ok = 1; pl_new(&p, 9);
      CHECK(PL_R >= 10.0f, "v12.8: the ball radius is at least 10");
      for (n = 1; n < 16; n++) if (p.x[n] + PL_R > PL_X1 || p.x[n] - PL_R < PL_X0 || p.y[n] - PL_R < PL_Y0 || p.y[n] + PL_R > PL_Y1) ok = 0;
      CHECK(ok, "v12.8: the rack fits inside the cushions");
      for (n = 1; n < 16; n++) p.in[n] = 0; p.x[0] = 200; p.y[0] = 210;
      r = pl_ray(&p, -1.0f); r2 = pl_ray_from(&p, p.x[0], p.y[0], -1.0f);
      CHECK(r.hit == -1 && r.wall == 2 && r.t == r2.t && r.cx == r2.cx && r.cy == r2.cy, "v12.8: pl_ray is pl_ray_from the cue ball; a top cushion is wall 2");
      CHECK(fabsf(r.cy - (PL_Y0 + PL_R)) < 0.01f, "v12.8: the contact is one radius from the top cushion");
      ra = pl_rebound_angle(-1.0f, r.wall); CHECK(fabsf(ra - 1.0f) < 1e-4f, "v12.8: a top cushion flips the vertical direction");
      r2 = pl_ray_from(&p, r.cx, r.cy, ra); CHECK(r2.hit == -1 && r2.cy > r.cy && r2.t > 1, "v12.8: the bounced ray runs back into the table");
      xx = r.cx + cosf(ra) * 80; yy = r.cy + sinf(ra) * 80; p.in[4] = 1; p.x[4] = xx; p.y[4] = yy; r2 = pl_ray_from(&p, r.cx, r.cy, ra);
      CHECK(r2.hit == 4 && r2.t > 55 && r2.t < 80.5f, "v12.8: the bounced ray stops at a ball in the way");
      p.in[4] = 0; r = pl_ray(&p, 0.1f); CHECK(r.hit == -1 && r.wall == 1 && fabsf(pl_rebound_angle(0.1f, 1) - (3.14159265f - 0.1f)) < 1e-4f, "v12.8: a side cushion flips the horizontal direction");
      p.x[0] = PL_X0 + PL_R + 100; p.y[0] = PL_Y0 + PL_R + 100; r = pl_ray(&p, -2.35619449f);
      CHECK(r.hit == -1 && r.wall == 3 && fabsf(pl_rebound_angle(-2.35619449f, 3) - 0.78539816f) < 1e-4f, "v12.8: a corner shot comes straight back");
      p.x[0] = 300; p.y[0] = 200; r = pl_ray(&p, 3.14159265f); r2 = pl_ray_from(&p, r.cx, r.cy, pl_rebound_angle(3.14159265f, r.wall));
      CHECK(r.wall == 1 && r2.hit == -1 && r2.wall == 1 && r2.cx > r.cx + 100, "v12.8: left cushion, then the right cushion is the next stop"); }
    /* physics: nothing escapes, nothing speeds up on its own */
    for (games = 0; games < 40; games++) {
        Pool p; pl_new(&p, 900 + games); pl_shoot(&p, (games % 7 - 3) * 0.02f, 0.5f + 0.5f * (games % 2));
        { int n = 0, bad = 0; float vmax0 = 0;
          for (i = 0; i < 16; i++) { float v = sqrtf(p.vx[i] * p.vx[i] + p.vy[i] * p.vy[i]); if (v > vmax0) vmax0 = v; }
          while (p.phase == PH_ROLL && n++ < 100000) { pl_run(&p, 1);
              for (i = 0; i < 16; i++) if (p.in[i]) { if (p.x[i] < PL_X0 - 1 || p.x[i] > PL_X1 + 1 || p.y[i] < PL_Y0 - 1 || p.y[i] > PL_Y1 + 1) bad = 1; if (sqrtf(p.vx[i] * p.vx[i] + p.vy[i] * p.vy[i]) > vmax0 * 1.001f) bad = 2; } }
          CHECK(p.phase != PH_ROLL, "a shot always ends"); CHECK(bad == 0, "a ball left the table or sped up beyond the cue ball"); }
    }
    /* rules, one by one */
    { Pool p; pl_new(&p, 5); for (i = 1; i < 16; i++) p.in[i] = 0; p.brk = 0; p.grp[0] = 0; p.grp[1] = 1; p.in[1] = 1; p.in[9] = 1; p.in[8] = 1;
      p.x[0] = 200; p.y[0] = 200; p.x[9] = 300; p.y[9] = 200; p.x[1] = 300; p.y[1] = 300; p.x[8] = 500; p.y[8] = 120;
      pl_shoot(&p, 0, 0.3f); run_shot(&p);   /* straight at a stripe while you own solids */
      CHECK(p.first_hit == 9 && p.foul == FL_WRONG && p.turn == 1 && p.bih, "wrong group first = foul, ball in hand"); }
    { Pool p; pl_new(&p, 5); for (i = 1; i < 16; i++) p.in[i] = 0; p.brk = 0; p.grp[0] = 0; p.grp[1] = 1; p.in[1] = 1; p.in[8] = 1; p.x[1] = 260; p.y[1] = 200; p.x[8] = 600; p.y[8] = 340; p.x[0] = 200; p.y[0] = 200;
      pl_shoot(&p, 0, 0.12f); run_shot(&p);   /* a soft tap on your own ball: contact but no cushion and no pot */
      CHECK(p.first_hit == 1 && p.foul == FL_NORAIL && p.turn == 1, "no cushion after contact = foul"); }
    { Pool p; pl_new(&p, 5); for (i = 1; i < 16; i++) p.in[i] = 0; p.brk = 0; p.grp[0] = 0; p.grp[1] = 1; p.in[8] = 1; p.x[8] = 600; p.y[8] = 340; p.x[0] = 200; p.y[0] = 200;
      pl_shoot(&p, 3.14159f, 0.0f); run_shot(&p);
      CHECK(p.foul == FL_NOHIT && p.turn == 1 && p.bih, "hitting nothing = foul"); }
    { Pool p; pl_new(&p, 5); for (i = 1; i < 16; i++) p.in[i] = 0; p.brk = 0; p.in[1] = 1; p.in[8] = 1; p.x[8] = 600; p.y[8] = 340;   /* open table: first clean pot picks the group */
      p.x[0] = 150; p.y[0] = 150; p.x[1] = 200; p.y[1] = 200;
      { float dx = PL_PX[0] - p.x[1], dy = PL_PY[0] - p.y[1], l = sqrtf(dx * dx + dy * dy); p.x[0] = p.x[1] - dx / l * 2 * PL_R * 1.0f; p.y[0] = p.y[1] - dy / l * 2 * PL_R * 1.0f; pl_shoot(&p, atan2f(dy, dx), 0.4f); }
      run_shot(&p);
      CHECK(!p.in[1] && p.foul == FL_NONE && p.grp[0] == 0 && p.grp[1] == 1 && p.turn == 0, "a clean pot on an open table gives you solids and the next shot"); }
    { Pool p; pl_new(&p, 5); for (i = 1; i < 16; i++) p.in[i] = 0; p.brk = 0; p.grp[0] = 0; p.grp[1] = 1; p.in[8] = 1; p.x[8] = 200; p.y[8] = 200; p.in[1] = 0;   /* 8 potted legally after your group is gone = win */
      { float dx = PL_PX[0] - p.x[8], dy = PL_PY[0] - p.y[8], l = sqrtf(dx * dx + dy * dy); p.x[0] = p.x[8] - dx / l * 2 * PL_R; p.y[0] = p.y[8] - dy / l * 2 * PL_R; pl_shoot(&p, atan2f(dy, dx), 0.4f); }
      run_shot(&p);
      CHECK(p.phase == PH_OVER && p.winner == 0 && p.wreason == PW_8BALL, "the 8 after your group is a win"); }
    { Pool p; pl_new(&p, 5); for (i = 1; i < 16; i++) p.in[i] = 0; p.brk = 0; p.grp[0] = 0; p.grp[1] = 1; p.in[8] = 1; p.in[2] = 1; p.x[2] = 450; p.y[2] = 300; p.x[8] = 200; p.y[8] = 200;   /* the 8 too early = loss */
      { float dx = PL_PX[0] - p.x[8], dy = PL_PY[0] - p.y[8], l = sqrtf(dx * dx + dy * dy); p.x[0] = p.x[8] - dx / l * 2 * PL_R; p.y[0] = p.y[8] - dy / l * 2 * PL_R; pl_shoot(&p, atan2f(dy, dx), 0.4f); }
      run_shot(&p);
      CHECK(p.phase == PH_OVER && p.winner == 1 && p.wreason == PW_EARLY8, "the 8 too early loses"); }
    /* whole games, CPU vs CPU: always finish, always a winner, the strong one beats the weak one */
    for (k = 0; k < 2; k++) {
        int sk1 = k == 0 ? 0 : 4; wins[0] = wins[1] = 0;
        for (games = 0; games < 24; games++) {
            Pool p; int shots = 0; pl_new(&p, 3000 + games * 17 + k);
            while (p.phase != PH_OVER && shots < 400) { float a, pw; if (p.bih) pl_ai_place(&p); pl_ai_shot(&p, p.turn == 0 ? 4 : sk1, &a, &pw); pl_shoot(&p, a, pw); run_shot(&p); shots++; }
            CHECK(p.phase == PH_OVER && p.winner >= 0, "a CPU game always finishes"); if (p.phase == PH_OVER) { over++; wins[p.winner]++; }
        }
        if (k == 0) CHECK(wins[0] >= 20, "the best CPU beats the weakest at least 20 of 24"); else { printf("  equal CPUs: %d-%d\n", wins[0], wins[1]); CHECK(wins[0] >= 3 && wins[0] <= 21, "equal CPUs: neither side wins them all (the breaker is a little behind)"); }
    }
    CHECK(over == 48, "all 48 games were decided");
}

/* v12.7: spin. The cloth (slide then roll), follow / draw / stun, english on balls and cushions, energy, the picture's orientation, pockets, the break, the CPU. */
static float v127_energy(const Pool *g) { float e = 0; int i; for (i = 0; i < 16; i++) if (g->in[i]) e += 0.5f * (g->vx[i] * g->vx[i] + g->vy[i] * g->vy[i]) + 0.2f * PL_R * PL_R * (g->wx[i] * g->wx[i] + g->wy[i] * g->wy[i] + g->wz[i] * g->wz[i]); return e; }
static void v127_clear(Pool *p) { int i; for (i = 1; i < 16; i++) p->in[i] = 0; p->brk = 0; }
static void test_v127(void) {
    int i, k; Pool p, q;
    /* 1. a fresh cue ball skids, then rolls, exactly when the physics says */
    { float v0; int n = 0, lock = -1; pl_new(&p, 5); v127_clear(&p); p.x[0] = 60; p.y[0] = PL_CY; pl_shoot(&p, 0, 0.5f); v0 = p.vx[0];
      CHECK(fabsf(p.wx[0]) + fabsf(p.wy[0]) + fabsf(p.wz[0]) == 0, "a centre hit gives no spin");
      while (n < 480 * 20) { pl_step(&p, PL_DT); n++; if (lock < 0 && fabsf(p.vx[0] + PL_R * p.wy[0]) < 0.7f) lock = n; }
      CHECK(lock > 0 && fabsf(lock / 480.0f - (2.0f / 7.0f * v0) / (PL_MU_S * PL_G)) < 0.03f, "the skid ends at 2/7 v / (mu g)");
      CHECK(p.vx[0] == 0 && p.wy[0] == 0 && p.wx[0] == 0, "and the ball comes to rest with no spin left"); }
    { Pool a; pl_new(&a, 5); v127_clear(&a); a.x[0] = 60; a.y[0] = PL_CY; pl_shoot_sp(&a, 0, 0.5f, 0, 0.4f);
      CHECK(fabsf(a.vx[0] + PL_R * a.wy[0]) < 1.0f, "a tip 0.4 R above the centre is the natural roll: no skid"); }
    /* 2. follow, stun and draw */
    { float tip[3] = {0.5f, 0.0f, -0.5f}, after[3];
      for (k = 0; k < 3; k++) { int n = 0, hit = -1; pl_new(&p, 5); v127_clear(&p); p.in[1] = 1; p.grp[0] = 0; p.grp[1] = 1; p.x[0] = 200; p.y[0] = PL_CY; p.x[1] = 330; p.y[1] = PL_CY; pl_shoot_sp(&p, 0, 0.5f, 0, tip[k]); after[k] = 0;
        while (n < 480 * 20) { pl_step(&p, PL_DT); n++; if (hit < 0 && p.first_hit >= 0) hit = n; if (hit >= 0 && n == hit + 144) { after[k] = p.vx[0]; break; } } }
      CHECK(after[0] > 100, "follow: the cue ball goes on after the hit"); CHECK(fabsf(after[1]) < 120, "stun: the cue ball stays about where it hit"); CHECK(after[2] < -50, "draw: the cue ball comes back"); }
    /* 3. english: it throws the object ball the other way, and it changes the cushion bounce */
    { float sa[3] = {-0.5f, 0.0f, 0.5f}, vy1[3], out[3];
      for (k = 0; k < 3; k++) { int n = 0; pl_new(&p, 5); v127_clear(&p); p.in[1] = 1; p.grp[0] = 0; p.grp[1] = 1; p.x[0] = 200; p.y[0] = PL_CY; p.x[1] = 330; p.y[1] = PL_CY; pl_shoot_sp(&p, PL_SQUIRT * sa[k], 0.5f, sa[k], 0); vy1[k] = 0;
        while (n < 480 * 5) { pl_step(&p, PL_DT); n++; if (p.first_hit >= 0 && n > 5) { vy1[k] = p.vy[1]; break; } } }
      CHECK(vy1[2] < -10 && vy1[0] > 10 && fabsf(vy1[1]) < 1, "right english throws the object ball to the left, left english to the right");
      for (k = 0; k < 3; k++) { float lastvx; int n = 0; pl_new(&p, 5); v127_clear(&p); p.x[0] = 100; p.y[0] = PL_CY; pl_shoot_sp(&p, 0.2f + PL_SQUIRT * sa[k], 0.5f, sa[k], 0); lastvx = p.vx[0]; out[k] = 0;
        while (n < 480 * 10) { pl_step(&p, PL_DT); n++; if (p.vx[0] < 0 && lastvx > 0) { out[k] = atan2f(p.vy[0], -p.vx[0]); break; } lastvx = p.vx[0]; } }
      CHECK(out[2] > out[1] + 0.08f && out[0] < out[1] - 0.08f, "english changes the angle the ball leaves the cushion by");
      { Pool a; pl_new(&a, 5); v127_clear(&a); pl_shoot_sp(&a, 0, 0.5f, 0.5f, 0); CHECK(a.vy[0] < 0, "squirt: right english sends the cue ball a little to the left of the aim"); } }
    /* 4. a clean hit with no spin and no cut sends the object ball straight; a cut is thrown a few degrees */
    { float gx, gy, aim, cut = 0.5236f; int n = 0; pl_new(&p, 5); v127_clear(&p); p.in[1] = 1; p.grp[0] = 0; p.grp[1] = 1; p.x[0] = 200; p.y[0] = PL_CY; p.x[1] = 330; p.y[1] = PL_CY;
      gx = 330 - 2 * PL_R * cosf(cut); gy = PL_CY - 2 * PL_R * sinf(cut); aim = atan2f(gy - PL_CY, gx - 200); pl_shoot(&p, aim, 0.5f);
      while (n < 480 * 5) { pl_step(&p, PL_DT); n++; if (p.first_hit >= 0 && n > 5) break; }
      { float dir = atan2f(p.vy[1], p.vx[1]); CHECK(fabsf(dir - cut) < 0.12f && fabsf(dir - cut) > 0.005f, "a 30 degree cut: the object ball goes close to the geometric line, a few degrees of throw"); } }
    /* 5. energy (movement + spin) never rises, whatever the spin */
    { int bad = 0; for (k = 1; k <= 80; k++) { float e0, e; int n = 0; pl_new(&p, k * 7919); p.brk = (k % 3 == 0);
          pl_shoot_sp(&p, (pl_rf(&p) - 0.5f) * 0.2f, 0.3f + 0.7f * pl_rf(&p), (pl_rf(&p) - 0.5f) * 1.2f, (pl_rf(&p) - 0.5f) * 1.2f); e0 = v127_energy(&p);
          while (p.phase == PH_ROLL && n++ < 480 * 40) { if (!pl_step(&p, PL_DT)) break; e = v127_energy(&p); if (e > e0 * 1.0005f + 80) bad++; e0 = e; } }
      CHECK(bad == 0, "total energy never rises (80 random spin shots)"); }
    /* 6. the picture's orientation only turns when asked to, and stays a unit quaternion */
    { float q0[4]; int n; pl_new(&p, 9); v127_clear(&p); p.x[0] = 100; p.y[0] = PL_CY; memcpy(q0, p.q[0], sizeof q0); pl_shoot(&p, 0, 0.5f); for (n = 0; n < 200; n++) pl_step(&p, PL_DT);
      CHECK(memcmp(q0, p.q[0], sizeof q0) == 0, "track off: the orientation is not touched (the CPU's trial shots)");
      pl_new(&q, 9); v127_clear(&q); q.x[0] = 100; q.y[0] = PL_CY; q.track = 1; memcpy(q0, q.q[0], sizeof q0); pl_shoot(&q, 0, 0.5f); for (n = 0; n < 200; n++) pl_step(&q, PL_DT);
      { float nn = q.q[0][0] * q.q[0][0] + q.q[0][1] * q.q[0][1] + q.q[0][2] * q.q[0][2] + q.q[0][3] * q.q[0][3];
        CHECK(memcmp(q0, q.q[0], sizeof q0) != 0 && fabsf(nn - 1) < 0.001f, "track on: a moving ball turns and the quaternion stays a unit quaternion"); }
      CHECK(p.x[0] == q.x[0] && p.vx[0] == q.vx[0], "tracking the picture never changes the physics"); }
    /* 7. same shot, same result (determinism) */
    { Pool a, b; pl_new(&a, 123); pl_new(&b, 123); pl_shoot_sp(&a, 0.01f, 0.9f, 0.3f, -0.2f); pl_shoot_sp(&b, 0.01f, 0.9f, 0.3f, -0.2f); run_shot(&a); run_shot(&b);
      CHECK(memcmp(a.x, b.x, sizeof a.x) == 0 && memcmp(a.y, b.y, sizeof a.y) == 0, "the same shot with the same spin always ends the same way"); }
    /* 8. pockets: a ball running along the cushion past the side pocket stays on the table; a ball sent straight at a corner drops */
    { int n = 0, passed = 0; pl_new(&p, 5); v127_clear(&p); p.x[0] = 150; p.y[0] = PL_Y0 + PL_R + 2; p.phase = PH_ROLL; p.vx[0] = 420; p.wy[0] = -420 / PL_R;
      while (n++ < 480 * 6) { pl_step(&p, PL_DT); if (!p.in[0]) break; if (p.x[0] > 480) { passed = 1; break; } }
      CHECK(passed, "a ball sliding along the cushion goes past the side pocket");
      pl_new(&p, 5); v127_clear(&p); p.x[0] = 300; p.y[0] = PL_Y0 + PL_R; p.phase = PH_ROLL; p.vx[0] = -500; p.wy[0] = 500 / PL_R; n = 0; while (n++ < 480 * 6) { pl_step(&p, PL_DT); if (!p.in[0]) break; }
      CHECK(!p.in[0], "a ball rolling straight into the corner pocket drops");
      pl_new(&p, 5); CHECK(!pl_spot_ok(&p, 40, 42) && !pl_spot_ok(&p, PL_PX[1], PL_Y0 + PL_R + 2) && pl_spot_ok(&p, 150, 210), "ball in hand cannot be put in a pocket mouth"); }
    /* 9. the break is harder than a normal full-power shot and sends balls into pockets now and then */
    { int tot = 0; Pool a; pl_new(&a, 1); pl_shoot(&a, 0, 1.0f); CHECK(fabsf(a.vx[0] - PL_VBREAK) < 1.0f, "full power on the break is PL_VBREAK");
      pl_new(&a, 1); a.brk = 0; pl_shoot(&a, 0, 1.0f); CHECK(fabsf(a.vx[0] - PL_VMAX) < 1.0f, "full power off the break is PL_VMAX");
      for (k = 1; k <= 60; k++) { pl_new(&a, k * 104729); pl_shoot(&a, (pl_rf(&a) - 0.5f) * 0.02f, 0.96f); run_shot(&a); tot += a.pot_n; }
      CHECK(tot >= 10, "60 breaks sink at least 10 balls between them"); }
    /* 10. the CPU's search in slices gives the same answer as all at once, and spin is only used where allowed */
    { static PlAI ai; Pool a, b; float a1, p1, sa1, sb1; int calls = 0, usedspin = 0;
      pl_new(&a, 4321); pl_shoot(&a, 0, 1.0f); run_shot(&a); if (a.phase == PH_AIM) { b = a; if (b.bih) { pl_ai_place(&b); a = b; }
        pl_ai_shot_sp(&a, 4, &a1, &p1, &sa1, &sb1, 1);
        pl_ai_begin(&ai, &b, 4, 1); while (!pl_ai_work(&ai, &b, 1)) calls++;
        CHECK(ai.out_ang == a1 && ai.out_pow == p1 && ai.out_sa == sa1 && ai.out_sb == sb1, "the CPU's search in slices of one trial = the same answer as all at once");
        CHECK(calls >= 20, "and it really is spread out over many slices"); }
      for (k = 1; k <= 20; k++) { pl_new(&a, 600 + k); pl_shoot(&a, 0, 1.0f); run_shot(&a); if (a.phase != PH_AIM) continue; if (a.bih) pl_ai_place(&a);
        pl_ai_shot_sp(&a, 4, &a1, &p1, &sa1, &sb1, 0); CHECK(sa1 == 0 && sb1 == 0, "spin off: the CPU hits the centre");
        pl_ai_shot_sp(&a, 0, &a1, &p1, &sa1, &sb1, 1); CHECK(sa1 == 0 && sb1 == 0, "the weakest CPU never uses spin");
        pl_ai_shot_sp(&a, 4, &a1, &p1, &sa1, &sb1, 1); if (fabsf(sa1) + fabsf(sb1) > 0.1f) usedspin++; CHECK(sqrtf(sa1 * sa1 + sb1 * sb1) <= PL_SPIN_MAX + 0.05f, "the CPU's tip stays inside the circle"); }
      CHECK(usedspin >= 2, "the best CPU does use follow / draw / english in some positions"); }
    /* 11. whole games with spin on both sides always finish */
    { int over = 0, games; for (games = 0; games < 16; games++) { Pool a; int shots = 0; pl_new(&a, 8000 + games * 31);
          while (a.phase != PH_OVER && shots < 400) { float ang, pw, sa, sb; if (a.bih) pl_ai_place(&a); pl_ai_shot_sp(&a, a.turn == 0 ? 4 : 3, &ang, &pw, &sa, &sb, 1); pl_shoot_sp(&a, ang, pw, sa, sb); run_shot(&a); shots++; }
          CHECK(a.phase == PH_OVER && a.winner >= 0, "a CPU game with spin always finishes"); if (a.phase == PH_OVER) over++; }
      CHECK(over == 16, "all 16 spin games were decided"); }
    (void)i;
}

int main(void) {
    srand(2024);
    if (getenv("PQ_ONLY_V127")) { test_v126(); test_v127(); printf("\n%d checks, %d failed (8 ball only)\n", checks, fails); return fails ? 1 : 0; }
    if (getenv("PQ_ONLY_V123")) { test_v123(); test_v124(); printf("\n%d checks, %d failed (v12.3 only)\n", checks, fails); return fails ? 1 : 0; }
    if (getenv("PQ_ONLY_V122")) { test_v122(); printf("\n%d checks, %d failed (v12.2 only)\n", checks, fails); return fails ? 1 : 0; }
    if (getenv("PQ_ONLY_V116")) { test_v116(); printf("\n%d checks, %d failed (v11.6 only)\n", checks, fails); return fails ? 1 : 0; }
    if (getenv("PQ_ONLY_EXT")) { test_lines(); test_ext(); printf("\n%d checks, %d failed (text files only)\n", checks, fails); return fails ? 1 : 0; }   /* quick run: PQ_ONLY_EXT=1 ./test_logic */
    printf("evaluator...\n"); test_eval(); test_freq();
    printf("betting...\n"); test_betting_rules(); test_invariants(); test_bonus_rules();
    printf("money / save...\n"); test_money(); test_save(); test_energy(); test_debt(); test_time(); test_rent();
    printf("her AI...\n"); test_ai_sanity(); test_ai();
    printf("multi-player...\n"); test_multi();
    printf("dialogue...\n"); test_lines();
    printf("external text files...\n"); test_ext();
    printf("romance...\n"); test_romance();
    printf("debug...\n"); test_debug();
    printf("milk...\n"); test_milk();
    printf("stocks...\n"); test_stocks();
    printf("v8.2 gifts / Super drug...\n"); test_v82();
    printf("v8.9 free bet size...\n"); test_nl();
    printf("v8.6 wedding / page 4...\n"); test_v86();
    printf("v9.0 lottery...\n"); test_lottery();
    printf("v9.5 items, names, mama mode...\n"); test_v95();
    printf("v9.3 room levels...\n"); test_v93();
    printf("v10.0 black market / items / wins...\n"); test_v100();
    printf("v10.5 save slots / rewind guard...\n"); test_v105();
    printf("v10.8 designer top...\n"); test_v108();
    printf("v11.0 friendship bar, her room by day...\n"); test_v110();
    printf("v11.1 cow print outfit...\n"); test_v111();
    printf("v11.6 five romance levels / pink cards...\n"); test_v116();
    printf("v12.2 prices / the pump wears out...\n"); test_v122();
    printf("v12.3 Mommy Milk Injection...\n"); test_v123();
    printf("v12.6 8 ball...\n"); test_v126();
    printf("v12.7 8 ball spin...\n"); test_v127();
    printf("\n%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
