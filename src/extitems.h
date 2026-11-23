/* Poker Mini v9.5 - items.txt and names.txt (no SDL in here, so tests/test_logic.c can test it). Included after extlines.h; it uses its log, its file reader and its text clean-up.

   items.txt   the NAME of every item, the INFO line under its price in the online store, the DESC (the info page, Y), the GIVE name (her give page) and the BOUGHT note.
               One block per item:   [FLOWER]   then   NAME = FLOWER   INFO = GIFT: +10% ROMANCE   ...   Items are found by their key, so the order does not matter.
               Numbers (prices, romance, days) are NOT in the file: they live in the code and the tests. A text that mentions a number is only text.
   names.txt   what you may type with R2 in her menu once you are married: [MAMA] = names that switch her to mama mode, [SOPHIA] = names that switch her back.
               SOPHIA always works, even if the file does not list it. Any other name changes nothing.
   Same safety net as lines.txt: the built-in text is the fallback, a bad entry is skipped and written to text_log.txt, a missing file is written from the built-in text. */
#ifndef PQ_EXTITEMS_H
#define PQ_EXTITEMS_H

/* ------------------------------------------------------------------ items.txt ------------------------------------------------------------------ */
enum { IF_NAME, IF_INFO, IF_DESC, IF_GIVE, IF_BOUGHT, IF_N };
static const char *const IT_FIELD[IF_N] = {"NAME", "INFO", "DESC", "GIVE", "BOUGHT"};
static const int IT_MAXCH[IF_N] = {23, 35, 140, 20, 36};   /* the longest text each place can show (characters at the game's text size); DESC: 5 lines of 28, checked by wrapping */
typedef struct { const char *key; const char **f[IF_N]; const char *what; } ItReg;
#define ITS(k, i, gf, w) {k, {&SH_NAME[i], &SH_INFO[i], &SH_DESC[i], gf >= 0 ? &R_GNAME[gf] : NULL, &SH_BOUGHT[i]}, w}
static const ItReg IT_REG[] = {
    ITS("FLOWER", SH_FLOWER, GF_FLOWER, "online store, ITEMS tab"),
    ITS("MONEY_STACK", SH_STACK, GF_STACK, "online store, ITEMS tab"),
    ITS("BREAST_ENHANCEMENT_PILL", SH_PILL, GF_PILL, "online store, BLACK MARKET tab (her give page calls it PILL by default)"),
    ITS("RUBY_RING", SH_RING, -1, "online store, BLACK MARKET tab (the proposal ring)"),
    ITS("BREAST_PUMP", SH_PUMP, GF_PUMP, "online store, ITEMS tab (after the day-8 scene)"),
    ITS("MILK_INDUCER_PILL", SH_MILK, GF_MILK, "online store, BLACK MARKET tab (after the day-8 scene)"),
    ITS("BODY_LOTION", SH_LOTION, GF_LOTION, "online store, ITEMS tab"),
    ITS("PINK_CARD_DECK", SH_DECK, GF_DECK, "online store, ITEMS tab"),
    ITS("YOGA_OUTFIT", SH_YOGA, GF_YOGA, "online store, ITEMS tab"),
    ITS("GOLD_BAR", SH_GOLD, GF_GOLD, "online store, ITEMS tab"),
    ITS("DIAMOND_RING", SH_DIAMOND, -1, "online store, ITEMS tab (the wedding ring)"),
    ITS("DESIGNER_TOP", SH_TOP, GF_TOP, "online store, ITEMS tab (a gift: a third outfit for after the wedding)"),
    ITS("COW_PRINT_OUTFIT", SH_COW, GF_COW, "online store, ITEMS tab (a gift: a fourth outfit for after the wedding)"),
    ITS("MOMMY_MILK_INJECTION", SH_INJECT, GF_INJECT, "online store, BLACK MARKET tab (after the day-8 scene): permanent milk, no gauge, one milk a day"),
    {"SODA", {&INV_NAME[0], NULL, NULL, NULL, &INV_BOUGHT[0]}, "fridge and lobby vending machine (the machine picture has its own painted text)"},
    {"ENERGY_DRINK", {&INV_NAME[1], NULL, NULL, NULL, &INV_BOUGHT[1]}, "fridge and lobby vending machine (the machine picture has its own painted text)"},
    {"SOPHIA_BREAST_MILK", {&INV_NAME[2], NULL, &MILK_INFO, NULL, NULL}, "fridge (the info page opens with Y on its row)"},
};
#define IT_N ((int)(sizeof(IT_REG) / sizeof(IT_REG[0])))
static const char *it_def[IT_N][IF_N];
static int it_snapped;
static void it_snapshot(void) { int i, k; if (it_snapped) return; it_snapped = 1; for (i = 0; i < IT_N; i++) for (k = 0; k < IF_N; k++) it_def[i][k] = IT_REG[i].f[k] ? *IT_REG[i].f[k] : NULL; }
static PQ_UNUSED void it_reset(void) { int i, k; it_snapshot(); for (i = 0; i < IT_N; i++) for (k = 0; k < IF_N; k++) if (IT_REG[i].f[k]) *IT_REG[i].f[k] = it_def[i][k]; }
static int it_find(const char *key) { int i; for (i = 0; i < IT_N; i++) if (ex_word_is(IT_REG[i].key, key)) return i; return -1; }

/* NULL = fine, else why the text is refused */
static const char *it_text_err(int fld, const char *s) {
    const char *p; int n = (int)strlen(s);
    if (!n) return "empty text";
    for (p = s; *p; p++) {
        int c = toupper((unsigned char)*p);
        if (c < 32 || c > 90) { snprintf(ex_msg, sizeof ex_msg, "the character '%c' cannot be drawn by the game font (no accents, no [ ] { } ~ ^ _ ` | or backslash)", (c >= 32 && c < 127) ? c : '?'); return ex_msg; }
    }
    if (n > IT_MAXCH[fld]) { snprintf(ex_msg, sizeof ex_msg, "too long for its place: %d characters, the limit for %s is %d", n, IT_FIELD[fld], IT_MAXCH[fld]); return ex_msg; }
    if (fld == IF_DESC) { int l = ex_lines_of(s, 28); if (l > 5) { snprintf(ex_msg, sizeof ex_msg, "does not fit the info page: %d lines of 28 characters, the limit is 5", l); return ex_msg; } }
    return NULL;
}
static void it_do_block(int ii, const ExLine *L, int n) {
    const ItReg *r = &IT_REG[ii]; const char *val[IF_N] = {0}; int seen[IF_N] = {0}, i, k;
    for (i = 0; i < n; i++) {
        char *eq = strchr(L[i].s, '='); char nm[32]; int len, fld = -1; const char *e;
        if (!eq) { ex_err(L[i].no, r->key, "a line here must be  FIELD = text  (NAME, INFO, DESC, GIVE or BOUGHT); skipped"); continue; }
        len = (int)(eq - L[i].s); while (len > 0 && L[i].s[len - 1] == ' ') len--;
        if (len <= 0 || len > 30) { ex_err(L[i].no, r->key, "missing field name before the = sign; skipped"); continue; }
        memcpy(nm, L[i].s, (size_t)len); nm[len] = 0;
        for (k = 0; k < IF_N; k++) if (ex_word_is(nm, IT_FIELD[k])) fld = k;
        if (fld < 0) { ex_err(L[i].no, r->key, "unknown field '%s' (use NAME, INFO, DESC, GIVE or BOUGHT); skipped", nm); continue; }
        if (!r->f[fld]) { ex_err(L[i].no, r->key, "this item has no %s place in the game; skipped", IT_FIELD[fld]); continue; }
        if (seen[fld]) { ex_err(L[i].no, r->key, "%s appears twice; the second one is skipped", IT_FIELD[fld]); continue; }
        seen[fld] = 1;
        e = it_text_err(fld, ex_trim(eq + 1)); if (e) { ex_err(L[i].no, r->key, "%s: %s; the built-in text stays", IT_FIELD[fld], e); continue; }
        val[fld] = ex_trim(eq + 1);
    }
    for (k = 0; k < IF_N; k++) if (val[k]) *r->f[k] = ex_dup(val[k]);
}
/* buf is modified. Returns the number of item blocks applied. */
static PQ_UNUSED int it_parse(char *buf) {
    ExLine *ln; int nl = 0, cap = 512, i, applied = 0; char *p, *q; char seen[IT_N + 1];
    it_snapshot(); ex_cur_file = "items.txt"; memset(seen, 0, sizeof seen);
    if ((unsigned char)buf[0] == 0xEF && (unsigned char)buf[1] == 0xBB && (unsigned char)buf[2] == 0xBF) buf += 3;
    ln = (ExLine *)malloc((size_t)cap * sizeof(ExLine)); if (!ln) return 0;
    for (p = buf, i = 1; *p; i++) {
        q = strchr(p, '\n'); if (q) *q = 0;
        { size_t l = strlen(p); if (l && p[l - 1] == '\r') p[l - 1] = 0; }
        ex_ascii(p);
        if (nl == cap) { ExLine *t = (ExLine *)realloc(ln, (size_t)(cap *= 2) * sizeof(ExLine)); if (!t) break; ln = t; }
        ln[nl].s = ex_trim(p); ln[nl].no = i; nl++;
        if (!q) break;
        p = q + 1;
    }
    for (i = 0; i < nl; ) {
        char *s = ln[i].s;
        if (s[0] == '[') {
            char name[64]; char *e = strchr(s, ']'); int ii, j, len, k, ns = 0; ExLine *sec;
            if (!e) { ex_err(ln[i].no, "", "a block header needs a closing ]; the lines below it are ignored"); i++; continue; }
            len = (int)(e - s) - 1; if (len > 62) len = 62; memcpy(name, s + 1, (size_t)len); name[len] = 0;
            { char *t = ex_trim(name); if (t != name) memmove(name, t, strlen(t) + 1); }
            for (j = i + 1; j < nl && ln[j].s[0] != '['; j++) {}
            ii = it_find(name);
            if (ii < 0) { if (!ex_word_is(name, "SUPER_MILK_DRUG")) ex_err(ln[i].no, name, "unknown item key (check the spelling); ignored"); }   /* v13.17: the Super milk drug was removed: an old items.txt may still have its block, ignored without a complaint */
            else if (seen[ii]) ex_err(ln[i].no, name, "the block appears twice; the second one is ignored");
            else {
                seen[ii] = 1; sec = (ExLine *)malloc((size_t)(j - i) * sizeof(ExLine));
                if (sec) { for (k = i + 1; k < j; k++) if (ln[k].s[0] && ln[k].s[0] != '#') sec[ns++] = ln[k]; it_do_block(ii, sec, ns); applied++; free(sec); }
            }
            i = j;
        } else { if (s[0] && s[0] != '#') ex_err(ln[i].no, "", "this line is outside any [ITEM] block; ignored"); i++; }
    }
    free(ln); return applied;
}
static PQ_UNUSED int it_load_string(const char *text) { char *b = ex_dup(text); int n; if (!b) return 0; n = it_parse(b); free(b); return n; }
static PQ_UNUSED int it_dump(const char *path) {
    FILE *f = fopen(path, "w"); int i, k;
    if (!f) return 0;
    fputs("# POKER MINI - items.txt: the names and info texts of the items (online store, fridge, her give page).\n"
          "# Edit with Notepad (plain text). Keep the name items.txt next to the game. The full guide is TEXT_GUIDE.txt. Delete this file to get a fresh copy.\n"
          "# [KEY] starts an item (do not rename the keys). Lines are  FIELD = text  and may be left out: the built-in text is used then.\n"
          "#   NAME   the item's name (23 characters at most)          INFO    the small line under the price in the store (35 characters)\n"
          "#   DESC   the info page that opens with Y (5 lines of 28 characters, about 140 characters)   GIVE   its name on her give page (20 characters)\n"
          "#   BOUGHT the note after you buy it (36 characters)\n"
          "# Prices, romance values and days are NOT here: they are part of the game rules. If you change a number in a text, only the text changes.\n"
          "# Anything the game refuses is skipped and written to text_log.txt; the built-in text is used instead. Plain characters only: A-Z 0-9 and . , ! ? ' \" - : ; ( ) $ & + = / * < > @ # %\n", f);
    for (i = 0; i < IT_N; i++) {
        const ItReg *r = &IT_REG[i];
        fprintf(f, "\n# %s\n[%s]\n", r->what, r->key);
        for (k = 0; k < IF_N; k++) {
            if (!r->f[k]) continue;
            if (*r->f[k]) fprintf(f, "%s = %s\n", IT_FIELD[k], *r->f[k]);
            else fprintf(f, "# %s = (this item has no info page yet: write a DESC line to add one)\n", IT_FIELD[k]);
        }
    }
    fclose(f); return 1;
}

/* ------------------------------------------------------------------ names.txt ------------------------------------------------------------------ */
#define NM_LEN 16          /* the longest name the on-screen keyboard takes */
#define NM_MAX 32          /* names per list */
static char nm_mama[NM_MAX][NM_LEN + 1] = {"MAMA", "MAMA SOPHIA"};
static int nm_mama_n = 2;
static char nm_soph[NM_MAX][NM_LEN + 1] = {"SOPHIA"};
static int nm_soph_n = 1;
/* the typed name made comparable: capitals, one space between words, no space at either end. Returns the length. */
static PQ_UNUSED int nm_norm(char *out, int cap, const char *in) {
    int n = 0, sp = 0;
    for (; *in && n + 1 < cap; in++) {
        int c = toupper((unsigned char)*in);
        if (c == ' ') { sp = n > 0; continue; }
        if (sp) { out[n++] = ' '; sp = 0; if (n + 1 >= cap) break; }
        out[n++] = (char)c;
    }
    out[n] = 0; return n;
}
/* what the typed name does: 1 = mama mode, 0 = back to SOPHIA, -1 = nothing (not a name she answers to) */
static PQ_UNUSED int nm_kind(const char *typed) {
    char t[NM_LEN + 8]; int i;
    if (!nm_norm(t, (int)sizeof t, typed)) return -1;
    for (i = 0; i < nm_mama_n; i++) if (!strcmp(t, nm_mama[i])) return 1;
    if (!strcmp(t, "SOPHIA")) return 0;   /* always works: you can always go back */
    for (i = 0; i < nm_soph_n; i++) if (!strcmp(t, nm_soph[i])) return 0;
    return -1;
}
static PQ_UNUSED void nm_reset(void) { memset(nm_mama, 0, sizeof nm_mama); memset(nm_soph, 0, sizeof nm_soph); strcpy(nm_mama[0], "MAMA"); strcpy(nm_mama[1], "MAMA SOPHIA"); nm_mama_n = 2; strcpy(nm_soph[0], "SOPHIA"); nm_soph_n = 1; }
static PQ_UNUSED int nm_parse(char *buf) {
    ExLine *ln; int nl = 0, cap = 128, i, applied = 0; char *p, *q;
    char tm[NM_MAX][NM_LEN + 1], ts[NM_MAX][NM_LEN + 1]; int nm = 0, ns = 0, sm = 0, ss = 0;
    ex_cur_file = "names.txt";
    if ((unsigned char)buf[0] == 0xEF && (unsigned char)buf[1] == 0xBB && (unsigned char)buf[2] == 0xBF) buf += 3;
    ln = (ExLine *)malloc((size_t)cap * sizeof(ExLine)); if (!ln) return 0;
    for (p = buf, i = 1; *p; i++) {
        q = strchr(p, '\n'); if (q) *q = 0;
        { size_t l = strlen(p); if (l && p[l - 1] == '\r') p[l - 1] = 0; }
        ex_ascii(p);
        if (nl == cap) { ExLine *t = (ExLine *)realloc(ln, (size_t)(cap *= 2) * sizeof(ExLine)); if (!t) break; ln = t; }
        ln[nl].s = ex_trim(p); ln[nl].no = i; nl++;
        if (!q) break;
        p = q + 1;
    }
    { int cur = -1;   /* 0 = [MAMA], 1 = [SOPHIA] */
      for (i = 0; i < nl; i++) {
        char *s = ln[i].s;
        if (!s[0] || s[0] == '#') continue;
        if (s[0] == '[') {
            char name[40]; char *e = strchr(s, ']'); int len;
            if (!e) { ex_err(ln[i].no, "", "a section header needs a closing ]"); cur = -1; continue; }
            len = (int)(e - s) - 1; if (len > 38) len = 38; memcpy(name, s + 1, (size_t)len); name[len] = 0;
            { char *t = ex_trim(name); if (ex_word_is(t, "MAMA")) { cur = 0; if (sm) ex_err(ln[i].no, "MAMA", "the section appears twice; the second one is ignored"), cur = -1; sm = 1; }
              else if (ex_word_is(t, "SOPHIA")) { cur = 1; if (ss) ex_err(ln[i].no, "SOPHIA", "the section appears twice; the second one is ignored"), cur = -1; ss = 1; }
              else { ex_err(ln[i].no, t, "unknown section (use [MAMA] or [SOPHIA]); ignored"); cur = -1; } }
            continue;
        }
        if (cur < 0) { ex_err(ln[i].no, "", "this line is outside a usable [MAMA] / [SOPHIA] section; ignored"); continue; }
        { char t[NM_LEN + 8]; int bad = 0; const char *c; int k, dup = 0;
          for (c = s; *c; c++) { int ch = toupper((unsigned char)*c); if (!((ch >= 'A' && ch <= 'Z') || ch == ' ')) bad = 1; }
          if (bad) { ex_err(ln[i].no, cur ? "SOPHIA" : "MAMA", "a name can only have the letters A-Z and spaces (the on-screen keyboard has nothing else); skipped"); continue; }
          if ((int)strlen(s) > 4 * NM_LEN) { ex_err(ln[i].no, cur ? "SOPHIA" : "MAMA", "too long (the keyboard takes %d characters at most); skipped", NM_LEN); continue; }
          if (!nm_norm(t, (int)sizeof t, s)) continue;
          if (cur == 0 && !strcmp(t, "SOPHIA")) { ex_err(ln[i].no, "MAMA", "SOPHIA always switches her back, it cannot be a mama name; skipped"); continue; }
          if ((int)strlen(t) > NM_LEN) { ex_err(ln[i].no, cur ? "SOPHIA" : "MAMA", "too long (the keyboard takes %d characters at most); skipped", NM_LEN); continue; }
          for (k = 0; k < nm; k++) if (!strcmp(tm[k], t)) dup = 1;
          for (k = 0; k < ns; k++) if (!strcmp(ts[k], t)) dup = 1;
          if (dup) { ex_err(ln[i].no, cur ? "SOPHIA" : "MAMA", "this name is already listed (a name can only do one thing); skipped"); continue; }
          if (cur == 0) { if (nm < NM_MAX) memcpy(tm[nm++], t, strlen(t) + 1); else ex_err(ln[i].no, "MAMA", "more than %d names; the rest are ignored", NM_MAX); }
          else { if (ns < NM_MAX) memcpy(ts[ns++], t, strlen(t) + 1); else ex_err(ln[i].no, "SOPHIA", "more than %d names; the rest are ignored", NM_MAX); } }
      } }
    free(ln);
    if (nm) { memset(nm_mama, 0, sizeof nm_mama); memcpy(nm_mama, tm, sizeof(tm[0]) * (size_t)nm); nm_mama_n = nm; applied++; }
    else if (sm) ex_err(0, "MAMA", "no usable name; the built-in names stay");
    if (ns) { memset(nm_soph, 0, sizeof nm_soph); memcpy(nm_soph, ts, sizeof(ts[0]) * (size_t)ns); nm_soph_n = ns; applied++; }
    else if (ss) ex_err(0, "SOPHIA", "no usable name; the built-in name stays");
    return applied;
}
static PQ_UNUSED int nm_load_string(const char *text) { char *b = ex_dup(text); int n; if (!b) return 0; n = nm_parse(b); free(b); return n; }
static PQ_UNUSED int nm_dump(const char *path) {
    FILE *f = fopen(path, "w"); int i;
    if (!f) return 0;
    fputs("# POKER MINI - names.txt: the names you can type for her (R2 in her menu, once you are married).\n"
          "# Keep the name names.txt next to the game. Delete this file to get a fresh copy. The full guide is TEXT_GUIDE.txt.\n"
          "# One name per line, letters A-Z and spaces only (the on-screen keyboard has nothing else), 16 characters at most. Upper or lower case does not matter.\n"
          "# [MAMA]    names that switch her to mama mode (her mama dialogue and the BREASTFEED row)\n"
          "# [SOPHIA]  names that switch her back. SOPHIA itself always works, even if you delete it here.\n"
          "# Any other name changes nothing. Add as many names as you like. Anything refused is written to text_log.txt.\n\n[MAMA]\n", f);
    for (i = 0; i < nm_mama_n; i++) fprintf(f, "%s\n", nm_mama[i]);
    fputs("\n[SOPHIA]\n", f);
    for (i = 0; i < nm_soph_n; i++) fprintf(f, "%s\n", nm_soph[i]);
    fclose(f); return 1;
}

/* ------------------------------------------------------------------ the entry point ------------------------------------------------------------------ */
/* lines.txt, talk.txt, items.txt and names.txt in one go (one text_log.txt for all). A missing file is written from the built-in text when dump_missing is set. Returns the number of problems. */
static PQ_UNUSED int ex_load_all4(const char *lpath, const char *tpath, const char *ipath, const char *npath, const char *logpath, int dump_missing) {
    const char *path[2]; int f; char *b; path[0] = lpath; path[1] = tpath;
    ex_snapshot(); it_snapshot(); ex_nlog = ex_nprob = 0;
    for (f = 0; f < 2; f++) {
        b = ex_read(path[f]);
        ex_cur_file = EX_FILE_NAME[f];
        if (!b) { if (dump_missing && ex_rd_missing) ex_dump(path[f], f); continue; }
        ex_parse(b, f); free(b);
    }
    b = ex_read(ipath); ex_cur_file = "items.txt";
    if (!b) { if (dump_missing && ex_rd_missing) it_dump(ipath); } else { it_parse(b); free(b); }
    b = ex_read(npath); ex_cur_file = "names.txt";
    if (!b) { if (dump_missing && ex_rd_missing) nm_dump(npath); } else { nm_parse(b); free(b); }
    ex_log_write(logpath);
    return ex_nprob;
}
#endif
