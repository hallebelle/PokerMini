#!/usr/bin/env python3
"""v12.1 (one-off): COW MODE. Disgust at the cow outfit (gift + wear exchange) and a cow mode while she wears it:
her whole dialogue turns negative, but she still accepts milk / pump / pill / super drug / breastfeed.
Run once on the v12.0 sources:  python3 tools/add_v121.py .   (already applied to the sources)"""
import re, sys
root = sys.argv[1] if len(sys.argv) > 1 else "."
def rd(p): return open(root + "/" + p).read()
def wr(p, s): open(root + "/" + p, "w").write(s)
def cs(s): return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'
BAD = set('[]{}~^_`|')
def chk(t, lim=130):
    assert len(t) <= lim, (len(t), t)
    assert not (set(t) & BAD), t
    assert "baby" not in t.lower() and not re.search(r"\bboy\b", t.lower()), t

# ---------------------------------------------------------------- texts: gift scene (disgust) ----
GO_COW = [
 "Cow print?! Ugh... Is that really what you think of when you look at me? Disgusting.",
 "Spots and a cutout. Ew. Why would anyone buy this for his own wife?",
 "You bought me a cow costume?! ...I feel sick just holding it. What is wrong with you?",
 "Is this a joke?! A cow print outfit... Ugh, keep it away from me. It's revolting.",
 "...What is this? Cow spots? Ew, ew. You truly think I would wear this?!",
 "Cow print. Ugh. I am a queen, not something you keep in a barn. This is repulsive.",
]
GL_COW = [
 "Get that thing away from my face. I can't believe you looked at it and thought of me.",
 "It's cheap fabric and bad taste. I'm disgusted that you picked it out at all.",
 "Do you understand how degrading this is? Ugh... I feel dirty just looking at the spots.",
 "Don't you dare smile. I'm not touching it until you stop looking so pleased. Ugh, I can't even look at you.",
 "Spare me the explanation. Whatever you imagined, it makes my skin crawl.",
 "A farm animal. That's what this says. I'm so disgusted with you that I can barely speak.",
]
GE_COW = [
 "She holds the bag at arm's length, wrinkles her nose as if it smelled, and carries it inside anyway.",
 "She shudders, pinches the cutout between two fingers, and shuts the door without a word.",
 "She flinches away from the spots as if they might bite, yet she doesn't give the outfit back.",
 "She goes pale, gags a little, and takes the outfit inside, holding it far from her body.",
 "She curls her lip at me with open revulsion, then snatches the bag and slams the door.",
 "She stares at the spots with pure disgust, mutters that she'll throw it out, and keeps it.",
]
GO_COW_MILK = [
 "Cow print, and my chest is heavy and sore from that pill? Ugh... Is this what you were thinking all along?",
 "A cow outfit right after that pill. ...Disgusting. Do you see me as a cow now?",
 "Cow print on top of this swollen chest... I feel sick. Don't you dare say what that pill is for.",
 "Spots and a cutout, and you know what that pill did to my chest. Ew. How low can you go?",
 "Cows are for milk, and I'm... no. No! This is revolting. Why this, right after that pill?",
 "I'm already sore from that pill, and now cow spots? ...I'm so disgusted I could scream.",
]
GL_COW_MILK = [
 "Say it. Say you picked this because of my chest, and watch how fast you regret it. Ugh!",
 "Cows give milk. Don't you dare say it out loud. I'm disgusted enough already.",
 "Do you think a cow is what that pill has turned me into? ...That's vile. Absolutely vile.",
 "My chest hurts, my head spins, and you hand me THIS? I can't even look at it. Disgusting.",
 "If you planned this with that pill, I will never forgive you. It makes me physically sick.",
 "Spots on a swollen chest... Ugh. Take that smile off your face before I take it off for you.",
]
GE_COW_MILK = [
 "She covers her chest with the bag, shivering with disgust, then slams the door.",
 "She glares at the spots with revulsion, then at me, and shuts the door hard.",
 "She presses the bag to her swollen chest, gags at the smell of new fabric, and storms inside.",
 "She hisses that I make her sick, then slams the door behind her, outfit and all.",
 "She goes pale, wrinkles her nose, and takes it inside, holding it far from her body.",
 "She shudders, says nothing, and closes the door so slowly that it feels worse than a slam.",
]
R_OUTFIT_COW = [
 "...It's on. Ugh. I feel filthy. Don't look at me. Don't you dare look.",
 "There. I look like a farm animal and I can feel it crawling on my skin. I hate this.",
 "Done. I'm disgusted with myself and with you. Not one word, husband.",
 "Hmph... the fabric clings in all the wrong places. Revolting. Just hand me a blanket.",
 "Ugh. There. Happy? I feel like something kept in a barn. Keep your eyes to yourself.",
 "It's on. Ugh. I want to scrub my skin. Say anything and it comes off.",
]
R_OUTFIT_COW_MM = [
 "...Done. Ugh. And you used that name while I put this on. I feel disgusting.",
 "There. A cow, and with that name too, apparently. Is that what you wanted? Revolting.",
 "It's on. I can't stand it. Don't look at me and don't say that word.",
 "Hmph. Cow spots on me and that name in your mouth. I want to hide under a blanket.",
 "Done. I hate how this feels. One comment from you and it goes straight in the bin.",
 "There. I'm sickened with myself. Turn around, and keep that name to yourself.",
]
R_OUTFIT_UNCOW = [
 "Finally. Out of that disgusting thing. Don't ever ask me again.",
 "Ahh... real fabric again. I feel clean. Never mention cows to me.",
 "That's better. Don't look at the discarded outfit. I'll burn it someday.",
 "Done. I can finally breathe. Don't you dare look so disappointed.",
]
R_OUTFIT_UNCOW_MM = [
 "Finally. Out of that disgusting thing. And not one word of that name.",
 "Ahh... real fabric again. I feel clean. Forget cows. Forget names.",
 "That's better. Don't look at it. And stop calling me that.",
 "Done. I can finally breathe. Don't look so disappointed.",
]
# wear exchange: groups of 3 (you, her refusal, her give-in)
ME, SO = "W_ME", "W_SOPHIA"
WEAR = [
 ("Sophia, would you wear the cow print outfit for me?",
  "The cow print?! Ugh... just hearing it turns my stomach. Do you have any idea how degrading that is?",
  "...Fine. But only because I said yes before. Turn around. And don't you dare enjoy it."),
 ("Could you put on the cow outfit today?",
  "That revolting thing again? Spots, a cutout... I can't stand even touching it. No.",
  "...Ugh. Fine. One hour. Face the wall and don't breathe a word."),
 ("I'd love to see you in the cow outfit.",
  "You'd love to see me humiliated, you mean. How disgusting. Do you get a kick out of it?",
  "...Hmph. Fine. Turn around. I'm doing this with my eyes shut and my teeth clenched."),
 ("Sophia, the cow print outfit... would you?",
  "I'd rather wear a trash bag. That outfit is repulsive, and so is the idea behind it.",
  "...Fine. Fine! But I will hate every second of it. Turn around."),
 ("Would you wear the cow outfit again for me?",
  "Again?! Ew. I can still feel the fabric on my skin from last time. Why do you do this to me?",
  "...Ugh. Fine. Face the wall. If you smile, I'm changing back instantly."),
 ("Please, Sophia. Just the cow outfit, for me.",
  "Please?! You say please for something this vile? I feel sick just picturing myself in it.",
  "...Hmph. Fine. Don't look. Don't talk. And don't you dare be pleased about it."),
]
WEAR_MM = [
 ("Mama, would you wear the cow print outfit for me?",
  "Don't call me that! And a cow outfit?! Ugh, that is revolting. I'm not your mama and I'm certainly not a cow.",
  "...Fine. But only because I'm too tired to argue. Turn around and keep that name to yourself."),
 ("Mama, could you put on the cow outfit today?",
  "Stop calling me that! And that disgusting outfit again? I can't stand the sight of it.",
  "...Ugh. Fine. Face the wall. One more name from you and I'll tear it off."),
 ("Mama... I want to see you in the cow outfit.",
  "Don't say it like that! Cow spots and that name together? That's degrading. I feel dirty just hearing it.",
  "...Hmph. Fine. Eyes on the wall, mouth shut, and no names."),
 ("Mama, the cow print outfit... would you?",
  "M-mama?! No! Not in that revolting thing. Do you hear what you're asking?",
  "...Ugh. Fine. But I'm doing it with my eyes closed. Don't turn around."),
 ("Mama, wear the cow outfit again for me?",
  "Again?! With that name?! Ew. I hate how it feels, and I hate how you look at me in it.",
  "...Fine. Turn around. And stop calling me that. Not one word."),
 ("Please, Mama. Just the cow outfit.",
  "Don't you dare say that name while you ask for that! It's vile. Both of them are vile.",
  "...Hmph. Fine. Don't look. Don't talk. And don't you dare look pleased."),
]

# ---------------------------------------------------------------- texts: cow mode menu pools ----
R_GREET_CW = [
 "Oh, it's you. Ugh... Come in. Don't look at what I'm wearing.",
 "Back again? Fine, come in. And stop staring at the spots.",
 "You again. I'm in this ridiculous outfit because of you. Sit down and say nothing.",
 "Hmph. Come in, husband. I feel like a farm animal, so don't make it worse.",
 "Come in. Quickly. I can't stand being seen like this.",
 "It's you. Sit. And if you smile at this outfit, you can leave.",
 "Ugh, you're here. Fine. Keep your eyes above my neck. I mean it.",
 "Come in. I'm in a foul mood and this outfit isn't helping. Behave.",
]
R_GREET_CW_MM = [
 "...Come in. I'm in this ridiculous outfit, and if you use that name, the door shuts.",
 "Oh, it's you. Disgusting outfit, disgusting name. Come in, husband, and say neither.",
 "Fine, come in. I'm not your mama and I'm not a cow. Remember both.",
 "You again. Sit. Not a word about the outfit and not a word of that name.",
 "Ugh. Come in before anyone sees me like this. And no names.",
 "Come in, husband. I feel ridiculous enough without that word from you.",
 "Hmph. Spots on me and that name from you. Come in and behave.",
 "It's you. Sit down quietly. I'm in no mood for names or comments.",
]
R_NIGHT_CW = [
 "It's late. ...Come in, and don't you dare look at this outfit.",
 "At this hour? Ugh. Fine, come in. I'm wearing this nonsense because of you.",
 "Shh. Get in here. I feel ridiculous, so keep your comments to yourself.",
 "Couldn't sleep? Come in. And don't stare at the spots.",
 "You woke me. Hmph. Come in, husband, and say nothing about the outfit.",
 "Middle of the night, and I'm dressed like livestock. Come in, quietly.",
]
R_NIGHT_CW_MM = [
 "It's late. ...Come in, and keep that name and any comment about this outfit to yourself.",
 "At this hour? Ugh. Come in. No names, no comments.",
 "Shh. Get in here. I'm not your mama and I'm not a cow. Remember that.",
 "Couldn't sleep? Fine. Come in. Say that name and I'll bar the door.",
 "You woke me. Come in, husband. Eyes up, mouth shut.",
 "Middle of the night, cow spots, and that name from you. Come in, quietly.",
]
R_MORE_CW = [
 "Anything else? Hurry up. This outfit itches.",
 "Are you done? I'd like to get out of these clothes.",
 "Hmph. Well? What else do you want from me?",
 "Is that all? I feel ridiculous standing here.",
 "Done talking? Then what else, husband?",
 "Ugh. Is there more? Make it quick.",
]
R_MORE_CW_MM = [
 "Anything else? And not that name, please.",
 "Are you done? I'm not your mama, I'm just stuck in this outfit.",
 "Hmph. What else do you want? No names this time.",
 "Is that all? I feel ridiculous. Make it quick.",
 "Done? Then what else, husband? Husband, understood?",
 "Ugh. More? Hurry up, and keep that word to yourself.",
]
R_NOT_FULL_CW = [
 "Not full yet. Come back when it is. Ugh, don't look at my chest like that.",
 "It's not time. Stop staring at this outfit and wait.",
 "Not yet. Honestly, you think of nothing else when I wear this, do you?",
 "Still not full. Don't rush me. Don't make me say more.",
 "No. Come back when the gauge is full. Disgusting.",
 "Not yet! And wipe that grin off your face.",
]
R_NOT_FULL_CW_MM = [
 "Not full yet. And don't call me that.",
 "It's not time. Stop using that name and wait.",
 "Not yet. You and your names. Come back when it's full.",
 "Still not full. Don't rush me, and don't say that word.",
 "No. Come back when the gauge is full. And no names, ugh.",
 "Not yet! Wipe that grin off your face and keep that name away.",
]
R_HUG_OPEN_CW = [
 "I open my arms. Sophia wrinkles her nose and steps back, then lets me hold her, stiff as a board.",
 "I wrap my arms around her. She shudders at the spotted fabric but doesn't pull away.",
 "I pull her close. She mutters about feeling filthy, yet she stays put.",
 "I hug her from behind. Sophia sighs like a martyr and goes rigid.",
 "I hold her without a word. She glares at the wall, jaw tight, and endures it.",
 "I step in and hug her. She turns her face away in disgust, but her hands stay on my arms.",
]
R_HUG_OPEN_CW_MM = [
 "I open my arms and whisper that name. Sophia grimaces and goes stiff, but she doesn't pull away.",
 "I rest my head on her shoulder. She shudders at the name and the spotted fabric, and still holds on.",
 "I pull her close. She hisses a protest and stays rigid in my arms.",
 "I hug her from behind. Sophia sighs as if I'd asked for the world, and endures it.",
 "I hold her quietly. She glares at the wall, jaw tight, and mutters about the name.",
 "I step in and hug her. She turns away in disgust, but her hands stay on my arms.",
]
R_KISS_OPEN_CW = [
 "I tilt her chin up. Sophia winces in distaste, then lets me kiss her cheek, stiff.",
 "I lean in and kiss her. She screws up her face, but she doesn't push me away.",
 "I brush her hair back and kiss her. She huffs with disgust, yet doesn't move.",
 "She turns her face away, so I kiss her cheek. She sighs and puts up with it.",
 "I kiss her quickly. Sophia grimaces as if she had tasted something sour.",
 "I cup her cheek and kiss her. She shudders, then lets it happen.",
]
R_KISS_OPEN_CW_MM = [
 "I whisper that name and kiss her cheek. Sophia winces and shudders, but she doesn't pull back.",
 "I lean in and kiss her. She screws up her face at the name, yet lets me.",
 "I brush her hair back and kiss her. She huffs in disgust, but stays put.",
 "She turns her face away, so I kiss her cheek. She mutters about names and endures it.",
 "I kiss her quickly. Sophia grimaces as if she had tasted something sour.",
 "I cup her cheek and kiss her. She shudders at that name, then lets it happen.",
]
C_FAIL_CW = [
 "Not now. I'm too disgusted with myself to talk.",
 "Go away. I'm busy hating this outfit.",
 "Not in the mood. Look at me, I'm dressed like a barn animal.",
 "Boring. And these spots itch. Try again later.",
 "Not now. Ask me when I'm in normal clothes.",
 "I'm in no state to chat. Ask me again later.",
]
C_FAIL_CW_MM = [
 "Not now. And stop calling me that.",
 "Go away. I'm busy hating this outfit, and that name.",
 "Not in the mood. Not a cow, not a mama. Ask me later.",
 "Boring. These spots itch. And no names.",
 "Not now. Ask when I'm in normal clothes.",
 "I'm in no state to chat, and you know why.",
]
# chat exchanges (you, her answer, her second line or None)
C_COW_T = [
 ("Do you want some tea, Sophia?", "Yes. ...And no, I'm not going to thank you in this ridiculous outfit.", None),
 ("You look... interesting today.", "Interesting?! Say that again and I'll feed you the spots. I feel disgusting.", None),
 ("Want to play cards tonight?", "Only if I'm dressed properly. ...Fine. Deal. And stop looking at me.", None),
 ("I missed you today.", "...You saw me an hour ago. Ugh. I missed you too. Don't tell anyone.", "And don't look at the outfit."),
 ("Is the outfit really that bad?", "It's revolting. The fabric itches and it shows everything.", "...And you know that. Stop smiling."),
 ("Shall we watch the rain together?", "Fine. Sit. A blanket for me. A big one. To cover this nonsense.", None),
 ("What are you thinking about?", "How much I'd like to burn this outfit. And how you'd cry if I did.", None),
 ("You're my favorite person, Sophia.", "...That is the only thing keeping me from tearing this off. Don't push it.", None),
]
C_COW_MM_T = [
 ("Mama, do you want some tea?", "Yes. ...And don't call me that. I'm not your mama, and I'm certainly not a cow.", None),
 ("You look so cute today, Mama.", "Stop it! Cute?! I feel disgusting. And that name makes it worse.", None),
 ("Mama, can we play cards tonight?", "Only if you stop that name. And let me change first. ...Fine. Deal.", None),
 ("Mama, I missed you today.", "...Silly. It was an hour. Ugh. I missed you too.", "Just don't say that name. Or look at this outfit."),
 ("Mama, is the outfit that bad?", "It's revolting. Don't ask me again, and don't call me that.", "...I hate it. Stop smiling."),
 ("Mama, shall we watch the rain?", "Sit. A big blanket. And that word stays outside. So does my dignity.", None),
 ("What are you thinking about, Mama?", "How to burn this outfit. And how to get you to stop that name.", None),
 ("You're my favorite person, Mama.", "That's... fine. The title is not fine. The outfit isn't either. Don't push it.", None),
]

# ---------------------------------------------------------------- texts: scenes ----
N, S, M = "W_NAR", "W_SOPHIA", "W_ME"
def beat(who, bg, spr, txt): return (who, bg, spr, txt)
HUG_CW = [beat(N,"BG_HUG",0,None), beat(S,"BG_HUG",0,"...Hmph. Only because I've already said yes. This fabric itches and I hate it. Don't get used to this."),
          beat(M,"BG_HUG",0,"I just want to hold you for a bit."),
          beat(N,"BG_HUG",0,"She stays stiff and grumbling in my arms, but she does not let go. For a while neither of us says anything.")]
HUG_CW_MM = [beat(N,"BG_HUG",0,None), beat(S,"BG_HUG",0,"...This doesn't count. I'm wearing this disgusting thing and you're using that name. Don't you dare say it again."),
          beat(M,"BG_HUG",0,"Your arms are the best place in the whole hotel, Mama."),
          beat(N,"BG_HUG",0,'She mutters, "Stop it, I\'m Sophia," with a sour face, yet she doesn\'t let go. For a while neither of us moves.')]
KISS_CW = [beat(N,"BG_KISS",0,None), beat(S,"BG_KISS",0,"...Ugh. In this outfit?! Who said you could do that so suddenly?"),
          beat(M,"BG_KISS",0,"I'm your husband. I don't need to ask."),
          beat(S,"BG_KISS",0,"Hmph... Fine. Just that. I'm not looking forward to another one in this thing."),
          beat(N,"BG_KISS",0,"Her nose is wrinkled and her cheeks are pink. She turns away, but her hand stays in mine.")]
KISS_CW_MM = [beat(N,"BG_KISS",0,None), beat(S,"BG_KISS",0,"...Hmph. That was your one. In this outfit, with that name. Don't get ideas."),
          beat(M,"BG_KISS",0,"Thank you, Mama."),
          beat(S,"BG_KISS",0,"Don't say that! I'm not that and I'm not a cow. ...Say my name, and maybe one more."),
          beat(N,"BG_KISS",0,"She kisses me once more, quickly, with a sour face, then turns away and refuses to look at me.")]
SLEEP_NAR1 = "It is late and the hotel is quiet. I ask Sophia if I can sleep beside her. She glances at her cow print outfit and winces."
SLEEP_CW = [beat(N,"BG_SLEEPT",0,SLEEP_NAR1),
          beat(S,"BG_SLEEPT",0,"...Fine. But only sleeping. I'm keeping this itchy thing on to spite you, husband. Stay on your half."),
          beat(M,"BG_SLEEPT",0,"Understood. Good night, Sophia."),
          beat(N,"BG_SLEEPT",0,"She turns off the light with a long sigh. In the dark her hand finds mine anyway, and I hear her breathing slow down."),
          beat(S,"BG_SLEEPT",0,"...Good night. And not one word about this outfit in the morning. Not one."),
          beat(N,"BG_SLEEPT",0,"I fall asleep to the sound of the rain against the window, with her warmth right beside me.")]
SLEEP_CW_MM = [beat(N,"BG_SLEEPT",0,SLEEP_NAR1),
          beat(S,"BG_SLEEPT",0,"...Only if you don't say that name. Not once. And not a word about this outfit either. Understood?"),
          beat(M,"BG_SLEEPT",0,"Good night, Mama."),
          beat(S,"BG_SLEEPT",0,"I SAID... ugh. Never mind. Good night, husband. And no stealing the blanket."),
          beat(N,"BG_SLEEPT",0,"She turns off the light, stiff and sour at first. Slowly she relaxes, and her hand finds mine."),
          beat(N,"BG_SLEEPT",0,"I fall asleep to the sound of the rain against the window, wrapped in her warmth.")]
NIGHTM_CW = [beat(N,"BG_BDOOR",0,"It's late at night. I knock softly on our bedroom door."),
          beat(S,"BG_BDOOR",0,"...Who is it? Don't tell me you want to see me in this ridiculous outfit again."),
          beat(S,"BG_BDOOR",5,"...It's you. Come in, quickly. And don't look at the spots."),
          beat(N,"BG_ROM",0,"I slip inside and close the door behind me. The lamp is low and her room is warm.")]
NIGHTM_CW_MM = [beat(N,"BG_BDOOR",0,"It's late at night. I knock softly on our bedroom door."),
          beat(S,"BG_BDOOR",0,"...Who is it? If it's that silly name again, or a comment on this outfit, go back to bed."),
          beat(S,"BG_BDOOR",5,"...It's you. Come in, quickly. And don't say any of it out loud."),
          beat(N,"BG_ROM",0,"I slip inside and close the door behind me. The lamp is low and her room is warm.")]
def feed(lr, mm):
    room = "sofa" if lr else "bed"
    if not mm:
        return [beat(N,"BG_ROM",1,"I ask her to breastfeed me. Sophia crosses her arms over her chest and looks down at her cow print outfit with disgust."),
          beat(S,"BG_ROM",1,("Right now?! In this? ...Ugh. Fine. You're my husband. But I feel disgusting. Sit on the sofa and don't stare." if lr else
                              "Right now?! In this? ...Ugh. Fine. You're my husband. But I feel disgusting. Don't stare while I get ready.")),
          beat(N,"BG_FEED",0,"Sophia sits on the %s, her mouth pressed into a thin line, her ears bright red." % ("sofa" if lr else "edge of the bed")),
          beat(S,"BG_FEED",0,("My chest is so full it aches, otherwise I would never. ...Not a word about this outfit, and keep it quiet." if lr else
                              "My chest is so full it aches, otherwise I would never. ...Not a word about this outfit. Not one.")),
          beat(M,"BG_FEED",0,"I won't. Tell me if it's too much."),
          beat(N,"BG_FEED",0,"She draws me close, stiff and sour-faced, one hand resting in my hair."),
          beat(S,"BG_FEED",0,("...Hurry up. I hate how this feels. Don't look so happy. And keep an eye on the door." if lr else
                              "...Hurry up. I hate how this feels. Don't look so happy. You're being awfully greedy.")),
          beat(N,"BG_FEED",0,"Afterwards she lets out a long breath and mutters that I'd better never mention it. She still holds me a bit longer.")]
    return [beat(N,"BG_ROM",1,'I sit beside her and ask softly, "Mama... may I?" Sophia crosses her arms over her chest, glaring down at the cow print.'),
          beat(S,"BG_ROM",1,("Don't call me that! ...And not out here, in this disgusting outfit. I'm only doing this because it aches." if lr else
                              "Don't call me that! ...And in this disgusting outfit, too. I'm only doing it because it aches. That's all.")),
          beat(N,"BG_FEED",0,"Sophia sits on the %s, avoiding my eyes, her ears bright red." % ("sofa" if lr else "edge of the bed")),
          beat(S,"BG_FEED",0,("...Just this once. If anyone knocks, we stop. And not a word about me, the outfit, or that name." if lr else
                              "...Just this once. Don't say a word. About me, the outfit, or that name. ...I feel sick about all of this.")),
          beat(M,"BG_FEED",0,"I understand. Tell me if it's too much, Sophia."),
          beat(N,"BG_FEED",0,"She draws me close, stiff and sour-faced, one hand hesitant in my hair."),
          beat(S,"BG_FEED",0,"...Good. Like that. Don't look up at me. ...I hate this outfit, and I still hate that name."),
          beat(M,"BG_FEED",0,"Thank you."),
          beat(N,"BG_FEED",0,"Afterwards she lets out a long breath and tells me to forget it ever happened. She still holds me a little longer.")]

# opener pairs (you, her reaction that complains and still agrees); one set per kind, normal and mama
ASK = [
 ("Sophia, is it full yet? I'd like a bottle.", "Do you hear yourself? Asking your cow-print wife for a bottle... Ugh. Fine. It's heavy anyway. Turn around."),
 ("Could you pump some for me today?", "In this outfit you want me to... ugh, I feel like livestock. Fine. Get the pump and face the wall."),
 ("My bottle is empty. Could you fill it?", "Fill it?! Disgusting. Like I'm a machine in a barn. ...Hmph. Fine. Don't look at me."),
 ("I'd love some of your milk today.", "Don't say it like that! It's revolting to hear. ...My chest is heavy, so fine. Turn around."),
 ("Sophia, would you pump some for me?", "Ugh, with spots on me, no less. ...Fine. But not one word while I do it."),
 ("Is it time for a bottle, Sophia?", "Stop asking so cheerfully! ...Fine. It does ache. Get out the pump and face away."),
]
ASK_MM = [
 ("Mama, is it full yet? I'd like a bottle.", "Don't call me that! And this outfit... ugh, I feel like livestock. ...Fine. Turn around."),
 ("Mama, could you pump some for me today?", "Stop calling me that! Disgusting, all of it. ...Hmph. Get the pump and face the wall."),
 ("Mama... my bottle is empty. Could you fill it?", "Don't call me that! Fill it?! Like a cow in a barn?! ...Ugh. Fine. Don't look."),
 ("Mama, I'd love some of your milk today.", "Don't say it like that! It's revolting. ...My chest is heavy, so fine. Turn around."),
 ("Mama, would you pump some for me?", "Mama?! In this outfit?! Ugh. ...Fine. But not one word while I do it."),
 ("Mama, is it time for a bottle?", "Stop asking so cheerfully, and stop that name! ...Fine. It does ache. Face away."),
]
PILL = [
 ("Sophia, here's your pill. Take it for me?", "Give me that. And don't look at me while I swallow it. Ugh, I look ridiculous."),
 ("Time for your pill, Sophia. Open up.", "I can take a pill by myself! ...Ugh. Fine. Hand it over and turn around."),
 ("Please take this one. For both of us.", "Both of us?! ...Disgusting. Fine. Give it here. And not a word about this outfit."),
 ("Your pill. I'll hold the water.", "I can hold my own glass. ...Hmph. Give it here. I hate every part of this."),
]
PILL_MM = [
 ("Mama, here's your pill. Take it for me?", "Don't call me that! ...Give it here. And don't watch me swallow it in this ridiculous outfit."),
 ("Mama, time for your pill. Open up.", "Stop calling me that! Ugh. Fine. Hand it over and turn around."),
 ("Mama, please take this one. For both of us.", "M-mama?! Disgusting. ...Fine. Give it here. Not a word."),
 ("Mama, your pill. I'll hold the water.", "Don't call me that! ...Hmph. Give it here. I hate every part of this."),
]
SUPER = [
 ("I got the strong one, Sophia. Please drink it.", "That one is strong. ...Ugh. Fine. Give it here. Face the wall and don't gloat."),
 ("This is the super drug. Are you ready?", "Ready?! No. ...Hand it over anyway. I'm too disgusted to argue."),
 ("Drink this and you'll feel so full.", "Don't say it like that! ...Hmph. Give me the vial and don't comment."),
 ("It's the strong one again. Ready?", "Ugh. Ready or not. Hand it over, and keep your eyes on the wall."),
]
SUPER_MM = [
 ("Mama, I got the strong one. Please drink it.", "Don't call me that! ...That one is strong. Fine. Give it here. Face the wall."),
 ("Mama, this is the super drug. Are you ready?", "Don't say that name! Ready?! No. ...Hand it over anyway. I'm too disgusted to argue."),
 ("Mama, drink this and you'll feel so full.", "Stop it with that name! ...Hmph. Give me the vial. And don't comment."),
 ("Mama, it's the strong one again. Ready?", "Stop calling me that! ...Ugh. Ready or not. Hand it over and face the wall."),
]
MID_PUMP = [
 ("Are you alright back there, Sophia?", "Don't talk to me! I feel like livestock. ...Just keep your eyes on the wall."),
 ("Take your time. I'm not looking.", "Good. Because I'm disgusted enough with myself already. Hmph."),
 ("Is it helping? Does it hurt?", "It's... fine! Ugh, stop asking. Every word makes this worse."),
 ("I'm facing the wall, I promise.", "You'd better be. This outfit and this pump... I could die of shame. Don't you dare turn."),
 ("Almost done?", "Don't rush me! ...Ugh. A little more. Don't turn around."),
]
MID_PUMP_MM = [
 ("Mama, are you alright back there?", "Don't call me that! I feel like livestock. ...Just keep your eyes on the wall."),
 ("Take your time, Mama. I'm not looking.", "Good. And stop that name. I'm disgusted enough already."),
 ("Mama, is it helping? Does it hurt?", "It's fine! Ugh, stop asking, and stop that name. Every word makes this worse."),
 ("I'm facing the wall, Mama. I promise.", "You'd better be. And don't call me that. I could die of shame. Don't turn."),
 ("Mama, almost done?", "Don't rush me! ...Ugh. A little more. Don't turn around, and no names."),
]
MID_FEED = [
 ("Is this alright, Sophia?", "Don't ask! It's... fine. Ugh. I hate how this feels. Just hold still."),
 ("Tell me if you want me to stop.", "If I wanted that I'd have said so. ...Hmph. Keep going. And stop talking."),
 ("You're so warm, Sophia.", "Disgusting. Don't say things like that in this outfit. ...Fine. Stay there."),
 ("Thank you, Sophia. Really.", "Save it. I feel humiliated enough. ...Just a little longer, then it's over."),
 ("Are you comfortable?", "No! ...Yes. Ugh. Don't make me answer that. Hold still."),
]
MID_FEED_MM = [
 ("Is this alright, Mama?", "Don't call me that! It's... fine. Ugh. I hate how this feels. Just hold still."),
 ("Mama, tell me if you want me to stop.", "If I wanted that I'd have said so. ...Hmph. Keep going. And no names."),
 ("You're so warm, Mama.", "Disgusting. Don't say that in this outfit. ...Fine. Stay there."),
 ("Thank you, Mama. Really.", "Save it, and that name. I feel humiliated enough. ...Just a little longer."),
 ("Mama, are you comfortable?", "No! ...Yes. Ugh. Don't make me answer. Hold still."),
]

# ---------------------------------------------------------------- validate every text ----
for lst in (GO_COW, GL_COW, GE_COW, GO_COW_MILK, GL_COW_MILK, GE_COW_MILK, R_OUTFIT_COW, R_OUTFIT_COW_MM, R_OUTFIT_UNCOW, R_OUTFIT_UNCOW_MM,
            R_GREET_CW, R_GREET_CW_MM, R_NIGHT_CW, R_NIGHT_CW_MM, R_MORE_CW, R_MORE_CW_MM, R_NOT_FULL_CW, R_NOT_FULL_CW_MM,
            R_HUG_OPEN_CW, R_HUG_OPEN_CW_MM, R_KISS_OPEN_CW, R_KISS_OPEN_CW_MM, C_FAIL_CW, C_FAIL_CW_MM):
    for t in lst: chk(t)
for g in WEAR + WEAR_MM:
    for t in g: chk(t)
for a, b in ASK + ASK_MM + PILL + PILL_MM + SUPER + SUPER_MM + MID_PUMP + MID_PUMP_MM + MID_FEED + MID_FEED_MM: chk(a); chk(b)
for a, b, c in C_COW_T + C_COW_MM_T:
    chk(a); chk(b); (c and chk(c))
for sc in (HUG_CW, HUG_CW_MM, KISS_CW, KISS_CW_MM, SLEEP_CW, SLEEP_CW_MM, NIGHTM_CW, NIGHTM_CW_MM, feed(0,0), feed(1,0), feed(0,1), feed(1,1)):
    for b in sc:
        if b[3]: chk(b[3], 140)
for t in GO_COW_MILK: assert any(w in t for w in ("pill", "milk", "chest")), t
for lst in (GO_COW, GL_COW): assert all("milk" not in t for t in lst)
assert len(GO_COW) == len(GL_COW) == len(GE_COW) and len(GO_COW_MILK) == len(GL_COW_MILK) == len(GE_COW_MILK)
for t in GO_COW_MILK + GL_COW_MILK + GE_COW_MILK + GO_COW + GL_COW + GE_COW: assert "baby" not in t.lower()
# mama (and mama+cow) texts must not call you boy / baby (the existing v97 check covers every name ending in _MM)

# ---------------------------------------------------------------- C data builders ----
def pool_c(name, lines): return "POOL(%s,\n%s);\n" % (name, ",\n".join("    " + cs(t) for t in lines))
def scene_c(name, beats, comment=""):
    rows = []
    for who, bg, spr, txt in beats:
        rows.append("    {%s, %s, %d, %s}" % (who, bg, spr, "NULL" if txt is None else cs(txt)))
    return "SCENE(%s,%s\n%s\n);\n" % (name, ("   /* " + comment + " */") if comment else "", ",\n".join(rows))
def pairs_c(name, pairs, comment=""):
    beats = []
    for a, b in pairs: beats += [(M, "BG_ROM", 1, a), (S, "BG_ROM", 1, b)]
    return scene_c(name, beats, comment)
def wear_c(name, groups):
    beats = []
    for a, b, c in groups: beats += [(M, "BG_ROM", 1, a), (S, "BG_ROM", 1, b), (S, "BG_ROM", 1, c)]
    return scene_c(name, beats)
def talk_c(name, rows):
    body = ",\n".join("    {%s, %s, %s}" % (cs(a), cs(b), "NULL" if c is None else cs(c)) for a, b, c in rows)
    return "static const Talk %s_T[] = {\n%s\n};\nstatic TalkPool %s = {%s_T, NL(%s_T)};\n" % (name, body, name, name, name)

def replace_block(src, kind, name, new):
    pat = re.compile(r"^%s\(%s,.*?\n?\);\n" % (kind, re.escape(name)), re.S | re.M)
    m = pat.findall(src); assert len(m) == 1, (name, len(m))
    return pat.sub(lambda _: new, src, count=1)

# ---------------------------------------------------------------- lines.h ----
L = rd("src/lines.h")
for nm, lst in (("GO_COW", GO_COW), ("GL_COW", GL_COW), ("GE_COW", GE_COW), ("GO_COW_MILK", GO_COW_MILK), ("GL_COW_MILK", GL_COW_MILK), ("GE_COW_MILK", GE_COW_MILK),
                ("R_OUTFIT_COW", R_OUTFIT_COW), ("R_OUTFIT_COW_MM", R_OUTFIT_COW_MM)):
    L = replace_block(L, "POOL", nm, pool_c(nm, lst))
L = replace_block(L, "SCENE", "B_WEAR_COW", wear_c("B_WEAR_COW", WEAR))
L = replace_block(L, "SCENE", "B_WEAR_COW_MM", wear_c("B_WEAR_COW_MM", WEAR_MM))
new = "\n/* ---------- v12.1: COW MODE. While she wears the cow print outfit (married) her whole dialogue turns negative (disgust), but she still accepts every request.\n"
new += "   _CW = cow mode, _CW_MM = cow mode and you call her Mama. Chosen with g_cow(): no save flag, it ends when she changes out of the outfit. ---------- */\n"
for nm, lst in (("R_GREET_CW", R_GREET_CW), ("R_GREET_CW_MM", R_GREET_CW_MM), ("R_NIGHT_CW", R_NIGHT_CW), ("R_NIGHT_CW_MM", R_NIGHT_CW_MM),
                ("R_MORE_CW", R_MORE_CW), ("R_MORE_CW_MM", R_MORE_CW_MM), ("R_NOT_FULL_CW", R_NOT_FULL_CW), ("R_NOT_FULL_CW_MM", R_NOT_FULL_CW_MM),
                ("R_HUG_OPEN_CW", R_HUG_OPEN_CW), ("R_HUG_OPEN_CW_MM", R_HUG_OPEN_CW_MM), ("R_KISS_OPEN_CW", R_KISS_OPEN_CW), ("R_KISS_OPEN_CW_MM", R_KISS_OPEN_CW_MM),
                ("R_OUTFIT_UNCOW", R_OUTFIT_UNCOW), ("R_OUTFIT_UNCOW_MM", R_OUTFIT_UNCOW_MM)):
    new += pool_c(nm, lst)
for nm, sc in (("B_HUG_CW", HUG_CW), ("B_HUG_CW_MM", HUG_CW_MM), ("B_KISS_CW", KISS_CW), ("B_KISS_CW_MM", KISS_CW_MM), ("B_SLEEP_CW", SLEEP_CW), ("B_SLEEP_CW_MM", SLEEP_CW_MM),
               ("B_NIGHTM_CW", NIGHTM_CW), ("B_NIGHTM_CW_MM", NIGHTM_CW_MM), ("B_FEED_CW", feed(0, 0)), ("B_FEED_CW_LR", feed(1, 0)), ("B_FEED_CW_MM", feed(0, 1)), ("B_FEED_CW_MM_LR", feed(1, 1))):
    new += scene_c(nm, sc)
for nm, pr in (("B_OPEN_ASK_CW", ASK), ("B_OPEN_ASK_CW_MM", ASK_MM), ("B_OPEN_PILL_CW", PILL), ("B_OPEN_PILL_CW_MM", PILL_MM), ("B_OPEN_SUPER_CW", SUPER), ("B_OPEN_SUPER_CW_MM", SUPER_MM),
               ("B_MID_PUMP_CW", MID_PUMP), ("B_MID_PUMP_CW_MM", MID_PUMP_MM), ("B_MID_FEED_CW", MID_FEED), ("B_MID_FEED_CW_MM", MID_FEED_MM)):
    new += pairs_c(nm, pr)
marker = '#include "talk.h"   /* v7.7: CHAT and TEASE */'
assert L.count(marker) == 1
L = L.replace(marker, new + marker)
wr("src/lines.h", L)

# ---------------------------------------------------------------- talk.h ----
T = rd("src/talk.h")
add = "\n/* v12.1: cow mode CHAT (she wears the cow print outfit): one topic, she is disgusted but still talks to you */\n" + talk_c("C_COW", C_COW_T) + talk_c("C_COW_MM", C_COW_MM_T)
add += pool_c("C_FAIL_CW", C_FAIL_CW) + pool_c("C_FAIL_CW_MM", C_FAIL_CW_MM)
assert T.rstrip().endswith("#endif")
i = T.rstrip().rfind("#endif")
T = T[:i] + add.lstrip("\n") + "\n" + T[i:]
wr("src/talk.h", T)

# ---------------------------------------------------------------- extreg.h ----
def reg(file, name, kind, count, desc, lock=0, tail=1):
    if kind == "TK_POOL": return '    {%d, "%s", TK_POOL, TC_PAGE, (void *)&%s, &%s_N, 0, 0, 0, %s},\n' % (file, name, name, name, cs(desc))
    if kind == "TK_TALK": return '    {%d, "%s", TK_TALK, TC_PAGE, (void *)&%s, 0, 0, 0, 0, %s},\n' % (file, name, name, cs(desc))
    return '    {%d, "%s", TK_BEATS, TC_PAGE, (void *)&%s, &%s_N, %d, %d, %d, %s},\n' % (file, name, name, name, count, lock, tail, cs(desc))
R = ""
cw = "Cow mode (she wears the cow print outfit)"
cwm = "Cow mode + Mama (she wears the cow print outfit and you call her Mama)"
def both(base, kind, count, d, n1, n2, file=0, lock=0):
    return reg(file, base + "_CW", kind, n1, cw + " " + d, lock) + reg(file, base + "_CW_MM", kind, n2, cwm + " " + d, lock)
R += both("R_GREET", "TK_POOL", 0, "- her greeting once you are married (day and evening)", 0, 0)
R += both("R_NIGHT", "TK_POOL", 0, "- her greeting when you knock at night", 0, 0)
R += both("R_MORE", "TK_POOL", 0, "CHAT - her line when the chat is over and the menu comes back", 0, 0, file=1)
R += both("R_NOT_FULL", "TK_POOL", 0, "- her answer to BREASTFEED / PUMP MILK when the gauge is not full", 0, 0)
R += both("R_HUG_OPEN", "TK_POOL", 0, "HUG - the first beat of the hug scene, picked at random", 0, 0)
R += both("R_KISS_OPEN", "TK_POOL", 0, "KISS - the first beat of the kiss scene, picked at random", 0, 0)
R += both("C_FAIL", "TK_POOL", 0, "CHAT - she is not in the mood", 0, 0, file=1)
R += reg(1, "C_COW", "TK_TALK", 0, "Cow mode CHAT: the exchanges (one topic only; she is disgusted by the outfit but still talks)")
R += reg(1, "C_COW_MM", "TK_TALK", 0, "Cow mode + Mama CHAT: the exchanges (one topic only; you call her Mama)")
R += reg(0, "R_OUTFIT_UNCOW", "TK_POOL", 0, "CHANGE OUTFIT: her relieved line when she changes OUT of the cow print outfit")
R += reg(0, "R_OUTFIT_UNCOW_MM", "TK_POOL", 0, "Mama mode CHANGE OUTFIT: her relieved line when she changes OUT of the cow print outfit")
R += reg(0, "B_HUG_CW", "TK_BEATS", len(HUG_CW), cw + " HUG (page 4). Beat 1 is filled in from R_HUG_OPEN_CW; picture sc_hug_cow (BG HUG)", lock=1)
R += reg(0, "B_HUG_CW_MM", "TK_BEATS", len(HUG_CW_MM), cwm + " HUG (page 4). Beat 1 is filled in from R_HUG_OPEN_CW_MM", lock=1)
R += reg(0, "B_KISS_CW", "TK_BEATS", len(KISS_CW), cw + " KISS (page 4). Beat 1 is filled in from R_KISS_OPEN_CW", lock=1)
R += reg(0, "B_KISS_CW_MM", "TK_BEATS", len(KISS_CW_MM), cwm + " KISS (page 4). Beat 1 is filled in from R_KISS_OPEN_CW_MM", lock=1)
R += reg(0, "B_SLEEP_CW", "TK_BEATS", len(SLEEP_CW), cw + " SLEEP TOGETHER (page 4, night only)")
R += reg(0, "B_SLEEP_CW_MM", "TK_BEATS", len(SLEEP_CW_MM), cwm + " SLEEP TOGETHER (page 4, night only)")
R += reg(0, "B_NIGHTM_CW", "TK_BEATS", len(NIGHTM_CW), cw + ", married, at night: you knock, she hides behind the door (sprite 5), you go in")
R += reg(0, "B_NIGHTM_CW_MM", "TK_BEATS", len(NIGHTM_CW_MM), cwm + ", married, at night: you knock, she hides behind the door (sprite 5), you go in")
R += reg(0, "B_FEED_CW", "TK_BEATS", len(feed(0, 0)), cw + " BREASTFEED (page 4, needs a full gauge, bedroom and door). She is disgusted but says yes in beat 2")
R += reg(0, "B_FEED_CW_LR", "TK_BEATS", len(feed(1, 0)), cw + " BREASTFEED in the living room (the Here?! line comes before). She is disgusted but says yes")
R += reg(0, "B_FEED_CW_MM", "TK_BEATS", len(feed(0, 1)), cwm + " BREASTFEED (bedroom and door). She rejects the name and the outfit, then says yes")
R += reg(0, "B_FEED_CW_MM_LR", "TK_BEATS", len(feed(1, 1)), cwm + " BREASTFEED in the living room (the Here?! line comes before). She rejects the name and the outfit, then says yes")
for nm, pr, d in (("B_OPEN_ASK_CW", ASK, "ASK FOR MILK / PUMP MILK: the OPENER before the lead-in (door, bedroom and living room). Pairs of beats: your line (ME), then her disgusted answer that still agrees (SOPHIA)"),
                  ("B_OPEN_PILL_CW", PILL, "MILK PILL: the OPENER before the gift scene (and before the pump scene the pill starts). Pairs of beats: your line (ME), then her disgusted answer that still agrees (SOPHIA)"),
                  ("B_OPEN_SUPER_CW", SUPER, "SUPER DRUG: the OPENER before the gift scene. Pairs of beats: your line (ME), then her disgusted answer that still agrees (SOPHIA)"),
                  ("B_MID_PUMP_CW", MID_PUMP, "pump scene (every bottle scene): in the middle, while you face the wall, you speak and she answers. Pairs (ME, SOPHIA). One pair is added before the last beat of the scene"),
                  ("B_MID_FEED_CW", MID_FEED, "BREASTFEED: in the middle you speak and she answers. Pairs (ME, SOPHIA). One pair is added before your last line")):
    R += reg(0, nm, "TK_BEATS", len(pr) * 2, cw + " " + d)
    R += reg(0, nm + "_MM", "TK_BEATS", len(pr) * 2, cwm + " " + d.replace("(ME,", "(ME line with Mama,").replace("(ME)", "(ME, with Mama in it)"))
# the R_MORE and C_FAIL pools live in talk.h (file 1); the registry rows above were written with the right file number
E = rd("src/extreg.h")
end = "};\n#define TXT_N"
assert E.count(end) == 1
E = E.replace(end, R + end)
wr("src/extreg.h", E)

# ---------------------------------------------------------------- logic.h ----
G = rd("src/logic.h")
a = "static inline int g_outfit_open(const Game *g)"
assert G.count(a) == 1
G = G.replace(a, "static inline int g_cow(const Game *g) { return g_p4_open(g) && g->outfit == OF_COW; }   /* v12.1: COW MODE: she wears the cow print outfit (married): her dialogue turns negative, she still accepts. No save flag: it ends when she changes out of it */\n" + a)
wr("src/logic.h", G)
print("ok: v12.1 texts and registry written")
