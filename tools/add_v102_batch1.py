#!/usr/bin/env python3
"""v10.2 batch 1 (one-off): grows the mama-mode tease topics (5 -> 12 exchanges), C_MAMA (12 -> 20) and the ending pools
(T_KICK_MM, T_OUT_MM, R_KICKED_MM, R_MORE_MM, C_FAIL_MM, R_MORE_M, T_KICK_M -> 20 each) in src/talk.h and src/lines.h.
Run once on the v10.1 sources:  python3 tools/add_v102_batch1.py .   (already applied to the sources)"""
import re, sys
root = sys.argv[1] if len(sys.argv) > 1 else "."
TALK = open(root + "/src/talk.h").read()
LINES = open(root + "/src/lines.h").read()

def cs(s):   # C string literal
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

def add_talk(src, name, items):
    m = re.search(r"static const Talk %s\[\] = \{" % name, src)
    assert m, name
    e = src.index("\n};", m.end())
    rows = []
    for it in items:
        me, a = it[0], it[1]; b = it[2] if len(it) > 2 else None
        rows.append("    {%s, %s, %s}" % (cs(me), cs(a), cs(b) if b else "NULL"))
    return src[:e] + ",\n" + ",\n".join(rows) + src[e:]

def add_pool(src, name, items):
    m = re.search(r"POOL\(%s,\s" % name, src)
    assert m, name
    e = src.index(");", m.end())
    return src[:e] + ",\n" + ",\n".join("    " + cs(x) for x in items) + src[e:]

# ---------------------------------------------------------------- tease topics (7 new exchanges each -> 12)
T = {}
T["T_LOOKS_MM"] = [
 ("Mama, you're glowing today.", "Glowing? ...Fine. But I have a name, and it isn't that."),
 ("Mama, who did your hair? It's perfect.", "Nobody. I'm naturally like this. And pick another word, please."),
 ("You'd win any beauty contest, Mama.", "I'd win anything I entered. ...Say my name next time."),
 ("Mama, I can't stop looking at your smile.", "Then look somewhere else! ...Wait, no. Just drop the title."),
 ("You're the prettiest woman in this hotel, Mama.", "I'm the prettiest woman anywhere. That is not my name, though.", "...Hmph. Thank you."),
 ("Mama, your hair smells nice.", "Stop sniffing me! And who raised you to call your wife that?"),
 ("That pose suits you, Mama.", "I'm not posing. ...Mama? Do I look like a mother to you?"),
]
T["T_SHIRT_MM"] = [
 ("Mama, is that shirt a size too small?", "Two sizes! Thanks to your pill! ...And that word is banned in this room."),
 ("Mama, one button is hanging by a thread.", "Then stop staring at it! My name has six letters. Use them."),
 ("That shirt shows off your figure, Mama.", "It's just a shirt! ...Hmph. Not that title again."),
 ("Should I buy you a bigger shirt, Mama?", "...That's actually thoughtful. Don't spoil it with that word."),
 ("Mama, the shirt looks warm. Can I borrow it?", "It would never fit you! ...And I am not your mother."),
 ("Mama, you keep tugging at your collar.", "It's tight! Don't comment! ...And say Sophia, not that."),
 ("Nice shirt, Mama. Very... stretchy.", "Say that again and I'll stretch your ears! Idiot."),
]
T["T_CHEST_MM"] = [
 ("Mama, is it sore today?", "...A little. Don't ask in that tone! I'm not your mother."),
 ("Mama, you're always so gentle with me.", "I am not! ...Fine, a little. But try my real name first."),
 ("May I just sit beside you, Mama?", "Beside. Only beside. And that title is not allowed in here."),
 ("Mama, I'll be careful with you. I promise.", "You'd better be. ...Sophia. The word you're looking for is Sophia."),
 ("Does it still ache at night, Mama?", "Sometimes. ...Don't make that worried face. And stop saying that."),
 ("You're so soft, Mama. Everything about you.", "Hmph. Flattery again. ...I have a name, you know."),
 ("I'll fetch a blanket for you, Mama.", "...That's kind. Just don't call me that. Not even kindly."),
]
T["T_BODY_MM"] = [
 ("Mama, can I lean on you for a moment?", "A moment. ...And that's not my name. Try again."),
 ("You always smell like warm soap, Mama.", "It's just soap! Stop sniffing. And I'm nobody's mama here."),
 ("Mama, your hands are so soft.", "I moisturize. A queen has standards. ...Don't use that word."),
 ("I'd carry you anywhere, Mama.", "You can't even carry the groceries. ...And that's not what I'm called."),
 ("You're like a warm blanket, Mama.", "A blanket?! ...Hmph. Fine. But say my name if you want a hug."),
 ("Mama, you look so tall in those shoes.", "They're heels, idiot. ...Wrong word. Say Sophia."),
 ("Can I hold your hand, Mama?", "...Fine. Just the hand. And choose another title."),
]
T["T_MILK_MM"] = [
 ("Mama, how full is the gauge today?", "Check it yourself! ...And no, I'm not your mother."),
 ("Mama, would you share a little tonight?", "Ask properly, with my name. Not that word. Never that word."),
 ("Mama, I'll do the dishes for a bottle.", "Hmph. A bribe? ...Not bad. But call me something else."),
 ("Does it hurt when it's full, Mama?", "A bit. Don't look at me like that! And that title stays outside."),
 ("Mama, I'll take good care of the bottle.", "You'd better. ...Sophia. Say Sophia."),
 ("Mama, I made tea. Drink first, then we talk.", "...Tea? That's actually kind. Hmph. But don't call me that."),
 ("I'm so grateful, Mama.", "Don't be dramatic! ...And be grateful to Sophia, not to some mama."),
]
T["T_TASTE_MM"] = [
 ("Mama, it tastes better every time.", "Stop reviewing it! ...And that is not my name."),
 ("It's like warm honey, Mama.", "Honey?! Don't say things like that out loud! ...Mama? No!"),
 ("Mama, could I have a little more tonight?", "You're asking for seconds?! ...Use my name, or no."),
 ("I'll never forget the taste, Mama.", "Forget it. Please. ...And I'm not your mother."),
 ("It's so sweet and warm, Mama.", "Stop describing it!! ...Pick another word, any other word!"),
 ("Mama, you're so generous.", "I'm not generous, I'm... full. ...Don't use that name!"),
 ("Mama, thank you for always sharing.", "...Don't thank me. And don't use that word to do it.", "...You're welcome."),
]

# ---------------------------------------------------------------- mama chat (8 new -> 20)
CHAT = [
 ("Mama, what are you reading?", "A book on probabilities. ...And try another word, please."),
 ("Mama, can we play cards tonight?", "Only if you stop calling me that. ...Fine. Deal."),
 ("You're beautiful when you concentrate, Mama.", "Quiet! I'm counting outs. ...And I'm not your mama."),
 ("Do you want tea, Mama?", "Yes, please. ...Sophia. The word is Sophia."),
 ("Mama, I missed you today.", "...Silly. You were gone two hours. ...I missed you too.", "Just don't use that name."),
 ("Mama, tell me a story from your tournaments.", "Once I won a pot with seven-deuce. ...Say my name and I'll tell the rest."),
 ("You're my favorite person, Mama.", "That's... fine. ...The title is not fine. I refuse it."),
 ("Mama, shall we watch the rain together?", "Sit. Blanket. ...And that word stays outside, please."),
]

# ---------------------------------------------------------------- single-line pools (to 20 each)
P = {}
P["C_FAIL_MM"] = [
 "Not now. And that is not my name.",
 "Hmph. Not in the mood. Try my name next time.",
 "I'm busy. Come back with something clever.",
 "Ask me again later. And drop the title.",
 "That topic bores me. So does that nickname.",
 "No. Call me by my name first.",
 "I'm thinking about cards. Try again later.",
 "Your timing is terrible. And so is that word.",
 "Not interested. Say something smarter, husband.",
 "My ears are ringing from that name. Later.",
 "Not today. I'm not your mother, remember?",
 "Too noisy in my head. Ask me tomorrow.",
 "Think before you talk. And think about my name.",
 "I'm not in the mood. Sit quietly for a bit.",
 "Mmh. No. Try again, with manners.",
]
P["R_MORE_MM"] = [
 "Anything else? And I'm still not your mother.",
 "Done talking? Then say something sensible.",
 "What now, husband? Be quick.",
 "Well? Speak up. Use my name if you want an answer.",
 "Is that all? Hmph. Fine.",
 "Still here? Good. ...I mean, whatever.",
 "Go on. I'm listening. Unfortunately.",
 "Anything else? No pet names this time.",
 "I could listen to you all night. ...Don't tell anyone.",
 "Tell me more. Or just stay close and be quiet.",
 "What else? And drop that word before you ask.",
 "Hm. Your turn. Choose carefully.",
 "Come on, husband. I haven't got all day.",
 "Is there something else on your mind? Besides that name?",
 "Sit with me. And if you must talk, talk normally.",
]
P["R_KICKED_MM"] = [
 "You're back already? ...Sit there, and no teasing.",
 "After earlier I should lock the door. ...Come in.",
 "I'm still red from before. Don't mention it.",
 "I won't discuss that name. Come in, quietly.",
 "Hmph. You again. One word out of line and you're out.",
 "Come in. I forgave you. Slightly.",
 "Behave, husband. I mean it this time.",
 "Don't smirk. I haven't forgotten what you said.",
 "Knock, knock. Of course it's you. ...Come in.",
 "Say that word again and you're out for the week.",
 "My ears are still hot. Be gentle with me.",
 "I should still be angry. ...Come in.",
 "Sit. Quietly. No names, no teasing.",
 "Tch. Stubborn. Come in and be good.",
 "You shameless idiot. ...Come in, then.",
]
P["T_KICK_MM"] = [
 "That's it! Out! And that is NOT my name!",
 "Enough! I'm not your mother! OUT!",
 "My face is on fire, you idiot! Get OUT!",
 "Out! Out! Out! And take that word with you!",
 "You've gone too far, husband! Go cool off!",
 "I said stop! Now go, before I throw something!",
 "Don't you dare say it again! Out of my room!",
 "Too embarrassing! OUT! Out this instant!",
 "You and your mouth! Get out of here, idiot!",
 "I'm calling security! ...No, I'm calling YOU out! GO!",
 "That does it! My name is Sophia! NOW LEAVE!",
 "Shameless! Absolutely shameless! OUT!",
 "I can't take it anymore! Go! Shoo!",
 "Husband or not, that is the end of it! OUT!",
]
P["T_OUT_MM"] = [
 "Sophia shoves me into the hall, red to her ears, and the door slams shut.",
 "A cushion hits my head and the door shuts. I hear her huff, \"Not your mama!\"",
 "She pushes me out with both hands and locks the door. Her whole face is red.",
 "Sophia points at the door with a trembling finger. I leave quickly, grinning.",
 "The door slams. Something soft hits it from the inside. She is still mumbling.",
 "She tugs my sleeve all the way to the door, her ears glowing, and shuts it behind me.",
 "I get a pillow to the face and a door in the nose. Worth it.",
 "Sophia herds me out, muttering about the name. The lock clicks.",
 "She turns her back, shoulders shaking, and orders me out. I obey.",
 "The door closes, then opens a crack. \"Idiot,\" she whispers, and shuts it again.",
 "Sophia stomps to the door, red as a tomato, and bangs it shut on my laughter.",
 "She shoos me into the hallway. I hear her fanning herself through the wood.",
 "The lock clicks. Through the wood I hear her whisper, \"Stupid husband.\"",
 "She shoves a pillow at me and the door at my back. Then there is only muttering.",
 "I'm out in the hall before I can speak. The whole door shakes with her slam.",
]
P["R_MORE_M"] = [
 "Anything else, husband? Don't dawdle.",
 "Done? Or is there more on your mind?",
 "Well? Speak, husband. I'm listening.",
 "Hmph. Anything else? Think carefully.",
 "Is that all? Then sit with me.",
 "Ask me something else. Something interesting.",
 "More talking? Fine. I don't mind. ...Don't tell anyone.",
 "Tell me more, husband. Or just stay close.",
 "You have my attention. Don't waste it.",
 "What else? Don't be shy now.",
 "Hm. Your move, husband.",
 "Come on, don't go quiet on me. Say something.",
 "I could listen for hours. ...Don't let it go to your head.",
 "What else is on your mind, dear?",
 "Go on. Surprise me.",
]
P["T_KICK_M"] = [
 "That's enough, husband! Out of my sight!",
 "You're impossible! Out, before I lose my temper!",
 "Husband or not, there are limits! LEAVE!",
 "One more word and you sleep on the sofa! OUT!",
 "Out! And think about what you just said!",
 "My face is burning, you fool! Out!",
 "That's the last straw, husband! Go!",
 "How dare you say that out loud! Get OUT!",
 "I need space. Immediately. GO!",
 "Out of my room! Come back when you've calmed down!",
 "Stop it! I mean it! Now get out!",
 "You shameless man! Out, out, out!",
 "I won't listen to this! The door is that way!",
 "Go cool off, husband. Preferably far away. OUT!",
]

# ---------------------------------------------------------------- checks (the game checks again when it loads)
ALLOWED = set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,!?'\"-:;()$&+=/*<>@#")
def chk(t, where):
    assert len(t) <= 128, (where, len(t), t)
    for ch in t: assert ch in ALLOWED, (where, repr(ch), t)
    assert not re.search(r"\bboy\b|baby", t, re.I), (where, t)
for k, v in T.items():
    for it in v:
        for x in it: chk(x, k)
for it in CHAT:
    for x in it: chk(x, "CHAT")
for k, v in P.items():
    for x in v: chk(x, k)

for k, v in T.items(): TALK = add_talk(TALK, k, v)
TALK = add_talk(TALK, "C_MAMA_T", CHAT)
for k in ("C_FAIL_MM", "T_KICK_MM", "T_OUT_MM", "T_KICK_M"): TALK = add_pool(TALK, k, P[k])
for k in ("R_MORE_M", "R_MORE_MM", "R_KICKED_MM"): LINES = add_pool(LINES, k, P[k])
open(root + "/src/talk.h", "w").write(TALK)
open(root + "/src/lines.h", "w").write(LINES)
print("ok:", sum(len(v) for v in T.values()), "tease exchanges,", len(CHAT), "chat exchanges,", sum(len(v) for v in P.values()), "single lines")
