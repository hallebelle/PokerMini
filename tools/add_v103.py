#!/usr/bin/env python3
"""v10.3 (one-off): mama-mode table pools (14) grown to 20+ lines, and NEW close-up pools CU_THINK_MM / L_STARE_MM (22+ lines each).
Run once on the v10.2b sources:  python3 tools/add_v103.py .   (already applied to the sources)"""
import re, sys
root = sys.argv[1] if len(sys.argv) > 1 else "."
LINES = open(root + "/src/lines.h").read()
REG = open(root + "/src/extreg.h").read()

def cs(s): return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

def pool_end(src, name):
    m = re.search(r"POOL\(%s,\s" % name, src); assert m, name
    return src.index(");", m.end())

def add_pool(src, name, items):
    e = pool_end(src, name)
    return src[:e] + ",\n" + ",\n".join("    " + cs(x) for x in items) + src[e:]

T = {}
T["L_START_MM"] = ["SIT. DEAL.|NO NICKNAMES.","CARDS UP,|HUSBAND.","I'LL GO EASY.|OR NOT.","BRING YOUR|BEST HAND.","LET'S BEGIN.|MIND YOUR TONGUE.","GOOD LUCK.|YOU'LL NEED IT.","TRY NOT TO|EMBARRASS US.","SHUFFLE UP.|SOPHIA IS READY.","FIRST HAND.|BEHAVE.","WELCOME TO MY|TABLE, HUSBAND.","NO PET NAMES.|JUST POKER.","ANTE UP.|AND STAY QUIET.","I'M WARMED UP.|ARE YOU?","SMILE ALL YOU|WANT. I'M WINNING.","PLAY NICE.|OR PAY FOR IT.","DON'T STARE.|PLAY.","SAY MY NAME,|THEN DEAL."]
T["L_HAND_MM"] = ["NEW HAND.|STAY FOCUSED.","DEAL, HUSBAND.|I'M WAITING.","CARDS, NOT|COMPLIMENTS.","MY TURN TO|BEAT YOU.","PAY ATTENTION.|NOT TO ME.","ANOTHER ONE.|KEEP UP.","WELL? DON'T|JUST SIT THERE.","EYES UP HERE.|ON THE CARDS.","HURRY UP.|I DON'T BITE.","DON'T LOOK SO|NERVOUS.","STOP SMIRKING|AND PLAY.","LET'S SEE YOUR|BEST TRY.","NO WHISPERING|THAT WORD.","COME ON.|MAKE A MOVE.","BET WISELY,|HUSBAND.","I'M WATCHING|YOUR FACE.","THINK FASTER.|I'M BORED."]
T["L_LAST_MM"] = ["LAST HAND.|BE A MAN.","ONE MORE HAND.|NO SULKING.","FINAL HAND.|DO YOUR BEST.","LAST ONE.|TRY NOT TO CRY.","LAST HAND.|SAY SOPHIA.","THE END IS|NEAR, HUSBAND.","FINAL ROUND.|DON'T FLINCH.","ONE LEFT. WIN|OR APOLOGIZE.","LAST HAND.|NO MERCY.","LAST ONE. DON'T|WASTE IT.","LAST HAND. EYES|ON THE CARDS.","ALMOST DONE.|BE BRAVE.","LAST HAND.|STOP GRINNING.","ONE FINAL TRY,|HUSBAND.","FINISH IT.|IF YOU CAN.","THE LAST HAND.|MAKE ME PROUD.","LAST CHANCE.|IMPRESS ME."]
T["L_HERRAISE_MM"] = ["I RAISE. CALL|IF YOU DARE.","UP IT GOES.|SWEAT, HUSBAND.","RAISE. KEEP|YOUR EYES UP.","MORE CHIPS.|MY RULES.","I RAISE. YOUR|FACE SAYS FOLD.","BIGGER. WILL YOU|FOLLOW, HUSBAND?","RAISING. CRY|LATER.","PRICE GOES UP.|PAY OR RUN.","I RAISE.|SCARED YET?","RAISE. AND NO|NAMES, PLEASE.","MORE, PLEASE.|OR GO HOME.","I'M RAISING.|STOP BLUSHING.","RAISE. DON'T|MAKE A FACE.","THAT'S A RAISE.|THINK HARD.","UP A LITTLE.|FOLLOW IF BOLD.","A RAISE. DON'T|CALL ME THAT."]
T["L_YOURAISE_MM"] = ["A RAISE? FROM|YOU? CUTE.","OH, YOU'RE|SERIOUS NOW.","FEELING LUCKY,|HUSBAND?","HMPH. SHOW ME|WHAT YOU HAVE.","BIG TALK. BIG|CHIPS. FINE.","YOU RAISE? I'M|NOT AFRAID.","BOLD. IT WON'T|SAVE YOU.","TRYING TO|IMPRESS ME?","RAISING ME?|HOW DARING.","INTERESTING.|I'LL THINK.","SHOWING OFF,|HUSBAND?","SO CONFIDENT.|IT'S ADORABLE.","THAT'S A BIG|BET. SWEATING?","A RAISE. I'M|NOT IMPRESSED.","MORE CHIPS?|FOOLISH MAN.","OH? NOW YOU|WANT TO PLAY.","DARING. I DO|LIKE DARING."]
T["L_SHEWIN_MM"] = ["MINE.|DON'T SULK.","POT'S MINE.|COUNT YOUR LOSS.","EASY. HAVE A|TISSUE, HUSBAND.","THANK YOU FOR|THE CHIPS.","WINNER.|SAY MY NAME.","I TOLD YOU|NOT TO STARE.","AND THAT'S|HOW IT'S DONE.","MY CHIPS NOW.|DON'T POUT.","WON. AGAIN.|HOW SAD FOR YOU.","SORRY. NOT|SORRY AT ALL.","TOO SLOW,|HUSBAND.","DID YOU THINK|I'D LOSE?","POT TO ME.|NO NICKNAMES.","CLEANED OUT.|WHAT A SHAME.","HA. SOPHIA|WINS AGAIN.","KEEP DREAMING.|THIS ONE'S MINE."]
T["L_YOUWIN_MM"] = ["HMPH. LUCKY.|DON'T GRIN.","YOU WON?!|DON'T GET USED.","WIPE THAT|SMILE OFF.","FINE. YOURS.|STOP BOUNCING.","I ALLOWED THAT.|REALLY.","LUCKY CARDS,|NOT LUCKY YOU.","ENJOY IT.|IT WON'T LAST.","WELL PLAYED.|NOW BEHAVE.","ONE POT. DON'T|BRAG, HUSBAND.","GRR. TAKE IT.","DON'T YOU DARE|GLOAT AT ME.","SMUG LOOKS BAD|ON YOU.","OKAY. YOU WON.|CALM DOWN.","A FLUKE.|A LOVELY FLUKE.","I WAS DISTRACTED.|THAT'S ALL.","HMPH. DON'T|EXPECT A PRIZE."]
T["L_SHEFOLD_MM"] = ["I FOLD. ENJOY|THE SCRAPS.","TAKE IT.|DON'T GET SMUG.","NOT MY HAND.|NEXT TIME.","FINE. YOU|WIN THAT ONE.","I'M OUT. BE|PROUD, HUSBAND.","FOLDING. IT'S|STRATEGY.","GO ON. TAKE IT.|STOP LOOKING.","MY CARDS STINK.|TAKE IT.","I'LL LET THAT|ONE GO.","ENOUGH. YOURS.|DON'T BRAG.","I FOLD. THE|NEXT ONE'S MINE.","DON'T CELEBRATE.|IT'S ONE POT.","NOT WORTH MY|TIME. TAKE IT.","HMPH. FINE.|GRAB YOUR CHIPS.","TAKE IT.|I'M BEING KIND.","YOU WIN THIS.|DON'T SMIRK.","I FOLD. SAVING|MY CHIPS."]
T["L_YOUFOLD_MM"] = ["FOLDING?|SO SOON?","RUNNING AWAY|AGAIN, HUSBAND?","A FOLD. HOW|VERY BRAVE OF YOU.","THOUGHT SO.|COWARD.","YOU FOLD? I|WASN'T TRYING.","SPINELESS.|BUT PRACTICAL.","SCARED OF ME,|HUSBAND?","SMART. MY HAND|WAS A BEAUTY.","THE POT'S MINE.|THANK YOU.","GIVING UP SO|EASILY?","HMPH. I DIDN'T|EVEN RAISE.","NO GUTS. NO|CHIPS.","YOU FOLD. I|BARELY TRIED.","DON'T RUN. FACE|ME, HUSBAND.","WISE. I WAS|HOLDING GOLD.","FOLD? OH, COME|ON. FIGHT.","HAH. RUN, THEN."]
T["L_SPLIT_MM"] = ["SPLIT. HOW|UNSATISFYING.","A TIE. DON'T|LOOK SO PLEASED.","EQUAL. FOR NOW.","HALF EACH.|I WANTED ALL.","SPLIT POT.|HOW DULL.","A DRAW. WE'LL|SETTLE THAT.","SAME HAND.|STOP COPYING.","A TIE. I HATE|TIES, HUSBAND.","HALF IS NOT|ENOUGH FOR ME.","EVEN AGAIN.|ANNOYING.","SPLIT. DON'T|CALL IT FATE.","EVEN? ONE MORE|HAND, THEN.","A DRAW.|I'LL WIN LATER.","SPLIT. YOU GOT|LUCKY AGAIN.","HALF AND HALF.|MEH.","IDENTICAL. HOW|SUSPICIOUS.","A TIE?|YOU'RE IRRITATING."]
T["L_ENDHER_MM"] = ["I WIN. NOW|SAY MY NAME.","MATCH OVER.|PAY UP.","WON. AS ALWAYS.|DON'T POUT.","GAME. SET.|SOPHIA WINS.","THANKS FOR|THE GAME.","BETTER LUCK|NEXT TIME.","YOU TRIED.|THAT'S ENOUGH.","I WIN. COME,|DON'T BE SAD.","ALL MINE.|BEHAVE NEXT TIME.","CHIN UP,|HUSBAND.","ANOTHER WIN.|NO NICKNAMES.","DONE. YOU'RE|OUT OF CHIPS.","I WAS BETTER.|ADMIT IT.","PLAY AGAIN?|I'LL WIN AGAIN.","THE QUEEN WINS.|AGAIN.","WELL, THAT'S|THAT. SULK AWAY.","NOT EVEN CLOSE.|NEXT TIME."]
T["L_ENDYOU_MM"] = ["YOU WIN. DON'T|DARE GLOAT.","CONGRATS.|NOW STOP SMILING.","I LOST. DON'T|SAY IT TWICE.","HMPH. YOU WON.|ONCE.","FINE. YOU WIN.|NO NAMES, THOUGH.","WELL DONE.|DON'T BE SMUG.","YOU BEAT ME.|DON'T ENJOY IT.","I LOST FAIRLY.|HMPH.","I'LL WIN IT|BACK, HUSBAND.","YOU WON. NOW|LEAVE ME BE.","LUCK. PURE|LUCK.","CONGRATS, I|SUPPOSE. HMPH.","ENJOY YOUR WIN.|BRIEFLY.","THIS ONCE. ONLY|THIS ONCE.","I'M PROUD. AND|ANNOYED.","WINNER. NOW BE|HUMBLE ABOUT IT.","FINE. YOU WON.|BUY ME TEA."]
T["L_ENDTIE_MM"] = ["A TIE.|HOW UNFAIR.","NO WINNER.|NO PRIDE.","EVEN. I'LL|WIN THE NEXT.","A DRAW. WE'LL|PLAY AGAIN.","EQUAL AT LAST.|DON'T GLOAT.","TIED. I HATE|IT, HUSBAND.","NOBODY WON.|ANNOYING.","A TIE?|WHAT A WASTE.","DEAD EVEN.|REMATCH. NOW.","STALEMATE.|I DISLIKE THAT.","NO WINNER.|NO NICKNAMES.","TIED. TRY HARDER|NEXT TIME.","EVEN. HOW|IRRITATING.","A TIE. DON'T|LOOK RELIEVED.","EQUAL. FOR NOW.|DON'T BLINK.","TIE GAME.|REMATCH SOON.","A TIE. WHAT|A BORING END.","NO LOSER.|NO WINNER."]
T["L_ENDLOST_MM"] = ["A LOSS, BUT|NOT A SHAME.","YOU LOST. BUT|THE BONUSES...","LOST WITH STYLE.|HMPH.","BONUSES? SWEET.|STILL LOST.","NICE CARDS.|STILL A LOSS.","YOU PLAYED WELL.|AND LOST.","CUTE BONUSES.|WRONG RESULT.","LOST BEAUTIFULLY,|HUSBAND.","THE CARDS LIKED|YOU. I DIDN'T.","YOU LOST. BUT|I'M IMPRESSED.","WELL FOUGHT.|STILL LOST.","SOME SKILL. NO|LUCK. HMPH.","GOOD HANDS.|BAD ENDING.","YOU LOST. EAT|YOUR BONUSES.","ALMOST HAD ME.|ALMOST.","A LOSS, YET|YOU SMILE. ODD.","NOT BAD AT ALL.|STILL LOST."]

for k, v in T.items():
    for x in v:
        parts = x.split("|"); assert len(parts) <= 2 and all(len(p) <= 18 for p in parts), (k, x)
    LINES = add_pool(LINES, k, v)

CU_THINK_MM = [
 "My wife's chest is right there. How am I meant to read my cards?",
 "She says don't call her mama. My head calls her that anyway.",
 "That shirt is working so hard. I feel sorry for the buttons.",
 "I could look at her all night. The cards can wait.",
 "So soft and so near. And she thinks I'm watching the pot.",
 "Mama. ...Don't say it aloud. Just think it. Just once.",
 "She's glaring at me, and I still can't look away.",
 "Those curves in the lamp light. I'm a lucky man.",
 "If I lose this hand, at least I lost looking at that.",
 "How can a woman be this scary and this gorgeous?",
 "Every breath she takes, the whole shirt moves. Unfair.",
 "She'll scold me for this. It's worth it for one more look.",
 "She's so full lately. I know why, and I'm grateful.",
 "She's pretending not to notice. She always notices.",
 "Her chest rises and falls, and my poker face goes with it.",
 "Everyone fears the Queen. I just want to rest against her.",
 "I'm holding a pair and staring at a perfect pair.",
 "Maybe I'll fold. Then I can look at her properly.",
 "She's going to scold me. I'm already smiling about it.",
 "My wife, my Queen, my... no. Don't say mama. Think it.",
 "That warmth, that softness. I'd trade the pot for a hug.",
 "She leans forward to bet and I forget my own cards."]
L_STARE_MM = [
 "Eyes up, husband. My face is up here.",
 "I SAW that. Look at my chest again and you sleep on the sofa.",
 "Stop staring! I'm your wife, not a dessert menu.",
 "Pay attention to your cards, not my shirt. Honestly!",
 "Do you think I can't see where you're looking? Foolish husband.",
 "Hmph. Staring at your wife in the middle of a match. Shameful.",
 "One more look and you pay double. Eyes. On. The. Table.",
 "I have a face, you know. Try looking at it for once.",
 "That silly grin again. Wipe it off and play.",
 "Is my chest more interesting than your hand? Pathetic.",
 "Stop it! And whatever you were thinking, don't say it. Not that word.",
 "You're a married man and you stare like a teenager. Shame on you.",
 "I'll cover up if I must. Keep your eyes on the cards!",
 "Not another peek. The next one costs you your chips.",
 "I know that look. It's the look of a man about to lose.",
 "Hmph. Looking is free. Losing isn't. Eyes on the game.",
 "Are you listening? Hands on the cards, eyes on the cards.",
 "My shirt is not a poker hand. Stop reading it.",
 "Do that again and I'll turn my chair around. See how you play then.",
 "Behave. This is a card game, not a show. Pay attention.",
 "Tsk. Staring at your wife like a hungry dog. Get a grip.",
 "If you're done drooling, it's your turn. Play.",
 "And don't you dare whisper that word. My name is Sophia."]

def wrap_rows(s, w):
    rows, cur = 1, 0
    for word in s.split(" "):
        n = len(word)
        if cur == 0: cur = n
        elif cur + 1 + n <= w: cur += 1 + n
        else: rows += 1; cur = n
    return rows
for s in CU_THINK_MM: assert wrap_rows(s, 46) <= 2, s
for s in L_STARE_MM: assert wrap_rows(s, 46) <= 3, s
for s in CU_THINK_MM + L_STARE_MM: assert not re.search(r"\b(boy|baby)", s, re.I), s

def pool_def(name, items):
    return "POOL(%s,\n%s);\n" % (name, ",\n".join("    " + cs(x) for x in items))
anchor = LINES.index('static const char *SN_DONE PQ_UNUSED')
LINES = LINES[:anchor] + "/* v10.3: close-up in mama mode (free match at home only): what you think / what she says, scolding */\n" + pool_def("CU_THINK_MM", CU_THINK_MM) + pool_def("L_STARE_MM", L_STARE_MM) + LINES[anchor:]

m = re.search(r'    \{0, "L_STARE", .*\n', REG); assert m
REG = REG[:m.end()] + '    {0, "CU_THINK_MM", TK_POOL, TC_PAGE2, (void *)&CU_THINK_MM, &CU_THINK_MM_N, 0, 0, 0, "Mama mode, R1 close-up (free match at home only): what you think, MAX 2 LINES"},\n    {0, "L_STARE_MM", TK_POOL, TC_PAGE, (void *)&L_STARE_MM, &L_STARE_MM_N, 0, 0, 0, "Mama mode, R1 close-up (free match at home only): her scolding answer, max 3 lines"},\n' + REG[m.end():]
open(root + "/src/lines.h", "w").write(LINES); open(root + "/src/extreg.h", "w").write(REG)
print("ok")
