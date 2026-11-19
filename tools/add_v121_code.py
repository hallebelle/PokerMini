#!/usr/bin/env python3
"""v12.1 (one-off): the cow mode hooks in src/main.c. Run once on the v12.0 sources after add_v121.py:  python3 tools/add_v121_code.py .   (already applied)"""
import sys
root = sys.argv[1] if len(sys.argv) > 1 else "."
M = open(root + "/src/main.c").read()
def sub(old, new, cnt=1):
    global M
    assert M.count(old) == cnt, (M.count(old), old[:90])
    M = M.replace(old, new)

# 1. outfit change remembers where she came from (the relieved line when she leaves the cow outfit)
sub("static const char *outfit_line(void) {   /* v10.8: her line when she has changed into the outfit G.outfit (mama mode has its own) */\n    int mm = g_mama(&G);\n",
    "static int outfit_from = OF_USUAL;   /* v12.1: the outfit she wore before the last change (leaving the cow outfit gives a relieved line) */\n"
    "static void outfit_apply(int k) { outfit_from = G.outfit; g_outfit_set(&G, k); }\n"
    "static const char *outfit_line(void) {   /* v10.8: her line when she has changed into the outfit G.outfit (mama mode has its own) */\n    int mm = g_mama(&G);\n"
    "    if (outfit_from == OF_COW && G.outfit != OF_COW) { outfit_from = OF_USUAL; return mm ? pick(R_OUTFIT_UNCOW_MM, R_OUTFIT_UNCOW_MM_N) : pick(R_OUTFIT_UNCOW, R_OUTFIT_UNCOW_N); }   /* v12.1 */\n")
sub("case SCR_OUTFIT: { g_outfit_set(&G, ow_target); persist();", "case SCR_OUTFIT: { outfit_apply(ow_target); persist();")
sub("if (!outfit_ask(wt[wk_wsel], 1)) { g_outfit_set(&G, wt[wk_wsel]); persist();", "if (!outfit_ask(wt[wk_wsel], 1)) { outfit_apply(wt[wk_wsel]); persist();")

# 2. greetings (day / evening and night)
sub("    if (g_p4_open(&G)) return g_mama(&G) ? mm_greet(pick(R_GREET_MM, R_GREET_MM_N)) : pick(R_GREET_M, R_GREET_M_N);",
    "    if (g_p4_open(&G)) { int cw = g_cow(&G); return g_mama(&G) ? mm_greet(cw ? pick(R_GREET_CW_MM, R_GREET_CW_MM_N) : pick(R_GREET_MM, R_GREET_MM_N)) : cw ? pick(R_GREET_CW, R_GREET_CW_N) : pick(R_GREET_M, R_GREET_M_N); }   /* v12.1: cow mode */")
sub("static void lr_here(void) {",
    "static const char *notfull_line(void);   /* v12.1 */\n"
    "static const char *night_greet(void) {   /* v12.1: her greeting when you knock at night, married (cow mode has its own) */\n"
    "    int cw = g_cow(&G);\n"
    "    return g_mama(&G) ? mm_greet(cw ? pick(R_NIGHT_CW_MM, R_NIGHT_CW_MM_N) : pick(R_NIGHT_MM, R_NIGHT_MM_N)) : cw ? pick(R_NIGHT_CW, R_NIGHT_CW_N) : pick(R_NIGHT_M, R_NIGHT_M_N);\n}\n"
    "static void lr_here(void) {")
sub("(g_mama(&G) ? mm_greet(pick(R_NIGHT_MM, R_NIGHT_MM_N)) : pick(R_NIGHT_M, R_NIGHT_M_N))", "night_greet()")

# 3. cow-mode openers and mid pairs (like the mama ones, one pair of beats)
sub("static int mm_react_on = 1;\n",
    "static int mm_react_on = 1;\n"
    "/* v12.1: COW MODE pairs (your line, her disgusted answer that still agrees). Slots follow OP_CW0: normal first, then the Mama set. */\n"
    "enum { CO_ASK, CO_PILL, CO_SUPER, CO_MID_PUMP, CO_MID_FEED, CO_N };\n"
    "static int cow_slot(int base) { return OP_CW0 + (g_mama(&G) ? CO_N : 0) + base; }\n")
sub("OP_MID_PUMP, OP_MID_FEED, OP_N };", "OP_MID_PUMP, OP_MID_FEED, OP_CW0, OP_N = OP_CW0 + 10 };")
sub("static int open_last[OP_N] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, bottle_pill;",
    "static int open_last[OP_N] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, bottle_pill;")
sub("&B_RM_FEED_MM, &B_RM_ASK_MM, &B_RM_PILL_MM, &B_RM_SUPER_MM, &B_PLEA_FEED_MM, &B_PLEA_ASK_MM, &B_PLEA_PILL_MM, &B_PLEA_SUPER_MM, &B_MID_PUMP_MM, &B_MID_FEED_MM};",
    "&B_RM_FEED_MM, &B_RM_ASK_MM, &B_RM_PILL_MM, &B_RM_SUPER_MM, &B_PLEA_FEED_MM, &B_PLEA_ASK_MM, &B_PLEA_PILL_MM, &B_PLEA_SUPER_MM, &B_MID_PUMP_MM, &B_MID_FEED_MM,\n"
    "    &B_OPEN_ASK_CW, &B_OPEN_PILL_CW, &B_OPEN_SUPER_CW, &B_MID_PUMP_CW, &B_MID_FEED_CW, &B_OPEN_ASK_CW_MM, &B_OPEN_PILL_CW_MM, &B_OPEN_SUPER_CW_MM, &B_MID_PUMP_CW_MM, &B_MID_FEED_CW_MM};")
sub("&B_RM_FEED_MM_N, &B_RM_ASK_MM_N, &B_RM_PILL_MM_N, &B_RM_SUPER_MM_N, &B_PLEA_FEED_MM_N, &B_PLEA_ASK_MM_N, &B_PLEA_PILL_MM_N, &B_PLEA_SUPER_MM_N, &B_MID_PUMP_MM_N, &B_MID_FEED_MM_N};",
    "&B_RM_FEED_MM_N, &B_RM_ASK_MM_N, &B_RM_PILL_MM_N, &B_RM_SUPER_MM_N, &B_PLEA_FEED_MM_N, &B_PLEA_ASK_MM_N, &B_PLEA_PILL_MM_N, &B_PLEA_SUPER_MM_N, &B_MID_PUMP_MM_N, &B_MID_FEED_MM_N,\n"
    "    &B_OPEN_ASK_CW_N, &B_OPEN_PILL_CW_N, &B_OPEN_SUPER_CW_N, &B_MID_PUMP_CW_N, &B_MID_FEED_CW_N, &B_OPEN_ASK_CW_MM_N, &B_OPEN_PILL_CW_MM_N, &B_OPEN_SUPER_CW_MM_N, &B_MID_PUMP_CW_MM_N, &B_MID_FEED_CW_MM_N};")
sub("static void mid_insert(int slot, int at) {",
    "static void cow_open_prepend(int base) {   /* v12.1: cow mode: one pair of beats in front of the scene (no memory, no plea: her answer already gives in) */\n"
    "    Beat op[2]; if (!open_pair(cow_slot(base), op)) return;\n"
    "    memmove(&vn_buf[2], &vn_buf[0], (size_t)vn_n * sizeof(Beat)); vn_buf[0] = op[0]; vn_buf[1] = op[1]; vn_n += 2;\n}\n"
    "static void cow_mid(int base, int at) {   /* v12.1: cow mode: one pair in the middle of the scene, before beat at (picture and sprite of the beat before it) */\n"
    "    Beat op[2]; if (at < 1 || at > vn_n || !open_pair(cow_slot(base), op)) return;\n"
    "    memmove(&vn_buf[at + 2], &vn_buf[at], (size_t)(vn_n - at) * sizeof(Beat)); vn_buf[at] = op[0]; vn_buf[at + 1] = op[1];\n"
    "    vn_buf[at].bg = vn_buf[at + 1].bg = vn_buf[at - 1].bg; vn_buf[at].spr = vn_buf[at + 1].spr = vn_buf[at - 1].spr; vn_n += 2;\n}\n"
    "static void mid_insert(int slot, int at) {")

# 4. scenes: hug, kiss, sleep, feed, night knock
sub("        case SCR_HUG: if (g_mama(&G)) { *n = B_HUG_MM_N; return B_HUG_MM; }",
    "        case SCR_HUG: if (g_cow(&G)) { if (g_mama(&G)) { *n = B_HUG_CW_MM_N; return B_HUG_CW_MM; } *n = B_HUG_CW_N; return B_HUG_CW; }   /* v12.1: cow mode */\n                      if (g_mama(&G)) { *n = B_HUG_MM_N; return B_HUG_MM; }")
sub("        case SCR_KISS: if (g_mama(&G)) { *n = B_KISS_MM_N; return B_KISS_MM; }",
    "        case SCR_KISS: if (g_cow(&G)) { if (g_mama(&G)) { *n = B_KISS_CW_MM_N; return B_KISS_CW_MM; } *n = B_KISS_CW_N; return B_KISS_CW; }\n                       if (g_mama(&G)) { *n = B_KISS_MM_N; return B_KISS_MM; }")
sub("        case SCR_SLEEP: if (g_mama(&G)) { *n = B_SLEEP_MM_N; return B_SLEEP_MM; }",
    "        case SCR_SLEEP: if (g_cow(&G)) { if (g_mama(&G)) { *n = B_SLEEP_CW_MM_N; return B_SLEEP_CW_MM; } *n = B_SLEEP_CW_N; return B_SLEEP_CW; }\n                        if (g_mama(&G)) { *n = B_SLEEP_MM_N; return B_SLEEP_MM; }")
sub("        case SCR_NIGHTM: if (g_mama(&G)) { *n = B_NIGHTM_MM_N; return B_NIGHTM_MM; }",
    "        case SCR_NIGHTM: if (g_cow(&G)) { if (g_mama(&G)) { *n = B_NIGHTM_CW_MM_N; return B_NIGHTM_CW_MM; } *n = B_NIGHTM_CW_N; return B_NIGHTM_CW; }\n                         if (g_mama(&G)) { *n = B_NIGHTM_MM_N; return B_NIGHTM_MM; }")
sub("        case SCR_FEED: if (lr_on()) { if (g_mama(&G)) {",
    "        case SCR_FEED: if (g_cow(&G)) { int mm = g_mama(&G);   /* v12.1: cow mode */\n"
    "                           if (lr_on()) { if (mm) { *n = B_FEED_CW_MM_LR_N; return B_FEED_CW_MM_LR; } *n = B_FEED_CW_LR_N; return B_FEED_CW_LR; }\n"
    "                           if (mm) { *n = B_FEED_CW_MM_N; return B_FEED_CW_MM; } *n = B_FEED_CW_N; return B_FEED_CW; }\n"
    "                       if (lr_on()) { if (g_mama(&G)) {")

# 5. vn_start hooks
sub("        case SCR_HUG: vn_buf[0].txt = g_mama(&G) ? pick(R_HUG_OPEN_MM, R_HUG_OPEN_MM_N) : pick(R_HUG_OPEN, R_HUG_OPEN_N); break;",
    "        case SCR_HUG: vn_buf[0].txt = g_cow(&G) ? (g_mama(&G) ? pick(R_HUG_OPEN_CW_MM, R_HUG_OPEN_CW_MM_N) : pick(R_HUG_OPEN_CW, R_HUG_OPEN_CW_N)) : g_mama(&G) ? pick(R_HUG_OPEN_MM, R_HUG_OPEN_MM_N) : pick(R_HUG_OPEN, R_HUG_OPEN_N); break;")
sub("        case SCR_KISS: vn_buf[0].txt = g_mama(&G) ? pick(R_KISS_OPEN_MM, R_KISS_OPEN_MM_N) : pick(R_KISS_OPEN, R_KISS_OPEN_N); break;",
    "        case SCR_KISS: vn_buf[0].txt = g_cow(&G) ? (g_mama(&G) ? pick(R_KISS_OPEN_CW_MM, R_KISS_OPEN_CW_MM_N) : pick(R_KISS_OPEN_CW, R_KISS_OPEN_CW_N)) : g_mama(&G) ? pick(R_KISS_OPEN_MM, R_KISS_OPEN_MM_N) : pick(R_KISS_OPEN, R_KISS_OPEN_N); break;")
# the milk pill / super drug gift scene
sub("            if (g_mama(&G) && (gift_kind == GF_MILK || gift_kind == GF_SUPER)) { if (lr_on())",
    "            if (g_cow(&G) && (gift_kind == GF_MILK || gift_kind == GF_SUPER)) cow_open_prepend(gift_kind == GF_SUPER ? CO_SUPER : CO_PILL);   /* v12.1: cow mode: disgusted, but she takes it */\n"
    "            else if (g_mama(&G) && (gift_kind == GF_MILK || gift_kind == GF_SUPER)) { if (lr_on())")
# the pump scene
sub("            if (lr) { if (g_mama(&G)) { int sl = bottle_pill ? OP_PILL_LR : OP_ASK_LR, before = vn_n; open_prepend(sl); if (vn_n == before) lr_here(); } else lr_here(); }\n"
    "            else if (g_mama(&G) && mm_react_on) open_prepend(bottle_pill ? OP_PILL : OP_ASK);",
    "            if (g_cow(&G)) { cow_open_prepend(bottle_pill ? CO_PILL : CO_ASK); if (lr) lr_here(); cow_mid(CO_MID_PUMP, vn_n - 1); }   /* v12.1: cow mode: Here?! first (living room), then her disgusted yes */\n"
    "            else {\n"
    "            if (lr) { if (g_mama(&G)) { int sl = bottle_pill ? OP_PILL_LR : OP_ASK_LR, before = vn_n; open_prepend(sl); if (vn_n == before) lr_here(); } else lr_here(); }\n"
    "            else if (g_mama(&G) && mm_react_on) open_prepend(bottle_pill ? OP_PILL : OP_ASK);")
sub("            if (g_mama(&G) && mm_react_on) mid_insert(OP_MID_PUMP, vn_n - 1);",
    "            if (g_mama(&G) && mm_react_on) mid_insert(OP_MID_PUMP, vn_n - 1);\n            }")
# breastfeed
sub("            int on = g_mama(&G), got = 0;\n            if (lr_on()) { if (on) got = open_replace(OP_FEED_LR); else lr_here(); }",
    "            int on = g_mama(&G), cw = g_cow(&G), got = 0;   /* v12.1: cow mode has its own scenes (the opener is inside them) and its own mid pair */\n            if (lr_on()) { if (on && !cw) got = open_replace(OP_FEED_LR); else lr_here(); }")
sub("                if (on) got = open_replace(OP_FEED);   /* v10.4: at her door and in her bedroom too */",
    "                if (on && !cw) got = open_replace(OP_FEED);   /* v10.4: at her door and in her bedroom too */")
sub("            if (on && mm_react_on && vn_n >= 4) mid_insert(OP_MID_FEED, vn_n - 2);",
    "            if (cw) { if (vn_n >= 4) cow_mid(CO_MID_FEED, vn_n - 2); }\n            else if (on && mm_react_on && vn_n >= 4) mid_insert(OP_MID_FEED, vn_n - 2);")

# 6. chat, her line after a chat, 'not full'
sub("static int chat_last_mama = -1;\n",
    "static int chat_last_mama = -1, chat_last_cow = -1;\n"
    "static const char *more_line(void) {   /* v12.1: her line when the chat is over and her menu comes back */\n"
    "    int cw = g_cow(&G);\n"
    "    return g_mama(&G) ? (cw ? pick(R_MORE_CW_MM, R_MORE_CW_MM_N) : pick(R_MORE_MM, R_MORE_MM_N)) : cw ? pick(R_MORE_CW, R_MORE_CW_N) : g_p4_open(&G) ? pick(R_MORE_M, R_MORE_M_N) : pick(R_MORE, R_MORE_N);\n}\n"
    "static const char *notfull_line(void) {   /* v12.1: 'not full yet' */\n"
    "    int cw = g_cow(&G);\n"
    "    return g_mama(&G) ? (cw ? pick(R_NOT_FULL_CW_MM, R_NOT_FULL_CW_MM_N) : pick(R_NOT_FULL_MM, R_NOT_FULL_MM_N)) : cw ? pick(R_NOT_FULL_CW, R_NOT_FULL_CW_N) : pick(R_NOT_FULL, R_NOT_FULL_N);\n}\n")
sub("    int tp = 0, ex, tries = 0; const Talk *t; int mm = g_mama(&G);\n    if (mm) {",
    "    int tp = 0, ex, tries = 0; const Talk *t; int mm = g_mama(&G), cw = g_cow(&G);\n"
    "    if (cw) { const TalkPool *cp = mm ? &C_COW_MM : &C_COW; ex = pick_new(cp->n, chat_last_cow); chat_last_cow = ex; t = &cp->t[ex]; }   /* v12.1: cow mode has its own chat */\n"
    "    else if (mm) {")
sub("do { f = mm ? pick(C_FAIL_MM, C_FAIL_MM_N) : pick(C_FAIL, C_FAIL_N); }",
    "do { f = cw ? (mm ? pick(C_FAIL_CW_MM, C_FAIL_CW_MM_N) : pick(C_FAIL_CW, C_FAIL_CW_N)) : mm ? pick(C_FAIL_MM, C_FAIL_MM_N) : pick(C_FAIL, C_FAIL_N); }")
sub("rom_open(g_mama(&G) ? pick(R_MORE_MM, R_MORE_MM_N) : g_p4_open(&G) ? pick(R_MORE_M, R_MORE_M_N) : pick(R_MORE, R_MORE_N));", "rom_open(more_line());")
sub("g_mama(&G) ? pick(R_NOT_FULL_MM, R_NOT_FULL_MM_N) : pick(R_NOT_FULL, R_NOT_FULL_N)", "notfull_line()", cnt=2)

# 7. debug panel: COW MODE ON
sub("#define DBG_N 8\n", "#define DBG_N 9\n")
sub('"ROOM UPGRADE (FREE)", "BACK"}', '"ROOM UPGRADE (FREE)", "COW MODE ON (OUTFIT)", "BACK"}')
sub("            default: scr = g_computer(&G) ? SC_SHOP : SC_WALK; break;\n        }\n    }\n}\n/* v8.4: the romance bar",
    "            case 7: if (!g_p4_open(&G)) dbg_note(\"ONLY AFTER THE WEDDING\"); else { G.cow_given = 1; G.outfit = OF_COW; persist(); dbg_note(\"COW MODE ON (SHE WEARS THE COW OUTFIT)\"); } break;   /* v12.1 */\n"
    "            default: scr = g_computer(&G) ? SC_SHOP : SC_WALK; break;\n        }\n    }\n}\n/* v8.4: the romance bar")
open(root + "/src/main.c", "w").write(M)
print("ok: v12.1 hooks written")
