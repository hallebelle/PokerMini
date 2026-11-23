#!/usr/bin/env python3
"""v10.4 (one-off): mama-mode openers. Your line (with 'Mama') first, her reaction to the name FIRST in her answer.
Run once on the v10.3 sources:  python3 tools/add_v104.py .   (already applied to the sources)"""
import sys
root = sys.argv[1] if len(sys.argv) > 1 else "."
def rd(p): return open(root + "/" + p).read()
def wr(p, s): open(root + "/" + p, "w").write(s)
def cs(s): return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

# ---- the opener pairs: (YOU line, SOPHIA reaction). The reaction to the name comes first. ----
FEED = [  # at her door and inside her bedroom (BREASTFEED, mama mode)
 ("Mama... may I? I'm so hungry.", "M-mama?! Don't call me that... I... are you sure about this? I'm not ready. Give me a moment."),
 ("Mama, please... can I have some?", "Don't call me that! ...Um... now? I... I'm not sure about this. Don't look at me yet."),
 ("Mama... I'm really hungry. May I?", "Don't... don't say that name! ...I... I don't know. Is it really alright? Wait."),
 ("Mama... can I rest against you? Just for a while?", "W-what?! Don't call me that! ...Rest? You mean... um... I'm not ready. Wait a moment."),
 ("Mama... it's just us. May I have some now?", "Stop calling me that! ...I... maybe... no, I don't know. It does ache, but... ugh. Wait."),
 ("Mama, I'll be gentle. May I?", "Mama?! I'm not... ...Ugh. Gentle? Promise? I'm really not sure about this. Don't look."),
]
FEED_LR = [  # living room (BREASTFEED, mama mode): the 'Here?!' is part of her reaction
 ("Mama... may I? My stomach feels so empty.", "M-mama?! Don't call me that... I... here? Are you sure? I don't know if I can... Give me a moment."),
 ("Mama, please... can I have some?", "Don't call me that! ...Um... right now? Out here? I'm not sure about this. What if someone comes in?"),
 ("Mama... I'm really hungry. May I?", "Don't... don't say that name! ...I... I don't know. Is it really alright? Out here?"),
 ("Mama... can I rest against you? Just for a while?", "W-what?! Don't call me that! ...Rest? Here? You mean... um... I... I'm not ready. Wait."),
 ("Mama... it's just us. May I have some now?", "Stop calling me that! ...Just us? Here? ...I... maybe... no, I don't know. My chest does ache, but..."),
 ("Mama, I'll be quick. May I?", "Mama?! I'm not... ...Ugh. Quick? Here? Promise? I'm really not sure about this. Don't look."),
]
ASK_LR = [  # living room: ASK FOR MILK / PUMP MILK (a lead-in and the pump scene follow)
 ("Mama... could you give me some milk? Please?", "Don't call me that! ...M-milk? Here? Now? I... fine, I suppose it does ache. Turn around, then."),
 ("Mama, is it full yet? I'd like a bottle.", "Stop calling me that! ...A bottle? Here? Hmph. Fine. It's heavy anyway. Don't look at me."),
 ("Mama... I'd love some of your milk today.", "Don't say it like that! ...Here?! Hmph... my chest does feel tight. I'll get the pump. Face away."),
 ("Mama, I'm thirsty. Can you pump some for me?", "Mama?! No! Don't call me that... Pump? Here? ...Fine. Don't peek this time."),
 ("Mama... my bottle is empty. Could you fill it?", "Don't call me that! ...F-fill it? Out here? I... fine. It does feel heavy. Turn around."),
 ("Mama, could you pump some for me today?", "Mama?! Stop that! ...Pump? Here? ...Hmph. I suppose. Just don't turn around."),
]
PILL_LR = [  # living room: the milk pill (also when the pill fills the gauge and the pump scene follows)
 ("Mama, here's your pill. Take it for me?", "Don't call me that! ...Give it here. And don't watch me swallow it."),
 ("Mama... time for your pill. Open up.", "Stop calling me that! ...Hmph. I can take a pill by myself. Turn around."),
 ("Mama, please take this one. For both of us.", "M-mama?! Don't... fine. Hand it over. And not a word about it."),
 ("Mama, your pill. I'll hold the water for you.", "Don't call me that! ...I can hold my own glass. Give me the pill and turn around."),
]
SUPER_LR = [  # living room: the Super drug
 ("Mama, I got the strong one. Please drink it.", "Don't call me that! ...That one is strong. Fine. Give it here. Face the wall."),
 ("Mama... this is the super drug. Are you ready?", "Don't say that name! ...Ready? ...No. Just give it to me. Don't stare."),
 ("Mama, drink this and you'll feel so full.", "Stop it with that name! ...Hmph. Give me the vial. And don't comment."),
 ("Mama... it's the strong one again. Ready?", "Stop calling me that! ...Ugh. Ready or not. Hand it over and face the wall."),
]
SETS = [
 ("B_OPEN_FEED_MM", FEED, "Mama mode BREASTFEED at her door and in her bedroom: the OPENER (replaces beats 1-2 of B_FEED_MM). Pairs of beats: your line (ME) with Mama in it, then her reaction (SOPHIA), which starts with her reaction to the name. Add or remove whole pairs (ME line + SOPHIA line)"),
 ("B_OPEN_FEED_MM_LR", FEED_LR, "Mama mode BREASTFEED in the living room: the OPENER (replaces beats 1-2 of B_FEED_MM_LR). Pairs of beats: your line (ME), then her reaction (SOPHIA) that starts with the name and has her Here?! in it. Add or remove whole pairs (ME line + SOPHIA line)"),
 ("B_OPEN_ASK_MM_LR", ASK_LR, "Mama mode, living room, ASK FOR MILK / PUMP MILK: the OPENER before the lead-in. Pairs of beats: your line (ME), then her reaction (SOPHIA) that starts with the name and has her Here?! in it. Add or remove whole pairs (ME line + SOPHIA line)"),
 ("B_OPEN_PILL_MM_LR", PILL_LR, "Mama mode, living room, MILK PILL: the OPENER before the gift scene. Pairs of beats: your line (ME), then her reaction (SOPHIA) that starts with the name. Add or remove whole pairs (ME line + SOPHIA line)"),
 ("B_OPEN_SUPER_MM_LR", SUPER_LR, "Mama mode, living room, SUPER DRUG: the OPENER before the gift scene. Pairs of beats: your line (ME), then her reaction (SOPHIA) that starts with the name. Add or remove whole pairs (ME line + SOPHIA line)"),
]
for _, pairs, _d in SETS:
    for a, b in pairs:
        assert len(a) <= 130 and len(b) <= 130, (a, b)
        assert "baby" not in (a + b).lower() and "good boy" not in (a + b).lower()
        assert b.lower().startswith(("m-mama", "don't call me that", "don't... don't say that name", "w-what?! don't call me that", "stop calling me that", "mama?!", "don't say it like that", "stop it with that name", "don't say that name")), b

# ---- lines.h ----
L = rd("src/lines.h")
block = "/* ---------- v10.4: mama-mode OPENERS. Each opener is a PAIR of beats: your line (it has Mama in it), then her reaction, which starts with her reaction to the name. ----------\n   One pair is picked at random (never the same twice in a row). Living room: ASK FOR MILK / PUMP MILK / MILK PILL / SUPER DRUG / BREASTFEED. Door and bedroom: BREASTFEED. */\n"
for name, pairs, _d in SETS:
    block += "SCENE(%s,\n" % name
    rows = []
    for a, b in pairs:
        rows.append("    {W_ME, BG_ROM, 1, %s}" % cs(a)); rows.append("    {W_SOPHIA, BG_ROM, 1, %s}" % cs(b))
    block += ",\n".join(rows) + "\n);\n"
marker = "POOL(R_SLEEP_NO_TEASE_MM,"
assert marker in L
L = L.replace(marker, block + marker, 1)
# the two BREASTFEED scenes: beats 1-2 are the fallback now; the third line (hers) is hesitant too
old1 = '{W_SOPHIA, BG_FEED, 0, "...Just this once. And you don\'t say a word. Not one. Understood?"}'
new1 = '{W_SOPHIA, BG_FEED, 0, "...Just... just this once. And you don\'t say a word. Okay? ...I\'m still not sure about this."}'
assert old1 in L; L = L.replace(old1, new1, 1)
old2 = '{W_SOPHIA, BG_FEED, 0, "...Just this once. Not a word. Not one. And if anyone knocks, we stop. Understood?"}'
new2 = '{W_SOPHIA, BG_FEED, 0, "...Just... just this once. And if anyone knocks, we stop. Okay? ...I\'m still not sure about this."}'
assert old2 in L; L = L.replace(old2, new2, 1)
wr("src/lines.h", L)

# ---- extreg.h ----
R = rd("src/extreg.h")
anchor = '    {0, "B_FEED_MM", TK_BEATS'
assert anchor in R
ent = "".join('    {0, "%s", TK_BEATS, TC_PAGE, (void *)&%s, &%s_N, %d, 0, 1, %s},\n' % (n, n, n, len(p) * 2, cs(d)) for n, p, d in SETS)
R = R.replace(anchor, ent + anchor, 1)
R = R.replace("Beats 1-2: she covers her chest (sprite 1); then the picture sc_feed_d8 / sc_feed_yoga (BG FEED)\"},", "Beats 1-2 are REPLACED by a random opener from B_OPEN_FEED_MM (these two stay as the fallback); then the picture sc_feed_d8 / sc_feed_yoga (BG FEED)\"},", 1)
R = R.replace("Living room, mama mode: BREASTFEED. Beats 1-2 she covers her chest (sprite 1); then the picture", "Living room, mama mode: BREASTFEED. Beats 1-2 are REPLACED by a random opener from B_OPEN_FEED_MM_LR (these two stay as the fallback); then the picture", 1)
wr("src/extreg.h", R)
print("lines.h / extreg.h done")
