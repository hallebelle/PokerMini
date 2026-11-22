#!/usr/bin/env python3
"""v10.1 one-off: appends the new Sophia line variants to src/lines.h (pools), converts the single-line replies to pools (src/lines.h, src/extreg.h, src/main.c).
Safe to run once on the v9.9 sources; kept for reference (the result is already in the sources)."""
import re, sys
root = sys.argv[1] if len(sys.argv) > 1 else "."
H = open(root + "/src/lines.h").read()

NEW = {   # existing POOL(name, ...): extra lines (page text: 3 lines of 46 chars; table text: 2 lines of 18 chars, | between)
"R_GREET": ["What now?", "Oh. It's you. Make it quick, boy.", "I was about to rest, you know. ...Come in, boy.", "Knocking again? You're persistent, boy.", "Hmph. Wipe that grin off your face first.", "Speak up, boy. I don't have all day."],
"R_GREET_M": ["Come in, husband. Close the door behind you.", "Hmph. You're late. ...I wasn't waiting, I just noticed.", "There you are. Sit. Don't hover in the doorway.", "Oh? A visit? ...Fine. You may come in.", "I was just thinking about you. ...Don't let it go to your head."],
"R_GREET_MM": ["Come in. And keep that word locked behind your teeth.", "You're here again. ...Sit. Quietly. No names.", "Hmph. I'll let you in if you promise to behave.", "It's you. Say that name and I'll bar the door.", "Come in, husband. Just husband. Understood?", "Oh, good. You're early. ...I didn't say that."],
"R_NIGHT_M": ["It's late, husband. ...Come in before the neighbors hear.", "Shh. Quietly. ...Get in here.", "You can't sleep alone, can you? Hmph. Come in.", "Close the door softly. Everyone is asleep."],
"R_NIGHT_MM": ["Quiet. Get in here. ...And not one word of that name.", "You woke me up. ...Come in, and keep that name to yourself.", "Late visitor. Shh. Come in, husband, before I change my mind.", "It's the middle of the night. ...Fine, come in. Behave."],
"R_HUG_OPEN": ["I spread my arms and Sophia sighs, but she steps into them anyway.", "I walk up to her and just hold her. Sophia goes still, then leans in.", "She turns around and I pull her close. Her arms creep around me slowly.", "I hug her without a word. Sophia grumbles, but her hands grip my back.", "I wrap her up from the side. She says nothing, but she doesn't leave."],
"R_HUG_OPEN_MM": ["I open my arms. Sophia rolls her eyes, but she steps in anyway.", "I hold her quietly. She stiffens, then lets out a long breath.", "Sophia mutters a complaint, yet her arms close around me.", "She stands rigid for a moment, then leans her head on my shoulder.", "I pull her into a hug and she pretends she wasn't expecting it."],
"R_KISS_OPEN": ["I cup her cheek and kiss her. Sophia gasps softly, then kisses back.", "She meets my eyes and her breath catches. I lean in and kiss her slowly.", "Sophia pretends to look away, but she lifts her face to me.", "I brush her hair back and kiss her forehead, then her lips.", "She starts to say something, so I kiss her before she can."],
"R_KISS_OPEN_MM": ["I lean in and kiss her. Sophia's face goes red, but she doesn't stop me.", "She huffs, then closes her eyes and lets me kiss her.", "I kiss her softly. Sophia clutches my sleeve and says nothing at all.", "She mumbles a protest, but it dies against my lips.", "Sophia lifts her chin, trying to look unbothered. I kiss her anyway."],
"R_SLEEP_NO_TEASE": ["Not tonight. You teased me far too much today.", "Sleep alone, husband. Think about what you said.", "No. I'm still red in the face from what you said. Alone."],
"R_SLEEP_NO_TEASE_MM": ["Not after the things you said today. Go to bed alone.", "No. Think about your teasing, then sleep alone.", "You were shameless today. Sleep alone tonight, husband.", "No sleeping together tonight. You know exactly why."],
"R_SLEEP_NO_MILK": ["Not tonight, husband. I'm tired and sore. Come back tomorrow.", "No. I gave enough today. Let me sleep by myself.", "I'm exhausted. Tomorrow, maybe. Tonight I want my bed to myself."],
"R_SLEEP_NO_MILK_MM": ["No. Today was enough for me. I want to sleep alone.", "I'm drained, husband. Come back tomorrow night.", "Not tonight. Today was more than enough for one person."],
"R_FEED_NO_MM": ["No. Absolutely not. And don't make that face.", "Not today. ...Not with that name in your mouth.", "I said no. A woman is allowed to say no, husband.", "Don't ask me that in that voice. No. Later.", "Not now. Ask when you've earned it.", "Hmph. You think asking sweetly will work? No.", "No. I'm not in the mood, and you know why. Later.", "Out of the question. Come back when you've learned manners."],
"R_FEED_AGAIN_MM": ["I said no already. Don't pester me.", "Asking twice doesn't make me say yes. Later.", "No means no, husband. Come back in a while.", "Are you deaf? I told you no. Later.", "Stop asking. Ask again later, if you must.", "Still no. Wait until I'm in a better mood."],
"R_LR_HERE": ["Here?! Out in the living room?! ...Keep it down, husband.", "Not here! The window's right there! ...Fine, but be quick.", "In the living room?! You really have no shame, do you?", "Here? Now? ...Hmph. Check that the door is locked first."],
"R_PILL6": ["My chest has felt so tight since yesterday. It's your pill, isn't it, boy?", "Something's wrong with my chest. It's heavy. Don't you dare laugh, boy."],
"R_PILL7": ["My chest feels heavier every day. ...Is this normal? Don't stare, boy.", "I can barely breathe in this shirt. It's your doing. Stop looking."],
"L_LAST": ["LAST HAND.|DON'T CHOKE, BOY.", "ONE HAND LEFT.|PRAY, BOY.", "FINAL HAND.|SHOW ME."],
"L_ENDLOST": ["YOU LOST, BUT|YOU PLAYED WELL.", "BONUSES? CUTE.|STILL A LOSS.", "A LOSS WITH|STYLE. HMPH."],
"L_SPLIT": ["A SPLIT.|SO BORING.", "EVEN? HOW|ANNOYING.", "A TIE. NO ONE|WINS, NO ONE LOSES."],
"L_YOURAISE": ["OH? YOU'RE|FEELING BOLD.", "A RAISE?|SHOW ME THEN.", "BRAVE. OR|JUST FOOLISH."],
"L_ENDTIE": ["EVEN. WE'LL|HAVE TO REMATCH.", "NO WINNER.|HOW DULL."],
"L_SHEFOLD": ["FINE. YOURS.|ENJOY IT.", "I FOLD. DON'T|LOOK SO SMUG.", "NOT WORTH THE|CHIPS. TAKE IT."],
"L_YOUFOLD": ["GIVING UP?|HOW SAD.", "SCARED, BOY?", "YOU FOLD? THEN|THE POT IS MINE."],
"L_ENDHER": ["I WIN. AGAIN.|SEE YOU SOON.", "CHEER UP, BOY.|YOU TRIED.", "TOO EASY.|PAY UP."],
"L_ENDYOU": ["YOU WIN. DON'T|LET IT GO TO YOUR HEAD.", "CONGRATS. NOW|STOP SMILING.", "I'LL WIN IT|BACK, BOY."],
"R_MEM_CALL_T_MM": ["You said it earlier. Say it again and see what happens.", "That name from before... I'm still thinking about it."],
"R_MEM_CALL_Y_MM": ["Yesterday you called me that. I'm still stewing.", "Don't start that name again like you did yesterday."],
"R_MEM_CHAT_T_MM": ["We talked earlier. You called me that again, didn't you?", "I've had a thought about our chat. Don't tempt me."],
"R_MEM_CHAT_Y_MM": ["Yesterday's chat was fine. Until that name. Remember?", "You talked my ear off yesterday. And used that name."],
"R_MEM_TEASE_Y_MM": ["Yesterday you teased me without mercy. Don't repeat it.", "I'm still annoyed about your teasing yesterday. Come in."],
"R_MEM_FEED_T_MM": ["Earlier doesn't count as anything. Do you hear me?", "I'm still flustered from earlier. Don't talk about it."],
"R_MEM_FEED_Y_MM": ["Yesterday... was a one-time thing. Don't get used to it.", "I still can't look at you after yesterday. Come in anyway."],
"R_MEM_FEEDNO_T_MM": ["I said no earlier. My answer hasn't changed.", "Don't look so hopeful. I refused you a minute ago."],
"R_MEM_FEEDNO_Y_MM": ["I turned you down yesterday. Don't sulk about it today.", "Yesterday's no was a no. Come in, but behave."],
"R_MEM_HUG_T_MM": ["That hug earlier was nothing. Don't get any ideas.", "Earlier was just... an accident. Sit."],
"R_MEM_HUG_Y_MM": ["Yesterday's hug... I'm still thinking about it. Don't smile.", "No hugs today. ...Well. Maybe a short one."],
"R_MEM_KISS_T_MM": ["I'm still blushing from earlier. Don't look at me.", "Don't expect a second kiss. ...Not yet."],
"R_MEM_KISS_Y_MM": ["After yesterday's kiss, I can't think straight. Come in.", "Yesterday was a mistake. ...A lovely mistake. Don't tell."],
"R_MEM_SLEEP_Y_MM": ["Last night was... fine. Don't gloat about it.", "You hogged the blanket last night. Remember that."],
}

# single-line replies that become pools: original text stays the first entry
SINGLE = {
"R_TIRED": ["You're swaying on your feet. Rest first, then we play.", "Go to bed, boy. Cards need a clear head.", "You look half dead. Sleep first."],
"R_TODAY": ["One match a day is plenty. Come back tomorrow.", "We played already today. Tomorrow, boy.", "I'm done for the day. Come back tomorrow."],
"R_BACK": ["Good game. Anything else?", "That was fun. ...Don't tell anyone I said that.", "Not bad. Is there something else you need?", "Well played. Now, what else?"],
"R_GIFT_TODAY": ["One gift a day is enough. Come back tomorrow.", "Don't spoil me so fast. Come back tomorrow.", "You already gave me something today. Tomorrow."],
"R_GIFT_NONE": ["You have nothing to give me.", "Your hands are empty. Bring something next time.", "Nothing? Then what are you doing here?"],
"R_GIFT_PILL": ["Not yet. Show me you really care about me first.", "Not yet. I'm not ready to take that.", "You haven't earned that yet. Keep trying."],
"R_GIFT_FULL": ["I have too many already. Keep it for later.", "I can't carry any more of these. Later.", "My place is full of those. Keep it for now."],
"R_NOT_FULL": ["I'm not full yet. Come back when the gauge is full.", "Not yet. Come back when I'm full.", "It's not full. Don't rush me."],
"R_NOT_FULL_MM": ["It isn't full yet. Come back when the gauge is full.", "Not yet. Come back when I'm full. And don't pout.", "Not full. Don't ask again until it is."],
"R_MILK_WAIT": ["Give the first one time to work. Come back in a day or two.", "It takes time. Be patient and come back in a day or two."],
"R_MILK_PUMP": ["It's not enough on its own. I need something to relieve this first. Find a pump.", "I need a way to relieve it first. Get a pump."],
"R_PUMP_NOW": ["What is that supposed to be? I don't need it.", "That? No. Not now. I don't need it.", "Put that away. I don't need it right now."],
"R_BOTTLE_FULL": ["Your pockets are full of bottles already.", "You can't carry any more bottles. Use some first.", "You have enough bottles. Don't be greedy."],
"R_OUTFIT_YOGA": ["Fine. Turn around for a moment... I'll change into the yoga outfit.", "Yoga outfit? Fine. Turn around. Don't peek."],
"R_OUTFIT_D8": ["Back to my usual clothes, then. Happy now?", "Fine. Back to my normal clothes. Turn around."],
"R_OUTFIT_YOGA_MM": ["Fine. Turn around for a moment... I'm changing into the yoga outfit.", "Yoga outfit again? Turn around. And no names."],
"R_OUTFIT_D8_MM": ["Back to my usual clothes, then. Don't say a word.", "Back to normal clothes. And not a word, husband."],
"R_MILK_FULL": ["I'm already full. Do something about it first, then come back with the next pill.", "I'm still full. Deal with that first, then bring the next pill."],
"R_MILK_AFTER": ["Hmph. Is there anything else you want from me?", "Well? Is there something else?", "Done? Then what else do you want?"],
"R_MILK_SUPER": ["Not that one. The strong one is still working in me. Come back in a few days.", "That strong one is still working. Come back in a few days."],
"R_SUPER_ON": ["The last one is still working. Don't be greedy.", "Slow down. The last one hasn't worn off yet."],
"R_ASK_DONE": ["Twice a day is more than enough. Come back tomorrow.", "That's enough for today. Come back tomorrow, greedy."],
"R_ASK_NONE": ["I have nothing to give you right now.", "There's nothing right now. Ask later."],
}
POOL_SINGLE_RE = r'static const char \*%s PQ_UNUSED = ("(?:[^"\\]|\\.)*");'

def esc(t): return '"' + t.replace('\\', '\\\\').replace('"', '\\"') + '"'

for name, extra in SINGLE.items():
    m = re.search(POOL_SINGLE_RE % name, H)
    if not m: print("single not found", name); continue
    orig = m.group(1)
    items = [orig] + [esc(t) for t in extra if esc(t) != orig]
    H = H.replace(m.group(0), "POOL(%s, %s);" % (name, ", ".join(items)), 1)

for name, extra in NEW.items():
    # POOL(name, ... ); possibly multi-line, ends with ");" at the end of the statement
    i = H.find("POOL(%s," % name)
    if i < 0: print("pool not found", name); continue
    j = H.index(");", i)
    # find real end: the first ");" that is outside a string
    k = i; ins = False
    while k < len(H):
        c = H[k]
        if c == '\\' and ins: k += 2; continue
        if c == '"': ins = not ins
        elif not ins and c == ')' and H[k+1:k+2] == ';': j = k; break
        k += 1
    add = ",".join("\n    " + esc(t) for t in extra)
    H = H[:j].rstrip() + "," + add + H[j:]
open(root + "/src/lines.h", "w").write(H)

# registry: TK_STR -> TK_POOL
R = open(root + "/src/extreg.h").read()
for name in SINGLE:
    pat = r'\{0, "%s", TK_STR, (TC_\w+), \(void \*\)&%s, 0, 0, 0, 0,' % (name, name)
    m = re.search(pat, R)
    if not m: print("reg not found", name); continue
    R = R.replace(m.group(0), '{0, "%s", TK_POOL, %s, (void *)&%s, &%s_N, 0, 0, 0,' % (name, m.group(1), name, name), 1)
open(root + "/src/extreg.h", "w").write(R)

# main.c: uses of the converted names become pick(...)
M = open(root + "/src/main.c").read()
lines = M.split("\n")
for idx, ln in enumerate(lines):
    for name in SINGLE:
        if re.search(r'\b%s\b' % name, ln) and not re.search(r'(pick|in_pool)\(\s*%s\b' % name, ln):
            ln = re.sub(r'\b%s\b(?!_N)' % name, "pick(%s, %s_N)" % (name, name), ln)
    lines[idx] = ln
open(root + "/src/main.c", "w").write("\n".join(lines))
