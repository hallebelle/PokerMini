/* Poker Queen - pure game logic (no SDL). Rules, betting, her AI, match + bank rules, save file.
   Kept separate so tests/test_logic.c can exercise it on its own. */
#ifndef PQ_LOGIC_H
#define PQ_LOGIC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>

/* ---------- table constants (fixed-limit heads-up) ---------- */
#define STACK      100     /* every hand starts from equal stacks */
#define SB_AMT     2
#define BB_AMT     4
#define SMALL_BET  4       /* preflop + flop */
#define BIG_BET    8       /* turn + river */
#define MAX_BETS   4       /* bet + 3 raises per round (preflop: the big blind counts as the first bet) */
#define TEST_BANK 300     /* TESTING: bank money of a NEW game (withdraw it at the ATM for the $300 shirt debt); set to 0 for the final build */
#define DEBUG_MODE 1       /* TESTING: 1 = the computer's online store has an X DEBUG panel (skip the Sophia match, +$50,000, new day, fill the romance bar); set to 0 for the final build (together with TEST_BANK) */
#define DEBUG_CASH 50000   /* what one press of "+$50,000" gives (cash) */
#define START_CASH 100     /* in your pocket when you arrive at the hotel */
#define DEBT 300           /* what Sophia wants for her stained shirt: paid in cash at her door before the first match */
#define MONEY_CAP  999999999   /* v13.15: cash and bank each stop at $999,999,999 (was $999,999); every sum of money is done in 64 bits (long is 32 bits on the Miyoo) */
/* energy (percent), drinks and the store. Energy is spent when a match STARTS (so walking out never dodges the cost) and only drinks restore it. */
#define ENERGY_MAX   100
#define PRICE_SODA   5     /* v12.4: was 1 */
#define PRICE_MONSTER 10   /* v12.4: was 3 */
#define REST_SODA    20
#define REST_MONSTER 50
#define INV_CAP      999999999   /* v13.15: the items in your room have no real limit any more (was 99; the store sells one at a time) */

enum { A_FOLD, A_CALL, A_RAISE };
enum { BN_STRAIGHT, BN_FLUSH, BN_FULL, BN_QUADS, BN_SFLUSH, BN_ROYAL, BN_N };
static const int BONUS_AMT[BN_N] = {20, 40, 80, 400, 1000, 2500};
static const char *const BONUS_NAME[BN_N] = {"STRAIGHT", "FLUSH", "FULL HOUSE", "QUADS", "STR FLUSH", "ROYAL FLUSH"};

/* ---------- hand evaluation (best 5 of 7) ---------- */
static inline int eval5(const int *c) {
    int r[5], i, j, t, fl = 1, cnt[13] = {0}, n4 = 0, n3 = 0, n2 = 0, d = 1, hi = -1, cat, k[5] = {0,0,0,0,0}, nk = 0, cn, rk;
    for (i = 0; i < 5; i++) { r[i] = c[i] % 13; cnt[r[i]]++; if (c[i]/13 != c[0]/13) fl = 0; }
    for (i = 0; i < 5; i++) for (j = i+1; j < 5; j++) if (r[j] > r[i]) { t = r[i]; r[i] = r[j]; r[j] = t; }
    for (i = 0; i < 13; i++) { if (cnt[i] > 1) d = 0; if (cnt[i] == 4) n4++; else if (cnt[i] == 3) n3++; else if (cnt[i] == 2) n2++; }
    if (d) { if (r[0] - r[4] == 4) hi = r[0]; else if (r[0] == 12 && r[1] == 3) hi = 3; }
    cat = (hi >= 0 && fl) ? 8 : n4 ? 7 : (n3 && n2) ? 6 : fl ? 5 : hi >= 0 ? 4 : n3 ? 3 : n2 == 2 ? 2 : n2 ? 1 : 0;
    for (cn = 4; cn >= 1; cn--) for (rk = 12; rk >= 0; rk--) if (cnt[rk] == cn) k[nk++] = rk;
    if (hi >= 0 && (cat == 4 || cat == 8)) { k[0] = hi; k[1] = k[2] = k[3] = k[4] = 0; }
    return (cat << 20) | (k[0] << 16) | (k[1] << 12) | (k[2] << 8) | (k[3] << 4) | k[4];
}
static inline int best7(const int *c) {
    int a, b, i, n, best = 0, h[5];
    for (a = 0; a < 7; a++) for (b = a+1; b < 7; b++) {
        for (i = 0, n = 0; i < 7; i++) if (i != a && i != b) h[n++] = c[i];
        i = eval5(h); if (i > best) best = i;
    }
    return best;
}
/* like best7, but also tells which 5 of the 7 cards make the best hand (bit i of *mask = card i), for the gold outline */
static inline int best7_pick(const int *c, int *mask) {
    int a, b, i, n, best = 0, h[5], ix[5];
    *mask = 0;
    for (a = 0; a < 7; a++) for (b = a+1; b < 7; b++) {
        for (i = 0, n = 0; i < 7; i++) if (i != a && i != b) { ix[n] = i; h[n++] = c[i]; }
        i = eval5(h);
        if (i > best) { best = i; *mask = (1<<ix[0]) | (1<<ix[1]) | (1<<ix[2]) | (1<<ix[3]) | (1<<ix[4]); }
    }
    return best;
}
/* which card bonus (if any) does a hand score pay? -1 = none (pair, two pair and trips pay nothing) */
static inline int bonus_kind(int score) {
    int cat = score >> 20;
    if (cat == 4) return BN_STRAIGHT;
    if (cat == 5) return BN_FLUSH;
    if (cat == 6) return BN_FULL;
    if (cat == 7) return BN_QUADS;
    if (cat == 8) return ((score >> 16) & 15) == 12 ? BN_ROYAL : BN_SFLUSH;
    return -1;
}

/* ---------- v7.8: stocks (online store tab STOCKS) ---------- */
enum { STK_NVIDIA, STK_GOOGLE, STK_OPENAI, STK_APPLE, STK_SONY, STK_SPACEX, STK_N };   /* v8.1: SONY and SPACEX added at the END (shares are saved by index) */
#define STK_CAP 999999999   /* shares per company: no real limit any more (was 99) */

/* ---------- the whole saved state ---------- */
#define SW_GOAL 20   /* v10.0: wins over Sophia that open the black market (and so the ring) */
#define MRW_N 8        /* v10.5: how many market reshuffles a career remembers (the oldest is forgotten first) */
typedef struct {
    int magic, ver, size;
    /* profile */
    int cash, bank, keyst, fee;                             /* keyst: Sophia's key card (0 not found yet, 1 you kept it, 2 you gave it back to the clerk); fee: buy-in of the match in progress (100/500/1000) */
    int m_played, m_won, m_lost, m_tied, best_net;
    int event, ex_played, ex_won, ex_big;                  /* event center unlocked, exhibition counters; ex_big = you have won the $1000 Exhibition once (the boss table opens) */
    int story, forfeit, hall_seen;                         /* story stage (ST_*), 1 = she is waiting to call you a coward, 1 = you have had the first look down the hall */
    int day, phase;                                        /* day counter (1, 2, ...) and time of day (PH_MORNING / PH_EVENING / PH_NIGHT); only saving moves the time on */
    int chal_day, rent_next;                               /* the day of your last match against Sophia (she plays you once a day); the day the next rent is due (RENT_DAYS apart) */
    int energy, soda, monster, rescue_used, last_drink;    /* energy 0..100, drinks in the inventory, free pity soda already given, reward of the last match (0 none, 1 soda, 2 monster) */
    /* romance (v6.2): rs = RS_*; room_up = the room level 0..3 (v9.3: base room, upgrade 1 with the computer, upgrade 2 with her door; v13.2: upgrade 3 with the playing room; rent 250 / 500 / 1000 / 2000); ring / pill = bought in the online store (one time each); pill_given = she got the pill (her pictures change);
       flower / stack = gifts in stock; gift_day = the day of the last gift (one gift per day); rom_lvl 1..5 (v11.6) and rom_pct 0..100 = the romance bar; home = the match in progress is a free one at home; last_rom = romance won by the last match */
    int rs, room_up, ring, pill, pill_given, flower, stack, gift_day, rom_lvl, rom_pct, home, last_rom;
    int pill_day, pill_seen;                               /* v6.4: the day she got the pill; 1 = the day-8 scene has played (her pictures changed, the night sneak-in at home is open) */
    /* v6.5 milk: pump = the breast pump is bought and not given yet; mpill = milk inducer pills in stock; milk_st 0 nothing / 1 first pill given (wait 2 days) / 2 "even more stuffy" scene seen (the pump may be given)
       / 3 the pump is given (the bottle gauge exists) / 4 you have received the first Baby Milk Bottle (v7.7: the milk topics of TEASE open); milk_day = the day of the first pill; milk_pct = the bottle gauge 0 or 50 (a full gauge becomes a bottle at once); bottle = Baby Milk Bottles in the inventory;
       milk_until = the last day the bottle works (0 = none); hint_day = the last day a day-6 / day-7 "asking her first" scene was played */
    int pump, mpill, milk_st, milk_day, milk_pct, bottle, milk_until, hint_day;
    /* match */
    int in_match, mlen, hand, net, button, style, tilt;   /* button: 0 = you, 1 = her (the button is the small blind) */
    int bn[BN_N];                                          /* card bonuses earned this match (paid only if the match is won) */
    int last_res, last_net, last_bonus, last_pay;          /* result of the match that just finished */
    /* hand */
    int deck[52];                                          /* 0,1 = yours  2,3 = hers  4..8 = board */
    int stage, nbets, acted, toact, over, fold, res, amt, showdown;
    int c[2], r[2], sc[2];                                 /* committed this hand, committed this round, showdown scores */
    /* v7.8 (keep these LAST: a version-12 save is exactly the part above): shares owned per company, and the seed of the price curves (0 = the fixed default seed) */
    int stk[STK_N], mseed;
    /* v8.2 (LAST again: a version-14 save is exactly the part above): new gifts and store items, the Super milk drug.
       lotion / pdeck / yoga / gold = gifts in stock; dring = the diamond ring is bought (one time, no use yet); sdrug = Super milk drugs in stock;
       sd_day = the day the last Super drug was given (0 = never); ask_day / ask_n = the day and how many times you already asked her for milk on it */
    int lotion, pdeck, yoga, gold, dring, sdrug, sd_day, ask_day, ask_n;
    /* v8.6 (LAST again: a version-15 save is exactly the part above): married = the diamond ring was given (page 4 of her menu: hug / kiss / sleep together / breastfeed);
       outfit = which outfit she wears in the page-4 scenes (0 = the after-day-8 outfit, 1 = the yoga outfit); yoga_given = you gave her the yoga outfit (she can change into it);
       mday = the last day she gave milk (pump, ASK FOR MILK or breastfeeding): she refuses to sleep beside you that night */
    int married, outfit, yoga_given, mday;
    /* v9.0 (LAST again: a version-16 save is exactly the part above): the lottery ticket from the Event Center counter.
       lot_price = the price of the ticket you hold (0 = no ticket), lot_day = the day you bought it, lot_win = 1 if it wins (rolled when you buy). The clerk tells you the result from the next day on. */
    int lot_price, lot_day, lot_win;
    /* v9.5 (LAST again: a version-18 save is exactly the part above): how you call her once you are married. 0 = SOPHIA (her usual self), 1 = MAMA (typed with R2 in her menu:
       MAMA or MAMA SOPHIA, see names.txt). Only mama mode has her own dialogue set and the BREASTFEED row. It is saved and stays until you type another name. */
    int call;
    /* v9.7 (LAST again: a version-19 save is exactly the part above): her memory of mama mode. mm_kind = the last mama-mode interaction (MK_*), mm_day = the day it happened
       (she greets you with it today and tomorrow, door and living room); kick_day = the last day you teased her and she threw you out (it used to be lost when the game closed). */
    int mm_kind, mm_day, kick_day;
    /* v10.0 (LAST again: a version-20 save is exactly the part above): how many times you have beaten Sophia in a real match (hotel tables 100/500/1000 and the $5000 boss match; ties, losses, forfeits and the free home matches do not count).
       20 wins open the BLACK MARKET (ring, pills) in the online store. It is never shown to the player. */
    int sw_wins;
    /* v10.5 (LAST again: a version-21 save is exactly the part above): save slots and the rewind guard.
       career = the id of this career (random, 0 = none: tests). Every slot of one career carries the same id; pokerqueen.prog remembers the furthest day reached per career.
       mrw_n / mrw_day[] / mrw_salt[] = the market reshuffles: from mrw_day[i] on the share prices use the seed mrw_salt[i] (loading a slot that is BEHIND the furthest day of its career
       reshuffles every day after the loaded one, so a price seen in another timeline is worth nothing; the days up to the loaded one keep their prices and the chart). */
    int career, mrw_n, mrw_day[MRW_N], mrw_salt[MRW_N];
    /* v10.8 (LAST again: a version-22 save is exactly the part above): the DESIGNER TOP, a third outfit. top = designer tops in stock (a gift, like the yoga outfit),
       top_given = you gave her one (she can change into it). outfit is now 0 = her usual (day-8) outfit, 1 = yoga, 2 = designer top. */
    int top, top_given;
    /* v11.0 (LAST again: a version-23 save is exactly the part above): the FRIENDSHIP bar. fr_wins = wins over her at her hotel tables since you beat her at the boss table
       (the boss win itself is not counted: the bar starts at ZERO). FR_GOAL wins fill it. Nothing else touches it. */
    int fr_wins;
    /* v11.1 (LAST again: a version-24 save is exactly the part above): the COW PRINT OUTFIT, a fourth outfit. cow = cow print outfits in stock (a gift, like the designer top),
       cow_given = you gave her one (she can change into it). outfit is now 0 = usual (day-8), 1 = yoga, 2 = designer top, 3 = cow print. */
    int cow, cow_given;
    /* v11.6 (LAST again: a version-25 save is exactly the part above): deck_given = you gave her the PINK CARD DECK while she lives with you. Only then Sophia's table has pink card backs (blue before).
       An older save where she already lives with you gets 1 (its table stays pink). */
    int deck_given;
    /* v12.2 (LAST again: a version-26 save is exactly the part above): the BREAST PUMP wears out. pump_life = how many bottles the pump in use can make in total (rolled PUMP_MIN..PUMP_MAX when she gets a pump; 0 = not rolled yet),
       pump_used = bottles made with it, pump_dead = 1 once she told you it is broken (only then the store lists a new pump). */
    int pump_life, pump_used, pump_dead;
    /* v12.3 (LAST again: a version-27 save is exactly the part above): the MOMMY MILK INJECTION (black market, $30,000, one time). inj = bought, not given yet; injected = she got it (permanent: the bottle gauge is gone);
       inj_day = the last day she gave milk (ASK FOR MILK, PUMP MILK or BREASTFEED): after the injection ONE of them per day in total. */
    int inj, injected, inj_day;
    /* v13.15 (LAST again: a version-29 save is exactly the part above): BANK INTEREST. int_day = the day the last interest was paid (it is paid once per new day, when you wake up in the morning),
       int_last = how much that payment was (the ATM shows it). A version-29 save gets 0 / 0 and is paid on its first morning. */
    int int_day, int_last;
    /* v13.16 (LAST again: a version-30 save is exactly the part above): the 8 ball TOURNAMENT is open once a week. pt_day = the day you last entered it (0 = never);
       the next entry is open 7 days later. A version-30 save gets 0 (open at once). */
    int pt_day;
} Game;

#define SAVE_MAGIC 0x50514132
#define SAVE_VER   31
#define SAVE_VER_30  30   /* v13.16: a version-30 save is the same struct without pt_day: it is loaded and upgraded in place (the tournament is open, nothing lost) */
#define SAVE_VER_29  29   /* v13.15: a version-29 save is the same struct without int_day / int_last: it is loaded and upgraded in place (nothing lost, interest starts with its first morning) */
#define SAVE_VER_28  28   /* v13.2: a version-28 save is the same struct (room_up only reached 2): it is loaded and upgraded in place, nothing is lost */
#define SAVE_VER_27  27   /* v12.3: a version-27 save is the same struct without inj / injected / inj_day: it is loaded and upgraded in place (no injection, nothing lost) */
#define SAVE_VER_26  26   /* v12.2: a version-26 save is the same struct without pump_life / pump_used / pump_dead: it is loaded and upgraded in place (the pump she has gets a fresh random life on its next use, nothing is lost) */
#define SAVE_VER_25  25   /* v11.6: a version-25 save is the same struct without deck_given: it is loaded and upgraded in place (pink cards stay if she lives with you; the romance bar is moved to the 5 levels, nothing is lost) */
#define SAVE_VER_24  24   /* v11.1: a version-24 save is the same struct without cow / cow_given: it is loaded and upgraded in place (no cow print outfit, nothing lost) */
#define SAVE_VER_23  23   /* v11.0: a version-23 save is the same struct without fr_wins: it is loaded and upgraded in place (the friendship bar starts at zero, nothing lost) */
#define SAVE_VER_22  22   /* v10.8: a version-22 save is the same struct without top / top_given: it is loaded and upgraded in place (no designer top, nothing lost) */
#define SAVE_VER_21  21   /* v10.5: a version-21 save is the same struct without career / mrw: it is loaded and upgraded in place (it gets a new career id, no reshuffle) */
#define SAVE_VER_20  20   /* v10.0: a version-20 save is the same struct without sw_wins: it is loaded and upgraded in place (0 wins; a game already past the proposal keeps its black market, see g_bm_open) */
#define SAVE_VER_19  19   /* v9.7: a version-19 save is the same struct without the mama memory fields: it is loaded and upgraded in place (no memory, nothing lost) */
#define SAVE_VER_18  18   /* v9.5: a version-18 save is the same struct without the call field: it is loaded and upgraded in place (she is called SOPHIA, nothing lost) */
#define SAVE_VER_17  17   /* v9.3: a version-17 save is the same struct, but room_up was 0 / 1 (1 = the one upgrade, with her door): it becomes level 2, a save past the shirt scene gets level 1 (it had the computer), nothing is lost */
#define SAVE_VER_16  16   /* v9.0: a version-16 save is the same struct without the lottery ticket fields: it is loaded and upgraded in place (no ticket, nothing lost) */
#define SAVE_VER_15  15   /* v8.6: a version-15 save is the same struct without married / outfit / yoga_given / mday: it is loaded and upgraded in place (nothing lost) */
#define SAVE_VER_14  14   /* v8.2: a version-14 save is the same struct without the new gift / Super drug fields: it is loaded and upgraded in place (nothing lost) */
#define SAVE_VER_OLD 12   /* v7.8: a save of this version is the same struct without stk[] and mseed: it is loaded and upgraded (no reset) */
#define SAVE_VER_13  13   /* v8.1: a version-13 save has stk[4] + mseed (4 companies): it is loaded and upgraded too (SONY / SPACEX start at 0 shares) */
#define STK_N13 4

/* story stages. The save only changes at safe points (finished match, finished scene, ATM). */
enum { ST_NEW, ST_HOME, ST_FIRST, ST_NAMED };   /* new game / intro done (the shirt is not paid yet) / the shirt is paid, she took her floor key card back and is the Exhibition boss (still unnamed) / she gave her name after you beat her at the Exhibition (the 3rd floor is open again) */
/* time of day. It only moves when you save: the sofa (morning -> evening -> night) and the bed (night -> next morning). */
enum { PH_MORNING, PH_EVENING, PH_NIGHT, PH_N };
#define DAY_MAX 99999
static inline int g_night(const Game *g) { return g->phase == PH_NIGHT; }
static inline void g_advance(Game *g) { if (g->phase < PH_NIGHT) g->phase++; else { g->phase = PH_MORNING; if (g->day < DAY_MAX) g->day++; } }

/* rent: RENT_AMT every RENT_DAYS days (due on day 7, 14, 21 ...). Pay at the front desk (cash) while she is there; at the start of the night what is still unpaid is taken
   from your cash first, then your bank. Not enough in both = game over. */
#define RENT_AMT  250      /* v9.3: the rent of the base room (room level 0) */
#define RENT_UP1_AMT 500   /* v9.3: room level 1 (upgrade 1, with the computer) */
#define RENT_UP_AMT 1000   /* v9.3: room level 2 (upgrade 2, her bedroom door) */
#define RENT_UP3_AMT 2000  /* v13.2: room level 3 (upgrade 3, the playing room) */
#define RENT_DAYS 7
static inline int g_rent_amt(const Game *g) { return g->room_up >= 3 ? RENT_UP3_AMT : g->room_up >= 2 ? RENT_UP_AMT : g->room_up == 1 ? RENT_UP1_AMT : RENT_AMT; }
static inline int g_rent_due(const Game *g) { return g->day >= g->rent_next; }
static inline int g_rent_pay_cash(Game *g) { int r = g_rent_amt(g); if (g->cash < r) return 0; g->cash -= r; g->rent_next += RENT_DAYS; return 1; }   /* the desk: cash only. 1 = paid */
/* the night starts: unpaid rent is taken. -1 = nothing was due, 1 = taken (*bank = how much of it came from the bank), 0 = cash + bank is not enough (game over, nothing is taken) */
static inline int g_rent_night(Game *g, int *bank) {
    int from_cash;
    *bank = 0;
    if (!g_rent_due(g)) return -1;
    if ((long long)g->cash + g->bank < g_rent_amt(g)) return 0;
    from_cash = g->cash < g_rent_amt(g) ? g->cash : g_rent_amt(g);
    g->cash -= from_cash; *bank = g_rent_amt(g) - from_cash; g->bank -= *bank; g->rent_next += RENT_DAYS;
    return 1;
}
/* Sophia plays you once a day (the day number, not the time of day); the match start counts, so walking out uses the day up too */
static inline int g_played_today(const Game *g) { return g->chal_day == g->day; }

static inline int rnd(int n) { return rand() % n; }
static inline float frnd(void) { return (float)rand() / ((float)RAND_MAX + 1.0f); }

static inline void g_init(Game *g) { memset(g, 0, sizeof *g); g->magic = SAVE_MAGIC; g->ver = SAVE_VER; g->size = (int)sizeof *g; g->energy = ENERGY_MAX; g->cash = START_CASH; g->day = 1; g->phase = PH_MORNING; g->rent_next = RENT_DAYS; g->rom_lvl = 1; }
static inline int g_career_id(void) { return (int)((((unsigned)time(NULL) * 2246822519u) ^ ((unsigned)rand() * 2654435761u)) & 0x7fffffffu) | 1; }   /* v10.5: a random career id (never 0) */
static inline void g_new(Game *g) { g_init(g); g->career = g_career_id(); g->bank = TEST_BANK; g->mseed = (int)(((unsigned)time(NULL) * 2654435761u) & 0x7fffffffu) | 1; }   /* a brand new career (title screen / after GAME OVER) */

/* ---------- betting ---------- */
static inline int g_pot(const Game *g) { return g->c[0] + g->c[1]; }
static inline int g_maxr(const Game *g) { return g->r[0] > g->r[1] ? g->r[0] : g->r[1]; }
static inline int g_size(const Game *g) { return g->stage < 2 ? SMALL_BET : BIG_BET; }
static inline int g_tocall(const Game *g, int p) { return g_maxr(g) - g->r[p]; }
static inline int g_can_raise(const Game *g) { return g->nbets < MAX_BETS; }
static inline int g_raise_cost(const Game *g, int p) { return g_tocall(g, p) + g_size(g); }

static inline void g_shuffle(Game *g) {
    int i; for (i = 0; i < 52; i++) g->deck[i] = i;
    for (i = 51; i > 0; i--) { int j = rnd(i + 1), t = g->deck[i]; g->deck[i] = g->deck[j]; g->deck[j] = t; }
}
/* a match against Sophia is always 10 hands and costs a BUY-IN in CASH, paid when it starts: 100, 500 or 1000.
   The buy-in is also the stake: every chip is worth fee/100 money (x1, x5, x10), and card bonuses are multiplied the same way.
   It costs no energy. A tie gives the buy-in back; a loss or walking out keeps it gone. */
#define SOPH_HANDS 10
#define SOPH_N 3
static const int SOPH_FEE[SOPH_N] = {100, 500, 1000};
static inline int g_mult(int fee) { return fee >= 100 ? fee / 100 : 1; }
#define SOPH_PRIZE_X 5   /* a win also pays a PRIZE of 5 x the buy-in, on top of the net chips and the card bonuses */
static inline int g_prize(int fee) { return fee * SOPH_PRIZE_X; }
static inline int g_can_afford_match(const Game *g, int fee) { return g->cash >= fee; }
static inline int g_can_play(const Game *g) { return g->cash >= SOPH_FEE[0]; }   /* the cheapest buy-in */
/* v12.4: a match at her 3rd-floor table (door or inside her room) also costs 20% ENERGY, on top of the buy-in. Charged when the match starts (walking out never dodges it, a tie
   gives back the buy-in but not the energy). The Baby Milk Bottle waives it like it waives the Exhibition's. The free match at home keeps HOME_ENERGY. */
#define SOPH_ENERGY 20
static inline void g_match_start(Game *g, int fee) {
    int i;
    g->cash -= fee; if (g->cash < 0) g->cash = 0; g->chal_day = g->day;
    g->in_match = 1; g->home = 0; g->mlen = SOPH_HANDS; g->fee = fee; g->hand = 0; g->net = 0; g->style = rnd(3); g->tilt = 0;
    for (i = 0; i < BN_N; i++) g->bn[i] = 0;
    g->over = 1;   /* no hand in progress until g_hand_start */
}
static inline int g_soph_energy_ok(const Game *g);
static inline void g_energy_spend(Game *g, int n);
/* v12.4: start a hotel match WITH its energy cost. 1 = started, 0 = not enough cash, -1 = not enough energy. (g_match_start itself charges cash only.) */
static inline int g_soph_start(Game *g, int fee) {
    if (g->cash < fee) return 0;
    if (!g_soph_energy_ok(g)) return -1;
    g_energy_spend(g, SOPH_ENERGY); g_match_start(g, fee);
    return 1;
}
/* deal a new hand (shuffles; tests/screenshots may overwrite g->deck afterwards) */
static inline void g_hand_start(Game *g) {
    int sb;
    g->hand++;
    g->button = g->hand == 1 ? rnd(2) : !g->button;
    g_shuffle(g);
    g->stage = 0; g->nbets = 1; g->acted = 0; g->over = 0; g->fold = 0; g->res = 0; g->amt = 0; g->showdown = 0;
    g->sc[0] = g->sc[1] = 0;
    sb = g->button;
    g->c[sb] = g->r[sb] = SB_AMT; g->c[!sb] = g->r[!sb] = BB_AMT;
    g->toact = sb;                     /* heads-up: the button posts the small blind and acts first preflop */
    if (g->tilt > 0) g->tilt--;
}
/* settle the hand, update the match net. Returns 1 */
static inline int g_finish(Game *g) {
    g->over = 1;
    if (g->fold) {
        if (g->fold == 1) { g->res = -1; g->amt = g->c[0]; } else { g->res = 1; g->amt = g->c[1]; }
    } else {
        int a[7], b[7], i;
        g->showdown = 1; g->stage = 4;
        for (i = 0; i < 5; i++) a[i] = b[i] = g->deck[4+i];
        a[5] = g->deck[0]; a[6] = g->deck[1]; b[5] = g->deck[2]; b[6] = g->deck[3];
        g->sc[0] = best7(a); g->sc[1] = best7(b);
        if (g->sc[0] > g->sc[1]) { g->res = 1; g->amt = g->c[1]; }
        else if (g->sc[0] < g->sc[1]) { g->res = -1; g->amt = g->c[0]; }
        else { g->res = 0; g->amt = 0; }
        if (g->res == 1) { int k = bonus_kind(g->sc[0]); if (k >= 0) g->bn[k]++; }   /* bonus: showdown wins only */
    }
    g->net += g->res * g->amt;
    if (g->res == 1 && g->amt >= 24) g->tilt = 3;   /* she's rattled by a big loss */
    return 1;
}
static inline int g_next_street(Game *g) {
    g->stage++;
    if (g->stage >= 4) return g_finish(g);
    g->r[0] = g->r[1] = 0; g->nbets = 0; g->acted = 0; g->toact = !g->button;   /* postflop the non-button acts first */
    return 0;
}
/* apply an action for g->toact. Returns 1 when the hand just ended. */
static inline int g_act(Game *g, int a) {
    int p = g->toact, pay;
    if (g->over) return 0;
    if (a == A_FOLD) { g->fold = p + 1; return g_finish(g); }
    if (a == A_RAISE && g_can_raise(g)) {
        pay = g_tocall(g, p) + g_size(g);
        g->r[p] += pay; g->c[p] += pay; g->nbets++; g->acted = 1; g->toact = !p; return 0;
    }
    pay = g_tocall(g, p); g->r[p] += pay; g->c[p] += pay; g->acted++;   /* call / check (also a capped "raise") */
    if (g->acted >= 2 && g->r[0] == g->r[1]) return g_next_street(g);
    g->toact = !p; return 0;
}

/* ---------- her AI ---------- */
/* equity (win chance, ties = half) of 'hole' against a random opponent hand, over nb known board cards */
static inline float equity(const int *hole, const int *board, int nb, int samples) {
    int used[52] = {0}, unk[52], nu = 0, i, s, need = 2 + (5 - nb); float w = 0;
    used[hole[0]] = used[hole[1]] = 1; for (i = 0; i < nb; i++) used[board[i]] = 1;
    for (i = 0; i < 52; i++) if (!used[i]) unk[nu++] = i;
    for (s = 0; s < samples; s++) {
        int a[7], b[7], sa, sb;
        for (i = 0; i < need; i++) { int j = i + rnd(nu - i), t = unk[i]; unk[i] = unk[j]; unk[j] = t; }
        a[0] = hole[0]; a[1] = hole[1]; b[0] = unk[0]; b[1] = unk[1];
        for (i = 0; i < nb; i++) a[2+i] = b[2+i] = board[i];
        for (i = nb; i < 5; i++) a[2+i] = b[2+i] = unk[2 + i - nb];
        sa = best7(a); sb = best7(b);
        w += sa > sb ? 1.0f : sa == sb ? 0.5f : 0.0f;
    }
    return w / (float)samples;
}
/* preflop equity is cached per hand class (rank pair + suited), computed on first use */
static inline float pre_equity(const int *hole) {
    static float cache[338]; static int init = 0;
    int r0 = hole[0] % 13, r1 = hole[1] % 13, hi = r0 > r1 ? r0 : r1, lo = r0 > r1 ? r1 : r0, su = hole[0] / 13 == hole[1] / 13, k;
    if (!init) { int i; for (i = 0; i < 338; i++) cache[i] = -1.0f; init = 1; }
    k = (hi * 13 + lo) + su * 169;
    if (cache[k] < 0) {
        int h[2], b[1] = {0};
        h[0] = hi;                          /* a representative hand of the class: first card is a spade */
        h[1] = su ? lo : 13 + lo;           /* suited = also a spade, otherwise a heart */
        cache[k] = equity(h, b, 0, 1500);
    }
    return cache[k];
}
typedef struct { float agg, bluff, tight; } Pers;
static inline Pers ai_pers(const Game *g) {
    static const float A[3] = {0.00f, 0.03f, 0.06f}, B[3] = {0.08f, 0.12f, 0.16f}, T[3] = {0.02f, 0.00f, 0.01f};
    int s = g->style % 3; Pers p; p.agg = A[s]; p.bluff = B[s]; p.tight = T[s];
    if (g->tilt > 0) { p.agg += 0.04f; p.bluff *= 1.5f; }   /* flustered after a big loss: looser and more aggressive */
    return p;
}
/* her decision (she is player 1). Uses only what she can see: her cards, the board, the betting. */
static inline int ai_decide_with(const Game *g, Pers pe) {
    int hole[2], board[5], nb = g->stage == 0 ? 0 : g->stage + 2, i, tocall = g_tocall(g, 1), pot = g_pot(g), aggr;
    float e, odds;
    hole[0] = g->deck[2]; hole[1] = g->deck[3]; for (i = 0; i < nb; i++) board[i] = g->deck[4+i];
    e = nb == 0 ? pre_equity(hole) : equity(hole, board, nb, 400);
    aggr = g->nbets - (g->stage == 0 ? 1 : 0);                 /* bets/raises in this round (the blind is not aggression) */
    if (tocall > 0 && aggr > 0) e -= 0.045f * (float)aggr;    /* a betting opponent has a better-than-random hand */
    e += (frnd() - 0.5f) * 0.12f;                              /* noise so she isn't a machine */
    if (tocall == 0) {
        if (e > 0.88f && frnd() < 0.20f) return A_CALL;        /* occasional slow-play */
        if (g_can_raise(g) && e > 0.60f - pe.agg) return A_RAISE;
        if (g_can_raise(g) && g->stage > 0 && e < 0.50f && frnd() < pe.bluff) return A_RAISE;   /* bluff */
        return A_CALL;
    }
    odds = (float)tocall / (float)(pot + tocall);
    if (g_can_raise(g) && e > 0.70f - pe.agg && !(e < 0.82f && frnd() < 0.15f)) return A_RAISE;
    if (g_can_raise(g) && e > 0.40f && frnd() < pe.bluff * 0.4f) return A_RAISE;   /* semi-bluff raise */
    if (e > odds + 0.03f + pe.tight) return A_CALL;
    return A_FOLD;
}
static inline int ai_decide(const Game *g) { return ai_decide_with(g, ai_pers(g)); }

/* ---------- romance (v6.2): the ring, the room upgrade, moving in, gifts and the romance bar ----------
   Story: get 20 wins over her (hotel matches and the boss match), buy the ring in the BLACK MARKET of the online store (it opens after the 20th win), beat her at the Exhibition (story = ST_NAMED, you have her floor key card again) and PROPOSE at her door.
   She accepts the ring and asks if you want her to live with you. YES: she visits your room and demands a bigger one (RS_VISIT -> RS_UPG), the desk then sells the upgrade (ROOM_UP_COST,
   rent 300). Upgraded (RS_READY): ask her at her door and she moves in (RS_LIVING). NO (RS_NO): no upgrade, she stays on the 3rd floor, nothing is unlocked, but you can still ask her to move in later.
   Living with you: the 3rd floor is closed, you knock on her bedroom door in your living room. There the romance bar, one gift a day and free matches (10 hands +5%, 20 hands +10%, 10% energy, no money, no drink) are available. */
enum { RS_NONE, RS_VISIT, RS_UPG, RS_READY, RS_NO, RS_LIVING, RS_N };
#define ROOM_UP1_COST 5000   /* v9.3: upgrade 1 (room level 0 -> 1: the computer appears) */
#define ROOM_UP_COST 10000   /* v9.3: upgrade 2 (level 1 -> 2: the Sophia flow, her bedroom door) */
#define ROOM_UP3_COST 20000  /* v13.2: upgrade 3 (level 2 -> 3: the playing room behind a new door in the living room) */
#define ROOM_LVL_MAX 3       /* v13.2: the last room level */
#define ROM_LVLS 5   /* v11.6: five romance levels (were 3); every unlock that waited for a full level 3 now waits for a full level 5 */
/* v11.6: each level is harder than the one before: a gift / chat / win adds only this percent of its value to the bar (at least 1 point). Level 1 is as before. */
static const int ROM_SCALE[ROM_LVLS] = {100, 90, 75, 60, 50};
static inline int g_rom_eff(const Game *g, int p) { int l = g->rom_lvl < 1 ? 1 : g->rom_lvl > ROM_LVLS ? ROM_LVLS : g->rom_lvl, e; if (p <= 0) return 0; e = p * ROM_SCALE[l - 1] / 100; return e < 1 ? 1 : e; }
#define ROM_FLOWER 10        /* percent of the bar */
#define ROM_STACK  20
#define ROM_WIN10   5
#define ROM_WIN20  10
#define HOME_ENERGY 10
enum { SH_FLOWER, SH_STACK, SH_PILL, SH_RING, SH_PUMP, SH_MILK, SH_LOTION, SH_DECK, SH_YOGA, SH_GOLD, SH_DIAMOND, SH_SUPER, SH_TOP, SH_COW, SH_INJECT, SH_N };   /* the pump, the milk inducer pill (v6.5) and the Super milk drug (v8.2) only appear in the store after the day-8 scene */
static const int SH_PRICE[SH_N] = {250, 1000, 15000, 12000, 500, 750, 300, 600, 3000, 6000, 50000, 2250, 3500, 4000, 30000};   /* v12.2: pill 15000 (was 10000), milk pill 750 (was 500), Super drug 2250 (was 1500), yoga 3000 (was 1500), designer top 3500 (was 2500), cow print 4000 (was 3000: the dearest outfit). v8.0: flower and money stack halved. v8.2: body lotion, pink card deck, yoga outfit, gold bar, diamond ring, Super milk drug */
enum { GF_FLOWER, GF_STACK, GF_PILL, GF_PUMP, GF_MILK, GF_LOTION, GF_DECK, GF_YOGA, GF_GOLD, GF_SUPER, GF_TOP, GF_COW, GF_INJECT, GF_N };
/* v8.2: what a gift adds to the romance bar (percent). The more a gift costs per percent, the more days it saves (only one gift counts per day). */
#define ROM_LOTION 12
#define ROM_DECK   15
#define ROM_YOGA   0    /* v11.3: the three outfits give NO romance any more (they were +25 / +30 / +35); they only unlock at max romance (g_outfit_unlocked) */
#define ROM_TOP    0
#define ROM_COW    0
#define ROM_GOLD   50
static inline int g_gift_gain(int kind) { return kind == GF_FLOWER ? ROM_FLOWER : kind == GF_STACK ? ROM_STACK : kind == GF_LOTION ? ROM_LOTION : kind == GF_DECK ? ROM_DECK : kind == GF_YOGA ? ROM_YOGA : kind == GF_TOP ? ROM_TOP : kind == GF_COW ? ROM_COW : kind == GF_GOLD ? ROM_GOLD : 0; }
/* v12.2: the breast pump wears out. Every bottle it makes (PUMP MILK, ASK FOR MILK, or a milk pill that fills the gauge before the wedding) uses it once; a pump makes PUMP_MIN..PUMP_MAX bottles (never fewer than 9),
   then the NEXT time you ask she tells you it is broken (no bottle, nothing used up). Only then the store lists a new pump (pump_dead); you hand the new one over from her give page. */
#define PUMP_MIN 9
#define PUMP_MAX 15
static inline int g_pump_roll(void) { return PUMP_MIN + rnd(PUMP_MAX - PUMP_MIN + 1); }
static inline int g_pump_wear(Game *g) {   /* 0 = the pump works (one use counted), 14 = it is worn out: she tells you (pump_dead = 1), the caller makes no bottle */
    if (g->pump_life <= 0) { g->pump_life = g_pump_roll(); g->pump_used = 0; g->pump_dead = 0; }   /* a pump from an older save (or a test fixture): the life is rolled now */
    if (g->pump_dead || g->pump_used >= g->pump_life) { g->pump_dead = 1; return 14; }
    g->pump_used++; return 0;
}
#define MILK_WAIT 2        /* days between the first milk pill and her "even more stuffy" scene */
#define MILK_STEP 50       /* v7.6: one milk pill fills 50% of the bottle gauge: 2 pills make a bottle (was 10% / 10 pills) */
#define MILK_DAYS 2        /* a Baby Milk Bottle stops the energy from dropping for this many days (today and tomorrow) */
/* v7.7: CHAT and TEASE in her room. CHAT is there once she lives with you (a chat can fail; a good one gives CHAT_ROM % romance, at most CHAT_DAY_MAX good ones count per day).
   TEASE exists only after the day-8 scene and never gives romance; she always throws you out afterwards. The milk topics of TEASE open with the first bottle (milk_st 4). */
#define CHAT_ROM       2    /* percent of the bar for a chat that went well */
#define CHAT_FAIL_PCT  25   /* chance (percent) that a chat goes badly: she snaps, no romance */
#define CHAT_DAY_MAX   4    /* good chats per day that still fill the bar (v13.10: never reached any more, CHAT_LIMIT is lower) */
#define CHAT_LIMIT     3    /* v13.10: chats with her per day (good or bad). After the last one she says she has talked enough, leaves the living room for her bedroom and stays there for the rest of the day; CHAT answers with her refusal until the next day */
static inline int g_chat_open(const Game *g) { return g->rs == RS_LIVING; }
static inline int g_tease_open(const Game *g) { return g->rs == RS_LIVING && g->pill_seen; }
static inline int g_tease_milk_open(const Game *g) { return g->rs == RS_LIVING && g->pill_seen && g->milk_st >= 4; }
static inline int g_rom_on(const Game *g) { return g->rs == RS_LIVING; }                   /* she lives with you: the bar, the gifts and the home matches exist */
static inline int g_rom_full(const Game *g) { return g->rom_lvl >= ROM_LVLS && g->rom_pct >= 100; }
/* v11.3: the outfits (yoga, designer top, cow print) are for sale, giftable and wearable only once she lives with you and the romance bar is FULL at the last level (the same moment the beauty pill unlocks).
   They give no romance, do not use the one gift a day and are not blocked by a full bar. A game that already gave an outfit keeps it (the *_given flags stay). */
static inline int g_pink_cards(const Game *g) { return g->rs == RS_LIVING && g->deck_given; }   /* v11.6: pink card backs at Sophia's table only once she lives with you AND you gave her the pink deck */
static inline int g_outfit_unlocked(const Game *g) { return g->rs == RS_LIVING && g_rom_full(g); }
/* v13.8: the designer top and the cow outfit (not yoga) are also tied to MAMA mode: they are listed in the store and giftable only while the name is MAMA (married and call == 1, the same test as g_mama,
   written out here because g_mama is declared further down). Switching back to SOPHIA hides them again; an outfit she already owns stays in her wardrobe; stock you already bought stays in the inventory. No save change. */
static inline int g_outfit_mama(const Game *g) { return g->rs == RS_LIVING && g->married && g->call == 1; }
static inline int g_outfit_ok(const Game *g, int mama_only) { return g_outfit_unlocked(g) && (!mama_only || g_outfit_mama(g)); }
static inline int g_room_lvl(const Game *g) { return g->rs == RS_LIVING && g->room_up < 2 ? 2 : g->room_up; }   /* v9.3: the room picture / map in use: 0 base room, 1 upgrade 1 (lamp, computer), 2 upgrade 2 (her door); level 2 is also used once she lives here */
static inline int g_computer(const Game *g) { return g_room_lvl(g) >= 1; }   /* v9.3: the computer (online store) exists only from upgrade 1 on */
static inline int g_upgrade_cost(const Game *g) { return g->room_up == 0 ? ROOM_UP1_COST : g->room_up == 1 ? ROOM_UP_COST : ROOM_UP3_COST; }   /* the price of the NEXT level */
/* v9.3: upgrade 1 is on offer at the desk from the moment the shirt is paid for (story >= ST_HOME); upgrade 2 once she has been asked to live with you (YES = RS_UPG, NOT NOW = RS_NO) and level 1 is bought */
static inline int g_upgrade_avail(const Game *g) { return (g->room_up == 0 && g->story >= ST_HOME) || (g->room_up == 1 && (g->rs == RS_UPG || g->rs == RS_NO)) || g->room_up == 2; }   /* v13.2: upgrade 3 is on offer as soon as level 2 is bought (with or without her living here) */
static inline int g_can_propose(const Game *g) { return g->story >= ST_NAMED && g->ring && g->rs == RS_NONE; }
static inline int g_can_askmove(const Game *g) { return g->story >= ST_NAMED && (g->rs == RS_UPG || g->rs == RS_READY || g->rs == RS_NO); }   /* v9.3: she moves in only into the level-2 room (g_move_ok); otherwise she sends you off to upgrade first */
static inline int g_move_ok(const Game *g) { return g_can_askmove(g) && g->room_up >= 2; }
static inline int g_floor3_open(const Game *g) { return g->rs != RS_LIVING; }
static inline int g_pay_online(Game *g, int price) {   /* the online store: cash first, then the bank. 0 ok, 1 not enough */
    int c;
    if ((long long)g->cash + g->bank < price) return 1;
    c = g->cash < price ? g->cash : price; g->cash -= c; g->bank -= price - c; return 0;
}
/* v10.0: the BLACK MARKET tab (ring, pills) of the online store exists after SW_GOAL wins over her. A game that already bought the ring or is past the proposal (rs != RS_NONE) keeps it. */
static inline int g_bm_open(const Game *g) { return g->sw_wins >= SW_GOAL || g->ring || g->rs != RS_NONE; }
/* v11.0: FRIENDSHIP gauge (blue hearts). fr_wins = wins over Sophia at her hotel tables since the boss win (the boss win itself does not count, so the bar starts at zero): FR_GOAL (20) wins fill it.
   Nothing else fills it (it is separate from sw_wins, the black-market counter). Tier 1..4 = 0-4, 5-9, 10-14, 15-19 wins; tier 5 = the full bar (20 wins). From the boss win on (story >= ST_NAMED) she lets you into her room by day (g_visit_ok). */
#define FR_GOAL 20
#define FR_TIERS 5
static inline int g_fr_wins(const Game *g) { return g->fr_wins < 0 ? 0 : g->fr_wins > FR_GOAL ? FR_GOAL : g->fr_wins; }
static inline int g_fr_pct(const Game *g) { return g_fr_wins(g) * 100 / FR_GOAL; }
static inline int g_fr_tier(const Game *g) { int t = 1 + g_fr_wins(g) / 5; return g_fr_wins(g) >= FR_GOAL ? FR_TIERS : t > FR_TIERS - 1 ? FR_TIERS - 1 : t; }   /* 0-4 > 1, 5-9 > 2, 10-14 > 3, 15-19 > 4, 20 (full) > 5 */
static inline int g_fr_full(const Game *g) { return g_fr_wins(g) >= FR_GOAL; }
static inline int g_visit_ok(const Game *g) { return g->story >= ST_NAMED && g_floor3_open(g) && !g_night(g); }
/* v10.0: the ITEMS tab (gifts, pump) exists only once she lives with you; before that the store shows the STOCKS tab only */
static inline int g_items_open(const Game *g) { return g->rs == RS_LIVING; }
static inline int g_shop_black(int item) { return item == SH_RING || item == SH_PILL || item == SH_MILK || item == SH_SUPER || item == SH_INJECT; }
static inline int g_shop_avail(const Game *g, int item) {   /* the pump, the milk pill and the Super drug are listed only after the day-8 scene */
    if (g_shop_black(item) ? !g_bm_open(g) : !g_items_open(g)) return 0;
    if (item == SH_YOGA || item == SH_TOP || item == SH_COW) return g_outfit_ok(g, item != SH_YOGA);   /* v11.3; v13.8: top and cow also need MAMA mode */
    if ((item == SH_MILK || item == SH_SUPER) && g->injected) return 0;   /* v12.3: she does not need them any more */
    return (item == SH_PUMP || item == SH_MILK || item == SH_SUPER || item == SH_INJECT) ? g->pill_seen : 1;
}
/* v10.0: a one-time item that is already bought is not listed any more (it used to show as SOLD OUT) */
static inline int g_shop_once_done(const Game *g, int item) { return (item == SH_RING && g->ring) || (item == SH_PILL && g->pill) || (item == SH_PUMP && (g->pump || (g->milk_st >= 3 && !g->pump_dead))) || (item == SH_DIAMOND && g->dring) || (item == SH_INJECT && (g->inj || g->injected)); }
static inline int g_shop_buy(Game *g, int item) {   /* 0 bought, 1 not enough money, 2 sold out (the ring, the pill and the pump are one time), 3 stock full */
    if (item < 0 || item >= SH_N || !g_shop_avail(g, item)) return 1;
    if (item == SH_PUMP && (g->pump || (g->milk_st >= 3 && !g->pump_dead))) return 2;
    if (item == SH_MILK && g->mpill >= INV_CAP) return 3;
    if (item == SH_RING && g->ring) return 2;
    if (item == SH_PILL && g->pill) return 2;
    if (item == SH_INJECT && (g->inj || g->injected)) return 2;   /* v12.3: one time */
    if (item == SH_FLOWER && g->flower >= INV_CAP) return 3;
    if (item == SH_STACK && g->stack >= INV_CAP) return 3;
    if (item == SH_DIAMOND && g->dring) return 2;
    if (item == SH_LOTION && g->lotion >= INV_CAP) return 3;
    if (item == SH_DECK && g->pdeck >= INV_CAP) return 3;
    if (item == SH_YOGA && g->yoga >= INV_CAP) return 3;
    if (item == SH_TOP && g->top >= INV_CAP) return 3;
    if (item == SH_COW && g->cow >= INV_CAP) return 3;
    if (item == SH_GOLD && g->gold >= INV_CAP) return 3;
    if (item == SH_SUPER && g->sdrug >= INV_CAP) return 3;
    if (g_pay_online(g, SH_PRICE[item])) return 1;
    if (item == SH_FLOWER) g->flower++; else if (item == SH_STACK) g->stack++; else if (item == SH_PILL) g->pill = 1; else if (item == SH_PUMP) g->pump = 1; else if (item == SH_MILK) g->mpill++;
    else if (item == SH_LOTION) g->lotion++; else if (item == SH_DECK) g->pdeck++; else if (item == SH_YOGA) g->yoga++; else if (item == SH_TOP) g->top++; else if (item == SH_COW) g->cow++; else if (item == SH_GOLD) g->gold++;
    else if (item == SH_DIAMOND) g->dring = 1; else if (item == SH_SUPER) g->sdrug++; else if (item == SH_INJECT) g->inj = 1; else g->ring = 1;
    return 0;
}

/* v7.8 STOCKS. The price of a company on a day is a pure function of (seed, company, day): a slow trend (a new random knot every 9 days, smoothly joined)
   plus a daily jitter. Nothing but the seed is saved, so loading a save never re-rolls the market. v13.15: the prices are HIGH now: every price stays between
   $3,000 and $5,000 (the largest swing is base x (1 +- amp%)): APPLE 3400 +-9% = 3094..3706, GOOGLE 3600 +-14% = 3096..4104, SONY 3800 +-15% = 3230..4370,
   OPENAI 4000 +-20% = 3200..4800, NVIDIA 4200 +-18% = 3444..4956, SPACEX 4500 +-11% = 4005..4995 (the dearest). A day only moves on when you save at the bed (g_advance). */
#define STK_MIN_PRICE 3000   /* v13.15: no price is ever at or below this ... */
#define STK_MAX_PRICE 5000   /* ... or above this (tests check every company, every day, every seed) */
static const int STK_BASE[STK_N] = {4200, 3600, 4000, 3400, 3800, 4500};   /* NVIDIA, GOOGLE, OPENAI, APPLE, SONY, SPACEX (same order as before: SPACEX is still the dearest, APPLE the cheapest) */
static const int STK_AMP[STK_N]  = {  18,   14,   20,    9,   15,   11};   /* percent: the most a price can be above or below its base */
static inline unsigned stk_h(unsigned a, unsigned b, unsigned c) {
    unsigned x = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA6Bu ^ (c + 1u) * 0xC2B2AE35u;
    x ^= x >> 15; x *= 0x2C1B3C6Du; x ^= x >> 12; x *= 0x297A2D39u; x ^= x >> 15; return x;
}
static inline int stk_n(unsigned seed, int s, int k) { return (int)(stk_h(seed, (unsigned)s, (unsigned)k) % 2001u) - 1000; }   /* -1000 .. 1000 */
/* v10.5: the price seed of a day: the career seed, or the newest reshuffle that starts on or before that day (mrw_day[] is ascending) */
static inline unsigned stk_seed(const Game *g, int day) {
    unsigned s = g->mseed ? (unsigned)g->mseed : 20260101u; int i, n = g->mrw_n < MRW_N ? g->mrw_n : MRW_N;
    for (i = 0; i < n; i++) if (day >= g->mrw_day[i]) s = (unsigned)g->mrw_salt[i];
    return s;
}
static inline int stk_price(const Game *g, int s, int day) {
    unsigned seed = stk_seed(g, day); int d = day < 1 ? 1 : day, k = d / 9, f = d % 9, t, sm, a, b, trend, jit, fx;
    if (s < 0 || s >= STK_N) return 0;
    a = stk_n(seed, s, k); b = stk_n(seed, s, k + 1);
    t = f * 1000 / 9; sm = (int)((long long)t * t * (3000 - 2 * t) / 1000000000LL);   /* smoothstep 0..1000 */
    trend = a + (b - a) * sm / 1000; jit = stk_n(seed, s + 16, d);
    fx = (700 * trend + 300 * jit) / 1000;                                       /* -1000 .. 1000 */
    { int p = STK_BASE[s] + (int)((long long)STK_BASE[s] * STK_AMP[s] * fx / 100000LL); return p < 1 ? 1 : p; }
}
static inline int stk_now(const Game *g, int s) { return stk_price(g, s, g->day); }
static inline int stk_change(const Game *g, int s) { return g->day <= 1 ? 0 : stk_now(g, s) - stk_price(g, s, g->day - 1); }   /* against yesterday */
/* buy / sell ONE share at today's price. Buy pays cash first, then the bank. Codes: 0 done, 1 not enough money (buy) / none owned (sell), 3 at the share cap (buy) / the cash would pass the cap (sell) */
static inline int g_stk_buy(Game *g, int s) {
    if (s < 0 || s >= STK_N) return 1;
    if (g->stk[s] >= STK_CAP) return 3;
    if (g_pay_online(g, stk_now(g, s))) return 1;
    g->stk[s]++; return 0;
}
static inline int g_stk_sell(Game *g, int s) {
    int p; if (s < 0 || s >= STK_N || g->stk[s] <= 0) return 1;
    p = stk_now(g, s); if ((long long)g->cash + p > MONEY_CAP) return 3;
    g->stk[s]--; g->cash += p; return 0;
}
static inline long long g_stk_value(const Game *g) { long long v = 0; int s; for (s = 0; s < STK_N; s++) v += (long long)g->stk[s] * stk_now(g, s); return v; }
static inline int g_upgrade_buy(Game *g) {   /* the desk: cash only (like the rent). 0 ok, 1 not enough cash, 2 not available */
    int c;
    if (!g_upgrade_avail(g)) return 2;
    c = g_upgrade_cost(g);
    if (g->cash < c) return 1;
    g->cash -= c; g->room_up++;
    if (g->room_up == 2) g->rs = RS_READY;   /* upgrade 2 opens "ask her to move in"; upgrade 1 changes nothing in the story */
    return 0;
}
static inline void g_rom_bump(Game *g, int p) {   /* the bar fills by p points; reaching 100 starts the next level at 0 (no carry); the last level stays full */
    if (g->rom_lvl < 1) g->rom_lvl = 1;
    g->rom_pct += p;
    if (g->rom_pct >= 100) { if (g->rom_lvl < ROM_LVLS) { g->rom_lvl++; g->rom_pct = 0; } else g->rom_pct = 100; }
}
static inline void g_rom_add(Game *g, int p) { g_rom_bump(g, g_rom_eff(g, p)); }   /* v11.6: p is the full value; the level scales it (g_rom_eff) */
static inline int g_gifted_today(const Game *g) { return g->gift_day == g->day; }
static inline int g_gift_is_outfit(int kind) { return kind == GF_YOGA || kind == GF_TOP || kind == GF_COW; }   /* v11.3 */
static inline int g_gift_stock(const Game *g, int kind) { return kind == GF_FLOWER ? g->flower : kind == GF_STACK ? g->stack : kind == GF_PUMP ? g->pump : kind == GF_MILK ? g->mpill : kind == GF_LOTION ? g->lotion : kind == GF_DECK ? g->pdeck : kind == GF_YOGA ? g->yoga : kind == GF_TOP ? g->top : kind == GF_COW ? g->cow : kind == GF_GOLD ? g->gold : kind == GF_SUPER ? g->sdrug : kind == GF_INJECT ? g->inj : (g->pill && !g->pill_given); }
/* v8.2: the SUPER MILK DRUG (black market). Given on day D it makes her bottle gauge full for SUPER_DAYS days (D and D+1) and opens ASK FOR MILK in her menu: SUPER_ASKS times a day,
   each one gives a Baby Milk Bottle. While it works, and for SUPER_BLOCK more days after it ends (D .. D+3), she refuses the normal milk inducer pill. It needs the pump (milk_st >= 3).
   A second drug is refused while the first one still works; in the two days after it you may give one again. */
#define SUPER_DAYS  2
#define SUPER_BLOCK 2
#define SUPER_ASKS  2
static inline int g_super_active(const Game *g) { return g->sd_day > 0 && g->day >= g->sd_day && g->day <= g->sd_day + SUPER_DAYS - 1; }
static inline int g_super_left(const Game *g) { return g_super_active(g) ? g->sd_day + SUPER_DAYS - g->day : 0; }   /* days of the effect left, today included */
static inline int g_super_block(const Game *g) { return g->sd_day > 0 && g->day >= g->sd_day && g->day <= g->sd_day + SUPER_DAYS + SUPER_BLOCK - 1; }   /* the normal milk pill is refused */
static inline int g_asks_left(const Game *g) { int used; if (g->injected) return g->inj_day != g->day ? 1 : 0; used = g->ask_day == g->day ? g->ask_n : 0; return g_super_active(g) ? (used >= SUPER_ASKS ? 0 : SUPER_ASKS - used) : 0; }
static inline int g_inj_free(const Game *g) { return g->injected && g->inj_day != g->day; }   /* v12.3: she has not given milk today (ask / pump / breastfeed: one of them a day) */
static inline int g_milk_gauge_pct(const Game *g) { return g_super_active(g) ? 100 : g->milk_pct; }   /* what the gauge shows */
/* v6.5: the pump and the milk inducer pill are NOT gifts of the day: they ignore the one-gift-a-day rule (and do not use it up) and still work when her heart is full.
   The milk pill: the first one starts the 2-day wait (milk_st 1), nothing more can be given until her \"even more stuffy\" scene (2) and the pump (3); then each pill is +10% on the bottle gauge and a full gauge
   becomes a Baby Milk Bottle at once (the gauge starts again at 0). Codes: 0 given, 1 none in stock, 2 already gave one today, 3 the pill is not allowed yet, 4 her heart is full already,
   5 the first milk pill is still working (wait), 6 she needs the pump first, 7 not now (nothing to do with it yet), 8 the bottle stock is full, 14 (v12.2) the pump is broken and this pill would have filled the gauge */
static inline int g_milk_gift(Game *g, int kind) {
    if (!g->pill_seen) return 7;
    if (kind == GF_MILK && g->injected) return 16;   /* v12.3: she does not need a pill any more */
    if (kind == GF_MILK && g_super_block(g)) return 9;   /* v8.2: she refuses the normal pill while the Super drug works and for 2 days after */
    if (kind == GF_PUMP) {
        if (g->milk_st >= 3) {   /* v12.2: the new pump for a broken one (she already told you): the gauge stays as it is */
            if (!g->pump_dead || g->pump <= 0) return 7;
            g->pump = 0; g->pump_life = g_pump_roll(); g->pump_used = 0; g->pump_dead = 0; return 0;
        }
        if (g->milk_st != 2) return 7;
        g->pump = 0; g->milk_st = 3; g->milk_pct = 0; g->pump_life = g_pump_roll(); g->pump_used = 0; g->pump_dead = 0; return 0;
    }
    if (g->milk_st == 0) { g->mpill--; g->milk_st = 1; g->milk_day = g->day; return 0; }
    if (g->milk_st == 1) return 5;
    if (g->milk_st == 2) return 6;
    if (g->married) {   /* v8.6: once married a full gauge STAYS full: you choose the pump (a bottle), ASK FOR MILK or breastfeeding, and each one empties it */
        if (g->milk_pct >= 100) return 11;
        if (g->milk_pct + MILK_STEP >= 100) { g->mpill--; g->milk_pct = 100; return 0; }
    }
    if (g->milk_pct + MILK_STEP >= 100 && g->bottle >= INV_CAP) return 8;
    if (g->milk_pct + MILK_STEP >= 100 && g_pump_wear(g)) return 14;   /* v12.2: this pill would fill the gauge: the pump makes the bottle, a broken one does not (the pill is not used up) */
    g->mpill--; g->milk_pct += MILK_STEP;
    if (g->milk_pct >= 100) { g->milk_pct = 0; g->bottle++; if (g->milk_st < 4) g->milk_st = 4; }
    return 0;
}
/* give the Super drug. 0 given, 1 none in stock, 5 the first milk pill is still working (wait), 6 she needs the pump first, 7 not now, 10 the last drug still works */
static inline int g_super_gift(Game *g) {
    if (!g->pill_seen) return 7;
    if (g->injected) return 16;   /* v12.3 */
    if (g->sdrug <= 0) return 1;
    if (g->milk_st == 1) return 5;
    if (g->milk_st == 2) return 6;
    if (g->milk_st < 3) return 7;
    if (g_super_active(g)) return 10;
    g->sdrug--; g->sd_day = g->day; g->ask_day = 0; g->ask_n = 0; return 0;
}
/* ASK FOR MILK (only while the Super drug works): a Baby Milk Bottle, SUPER_ASKS times a day. 0 got a bottle, 1 not available, 2 the asks of today are used, 8 the bottle stock is full, 14 the pump is broken */
static inline int g_ask_milk(Game *g) {
    if (g->injected) {   /* v12.3: after the Mommy Milk Injection: no gauge, no drug. ONE of ask / pump / breastfeed a day; it gives TWO bottles and uses the pump once */
        if (g->inj_day == g->day) return 2;
        if (g->bottle > INV_CAP - 2) return 8;
        if (g_pump_wear(g)) return 14;
        g->bottle += 2; g->inj_day = g->day; g->mday = g->day; if (g->milk_st < 4) g->milk_st = 4; return 0;
    }
    if (!g_super_active(g)) return 1;
    if (g->ask_day != g->day) { g->ask_day = g->day; g->ask_n = 0; }
    if (g->ask_n >= SUPER_ASKS) return 2;
    if (g->bottle >= INV_CAP) return 8;
    if (g_pump_wear(g)) return 14;   /* v12.2: a broken pump: she tells you, the ask is not used up */
    g->ask_n++; g->bottle++; if (g->milk_st < 4) g->milk_st = 4; g->mday = g->day; return 0;
}
/* ---------- v8.6: marriage and page 4 of her menu ----------
   The diamond ring ($50,000, online store) is given from her give page after the day-8 scene: that is the wedding (married = 1). Then page 4 opens: HUG, KISS, SLEEP TOGETHER (night only)
   and BREASTFEED. Breastfeeding needs a FULL milk gauge (or the Super drug, where it uses one of the daily asks) and empties it like the pump / the Baby Milk Bottle do.
   When married a full gauge stays full (the pump no longer runs by itself): PUMP MILK (a Baby Milk Bottle), ASK FOR MILK and BREASTFEED all need it full again afterwards.
   She refuses to sleep beside you on a day she gave milk (mday) or after you teased her (kicked_day in main.c). Two outfits (day 8 / yoga) change only the page-4 scene pictures. */
static inline int g_can_marry(const Game *g) { return g->rs == RS_LIVING && g->pill_seen && g->dring && !g->married; }
static inline int g_marry(Game *g) { if (!g_can_marry(g)) return 1; g->married = 1; g->outfit = 0; g->call = 0; g->mm_kind = 0; g->mm_day = 0; return 0; }
static inline int g_married_day_door(const Game *g) { return g->married && g->phase != PH_NIGHT; }   /* v8.7: married, morning / evening: no hiding picture at her door */
static inline int g_p4_open(const Game *g) { return g->rs == RS_LIVING && g->married; }
static inline int g_mama(const Game *g) { return g_p4_open(g) && g->call == 1; }   /* v9.5: you call her MAMA: her mama dialogue set, and the BREASTFEED row exists */
/* v9.7: what she remembers of mama mode. The kinds are only noted while you call her MAMA. */
enum { MK_NONE, MK_CALL, MK_CHAT, MK_TEASE, MK_FEED, MK_FEEDNO, MK_HUG, MK_KISS, MK_SLEEP, MK_N };
static inline void g_mm_note(Game *g, int kind) { if (g_mama(g) && kind > MK_NONE && kind < MK_N) { g->mm_kind = kind; g->mm_day = g->day; } }
static inline int g_mm_age(const Game *g) { int d; if (!g_mama(g) || g->mm_kind == MK_NONE) return -1; d = g->day - g->mm_day; return d == 0 || d == 1 ? d : -1; }   /* 0 = earlier today, 1 = yesterday, -1 = nothing fresh */
enum { OF_USUAL, OF_YOGA, OF_TOP, OF_COW, OF_N };   /* v10.8: her outfits (Game.outfit) */
static inline int g_cow(const Game *g) { return g_p4_open(g) && g->outfit == OF_COW; }   /* v12.1: COW MODE: she wears the cow print outfit (married): her dialogue turns negative, she still accepts. No save flag: it ends when she changes out of it */
static inline int g_outfit_open(const Game *g) { return g_p4_open(g) && (g->yoga_given || g->top_given || g->cow_given); }
static inline int g_outfit_has(const Game *g, int k) { return k == OF_USUAL || (k == OF_YOGA && g->yoga_given) || (k == OF_TOP && g->top_given) || (k == OF_COW && g->cow_given); }
static inline int g_outfit_targets(const Game *g, int *t) {   /* v10.8: the outfits she can change into now: every owned one except the one she wears (usual outfit first). 0 when the wardrobe is closed */
    int n = 0, k; if (!g_outfit_open(g)) return 0;
    for (k = 0; k < OF_N; k++) if (k != g->outfit && g_outfit_has(g, k)) t[n++] = k;
    return n;
}
static inline int g_outfit_set(Game *g, int k) { if (g_outfit_open(g) && k >= 0 && k < OF_N && g_outfit_has(g, k)) { g->outfit = k; return 0; } if (!g_outfit_open(g)) g->outfit = 0; return 1; }
static inline void g_outfit_toggle(Game *g) { int k, o = g->outfit; if (!g_outfit_open(g)) { g->outfit = 0; return; } for (k = 1; k <= OF_N; k++) if (g_outfit_has(g, (o + k) % OF_N)) { g->outfit = (o + k) % OF_N; return; } }   /* next owned outfit in the order usual > yoga > top > usual (with only the yoga outfit it is the old two-way switch) */
static inline int g_pump_ready(const Game *g) { return g_p4_open(g) && g->milk_st >= 3 && (g->injected ? g->inj_day != g->day : g->milk_pct >= 100); }   /* v12.3: injected: no gauge, the one daily milk is still free */
static inline int g_pump_use(Game *g) {   /* PUMP MILK: 0 got a bottle, 1 gauge not full, 8 the bottle stock is full, 14 the pump is broken */
    if (!g_pump_ready(g)) return 1;
    if (g->bottle >= INV_CAP) return 8;
    if (g_pump_wear(g)) return 14;   /* v12.2: a broken pump: she tells you, the gauge stays full */
    if (g->injected) { g->bottle++; g->inj_day = g->day; g->mday = g->day; if (g->milk_st < 4) g->milk_st = 4; return 0; }   /* v12.3: the daily milk is used, the gauge does not exist */
    g->milk_pct = 0; g->bottle++; if (g->milk_st < 4) g->milk_st = 4; g->mday = g->day; return 0;
}
static inline int g_feed_ready(const Game *g) { return g_p4_open(g) && g->pill_seen && g->milk_st >= 3 && ((g_super_active(g) && g_asks_left(g) > 0) || g->milk_pct >= 100 || g_inj_free(g)); }
static inline int g_bfeed_ready(const Game *g) { return g_mama(g) && (g_pump_ready(g) || (g_super_active(g) && g_asks_left(g) > 0)); }   /* v13.11: BOTTLE FEEDING (mama mode) has the gate of PUMP MILK (full gauge / the daily injected milk) or of ASK FOR MILK (Super drug) */
static inline int g_feed(Game *g) {   /* BREASTFEED: 0 done, 1 gauge not full. Under the Super drug it uses one of the daily asks, else it empties the real gauge */
    if (!g_mama(g) || !g_feed_ready(g)) return 1;   /* v9.5: BREASTFEED only exists in mama mode */
    if (g->injected) { g->inj_day = g->day; g->mday = g->day; return 0; }   /* v12.3: the one daily milk: no gauge, no pump */
    if (g_super_active(g) && g_asks_left(g) > 0) { if (g->ask_day != g->day) { g->ask_day = g->day; g->ask_n = 0; } g->ask_n++; }
    else g->milk_pct = 0;
    g->mday = g->day; return 0;
}
static inline int g_sleep_refused_milk(const Game *g) { return g->mday == g->day; }   /* she gave milk today: not tonight */
/* v12.3: give the MOMMY MILK INJECTION. 0 given, 1 none, 5 / 6 / 7 as the pump gate (first pill still working / she needs the pump first / not now), 15 a Super drug still works (wait) */
static inline int g_inject_gift(Game *g) {
    if (!g->pill_seen) return 7;
    if (g->inj <= 0 || g->injected) return 1;
    if (g->milk_st == 1) return 5;
    if (g->milk_st == 2) return 6;
    if (g->milk_st < 3) return 7;
    if (g_super_active(g)) return 15;
    g->inj = 0; g->injected = 1; g->inj_day = 0; g->milk_pct = 0; g->sd_day = 0; g->ask_day = 0; g->ask_n = 0; return 0;
}
static inline int g_gift(Game *g, int kind) {
    if (!g_gift_stock(g, kind)) return kind == GF_PUMP || kind == GF_MILK || kind == GF_SUPER || kind == GF_INJECT ? (g->pill_seen ? 1 : 7) : 1;
    if (kind == GF_SUPER) return g_super_gift(g);
    if (kind == GF_INJECT) return g_inject_gift(g);
    if (kind == GF_PUMP || kind == GF_MILK) return g_milk_gift(g, kind);
    if (g_gift_is_outfit(kind)) {   /* v11.3: an outfit: only at max romance, no romance gain, not the gift of the day. 0 given, 1 none to give / she has it already, 12 not unlocked yet */
        if (!g_outfit_ok(g, kind != GF_YOGA)) return 12;   /* v13.8: top and cow also need MAMA mode */
        if (kind == GF_YOGA) { if (g->yoga_given) return 1; g->yoga--; g->yoga_given = 1; }
        else if (kind == GF_TOP) { if (g->top_given) return 1; g->top--; g->top_given = 1; }
        else { if (g->cow_given) return 1; g->cow--; g->cow_given = 1; }
        return 0;
    }
    if (kind == GF_DECK && g->rs == RS_LIVING && !g->deck_given && (g_rom_full(g) || g_gifted_today(g))) { g->pdeck--; g->deck_given = 1; return 0; }   /* v11.6: the first pink deck since she lives here is always accepted (the table turns pink), even on a day with a gift or at a full bar: then no romance, not the gift of the day */
    if (g_gifted_today(g)) return 2;
    if (kind == GF_PILL && !g_rom_full(g)) return 3;
    if (kind != GF_PILL && g_rom_full(g)) return 4;
    if (kind == GF_FLOWER) g->flower--; else if (kind == GF_STACK) g->stack--; else if (kind == GF_LOTION) g->lotion--; else if (kind == GF_DECK) { g->pdeck--; if (g->rs == RS_LIVING) g->deck_given = 1; } else if (kind == GF_YOGA) { g->yoga--; g->yoga_given = 1; } else if (kind == GF_TOP) { g->top--; g->top_given = 1; } else if (kind == GF_COW) { g->cow--; g->cow_given = 1; } else if (kind == GF_GOLD) g->gold--;
    if (kind == GF_PILL) { g->pill_given = 1; g->pill_day = g->day; }
    else g_rom_add(g, g_gift_gain(kind));
    g->gift_day = g->day; return 0;
}
/* v6.4: the pill works after 7 days. The gift day is day 1: on days 6 and 7 she complains (pill_hint 1 / 2), from day 8 (pill_day + 7) the first knock in the morning
   or evening plays the body-change scene (pill_due). That scene is when her pictures change (pill_seen) and the night sneak-in at home opens. */
#define PILL_DAYS 7
static inline int g_pill_due(const Game *g) { return g->pill_given && !g->pill_seen && g->day >= g->pill_day + PILL_DAYS; }
static inline int g_pill_hint(const Game *g) { int d = g->day - g->pill_day; return (g->pill_given && !g->pill_seen) ? (d == PILL_DAYS - 2 ? 1 : d == PILL_DAYS - 1 ? 2 : 0) : 0; }
/* v6.5: she is asked first (days 6 and 7 after the pill, once per day): the scene plays on the first knock of that day */
static inline int g_hint_scene(const Game *g) { return g_pill_hint(g) && g->hint_day != g->day; }
/* v6.6: before day 6 of the pill she only stands in front of her door; from day 6 on (and after the day-8 scene) she invites you into her room */
static inline int g_invited(const Game *g) { return g->pill_seen || (g->pill_given && g->day >= g->pill_day + PILL_DAYS - 2); }
static inline int g_milk_due(const Game *g) { return g->pill_seen && g->milk_st == 1 && g->day >= g->milk_day + MILK_WAIT; }   /* the first knock on day 3 after the first milk pill */
static inline int g_milk_gauge(const Game *g) { return g->milk_st >= 3 && !g->injected; }   /* the bottle gauge exists once the pump is given */
static inline int g_milk_active(const Game *g) { return g->milk_until > 0 && g->day <= g->milk_until; }   /* a Baby Milk Bottle is working */
static inline int g_milk_left(const Game *g) { return g_milk_active(g) ? g->milk_until - g->day + 1 : 0; }
static inline int g_drink_bottle(Game *g) {   /* 0 ok, 1 none left, 2 one is working already */
    if (g->bottle <= 0) return 1;
    if (g_milk_active(g)) return 2;
    g->bottle--; g->milk_until = g->day + MILK_DAYS - 1; return 0;
}
static inline int g_energy_ok(const Game *g, int need) { return g_milk_active(g) || g->energy >= need; }   /* the bottle: energy is not needed and not spent */
static inline void g_energy_spend(Game *g, int n) { if (!g_milk_active(g)) { g->energy -= n; if (g->energy < 0) g->energy = 0; } }
static inline int g_soph_energy_ok(const Game *g) { return g_energy_ok(g, SOPH_ENERGY); }
static inline int g_home_sneak(const Game *g) { return g->rs == RS_LIVING && g->pill_seen; }   /* at night, in your own home, only after the day-8 scene */
static inline int g_home_start(Game *g, int hands) {   /* a free match at home: once a day, 10 or 20 hands. 1 = started, 0 = too tired, -1 = already played today */
    int i;
    if (g_played_today(g)) return -1;
    if (!g_energy_ok(g, HOME_ENERGY)) return 0;
    g_energy_spend(g, HOME_ENERGY); g->chal_day = g->day;
    g->in_match = 1; g->home = 1; g->mlen = hands; g->fee = 0; g->hand = 0; g->net = 0; g->style = rnd(3); g->tilt = 0;
    for (i = 0; i < BN_N; i++) g->bn[i] = 0;
    g->over = 1; return 1;
}


/* ---------- match end, bank ---------- */
static inline int bonus_total(const Game *g) { int i, t = 0; for (i = 0; i < BN_N; i++) t += g->bn[i] * BONUS_AMT[i]; return t * g_mult(g->fee); }   /* (times the stake) */
static inline int cap_money(long long v) { return v > MONEY_CAP ? MONEY_CAP : (int)v; }

/* ---------- debug panel (DEBUG_MODE): shortcuts for testing the romance part ---------- */
/* skip the $5000 boss match: the state you would have after beating her (she has her name, the 3rd floor is open, the Event Center is open).
   0 = done, 1 = already past that point, 2 = the intro is not finished yet (story == ST_NEW) */
static inline int g_dbg_skip(Game *g) {
    if (g->story == ST_NEW) return 2;
    if (g->story >= ST_NAMED) return 1;
    g->story = ST_NAMED; g->event = 1; g->ex_big = 1; g->hall_seen = 3; if (g->sw_wins < SW_GOAL) g->sw_wins = SW_GOAL; return 0;   /* v10.0: the test shortcut also opens the black market (20 wins) */
}
static inline void g_dbg_cash(Game *g) { g->cash = cap_money((long long)g->cash + DEBUG_CASH); }   /* cash, not bank: the room upgrade at the desk is cash only */
static inline void g_dbg_newday(Game *g) { if (g->day < DAY_MAX) g->day++; g->phase = PH_MORNING; }   /* next morning: the daily gift / match limits are open again (nothing is saved, no rent is taken) */
static inline void g_dbg_week(Game *g) { g->day = g->day + 7 > DAY_MAX ? DAY_MAX : g->day + 7; g->phase = PH_MORNING; }   /* skip 7 days: next week's morning (nothing is saved, no rent is taken) */
static inline void g_dbg_milk(Game *g) {   /* test shortcut: the day-8 scene counts as seen (the store lists the milk things), a pump and 4 milk pills are added (1 for the wait, 2 for a bottle, 1 spare) */
    g->pill = 1; g->pill_given = 1; g->pill_seen = 1; if (g->pill_day < 1) g->pill_day = 1;
    if (g->milk_st < 3) g->pump = 1;
    g->mpill = g->mpill + 4 > INV_CAP ? INV_CAP : g->mpill + 4;
    g->sdrug = g->sdrug + 2 > INV_CAP ? INV_CAP : g->sdrug + 2;   /* v8.2: and 2 Super milk drugs */
}
static inline void g_dbg_fillbar(Game *g) { g_rom_bump(g, 100); }   /* fills the current level: the next level starts at 0; level 3 stays full */

/* final result of a match once the last hand is done. Net chips are paid 1:1 as cash, plus card bonuses,
   but only for a WIN. A loss or a tie pays nothing (and costs nothing). */
static inline void g_match_finish(Game *g) {
    int res = g->net > 0 ? 1 : g->net < 0 ? -1 : 0, bonus = g->home ? 0 : bonus_total(g), pay = (res > 0 && !g->home) ? g->net * g_mult(g->fee) + bonus + g_prize(g->fee) : 0;
    g->last_rom = 0;
    if (g->home && res > 0) { g->last_rom = g_rom_eff(g, g->mlen >= 20 ? ROM_WIN20 : ROM_WIN10); g_rom_bump(g, g->last_rom); }   /* at home a win only fills the romance bar: no money, no card bonus, no drink */
    g->cash = cap_money((long long)g->cash + pay);
    if (res == 0) g->cash = cap_money((long long)g->cash + g->fee);   /* a tie: the buy-in comes back */
    g->m_played++; if (res > 0) g->m_won++; else if (res < 0) g->m_lost++; else g->m_tied++;
    if (res > 0 && !g->home && g->story >= ST_NAMED && g->fr_wins < 999) g->fr_wins++;   /* v11.0: the friendship bar: a win at her table once she has a name (the boss win does not count: it starts at zero) */
    if (res > 0 && !g->home && g->sw_wins < 999) g->sw_wins++;   /* v10.0: a win over Sophia at her table counts toward the black market */
    if (g->net > g->best_net) g->best_net = g->net;
    g->last_res = res; g->last_net = g->net; g->last_bonus = bonus; g->last_pay = pay;
    g->last_drink = 0;   /* a win at the 500 table gives a free soda, at the 1000 table a free energy drink (into the inventory) */
    if (res > 0 && g->fee == SOPH_FEE[1]) { g->last_drink = 1; if (g->soda < INV_CAP) g->soda++; }
    if (res > 0 && g->fee >= SOPH_FEE[2]) { g->last_drink = 2; if (g->monster < INV_CAP) g->monster++; }
    g->in_match = 0; g->home = 0;
}
/* walking out of a match (or the power going off during one) counts as a LOSS: no payout, no bonuses, costs nothing */
static inline void g_forfeit(Game *g) {
    int i; for (i = 0; i < BN_N; i++) g->bn[i] = 0;
    g->m_played++; g->m_lost++;
    g->last_res = -1; g->last_net = g->net; g->last_bonus = 0; g->last_pay = 0; g->last_drink = 0;
    g->in_match = 0; g->home = 0; g->last_rom = 0; g->forfeit = 1;
}
/* exhibition (event center): the buy-in is paid from CASH when you sit down; winning pays 3.5x the buy-in, anything else pays nothing.
   The 6th table is the BOSS (Sophia, still unnamed): $5000 buy-in, a fixed $10000 prize, one time only. It shows up in the list once you have won the $1000 table
   (ex_big) and only until you beat her (story == ST_FIRST); beating her gives her name (story = ST_NAMED) and the 3rd floor key card back. */
#define EX_N 6
static const int EX_BUYIN[EX_N] = {10, 50, 100, 500, 1000, 5000};
/* v13.16: the energy a game uses depends on the buy-in (was a flat 15%): $10 = 5%, $50 = 10%, $100 = 15%, $500 = 25%, $1000 = 30%; the $5000 boss table costs 30% like the top table.
   The same price list is used by the Exhibition and the 8 ball single match. Needed = used. The Milk Bottle still waives it. */
static const int EX_ENERGY_OF[EX_N] = {5, 10, 15, 25, 30, 30};
#define EX_ENERGY_MIN 5   /* the cheapest game: the event menu only opens the buy-in list when you have at least this much */
static inline int g_buyin_energy(int buyin) { int i; for (i = 0; i < EX_N; i++) if (EX_BUYIN[i] == buyin) return EX_ENERGY_OF[i]; return EX_ENERGY_OF[EX_N - 1]; }
#define EX_BOSS 5               /* index of the boss table in EX_BUYIN / EX_START_BB */
#define EX_BOSS_PRIZE 10000
static inline int g_boss_open(const Game *g) { return g->story == ST_FIRST && g->ex_big; }
static inline int g_ex_n(const Game *g) { return g_boss_open(g) ? EX_N : EX_N - 1; }   /* how many tables the list shows */
/* game over: no cash and no bank money left that could pay for even the cheapest Exhibition buy-in (the only way left to earn money) */
static inline int g_broke(const Game *g) { return g->story >= ST_HOME && g->cash + g->bank < EX_BUYIN[0]; }   /* (the debt phase, the paid-but-not-yet-played phase and later: nothing left means nothing can ever be earned or played) */
static inline int g_ex_prize(int buyin) { return buyin == EX_BUYIN[EX_BOSS] ? EX_BOSS_PRIZE : buyin * 7 / 2; }   /* 3.5x the buy-in (paid in full; you profit 2.5x); the boss pays a fixed 10000 */
static inline int g_ex_start(Game *g, int buyin) {   /* 1 = seated, 0 = not enough cash, -1 = not enough energy (an exhibition uses 20%) */
    if (buyin <= 0) return 0;
    if (!g_energy_ok(g, g_buyin_energy(buyin))) return -1;
    if (g->cash < buyin) return 0;
    if (buyin == EX_BUYIN[EX_BOSS] && !g_boss_open(g)) return 0;   /* the boss table is only there until she is beaten */
    g->cash -= buyin; g_energy_spend(g, g_buyin_energy(buyin)); g->ex_played++; return 1;
}
/* ---------- v12.6: 8 ball at the Event Center ----------
   Five tables (the same buy-ins as the Exhibition, without the boss). Cash buy-in, energy by buy-in (g_buyin_energy, 5 to 30%; the Milk Bottle waives it), saved at once: leaving mid-game loses the buy-in.
   The winner is paid POOL_PRIZE_PCT percent of the buy-in (2.5x since v13.9). The CPU gets better with the stakes (see PL_AIM_ERR in pool.h). Nothing new is saved. */
#define POOL_N 5
#define POOL_PRIZE_PCT 250   /* v13.9 (was 200; v13.1 was 180): the winner is paid 2.5x the buy-in */
static const int POOL_BUYIN[POOL_N] = {10, 50, 100, 500, 1000};
static inline int g_pool_prize(int buyin) { return buyin * POOL_PRIZE_PCT / 100; }
static inline int g_pool_start(Game *g, int buyin) {   /* 1 = seated, 0 = not enough cash, -1 = not enough energy */
    if (buyin <= 0) return 0;
    if (!g_energy_ok(g, g_buyin_energy(buyin))) return -1;
    if (g->cash < buyin) return 0;
    g->cash -= buyin; g_energy_spend(g, g_buyin_energy(buyin)); return 1;
}
static inline void g_pool_win(Game *g, int buyin) { g->cash = cap_money((long long)g->cash + g_pool_prize(buyin)); }
/* ---------- v13.1: the 8 ball TOURNAMENT (a ladder) ----------
   Fee POOL_T_FEE from cash and POOL_T_ENERGY energy once, at entry (the Milk Bottle waives the energy); saved at once, so leaving loses the fee.
   You play the 6 CPUs one after another, weakest to slightly stronger (POOL_T_SKILL = the CPU skill of each match, same scale as the single-match tables).
   Win all 6 = POOL_T_PRIZE. Losing any match ends the run. Nothing is saved between the matches. */
#define POOL_T_N 6
#define POOL_T_FEE 1000
#define POOL_T_PRIZE 6000
#define POOL_T_ENERGY 50   /* v13.16 (was 30) */
#define POOL_T_WEEK 7      /* v13.16: the tournament is open once a week: 7 days after the day you entered */
static const int POOL_T_SKILL[POOL_T_N] = {0, 0, 1, 1, 2, 2};
static inline int g_pool_t_wait(const Game *g) { int left = g->pt_day > 0 ? g->pt_day + POOL_T_WEEK - g->day : 0; return left > 0 ? left : 0; }   /* days until the next entry (0 = open) */
static inline int g_pool_t_start(Game *g) {   /* 1 = entered, 0 = not enough cash, -1 = not enough energy, -2 = already played this week */
    if (g_pool_t_wait(g) > 0) return -2;
    if (!g_energy_ok(g, POOL_T_ENERGY)) return -1;
    if (g->cash < POOL_T_FEE) return 0;
    g->cash -= POOL_T_FEE; g_energy_spend(g, POOL_T_ENERGY); g->pt_day = g->day; return 1;
}
static inline void g_pool_t_win(Game *g) { g->cash = cap_money((long long)g->cash + POOL_T_PRIZE); }
/* ---------- v13.7: 8 ball WITH SOPHIA in the playing room ----------
   A on the pool table with her in the room. She asks: GO EASY (CPU skill POOL_S_SKILL_EASY) or NO HOLD BACK (POOL_S_SKILL_HARD).
   A match is a best of 5: the first to POOL_S_SETS (3) sets wins. Cost POOL_S_ENERGY energy (the Milk Bottle waives it), no cash, no prize.
   Leaving halfway loses the match, the energy is not refunded. A win gives romance, but only the first win of each day (the day is kept in memory only, like the chat counter).
   Breaks alternate: you break set 1, she breaks set 2, and so on. Nothing new is saved. */
#define POOL_S_ENERGY 20
#define POOL_S_SETS 3
#define POOL_S_MAX_SETS (2 * POOL_S_SETS - 1)
#define POOL_S_SKILL_EASY 1
#define POOL_S_SKILL_HARD 4
#define POOL_S_ROM_EASY 5
#define POOL_S_ROM_HARD 10
static inline int g_pool_s_start(Game *g) {   /* 1 = started (energy paid), -1 = not enough energy */
    if (!g_energy_ok(g, POOL_S_ENERGY)) return -1;
    g_energy_spend(g, POOL_S_ENERGY); return 1;
}
static inline int g_pool_s_skill(int hard) { return hard ? POOL_S_SKILL_HARD : POOL_S_SKILL_EASY; }
static inline int g_pool_s_over(int you, int her) { return you >= POOL_S_SETS || her >= POOL_S_SETS; }   /* the match is decided */
static inline int g_pool_s_breaker(int set) { return set & 1; }   /* who breaks set number `set` (0 = the first): 0 = you, 1 = she */
static inline int g_pool_s_rom_pts(int hard) { return hard ? POOL_S_ROM_HARD : POOL_S_ROM_EASY; }
static inline int g_pool_s_win(Game *g, int hard, int *last_day) {   /* you won the match: romance for the first win of the day. returns the romance points added (0 = none today) */
    int pc0, lv0;
    if (*last_day == g->day) return 0;
    *last_day = g->day; lv0 = g->rom_lvl; pc0 = g->rom_pct; g_rom_add(g, g_pool_s_rom_pts(hard));
    return g->rom_lvl > lv0 ? 100 - pc0 : g->rom_pct - pc0;
}

/* ---------- v9.0: the lottery (Event Center counter) ----------
   Six tickets. A winning ticket pays LOT_MULT times its price. Every ticket wins with the same chance, LOT_PCT percent (an average return of 90 percent).
   One ticket at a time, paid from CASH, no energy. The result is rolled when you buy and kept in the save; the clerk tells you from the NEXT day on (the day is the one that moves when you sleep). */
#define LOT_N 6
#define LOT_MULT 3
#define LOT_PCT 30
static const int LOT_PRICE[LOT_N] = {20, 50, 100, 250, 600, 1500};
static inline int g_lot_prize(int price) { return price * LOT_MULT; }
static inline int g_lot_has(const Game *g) { return g->lot_price > 0; }
static inline int g_lot_ready(const Game *g) { return g->lot_price > 0 && g->day > g->lot_day; }   /* the result can be told today */
/* buy ticket idx; roll = 0..99 (a win when roll < LOT_PCT). 1 = bought, 0 = not enough cash, -1 = you still hold a ticket, -2 = no such ticket */
static inline int g_lot_buy(Game *g, int idx, int roll) {
    if (idx < 0 || idx >= LOT_N) return -2;
    if (g->lot_price > 0) return -1;
    if (g->cash < LOT_PRICE[idx]) return 0;
    g->cash -= LOT_PRICE[idx]; g->lot_price = LOT_PRICE[idx]; g->lot_day = g->day; g->lot_win = roll >= 0 && roll < LOT_PCT ? 1 : 0;
    return 1;
}
/* the clerk tells the result: -1 = nothing to tell yet (no ticket, or it was bought today), 0 = no win, >0 = the prize (paid into your cash) */
static inline int g_lot_claim(Game *g) {
    int prize;
    if (!g_lot_ready(g)) return -1;
    prize = g->lot_win ? g_lot_prize(g->lot_price) : 0;
    if (prize) g->cash = cap_money((long long)g->cash + prize);
    g->lot_price = 0; g->lot_day = 0; g->lot_win = 0;
    return prize;
}
static inline void g_ex_win(Game *g, int buyin) {
    g->cash = cap_money((long long)g->cash + g_ex_prize(buyin)); g->ex_won++;
    if (buyin == EX_BUYIN[EX_N - 2]) g->ex_big = 1;                           /* the $1000 table: the boss table opens */
    if (buyin == EX_BUYIN[EX_BOSS] && g->story == ST_FIRST) { g->story = ST_NAMED; if (g->sw_wins < 999) g->sw_wins++; }   /* v10.0: the boss win counts as a win over her */
      /* she is beaten: she gives her name, the elevator takes you to her floor again */
}
/* store and inventory. Buying puts the drink in the inventory (cash only); drinking is a separate step. */
static inline int g_buy(Game *g, int kind) {   /* kind 0 soda, 1 energy drink. 0 ok, 1 not enough cash, 2 inventory full */
    int price = kind ? PRICE_MONSTER : PRICE_SODA, *n = kind ? &g->monster : &g->soda;
    if (*n >= INV_CAP) return 2;
    if (g->cash < price) return 1;
    g->cash -= price; (*n)++; return 0;
}
static inline int g_drink(Game *g, int kind) {   /* 0 ok, 1 none left, 2 energy already full */
    int *n = kind ? &g->monster : &g->soda, rest = kind ? REST_MONSTER : REST_SODA;
    if (*n <= 0) return 1;
    if (g->energy >= ENERGY_MAX) return 2;
    (*n)--; g->energy += rest; if (g->energy > ENERGY_MAX) g->energy = ENERGY_MAX; return 0;
}
/* the one-time pity soda: no energy, no drinks, no money anywhere. Later the player has to mind the money. */
static inline int g_rescue_due(const Game *g) { return !g->rescue_used && g->energy == 0 && g->soda == 0 && g->monster == 0 && (long long)g->cash + g->bank < PRICE_SODA; }
static inline void g_rescue(Game *g) { g->rescue_used = 1; g->soda = 1; }
/* ATM. 0 ok, 1 not enough money, 2 over the limit, 3 zero amount */
static inline int g_deposit(Game *g, int v) {
    if (v <= 0) return 3;
    if (v > g->cash) return 1;
    if ((long long)g->bank + v > MONEY_CAP) return 2;
    g->cash -= v; g->bank += v; return 0;
}
static inline int g_withdraw(Game *g, int v) {
    if (v <= 0) return 3;
    if (v > g->bank) return 1;
    if ((long long)g->cash + v > MONEY_CAP) return 2;
    g->bank -= v; g->cash += v; return 0;
}
/* v13.15 BANK INTEREST: every new day (you wake up in the morning after sleeping in the bed) the bank pays INT_PM per mille of the first INT_LIMIT dollars of the bank balance
   (0.5 percent a day = $5 for every $1,000, at most $50 a day), added to the bank (it compounds, but only the first $10,000 earn). Once per day (int_day), never on the sofa phases, never for cash. */
#define INT_PM    5        /* per mille per day */
#define INT_LIMIT 10000    /* only this much of the bank earns */
static inline int g_interest_calc(const Game *g) { long long b = g->bank < INT_LIMIT ? g->bank : INT_LIMIT; return b > 0 ? (int)(b * INT_PM / 1000) : 0; }   /* what the bank would pay at the next morning */
static inline int g_interest_pay(Game *g) {   /* pays it if today's has not been paid; returns the amount (0 = nothing: no money, already paid or the bank is full) */
    int v;
    if (g->int_day >= g->day) return 0;
    g->int_day = g->day; v = g_interest_calc(g);
    if ((long long)g->bank + v > MONEY_CAP) v = MONEY_CAP - g->bank;
    if (v < 0) v = 0;
    g->bank += v; g->int_last = v; return v;
}
/* ---------- text helpers ---------- */
static inline void fmt_money64(char *out, long long v) {   /* 1250 -> "1,250"; v13.15: 64 bits (a stack of shares can be worth more than 2 billion); at most 26 characters */
    char t[24]; int n, i, j = 0; snprintf(t, sizeof t, "%lld", v); n = (int)strlen(t);
    for (i = 0; i < n; i++) { if (i && (n - i) % 3 == 0 && t[0] != '-') out[j++] = ','; out[j++] = t[i]; }
    out[j] = 0;
}
static inline void fmt_money(char *out, int v) { fmt_money64(out, v); }   /* 999999999 -> "999,999,999" (11 characters) */
/* word-wrap s into out (lines joined by '\n') at maxc columns; returns the number of lines */
static inline int wrap_buf(char *out, int outsz, const char *s, int maxc) {
    int col = 0, line = 1, j = 0; const char *p = s;
    while (*p && j < outsz - 2) {
        int wl = 0, i; while (p[wl] && p[wl] != ' ') wl++;
        if (col > 0 && col + 1 + wl > maxc) { out[j++] = '\n'; line++; col = 0; }
        else if (col > 0) { out[j++] = ' '; col++; }
        for (i = 0; i < wl && j < outsz - 2; i++) { out[j++] = p[i]; col++; }
        p += wl; while (*p == ' ') p++;
    }
    out[j] = 0; return line;
}

/* ---------- v10.5: the rewind guard (pokerqueen.prog = the furthest day reached, per career) ---------- */
#define SLOT_N 5       /* v10.5: save slots (slot 1 = pokerqueen.sav, then pokerqueen2.sav ...) */
#define PROG_PATH "pokerqueen.prog"
#define PROG_N 16
static inline int prog_get(const char *path, int career) {
    FILE *f; int c, d, best = 0;
    if (career <= 0) return 0;
    f = fopen(path, "r"); if (!f) return 0;
    while (fscanf(f, "%d %d", &c, &d) == 2) if (c == career && d > best) best = d;
    fclose(f); return best;
}
static inline void prog_set(const char *path, int career, int day) {   /* remember the furthest day (never lowers it); the career just saved moves to the end of the list */
    int cs[PROG_N + 1], ds[PROG_N + 1], n = 0, i, best = day, c, d; char tmp[300]; FILE *f;
    if (career <= 0) return;
    f = fopen(path, "r");
    if (f) { while (n < PROG_N && fscanf(f, "%d %d", &c, &d) == 2) { if (c == career) { if (d > best) best = d; } else { cs[n] = c; ds[n] = d; n++; } } fclose(f); }
    if (n == PROG_N) { for (i = 1; i < n; i++) { cs[i - 1] = cs[i]; ds[i - 1] = ds[i]; } n--; }   /* full: the career not saved for the longest time is forgotten */
    cs[n] = career; ds[n] = best; n++;
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "w"); if (!f) return;
    for (i = 0; i < n; i++) fprintf(f, "%d %d\n", cs[i], ds[i]);
    if (fclose(f) != 0) { remove(tmp); return; }
#ifdef _WIN32
    remove(path);
#endif
    if (rename(tmp, path) != 0) remove(tmp);
}
/* the loaded game is BEHIND its career when another save of it reached a later day: the lottery stays shut and the market is reshuffled until you catch up */
static inline int g_behind(const Game *g, int far_day) { return g->career > 0 && far_day > g->day; }
/* reshuffle the share prices of every day AFTER today (the days up to today keep theirs); a reshuffle of a future that was abandoned is dropped */
static inline void g_rewind(Game *g, unsigned salt) {
    int n = 0, i, from = g->day + 1, k = g->mrw_n < MRW_N ? g->mrw_n : MRW_N;
    for (i = 0; i < k; i++) if (g->mrw_day[i] < from) { g->mrw_day[n] = g->mrw_day[i]; g->mrw_salt[n] = g->mrw_salt[i]; n++; }
    if (n == MRW_N) { for (i = 1; i < n; i++) { g->mrw_day[i - 1] = g->mrw_day[i]; g->mrw_salt[i - 1] = g->mrw_salt[i]; } n--; }
    g->mrw_day[n] = from; g->mrw_salt[n] = (int)(salt & 0x7fffffffu) | 1; g->mrw_n = n + 1;
    for (i = g->mrw_n; i < MRW_N; i++) { g->mrw_day[i] = 0; g->mrw_salt[i] = 0; }
}

/* ---------- save file (atomic: temp file, then rename) ---------- */
static inline int save_game(const Game *g, const char *path) {
    char tmp[300]; FILE *f; Game c = *g; int ok;
    c.magic = SAVE_MAGIC; c.ver = SAVE_VER; c.size = (int)sizeof c;
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "wb"); if (!f) return 0;
    ok = fwrite(&c, sizeof c, 1, f) == 1; if (fclose(f) != 0) ok = 0;
    if (!ok) { remove(tmp); return 0; }
#ifdef _WIN32
    remove(path);   /* Windows rename() will not overwrite */
#endif
    if (rename(tmp, path) != 0) { remove(tmp); return 0; }
    return 1;
}
static inline int save_sane(const Game *g) {
    if (g->magic != SAVE_MAGIC || g->ver != SAVE_VER || g->size != (int)sizeof *g) return 0;
    if (g->cash < 0 || g->cash > MONEY_CAP || g->bank < 0 || g->bank > MONEY_CAP) return 0;
    if (g->event < 0 || g->event > 1 || g->ex_played < 0 || g->ex_won < 0 || g->ex_big < 0 || g->ex_big > 1) return 0;
    if (g->story < ST_NEW || g->story > ST_NAMED || g->forfeit < 0 || g->forfeit > 1) return 0;
    if (g->phase < 0 || g->phase >= PH_N || g->day < 1 || g->day > DAY_MAX) return 0;
    if (g->energy < 0 || g->energy > ENERGY_MAX || g->soda < 0 || g->soda > INV_CAP || g->monster < 0 || g->monster > INV_CAP) return 0;
    if (g->rescue_used < 0 || g->rescue_used > 1 || g->last_drink < 0 || g->last_drink > 2) return 0;
    if (g->keyst < 0 || g->keyst > 2) return 0;
    if (g->rs < 0 || g->rs >= RS_N || g->room_up < 0 || g->room_up > ROOM_LVL_MAX || g->ring < 0 || g->ring > 1 || g->pill < 0 || g->pill > 1 || g->pill_given < 0 || g->pill_given > 1 || g->pill_seen < 0 || g->pill_seen > 1 || g->pill_day < 0) return 0;
    if (g->pump < 0 || g->pump > 1 || g->mpill < 0 || g->mpill > INV_CAP || g->milk_st < 0 || g->milk_st > 4 || g->milk_day < 0 || g->milk_pct < 0 || g->milk_pct > 100 || g->milk_pct % MILK_STEP) return 0;
    if (g->bottle < 0 || g->bottle > INV_CAP || g->milk_until < 0 || g->hint_day < 0) return 0;
    if (g->flower < 0 || g->flower > INV_CAP || g->stack < 0 || g->stack > INV_CAP || g->rom_lvl < 0 || g->rom_lvl > ROM_LVLS || g->rom_pct < 0 || g->rom_pct > 100 || g->home < 0 || g->home > 1) return 0;
    { int s; for (s = 0; s < STK_N; s++) if (g->stk[s] < 0 || g->stk[s] > STK_CAP) return 0; if (g->mseed < 0) return 0; }
    if (g->lotion < 0 || g->lotion > INV_CAP || g->pdeck < 0 || g->pdeck > INV_CAP || g->yoga < 0 || g->yoga > INV_CAP || g->gold < 0 || g->gold > INV_CAP) return 0;
    if (g->dring < 0 || g->dring > 1 || g->sdrug < 0 || g->sdrug > INV_CAP || g->sd_day < 0 || g->ask_day < 0 || g->ask_n < 0 || g->ask_n > SUPER_ASKS) return 0;
    { int ok = g->lot_price == 0, k; for (k = 0; k < LOT_N; k++) if (g->lot_price == LOT_PRICE[k]) ok = 1;
      if (!ok || g->lot_day < 0 || g->lot_win < 0 || g->lot_win > 1 || (g->lot_price == 0 && (g->lot_win || g->lot_day))) return 0; }
    if (g->married < 0 || g->married > 1 || g->outfit < 0 || g->outfit >= OF_N || g->yoga_given < 0 || g->yoga_given > 1 || g->top < 0 || g->top > INV_CAP || g->top_given < 0 || g->top_given > 1 || g->cow < 0 || g->cow > INV_CAP || g->cow_given < 0 || g->cow_given > 1 || g->deck_given < 0 || g->deck_given > 1 || g->mday < 0) return 0;
    if (g->call < 0 || g->call > 1 || (g->call && !g->married)) return 0;   /* v9.5 */
    if (g->pump_life < 0 || g->pump_life > PUMP_MAX || g->pump_used < 0 || g->pump_used > g->pump_life || g->pump_dead < 0 || g->pump_dead > 1) return 0;   /* v12.2 */
    if (g->inj < 0 || g->inj > 1 || g->injected < 0 || g->injected > 1 || g->inj_day < 0 || (g->inj && g->injected)) return 0;   /* v12.3 */
    if (g->int_day < 0 || g->int_last < 0 || g->int_last > MONEY_CAP) return 0;   /* v13.15 */
    if (g->pt_day < 0) return 0;   /* v13.16 */
    if (g->sw_wins < 0 || g->sw_wins > 999) return 0;   /* v10.0 */
    if (g->fr_wins < 0 || g->fr_wins > 999) return 0;   /* v11.0 */
    if (g->career < 0 || g->mrw_n < 0 || g->mrw_n > MRW_N) return 0;   /* v10.5 */
    { int k; for (k = 0; k < g->mrw_n; k++) if (g->mrw_day[k] < 2 || g->mrw_salt[k] <= 0 || (k > 0 && g->mrw_day[k] <= g->mrw_day[k - 1])) return 0; }
    if (g->mm_kind < 0 || g->mm_kind >= MK_N || g->mm_day < 0 || g->kick_day < 0 || (g->mm_kind == MK_NONE && g->mm_day != 0)) return 0;   /* v9.7 */
    if (g->in_match && g->home) { if ((g->mlen != 10 && g->mlen != 20) || g->fee != 0) return 0; }
    else if (g->in_match && (g->mlen != SOPH_HANDS || (g->fee != SOPH_FEE[0] && g->fee != SOPH_FEE[1] && g->fee != SOPH_FEE[2]))) return 0;
    return 1;
}
static inline int load_game(Game *g, const char *path) {
    FILE *f = fopen(path, "rb"); Game t; size_t n; unsigned char raw[sizeof(Game) + 16]; int legacy = 0;
    if (!f) return 0;
    memset(&t, 0, sizeof t);
    n = fread(raw, 1, sizeof raw, f);
    fclose(f);
    if (n == sizeof t) { memcpy(&t, raw, sizeof t); }   /* the current version (save_sane checks the version) */
    else if (n == offsetof(Game, pt_day)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_30) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v13.16: a version-30 save is upgraded: the tournament is open */
    else if (n == offsetof(Game, int_day)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || (t.ver != SAVE_VER_29 && t.ver != SAVE_VER_28)) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v13.15: a version-29 save (v13.2: a version-28 save is the same struct) is upgraded: no interest paid yet */
    else if (n == offsetof(Game, inj)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_27) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v12.3: a version-27 save is upgraded: no injection */
    else if (n == offsetof(Game, pump_life)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_26) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v12.2: a version-26 save is upgraded: the pump gets its life rolled on its next use */
    else if (n == offsetof(Game, deck_given)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_25) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v11.6: a version-25 save is upgraded: see below */
    else if (n == offsetof(Game, cow)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_24) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v11.1: a version-24 save is upgraded: no cow print outfit */
    else if (n == offsetof(Game, fr_wins)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_23) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v11.0: a version-23 save is upgraded: the friendship bar starts at zero */
    else if (n == offsetof(Game, top)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_22) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v10.8: a version-22 save is upgraded: no designer top */
    else if (n == offsetof(Game, career)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_21) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; t.career = g_career_id(); }   /* v10.5: a version-21 save is upgraded: a new career id, no reshuffle */
    else if (n == offsetof(Game, sw_wins)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_20) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v10.0: a version-20 save is upgraded: 0 wins (the black market stays for a game past the proposal) */
    else if (n == offsetof(Game, mm_kind)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_19) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v9.7: a version-19 save is upgraded: she remembers nothing yet */
    else if (n == offsetof(Game, call)) {   /* v9.5: a version-18 save is the same struct without call; a version-17 save is that too, with the old room_up meaning (v9.3) */
        memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || (t.ver != SAVE_VER_18 && t.ver != SAVE_VER_17)) return 0;
        if (t.ver == SAVE_VER_17) legacy = 1;
        t.ver = SAVE_VER; t.size = (int)sizeof t;
    }
    else if (n == offsetof(Game, lot_price)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_16) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v9.0: a version-16 save is upgraded: no ticket */
    else if (n == offsetof(Game, married)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_15) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v8.6: a version-15 save is upgraded: not married, day-8 outfit */
    else if (n == offsetof(Game, lotion)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_14) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v8.2: a version-14 save is upgraded: no new items, no Super drug */
    else if (n == offsetof(Game, stk) + (STK_N13 + 1) * sizeof(int)) {   /* v8.1: a version-13 save (4 companies + seed): upgraded, SONY / SPACEX start at 0 shares */
        int k; memcpy(&t, raw, offsetof(Game, stk));
        if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_13) return 0;
        for (k = 0; k < STK_N13; k++) memcpy(&t.stk[k], raw + offsetof(Game, stk) + k * sizeof(int), sizeof(int));
        memcpy(&t.mseed, raw + offsetof(Game, stk) + STK_N13 * sizeof(int), sizeof(int));
        t.ver = SAVE_VER; t.size = (int)sizeof t;
    }
    else if (n == offsetof(Game, stk)) { memcpy(&t, raw, n); if (t.magic != SAVE_MAGIC || t.ver != SAVE_VER_OLD) return 0; t.ver = SAVE_VER; t.size = (int)sizeof t; }   /* v7.8: a version-12 save is upgraded: no shares, default market seed */
    else return 0;
    if (n != sizeof t && n != offsetof(Game, int_day) && n != offsetof(Game, inj) && n != offsetof(Game, pump_life) && n != offsetof(Game, deck_given) && n != offsetof(Game, cow) && n != offsetof(Game, fr_wins) && n != offsetof(Game, top) && n != offsetof(Game, career) && n != offsetof(Game, call) && n != offsetof(Game, mm_kind) && n != offsetof(Game, sw_wins)) legacy = 1;   /* every file older than version 17 has the old room_up meaning too (a version-18 file does not) */
    if (t.magic == SAVE_MAGIC && n != sizeof t && n != offsetof(Game, int_day) && n != offsetof(Game, inj) && n != offsetof(Game, pump_life)) {   /* v11.6: every older save: pink cards stay if she already lives with you; the romance bar moves to 5 levels */
        t.deck_given = t.rs == RS_LIVING ? 1 : 0;
        if (t.pill_given || t.pill_seen || t.married) { t.rom_lvl = ROM_LVLS; t.rom_pct = 100; }   /* already past the max-romance unlock: nothing locks again */
        else if (t.rom_lvl == 3 && t.rom_pct >= 100) { t.rom_lvl = 4; t.rom_pct = 0; }          /* a full old level 3 starts level 4 */
    }
    if (t.magic == SAVE_MAGIC && legacy) { if (t.room_up == 1 || t.rs == RS_LIVING) t.room_up = 2; else if (t.room_up == 0 && t.story >= ST_HOME) t.room_up = 1; }   /* v9.3: the old upgrade is level 2; a game past the shirt scene already had the computer */
    if (t.magic == SAVE_MAGIC && t.milk_pct > 0 && t.milk_pct <= 90 && t.milk_pct % 10 == 0) t.milk_pct = 50;   /* v7.6: a gauge from the 10-pill days (10..90%) becomes half full: the save is kept */
    if (t.magic == SAVE_MAGIC && t.milk_st == 3 && (t.bottle > 0 || t.milk_until > 0)) t.milk_st = 4;   /* v7.7: a save that already received a bottle opens the milk topics (no save reset) */
    if (!save_sane(&t)) return 0;
    if (t.in_match) g_forfeit(&t);   /* the app was closed during a match: that counts as walking out */
    if (t.story >= ST_HOME) t.event = 1;   /* saves from before the shirt debt: the Exhibition is how you earn it, so it must be open */
    *g = t; return 1;
}
/* v10.5: open a slot to PLAY it (not to look at it): the rewind guard reshuffles the market when the slot is behind the furthest day of its career. *behind = 1 then. */
static inline int g_open_slot(Game *g, const char *path, const char *prog, unsigned salt, int *behind) {
    Game t; int far_day;
    if (behind) *behind = 0;
    if (!load_game(&t, path)) return 0;
    far_day = prog_get(prog, t.career);
    if (g_behind(&t, far_day)) { g_rewind(&t, salt); if (behind) *behind = 1; }
    *g = t; return 1;
}
#endif
