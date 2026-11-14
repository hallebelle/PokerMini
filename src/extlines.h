/* Poker Mini v7.9 - EXTERNAL TEXT FILES. No SDL in here, so tests/test_logic.c can test it.

   lines.txt  and  talk.txt  (next to the program, like pokerqueen.cfg) can replace the built-in texts of src/lines.h and src/talk.h.
   - The built-in text is always the fallback: a missing file, a missing section or a bad entry just leaves the built-in text in place.
   - Nothing here can crash the game: every entry is checked (characters the font can draw, page size, %d placeholders, scene shape) BEFORE it is used.
   - Problems are written to text_log.txt (file, line number, section, reason). No problems = no log file.
   - If a file is missing the game writes it from the built-in text (first-run dump), so there is always a template to edit.
   - The list of editable items is src/extreg.h (generated list, see "developer" in TEXT_GUIDE.txt).
   Formats (full description in TEXT_GUIDE.txt):
     [NAME]            starts a section; one entry per line; '#' starts a comment; blank lines are ignored
     pools / singles   one text per line  (table lines use | to split into two lines)
     scenes (B_...)    WHO BACKGROUND SPRITE | text        (BACKGROUND and SPRITE may be left out: same as the beat before)
     talk.txt          > what you say   then   < her answer   and optionally a second   < line ;  a blank line or the next > starts a new exchange */
#ifndef PQ_EXTLINES_H
#define PQ_EXTLINES_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "logic.h"
#include "lines.h"
#include "extreg.h"

#define EX_MAXTEXT   300        /* longest single text */
#define EX_MAXPOOL   256        /* most entries in a pool / most exchanges in a topic */
#define EX_MAXBEATS  60         /* most beats in a scene (vn_buf in main.c holds 200: scene + lead-in + scolding) */
#define EX_MAXLEAD   24         /* most lead-ins */
#define EX_MAXFILE   (1500 * 1024)
#define EX_MAXLOG    200

static const char *const EX_FILE_NAME[2] = {"lines.txt", "talk.txt"};
static const char *const EX_WHO[6] = {"NAR", "ME", "UNK", "AW", "SOPHIA", "HER"};
static const char *const EX_BG[BG_N] = {"LOBBY", "MYROOM", "HALL", "LIVING", "TABLE", "EVENT", "EXHIB", "LOBBYMAP", "RAIN", "SROOM", "SLEEP", "CLOSE", "LONG", "TORSO", "ROM", "BDOOR", "PUMP", "HUG", "KISS", "SLEEPT", "FEED", "BOTTLE"};

/* ------------------------------------------------------------------ the log ------------------------------------------------------------------ */
static char ex_log[EX_MAXLOG][200];
static int ex_nlog, ex_nprob;                  /* ex_nprob counts every problem (ex_log keeps the first EX_MAXLOG) */
static const char *ex_cur_file = "";
static void ex_err(int lineno, const char *sec, const char *fmt, ...) {
    char m[150]; va_list ap; va_start(ap, fmt); vsnprintf(m, sizeof m, fmt, ap); va_end(ap);
    ex_nprob++;
    if (ex_nlog < EX_MAXLOG) {
        if (lineno > 0) snprintf(ex_log[ex_nlog], sizeof ex_log[0], "%s line %d [%s]: %s", ex_cur_file, lineno, sec ? sec : "", m);
        else snprintf(ex_log[ex_nlog], sizeof ex_log[0], "%s [%s]: %s", ex_cur_file, sec ? sec : "", m);
        ex_nlog++;
    }
}
static void ex_log_write(const char *path) {
    FILE *f; int i;
    if (!ex_nprob) { remove(path); return; }
    f = fopen(path, "w"); if (!f) return;
    fprintf(f, "Poker Mini text problems: %d. The built-in text is used wherever a line was rejected.\n", ex_nprob);
    for (i = 0; i < ex_nlog; i++) fprintf(f, "%s\n", ex_log[i]);
    if (ex_nprob > ex_nlog) fprintf(f, "... and %d more.\n", ex_nprob - ex_nlog);
    fclose(f);
}

/* ------------------------------------------------------------ built-in snapshot ------------------------------------------------------------ */
typedef struct { const void *p; int n; const char *list[8]; } ExDef;
static ExDef ex_def[TXT_N];
static int ex_snapped;
static void ex_snapshot(void) {
    int i, k;
    if (ex_snapped) return;
    ex_snapped = 1;
    for (i = 0; i < TXT_N; i++) {
        const TxtReg *r = &TXT_REG[i]; ExDef *d = &ex_def[i];
        switch (r->kind) {
            case TK_STR: d->p = *(const char *const *)r->p; break;
            case TK_POOL: d->p = *(const char *const *const *)r->p; d->n = *r->np; break;
            case TK_LIST: for (k = 0; k < r->fixn && k < 8; k++) d->list[k] = ((const char *const *)r->p)[k]; break;
            case TK_BEATS: d->p = *(const Beat *const *)r->p; d->n = *r->np; break;
            case TK_LEAD: d->p = *(const Beat (*const *)[LEAD_BEATS])r->p; d->n = *r->np; break;
            case TK_TALK: d->p = ((const TalkPool *)r->p)->t; d->n = ((const TalkPool *)r->p)->n; break;
        }
    }
}
/* back to the built-in text (tests, and a clean start before loading) */
static PQ_UNUSED void ex_reset(void) {
    int i, k;
    ex_snapshot();
    for (i = 0; i < TXT_N; i++) {
        const TxtReg *r = &TXT_REG[i]; const ExDef *d = &ex_def[i];
        switch (r->kind) {
            case TK_STR: *(const char **)r->p = (const char *)d->p; break;
            case TK_POOL: *(const char *const **)r->p = (const char *const *)d->p; *r->np = d->n; break;
            case TK_LIST: for (k = 0; k < r->fixn && k < 8; k++) ((const char **)r->p)[k] = d->list[k]; break;
            case TK_BEATS: *(const Beat **)r->p = (const Beat *)d->p; *r->np = d->n; break;
            case TK_LEAD: *(const Beat (**)[LEAD_BEATS])r->p = (const Beat (*)[LEAD_BEATS])d->p; *r->np = d->n; break;
            case TK_TALK: ((TalkPool *)r->p)->t = (const Talk *)d->p; ((TalkPool *)r->p)->n = d->n; break;
        }
    }
}
static PQ_UNUSED int ex_find(const char *name, int file) {   /* index in TXT_REG (case-insensitive), -1 = unknown, -2 = it belongs in the other file */
    int i, other = 0;
    for (i = 0; i < TXT_N; i++) {
        const char *a = TXT_REG[i].name, *b = name;
        while (*a && *b && toupper((unsigned char)*a) == toupper((unsigned char)*b)) { a++; b++; }
        if (!*a && !*b) { if (TXT_REG[i].file == file) return i; other = 1; }
    }
    return other ? -2 : -1;
}

/* ----------------------------------------------------------------- checking text ----------------------------------------------------------------- */
static char ex_msg[160];
static int ex_specs(const char *s, char *out, int cap) {   /* the conversions of a format text ("dd" for two %d); -1 when it holds anything but %d */
    int n = 0;
    for (; *s; s++) if (*s == '%') { if (s[1] != 'd' || n + 1 >= cap) return -1; out[n++] = 'd'; s++; }
    out[n] = 0; return n;
}
static void ex_sample(char *dst, int cap, const char *s) {   /* the text with every %d as the biggest number (what the page will look like) */
    int n = 0;
    for (; *s && n + 8 < cap; s++) { if (*s == '%' && s[1] == 'd') { memcpy(dst + n, "999999", 6); n += 6; s++; } else dst[n++] = *s; }
    dst[n] = 0;
}
static int ex_lines_of(const char *s, int cols) { char w[900]; return wrap_buf(w, (int)sizeof w, s, cols); }
/* NULL = fine, else why the text is refused. 'def' = the built-in text (decides whether %d is allowed), may be NULL */
static const char *ex_text_err(int cls, const char *s, const char *def) {
    char sam[EX_MAXTEXT + 40], a[16], b[16]; const char *p; int n, nb = 0;
    if (!*s) return "empty text";
    if ((int)strlen(s) > EX_MAXTEXT) { snprintf(ex_msg, sizeof ex_msg, "too long (%d characters, the limit is %d)", (int)strlen(s), EX_MAXTEXT); return ex_msg; }
    for (p = s; *p; p++) {
        int c = toupper((unsigned char)*p);
        if (*p == '|') { if (cls == TC_TBL && ++nb == 1) continue; return cls == TC_TBL ? "more than one | (a table line has two lines at most)" : "the | sign is only for table lines"; }
        if (c < 32 || c > 90) { snprintf(ex_msg, sizeof ex_msg, "the character '%c' cannot be drawn by the game font (no accents, no [ ] { } ~ ^ _ ` or backslash)", (c >= 32 && c < 127) ? c : '?'); return ex_msg; }
    }
    n = ex_specs(s, a, (int)sizeof a);
    if (n < 0) return "a % may only be written as %d (the game puts a number there)";
    if (def) { int m = ex_specs(def, b, (int)sizeof b); if (m < 0) m = 0, b[0] = 0; if (strcmp(a, b)) { snprintf(ex_msg, sizeof ex_msg, "needs exactly %d x %%d (like the built-in text), has %d", m, n); return ex_msg; } }
    else if (n) return "% is not allowed in this text";
    ex_sample(sam, (int)sizeof sam, s);
    switch (cls) {
        case TC_TBL: { const char *bar = strchr(sam, '|'); int l1 = bar ? (int)(bar - sam) : (int)strlen(sam), l2 = bar ? (int)strlen(bar + 1) : 0;
            if (l1 > 18 || l2 > 18) { snprintf(ex_msg, sizeof ex_msg, "a table line is 2 lines of 18 characters at most (this has %d and %d)", l1, l2); return ex_msg; } break; }
        case TC_PAGE: n = ex_lines_of(sam, 46); if (n > 3) { snprintf(ex_msg, sizeof ex_msg, "does not fit the dialogue box: %d lines of 46 characters, the limit is 3", n); return ex_msg; } break;
        case TC_PAGE2: n = ex_lines_of(sam, 46); if (n > 2) { snprintf(ex_msg, sizeof ex_msg, "does not fit: %d lines of 46 characters, the limit here is 2", n); return ex_msg; } break;
        case TC_OBJ: n = ex_lines_of(sam, 32); if (n > 5) { snprintf(ex_msg, sizeof ex_msg, "does not fit the objective page: %d lines of 32 characters, the limit is 5", n); return ex_msg; } break;
        case TC_INFO: n = ex_lines_of(sam, 28); if (n > 6) { snprintf(ex_msg, sizeof ex_msg, "does not fit the info page: %d lines of 28 characters, the limit is 6", n); return ex_msg; } break;
        case TC_MONO: n = ex_lines_of(sam, 40); if (n > 12) { snprintf(ex_msg, sizeof ex_msg, "does not fit the monologue page: %d lines of 40 characters, the limit is 12", n); return ex_msg; } break;
        case TC_GEND: { char t2[EX_MAXTEXT + 90]; snprintf(t2, sizeof t2, "%s  (ROMANCE LEVEL UP!  PINK CARDS!)", sam); n = ex_lines_of(t2, 46);
            if (n > 3) { snprintf(ex_msg, sizeof ex_msg, "too long: the game adds a note in brackets after it, and the whole must fit 3 lines of 46 characters (this needs %d)", n); return ex_msg; } break; }
    }
    return NULL;
}
static char *ex_dup(const char *s) { size_t n = strlen(s) + 1; char *d = (char *)malloc(n); if (d) memcpy(d, s, n); return d; }

/* ------------------------------------------------------------------ reading a file ------------------------------------------------------------------ */
/* plain-text cleanup, in place: tabs, UTF-8 look-alikes (curly quotes, dashes, ellipsis, no-break space) become ASCII; any other non-ASCII byte becomes 0x01 (refused later) */
static void ex_ascii(char *s) {
    unsigned char *r = (unsigned char *)s, *w = (unsigned char *)s;
    while (*r) {
        if (*r < 0x80) { *w++ = (*r == '\t') ? ' ' : *r; r++; }
        else if (r[0] == 0xE2 && r[1] == 0x80 && (r[2] == 0x98 || r[2] == 0x99)) { *w++ = '\''; r += 3; }
        else if (r[0] == 0xE2 && r[1] == 0x80 && (r[2] == 0x9C || r[2] == 0x9D)) { *w++ = '"'; r += 3; }
        else if (r[0] == 0xE2 && r[1] == 0x80 && (r[2] == 0x93 || r[2] == 0x94)) { *w++ = '-'; r += 3; }
        else if (r[0] == 0xE2 && r[1] == 0x80 && r[2] == 0xA6) { *w++ = '.'; *w++ = '.'; *w++ = '.'; r += 3; }
        else if (r[0] == 0xC2 && r[1] == 0xA0) { *w++ = ' '; r += 2; }
        else { *w++ = 0x01; r++; while ((*r & 0xC0) == 0x80) r++; }   /* the whole multi-byte character counts once */
    }
    *w = 0;
}
static char *ex_trim(char *s) { char *e; while (*s == ' ') s++; e = s + strlen(s); while (e > s && e[-1] == ' ') *--e = 0; return s; }

/* the lines of one section: pointers into the file buffer + their line numbers */
typedef struct { char *s; int no; } ExLine;

static void ex_set_str(const TxtReg *r, const char *s) { *(const char **)r->p = s; }

/* ---- single text ---- */
static void ex_do_str(int ri, const ExLine *L, int n) {
    const TxtReg *r = &TXT_REG[ri]; const char *e;
    if (n != 1) { ex_err(n ? L[n > 1 ? 1 : 0].no : 0, r->name, "this section takes exactly one line (it has %d); the built-in text stays", n); return; }
    e = ex_text_err(r->cls, L[0].s, (const char *)ex_def[ri].p);
    if (e) { ex_err(L[0].no, r->name, "%s; the built-in text stays", e); return; }
    ex_set_str(r, ex_dup(L[0].s));
}
/* ---- list of exactly fixn texts ---- */
static void ex_do_list(int ri, const ExLine *L, int n) {
    const TxtReg *r = &TXT_REG[ri]; const char *tmp[8]; int i, bad = 0;
    if (n != r->fixn) { ex_err(n ? L[0].no : 0, r->name, "this list needs exactly %d lines (it has %d); the built-in texts stay", r->fixn, n); return; }
    for (i = 0; i < n; i++) { const char *e = ex_text_err(r->cls, L[i].s, NULL); if (e) { ex_err(L[i].no, r->name, "%s", e); bad = 1; } tmp[i] = L[i].s; }
    if (bad) { ex_err(0, r->name, "the built-in texts stay"); return; }
    for (i = 0; i < n; i++) ((const char **)r->p)[i] = ex_dup(tmp[i]);
}
/* ---- pool ---- */
static void ex_do_pool(int ri, const ExLine *L, int n) {
    const TxtReg *r = &TXT_REG[ri]; const char **a; int i, k = 0;
    if (n > EX_MAXPOOL) { ex_err(L[EX_MAXPOOL].no, r->name, "more than %d lines; the extra lines are ignored", EX_MAXPOOL); n = EX_MAXPOOL; }
    a = (const char **)malloc((size_t)(n ? n : 1) * sizeof(char *)); if (!a) return;
    for (i = 0; i < n; i++) { const char *e = ex_text_err(r->cls, L[i].s, NULL); if (e) ex_err(L[i].no, r->name, "%s; this line is skipped", e); else a[k++] = ex_dup(L[i].s); }
    if (!k) { free(a); ex_err(n ? L[0].no : 0, r->name, "no usable line; the built-in lines stay"); return; }
    *(const char *const **)r->p = a; *r->np = k;
}

/* ---- scenes ---- */
static int ex_word_is(const char *a, const char *b) { while (*a && *b && toupper((unsigned char)*a) == toupper((unsigned char)*b)) { a++; b++; } return !*a && !*b; }
/* one beat line "WHO [BG [SPR]] | text". prev = the beat before it (its background and sprite are inherited). out->txt = NULL for (auto). Returns 0 = fine, else an error text */
static const char *ex_beat(char *line, const Beat *prev, Beat *out, int cls) {
    char *bar = strchr(line, '|'), *tok[4]; int nt = 0, i; char *p, *txt; const char *e;
    if (!bar) return "a beat needs  WHO BACKGROUND SPRITE | text  (there is no | sign)";
    *bar = 0; txt = ex_trim(bar + 1);
    for (p = line; *p; ) { while (*p == ' ') p++; if (!*p) break; if (nt == 4) return "too many words before the | sign (WHO, BACKGROUND, SPRITE at most)"; tok[nt++] = p; while (*p && *p != ' ') p++; if (*p) *p++ = 0; }
    if (!nt) return "a beat needs a speaker: NAR, ME, UNK, AW, SOPHIA or HER";
    for (i = 0; i < 6; i++) if (ex_word_is(tok[0], EX_WHO[i])) break;
    if (i == 6) { snprintf(ex_msg, sizeof ex_msg, "unknown speaker '%s' (use NAR, ME, UNK, AW, SOPHIA or HER)", tok[0]); return ex_msg; }
    out->who = (unsigned char)i; out->bg = prev->bg; out->spr = prev->spr;
    if (nt >= 2) { for (i = 0; i < BG_N; i++) if (ex_word_is(tok[1], EX_BG[i])) break;
        if (i == BG_N) { snprintf(ex_msg, sizeof ex_msg, "unknown background '%s' (see TEXT_GUIDE.txt for the list)", tok[1]); return ex_msg; } out->bg = (unsigned char)i; }
    if (nt >= 3) { if (tok[2][0] < '0' || tok[2][0] > '5' || tok[2][1]) { snprintf(ex_msg, sizeof ex_msg, "sprite '%s' must be 0, 1, 2, 3, 4 or 5", tok[2]); return ex_msg; } out->spr = (unsigned char)(tok[2][0] - '0'); }
    if (nt == 4) return "too many words before the | sign (WHO, BACKGROUND, SPRITE at most)";
    if (ex_word_is(txt, "(auto)")) { out->txt = NULL; return NULL; }
    e = ex_text_err(cls, txt, NULL); if (e) return e;
    out->txt = ex_dup(txt); return NULL;
}
static void ex_do_beats(int ri, const ExLine *L, int n) {
    const TxtReg *r = &TXT_REG[ri]; const Beat *def = (const Beat *)ex_def[ri].p; int dn = ex_def[ri].n, i, k = 0, bad = 0; Beat *a; Beat first;
    if (n > EX_MAXBEATS) { ex_err(L[EX_MAXBEATS].no, r->name, "a scene can have %d beats at most; the built-in scene stays", EX_MAXBEATS); return; }
    if (!n) { ex_err(0, r->name, "no beat; the built-in scene stays"); return; }
    a = (Beat *)malloc((size_t)n * sizeof(Beat)); if (!a) return;
    first = def[0];
    for (i = 0; i < n; i++) {
        char tmp[EX_MAXTEXT + 80]; const char *e;
        snprintf(tmp, sizeof tmp, "%s", L[i].s);
        e = ex_beat(tmp, k ? &a[k - 1] : &first, &a[k], TC_PAGE);
        if (e) { ex_err(L[i].no, r->name, "%s", e); bad = 1; } else k++;
    }
    if (bad) { free(a); ex_err(0, r->name, "the built-in scene stays (a scene is only used when every beat is fine)"); return; }
    if (!r->tail && k != dn) { ex_err(L[0].no, r->name, "this scene needs exactly %d beats (the game fills in some of them by position); it has %d. The built-in scene stays", dn, k); free(a); return; }
    if (k < r->lock) { ex_err(L[0].no, r->name, "this scene needs at least %d beats (the first %d are fixed); it has %d. The built-in scene stays", r->lock, r->lock, k); free(a); return; }
    for (i = 0; i < k; i++) {
        int want_auto = i < r->lock && def[i].txt == NULL, is_auto = a[i].txt == NULL;
        if (want_auto != is_auto) {
            if (want_auto) ex_err(L[i].no, r->name, "beat %d must stay  (auto)  - the game fills in this line. The built-in scene stays", i + 1);
            else ex_err(L[i].no, r->name, "beat %d cannot be (auto) - only the beats the game fills in can. The built-in scene stays", i + 1);
            free(a); return;
        }
    }
    *(const Beat **)r->p = a; *r->np = k;
}
/* lead-ins: groups of exactly LEAD_BEATS beats, a blank line between groups (the section's blank lines arrive as empty entries, s == "") */
static void ex_do_lead(int ri, const ExLine *L, int n) {
    const TxtReg *r = &TXT_REG[ri]; const Beat *def = (const Beat *)ex_def[ri].p; Beat (*a)[LEAD_BEATS]; Beat grp[EX_MAXBEATS]; int i, ng = 0, nb = 0, gbad = 0, start = 0;
    a = (Beat (*)[LEAD_BEATS])malloc((size_t)(EX_MAXLEAD + 1) * sizeof(Beat) * LEAD_BEATS); if (!a) return;
    for (i = 0; i <= n; i++) {
        int blank = i == n || !L[i].s[0];
        if (!blank) {
            if (!nb) { start = L[i].no; gbad = 0; }
            if (nb < EX_MAXBEATS) {
                char tmp[EX_MAXTEXT + 80]; const char *e;
                snprintf(tmp, sizeof tmp, "%s", L[i].s);
                e = ex_beat(tmp, nb ? &grp[nb - 1] : &def[0], &grp[nb], TC_PAGE);
                if (e) { ex_err(L[i].no, r->name, "%s", e); gbad = 1; }
                else if (!grp[nb].txt) { ex_err(L[i].no, r->name, "(auto) cannot be used in a lead-in"); gbad = 1; }
            } else gbad = 1;
            nb++;
        } else if (nb) {
            if (!gbad && nb != LEAD_BEATS) { ex_err(start, r->name, "a lead-in needs exactly %d beats (this one has %d)", LEAD_BEATS, nb); gbad = 1; }
            if (gbad) ex_err(start, r->name, "the lead-in starting here is skipped");
            else if (ng >= EX_MAXLEAD) ex_err(start, r->name, "more than %d lead-ins; the rest are ignored", EX_MAXLEAD);
            else { memcpy(a[ng], grp, sizeof(Beat) * LEAD_BEATS); ng++; }
            nb = 0; gbad = 0;
        }
    }
    if (!ng) { free(a); ex_err(n ? L[0].no : 0, r->name, "no usable lead-in; the built-in lead-ins stay"); return; }
    *(const Beat (**)[LEAD_BEATS])r->p = (const Beat (*)[LEAD_BEATS])a; *r->np = ng;
}

/* ---- talk exchanges ---- */
static void ex_do_talk(int ri, const ExLine *L, int n) {
    const TxtReg *r = &TXT_REG[ri]; TalkPool *tp = (TalkPool *)r->p; Talk *a; int i, k = 0;
    const char *me = NULL, *ha = NULL, *hb = NULL; int meno = 0, bad = 0, open = 0; const char *e;
    a = (Talk *)malloc((size_t)(EX_MAXPOOL + 1) * sizeof(Talk)); if (!a) return;
    for (i = 0; i <= n; i++) {
        const char *s = i < n ? L[i].s : ""; char c = s[0];
        if (c == '>' || c == 0) {   /* an exchange ends: at a blank line, at the next >, or at the end */
            if (open) {
                if (!bad && !ha) { ex_err(meno, r->name, "the exchange has no answer (a line starting with <); it is skipped"); bad = 1; }
                if (!bad) { if (k >= EX_MAXPOOL) ex_err(meno, r->name, "more than %d exchanges; the rest are ignored", EX_MAXPOOL); else { a[k].me = me; a[k].a = ha; a[k].b = hb; k++; } }
                open = 0;
            }
            me = ha = hb = NULL; bad = 0;
            if (c == '>') {
                const char *t = ex_trim((char *)s + 1); open = 1; meno = L[i].no;
                e = ex_text_err(TC_PAGE, t, NULL); if (e) { ex_err(L[i].no, r->name, "%s; the exchange is skipped", e); bad = 1; } else me = ex_dup(t);
            }
        } else if (c == '<') {
            const char *t = ex_trim((char *)s + 1);
            if (!open) { ex_err(L[i].no, r->name, "an answer (<) without your line (>) before it; skipped"); continue; }
            if (bad) continue;
            if (ha && hb) { ex_err(L[i].no, r->name, "an exchange has one answer and at most one second line; the exchange is skipped"); bad = 1; continue; }
            e = ex_text_err(TC_PAGE, t, NULL); if (e) { ex_err(L[i].no, r->name, "%s; the exchange is skipped", e); bad = 1; continue; }
            if (!ha) ha = ex_dup(t); else hb = ex_dup(t);
        } else ex_err(L[i].no, r->name, "a line here must start with > (you) or < (her); skipped");
    }
    if (!k) { free(a); ex_err(n ? L[0].no : 0, r->name, "no usable exchange; the built-in exchanges stay"); return; }
    tp->t = a; tp->n = k;
}

static void ex_apply(int ri, const ExLine *L, int n) {
    switch (TXT_REG[ri].kind) {
        case TK_STR: ex_do_str(ri, L, n); break;
        case TK_LIST: ex_do_list(ri, L, n); break;
        case TK_POOL: ex_do_pool(ri, L, n); break;
        case TK_BEATS: ex_do_beats(ri, L, n); break;
        case TK_LEAD: ex_do_lead(ri, L, n); break;
        case TK_TALK: ex_do_talk(ri, L, n); break;
    }
}

/* ------------------------------------------------------------------ parsing a whole file ------------------------------------------------------------------ */
/* buf is modified. Returns the number of sections applied. */
static PQ_UNUSED int ex_parse(char *buf, int file) {
    ExLine *ln; int nl = 0, cap = 4096, i, applied = 0; char *p, *q;
    static char seen[TXT_N];
    ex_snapshot(); ex_cur_file = EX_FILE_NAME[file]; memset(seen, 0, sizeof seen);
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
            char name[64]; char *e = strchr(s, ']'); int ri, j, len, st;
            if (!e) { ex_err(ln[i].no, "", "a section header needs a closing ]; the lines below it are ignored"); i++; continue; }
            len = (int)(e - s) - 1; if (len > 62) len = 62; memcpy(name, s + 1, (size_t)len); name[len] = 0;
            { char *t = ex_trim(name); if (t != name) memmove(name, t, strlen(t) + 1); }
            ri = ex_find(name, file); st = i + 1;
            for (j = st; j < nl && ln[j].s[0] != '['; j++) {}
            if (ri == -1 && file == 0 && ex_word_is(name, "MILK_INFO")) { /* v9.5: moved to items.txt (SOPHIA_BREAST_MILK, DESC): an old lines.txt may still have it, ignored without a complaint */ }
            else if (ri == -1) ex_err(ln[i].no, name, "unknown section name (check the spelling); ignored");
            else if (ri == -2) ex_err(ln[i].no, name, "this section belongs in %s, not in %s; ignored", EX_FILE_NAME[1 - file], EX_FILE_NAME[file]);
            else if (seen[ri]) ex_err(ln[i].no, name, "the section appears twice; the second one is ignored");
            else {
                ExLine *sec; int ns = 0, k; const int keep_blank = TXT_REG[ri].kind == TK_LEAD || TXT_REG[ri].kind == TK_TALK;
                seen[ri] = 1; sec = (ExLine *)malloc((size_t)(j - st + 1) * sizeof(ExLine));
                if (sec) {
                    for (k = st; k < j; k++) {
                        if (ln[k].s[0] == '#') continue;
                        if (!ln[k].s[0] && !keep_blank) continue;
                        sec[ns++] = ln[k];
                    }
                    while (ns && !sec[ns - 1].s[0]) ns--;                 /* trailing blank lines mean nothing */
                    { int a = 0; while (a < ns && !sec[a].s[0]) a++; if (a) { memmove(sec, sec + a, (size_t)(ns - a) * sizeof(ExLine)); ns -= a; } }
                    ex_apply(ri, sec, ns); applied++; free(sec);
                }
            }
            i = j;
        } else {
            if (s[0] && s[0] != '#') ex_err(ln[i].no, "", "this line is outside any [SECTION]; ignored");
            i++;
        }
    }
    free(ln);
    return applied;
}
static int ex_rd_missing;   /* set by ex_read: the file does not exist (as opposed to: it exists but is unusable) */
static char *ex_read(const char *path) {
    FILE *f = fopen(path, "rb"); long n; char *b;
    ex_rd_missing = 0;
    if (!f) { ex_rd_missing = 1; return NULL; }
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0 || n > EX_MAXFILE) { fclose(f); ex_cur_file = path; ex_err(0, "", "the file is too big (limit %d KB); the built-in text is used", EX_MAXFILE / 1024); return NULL; }
    b = (char *)malloc((size_t)n + 1); if (!b) { fclose(f); return NULL; }
    if (fread(b, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(b); return NULL; }
    b[n] = 0; fclose(f); return b;
}

/* ------------------------------------------------------------------ dumping the current text ------------------------------------------------------------------ */
static const char *ex_limit_text(int cls) {
    switch (cls) {
        case TC_TBL: return "each line: two lines of 18 characters at most, split with |";
        case TC_PAGE: return "each text: one dialogue page, 3 lines of 46 characters at most";
        case TC_PAGE2: return "each text: 2 lines of 46 characters at most";
        case TC_OBJ: return "each text: 5 lines of 32 characters at most";
        case TC_INFO: return "each text: 6 lines of 28 characters at most";
        case TC_MONO: return "each text: 12 lines of 40 characters at most";
        default: return "each text: 3 lines of 46 characters at most, including the note the game adds";
    }
}
static void ex_dump_section(FILE *f, int ri) {
    const TxtReg *r = &TXT_REG[ri]; int i, k;
    fprintf(f, "\n# %s\n", r->desc);
    switch (r->kind) {
        case TK_STR: fprintf(f, "# one line; %s\n[%s]\n%s\n", ex_limit_text(r->cls), r->name, *(const char *const *)r->p); break;
        case TK_LIST: fprintf(f, "# EXACTLY %d lines; %s\n[%s]\n", r->fixn, ex_limit_text(r->cls), r->name); for (i = 0; i < r->fixn; i++) fprintf(f, "%s\n", ((const char *const *)r->p)[i]); break;
        case TK_POOL: { const char *const *a = *(const char *const *const *)r->p; int n = *r->np; fprintf(f, "# one line per entry, add or remove lines freely; %s\n[%s]\n", ex_limit_text(r->cls), r->name); for (i = 0; i < n; i++) fprintf(f, "%s\n", a[i]); break; }
        case TK_BEATS: { const Beat *b = *(const Beat *const *)r->p; int n = *r->np;
            fprintf(f, "# beats: WHO BACKGROUND SPRITE | text   (see TEXT_GUIDE.txt); %s\n", ex_limit_text(TC_PAGE));
            if (!r->tail) fprintf(f, "# FIXED: keep exactly %d beats; the lines marked (auto) are filled in by the game\n", n);
            else if (r->lock) fprintf(f, "# the first %d beats are fixed ((auto) = filled in by the game); beats after them can be added or removed\n", r->lock);
            fprintf(f, "[%s]\n", r->name);
            for (i = 0; i < n; i++) fprintf(f, "%-6s %-8s %d | %s\n", EX_WHO[b[i].who], EX_BG[b[i].bg], b[i].spr, b[i].txt ? b[i].txt : "(auto)");
            break; }
        case TK_LEAD: { const Beat (*b)[LEAD_BEATS] = *(const Beat (*const *)[LEAD_BEATS])r->p; int n = *r->np;
            fprintf(f, "# every lead-in has EXACTLY %d beats; a blank line separates lead-ins; add or remove whole lead-ins freely\n[%s]\n", LEAD_BEATS, r->name);
            for (i = 0; i < n; i++) { if (i) fprintf(f, "\n"); for (k = 0; k < LEAD_BEATS; k++) fprintf(f, "%-6s %-8s %d | %s\n", EX_WHO[b[i][k].who], EX_BG[b[i][k].bg], b[i][k].spr, b[i][k].txt); } break; }
        case TK_TALK: { const TalkPool *tp = (const TalkPool *)r->p; fprintf(f, "# > = what you say, < = her answer, second < = her second line (optional); a blank line between exchanges; %s\n[%s]\n", ex_limit_text(TC_PAGE), r->name);
            for (i = 0; i < tp->n; i++) { const Talk *x = &tp->t[i]; if (i) fprintf(f, "\n"); fprintf(f, "> %s\n< %s\n", x->me, x->a); if (x->b) fprintf(f, "< %s\n", x->b); } break; }
    }
}
static const char *const EX_HEAD0 =
    "# POKER MINI - lines.txt: story scenes, her table lines, walking messages, objectives.\n"
    "# Edit with Notepad (plain text). Save as plain text, keep the name lines.txt next to the game. The full guide is TEXT_GUIDE.txt.\n"
    "# '#' at the start of a line is a comment. [NAME] starts a section. Delete this file to get a fresh copy of the built-in text.\n"
    "# Anything the game refuses (too long, wrong characters, wrong shape) is skipped and written to text_log.txt; the built-in text is used instead.\n"
    "# Letters can be upper or lower case (the game shows capitals). Use plain characters only: A-Z 0-9 and . , ! ? ' \" - : ; ( ) $ & + = / * < > @ #\n";
static const char *const EX_HEAD1 =
    "# POKER MINI - talk.txt: the CHAT and TEASE exchanges in her room (and what she says around them).\n"
    "# Edit with Notepad (plain text). Keep the name talk.txt next to the game. The full guide is TEXT_GUIDE.txt.\n"
    "# An exchange:   > what you say      < her answer      < her second line (optional)      then a blank line.\n"
    "# You can add as many exchanges to a topic as you like. The topics themselves (poker, weather, ...) are fixed.\n"
    "# Anything the game refuses is skipped and written to text_log.txt; the built-in text is used instead. Delete this file to get a fresh copy.\n";
static PQ_UNUSED int ex_dump(const char *path, int file) {
    FILE *f = fopen(path, "w"); int i;
    if (!f) return 0;
    fputs(file ? EX_HEAD1 : EX_HEAD0, f);
    for (i = 0; i < TXT_N; i++) if (TXT_REG[i].file == file) ex_dump_section(f, i);
    fclose(f); return 1;
}

/* ------------------------------------------------------------------ the entry point ------------------------------------------------------------------ */
/* load a text from memory instead of a file (tests); problems are counted in ex_nprob as usual */
static PQ_UNUSED int ex_load_string(const char *text, int file) {
    char *b = ex_dup(text); int n;
    if (!b) return 0;
    n = ex_parse(b, file); free(b); return n;
}
/* Load lines.txt and talk.txt. A missing file is written from the built-in text when dump_missing is set. Returns the number of problems (0 = all fine). */
static PQ_UNUSED int ex_load_all(const char *lpath, const char *tpath, const char *logpath, int dump_missing) {
    const char *path[2]; int f; path[0] = lpath; path[1] = tpath;
    ex_snapshot(); ex_nlog = ex_nprob = 0;
    for (f = 0; f < 2; f++) {
        char *b = ex_read(path[f]);
        ex_cur_file = EX_FILE_NAME[f];
        if (!b) { if (dump_missing && ex_rd_missing) ex_dump(path[f], f); continue; }
        ex_parse(b, f); free(b);
    }
    ex_log_write(logpath);
    return ex_nprob;
}
#endif
