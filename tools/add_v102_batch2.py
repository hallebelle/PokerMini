#!/usr/bin/env python3
"""v10.2 batch 2 (one-off): the gift scene gets pools. For each of the 8 repeatable gifts (flower, money, lotion, pink cards, yoga, gold, milk pill, super drug)
the four beats (you hold it out / her thanks / her reaction / closing narration) are pools of 5 (the old text is entry 1). The milk pill thanks already is a pool (R_MILK_OK).
Edits src/lines.h, src/extreg.h, src/main.c and tests/test_logic.c. Run once on the v10.1 + batch 1 sources:  python3 tools/add_v102_batch2.py ."""
import re, sys
root = sys.argv[1] if len(sys.argv) > 1 else "."
def rd(p): return open(root + "/" + p).read()
LH, XR, MC, TL = rd("src/lines.h"), rd("src/extreg.h"), rd("src/main.c"), rd("tests/test_logic.c")
def cs(s): return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

KINDS = ["FLOWER", "STACK", "LOTION", "DECK", "YOGA", "GOLD", "MILK", "SUPER"]
KNAME = {"FLOWER": "flower", "STACK": "money stack", "LOTION": "body lotion", "DECK": "pink cards", "YOGA": "yoga outfit", "GOLD": "gold bar", "MILK": "milk inducer pill", "SUPER": "super milk drug"}
# beat -> kind -> [old text] + 4 new
G = {
"H": {   # you hold it out
 "FLOWER": ["I hold out the flowers to her.", "I step forward and offer her a small bunch of flowers.", "I take the flowers from behind my back and hold them out.", "I hold the flowers out, a little awkwardly, and wait.", "I offer her the flowers and try not to blush."],
 "STACK": ["I hold out a thick stack of cash to her.", "I put a thick bundle of cash in her hand.", "I hold out an envelope stuffed with money.", "I fan out a stack of cash and offer it to her.", "I slide a thick wad of cash across to her."],
 "LOTION": ["I hold out a bottle of body lotion to her.", "I hand her a bottle of body lotion.", "I hold out a small bottle of lotion for her.", "I offer her the lotion and look away politely.", "I set a bottle of body lotion in her hands."],
 "DECK": ["I hold out a deck of pink playing cards to her.", "I hand her a deck of pink playing cards.", "I hold out a fresh deck of pink cards, still sealed.", "I offer her the pink deck with a small bow.", "I set a neat deck of pink cards in her hand."],
 "YOGA": ["I hold out a neatly folded yoga outfit to her.", "I hold out a folded yoga outfit for her.", "I hand her a soft yoga outfit in a neat bag.", "I offer her the yoga outfit, trying to look casual.", "I place a yoga outfit in her arms."],
 "GOLD": ["I hold out a heavy gold bar to her.", "I hold out a gleaming gold bar.", "I place a heavy gold bar in her hands.", "I offer her a gold bar, still cold from my bag.", "I unwrap a gold bar and hold it out to her."],
 "MILK": ["I hold out another milk inducer pill to her.", "I hold out another milk inducer pill for her.", "I place one more milk inducer pill in her palm.", "I offer her another pill and a glass of water.", "I hand her the small pill without a word."],
 "SUPER": ["I hold out the super milk drug to her.", "I hold out the super milk drug.", "I hand her the small vial of super milk drug.", "I offer her the super drug, stronger than any pill.", "I place the super milk drug in her hand."],
},
"O": {   # her thanks (milk pill: R_MILK_OK already is a pool)
 "FLOWER": ["Flowers? ...Thank you.", "Flowers again? ...They smell nice. Thank you.", "Oh. Flowers. ...Where did you even find these?", "For me? ...Hmph. Fine. They're not ugly.", "Flowers? You do know I'm not that easy to impress."],
 "STACK": ["Money? Hmph. I'll take it.", "Cash? ...Hmph. At least it's real.", "Money, huh. You have no subtlety at all.", "Is this a bribe? ...Fine, I'll accept it.", "Hmph. Practical. I like practical."],
 "LOTION": ["Body lotion? ...It's the kind I like. Fine, I'll take it.", "Lotion? ...Mm. It smells lovely. Thank you.", "You remembered my skin gets dry. ...Hmph.", "Body lotion? That's... surprisingly thoughtful.", "Hmph. The expensive kind. You have good taste."],
 "DECK": ["Cards? Pink ones, too. Hmph. At least you have taste.", "A new deck? ...Pink. Of course it's pink.", "Cards? Sealed, too. ...Hmph. Acceptable.", "You bought me cards? ...I suppose that's fitting.", "Oh. A fresh deck. You have good instincts."],
 "YOGA": ["Yoga clothes? Are you saying something about my body?", "Yoga clothes? ...Why does it fit my size?", "A yoga outfit? ...Hmph. Don't look at me like that.", "Workout clothes? Are you saying I need to exercise?", "This is surprisingly my style. ...Don't gloat."],
 "GOLD": ["Gold? Real gold? Hmph. Now you have my attention.", "Gold? ...Is this real? Hmph. It is.", "A whole bar? ...You're either rich or insane.", "Gold, hm? You finally speak my language.", "That's heavy. ...Don't drop it. Give it here."],
 "SUPER": ["A stronger one? ...You really have no shame.", "This again? ...You really have no shame.", "The strong stuff? ...Hmph. Fine.", "Oh no. That one. ...Give it here.", "You're enabling me, you know that? ...Hand it over."],
},
"L": {   # her reaction
 "FLOWER": ["Don't get the wrong idea, boy. They're pretty, that's all.", "Don't get the wrong idea. I just don't like waste.", "Don't read into this. A queen deserves flowers, that's all.", "It means nothing. Don't smile like that.", "Don't get ideas. I'll put them in water, that's all."],
 "STACK": ["Don't get the wrong idea. I'm only taking it because I deserve it.", "Don't get ideas. Money is just money.", "Don't get the wrong idea. I'll find a good use for it.", "This doesn't buy you anything. ...Probably.", "Don't think you can buy me. ...Not that I'm refusing."],
 "LOTION": ["Don't get the wrong idea. My skin is only dry from the air in this hotel.", "Don't get the wrong idea. The hotel air really is dry.", "It's just practical. Don't read anything into it.", "Don't stare! It's for my skin, that's all.", "Don't get any ideas about helping me apply it."],
 "DECK": ["Don't get the wrong idea. A queen needs a proper deck, that's all.", "Don't get the wrong idea. My old deck was worn out.", "Don't think this makes us partners at the table.", "It's for practice. Don't smile at me like that.", "Don't get excited. A queen simply needs good tools."],
 "YOGA": ["Don't get the wrong idea. I only need something comfortable to stretch in.", "Don't get the wrong idea. Stretching is good for posture.", "Don't picture me in it. ...I mean it.", "It's just comfortable clothing. Nothing more.", "Don't get ideas. I might not even wear it."],
 "GOLD": ["Don't get the wrong idea. I just know a good deal when I see one.", "Don't get the wrong idea. I'm just appraising it.", "Don't think this impresses me. ...Much.", "It's a good investment, that's all.", "Don't get cocky. It's just shiny metal."],
 "MILK": ["Now leave me alone. I need some rest.", "Don't stare while I take it. Turn around.", "Don't look so hopeful. It's only medicine.", "Fine. But this is your responsibility.", "Don't comment. Not a single word."],
 "SUPER": ["Give it here. And don't you dare stare at me while I drink it.", "Don't watch me drink it. Face the wall.", "Don't stare! Turn around, now.", "This is embarrassing enough. Don't comment.", "Fine. But if this goes wrong, it's your fault."],
},
"E": {   # closing narration (the game adds the note in brackets)
 "FLOWER": ["She turns away, but I can tell she's pleased.", "She hides her face behind the petals and closes the door.", "She sniffs them quietly, then tells me to leave.", "She turns away, but I catch her smiling as the door shuts.", "She presses the flowers to her chest and shoos me away."],
 "STACK": ["She counts it without looking at me, then closes the door.", "She counts it twice, nods once, and closes the door.", "She tucks it away with a smug little smile and shuts the door.", "She riffles through the bills, then waves me out.", "She weighs the stack in her hand and quietly shuts the door."],
 "LOTION": ["She sniffs the bottle and hides a small smile before closing the door.", "She uncaps it, smells it and tries not to smile as she shuts the door.", "She hugs the bottle to herself and closes the door quickly.", "She turns away, but I hear her humming behind the door.", "She sniffs the bottle once more, then pushes the door shut."],
 "DECK": ["She fans the cards in her hand, then shuts the door.", "She shuffles the cards with a flick of her wrist and shuts the door.", "She breaks the seal, smiles faintly and closes the door.", "She cuts the deck with one hand and waves me away.", "She taps the box against her palm and shuts the door."],
 "YOGA": ["She holds it against herself, glances at me, then slams the door.", "She checks the fabric, glances at me, and shuts the door with a huff.", "She holds it up to herself, blushes, and closes the door fast.", "She mutters about my taste, but she takes it inside.", "She folds it carefully, as if it were precious, and shuts the door."],
 "GOLD": ["She weighs it in her hand and looks at me a moment too long before closing the door.", "She bites the corner like a pirate, grins, and shuts the door.", "She hugs it against herself for a moment, then closes the door.", "She runs her thumb over it and quietly shuts the door.", "She lifts it to the light, smirks at me, and slams the door."],
 "MILK": ["She swallows it, then shuts the door. Maybe the bottle gauge fills a little more.", "She swallows it with a grimace and shuts the door. The gauge should fill a bit more.", "She downs the pill, makes a face, and closes the door. Maybe the gauge moves.", "She takes it, flushes red, and shuts the door. The bottle gauge fills a little.", "She gulps it down and waves me off. Maybe the gauge fills more."],
 "SUPER": ["She drinks it and shuts the door. The gauge is full now. I can ask her for milk.", "She drinks it and shuts the door. The drug is working now. I can ask her for milk.", "She downs it, flushes red, and closes the door. I can ask her for milk now.", "She swallows it with a shudder and shuts the door. The gauge is full for now.", "She finishes it, shoves the empty vial back, and slams the door. I can ask for milk."],
},
}
BEAT_DESC = {"H": "you hold out the %s", "O": "her thanks for the %s", "L": "her reaction to the %s", "E": "closing narration for the %s (the game adds a note in brackets)"}
BEAT_PFX = {"H": "GH", "O": "GO", "L": "GL", "E": "GE"}

# ---- checks
ALLOWED = set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,!?'\"-:;()$&+=/*<>@#")
for b, d in G.items():
    for k, v in d.items():
        assert len(v) == 5, (b, k)
        for t in v:
            assert len(t) <= (100 if b == "E" else 128), (b, k, len(t), t)
            assert all(c in ALLOWED for c in t), (b, k, t)
            if k in ("MILK", "SUPER") or t is not v[0]:
                assert not re.search(r"\bboy\b|baby", t, re.I), (b, k, t)
# the first entry of every pool must be the old text (they stay in the old lists, which the pill / pump gifts still use)
for b, lst in (("H", "R_GIFT_HAND"), ("L", "R_GIFT_L2"), ("E", "R_GIFT_END")):
    for k in KINDS:
        assert cs(G[b][k][0]) in LH, (b, k, G[b][k][0])

# ---- lines.h: the pools, after R_GIFT_END
pools = ["/* v10.2: the gift scene: every repeatable gift has pools for its four beats (the first entry is the old text); the pill and the pump have their own scenes */"]
names = []
for b in "HOLE":
    for k in KINDS:
        if b == "O" and k == "MILK": continue   # R_MILK_OK is already a pool
        nm = "%s_%s" % (BEAT_PFX[b], k); names.append((nm, b, k))
        pools.append("POOL(%s, %s);" % (nm, ", ".join(cs(t) for t in G[b][k])))
m = re.search(r"static const char \*R_GIFT_END\[10\]", LH); assert m
e = LH.index("};\n", m.end()) + 3   # the list can span several lines
LH = LH[:e] + "\n".join(pools) + "\n" + LH[e:]

# ---- extreg.h: register them (after the R_GIFT_END entry)
reg = []
for nm, b, k in names:
    cat = "TC_GEND" if b == "E" else "TC_PAGE"
    reg.append('    {0, "%s", TK_POOL, %s, (void *)&%s, &%s_N, 0, 0, 0, "Gift scene, %s: %s (pool of 5)"},' % (nm, cat, nm, nm, KNAME[k], BEAT_DESC[b] % KNAME[k]))
m = re.search(r'    \{0, "R_GIFT_END"[^\n]*\n', XR); assert m
XR = XR[:m.end()] + "\n".join(reg) + "\n" + XR[m.end():]
XR = XR.replace("Gift scene: you hold out the gift: EXACTLY 10 (", "Gift scene (only the pill and the pump use this list now; the other gifts use the GH_ pools): you hold out the gift: EXACTLY 10 (", 1)
XR = XR.replace("Gift scene: her reaction: EXACTLY 10 (same order)", "Gift scene (only the pill and the pump use this list now; the others use the GL_ pools): her reaction: EXACTLY 10 (same order)", 1)
XR = XR.replace("Gift scene: closing narration, the game adds a note in brackets: EXACTLY 10 (same order)", "Gift scene (only the pill and the pump use this list now; the others use the GE_ pools): closing narration, the game adds a note in brackets: EXACTLY 10 (same order)", 1)
XR = XR.replace("Her thanks for flower / money / pill (unused) / body lotion", "Her thanks (the pill and pump do not use it; flower, money, lotion, cards, yoga, gold and super drug use the GO_ pools now) for flower / money / pill (unused) / body lotion", 1)

# ---- main.c
assert "PK_SLOTS = 192" in MC
MC = MC.replace("PK_SLOTS = 192", "PK_SLOTS = 512", 1)   # more pools remember their last picks now
# table + helpers right after pick()
m = re.search(r"static const char \*pick\(const char \*const \*pool, int n\) \{", MC); assert m
e = MC.index("\n}\n", m.end()) + 3
table = ["", "/* v10.2: the gift scene pools, by beat (0 hand, 1 thanks, 2 reaction, 3 closing) and gift kind (GF_*; the pill and the pump keep the old fixed lists, the milk pill thanks uses R_MILK_OK) */"]
kindcol = {"FLOWER": "GF_FLOWER", "STACK": "GF_STACK", "LOTION": "GF_LOTION", "DECK": "GF_DECK", "YOGA": "GF_YOGA", "GOLD": "GF_GOLD", "MILK": "GF_MILK", "SUPER": "GF_SUPER"}
table.append("#define GIFT_ROW(b) { b##_FLOWER_P, b##_STACK_P }")   # (placeholder, replaced below)
table = table[:2]
rows = []
for bi, b in enumerate("HOLE"):
    cells = []
    for k in KINDS:
        cells.append("[%s] = &%s_%s" % (kindcol[k], BEAT_PFX[b], k) if not (b == "O" and k == "MILK") else None)
    cells = [c for c in cells if c]
    rows.append("    {" + ", ".join(cells) + "}")
table.append("static const char *const *const *const GIFT_POOL[4][GF_N] = {\n" + ",\n".join(rows) + "\n};")
rows = []
for b in "HOLE":
    cells = ["[%s] = &%s_%s_N" % (kindcol[k], BEAT_PFX[b], k) for k in KINDS if not (b == "O" and k == "MILK")]
    rows.append("    {" + ", ".join(cells) + "}")
table.append("static int *const GIFT_POOL_N[4][GF_N] = {\n" + ",\n".join(rows) + "\n};")
table.append("""static const char *gift_pick(int beat, int kind) {   /* a line for one beat of the gift scene (NULL = this kind has no pool for that beat) */
    if (GIFT_POOL[beat][kind]) return pick(*GIFT_POOL[beat][kind], *GIFT_POOL_N[beat][kind]);
    return beat == 0 ? R_GIFT_HAND[kind] : beat == 2 ? R_GIFT_L2[kind] : beat == 3 ? R_GIFT_END[kind] : NULL;
}
static int gift_in_pool(int beat, int kind, const char *t) {   /* is t one of the pool lines (or, with a closing note after it, starts with one)? */
    const char *const *p; int n, i; if (!GIFT_POOL[beat][kind]) return 0;
    p = *GIFT_POOL[beat][kind]; n = *GIFT_POOL_N[beat][kind];
    for (i = 0; i < n; i++) if (beat == 3 ? !strncmp(t, p[i], strlen(p[i])) : t == p[i]) return 1;
    return 0;
}""")
MC = MC[:e] + "\n".join(table) + "\n" + MC[e:]

old = "case SCR_GIFT: vn_buf[0].txt = R_GIFT_HAND[gift_kind]; vn_buf[1].txt = gift_l1; vn_buf[2].txt = R_GIFT_L2[gift_kind];\n            snprintf(gift_end, sizeof gift_end, \"%s  (%s)\", R_GIFT_END[gift_kind], gift_note);"
new = "case SCR_GIFT: vn_buf[0].txt = gift_pick(0, gift_kind); vn_buf[1].txt = gift_l1; vn_buf[2].txt = gift_pick(2, gift_kind);   /* v10.2: pools */\n            snprintf(gift_end, sizeof gift_end, \"%s  (%s)\", gift_pick(3, gift_kind), gift_note);"
assert old in MC; MC = MC.replace(old, new, 1)
old = "gift_kind = kind; gift_l1 = R_GIFT_OK[kind == GF_STACK ? 1 : kind >= GF_LOTION ? 3 + kind - GF_LOTION : 0];"
new = "gift_kind = kind; gift_l1 = gift_pick(1, kind); if (!gift_l1) gift_l1 = R_GIFT_OK[kind == GF_STACK ? 1 : kind >= GF_LOTION ? 3 + kind - GF_LOTION : 0];   /* v10.2: pool, else the old list (pill, pump, milk pill: R_MILK_OK below) */"
assert old in MC; MC = MC.replace(old, new, 1)

# old tests that compared with one fixed string
old = "vn_buf[0].txt != R_GIFT_HAND[kind[k]] || vn_buf[1].txt != R_GIFT_OK[3 + kind[k] - GF_LOTION] || vn_buf[2].txt != R_GIFT_L2[kind[k]] || !strstr(vn_buf[3].txt, R_GIFT_END[kind[k]])"
new = "!gift_in_pool(0, kind[k], vn_buf[0].txt) || !gift_in_pool(1, kind[k], vn_buf[1].txt) || !gift_in_pool(2, kind[k], vn_buf[2].txt) || !gift_in_pool(3, kind[k], vn_buf[3].txt)"
assert old in MC; MC = MC.replace(old, new, 1)
old = "vn_buf[0].txt != R_GIFT_HAND[GF_SUPER] || vn_buf[1].txt != R_GIFT_OK[7] || vn_buf[2].txt != R_GIFT_L2[GF_SUPER]"
new = "!gift_in_pool(0, GF_SUPER, vn_buf[0].txt) || !gift_in_pool(1, GF_SUPER, vn_buf[1].txt) || !gift_in_pool(2, GF_SUPER, vn_buf[2].txt)"
assert old in MC; MC = MC.replace(old, new, 1)

# new test block
test = r'''
static int v102_tests(void) {   /* v10.2: the gift scene pools: five versions of every beat for the eight repeatable gifts, no immediate repeat, the right note, nothing from the old fixed text lost */
    int bad = 0, k, b, z, i; Game keep = G, keeps = saved_G; int kscr = scr, fr = frozen, krom = rom_in;
    static const int kinds[8] = {GF_FLOWER, GF_STACK, GF_LOTION, GF_DECK, GF_YOGA, GF_GOLD, GF_MILK, GF_SUPER};
    frozen = 1; pause_kind = PK_NONE; rom_in = 0;
    for (k = 0; k < 8; k++) {
        int kind = kinds[k]; static char seen[4][8][200]; int ns[4] = {0, 0, 0, 0}; const char *prev0 = NULL, *prev2 = NULL; int rep = 0;
        for (b = 0; b < 4; b++) {
            if (b == 1 && kind == GF_MILK) continue;
            if (!GIFT_POOL[b][kind] || *GIFT_POOL_N[b][kind] < 5) { bad++; printf("v102: gift %d beat %d: fewer than 5 versions\n", kind, b); continue; }
            { const char *const *p = *GIFT_POOL[b][kind]; int n = *GIFT_POOL_N[b][kind];
              const char *oldt = b == 0 ? R_GIFT_HAND[kind] : b == 2 ? R_GIFT_L2[kind] : b == 3 ? R_GIFT_END[kind] : (kind == GF_STACK ? R_GIFT_OK[1] : R_GIFT_OK[3 + kind - GF_LOTION < 8 && kind >= GF_LOTION ? 3 + kind - GF_LOTION : 0]);
              if (strcmp(p[0], oldt)) { bad++; printf("v102: gift %d beat %d: the old text must stay entry 1\n", kind, b); }
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
'''
m = re.search(r"static int v101_tests\(void\) \{", MC); assert m
MC = MC[:m.start()] + test.lstrip("\n") + "\n" + MC[m.start():]
MC = MC.replace("static int v101_tests(void);", "static int v101_tests(void); static int v102_tests(void);", 1)
m = re.search(r'    printf\("v10\.1: BREASTFEED[^\n]*v101_tests\(\)\);\n', MC); assert m
MC = MC[:m.end()] + '    printf("v10.2: gift scene pools (5 versions per beat for the 8 repeatable gifts): problems %d\\n", v102_tests());\n' + MC[m.end():]

# ---- test_logic.c: registry size + the new pools are registered and fit
newn = len(names)
old = "CHECK(TXT_N == 268, "
assert old in TL
TL = TL.replace(old, "CHECK(TXT_N == %d, " % (268 + newn), 1)
open(root + "/src/lines.h", "w").write(LH)
open(root + "/src/extreg.h", "w").write(XR)
open(root + "/src/main.c", "w").write(MC)
open(root + "/tests/test_logic.c", "w").write(TL)
print("ok:", newn, "pools,", newn * 5, "lines; registry", 268 + newn)
