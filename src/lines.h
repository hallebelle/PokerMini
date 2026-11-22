/* Poker Queen - her dialogue. Pure data.
   Table lines: two lines of at most 18 characters each, split with '|'.
   Scene pages (coach talk): split into pages with separate strings; each page wraps to 3 lines of 46 characters.
   The font is upper-case only, so write whatever is easiest to read; tests/test_logic.c checks the limits. */
#ifndef PQ_LINES_H
#define PQ_LINES_H

/* NL(array) = number of elements of a REAL array. It refuses pointers at compile time: the texts that the external files can replace (pools, scenes) are pointers
   with a count, use NAME_N for those. */
#define NL(a) ((int)(sizeof(a) / sizeof((a)[0])) + 0 * (int)sizeof(char[1 - 2 * __builtin_types_compatible_p(__typeof__(a), __typeof__(&(a)[0]))]))
#ifdef __GNUC__
#define PQ_UNUSED __attribute__((unused))
#else
#define PQ_UNUSED
#endif
/* v7.9: texts that lines.txt / talk.txt can replace (see extlines.h and extreg.h). POOL(NAME, "a", "b", ...) = the built-in list NAME_D plus a pointer NAME and a count NAME_N;
   SCENE(NAME, {...}, ...) the same for a story scene. Use NAME[i] and NAME_N exactly as before; never NL(NAME).
   Single texts and fixed-size lists are plain (writable) pointers; to make a NEW one editable, add it to the list in extreg.h. */
#define POOL(name, ...) \
    static const char *const name##_D[] = { __VA_ARGS__ }; \
    static const char *const *name PQ_UNUSED = name##_D; \
    static int name##_N PQ_UNUSED = (int)(sizeof(name##_D) / sizeof(name##_D[0]))
#define SCENE(name, ...) \
    static const Beat name##_D[] = { __VA_ARGS__ }; \
    static const Beat *name PQ_UNUSED = name##_D; \
    static int name##_N PQ_UNUSED = (int)(sizeof(name##_D) / sizeof(name##_D[0]))

/* ---- at the table ---- */
POOL(L_LEN, "HOW MUCH WILL|YOU BUY IN FOR?");
POOL(L_START, "LET'S SEE WHAT|YOU'VE GOT.", "DON'T LOSE|TOO MUCH, BOY.", "YOUR MONEY IS|ABOUT TO BE MINE.", "DEAL THE CARDS,|BOY.", "ANOTHER CHALLENGE|HOW CUTE.");
POOL(L_HAND, "YOUR MOVE,|BOY.", "I HOPE YOU'RE|READY.", "DON'T THINK|TOO HARD.", "SHALL WE?");
POOL(L_LAST, "LAST HAND BOY.|MAKE IT COUNT.",
    "LAST HAND.|DON'T CHOKE, BOY.",
    "ONE HAND LEFT.|PRAY, BOY.",
    "FINAL HAND.|SHOW ME.");
POOL(L_SHEWIN, "TOO EASY.", "THANKS FOR|THE CHIPS.", "ALL MINE.", "DID YOU THINK|YOU HAD ME?", "BETTER LUCK|NEXT HAND.");
POOL(L_YOUWIN, "LUCKY.|JUST LUCKY.", "ENJOY IT WHILE|IT LASTS.", "HMPH. FINE.", "BEGINNER'S LUCK.", "DON'T GET USED|TO IT.");
POOL(L_SHEFOLD, "FINE. TAKE IT.", "YOU GOT ME|THIS TIME.", "HMPH. NOT WORTH|IT.",
    "FINE. YOURS.|ENJOY IT.",
    "I FOLD. DON'T|LOOK SO SMUG.",
    "NOT WORTH THE|CHIPS. TAKE IT.");
POOL(L_YOUFOLD, "SMART. THAT WAS|MY HAND.", "RUNNING AWAY?", "WISE CHOICE,|BOY.",
    "GIVING UP?|HOW SAD.",
    "SCARED, BOY?",
    "YOU FOLD? THEN|THE POT IS MINE.");
POOL(L_SPLIT, "A SPLIT POT.|HOW ODD.", "A TIE? HOW|IRRITATING.",
    "A SPLIT.|SO BORING.",
    "EVEN? HOW|ANNOYING.",
    "A TIE. NOBODY|WINS. HOW DULL.");
POOL(L_HERRAISE, "THIS WILL COST|YOU.", "RAISE.", "YOU SURE YOU|WANT TO STAY?", "THE PRICE JUST|WENT UP.");
POOL(L_YOURAISE, "OH? YOU'RE BRAVE.", "LOOK AT YOU,|TRY TO RAISE.",
    "OH? YOU'RE|FEELING BOLD.",
    "A RAISE?|SHOW ME THEN.",
    "BRAVE. OR|JUST FOOLISH.");

/* ---- match end ---- */
POOL(L_ENDHER, "AS EXPECTED.|SEE YOU AGAIN.", "NOT EVEN|A SWEAT.", "THANKS FOR THE|CHIPS, BOY.",
    "I WIN. AGAIN.|SEE YOU SOON.",
    "CHEER UP, BOY.|YOU TRIED.",
    "TOO EASY.|PAY UP.");
POOL(L_ENDYOU, "...WELL PLAYED.|DON'T GLOAT.", "HMPH. TAKE IT.|I'LL WIN IT BACK.", "I LET YOU WIN.|REALLY.",
    "YOU WIN. DON'T|GET CONCEITED.",
    "CONGRATS. NOW|STOP SMILING.",
    "I'LL WIN IT|BACK, BOY.");
POOL(L_ENDTIE, "A TIE?|HOW IRRITATING.", "NEITHER OF US|WINS. AGAIN?",
    "EVEN. WE'LL|HAVE TO REMATCH.",
    "NO WINNER.|HOW DULL.");
POOL(L_ENDLOST, "NICE HAND. SHAME|YOU LOST ANYWAY.",
    "YOU LOST, BUT|YOU PLAYED WELL.",
    "BONUSES? CUTE.|STILL A LOSS.",
    "A LOSS WITH|STYLE. HMPH.");   /* you lost, but had earned card bonuses */


/* ---- her refusal scenes (scene pages) ---- */
/* challenged with no energy left: a single page, then back to the door menu */
POOL(S_ZOMBIE, 
    "You look like a zombie. I don't want a dead body end up in my room. Come back when you look like a human.",
    "Look at you, swaying on your feet. Go find a drink and some energy. Then knock again.",
    "You look too tired boy, Come back again later.");
/* challenged without the cash for even the cheapest game: a single page, then back to the door menu */
POOL(S_BROKE, 
    "No money, no game. Come back when you can pay for a seat at my table.",
    "A hundred is the cheapest seat at my table, and you can't even afford that. Go away.",
    "Broke? Then you're no use to me. Come back with some cash, boy.");

/* ============================ story scenes (visual-novel style) ============================
   A scene is a list of beats. Each beat shows one text box (max 3 lines of 46 chars) over a background,
   with or without her sprite. Changing the background between beats fades through black.
   Where a beat's text is NULL the game fills it in at run time (the line depends on how you played). */
enum { W_NAR, W_ME, W_UNK, W_AW, W_SOPHIA, W_HER };               /* who talks. W_HER = "Arrogant Woman" until she gives her name, then "Sophia" */
enum { BG_LOBBY, BG_MYROOM, BG_HALL, BG_LIVING, BG_TABLE, BG_EVENT, BG_EXHIB, BG_LOBBYMAP, BG_RAIN, BG_SROOM, BG_SLEEP, BG_CLOSE, BG_LONG, BG_TORSO, BG_ROM, BG_BDOOR, BG_PUMP, BG_HUG, BG_KISS, BG_SLEEPT, BG_FEED, BG_BOTTLE, BG_PLAYROOM, BG_IHUG, BG_BFEED, BG_BEDFEED, BG_SBATH, BG_SBPEEK, BG_STUB, BG_N };  /* backgrounds (BG_LOBBY = the painted lobby, BG_LOBBYMAP = the walking lobby map) */
enum { SCR_INTRO, SCR_BUMP, SCR_DOOR1, SCR_REPEAT, SCR_NAMED, SCR_LEAVE, SCR_GREET, SCR_COMMENT, SCR_RAIN, SCR_AFTER, SCR_NIGHT, SCR_BEDDOOR, SCR_BOSS, SCR_PROPOSE, SCR_LIVEYES, SCR_LIVENO, SCR_DEMAND, SCR_MOVEIN, SCR_DAILY, SCR_KNOCKHOME, SCR_NIGHTHOME, SCR_PILLGIVE, SCR_PILLDAY, SCR_PILL6, SCR_PILL7, SCR_KNOCKIN, SCR_MILK1, SCR_MILK2, SCR_PUMP, SCR_BOTTLE, SCR_GIFT, SCR_PEEKPIC, SCR_PUMPB, SCR_BOTTLEB, SCR_SCOLD, SCR_WEDDING, SCR_HUG, SCR_KISS, SCR_SLEEP, SCR_FEED, SCR_NIGHTM, SCR_CALLM, SCR_CALLB, SCR_OUTFIT, SCR_IHUG, SCR_BFEEDH, SCR_BFEEDL, SCR_BFEED, SCR_SBATH, SCR_BEDFEED, SCR_SBKNOCK, SCR_SBPEEK, SCR_SBPLEAD, SCR_SBTUB, SCR_SBSEND, SCR_SBNODAY, SCR_SBTIRED, SCR_ALLOW, SCR_FRIDGE, SCR_PUMPASK, SCR_N };
typedef struct { unsigned char who, bg, spr; const char *txt; } Beat;

SCENE(B_INTRO, 
    {W_NAR, BG_LOBBYMAP, 0, "The hotel quiet nice. different from the outside, i don't see any people though, i hope i have enough money to afford the room."},
    {W_ME,  BG_LOBBYMAP, 0, "I will go to the front desk, and asking for a room, hope the rain will stop tommorow."}
);
/* part 2: you speak to the receptionist at the desk (you press A on it) and bump into her instead; ends with you in front of the desk, free to walk */
SCENE(B_BUMP, 
    {W_NAR, BG_LOBBY, 0, "After I speak to the receptionist, I turn around and bump into someone."},
    {W_UNK, BG_LOBBY, 0, "Do you have eyes?"},
    {W_ME,  BG_LOBBY, 0, "Sorry! I didn't see you there..."},
    {W_AW,  BG_LOBBY, 1, "Clearly. Some people never look where they're walking."},
    {W_ME,  BG_LOBBY, 1, "I said I'm sorry. Are you okay?"},
    {W_AW,  BG_LOBBY, 1, "I'm fine. No thanks to you."},
    {W_AW,  BG_LOBBY, 1, "...Wait. What is this?"},
    {W_NAR, BG_LOBBY, 1, "My wet coat has left a dark stain across her shirt."},
    {W_AW,  BG_LOBBY, 1, "My shirt! Do you have any idea what this cost?"},
    {W_ME,  BG_LOBBY, 1, "I'm so sorry. How much is it?"},
    {W_AW,  BG_LOBBY, 1, "Three hundred."},
    {W_ME,  BG_LOBBY, 1, "Three hundred?! I only have a hundred on me..."},   /* (START_CASH) */
    {W_AW,  BG_LOBBY, 1, "A hundred. ...That's all you have?"},
    {W_NAR, BG_LOBBY, 1, "She looks me over, from my soaked coat to my worn-out shoes."},
    {W_AW,  BG_LOBBY, 1, "You walk into a hotel like this with a hundred in your pocket. How poor can one person be?"},
    {W_ME,  BG_LOBBY, 1, "I only needed a room until the weather clears..."},
    {W_AW,  BG_LOBBY, 1, "A hundred wouldn't even pay for the sleeve. Pathetic."},
    {W_AW,  BG_LOBBY, 1, "I don't care how you do it. Work, beg, borrow. Try whatever you can to get me my money."},
    {W_ME,  BG_LOBBY, 1, "Where am I supposed to find three hundred?"},
    {W_AW,  BG_LOBBY, 1, "...Urgh. If you're desperate enough, there's an exhibition in the Event Center. A Poker match. Go win some money."},
    {W_AW,  BG_LOBBY, 1, "Come to my room on the third floor when you have the three hundred. Until then, don't knock on my door."},
    {W_ME,  BG_LOBBY, 1, "The third floor? How am I supposed to get up there?"},
    {W_NAR, BG_LOBBY, 1, "She pulls a card from her pocket and drops it into my hand without looking at me."},
    {W_AW,  BG_LOBBY, 1, "My floor key card. The elevator won't stop on my floor without it. Don't lose it, and don't get ideas."},
    {W_NAR, BG_LOBBY, 0, "A floor key card for the third floor. Then she walks off without looking back."},
    {W_NAR, BG_LOBBY, 0, "Behind me the glass doors rattle. A storm has rolled in out of nowhere. Nobody is leaving in this weather."},
    {W_ME,  BG_LOBBY, 0, "What an arrogant woman. Three hundred... I'd better find a way to earn it. First, my room."}
);
/* you knock with the $300: she takes it, then takes her floor key card back (the 3rd floor is closed to you now); the scene ends with you on the 2nd floor in front of the elevator */
SCENE(B_DOOR1, 
    {W_NAR, BG_HALL, 0, "I have the $300. I take a breath and knock on her door."},
    {W_AW,  BG_HALL, 1, "You... ...Do you have my shirt money?"},
    {W_ME,  BG_HALL, 1, "Three hundred, as promised. I'm sorry about your shirt."},
    {W_NAR, BG_HALL, 1, "I hand her the $300. She counts it swiftly."},
    {W_AW,  BG_HALL, 1, "You actually earned it. I thought you will never got it."},
    {W_AW,  BG_HALL, 1, "Good. Now give me back my floor key card. We are done, so you have no business on my floor."},
    {W_NAR, BG_HALL, 1, "She holds out her hand and waits until I put the floor key card in it."},
    {W_AW,  BG_HALL, 1, "Don't come looking for me again. People like you don't belong on the third floor."},
    {W_NAR, BG_HALL, 0, "The door closes. The elevator carries me back down to the second floor."}
);
/* you left the first match: she greets you with a coward line (text filled in), then it is ten hands again */
SCENE(B_REPEAT, 
    {W_NAR, BG_HALL, 0, "I knock on her door."},
    {W_AW,  BG_HALL, 1, NULL},     /* the greeting (a coward line after a walk-out) */
    {W_AW,  BG_HALL, 1, "Good. You can pay for a seat. If you win, I pay you the chips you took off me. The bigger your buy-in, the more each chip is worth."},   /* from here on: the first time only */
    {W_AW,  BG_HALL, 1, "Win the match holding a straight or better and I'll pay a bonus on top. If you ever get one. But I never go back on my word."},
    {W_AW,  BG_HALL, 1, "And don't walk out halfway. Quit the table, or let that thing switch off, and it counts as a loss."},
    {W_AW,  BG_HALL, 1, "Now tell me how much you want to lose."}
);
#define REPEAT_SHORT 2   /* the knock and the greeting; the rules follow only the first time */
/* knocking at night: she is in her night clothes (sprite 3) and furious */
SCENE(B_NIGHT, 
    {W_NAR, BG_HALL, 0, "It's the middle of the night, but I knock on her door anyway."},
    {W_HER, BG_HALL, 3, "Do you have ANY idea what time it is?!"},
    {W_HER, BG_HALL, 3, "Pounding on my door in the middle of the night. Are you stupid, or do you just have no manners at all?"},
    {W_HER, BG_HALL, 3, "Do you know i'm about to sleep. Come back in the morning. If you have business."},
    {W_HER, BG_HALL, 3, "Go back to your room before I call the front desk. And if you knock like that again, you'll regret it."},
    {W_NAR, BG_HALL, 0, "The door slams shut. I decide it's wiser to walk away."}
);
/* opening her bedroom door at night: your narration first (placeholder picture = her living room; a bedroom-door picture comes later), then the sleeping picture */
SCENE(B_BEDDOOR, 
    {W_NAR, BG_LIVING, 0, "I slowly open her bedroom door and try to not make any sound."}
);
SCENE(B_NAMED, 
    {W_AW,     BG_LIVING, 1, NULL},     /* depends on how the match went */
    {W_ME,     BG_LIVING, 1, "That's a good match. I learned a lot. What's your name?."},
    {W_SOPHIA, BG_LIVING, 1, "You don't know who i am? I'm Sophia the Poker Queen."},
    {W_ME,     BG_LIVING, 1, "Wait, You the one who won every single tournament without lose."},
    {W_SOPHIA, BG_LIVING, 1, "Don't flatter me boy."},
    {W_ME,     BG_LIVING, 1, "Can I challenge you again?"},
    {W_SOPHIA, BG_LIVING, 1, "If you have the money. I will enjoyed seeing you lose boy."},
    {W_SOPHIA, BG_LIVING, 1, "If you want to play against me again, better prepare the money."}
);
/* you beat her at the Exhibition ($5000 table): she gives her name and invites you to her room; the floor key card comes back (sprite 4 = her boss outfit, a placeholder picture for now) */
SCENE(B_BOSS, 
    {W_AW,     BG_EXHIB, 4, "...Hmph. I didn't expect that. You actually beat me."},
    {W_ME,     BG_EXHIB, 4, "That was a good match. I learned a lot. What's your name?"},
    {W_SOPHIA, BG_EXHIB, 4, "You don't know who I am? I'm Sophia the Poker Queen."},
    {W_ME,     BG_EXHIB, 4, "Wait. You're the one who won every single tournament without a loss?"},
    {W_SOPHIA, BG_EXHIB, 4, "Don't flatter me, boy. Ten thousand gone in one night. Nobody has done that to me before."},
    {W_SOPHIA, BG_EXHIB, 4, "You interest me. Come to my room on the third floor. We can play there, whenever you like."},
    {W_NAR,    BG_EXHIB, 4, "She pulls a floor key card from her pocket and presses it into my hand."},
    {W_SOPHIA, BG_EXHIB, 4, "Don't lose it again. And bring money, boy. I enjoy watching you lose."}
);
SCENE(B_LEAVE, {W_HER, BG_HALL, 0, NULL});    /* she calls after you */
SCENE(B_GREET, {W_NAR, BG_HALL, 0, "Knock, knock."}, {W_HER, BG_HALL, 1, NULL});

/* v6.4: she plays you once a day: the knock first, then her refusal (the line is filled in) */
SCENE(B_DAILY, {W_NAR, BG_HALL, 0, "Knock, knock."}, {W_HER, BG_HALL, 1, NULL});
/* v6.4: her bedroom door at home: the knock (morning / evening), then her menu (or, on day 8 of the pill, the body-change scene) */
SCENE(B_KNOCKHOME, {W_NAR, BG_BDOOR, 0, "Knock, knock. I knock on her bedroom door and wait."});
SCENE(B_NIGHTHOME, 
    {W_NAR,    BG_BDOOR, 0, "It's the middle of the night, but I knock on her bedroom door anyway."},
    {W_SOPHIA, BG_BDOOR, 3, "Do you have ANY idea what time it is?!"},
    {W_SOPHIA, BG_BDOOR, 3, "I am trying to sleep. Knocking on my door in the middle of the night, are you stupid?"},
    {W_SOPHIA, BG_BDOOR, 3, "Come back in the morning. If you have something to say, you can say it then."},
    {W_SOPHIA, BG_BDOOR, 3, "Now go to bed. And if you knock like that again, you'll regret it."},
    {W_NAR,    BG_BDOOR, 0, "The door slams shut. I decide it's wiser to leave her alone."}
);
/* giving her the pill (her picture is still the normal one: the pill needs 7 days) */
SCENE(B_PILLGIVE, 
    {W_NAR,    BG_BDOOR, 0, "I take the small bottle out of my pocket and hold it out to her."},
    {W_SOPHIA, BG_BDOOR, 1, "...What is this pill?"},
    {W_ME,     BG_BDOOR, 1, "It's a beauty supplement."},
    {W_SOPHIA, BG_BDOOR, 1, "A beauty supplement? You're know i'll only take the expensive one only?"},
    {W_ME,     BG_BDOOR, 1, "This is the best one. It's quite expensive."},
    {W_SOPHIA, BG_BDOOR, 1, "Hmph. really? If this is some kind of cheap crap, you will regret it."},
    {W_SOPHIA, BG_BDOOR, 1, "...Fine. I'll take it. Only because you went to the trouble."},
    {W_NAR,    BG_BDOOR, 0, "She slips the bottle into her pocket. Now all I can do is wait and see."}
);
/* the first knock on day 8 after the pill: her picture is already the changed one. YOU ask first, then she pulls you inside (from now on you knock and step into her room) */
SCENE(B_PILLDAY, 
    {W_NAR,    BG_BDOOR, 0, "The door opens only a little. I can't see her, only hear her."},
    {W_ME,     BG_BDOOR, 0, "Sophia? It's me. You haven't come out for days. Are you alright in there?"},
    {W_SOPHIA, BG_BDOOR, 0, "...Don't open the door, stay there."},
    {W_NAR,    BG_BDOOR, 5, "Sophia hides behind the door, only her face peeking out."},
    {W_ME,     BG_BDOOR, 5, "I'm not going inside. Please, tell me what's wrong."},
    {W_SOPHIA, BG_BDOOR, 5, "Something is wrong with my body, that's what. That pill you gave me... what kind of pill is it?."},
    {W_ME,     BG_BDOOR, 5, "Do you have allergy?"},
    {W_SOPHIA, BG_BDOOR, 5, "My chest, my hips... somehow it's grow. All my clothes are too tight."},
    {W_ME,     BG_BDOOR, 5, "Is it a bad change? I mean... you look..."},
    {W_SOPHIA, BG_BDOOR, 5, "Don't you dare finish that sentence! I can't go outside with a body like this. Everyone would stare at me."},
    {W_ME,     BG_BDOOR, 5, "Then stay here. I'll take care of you."},
    {W_SOPHIA, BG_BDOOR, 5, "Hmph. You'd better. This is all your fault, boy."},
    {W_SOPHIA, BG_BDOOR, 5, "And my chest is so heavy and stuffy that it hurts. I don't know what to do about it."},
    {W_ME,     BG_BDOOR, 5, "Let me help. I'll find something that eases it. I promise. Can i come inside now?"},
    {W_SOPHIA, BG_BDOOR, 5, "...Fine. But not a word about this to anyone. Do you understand?"},
    {W_NAR,    BG_BDOOR, 5, "She grabs my sleeve, pulls me inside and shuts the door behind us."},
    {W_NAR,    BG_ROM, 0, "Her breast grow a lot bigger, that pill really work."}
);
/* after day 8 you knock and she lets you in: the visual novel plays inside her room (BG_ROM) */
SCENE(B_KNOCKIN, 
    {W_NAR,    BG_BDOOR, 0, "Knock, knock. I knock on her bedroom door and wait."},
    {W_SOPHIA, BG_BDOOR, 0, "...Who is it? Wait a moment, don't open it yet."},
    {W_SOPHIA, BG_BDOOR, 5, "...Come in. Quickly, and close the door behind you."},
    {W_NAR,    BG_ROM,   0, "I slip inside and close the door behind me. Her room smells faintly of her perfume."}
);
/* v8.7: married, at night: you knock softly, she hides behind the door, then you are in her room (page 4 of her menu follows) */
SCENE(B_NIGHTM, 
    {W_NAR,    BG_BDOOR, 0, "It's late at night. I knock softly on our bedroom door."},
    {W_SOPHIA, BG_BDOOR, 0, "...Who is it? Do you know what time it is?"},
    {W_SOPHIA, BG_BDOOR, 5, "...It's you. Come in, husband. Quickly, and close the door behind you."},
    {W_NAR,    BG_ROM,   0, "I slip inside and close the door behind me. The lamp is low and her room is warm."}
);
/* days 6 and 7 after the pill: she complains when you visit (these replace her greeting) */
POOL(R_PILL6, "My chest feels so stuffy lately. It's hard to breathe. Must be the pills.", "Why does my chest feel so stuffy? Don't stare, boy. It's your doing.",
    "My chest has felt so tight since yesterday. It's your pill, isn't it, boy?",
    "Something's wrong with my chest. It's heavy. Don't you dare laugh, boy.");
POOL(R_PILL7, "My clothes feel even tighter than yesterday. At this rate, none my cloth will fit", "My clothes feel so tight. And my chest is still stuffy. Something is strange.",
    "My chest feels heavier every day. ...Is this normal? Don't stare, boy.",
    "I can barely breathe in this shirt. It's your doing. Stop looking.");
/* day 6 after the pill: you ask first. Played once, on the first knock of that day */
SCENE(B_PILL6, 
    {W_NAR,    BG_BDOOR, 0, "The door opens just a crack. Sophia looks restless, one hand resting on her chest."},
    {W_ME,     BG_BDOOR, 0, "Sophia, are you okay? You look a little pale today."},
    {W_SOPHIA, BG_BDOOR, 0, "I'm fine. It's only... my chest feels so stuffy lately. It's hard to breathe."},
    {W_ME,     BG_BDOOR, 0, "Maybe you caught a cold in this weather. Should I get you some medicine?"},
    {W_SOPHIA, BG_BDOOR, 0, "I don't need medicine. I will be fine. It must be this stupid rain."},
    {W_SOPHIA, BG_BDOOR, 0, "...Why are you looking at me like that? Stop staring, boy."},
    {W_ME,     BG_BDOOR, 0, "Sorry. I'm just worried about you. Can i come in"},
    {W_SOPHIA, BG_BDOOR, 0, "Hmph. Worry about your own self. ...But thank you for asking."},
    {W_NAR,    BG_BDOOR, 0, "She hesitates for a moment, then opens the door a little wider."},
    {W_SOPHIA, BG_BDOOR, 0, "...Come in. Quickly, and close the door behind you."},
    {W_NAR,    BG_ROM,   0, "I slip inside and close the door behind me. Her room smells faintly of her perfume."}
);
/* day 7 after the pill */
SCENE(B_PILL7, 
    {W_NAR,    BG_BDOOR, 0, "Sophia opens the door slowly. She keeps tugging at the collar of her blouse."},
    {W_ME,     BG_BDOOR, 0, "Hi, Sophia. How are you feeling today?"},
    {W_SOPHIA, BG_BDOOR, 0, "Worse. All My clothes feel so tight. Even my favorite blouse doesn't fit anymore."},
    {W_ME,     BG_BDOOR, 0, "Did the hotel laundry shrink them again?"},
    {W_SOPHIA, BG_BDOOR, 0, "That's what I thought. But my skirt fits and my blouse doesn't. And my chest feels stuffy."},
    {W_SOPHIA, BG_BDOOR, 0, "I feel hot and strange all day long. Something is wrong with me, boy."},
    {W_ME,     BG_BDOOR, 0, "Maybe you should see a doctor. I can go with you."},
    {W_SOPHIA, BG_BDOOR, 0, "A doctor?! And let strangers stare at me? Absolutely not."},
    {W_SOPHIA, BG_BDOOR, 0, "If this keeps getting worse, you're the first one I'll blame. That pill, I mean... Don't pretend you don't know."},
    {W_ME,     BG_BDOOR, 0, "It was only a supplement. It should be harmless."},
    {W_SOPHIA, BG_BDOOR, 0, "It had better be."},
    {W_NAR,    BG_BDOOR, 0, "She sighs and steps back from the door."},
    {W_SOPHIA, BG_BDOOR, 0, "...Come in. Quickly, and close the door behind you."},
    {W_NAR,    BG_ROM,   0, "I slip inside and close the door behind me. Her room smells faintly of her perfume."}
);
/* after day 8 her greeting in her room: she is shy and short-tempered about her body */
POOL(R_GREET_P, 
    "Hmph. You came. Don't stand there, close the door.",
    "...Don't look at my chest. Look at my face. My face!",
    "Everything is still too tight. I hate this, boy.",
    "I haven't been outside for days. You'd better be worth it.",
    "Don't just stare. Say what you want.",
    "My chest feels so heavy today. Don't you dare look at my chest.",
    "...At least you knock first. What you want?.",
    "Make it quick. I'm not in the mood today.");
/* the milk inducer pill. First pill: you give it, she complains, then you wait 2 days */
SCENE(B_MILK1, 
    {W_NAR,    BG_ROM, 0, "I take out the small bottle of pills and hold it out to her."},
    {W_SOPHIA, BG_ROM, 1, "...What is this thing now? Another one of your strange pills?"},
    {W_ME,     BG_ROM, 1, "It's a swell relieve pill. It might ease the pressure in your chest."},
    {W_SOPHIA, BG_ROM, 1, "Really?! Do you thing it's going to work, boy?!"},
    {W_ME,     BG_ROM, 1, "Maybe, It's quite expansive! The pharmacy said it's good for your condition. This should help it settle."},
    {W_SOPHIA, BG_ROM, 1, "...You'd better not be making fun of me. My chest hurts and it's all your fault."},
    {W_ME,     BG_ROM, 1, "Take one, then wait two days. If it works, you will feel it."},
    {W_SOPHIA, BG_ROM, 1, "Two days. Hmph. Fine, give it here. And don't you dare tell anyone."},
    {W_NAR,    BG_ROM, 0, "She snatches the bottle and swallows one pill right away. Now all I can do is wait two days."}
);
/* two days later, the first knock: you ask first, she is worse, the pump is the answer */
SCENE(B_MILK2, 
    {W_NAR,    BG_ROM, 0, "Sophia is sitting on the edge of her bed with her arms wrapped around her chest."},
    {W_ME,     BG_ROM, 1, "Sophia? Did the pill ease you a little. How are you feeling?"},
    {W_SOPHIA, BG_ROM, 1, "Worse! My chest feels even more stuffy than before. It's heavy and it aches."},
    {W_ME,     BG_ROM, 1, "So the pill didn't work?"},
    {W_SOPHIA, BG_ROM, 1, "How would I know?! It feels like it's getting bigger and heavier, and I can't do anything about it."},
    /* v6.7: [5] and [6] play only when you already own the pump (the pump scene follows by itself); otherwise [7] replaces them and the pump objective shows */
    {W_ME,     BG_ROM, 1, "I thought this might happen, so I already bought something for you."},
    {W_SOPHIA, BG_ROM, 1, "...What is it? I don't want another dumb thing."},
    {W_NAR,    BG_ROM, 0, "The pills really work. She needs something to relieve the pressure. A breast pump from the online store should do it."}
);
#define MILK2_OWNED 7
/* giving her the pump (v6.7: it plays by itself when you own the pump). Part 1: she hesitates, then the choice: FACE THE WALL / LOOK AT HER */
SCENE(B_PUMP, 
    {W_NAR,    BG_ROM, 0, "I hand her the box with the breast pump. Sophia stares at it, her face turning red."},
    {W_NAR,    BG_ROM, 0, "I give her a clean baby bottle too, so she has something to pour the milk into."},
    {W_SOPHIA, BG_ROM, 1, "...This. This is breast pump?"},
    {W_ME,     BG_ROM, 1, "There maybe some air trapped inside your chest. Use it to pump it out."},
    {W_SOPHIA, BG_ROM, 1, "Hmph. Turn around. And don't you dare peek."},
    {W_ME,     BG_ROM, 1, "I won't. I don't even know how that thing works."},
    {W_SOPHIA, BG_ROM, 1, "...Wait. Not like that. You're still half turned. Face the wall properly!"},
    {W_SOPHIA, BG_ROM, 1, "It's... so embarrassing. Why do I have to do something like this..."},
    {W_ME,     BG_ROM, 1, "Just follow the instructions on the box. I'm not looking."},
    {W_SOPHIA, BG_ROM, 1, "I... I don't know if I can do this. it's doesn't feels right? What if it doesn't even work?"},
    {W_ME,     BG_ROM, 1, "Then we stop. Take your time."},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. Fine. But the moment you turn around, you're dead. Don't look!"},
    {W_NAR,    BG_ROM, 0, "I turn to the wall. Behind me fabric rustles, and then there is only the sound of her nervous breathing."}
);
/* LOOK AT HER (the same for the pump and for every bottle): her picture, then you turn back */
SCENE(B_PEEKPIC, 
    {W_NAR,    BG_PUMP, 0, "I can't help it. I turn my head just a little and glance back over my shoulder."},
    {W_NAR,    BG_PUMP, 0, "Sophia sits on the edge of her bed, her eyes fixed on the bottle and her cheeks bright red."},
    {W_SOPHIA, BG_PUMP, 0, "...Almost... just a little more..."},
    {W_NAR,    BG_PUMP, 0, "She hasn't noticed me yet. Before she does, I quickly turn back to the wall."}
);
/* part 2 of the pump scene: the bottle is full (if you peeked, B_SCOLD is added at the end) */
SCENE(B_PUMPB, 
    {W_NAR,    BG_ROM, 0, "A long while later, Sophia taps my shoulder, holding the pump and the baby bottle, now filled with her fresh milk."},
    {W_SOPHIA, BG_ROM, 1, "...This cannot be. Not air but...milk. It filled the bottle you gave me."},
    {W_ME,     BG_ROM, 1, "What do you want me to do? You need to take the pills to fully cure it."},
    {W_SOPHIA, BG_ROM, 1, "What? I need to keep eating the pill!..For now dispose of this. And don't say anything, pretend you didn't see it."},
    {W_ME,     BG_ROM, 1, "Okay..Give it to me, I dispose it for you"},
    {W_SOPHIA, BG_ROM, 1, "Make sure you disposed it properly boy"},
    {W_NAR,    BG_ROM, 0, "I got Sophia breast milk. I will keep in fridge so it's stay fresh."}
);
/* she scolds you for peeking (added after she hands over the bottle, pump and bottle scenes alike) */
SCENE(B_SCOLD, 
    {W_SOPHIA, BG_ROM, 1, "...Wait. I felt someone staring at me the whole time. You peeked, didn't you?!"},
    {W_ME,     BG_ROM, 1, "I-I didn't!"},
    {W_SOPHIA, BG_ROM, 1, "Don't lie to me! Your ears are red! Do that again and I'll lock this door for good, you pervert!"},
    {W_ME,     BG_ROM, 1, "S-sorry! I'll throw it away right now!"}
);
/* every milk pill after that: +50% on the bottle gauge, she says one of these */
POOL(R_MILK_OK, 
    "Another one? ...Fine. Hand it over.",
    "Hmph. My chest already feels heavier.",
    "...But it will make the milk come out again..",
    "Is this really going to work?.",
    "Just this one more. This is all your fault.",
    "You better make sure i'm getting better after this.",
    "...It tastes awful. And i have to keep taking this.");
/* v6.8: every time the gauge is full one of these four-beat lead-ins plays first (random, never the same twice in a row), then B_BOTTLE */
#define LEAD_N 6
#define LEAD_BEATS 4
static const Beat B_LEAD_D[LEAD_N][LEAD_BEATS] = {
    {   {W_NAR,    BG_ROM, 0, "After the last pill, Sophia goes quiet and presses her arms tightly against her chest."},
        {W_SOPHIA, BG_ROM, 1, "...It's happening again. It feels so full and heavy that it aches. I can't stand it, boy."},
        {W_ME,     BG_ROM, 1, "That means the pills are working. You'll feel better once you let it out."},
        {W_SOPHIA, BG_ROM, 1, "Don't say it like it's nothing! Hmph... Fine. Just don't you dare look at me."} },
    {   {W_NAR,    BG_ROM, 0, "Sophia fidgets with her collar and keeps shifting her weight, her cheeks turning pink."},
        {W_SOPHIA, BG_ROM, 1, "My blouse is so tight again. It's hot and heavy, and I can't think about anything else."},
        {W_ME,     BG_ROM, 1, "Then it's time to use the pump again, isn't it? It helped last time."},
        {W_SOPHIA, BG_ROM, 1, "I know that already! ...Just keep your eyes on the wall this time."} },
    {   {W_NAR,    BG_ROM, 0, "Sophia lets out a shaky breath and sits on the edge of her bed, hugging a pillow to her chest."},
        {W_SOPHIA, BG_ROM, 1, "It feel strange, boy. It feels like it's about to burst. This is all because of your stupid pills."},
        {W_ME,     BG_ROM, 1, "I'll stay right here. Tell me what I can do."},
        {W_SOPHIA, BG_ROM, 1, "...Just turn around and stay quiet. And no peeking."} },
    {   {W_NAR,    BG_ROM, 0, "She picks up the pump from her nightstand. Her ears are bright red."},
        {W_SOPHIA, BG_ROM, 1, "What with the pump?, No.. i will not do it."},
        {W_ME,     BG_ROM, 1, "Sorry. I want you to feel better"},
        {W_SOPHIA, BG_ROM, 1, "Fine, give the pump to me. Now, turn around face the wall."} },
    {   {W_NAR,    BG_ROM, 0, "Sophia swallows the pill and pulls a face, one hand pressed against her chest."},
        {W_SOPHIA, BG_ROM, 1, "Why is it always worse after this... My chest feels like it's going to explode."},
        {W_ME,     BG_ROM, 1, "Does it still hurt that much?"},
        {W_SOPHIA, BG_ROM, 1, "Of course it does! Hmph. At least the pump makes it better. ...Turn around."} },
    {   {W_NAR,    BG_ROM, 0, "Sophia takes a deep breath and tugs at her clothes, glancing at the door as if someone could be listening."},
        {W_SOPHIA, BG_ROM, 1, "Make sure the door is closed. If anyone found out about this, I would never leave this room again."},
        {W_ME,     BG_ROM, 1, "Nobody will know. There's only our room at this floor."},
        {W_SOPHIA, BG_ROM, 1, "You'd never know. ...Now, I need to take care of this. Don't look."} }
};
static const Beat (*B_LEAD)[LEAD_BEATS] PQ_UNUSED = B_LEAD_D;   /* v7.9: the lead-ins; lines.txt may change their number (each one still has LEAD_BEATS beats) */
static int B_LEAD_N PQ_UNUSED = LEAD_N;
/* the second pill: the gauge is full. Part 1: she uses the pump again (you may peek, same choice as the first time) */
SCENE(B_BOTTLE, 
    {W_NAR,    BG_ROM, 0, "Sophia takes out the breast pump and the baby bottle you gave her. Her ears are already red."},
    {W_SOPHIA, BG_ROM, 1, "...Turn around. Again. And this time don't even think about looking."},
    {W_ME,     BG_ROM, 1, "I'm already facing the wall."},
    {W_SOPHIA, BG_ROM, 1, "I still hate this. Why does it have to be me who... Hmph. Just don't turn around!"},
    {W_NAR,    BG_ROM, 0, "I face the wall again. Behind me I hear the soft hum of the pump and her unsteady breathing."}
);
/* part 2: she hands you the Baby Milk Bottle (if you peeked, B_SCOLD is added before the last line) */
SCENE(B_BOTTLEB, 
    {W_NAR,    BG_ROM, 0, "A while later, Sophia stands next to me, holding a baby bottle filled to the brim."},
    {W_SOPHIA, BG_ROM, 1, "...Here. It's full. Take it before I change my mind. Can you find anything else other than a baby bottle."},
    {W_ME,     BG_ROM, 1, "That is the only bottle I had around."},
    {W_SOPHIA, BG_ROM, 1, "Don't look at it like that! It's not for drink. go disposed it."},
    {W_ME,     BG_ROM, 1, "Yes, i go throw it right away."},
    {W_SOPHIA, BG_ROM, 1, "Make sure you clean the bottle."},
    {W_NAR,    BG_ROM, 0, "I got Sophia breast milk. Such a fool will throw this away. Her sweet breast milk is now mine."}
);
POOL(R_MILK_WAIT, "Give the first one time to work. Come back in a day or two.", "It takes time. Be patient and come back in a day or two.");
POOL(R_MILK_PUMP, "It's not enough on its own. I need something to relieve this first. Find a pump.", "I need a way to relieve it first. Get a pump.");
POOL(R_PUMP_NOW, "What is that supposed to be? I don't need it.", "That? No. Not now. I don't need it.", "Put that away. I don't need it right now.");
POOL(R_BOTTLE_FULL, "Your pockets are full of bottles already.", "You can't carry any more bottles. Use some first.", "You have enough bottles. Don't be greedy.");

/* ---------- v8.6: the wedding and page 4 of her menu (hug, kiss, sleep together, breastfeed) ----------
   The four page-4 scenes use their own pictures, one per outfit: assets/sc_hug_d8.png / sc_hug_yoga.png, sc_kiss_*, sc_sleep_*, sc_feed_* (d8 = her outfit after the day-8 scene).
   Beat 0 of B_HUG and B_KISS is filled in by the game from R_HUG_OPEN / R_KISS_OPEN. */
SCENE(B_WEDDING, 
    {W_NAR,    BG_ROM, 0, "I take the small velvet box out of my pocket. Inside, the diamond ring catches the light."},
    {W_SOPHIA, BG_ROM, 1, "...What is that? That sparkle... boy, do you know how much that thing must cost?"},
    {W_ME,     BG_ROM, 1, "Every cent of it. I want you to have it. Sophia, will you marry me?"},
    {W_SOPHIA, BG_ROM, 1, "M-marry?! Just like that, in my room, with that stupid face?!"},
    {W_ME,     BG_ROM, 1, "Yes. Right here, right now. I mean every word."},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. You really are an idiot. A stubborn, reckless, ridiculous idiot."},
    {W_SOPHIA, BG_ROM, 1, "Fine. Put it on me. If you drop it, the wedding is off."},
    {W_NAR,    BG_ROM, 0, "My hands shake as I slide the ring onto her finger. It fits perfectly, and she stares at it for a long time."},
    {W_SOPHIA, BG_ROM, 1, "...Now you are my husband, so you had better take responsibility. Don't you dare make me regret this."},
    {W_NAR,    BG_ROM, 0, "Sophia is my wife now. My heart is still racing as I look at the ring on her finger."}
);
POOL(R_HUG_OPEN, 
    "I step closer and open my arms. Sophia huffs, but she doesn't pull away.",
    "Sophia is standing still, so I wrap my arms around her from behind.",
    "I pull her gently into a hug. She stiffens for one second, then relaxes against me.",
    "I spread my arms and Sophia sighs, but she steps into them anyway.",
    "I walk up to her and just hold her. Sophia goes still, then leans in.",
    "She turns around and I pull her close. Her arms creep around me slowly.",
    "I hug her without a word. Sophia grumbles, but her hands grip my back.",
    "I wrap her up from the side. She says nothing, but she doesn't leave.");
POOL(R_KISS_OPEN, 
    "I touch her chin gently and lean in. Sophia's eyes widen, then slowly close.",
    "She looks up at me and I can't hold back any longer. I kiss her softly.",
    "I brush her hair behind her ear and kiss her before she can complain.",
    "I cup her cheek and kiss her. Sophia gasps softly, then kisses back.",
    "She meets my eyes and her breath catches. I lean in and kiss her slowly.",
    "Sophia pretends to look away, but she lifts her face to me.",
    "I brush her hair back and kiss her forehead, then her lips.",
    "She starts to say something, so I kiss her before she can.");
SCENE(B_HUG, 
    {W_NAR,    BG_HUG, 0, NULL},
    {W_SOPHIA, BG_HUG, 0, "...Hmph. Only because nobody is watching. Don't get used to it."},
    {W_ME,     BG_HUG, 0, "You're warm. I could stay like this all day."},
    {W_NAR,    BG_HUG, 0, "She rests her head on my shoulder and her fingers curl into my shirt. For a while neither of us says anything."}
);
SCENE(B_KISS, 
    {W_NAR,    BG_KISS, 0, NULL},
    {W_SOPHIA, BG_KISS, 0, "...W-who said you could do that so suddenly?! My heart is going to burst."},
    {W_ME,     BG_KISS, 0, "I'm your husband. I don't need to ask."},
    {W_SOPHIA, BG_KISS, 0, "Hmph... then do it properly next time. Not that I'm asking for another one."},
    {W_NAR,    BG_KISS, 0, "Her cheeks are bright pink and she turns away, but her hand stays in mine."}
);
SCENE(B_SLEEP, 
    {W_NAR,    BG_SLEEPT, 0, "It is late and the hotel is quiet. I ask Sophia if I can sleep beside her tonight."},
    {W_SOPHIA, BG_SLEEPT, 0, "...Fine. But only sleeping. And stay on your own half of the bed, husband."},
    {W_ME,     BG_SLEEPT, 0, "Understood. Good night, Sophia."},
    {W_NAR,    BG_SLEEPT, 0, "She turns off the light. In the dark her hand finds mine, and I hear her breathing slow down."},
    {W_SOPHIA, BG_SLEEPT, 0, "...Good night. And thank you for staying. Don't tell anyone I said that."},
    {W_NAR,    BG_SLEEPT, 0, "I fall asleep to the sound of the rain against the window, with her warmth right beside me."}
);
SCENE(B_FEED, 
    {W_NAR,    BG_ROM,  1, "I ask her to breastfeed me. Sophia quickly crosses her arms over her chest, her ears bright red."},
    {W_SOPHIA, BG_ROM,  1, "W-what, right now?! ...Hmph. Fine. You are my husband. Just don't stare while I get ready."},
    {W_NAR,    BG_FEED, 0, "Sophia sits on the edge of the bed and pats the spot beside her, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "My chest is so full it aches. You are my husband now, so... just this once, don't make a fuss about it."},
    {W_ME,     BG_FEED, 0, "I won't. Tell me if it's too much."},
    {W_NAR,    BG_FEED, 0, "She draws me close and holds me against her, one hand gently stroking my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Hmph. Don't look so happy. You're being awfully greedy."},
    {W_NAR,    BG_FEED, 0, "Afterwards she lets out a long, relieved breath, and I stay in her arms a little longer. The ache is gone."}
);
POOL(R_GREET_M, 
    "Oh, it's you. Come in. ...Don't just stand there grinning.",
    "Back again, husband? Hmph. Sit down.",
    "You missed me, didn't you? Don't answer. I can see it on your face.",
    "Welcome home, husband. ...I did not say that. Forget it.",
    "Hmph. My husband again. Come in and sit. Quietly.",
    "Come in, husband. Close the door behind you.",
    "Hmph. You're late. ...I wasn't waiting, I just noticed.",
    "There you are. Sit. Don't hover in the doorway.",
    "Oh? A visit? ...Fine. You may come in.",
    "I was just thinking about you. ...Don't let it go to your head.");
POOL(R_NIGHT_M, 
    "It's late. ...Come in, quickly, before someone sees you.",
    "Couldn't sleep without me? Hmph. Fine, come in.",
    "You're knocking at this hour? ...Get in here.",
    "It's late, husband. ...Come in before the neighbors hear.",
    "Shh. Quietly. ...Get in here.",
    "You can't sleep alone, can you? Hmph. Come in.",
    "Close the door softly. Everyone is asleep.");
POOL(R_SLEEP_NO_TEASE, 
    "After what you said to me today? Go and sleep alone, husband.",
    "No. You embarrassed me today. Sleep alone tonight.",
    "Not after those shameless comments. Good night. Alone.",
    "Not tonight. You teased me far too much today.",
    "Sleep alone, husband. Think about what you said.",
    "No. I'm still red in the face from what you said. Alone.");
POOL(R_SLEEP_NO_MILK, 
    "Not tonight. I'm still sore after the milk. Let me rest alone.",
    "My chest still aches after today. Come back tomorrow night.",
    "No. After everything you made me do today, I want to sleep alone.",
    "Not tonight, husband. I'm tired and sore. Come back tomorrow.",
    "No. I gave enough today. Let me sleep by myself.",
    "I'm exhausted. Tomorrow, maybe. Tonight I want my bed to myself.");
POOL(R_OUTFIT_YOGA,
    "...It's on. It's so thin I can feel everything. Don't you dare look.",
    "There. Happy? I can't believe how little this covers.",
    "Hmph. I feel half naked. Say one word and I'm changing back.");
POOL(R_OUTFIT_D8, "Back to my usual clothes, then. Happy now?", "Fine. Back to my normal clothes. Turn around.");
POOL(R_OUTFIT_COW,
    "...It's on. Ugh. I feel filthy. Don't look at me. Don't you dare look.",
    "There. I look like a farm animal and I can feel it crawling on my skin. I hate this.",
    "Done. I'm disgusted with myself and with you. Not one word, husband.",
    "Hmph... the fabric clings in all the wrong places. Revolting. Just hand me a blanket.",
    "Ugh. There. Happy? I feel like something kept in a barn. Keep your eyes to yourself.",
    "It's on. Ugh. I want to scrub my skin. Say anything and it comes off.");
POOL(R_OUTFIT_TOP,
    "...It's on. It's so thin and snug. Not one word about my chest.",
    "There. My whole chest is on display. Don't you dare stare.",
    "Hmph. I feel exposed. If you stare, I'm changing back.");
POOL(R_MILK_FULL, "I'm already full. Do something about it first, then come back with the next pill.", "I'm still full. Deal with that first, then bring the next pill.");
POOL(R_NOT_FULL, "I'm not full yet. Come back when the gauge is full.", "Not yet. Come back when I'm full.", "It's not full. Don't rush me.");
POOL(R_MILK_AFTER, "Hmph. Is there anything else you want from me?", "Well? Is there something else?", "Done? Then what else do you want?");
static const char *W_WAITHER PQ_UNUSED = "Sophia said she would come and see my room. I wait for her here.";
static const char *W_WAIT2 PQ_UNUSED = "Time passes slowly. The rain keeps beating against the window, and nobody comes.";
SCENE(B_COMMENT, {W_HER, BG_HALL, 1, NULL});


/* the lobby doors, seen from inside: the rain picture */
/* after every match she sends you out of her room (her room background, the ignoring pose = sprite 2).
   Page 1 depends on the result; page 2 (a free drink for a 20/30-hand win) is only shown when you earned one. */
SCENE(B_AFTER, {W_HER, BG_LIVING, 2, NULL}, {W_HER, BG_LIVING, 2, NULL});   /* inside her living room (not at her front door) */
POOL(A_WIN, "Here's your reward money. Now get out of my room.");
POOL(A_WINBONUS, "Here's your reward money. I included the bonus, as promised. Now get out.");
POOL(A_DRINK, "...Wait. Take this drink too. You look like you about to die. Now go.");
POOL(A_LOSS, "You lost. Don't look so surprised. Now get out of my room.", "Come back when you can actually play. Out.", "Better luck next time, boy. Now go home.", "That was embarrassing. For you. Get out of my room.");
POOL(A_LOSSBONUS, "Nice hand. Shame you lost anyway. No bonus for you. Now get out.", "Good cards, wasted. Now Out.");
POOL(A_TIE, "A tie? How irritating. Get out before I change my mind.", "Neither of us wins. How unfortunate. Now get out of my room.");

/* v13.19: Sophia's bathroom door in her bedroom (placeholder scene: she is behind the door, only her picture and dialogue) */
SCENE(B_SBATH,
    {W_NAR, BG_SBATH, 0, "The bathroom door is only open a crack. Warm steam drifts out into the room."},
    {W_SOPHIA, BG_SBATH, 0, "What are you doing at my bathroom door? Can't a woman have a little privacy?"},
    {W_ME, BG_SBATH, 0, "I just wanted to see you."},
    {W_SOPHIA, BG_SBATH, 0, "Hmph. You've seen enough. Close the door and wait outside."},
    {W_NAR, BG_SBATH, 0, "She pushes the door shut, and I'm left standing in front of it."}
);
/* v13.22: you walked into her bedroom without knocking: she is in the bathroom. PEEP / KNOCK at the bathroom door (placeholder pictures bg_sbpeek / bg_stub) */
POOL(W_SB_UNIN, "I open her bedroom door without knocking. The room is empty, and I can hear water splashing behind the bathroom door.", "I slip into her bedroom without a knock. Nobody is here, but warm steam is drifting out from under the bathroom door.", "The bedroom is quiet and she is nowhere in sight. Behind the bathroom door the water is running, and she is humming softly.");
POOL(S_OWNBATH, "Use your own bathroom!", "Hey! That one is mine. Yours is in your own room, boy.", "Out of there. You have a perfectly good bathroom of your own.", "Don't even think about it. Use your own bathroom.", "What is wrong with the bathroom in your own room? Go and use that one.", "Do you really need mine? Go use your own bathroom, right outside.", "That bathroom is mine. Yours is the one in your room. Go.");
SCENE(B_SBKNOCK,
    {W_NAR, BG_SBATH, 0, "I knock on the bathroom door. The splashing stops, and the door opens just a crack."},
    {W_NAR, BG_SBATH, 0, "Sophia hides behind the door, only her face showing over the edge, her cheeks red from the steam."},
    {W_SOPHIA, BG_SBATH, 0, "You came into my bedroom without being invited?! Have you no shame at all?"},
    {W_SOPHIA, BG_SBATH, 0, "I never said you could come in. Explain yourself, boy. Now."}
);
SCENE(B_SBSEND,
    {W_ME, BG_SBATH, 0, "I'm sorry. I'll go."},
    {W_SOPHIA, BG_SBATH, 0, "Good. Next time, knock on the front door and wait until I let you in."}
);
SCENE(B_SBPLEAD,
    {W_ME, BG_SBATH, 0, "Please, Sophia. Let me in. I just want to be with you."},
    {W_SOPHIA, BG_SBATH, 0, "In the bath? With me? Absolutely not! Are you out of your mind?"},
    {W_ME, BG_SBATH, 0, "I'll behave, I promise. I only want to be near you."},
    {W_SOPHIA, BG_SBATH, 0, "...Stop looking at me with those eyes. It isn't fair."},
    {W_SOPHIA, BG_SBATH, 0, "Hmph... Fine. But only for a little while, and you keep your hands to yourself."},
    {W_NAR, BG_SBATH, 0, "She opens the door and steps back. Warm steam rolls over me as I go in."}
);
SCENE(B_SBNODAY,
    {W_ME, BG_SBATH, 0, "Please, Sophia. Let me in."},
    {W_SOPHIA, BG_SBATH, 0, "Again? We already shared a bath today. Once a day is quite enough. Go away."}
);
SCENE(B_SBTIRED,
    {W_ME, BG_SBATH, 0, "Please, Sophia. Let me in."},
    {W_SOPHIA, BG_SBATH, 0, "Look at you, you can barely stand. Go and rest first, then ask me again."}
);
SCENE(B_SBTUB,
    {W_NAR, BG_STUB, 0, "The tub is warm and full of foam. She sits at the far end, her hair pinned up, her shoulders pink from the heat."},
    {W_SOPHIA, BG_STUB, 0, "Don't just stare. Come here, then, and sit down."},
    {W_ME, BG_STUB, 0, "You look beautiful, Sophia."},
    {W_SOPHIA, BG_STUB, 0, "Flatterer... But I'll allow it. Just this once."},
    {W_NAR, BG_STUB, 0, "We stay in the water until it starts to cool, her head resting on my shoulder. She doesn't scold me, not even once."}
);
SCENE(B_SBPEEK,
    {W_NAR, BG_SBPEEK, 0, "I push the bathroom door open just enough to see inside. Warm steam drifts out, and the room smells of lavender."},
    {W_NAR, BG_SBPEEK, 0, "Sophia sits in the tub, her back half turned to me. Her wet hair is pinned up, a few dark strands clinging to her neck."},
    {W_NAR, BG_SBPEEK, 0, "Water runs down her bare shoulders and along her back. Her skin glows in the candlelight, flushed and flawless."},
    {W_NAR, BG_SBPEEK, 0, "Her figure is more perfect than I imagined. The soft lines of her body show through the foam and steam."},
    {W_NAR, BG_SBPEEK, 0, "I can't look away. My wife is the most beautiful woman I have ever seen."},
    {W_SOPHIA, BG_SBPEEK, 0, "...Did something creak? Is someone there?"},
    {W_NAR, BG_SBPEEK, 0, "I ease the door shut and tiptoe back before she turns around."}
);
SCENE(B_RAIN, 
    {W_NAR, BG_RAIN, 0, "Thunder shakes the glass doors and the rain comes down quite heavy. Lightning tears across the empty street."},
    {W_ME,  BG_RAIN, 0, "I'm not going anywhere in this weather. I'm staying inside until it clears."}
);

/* the opening monologue: black screen, centred, typed out (two pages) */
static const char *M_PAGE[2] PQ_UNUSED = {
    "Your car broke down in the middle of your journey, leaving you stranded with nowhere to go. Fortunately, you managed to find a nearby hotel where you could take shelter for the day.",
    "As you settled in, dark clouds swallowed the sky, and the distant rumble of thunder grew louder. You had arrived just in time. Moments later, heavy rain began pouring down, fierce winds swept through the streets, and flashes of lightning illuminated the night outside."};

/* Sophia's door (after the forced match): knock, or go in uninvited */
static const char *const D_KNOCK = "KNOCK ON HER DOOR", *const D_ENTER = "QUIETLY ENTER HER ROOM";
static const char *SN_IN PQ_UNUSED = "The door isn't locked. I slip inside as quietly as I can.";
static const char *SN_TIRED PQ_UNUSED = "I'm too tired to be sneaking around. I need some energy first.";
static const char *SN_BATH PQ_UNUSED = "The bathroom door is closed. Nothing to see here yet.";
/* R1 close-up at her table: first what you think (2 lines of 46 characters at most), then, back at the table, what she says (random, never the same twice in a row) */
POOL(CU_THINK, 
    "Damn, how can her tits be so big. I can't take my eyes off them.",
    "Look the tits move. ...It's must be so soft.",
    "She has such hot and sexy figure, escpecially those titties",
    "Her tits make it so distracting, but so beautiful to look at.",
    "I could lose this match and this view might still be worth it.",
    "Somehow it's make hard watching those tits.",
    "How can her shirt support those massive tits, maybe that's why it's expansive.",
    "They call her the Queen. Should call her mama instead.");
POOL(L_STARE, 
    "Did I just catch you staring at my chest?",
    "If you value eyes, you better stop staring.",
    "If you don't want to continue the match, Get out.",
    "Hmph. what a stupid look you gave, while watching me.",
    "I saw that. Pay attention to your hand, boy.",
    "I rip your eyes out, if you keep staring at my chest.",
    "I don't accept excuse, if you lose because you keep looking at me.",
    "Why don't you just focus on the game.");
/* v10.3: close-up in mama mode (free match at home only): what you think / what she says, scolding */
POOL(CU_THINK_MM,
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
    "She leans forward to bet and I forget my own cards.");
POOL(L_STARE_MM,
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
    "And don't you dare whisper that word. My name is Sophia.");
static const char *SN_DONE PQ_UNUSED = "I've already taken a good look. Better not push my luck.";
static const char *SN_NEEDCLOSER PQ_UNUSED = "I'm not close enough to see more. I should take a step closer first.";
static const char *SN_NOVIEW_C PQ_UNUSED = "I haven't taken a closer look yet.";
static const char *SN_NOVIEW_L PQ_UNUSED = "I haven't looked that long yet.";
static const char *SN_CLOSER[3] PQ_UNUSED = {
    "I climb to her bed. Her breast is too mesmerizing to ignore, i take a sniff at her breast.",
    "Her scent so good. I want to touch and buried my face between them.",
    "Her breast so soft, It's definitely not fake. I freeze. Nothing. She is still asleep. I ease back."};
static const char *SN_LONGER[3] PQ_UNUSED = {
    "I stay where I am and just watch her breathe. The room is quiet except for the rain on the window.",
    "She shifts and her brow creases for a moment. Her eyelashes flutter. My heart is pounding.",
    "This isn't right. I shouldn't be here. I slip back towards the door before she opens her eyes."};
POOL(SN_ADMIRE, 
    "She sleeps so peacefully. In the dim light her face is softer than I ever saw it, her body really stunning.",
    "Up close she is even more beautiful. Those massive tits, Round and soft milker, I wish i could played with them.",
    "I'm afraid if i stayed for much longer she will wake up. I take one last look, then step back before she wakes.");

/* ---- walking around (your room, the hall, the lobby): narration and the receptionist ---- */
static const char *const W_CLERK_TAG = "RECEPTIONIST";
POOL(W_EVCLERK, "Welcome to the Event Center. What can I do for you?", "Good to see you. An Exhibition, or a lottery ticket today?", "Step right up. Exhibition or lottery?");
POOL(W_EV_LATE, "That is enough for today. The tables are closed. Go home, sleep, and come back tomorrow.", "No more games tonight. Rest up and come back tomorrow.", "It is late and you have played plenty. Come back tomorrow, I will keep your seat warm.");
POOL(W_LOTWIN, "I checked your lottery ticket. It is a winner! Congratulations!", "We have a winner! Your lottery ticket came in.", "Lucky you! Your ticket from yesterday won a prize.");   /* v9.0: the clerk tells you the result of your ticket (a second page gives the amount) */
POOL(W_LOTLOSE, "I checked your lottery ticket. No prize this time. Better luck next time.", "Sorry, your lottery ticket did not win. Try again, it could be the next one!", "No prize on your ticket this time. The next one might be the lucky one.");   /* the Event Center clerk (same receptionist tag) */
POOL(W_CLERK, "Welcome! Let me know if you need anything.", "Hello there. Anything I can help you with?", "Hi! If you are thirsty, the vending machine is right there.");
static const char *W_STORM PQ_UNUSED = "Rain is hammering the glass doors and the wind is howling. Nobody is going outside in this storm.";
static const char *W_BATH PQ_UNUSED = "The bathroom. I don't need it right now.";
static const char *W_SINK PQ_UNUSED = "The water is cold.";
static const char *W_EVENT_LOCKED PQ_UNUSED = "The Event Center. The doors are locked and I have no pass. Maybe i should ask someone for how to get in.";
static const char *W_SAVED PQ_UNUSED = "Game saved.";
/* time of day */
static const char *const PH_NAME[3] = {"MORNING", "EVENING", "NIGHT"};
static const char *W_SAVED_DAY PQ_UNUSED = "Game saved. You sleep through the night... a new day begins.";
static const char *W_SAVED_EVE PQ_UNUSED = "Game saved. Time passes... it is evening now.";
static const char *W_SAVED_NIGHT PQ_UNUSED = "Game saved. Time passes... it is night now.";
static const char *W_SAVED_ONLY PQ_UNUSED = "Game saved.";   /* v10.5: the sofa, SAVE ONLY (the time stays) */
static const char *W_SOFA_NIGHT PQ_UNUSED = "It's too late to sit around. I should go to bed.";
static const char *W_BED_DAY PQ_UNUSED = "I'm not tired yet.";
static const char *W_DESK_NIGHT PQ_UNUSED = "Nobody is at the desk. The store is closed at night.";
static const char *const T_SAVE = "SAVE THE GAME?", *const T_SLEEP = "SLEEP AND SAVE?";
/* the first time you reach the hall you cannot move: you look at both doors */
static const char *W_F2 PQ_UNUSED = "This must be the second floor. Quiet, and only one door.";
static const char *W_F3 PQ_UNUSED = "The third floor. Even the air smells expensive up here.";
static const char *W_HALL2 PQ_UNUSED = "That room at the end looks way too luxurious. That's definitely her room.";   /* 3rd floor */
static const char *W_HALL3 PQ_UNUSED = "That barred door must be my room.";   /* 2nd floor */
/* objective hint at the top of the screen while the story needs you somewhere */
static const char *const H_DESK = "GO TO THE FRONT DESK", *const H_ROOM = "GO TO THE 2ND FLOOR", *const H_ENTER = "ENTER YOUR ROOM";   /* H_ENTER: after the look down the hall, until you are inside */
static const char *W_DEBT_NO PQ_UNUSED = "I don't have the money, I will come back later.";   /* her door, while you hold less than DEBT in cash */
/* the Objective entry of the pause menu (wrapped to 34 characters by the game; $%d are filled in) */
static const char *O_NONE PQ_UNUSED = "No current objective.";
static const char *O_DESK PQ_UNUSED = "Go to the front desk and check in.";
static const char *O_HALL PQ_UNUSED = "Take the elevator up to the 2nd floor.";
static const char *O_ENTER PQ_UNUSED = "Enter your room.";
static const char *O_DEBT PQ_UNUSED = "Pay Sophia $%d for her stained shirt (3rd floor). Go and win the money at the Exhibition in the Event Center.";
static const char *O_DEBT_OK PQ_UNUSED = "You have the $%d. Take the elevator to the 3rd floor and knock on Sophia's door in the morning or evening.";
static const char *O_FIRST PQ_UNUSED = "She took her floor key card back. Win the $1000 table at the Exhibition in the Event Center to get her attention.";
static const char *O_BOSS PQ_UNUSED = "A new table opened at the Exhibition: $5000, and she is the one sitting there. Beat her.";
static const char *W_BOSS_NO PQ_UNUSED = "The elevator will not stop on her floor without the key card.";
static const char *const OVER_T = "GAME OVER", *const OVER_L1 = "YOU RAN OUT OF MONEY.", *const OVER_L2 = "LOAD A SAVE OR START OVER.";
static const char *const OVER_RENT_L1 = "YOU COULD NOT PAY THE RENT.", *const OVER_RENT_L2 = "THE HOTEL THREW YOU OUT. LOAD A SAVE.";

/* ---- once a day: she will not play you a second time on the same day (random, never the same twice in a row) ---- */
POOL(S_DAILY, 
    "You think I don't have anything better to do than this? Go to the exhibition if you want to play.",
    "Again? I've wasted enough of my day on you. Come back tomorrow.",
    "One lesson a day is my limit. Try the exhibition, it's more your level.",
    "Do I look like a vending machine to you? Go play with the CPUs downstairs.",
    "Your wallet may be bottomless, but my patience isn't. Come back tomorrow.",
    "Don't knock again. Go practice before you embarrass yourself twice in one day.");

/* ---- v11.0: FRIENDSHIP (blue gauge) and her room by day. Greetings at the door and her talk depend on the tier (1..5, one per 5 wins over her); the object lines are narration ---- */
POOL(G_FIRSTVIS,
    "So you actually knocked. Fine. Don't touch anything, boy.",
    "I invited you, so don't just stand there. Don't expect tea, though.",
    "The boy who took ten thousand from me. Hmph. Come in, if you must.");
POOL(G_FR1,
    "Oh, it's you. Wipe your feet if you want to come in.",
    "You again. I suppose a guest is better than silence.",
    "Knock like a gentleman, boy. Not like a debt collector.",
    "I'm busy, but not too busy to watch you lose.",
    "Hmph. Persistence is the only thing you have. Use it well.");
POOL(G_FR2,
    "Back again? You're starting to be a habit.",
    "You're getting better, boy. Don't let it go to your head.",
    "Come in or play, I don't care. Just don't be boring.",
    "I almost missed your knock. Almost. Don't smile.",
    "Not bad lately. I'm keeping my eye on you.");
POOL(G_FR3,
    "There you are. I was starting to wonder where you were.",
    "Halfway to being worth my time. Don't stop now.",
    "Knock, knock. You always knock like you're happy to be here.",
    "My favorite opponent. Don't tell anyone I said that.",
    "Come. Sit. I'll even pretend I'm not glad you came.");
POOL(G_FR4,
    "Finally. I almost put the kettle on for you.",
    "You know I don't open for just anybody, boy.",
    "Tired? You look tired. Come in and rest, then lose to me.",
    "I kept the good chair free. Don't make a big thing of it.",
    "It's you. Good. The room was too quiet.");
POOL(G_FR5,
    "My friend, at last. Come in, the door was never locked for you.",
    "Twenty wins. Hmph. I might have to call you a friend.",
    "You don't have to knock anymore. But I like that you do.",
    "Welcome. Sit anywhere. Even there, if you want.",
    "Friend. That's the word, isn't it? Don't make it strange.");
POOL(V_IN,
    "The door opens and I step inside. Her room is warm and smells of her perfume.",
    "I walk into her room. Everything is neat, expensive and perfectly in place.",
    "She steps aside and I come in. The room is quiet, with soft light.",
    "I come in and close the door behind me. She is already watching me.");
POOL(V_HELLO,
    "Yes? Speak up, boy.",
    "Don't just stare at me. Say what you want.",
    "You want something, or are you just looking?",
    "Hmph. I'm listening. Make it interesting.",
    "Did you come to talk or to play?",
    "I have time. A little. Don't waste it.");
POOL(V_BYE,
    "Leaving already? Fine. Close the door softly.",
    "Go on, then. Come back when you've learned something.",
    "Be careful out there, boy. Don't lose all your money without me.",
    "Off you go. And don't forget to knock next time.",
    "Hmph. Go. The room will be quiet again. I hate that.");
POOL(V_BED,
    "That's my bedroom. Don't even think about it.",
    "Hands off that door, boy. It's not for guests.",
    "Not that door. Not yet, and maybe not ever.");
POOL(V_BATH,
    "The bathroom is private. Use the one in your room.",
    "Are you looking for something? That's my bathroom. Out.",
    "No. Find your own towels, boy.");
POOL(V_TV,
    "A big TV on a dark stand. A poker tournament replay is paused on the screen.",
    "The TV is off. In the black screen I can see my own tired reflection.",
    "Her TV. A neat stack of tournament tapes sits beside it, labeled in her handwriting.",
    "The TV is huge. I wonder if she watches her own matches on it.");
POOL(V_WIN,
    "Tall windows with heavy curtains. The light falls neatly across the floor.",
    "Through the window I can see the grey sky. The storm hasn't cleared yet.",
    "The window is spotless. Of course it is.",
    "A small clock ticks by the window. Even her time seems well organized.");
POOL(V_TABLE,
    "A glass table, polished like a mirror. There's not a single fingerprint on it.",
    "Two chairs face each other across the table. A place for serious games.",
    "A deck of cards sits at the edge of the table, perfectly squared.",
    "A little dish of poker chips. Not a single one is out of place.");
POOL(V_SOFA,
    "A soft bench with cushions. It looks expensive, and she probably knows exactly how much.",
    "A long cushioned bench. I'd rather not sit without being asked.",
    "The bench still holds a faint warmth. She must have been sitting here.",
    "Cushions arranged in perfect order. I would not dare move one.");
POOL(V_TALK1,
    "You want to talk? What a waste of my time. But go on.",
    "I invited you to play, not to chat. Remember that.",
    "My room, my rules. First, don't touch. Second, don't talk too much.",
    "Don't get comfortable. You're still a beginner who got lucky once.",
    "What do you want to know? My secrets are not free, boy.");
POOL(V_TALK2,
    "Ten thousand in one night. Nobody ever did that to me. I still think about it.",
    "I don't like losing. I like it even less when you smirk.",
    "You play with your face too much. Keep it blank. That's free advice.",
    "Practice more. Then maybe, just maybe, I'll start to take you seriously.",
    "The Exhibition crowd is soft. Real players don't smile when they raise.");
POOL(V_TALK3,
    "I bluff more when I'm bored. Don't tell anyone I told you that.",
    "You're easier to read when you're thinking about the pot.",
    "Sit down, boy. Fine, don't. Stand there and look awkward.",
    "You make this place less quiet. That's... not terrible.",
    "Tell me about your day. Briefly. I'm not that curious.");
POOL(V_TALK4,
    "Do you know why I keep the lights low? It's easier to think. And to relax.",
    "I've been alone in this room for a long time. It's strange to have someone in it.",
    "Everyone fears me. You just sit there and ask for another match. Strange.",
    "If you keep winning, I'll have to be nice. I hate being nice.",
    "You're improving fast. A little scary, honestly. Don't tell anyone I said that.");
POOL(V_TALK5,
    "A friend. I haven't used that word in a long time. It sounds odd, doesn't it?",
    "You beat me, you stayed, and you keep knocking. That's what a friend does.",
    "Stay as long as you like. I'll pretend I'm doing you a favor.",
    "Whatever happens next, boy, you earned a seat in this room. Remember that.",
    "Don't look so proud. ...Fine. Be a little proud. You earned it.");

/* ---- rent: RENT_AMT every RENT_DAYS days; the desk, the night deduction, the reminders ---- */
static const char *const RENT_DUE_TXT = "RENT DUE TODAY";
POOL(W_CLERK_RENT, 
    "Good day. Your rent is due today. The amount is on the menu.",
    "Welcome. It's rent day, I'm afraid. Please pay at the desk.",
    "Hello! Today is the day for the room fee. Pay here, if you please.");
POOL(W_RENT_OK, "Thank you. You're paid up for another seven days.", "Payment received. See you next week.");
POOL(W_RENT_SHORT, "That's not enough for the room. The ATM is right next to my desk.", "You're short. Get the money and come back before nightfall.");
static const char *W_RENT_REMIND PQ_UNUSED = "Today the rent is due. Pay it at the front desk before night.";
static const char *W_RENT_TAKEN PQ_UNUSED = "The hotel took $%d for the room, from your pocket.";
static const char *W_RENT_TAKEN_BANK PQ_UNUSED = "The hotel took your rent: $%d from your pocket and $%d from your bank.";
static const char *W_INTEREST PQ_UNUSED = "Your bank paid you $%d interest.";   /* v13.15: waking up on a new day */
static const char *const DESK_ITEMS[3] = {"PAY RENT $%d", "STORE", "LEAVE"};
static const char *O_RENT PQ_UNUSED = "RENT $%d DUE TODAY. PAY AT THE DESK BEFORE NIGHT.";
static const char *W_CHECKIN PQ_UNUSED = "I should check in at the front desk first.";

static const char *const N_AW = "ARROGANT WOMAN", *const N_SOPHIA = "SOPHIA", *const N_ME = "YOU", *const N_UNK = "???";

/* her door greeting on later visits, by how your last match went */
POOL(G_NORMAL, "Oh, it's you. Come to lose again?", "Back already? Persistent. I'll give you that.", "Knock, knock, knock. Do you ever stop?");
POOL(G_WIN, "Beginner's luck doesn't come twice, you know.", "You won, once. Don't push your luck boy.", "Back to lose some more? Brave, after getting lucky.");
POOL(G_LOSS, "Still alive? Fine. Let's see how long you last.", "Back to donate more? How generous.", "You lost, and you came back. I don't know if you persistant or stupid.");
POOL(G_TIE, "Still even? I'll fix that today.", "A tie is just a loss that hasn't happened yet.");
POOL(G_COWARD, "Look who's back. The coward who ran from the match.", "Oh, it's you. The one who ran off halfway. You still dare to knock.", "Back again? Last time you ran away halfway through.");
POOL(G_REPEAT_OK, "Back already? Let me see if you can actually pay.");
/* shouted after you when you leave the table halfway */
POOL(L_COWARD, "Leaving halfway? What a coward. I even played it easy", "Run along, then. Some people can't handle losing.", "Scared of losing? Boy, you such a coward.");
/* after the very first match */
POOL(N_WIN, "Beginner's luck. But don't push your luck.");
POOL(N_LOSE, "Ten hands. I just take it easy.");
POOL(N_TIE, "Even? What a waste of times.");
/* her answer when you pick the match length */
static const char *C_LEN[3] PQ_UNUSED = {"Fine. Come in.", "You want to lose more?. Then get in.", "You want to waste my time even more?. I'll make you suffer."};
/* ============================ v6.2: the ring, moving in, the online store, the romance scene ============================ */
/* you propose at her door (3rd floor). The last beat is the question: a YES / NO panel follows */
SCENE(B_PROPOSE, 
    {W_NAR,    BG_HALL, 1, "My heart is pounding. I take the ruby ring out of my pocket and hold it out to her."},
    {W_SOPHIA, BG_HALL, 1, "...What is that? A ring? Is this some kind of joke, boy?"},
    {W_ME,     BG_HALL, 1, "It's not a joke. I bought it for you."},
    {W_SOPHIA, BG_HALL, 1, "For me? With what money? Don't tell me you wasted your winnings on this."},
    {W_ME,     BG_HALL, 1, "Every single dollar of it. Sophia, I like you. Be with me."},
    {W_SOPHIA, BG_HALL, 1, "..."},
    {W_SOPHIA, BG_HALL, 1, "Nobody has ever dared to say that to me."},
    {W_ME,     BG_HALL, 1, "Then I'll be the first."},
    {W_NAR,    BG_HALL, 1, "She stares at the ring for a long time. Her fingers twitch as if she wants to take it, but her pride won't let her."},
    {W_SOPHIA, BG_HALL, 1, "...It's beautiful. Much better than I expected from someone like you."},
    {W_SOPHIA, BG_HALL, 1, "Fine. I accept it. But don't think this makes you special, boy."},
    {W_NAR,    BG_HALL, 1, "She takes the ring and slides it onto her finger. For a moment she can't hide her smile."},
    {W_SOPHIA, BG_HALL, 1, "Don't look at me. I'm only trying it on to see if it fits."},
    {W_SOPHIA, BG_HALL, 1, "...Tell me, do you want me to live with you?"}
);
SCENE(B_LIVEYES, 
    {W_ME,     BG_HALL, 1, "Yes. I want you to live with me."},
    {W_SOPHIA, BG_HALL, 1, "...Hmph. Straight to the point. You really are shameless."},
    {W_ME,     BG_HALL, 1, "I mean it. I want to see you every day."},
    {W_SOPHIA, BG_HALL, 1, "Every day? You would get tired of me within a week."},
    {W_ME,     BG_HALL, 1, "Never."},
    {W_SOPHIA, BG_HALL, 1, "...We'll see about that. I'm coming to look at your room. Don't go anywhere."}
);
SCENE(B_LIVENO, 
    {W_ME,     BG_HALL, 1, "No, not now. Keep the ring for the moment."},
    {W_SOPHIA, BG_HALL, 1, "Not now? You hand me a ring and then you hesitate? Hmph."},
    {W_ME,     BG_HALL, 1, "I just want to do this properly. Give me some time."},
    {W_SOPHIA, BG_HALL, 1, "Fine by me. I like my own room better anyway."},
    {W_SOPHIA, BG_HALL, 1, "Ask me again if you change your mind. ...Not that I'm waiting. Now get out, boy."}
);
/* she comes to look at your room (after the slow wait, the knocking and the camera tilting down) */
SCENE(B_DEMAND, 
    {W_NAR,    BG_MYROOM, 0, "Knock, knock. Somebody is knocking at my door."},
    {W_NAR,    BG_MYROOM, 0, "Knock, knock, knock. The knocking is getting louder."},
    {W_NAR,    BG_MYROOM, 0, "I open the door. Sophia walks in without waiting for an answer."},
    {W_SOPHIA, BG_MYROOM, 1, "So this is where you live? It's tiny. Is this a closet?"},
    {W_ME,     BG_MYROOM, 1, "It's all I could afford..."},
    {W_SOPHIA, BG_MYROOM, 1, "One sofa, one fridge. Where would I even put my things?"},
    {W_SOPHIA, BG_MYROOM, 1, "I will not live in a cramped room like this."},
    {W_ME,     BG_MYROOM, 1, "So you do want to live here with me?"},
    {W_SOPHIA, BG_MYROOM, 1, "Don't twist my words! I only said that if I move in, it will be somewhere decent."},
    {W_SOPHIA, BG_MYROOM, 1, "Upgrade it. Ask the receptionist. Then ask me again to move in."},
    {W_NAR,    BG_MYROOM, 0, "She turns and leaves. I should talk to the receptionist about a bigger room."}
);
SCENE(B_MOVEIN, 
    {W_ME,     BG_HALL, 1, "Sophia, the room is ready. Will you move in with me?"},
    {W_SOPHIA, BG_HALL, 1, "You better have my room ready boy. I was starting to think over of you proposal."},
    {W_ME,     BG_HALL, 1, "I'll help you carry your things."},
    {W_SOPHIA, BG_HALL, 1, "Don't touch my things. I'll pack them myself."},
    {W_SOPHIA, BG_HALL, 1, "From now on, if you want to talk with me, knock on my bedroom door. Knock. Don't just walk in."},
    {W_NAR,    BG_HALL, 0, "Sophia is going to live with me now. I'm the happiest guy in the world"}
);
/* v13.23: she is a high maintenance woman: the first time you stand in your living room once she lives with you */
SCENE(B_ALLOW,
    {W_NAR,    BG_MYROOM, 0, "Sophia's bags are inside now. She looks around the living room like she owns it."},
    {W_SOPHIA, BG_MYROOM, 1, "Listen carefully, boy. I am a high maintenance woman."},
    {W_SOPHIA, BG_MYROOM, 1, "Spa days, salons, shoes, bags. None of it is free, and I won't ask twice."},
    {W_ME,     BG_MYROOM, 1, "...How much are we talking about?"},
    {W_SOPHIA, BG_MYROOM, 1, "That is for me to decide. Every three days I take my spending money straight from your bank."},
    {W_SOPHIA, BG_MYROOM, 1, "Don't bother handing it to me. I will take it myself. You just make sure it is there."},
    {W_SOPHIA, BG_MYROOM, 1, "Keep at least $500 in your bank, every single time I come to collect."},
    {W_SOPHIA, BG_MYROOM, 1, "If it isn't there, I stop talking to you. At all. No door, no knock, nothing."},
    {W_ME,     BG_MYROOM, 1, "Understood. I will make sure the bank is never empty."},
    {W_SOPHIA, BG_MYROOM, 1, "Good boy. Now move. I need to decide where my things go."},
    {W_NAR,    BG_MYROOM, 0, "She just moved in and my wallet already hurts. I would still do it all again."}
);
/* v13.26: you woke up with the bottle gauge still full (you never asked her for the milk): the next morning, in the living room, she tells you she put the bottle in the fridge (the gauge is empty now) */
SCENE(B_FRIDGE,
    {W_NAR,    BG_MYROOM, 0, "I wake up to the sound of the fridge door closing. Sophia is standing in the living room."},
    {W_SOPHIA, BG_MYROOM, 1, "You never asked me for my milk yesterday. Not once."},
    {W_SOPHIA, BG_MYROOM, 1, "It was just sitting there, so I took care of it myself. I used the pump."},
    {W_SOPHIA, BG_MYROOM, 1, "The bottle is in the fridge now. My bottle gauge is empty again."},
    {W_ME,     BG_MYROOM, 1, "Thanks, Sophia. I should have asked."},
    {W_SOPHIA, BG_MYROOM, 1, "Next time, ask me yourself. I prefer to hand it to you."},
    {W_NAR,    BG_MYROOM, 0, "A fresh Baby Milk Bottle is waiting for me in the fridge."}
);
/* v13.27: you lie down on your bed with a full gauge and a worn-out pump: she knocks and asks for a new pump (you cannot sleep until she has it) */
SCENE(B_PUMPASK,
    {W_NAR,    BG_MYROOM, 0, "I have just lain down on my bed when somebody knocks on my bedroom door. Three slow knocks."},
    {W_SOPHIA, BG_MYROOM, 0, "It's me. Open the door. I know you are not asleep yet."},
    {W_NAR,    BG_MYROOM, 0, "I open the door. Sophia stands there in her nightclothes, holding the old breast pump."},
    {W_SOPHIA, BG_MYROOM, 1, "This stupid thing is broken. My bottle gauge is full and I cannot do anything about it."},
    {W_SOPHIA, BG_MYROOM, 1, "Buy me a new pump from the online store. You are not going to sleep until it is in my hands."},
    {W_ME,     BG_MYROOM, 1, "Okay, okay. I will order one right now."},
    {W_SOPHIA, BG_MYROOM, 1, "Good. I will wait here in the living room. Hurry up."},
    {W_NAR,    BG_MYROOM, 0, "No sleep for me until Sophia has her new pump. I can order it on the computer and give it to her."}
);
/* v13.27: the bed while she waits for the pump / SLEEP TOGETHER before she has it / her words when you talk to her while she waits */
POOL(W_BED_PUMP, "Sophia is waiting in the living room for her pump. I cannot sleep before I give it to her.", "No sleep yet. Sophia is standing right there, waiting for her new pump.", "I should not lie down now. She wants her pump first, and she looks impatient.");
POOL(R_SLEEP_NO_PUMP, "Not until I have my new pump. I cannot sleep with a full gauge.", "No. Go and order my pump first, then we talk about sleeping.", "I am not going to bed like this. My pump, please. Then you can sleep.");
POOL(R_PUMP_WAIT, "Did you order my new pump yet? Hand it over from the GIVE page.", "I am still waiting for my pump. Order it on your computer, then give it to me.", "Pump first. Then I go back to my room and you can sleep.");
/* v13.23: her door while the bank was short at a payout (she does not answer; she never stands in the living room either) */
POOL(W_AL_DOOR, "I knock. No answer. Not until the bank has at least $500 again.", "Her door stays shut. She meant it: no money in the bank, no Sophia.", "Silence behind the door. I should visit the ATM first.", "I can hear her music behind the door, but she does not answer. The bank needs at least $500.", "Nothing. She is ignoring me completely until the bank is filled up.");
static const char *W_ALLOWANCE PQ_UNUSED = "Sophia took $%d from your bank.";   /* v13.23: waking up on a payout day (needs exactly one %d) */
POOL(S_UPFIRST, "Not yet. Your room is too small for me. Upgrade it at the front desk first, then ask me again.");
static const char *const DOOR_ITEMS[5] = {"PLAY", "PROPOSE", "ASK TO MOVE IN", "LEAVE", "COME IN"};   /* (v11.0: COME IN = the visit to her room by day, DR_COMEIN) */
static const char *W_SPARE PQ_UNUSED = "The door of the spare room. Nobody lives there yet.";
static const char *W_CLOSED3 PQ_UNUSED = "Her door is locked. The room behind it is empty now.";
/* the desk: the upgrade, and the rent lines for the bigger room */
static const char *const DESK_UPG = "UPGRADE ROOM $%d";
static const char *W_UPG_OK PQ_UNUSED = "Done! The bigger room is ready. The rent is higher now, of course.";
static const char *W_UPG_SHORT PQ_UNUSED = "You need more cash for the upgrade. The ATM is right next to my desk.";
POOL(W_CLERK_UPG, "Welcome! Looking for a bigger room, perhaps?", "Hello there. How can I help?");
static const char *W_UPG_OK3 PQ_UNUSED = "Done! There is a playing room behind the new door in your living room. The rent is higher now, of course.";   /* v13.2: upgrade 3 */
POOL(W_CLERK_UP3, "Welcome! Would you like a playing room next to your living room?", "Hello there. A room with a poker table and a pool table is on offer.");   /* v13.2: upgrade 3 greeting */
POOL(R_INV_YES, "A playing room? All right, I'll come and look. Use the door on the left.", "Fine, I'm curious. Go to the left door and I'll follow you in.", "Okay. Show me this playing room. The left door, right?", "Sure, why not. Lead the way through the left door.");   /* v13.3: she accepts the invitation (living room) */
POOL(R_INV_NO, "Not now. I'm in my room. Ask me when I'm out in the living room.", "I'm not leaving my room right now. Ask me out there.", "No. I'm settled in here. Come and ask me in the living room.");   /* v13.3: she refuses (at her door or in her bedroom) */
POOL(R_PLAY_ALONE, "I should ask Sophia to come in here first.", "Nobody to play with. Maybe invite Sophia to the playing room.", "A poker table needs two players. Sophia is not here.");   /* v13.4: you press A on the poker table without her in the room */
POOL(R_PLAY_TALK, "This room is nice. We could play something here later.", "Look at that pool table. Maybe another time.", "I like it in here. Don't get any ideas yet.");   /* v13.3: PLACEHOLDER: what she says when you talk to her in the playing room */
POOL(R_POOL_ASK, "So you want to play pool with me? Should I go easy on you, or should I not hold back?", "Eight ball? Fine. Do you want me to go easy on you, or do I play for real?", "Racking them up? Best of five sets. Do I go easy on you, or hold nothing back?", "Best of five sets. Should I go easy on you, or do you want the real me?");   /* v13.7: Her question before an 8 ball match in the playing room: go easy or not (4 lines) */
POOL(R_POOL_EASY, "Fine, I will go easy on you. Try not to look too relieved.", "Easy it is. Don't expect me to say sorry when you lose.", "Understood. I'll miss a shot or two. On purpose. Don't get used to it.", "Alright, gentle Sophia it is. Just this once.");   /* v13.7: Her answer when you pick GO EASY */
POOL(R_POOL_HARD, "Oh? No hold back? Then don't cry when I clear the table.", "Brave. Then I won't go easy on you. Not even a little.", "Don't say I didn't warn you. Rack them up.", "You asked for it. I'm playing for real now.");   /* v13.7: Her answer when you pick NO HOLD BACK */
POOL(R_POOL_ALONE, "Nobody to play with. I should ask Sophia to come in here first.", "Eight ball needs two players. Sophia is not here.", "A pool table and no opponent. Maybe invite Sophia to the playing room.");   /* v13.7: What you think when you press A on the pool table and Sophia is not in the playing room */
POOL(R_POOL_TIRED, "You can barely hold a cue like that. Drink something and come back.", "No. You look exhausted. Rest first, then I will beat you.", "Not like this. Go get some energy back first.");   /* v13.7: Her refusal when you are too tired for an 8 ball match (needs 20 percent energy) */
POOL(R_POOL_SETW, "Tch. Lucky shot. Rack them up again.", "Fine, that set is yours. It won't happen twice.", "You won that one. Enjoy it while it lasts.", "Hmph. Don't get comfortable.");   /* v13.7: Her line when you win a set of the 8 ball match */
POOL(R_POOL_SETL, "That's one for me. Keep up, will you?", "Too slow. Next set.", "Did you even aim? Rack them up.", "Mine. Try harder.");   /* v13.7: Her line when she wins a set of the 8 ball match */
POOL(R_POOL_WIN_E, "You won? I went easy on you, so that does not count!", "Hmph. I was going easy. Don't let it go to your head.", "Beating me on easy is nothing to brag about.");   /* v13.7: Her line when you win the 8 ball match and she went easy */
POOL(R_POOL_WIN_H, "...You beat me when I wasn't holding back? ...That was good.", "I played for real and you still won. I hate that. And I respect it.", "Don't smile like that. ...Fine. You earned it.");   /* v13.7: Her line when you win the 8 ball match and she did not hold back */
POOL(R_POOL_LOSE_E, "I went easy on you and you still lost? Impressive. Not in a good way.", "Even on easy you could not beat me. Practice more.", "That was me going easy. Think about that.");   /* v13.7: Her line when she wins the 8 ball match and she went easy */
POOL(R_POOL_LOSE_H, "I told you I would not hold back. Better luck next time.", "That's what happens when I'm serious. Come back when you're ready.", "Not bad for someone who lost. Challenge me again.");   /* v13.7: Her line when she wins the 8 ball match and she did not hold back */
static const char *W_UPG_OK1 PQ_UNUSED = "Done! Your room is upgraded, with a computer in the bedroom. The rent is higher now.";   /* v9.3: upgrade 1 */
POOL(W_CLERK_UP1, "Welcome! Tired of that tiny room? I can upgrade it for you.", "Hello there. A bigger room with a computer is on offer.");   /* v9.3: upgrade 1 greeting */
POOL(W_CLERK_RENT_UP, 
    "Good day. Your rent is due today for the bigger room.",
    "Welcome. It's rent day, I'm afraid. Please pay at the desk.");
static const char *W_RENT_REMIND_UP PQ_UNUSED = "Today the rent is due. Pay it at the front desk before night.";
static const char *O_RING PQ_UNUSED = "PROPOSE TO HER AT HER DOOR (3RD FLOOR).";
static const char *O_VISIT PQ_UNUSED = "GO BACK TO YOUR ROOM.";
static const char *O_UPG PQ_UNUSED = "PAY $10000 AT THE DESK TO UPGRADE YOUR ROOM AGAIN.";
static const char *O_UP1 PQ_UNUSED = "UPGRADE YOUR ROOM AT THE FRONT DESK ($5000).";   /* v9.3: upgrade 1 (it brings the computer) */
static const char *O_ASKMOVE PQ_UNUSED = "ASK SOPHIA TO MOVE IN (3RD FLOOR DOOR).";
/* the online store (the computer in your bedroom) */
static const char *SH_NAME[14] = {   /* v9.5: writable: items.txt can replace the name, info, info page, give-page name and "bought" note of every item (src/extitems.h) */"FLOWER", "MONEY STACK", "BREAST ENHANCEMENT PILL", "RUBY RING", "BREAST PUMP", "MILK INDUCER PILL", "BODY LOTION", "PINK CARD DECK", "YOGA OUTFIT", "GOLD BAR", "DIAMOND RING", "DESIGNER TOP", "COW PRINT OUTFIT", "MOMMY MILK INJECTION"};
static const char *SH_INFO[14] = {"GIFT: +10% ROMANCE", "GIFT: +20% ROMANCE", "GIFT: ONE TIME ONLY", "PROPOSAL RING: ONE TIME ONLY", "GIVE IT TO HER. IT CAN BREAK", "GIFT: +25% OF THE BOTTLE GAUGE",
    "GIFT: +12% ROMANCE", "GIFT: +15% ROMANCE", "GIFT: UNLOCKS HER OUTFIT", "GIFT: +50% ROMANCE", "FOR A WEDDING. NO USE YET", "GIFT: UNLOCKS HER OUTFIT", "GIFT: UNLOCKS HER OUTFIT", "PERMANENT MILK. NO FULL GAUGE"};
static const char *SH_BOUGHT[14] = {"BOUGHT A FLOWER", "BOUGHT A MONEY STACK", "BOUGHT THE PILL", "BOUGHT THE RUBY RING", "BOUGHT THE BREAST PUMP", "BOUGHT A MILK INDUCER PILL", "BOUGHT BODY LOTION", "BOUGHT A PINK CARD DECK", "BOUGHT A YOGA OUTFIT", "BOUGHT A GOLD BAR", "BOUGHT THE DIAMOND RING", "BOUGHT A DESIGNER TOP", "BOUGHT A COW PRINT OUTFIT", "BOUGHT THE MOMMY MILK INJECTION"};
/* v9.5: the fridge / lobby vending machine names (the machine picture has its own painted text) */
static const char *INV_NAME[3] = {"SODA", "ENERGY DRINK", "SOPHIA BREAST MILK"};
static const char *INV_BOUGHT[2] = {"BOUGHT A SODA", "BOUGHT AN ENERGY DRINK"};
/* the romance scene (you knock on her bedroom door in your living room) */
POOL(R_GREET, "What do you want?", "Hmph. You again.", "Make it quick, boy.",
    "What now?",
    "Oh. It's you. Make it quick, boy.",
    "I was about to rest, you know. ...Come in, boy.",
    "Knocking again? You're persistent, boy.",
    "Hmph. Wipe that grin off your face first.",
    "Speak up, boy. I don't have all day.");
POOL(R_TIRED, "You look exhausted. Rest first, then we play.", "You're swaying on your feet. Rest first, then we play.", "Go to bed, boy. Cards need a clear head.", "You look half dead. Sleep first.");
POOL(R_TODAY, "We already played today. Come back tomorrow.", "One match a day is plenty. Come back tomorrow.", "We played already today. Tomorrow, boy.", "I'm done for the day. Come back tomorrow.");
POOL(R_GIFT_TODAY, "One gift a day is enough. Come back tomorrow.", "Don't spoil me so fast. Come back tomorrow.", "You already gave me something today. Tomorrow.");
POOL(R_GIFT_NONE, "You have nothing to give me.", "Your hands are empty. Bring something next time.", "Nothing? Then what are you doing here?");
POOL(R_GIFT_PILL, "Not yet. Show me you really care about me first.", "Not yet. I'm not ready to take that.", "You haven't earned that yet. Keep trying.");
POOL(R_GIFT_FULL, "I have too many already. Keep it for later.", "I can't carry any more of these. Later.", "My place is full of those. Keep it for now.");
static const char *R_GIFT_OK[8] PQ_UNUSED = {"Flowers? ...Thank you.", "Money? Hmph. I'll take it.", "...What is this? Fine. I'll take it.",
    "Body lotion? ...It's the kind I like. Fine, I'll take it.", "Cards? Pink ones, too. Hmph. At least you have taste.", "Yoga clothes? Are you saying something about my body?", "Gold? Real gold? Hmph. Now you have my attention.",
    "An injection? ...You really have no shame."};
POOL(R_ASK_NONE, "I have nothing to give you right now.", "There's nothing right now. Ask later.");
POOL(R_BACK, "Good game. Anything else?", "That was fun. ...Don't tell anyone I said that.", "Not bad. Is there something else you need?", "Well played. Now, what else?");
static const char *const R_ITEMS[4] = {"GIVE A GIFT", "PLAY A MATCH", "LEAVE", "LEAVE"};   /* (v6.6: after the day-8 scene a 4th entry sits before LEAVE, see R_MILKITEM) */
static const char *const R_MILKITEM = "GIVE MILK PILL";
static const char *const R_ASKITEM = "ASK FOR MILK";        /* v8.2: v13.17: only after the Mommy Milk Injection: "ASK FOR MILK  1 LEFT TODAY" */
static const char *const R_LEN[3] = {"10 HANDS   +5% ROMANCE", "20 HANDS   +10% ROMANCE", "BACK"};
static const char *R_GNAME[12] = {"FLOWER", "MONEY STACK", "PILL", "PUMP", "MILK PILL", "BODY LOTION", "PINK CARD", "YOGA OUTFIT", "GOLD BAR", "DESIGNER TOP", "COW PRINT OUTFIT", "MOMMY INJECTION"};
/* v6.6: every gift is a short scene (she takes it, a few lines), then you are back on the map in front of her door */
static const char *R_GIFT_HAND[12] PQ_UNUSED = {"I hold out the flowers to her.", "I hold out a thick stack of cash to her.", "I hold out the small bottle to her.", "I hand her the box with the breast pump.", "I hold out another milk inducer pill to her.",
    "I hold out a bottle of body lotion to her.", "I hold out a deck of pink playing cards to her.", "I hold out a neatly folded yoga outfit to her.", "I hold out a heavy gold bar to her.", "I hold out a sleek designer top to her.", "I hold out a thin cow print outfit to her.", "I hold out the Mommy Milk Injection to her."};
static const char *R_GIFT_L2[12] PQ_UNUSED = {"Don't get the wrong idea, boy. They're pretty, that's all.", "Don't get the wrong idea. I'm only taking it because I deserve it.", "Don't get the wrong idea.", "Don't get the wrong idea.", "Now leave me alone. I need some rest.",
    "Don't get the wrong idea. My skin is only dry from the air in this hotel.", "Don't get the wrong idea. A queen needs a proper deck, that's all.", "Don't get the wrong idea. I only need something comfortable to stretch in.", "Don't get the wrong idea. I just know a good deal when I see one.",
    "Don't get the wrong idea. It's only a top.", "Don't get the wrong idea. It's only an outfit.", "Don't get the wrong idea. I'm only doing this once."};
static const char *R_GIFT_END[12] PQ_UNUSED = {"She turns away, but I can tell she's pleased.", "She counts it without looking at me, then closes the door.", "She takes it.", "She takes it.", "She swallows it, then shuts the door. Maybe the bottle gauge fills a little more.",
    "She sniffs the bottle and hides a small smile before closing the door.", "She fans the cards in her hand, then shuts the door.", "She holds it against herself, glances at me, then slams the door.", "She weighs it in her hand and looks at me a moment too long before closing the door.",
    "She holds it up to the light, checks the fit, and shuts the door.", "She holds it up to the light, turns red, and shuts the door.", "She takes the syringe and shuts the door."};
/* v10.2: the gift scene: every repeatable gift has pools for its four beats (the first entry is the old text); the pill and the pump have their own scenes */
POOL(GH_FLOWER, "I hold out the flowers to her.", "I step forward and offer her a small bunch of flowers.", "I take the flowers from behind my back and hold them out.", "I hold the flowers out, a little awkwardly, and wait.", "I offer her the flowers and try not to blush.");
POOL(GH_STACK, "I hold out a thick stack of cash to her.", "I put a thick bundle of cash in her hand.", "I hold out an envelope stuffed with money.", "I fan out a stack of cash and offer it to her.", "I slide a thick wad of cash across to her.");
POOL(GH_LOTION, "I hold out a bottle of body lotion to her.", "I hand her a bottle of body lotion.", "I hold out a small bottle of lotion for her.", "I offer her the lotion and look away politely.", "I set a bottle of body lotion in her hands.");
POOL(GH_DECK, "I hold out a deck of pink playing cards to her.", "I hand her a deck of pink playing cards.", "I hold out a fresh deck of pink cards, still sealed.", "I offer her the pink deck with a small bow.", "I set a neat deck of pink cards in her hand.");
POOL(GH_YOGA, "I hold out a neatly folded yoga outfit to her.", "I hold out a folded yoga outfit for her.", "I hand her a soft yoga outfit in a neat bag.", "I offer her the yoga outfit, trying to look casual.", "I place a yoga outfit in her arms.");
POOL(GH_GOLD, "I hold out a heavy gold bar to her.", "I hold out a gleaming gold bar.", "I place a heavy gold bar in her hands.", "I offer her a gold bar, still cold from my bag.", "I unwrap a gold bar and hold it out to her.");
POOL(GH_MILK, "I hold out another milk inducer pill to her.", "I hold out another milk inducer pill for her.", "I place one more milk inducer pill in her palm.", "I offer her another pill and a glass of water.", "I hand her the small pill without a word.");
POOL(GO_FLOWER, "Flowers? ...Thank you.", "Flowers again? ...They smell nice. Thank you.", "Oh. Flowers. ...Where did you even find these?", "For me? ...Hmph. Fine. They're not ugly.", "Flowers? You do know I'm not that easy to impress.");
POOL(GO_STACK, "Money? Hmph. I'll take it.", "Cash? ...Hmph. At least it's real.", "Money, huh. You have no subtlety at all.", "Is this a bribe? ...Fine, I'll accept it.", "Hmph. Practical. I like practical.");
POOL(GO_LOTION, "Body lotion? ...It's the kind I like. Fine, I'll take it.", "Lotion? ...Mm. It smells lovely. Thank you.", "You remembered my skin gets dry. ...Hmph.", "Body lotion? That's... surprisingly thoughtful.", "Hmph. The expensive kind. You have good taste.");
POOL(GO_DECK, "Cards? Pink ones, too. Hmph. At least you have taste.", "A new deck? ...Pink. Of course it's pink.", "Cards? Sealed, too. ...Hmph. Acceptable.", "You bought me cards? ...I suppose that's fitting.", "Oh. A fresh deck. You have good instincts.");
POOL(GO_YOGA,
    "Yoga clothes? ...These are so thin. I can see my own hand through the fabric!",
    "You want me to wear THIS? It's nothing but a few strips of cloth.",
    "Is this a yoga outfit or underwear? Why does it show so much?",
    "Yoga clothes? The fabric is paper-thin... Are you out of your mind?",
    "This barely covers anything. Who wears this in public?!");
POOL(GO_GOLD, "Gold? Real gold? Hmph. Now you have my attention.", "Gold? ...Is this real? Hmph. It is.", "A whole bar? ...You're either rich or insane.", "Gold, hm? You finally speak my language.", "That's heavy. ...Don't drop it. Give it here.");
POOL(GL_FLOWER, "Don't get the wrong idea, boy. They're pretty, that's all.", "Don't get the wrong idea. I just don't like waste.", "Don't read into this. A queen deserves flowers, that's all.", "It means nothing. Don't smile like that.", "Don't get ideas. I'll put them in water, that's all.");
POOL(GL_STACK, "Don't get the wrong idea. I'm only taking it because I deserve it.", "Don't get ideas. Money is just money.", "Don't get the wrong idea. I'll find a good use for it.", "This doesn't buy you anything. ...Probably.", "Don't think you can buy me. ...Not that I'm refusing.");
POOL(GL_LOTION, "Don't get the wrong idea. My skin is only dry from the air in this hotel.", "Don't get the wrong idea. The hotel air really is dry.", "It's just practical. Don't read anything into it.", "Don't stare! It's for my skin, that's all.", "Don't get any ideas about helping me apply it.");
POOL(GL_DECK, "Don't get the wrong idea. A queen needs a proper deck, that's all.", "Don't get the wrong idea. My old deck was worn out.", "Don't think this makes us partners at the table.", "It's for practice. Don't smile at me like that.", "Don't get excited. A queen simply needs good tools.");
POOL(GL_YOGA,
    "I will never wear this. It shows my whole waist and far more besides!",
    "Don't you dare picture me in it. It shows far too much skin. Pervert.",
    "Take your eyes off the bag! No one gets to see that much of me. Ever.",
    "I am not a woman who walks around half naked. What kind of girl do you think I am?",
    "Too thin, too tight, too much skin. ...I should throw it in your face.");
POOL(GL_GOLD, "Don't get the wrong idea. I just know a good deal when I see one.", "Don't get the wrong idea. I'm just appraising it.", "Don't think this impresses me. ...Much.", "It's a good investment, that's all.", "Don't get cocky. It's just shiny metal.");
POOL(GL_MILK, "Now leave me alone. I need some rest.", "Don't stare while I take it. Turn around.", "Don't look so hopeful. It's only medicine.", "Fine. But this is your responsibility.", "Don't comment. Not a single word.");
POOL(GE_FLOWER, "She turns away, but I can tell she's pleased.", "She hides her face behind the petals and closes the door.", "She sniffs them quietly, then tells me to leave.", "She turns away, but I catch her smiling as the door shuts.", "She presses the flowers to her chest and shoos me away.");
POOL(GE_STACK, "She counts it without looking at me, then closes the door.", "She counts it twice, nods once, and closes the door.", "She tucks it away with a smug little smile and shuts the door.", "She riffles through the bills, then waves me out.", "She weighs the stack in her hand and quietly shuts the door.");
POOL(GE_LOTION, "She sniffs the bottle and hides a small smile before closing the door.", "She uncaps it, smells it and tries not to smile as she shuts the door.", "She hugs the bottle to herself and closes the door quickly.", "She turns away, but I hear her humming behind the door.", "She sniffs the bottle once more, then pushes the door shut.");
POOL(GE_DECK, "She fans the cards in her hand, then shuts the door.", "She shuffles the cards with a flick of her wrist and shuts the door.", "She breaks the seal, smiles faintly and closes the door.", "She cuts the deck with one hand and waves me away.", "She taps the box against her palm and shuts the door.");
POOL(GE_YOGA,
    "She glares at the thin fabric, stuffs it into the bag, and slams the door.",
    "She holds it at arm's length like something dirty, then slams the door.",
    "She crushes the fabric in her fist, snaps at me, and slams the door.",
    "She flushes red to her ears, glares at me, and shuts the door hard.",
    "She mutters that I'm a pervert, but she still takes it inside.");
/* v10.8: the DESIGNER TOP gift scene (4 beats, 5 versions each) */
POOL(GH_TOP, "I hold out a sleek designer top to her.", "I hand her a designer top in a glossy shopping bag.", "I present her with a fitted designer top, still folded.", "I offer her the designer top, trying not to smile.", "I place a fitted designer top in her arms.");
POOL(GO_TOP,
    "A designer top? ...It's see-through! The fabric is thinner than paper.",
    "This neckline is so low. Who would wear this outside the bedroom?!",
    "Designer, but there's barely any top. ...Is the rest of it missing?",
    "The back is open and the front is so thin. Are you out of your mind?",
    "It's tiny! It would show my shoulders, my chest, everything!");
POOL(GL_TOP,
    "I will not wear this. It shows far too much skin, and every curve.",
    "Don't you dare imagine me in it. It's thin enough to see through. Pervert.",
    "My whole chest would be on display! ...I should throw this at your head.",
    "Stop smiling! This is not a top, it's an insult.",
    "Even the yoga outfit covers more. What are you trying to do to me?");
POOL(GE_TOP,
    "She pinches the thin fabric between two fingers, goes pale, then red, and slams the door.",
    "She stares at the neckline, mutters something sharp, and shuts the door hard.",
    "She holds it to the light, gasps at how thin it is, and slams the door.",
    "She stuffs it back into the bag, glares at me, and bangs the door shut.",
    "She hugs it against her chest so I can't see it, then slams the door.");
/* v11.1: the COW PRINT OUTFIT gift scene (4 beats, 5 versions each) */
POOL(GH_COW, "I hold out a thin cow print outfit to her.", "I hand her a cow print outfit in a small paper bag.", "I present her with a cow print outfit, neatly folded.", "I offer her the cow print outfit and try not to grin.", "I place a soft cow print outfit in her arms.");
POOL(GO_COW,
    "Cow print?! Ugh... Is that really what you think of when you look at me? Disgusting.",
    "Spots and a cutout. Ew. Why would anyone buy this for his own wife?",
    "You bought me a cow costume?! ...I feel sick just holding it. What is wrong with you?",
    "Is this a joke?! A cow print outfit... Ugh, keep it away from me. It's revolting.",
    "...What is this? Cow spots? Ew, ew. You truly think I would wear this?!",
    "Cow print. Ugh. I am a queen, not something you keep in a barn. This is repulsive.");
POOL(GL_COW,
    "Get that thing away from my face. I can't believe you looked at it and thought of me.",
    "It's cheap fabric and bad taste. I'm disgusted that you picked it out at all.",
    "Do you understand how degrading this is? Ugh... I feel dirty just looking at the spots.",
    "Don't you dare smile. I'm not touching it until you stop looking so pleased. Ugh, I can't even look at you.",
    "Spare me the explanation. Whatever you imagined, it makes my skin crawl.",
    "A farm animal. That's what this says. I'm so disgusted with you that I can barely speak.");
POOL(GE_COW,
    "She holds the bag at arm's length, wrinkles her nose as if it smelled, and carries it inside anyway.",
    "She shudders, pinches the cutout between two fingers, and shuts the door without a word.",
    "She flinches away from the spots as if they might bite, yet she doesn't give the outfit back.",
    "She goes pale, gags a little, and takes the outfit inside, holding it far from her body.",
    "She curls her lip at me with open revulsion, then snatches the bag and slams the door.",
    "She stares at the spots with pure disgust, mutters that she'll throw it out, and keeps it.");
POOL(GE_GOLD, "She weighs it in her hand and looks at me a moment too long before closing the door.", "She bites the corner like a pirate, grins, and shuts the door.", "She hugs it against herself for a moment, then closes the door.", "She runs her thumb over it and quietly shuts the door.", "She lifts it to the light, smirks at me, and slams the door.");
POOL(GE_MILK, "She swallows it, then shuts the door. Maybe the bottle gauge fills a little more.", "She swallows it with a grimace and shuts the door. The gauge should fill a bit more.", "She downs the pill, makes a face, and closes the door. Maybe the gauge moves.", "She takes it, flushes red, and shuts the door. The bottle gauge fills a little.", "She gulps it down and waves me off. Maybe the gauge fills more.");
SCENE(B_GIFT, {W_NAR, BG_ROM, 0, NULL}, {W_SOPHIA, BG_ROM, 1, NULL}, {W_SOPHIA, BG_ROM, 1, NULL}, {W_NAR, BG_ROM, 0, NULL});   /* filled in by vn_start (the pictures too: her door or her room) */


/* v6.8: you must unpack in your room before the Event Center and the 3rd floor open up */
static const char *W_UNPACK_NO PQ_UNUSED = "I need to unpack my things first.";
static const char *W_UNPACKED PQ_UNUSED = "Alright, finished unpacking my things.";
/* v6.8: the info pages in the online store (Y): ring, breast enhancement pill, milk inducer pill. Pages of at most 3 lines of 32 characters. */
static const char *SH_DESC[14] = {
    NULL, NULL,
    "A pill said to make a woman's body grow fuller. It needs about a week before the change shows. Give it to her one time only.",
    "The mysterious, beautiful ring that can charm people and make them fall for whoever gives it.",
    NULL,
    "A pill that makes the body produce milk. Every pill fills the bottle gauge by 25%. Four pills fill a bottle.",
    NULL, NULL, NULL, NULL,
    "A dazzling diamond ring, made for a wedding with Sophia. After the day-8 scene, give it to her from the gift page to marry her.",
    "Contours to the wearer's body, emphasizing her shape and chest. Slightly more expansive than the yoga outfit.",   /* v10.8: SH_TOP */
    "A thin and comfy outfit that contours the wearer's body shape, with a chest cutout.",   /* v11.1: SH_COW */
    "This experimental injection will make a woman produce milk permanently. No full gauge needed. One milk a day." };   /* v12.3: SH_INJECT */

/* v7.8: the info page of the Sophia breast milk in the fridge (Y), and the online store tabs */
static const char *MILK_INFO PQ_UNUSED = "This sweet milk is from Sophia's breast. Because of the special medicine, the milk never spoils and stays fresh.";
static const char *const FRIDGE_EMPTY = "YOU HAVE NO ITEM IN FRIDGE";

/* ---------- v9.5: MAMA MODE ----------
   Once you are married, R2 in her menu opens an on-screen keyboard: type MAMA or MAMA SOPHIA (names.txt) and she answers to it (Game.call = 1);
   type SOPHIA and she is her usual self again. Mama mode has its own dialogue (these texts), and only in mama mode the BREASTFEED row exists on page 4.
   Married but not in mama mode she calls you HUSBAND (R_GREET_M, R_MORE_M, T_KICK_M ...); everything before the wedding keeps calling you boy. */
SCENE(B_CALLMAMA,   /* you type MAMA: she refuses the name, the mode still starts */
    {W_NAR, BG_ROM, 1, "I take a breath and call her by a new name. \"Mama.\" Sophia goes completely still."},
    {W_SOPHIA, BG_ROM, 1, "...What did you just call me? Don't. Don't you ever call me that again."},
    {W_ME, BG_ROM, 1, "But I..."},
    {W_SOPHIA, BG_ROM, 1, "I'm your wife, not your mother! Say my name. Sophia. S-O-P-H-I-A."},
    {W_NAR, BG_ROM, 1, "Her face is bright red, and she is looking everywhere except at me."},
    {W_SOPHIA, BG_ROM, 1, "...Do what you like. But I will not answer to it. Not once."}
);
SCENE(B_CALLBACK,   /* you type SOPHIA again */
    {W_NAR, BG_ROM, 1, "I clear my throat and call her by her name again. \"Sophia.\""},
    {W_SOPHIA, BG_ROM, 1, "...Finally. That is my name. Use it."},
    {W_ME, BG_ROM, 1, "You looked a little disappointed."},
    {W_SOPHIA, BG_ROM, 1, "Don't push your luck, husband."}
);
POOL(R_GREET_MM,
    "...Come in. And don't you dare call me that.",
    "Oh, it's you. Say it and the door shuts, husband.",
    "Come in. Keep that silly name to yourself, please.",
    "You again. ...Not a word of that name, understood?",
    "Fine, come in. I'm still not your mama.",
    "Hmph. Come in. Behave, or the door stays shut.",
    "Come in. And keep that word locked behind your teeth.",
    "You're here again. ...Sit. Quietly. No names.",
    "Hmph. I'll let you in if you promise to behave.",
    "It's you. Say that name and I'll bar the door.",
    "Come in, husband. Just husband. Understood?",
    "Oh, good. You're early. ...I didn't say that.");
POOL(R_NIGHT_MM,
    "It's late. ...Come in, and keep that name to yourself.",
    "At this hour? Get in here. And don't you dare say it.",
    "Shh, keep your voice down. I'm not answering to that name.",
    "Couldn't sleep? Fine. Come in. Just don't call me that.",
    "Quiet. Get in here. ...And not one word of that name.",
    "You woke me up. ...Come in, and keep that name to yourself.",
    "Late visitor. Shh. Come in, husband, before I change my mind.",
    "It's the middle of the night. ...Fine, come in. Behave.");
SCENE(B_NIGHTM_MM,   /* married night knock in mama mode (same pictures as B_NIGHTM) */
    {W_NAR, BG_BDOOR, 0, "It's late at night. I knock softly on our bedroom door."},
    {W_SOPHIA, BG_BDOOR, 0, "...Who is it? If it's that silly name again, go back to bed."},
    {W_SOPHIA, BG_BDOOR, 5, "...It's you. Come in, quickly. And don't say it out loud."},
    {W_NAR, BG_ROM, 0, "I slip inside and close the door behind me. The lamp is low and her room is warm."}
);
POOL(R_HUG_OPEN_MM,
    "I step closer and open my arms. Sophia stiffens and mutters something, then hugs me back anyway.",
    "I rest my head on her shoulder and whisper that name. She flinches, but she doesn't let go.",
    "I slip my arms around her. Sophia sighs as if it were a great burden, and holds on.",
    "I open my arms. Sophia rolls her eyes, but she steps in anyway.",
    "I hold her quietly. She stiffens, then lets out a long breath.",
    "Sophia mutters a complaint, yet her arms close around me.",
    "She stands rigid for a moment, then leans her head on my shoulder.",
    "I pull her into a hug and she pretends she wasn't expecting it.");
POOL(R_KISS_OPEN_MM,
    "Sophia hesitates, her hands hovering, then cups my face and kisses my cheek, flustered.",
    "I whisper that name and she goes red. \"Don't!\" she says, and kisses me before I can say it again.",
    "I tug her sleeve and ask. Sophia looks away and huffs, then kisses me quickly, quietly.",
    "I lean in and kiss her. Sophia's face goes red, but she doesn't stop me.",
    "She huffs, then closes her eyes and lets me kiss her.",
    "I kiss her softly. Sophia clutches my sleeve and says nothing at all.",
    "She mumbles a protest, but it dies against my lips.",
    "Sophia lifts her chin, trying to look unbothered. I kiss her anyway.");
SCENE(B_HUG_MM,   /* beat 1 is filled in from R_HUG_OPEN_MM */
    {W_NAR, BG_HUG, 0, NULL},
    {W_SOPHIA, BG_HUG, 0, "...This doesn't count as anything. And don't you dare call me that while I'm holding you."},
    {W_ME, BG_HUG, 0, "Your arms are the best place in the whole hotel, Mama."},
    {W_NAR, BG_HUG, 0, "She mutters, \"Stop it, I'm Sophia,\" yet she doesn't let go, her ears bright red. For a while neither of us moves."}
);
SCENE(B_KISS_MM,   /* beat 1 is filled in from R_KISS_OPEN_MM */
    {W_NAR, BG_KISS, 0, NULL},
    {W_SOPHIA, BG_KISS, 0, "...Hmph. That was your one. Don't get ideas."},
    {W_ME, BG_KISS, 0, "Thank you, Mama."},
    {W_SOPHIA, BG_KISS, 0, "Don't say that! I'm not that. ...Say my name, and I might allow one more."},
    {W_NAR, BG_KISS, 0, "She kisses me once more anyway, quickly, then hides her red face and refuses to look at me."}
);
SCENE(B_SLEEP_MM,
    {W_NAR, BG_SLEEPT, 0, "It is late and the hotel is quiet. I ask Sophia if I can sleep beside her tonight."},
    {W_SOPHIA, BG_SLEEPT, 0, "...Only if you don't say it. Not once. Understood?"},
    {W_ME, BG_SLEEPT, 0, "Good night, Mama."},
    {W_SOPHIA, BG_SLEEPT, 0, "I SAID... ...Never mind. Good night, husband. And no stealing the blanket."},
    {W_NAR, BG_SLEEPT, 0, "She turns off the light, stiff at first. Slowly she relaxes, and her hand finds mine."},
    {W_NAR, BG_SLEEPT, 0, "I fall asleep to the sound of the rain against the window, wrapped in her warmth."}
);
SCENE(B_FEED_MM,   /* BREASTFEED: beats 1-2 she covers her chest in her room, then the scene picture */
    {W_NAR, BG_ROM, 1, "I sit beside her and ask softly, \"Mama... may I?\" Sophia crosses her arms over her chest, her whole face red."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...I'm not doing this because of that name. I'm doing it because it aches. That's all."},
    {W_NAR, BG_FEED, 0, "Sophia sits on the edge of the bed, avoiding my eyes, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "...Just... just this once. And you don't say a word. Okay? ...I'm still not sure about this."},
    {W_ME, BG_FEED, 0, "I understand. Tell me if it's too much, Sophia."},
    {W_NAR, BG_FEED, 0, "She draws me close, stiff and embarrassed, one hand resting hesitantly in my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Good. Like that. Don't look up at me. ...I still don't like that name."},
    {W_ME, BG_FEED, 0, "Thank you."},
    {W_NAR, BG_FEED, 0, "Afterwards she lets out a long breath and tells me to forget it ever happened. She still holds me a little longer."}
);
/* ---------- v9.9: she is asked in the LIVING ROOM (married; her menu opened by walking up to her): own pictures and lines ----------
   ASK FOR MILK, PUMP MILK and BREASTFEED use these instead of the bedroom scenes. Every one of them starts with one of the R_LR_HERE lines ("Here?!").
   Pictures: assets/sc_feed_lr_d8.png / sc_feed_lr_yoga.png (BREASTFEED) and assets/pump_lr.png (the peek) - placeholders for now (copies of the bedroom ones). */
POOL(R_LR_HERE,
    "Here?! In the living room?! ...Anyone could walk in on us.",
    "Here?! Out in the open like this? ...Hmph. Keep your voice down.",
    "Here?! Have you lost your mind, husband? This is the living room!",
    "H-here?! Can't you wait until we're in my room? ...Fine. But make it quick.",
    "Here?! Out in the living room?! ...Keep it down, husband.",
    "Here?! The window is right there! ...Fine, but be quick.",
    "Here?! In the living room?! You really have no shame, do you?",
    "Here?! Now?! ...Hmph. Check that the door is locked first.");
static const Beat B_LEAD_LR_D[LEAD_N][LEAD_BEATS] = {
    {   {W_NAR,    BG_ROM, 0, "Sophia stops in the middle of the living room and presses her arms tightly against her chest."},
        {W_SOPHIA, BG_ROM, 1, "...It's happening again. It feels so full and heavy that it aches. I can't stand it, husband."},
        {W_ME,     BG_ROM, 1, "That means the pills are working. You'll feel better once you let it out."},
        {W_SOPHIA, BG_ROM, 1, "Don't say it like it's nothing! Hmph... Fine. Just don't you dare look at me."} },
    {   {W_NAR,    BG_ROM, 0, "Sophia fidgets with her collar and keeps glancing at the window, her cheeks turning pink."},
        {W_SOPHIA, BG_ROM, 1, "My blouse is so tight again. It's hot and heavy, and I can't think about anything else."},
        {W_ME,     BG_ROM, 1, "Then it's time to use the pump again, isn't it? It helped last time."},
        {W_SOPHIA, BG_ROM, 1, "I know that already! ...Just keep your eyes on the wall this time."} },
    {   {W_NAR,    BG_ROM, 0, "Sophia lets out a shaky breath and sinks onto the sofa, hugging a cushion to her chest."},
        {W_SOPHIA, BG_ROM, 1, "It feels strange, husband. It feels like it's about to burst. This is all because of your stupid pills."},
        {W_ME,     BG_ROM, 1, "I'll stay right here. Tell me what I can do."},
        {W_SOPHIA, BG_ROM, 1, "...Just turn around and stay quiet. And no peeking."} },
    {   {W_NAR,    BG_ROM, 0, "She takes the pump out of her bag. Her ears are bright red."},
        {W_SOPHIA, BG_ROM, 1, "What, with the pump? Out here? No... I will not do it."},
        {W_ME,     BG_ROM, 1, "Sorry. I want you to feel better."},
        {W_SOPHIA, BG_ROM, 1, "Fine, give the pump to me. Now, turn around and face the wall."} },
    {   {W_NAR,    BG_ROM, 0, "Sophia swallows hard and pulls a face, one hand pressed against her chest."},
        {W_SOPHIA, BG_ROM, 1, "Why is it always worse after this... My chest feels like it's going to explode."},
        {W_ME,     BG_ROM, 1, "Does it still hurt that much?"},
        {W_SOPHIA, BG_ROM, 1, "Of course it does! Hmph. At least the pump makes it better. ...Turn around."} },
    {   {W_NAR,    BG_ROM, 0, "Sophia takes a deep breath, tugs at her clothes and glances at the front door and the window."},
        {W_SOPHIA, BG_ROM, 1, "Make sure the door is locked. If anyone saw this, I would never leave the house again."},
        {W_ME,     BG_ROM, 1, "Nobody will know. It's just the two of us."},
        {W_SOPHIA, BG_ROM, 1, "You'd never know. ...Now, I need to take care of this. Don't look."} }
};
static const Beat (*B_LEAD_LR)[LEAD_BEATS] PQ_UNUSED = B_LEAD_LR_D;
static int B_LEAD_LR_N PQ_UNUSED = LEAD_N;
SCENE(B_BOTTLE_LR,
    {W_NAR,    BG_ROM, 0, "Sophia takes out the breast pump and the baby bottle you gave her and draws the curtains with a jerk. Her ears are already red."},
    {W_SOPHIA, BG_ROM, 1, "...Turn around. Face the sofa. And don't even think about looking."},
    {W_ME,     BG_ROM, 1, "I'm already facing the wall."},
    {W_SOPHIA, BG_ROM, 1, "I still hate this. Why does it have to be in the middle of the living room... Hmph. Just don't turn around!"},
    {W_NAR,    BG_ROM, 0, "I stare at the wall. Behind me I hear the soft hum of the pump and her unsteady breathing."}
);
SCENE(B_PEEKPIC_LR,
    {W_NAR,    BG_PUMP, 0, "I can't help it. I turn my head just a little and glance back over my shoulder."},
    {W_NAR,    BG_PUMP, 0, "Sophia sits on the end of the sofa, her eyes fixed on the bottle and her cheeks bright red."},
    {W_SOPHIA, BG_PUMP, 0, "...Almost... just a little more..."},
    {W_NAR,    BG_PUMP, 0, "She hasn't noticed me yet. Before she does, I quickly turn back to the wall."}
);
SCENE(B_FEED_LR,   /* BREASTFEED in the living room: beat 0 and 1 she covers her chest (the "Here?!" line comes before), then the scene picture */
    {W_NAR,    BG_ROM,  1, "I ask her to breastfeed me. Sophia quickly crosses her arms over her chest and looks at the window, her ears bright red."},
    {W_SOPHIA, BG_ROM,  1, "...Hmph. Fine. You are my husband. Sit on the sofa and don't stare while I get ready."},
    {W_NAR,    BG_FEED, 0, "Sophia sits on the sofa and pats the cushion beside her, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "My chest is so full it aches. You are my husband now, so... just this once, and quietly, in case someone walks in."},
    {W_ME,     BG_FEED, 0, "I won't make a sound. Tell me if it's too much."},
    {W_NAR,    BG_FEED, 0, "She draws me close and holds me against her, one hand gently stroking my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Hmph. Don't look so happy. You're being awfully greedy. And keep an eye on the door."},
    {W_NAR,    BG_FEED, 0, "Afterwards she lets out a long, relieved breath, and I stay in her arms a little longer. The ache is gone."}
);
SCENE(B_FEED_MM_LR,   /* mama mode, living room: she never says mama about herself */
    {W_NAR, BG_ROM, 1, "I sit beside her on the sofa and ask softly, \"Mama... may I?\" Sophia crosses her arms over her chest, her whole face red."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...And not out here, either. I'm only doing this because it aches. That's all."},
    {W_NAR, BG_FEED, 0, "Sophia sits on the sofa, avoiding my eyes, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "...Just... just this once. And if anyone knocks, we stop. Okay? ...I'm still not sure about this."},
    {W_ME, BG_FEED, 0, "I understand. Tell me if it's too much, Sophia."},
    {W_NAR, BG_FEED, 0, "She draws me close, stiff and embarrassed, one hand resting hesitantly in my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Good. Like that. Don't look up at me. ...I still don't like that name."},
    {W_ME, BG_FEED, 0, "Thank you."},
    {W_NAR, BG_FEED, 0, "Afterwards she lets out a long breath and tells me to forget it ever happened. She still holds me a little longer."}
);
/* ---------- v10.4: mama-mode OPENERS. Each opener is a PAIR of beats: your line (it has Mama in it), then her reaction, which starts with her reaction to the name. ----------
   One pair is picked at random (never the same twice in a row). Living room: ASK FOR MILK / PUMP MILK / MILK PILL / BREASTFEED. Door and bedroom: BREASTFEED. */
/* v10.6: stronger first rejection, a plea + her answer after it, openers at the door / bedroom for ASK and PILL, a memory version (B_RM_*) when you asked before, and mid-scene hesitation (B_MID_*). */
SCENE(B_OPEN_FEED_MM,
    {W_ME, BG_ROM, 1, "Mama... may I? I'm so hungry."},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! Don't call me that! No. Absolutely not. I'm not ready for this, and I won't be."},
    {W_ME, BG_ROM, 1, "Mama, please... can I have some?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! Some?! You can't just ask like that! No. Go away."},
    {W_ME, BG_ROM, 1, "Mama... I'm really hungry. May I?"},
    {W_SOPHIA, BG_ROM, 1, "Don't say that name! I told you not to! Hungry? Go and eat something else. Not from me."},
    {W_ME, BG_ROM, 1, "Mama... can I rest against you? Just for a while?"},
    {W_SOPHIA, BG_ROM, 1, "W-what?! Don't call me that! Rest?! You know exactly what that means. No, no, no."},
    {W_ME, BG_ROM, 1, "Mama... it's just us. May I have some now?"},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! It is not alright just because we are alone! My answer is no."},
    {W_ME, BG_ROM, 1, "Mama, I'll be gentle. May I?"},
    {W_SOPHIA, BG_ROM, 1, "Mama?! No! Stop that! Gentle or not, I am not doing this. Not while you say that word."}
);
SCENE(B_OPEN_FEED_MM_LR,
    {W_ME, BG_ROM, 1, "Mama... may I? My stomach feels so empty."},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! Don't call me that! Here?! Absolutely not. Anyone could walk in. No."},
    {W_ME, BG_ROM, 1, "Mama, please... can I have some?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! Out here?! Are you out of your mind? No. Not here."},
    {W_ME, BG_ROM, 1, "Mama... I'm really hungry. May I?"},
    {W_SOPHIA, BG_ROM, 1, "Don't say that name! Here? In the living room?! I said no. Don't ask again."},
    {W_ME, BG_ROM, 1, "Mama... can I rest against you? Just for a while?"},
    {W_SOPHIA, BG_ROM, 1, "W-what?! Don't call me that! Rest here?! You know where that leads. No!"},
    {W_ME, BG_ROM, 1, "Mama... it's just us. May I have some now?"},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! Just us? Here? The door could open any second! No way."},
    {W_ME, BG_ROM, 1, "Mama, I'll be quick. May I?"},
    {W_SOPHIA, BG_ROM, 1, "Mama?! No! Quick? Here?! That makes it worse! I refuse. Absolutely not."}
);
SCENE(B_OPEN_ASK_MM_LR,
    {W_ME, BG_ROM, 1, "Mama... could you give me some milk? Please?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! Milk?! Here?! In the open? Absolutely not."},
    {W_ME, BG_ROM, 1, "Mama, is it full yet? I'd like a bottle."},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! A bottle?! Here, with the curtains open? No. Never."},
    {W_ME, BG_ROM, 1, "Mama... I'd love some of your milk today."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! Here?! Someone could see! No. Not out here."},
    {W_ME, BG_ROM, 1, "Mama, I'm thirsty. Can you pump some for me?"},
    {W_SOPHIA, BG_ROM, 1, "Mama?! No! Don't call me that! Pump?! Here?! Have you lost your mind?"},
    {W_ME, BG_ROM, 1, "Mama... my bottle is empty. Could you fill it?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! Fill it out here?! Absolutely not! Do you want the whole hotel to know?"},
    {W_ME, BG_ROM, 1, "Mama, could you pump some for me today?"},
    {W_SOPHIA, BG_ROM, 1, "Mama?! Stop that! Pump here?! In the living room?! No. Not a chance."}
);
SCENE(B_OPEN_PILL_MM_LR,
    {W_ME, BG_ROM, 1, "Mama, here's your pill. Take it for me?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! I'm not taking anything from you in the middle of the living room!"},
    {W_ME, BG_ROM, 1, "Mama... time for your pill. Open up."},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! Open up?! I am not a child. Keep that pill away from me."},
    {W_ME, BG_ROM, 1, "Mama, please take this one. For both of us."},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! Both of us?! No! I'm not swallowing anything while you say that."},
    {W_ME, BG_ROM, 1, "Mama, your pill. I'll hold the water for you."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! I can hold my own glass. And no, not out here. Not now."},
    {W_ME, BG_ROM, 1, "Mama... I brought your medicine. Please."},
    {W_SOPHIA, BG_ROM, 1, "Stop it with that name! Medicine?! Don't pretend this is innocent. I refuse."},
    {W_ME, BG_ROM, 1, "Mama, take one for me. Just one."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! For you? Never. Put it away. Someone could walk in."}
);
SCENE(B_OPEN_ASK_MM,
    {W_ME, BG_ROM, 1, "Mama... could you give me some milk today? Please?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! I told you, I am not your mother. ...And no. Not today. Absolutely not."},
    {W_ME, BG_ROM, 1, "Mama, is it full yet? I'd like a bottle."},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! Do you hear how that sounds?! I'm not doing anything while you say it."},
    {W_ME, BG_ROM, 1, "Mama... I would love some of your milk."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! My answer is no. Go away and think about what you just said."},
    {W_ME, BG_ROM, 1, "Mama, I'm thirsty. Could you pump some for me?"},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! No! Never! I'm not pumping anything for you. Not while you use that word."},
    {W_ME, BG_ROM, 1, "Mama... my bottle is empty. Could you fill it?"},
    {W_SOPHIA, BG_ROM, 1, "Stop it with that name! Fill it yourself! ...That is not my job and I will not do it."},
    {W_ME, BG_ROM, 1, "Mama, could you pump some for me today?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! Why would I pump for someone who talks like that? No. Not a chance."}
);
SCENE(B_OPEN_PILL_MM,
    {W_ME, BG_ROM, 1, "Mama, here's your pill. Please take it."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! I am not taking anything from someone who says that word."},
    {W_ME, BG_ROM, 1, "Mama... time for your pill. Open up."},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! Open up?! I am not a child. Keep your pill, and keep your mouth shut."},
    {W_ME, BG_ROM, 1, "Mama, please take this one. It will help your chest."},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! No. I have had enough of your pills and your names. Go away."},
    {W_ME, BG_ROM, 1, "Mama, your pill. I'll fetch the water for you."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! I'll fetch my own water, thank you. And no, not right now."},
    {W_ME, BG_ROM, 1, "Mama... I brought your medicine. Please don't be angry."},
    {W_SOPHIA, BG_ROM, 1, "Stop it with that name! Angry? I'm furious! Put the bottle away before I throw it at you."},
    {W_ME, BG_ROM, 1, "Mama, take one for me. Just one."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! For you? Why would I? Absolutely not. I refuse."}
);
SCENE(B_RM_FEED_MM,
    {W_ME, BG_ROM, 1, "Mama... may I feed again, like before?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...I remember the last time. I said never again, and here you are, asking again."},
    {W_ME, BG_ROM, 1, "Mama, it's me again. May I?"},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! You asked me this recently, and I still haven't forgotten how embarrassed I was."},
    {W_ME, BG_ROM, 1, "Mama... I'm hungry again. Please?"},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! Already? I remember every single time you've asked. You are shameless."},
    {W_ME, BG_ROM, 1, "Mama, you let me before. Could you once more?"},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! Don't use that against me! Yes, I did, and I regretted it right after."},
    {W_ME, BG_ROM, 1, "Mama... you said it was the last time. Just once more?"},
    {W_SOPHIA, BG_ROM, 1, "Stop it with that name! I did say that! And you promised to stop asking. Liar."},
    {W_ME, BG_ROM, 1, "Mama, I'll be as gentle as last time. Please?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...You were gentle last time. That is the only reason I'm still listening."}
);
SCENE(B_RM_ASK_MM,
    {W_ME, BG_ROM, 1, "Mama... could I have some milk again, like before?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...I remember the last time. I said never again, and here you are."},
    {W_ME, BG_ROM, 1, "Mama, it's me again. May I have a bottle?"},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! You asked me this recently, and I still haven't forgotten how embarrassed I was."},
    {W_ME, BG_ROM, 1, "Mama... I'm out again. Could you pump for me?"},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! Already? I remember every time you've asked. You are shameless."},
    {W_ME, BG_ROM, 1, "Mama, you did it before. Could you do it once more?"},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! Don't use that against me! Yes, I did it before, and I regretted it right after."},
    {W_ME, BG_ROM, 1, "Mama... you said it was the last time. Once more?"},
    {W_SOPHIA, BG_ROM, 1, "Stop it with that name! I did say that! And you promised you'd stop asking. Liar."},
    {W_ME, BG_ROM, 1, "Mama, I'll be as quiet as last time. Please?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...You were quiet last time, yes. That is the only reason I'm still listening."}
);
SCENE(B_RM_PILL_MM,
    {W_ME, BG_ROM, 1, "Mama, it's pill time again. You know the one."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! I know exactly which one. I remember how it felt last time, thank you very much."},
    {W_ME, BG_ROM, 1, "Mama... one more pill, like before?"},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! Do not bring up last time. I have not forgotten what that pill did to me."},
    {W_ME, BG_ROM, 1, "Mama, you took it before without any trouble."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! Without trouble?! I remember every second of it. Never remind me."},
    {W_ME, BG_ROM, 1, "Mama, I brought your pill again."},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! Again?! I can still feel the last one. Do you want me to burst?"},
    {W_ME, BG_ROM, 1, "Mama, please, the same pill as last time."},
    {W_SOPHIA, BG_ROM, 1, "Stop it with that name! Last time I said it was the last time. You never listen."},
    {W_ME, BG_ROM, 1, "Mama... you promised you'd take the next one."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...I did promise, didn't I. I hate that I remember that."}
);
SCENE(B_PLEA_FEED_MM,
    {W_ME, BG_ROM, 1, "Please, Mama... just a little. I'll be so quiet."},
    {W_SOPHIA, BG_ROM, 1, "...Stop pleading like that. It's unfair. ...Fine. But only because it aches. Don't look at me."},
    {W_ME, BG_ROM, 1, "Mama, please... I'll be so good. I promise."},
    {W_SOPHIA, BG_ROM, 1, "Hmph. Don't make that face. ...Just this once. And you tell nobody."},
    {W_ME, BG_ROM, 1, "Please, Sophia... Mama... I only want to be close to you."},
    {W_SOPHIA, BG_ROM, 1, "...Close. You make it sound so harmless. ...Fine. Sit down. Quietly."},
    {W_ME, BG_ROM, 1, "Mama, I'm begging you. Please, I need you."},
    {W_SOPHIA, BG_ROM, 1, "Begging does not suit you. ...But fine. Come here, before I come to my senses."},
    {W_ME, BG_ROM, 1, "Please, Mama... I'll say thank you afterwards."},
    {W_SOPHIA, BG_ROM, 1, "You'd better. ...Ugh, I can't believe I'm agreeing to this. Sit. Now."},
    {W_ME, BG_ROM, 1, "Mama, please... just for me? Only for me?"},
    {W_SOPHIA, BG_ROM, 1, "...Only for you. Don't you dare let that go to your head. Fine. Come here."}
);
SCENE(B_PLEA_ASK_MM,
    {W_ME, BG_ROM, 1, "Please, Mama... I'll turn around and not say a word. Just this once."},
    {W_SOPHIA, BG_ROM, 1, "...Stop pleading like that. It's unfair. ...Fine. Face the wall and don't turn around."},
    {W_ME, BG_ROM, 1, "Mama, please... I'll be so good. I only want a little bottle."},
    {W_SOPHIA, BG_ROM, 1, "Hmph. Don't make that face. ...I'm only doing it because it aches. Turn around."},
    {W_ME, BG_ROM, 1, "Please, Sophia... Mama... I'll never ask for anything else today."},
    {W_SOPHIA, BG_ROM, 1, "You always say that! ...Ugh. Fine. One peek and I stop. Turn around."},
    {W_ME, BG_ROM, 1, "Mama, I'm begging you. Just one bottle, I'll be quiet."},
    {W_SOPHIA, BG_ROM, 1, "Begging does not suit you. ...But fine. I'll get the pump. Don't follow me."},
    {W_ME, BG_ROM, 1, "Please, Mama... I'll be gentle. I'll even say thank you."},
    {W_SOPHIA, BG_ROM, 1, "...Gentle? With a pump? You are impossible. Fine. Turn around, and I mean it."},
    {W_ME, BG_ROM, 1, "Mama, please... just for me? I'll do anything you ask afterwards."},
    {W_SOPHIA, BG_ROM, 1, "Anything? ...Don't promise what you can't keep. Fine. Wall. Now."}
);
SCENE(B_PLEA_PILL_MM,
    {W_ME, BG_ROM, 1, "Please, Mama... it's the only thing that eases the ache. One pill."},
    {W_SOPHIA, BG_ROM, 1, "...Don't use my chest against me. That's low. ...Give it here. And turn around."},
    {W_ME, BG_ROM, 1, "Mama, please... I hate seeing you uncomfortable. Just one."},
    {W_SOPHIA, BG_ROM, 1, "Hmph. Don't act so concerned. ...Fine. One. Not two. Now give me some space."},
    {W_ME, BG_ROM, 1, "Please, Sophia... Mama... I'll stay here until it helps."},
    {W_SOPHIA, BG_ROM, 1, "You'll stay? ...Ugh. Fine. I'll swallow it. But you are not allowed to watch."},
    {W_ME, BG_ROM, 1, "Mama, I'll never ask again today. Please just take it."},
    {W_SOPHIA, BG_ROM, 1, "You always say that. ...Give me the glass. Turn around, and don't breathe."},
    {W_ME, BG_ROM, 1, "Please, Mama... I'll carry everything for you from now on."},
    {W_SOPHIA, BG_ROM, 1, "I do not need a pet. ...But fine, hand it over. Quickly, before I change my mind."},
    {W_ME, BG_ROM, 1, "Mama... please. I worry about you. Take it for me."},
    {W_SOPHIA, BG_ROM, 1, "...Worry? Hmph. That is no excuse. ...But fine. Just this one."}
);
SCENE(B_MID_PUMP_MM,
    {W_ME, BG_ROM, 1, "Mama... take your time. I'm not looking. Please don't stop."},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! ...I can't do this while you keep talking. ...Ugh. Fine. Keep still."},
    {W_ME, BG_ROM, 1, "Mama, are you alright? Please, don't give up now."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! I... this is so embarrassing. ...Fine. Don't turn around."},
    {W_ME, BG_ROM, 1, "Mama... please, you'll feel better afterwards. Try again."},
    {W_SOPHIA, BG_ROM, 1, "Stop it with that name! ...I want to stop. ...But it does ache. Don't you dare turn."},
    {W_ME, BG_ROM, 1, "Mama, just a little more. I'm right here, facing the wall."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...Hmph. A little more. Only because it hurts. Stay turned."},
    {W_ME, BG_ROM, 1, "Mama, please don't be shy. I won't look. I promise."},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! I'm not shy! I'm... ...Ugh. Be quiet. Let me focus."},
    {W_ME, BG_ROM, 1, "Mama... you're doing so well. Please keep going."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! Stop talking. ...You make it harder. ...Fine. Don't move."}
);
SCENE(B_MID_FEED_MM,
    {W_ME, BG_ROM, 1, "Mama... just a little longer. Please?"},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! ...I wasn't going to... ugh. Just a little longer. Not one second more."},
    {W_ME, BG_ROM, 1, "Mama, please... don't stop yet."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! ...I want to stop. ...But fine. Only a moment. Keep still."},
    {W_ME, BG_ROM, 1, "Mama... it feels so warm. Please, stay like this."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...You are making me so embarrassed. ...Fine. Stay. But hush."},
    {W_ME, BG_ROM, 1, "Mama, I'm not finished. May I have a little more?"},
    {W_SOPHIA, BG_ROM, 1, "Stop it with that name! ...Not finished? You are greedy. ...Fine. A little more."},
    {W_ME, BG_ROM, 1, "Mama... please, a bit longer. I'll behave."},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! You said that before! ...Hmph. Behave, then. Quietly."},
    {W_ME, BG_ROM, 1, "Please, Mama... just five more minutes."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! Five minutes?! ...Three. Not a minute more."}
);
POOL(R_SLEEP_NO_TEASE_MM,
    "After what you said today? Sleep alone tonight.",
    "No. You embarrassed me today. Alone. Good night.",
    "Not after those shameless comments. Go to bed alone.",
    "Not after the things you said today. Go to bed alone.",
    "No. Think about your teasing, then sleep alone.",
    "You were shameless today. Sleep alone tonight, husband.",
    "No sleeping together tonight. You know exactly why.");
POOL(R_SLEEP_NO_MILK_MM,
    "Not tonight. I'm still sore after the milk.",
    "My chest still aches from today. Come back tomorrow night.",
    "No. After everything today, I want to sleep alone.",
    "No. Today was enough for me. I want to sleep alone.",
    "I'm drained, husband. Come back tomorrow night.",
    "Not tonight. Today was more than enough for one person.");
POOL(R_KICKED_MM,
    "You're still in trouble, husband. ...Come in, quietly.",
    "I'm still blushing, so no talking about earlier.",
    "Back already? Hmph. Behave this time, I mean it.",
    "I haven't forgiven you yet. ...Come in anyway.",
    "Don't mention earlier. And not that name. Not a word.",
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
    "You shameless idiot. ...Come in, then.");
POOL(R_NOT_FULL_MM, "It isn't full yet. Come back when the gauge is full.", "Not yet. Come back when I'm full. And don't pout.", "Not full. Don't ask again until it is.");
POOL(R_OUTFIT_YOGA_MM,
    "...It's on. It's so thin it shows everything. And no names, ever.",
    "There. Don't look. And stop using that word for me.",
    "Hmph. Half naked, and you still say that name?");
POOL(R_OUTFIT_D8_MM, "Back to my usual clothes, then. Don't say a word.", "Back to normal clothes. And not a word, husband.");
POOL(R_OUTFIT_COW_MM,
    "...Done. Ugh. And you used that name while I put this on. I feel disgusting.",
    "There. A cow, and with that name too, apparently. Is that what you wanted? Revolting.",
    "It's on. I can't stand it. Don't look at me and don't say that word.",
    "Hmph. Cow spots on me and that name in your mouth. I want to hide under a blanket.",
    "Done. I hate how this feels. One comment from you and it goes straight in the bin.",
    "There. I'm sickened with myself. Turn around, and keep that name to yourself.");
POOL(R_OUTFIT_TOP_MM,
    "...It's on. It's so thin and snug. And not one word about my chest.",
    "There. My chest is on display and you still use that name. Don't stare.",
    "Hmph. I feel exposed. Stare once and I'm changing back.");

/* v11.2: the COW PRINT gift, once she has taken the beauty pill (her chest is swollen and stuffy, she suspects the pill): she thinks of milk (3 beats, 5 versions each) */
POOL(GO_COW_MILK,
    "Cow print, and my chest is heavy and sore from that pill? Ugh... Is this what you were thinking all along?",
    "A cow outfit right after that pill. ...Disgusting. Do you see me as a cow now?",
    "Cow print on top of this swollen chest... I feel sick. Don't you dare say what that pill is for.",
    "Spots and a cutout, and you know what that pill did to my chest. Ew. How low can you go?",
    "Cows are for milk, and I'm... no. No! This is revolting. Why this, right after that pill?",
    "I'm already sore from that pill, and now cow spots? ...I'm so disgusted I could scream.");
POOL(GL_COW_MILK,
    "Say it. Say you picked this because of my chest, and watch how fast you regret it. Ugh!",
    "Cows give milk. Don't you dare say it out loud. I'm disgusted enough already.",
    "Do you think a cow is what that pill has turned me into? ...That's vile. Absolutely vile.",
    "My chest hurts, my head spins, and you hand me THIS? I can't even look at it. Disgusting.",
    "If you planned this with that pill, I will never forgive you. It makes me physically sick.",
    "Spots on a swollen chest... Ugh. Take that smile off your face before I take it off for you.");
POOL(GE_COW_MILK,
    "She covers her chest with the bag, shivering with disgust, then slams the door.",
    "She glares at the spots with revulsion, then at me, and shuts the door hard.",
    "She presses the bag to her swollen chest, gags at the smell of new fabric, and storms inside.",
    "She hisses that I make her sick, then slams the door behind her, outfit and all.",
    "She goes pale, wrinkles her nose, and takes it inside, holding it far from her body.",
    "She shudders, says nothing, and closes the door so slowly that it feels worse than a slam.");
/* v11.2: CHANGE OUTFIT (married): asking her to wear the yoga outfit, the designer top or the cow print outfit is a short exchange. Groups of 3 beats:
   your request (ME), her refusal (SOPHIA), her grudging give-in (SOPHIA); one group is picked at random (never the same twice in a row). The outfit changes when the exchange ends.
   Mama mode (_MM): you say Mama and she rejects the name first. Going back to her usual outfit has no exchange. */
SCENE(B_WEAR_YOGA,
    {W_ME, BG_ROM, 1, "Sophia, could you wear the yoga outfit for me?"},
    {W_SOPHIA, BG_ROM, 1, "Absolutely not! It's so thin it might as well not exist, and it shows far too much skin. Ask again and I'll burn it."},
    {W_SOPHIA, BG_ROM, 1, "...Don't stand there grinning. Fine. One time. Turn around, and don't you dare look until I say."},
    {W_ME, BG_ROM, 1, "Would you put on the yoga outfit today? Just for me?"},
    {W_SOPHIA, BG_ROM, 1, "For you? In front of you? It shows my waist, my legs, everything! Are you out of your mind?"},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. You won't stop asking, will you? Fine. But if you stare, I'm changing back at once."},
    {W_ME, BG_ROM, 1, "I'd like to see you in the yoga outfit, Sophia."},
    {W_SOPHIA, BG_ROM, 1, "You'd like to SEE... Shameless! That fabric is so thin it's practically see-through."},
    {W_SOPHIA, BG_ROM, 1, "Ugh. ...Fine. Only because I'm tired of arguing. Face the wall. NOW."},
    {W_ME, BG_ROM, 1, "Sophia, the yoga outfit... please?"},
    {W_SOPHIA, BG_ROM, 1, "No. No, no, no. I'm a married woman, not a showgirl. It shows way too much skin."},
    {W_SOPHIA, BG_ROM, 1, "...Don't make that face. Fine, fine! Turn around. And not one comment, husband."},
    {W_ME, BG_ROM, 1, "Could you try the yoga outfit again today?"},
    {W_SOPHIA, BG_ROM, 1, "Again?! Do you enjoy humiliating me? It's so thin I can feel the air right through it!"},
    {W_SOPHIA, BG_ROM, 1, "...Fine. But you owe me for this. Turn around and keep your hands to yourself."}
);
SCENE(B_WEAR_TOP,
    {W_ME, BG_ROM, 1, "Sophia, would you wear the designer top for me?"},
    {W_SOPHIA, BG_ROM, 1, "That scrap? It's so thin and so tight that everything shows. I'd rather go out in a towel."},
    {W_SOPHIA, BG_ROM, 1, "...Stop staring at me like a puppy. Fine. Turn around. And it's only because I'm kind."},
    {W_ME, BG_ROM, 1, "Could you put on the designer top today?"},
    {W_SOPHIA, BG_ROM, 1, "No. The neckline is far too low and the fabric is paper-thin. Every curve on display!"},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. You really won't give up. Fine, I'll wear it. Don't you dare look at my chest."},
    {W_ME, BG_ROM, 1, "I'd love to see you in the designer top."},
    {W_SOPHIA, BG_ROM, 1, "I'm sure you would, pervert. It reveals way too much skin. I refuse."},
    {W_SOPHIA, BG_ROM, 1, "...Don't pout. Ugh. Fine. Turn around, and the moment you stare, I'm taking it off. I mean it!"},
    {W_ME, BG_ROM, 1, "Sophia, please wear the designer top. Just once more."},
    {W_SOPHIA, BG_ROM, 1, "Once more?! I felt naked the last time. The fabric is far too thin!"},
    {W_SOPHIA, BG_ROM, 1, "...Fine. But don't you dare comment on it. Not a single word."},
    {W_ME, BG_ROM, 1, "Would you wear the designer top for me? Please."},
    {W_SOPHIA, BG_ROM, 1, "A top that shows my skin, my shoulders, my chest... and you say please as if that makes it fine?"},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. Turn around. I'm doing this under protest, husband."}
);
SCENE(B_WEAR_COW,
    {W_ME, BG_ROM, 1, "Sophia, would you wear the cow print outfit for me?"},
    {W_SOPHIA, BG_ROM, 1, "The cow print?! Ugh... just hearing it turns my stomach. Do you have any idea how degrading that is?"},
    {W_SOPHIA, BG_ROM, 1, "...Fine. But only because I said yes before. Turn around. And don't you dare enjoy it."},
    {W_ME, BG_ROM, 1, "Could you put on the cow outfit today?"},
    {W_SOPHIA, BG_ROM, 1, "That revolting thing again? Spots, a cutout... I can't stand even touching it. No."},
    {W_SOPHIA, BG_ROM, 1, "...Ugh. Fine. One hour. Face the wall and don't breathe a word."},
    {W_ME, BG_ROM, 1, "I'd love to see you in the cow outfit."},
    {W_SOPHIA, BG_ROM, 1, "You'd love to see me humiliated, you mean. How disgusting. Do you get a kick out of it?"},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. Fine. Turn around. I'm doing this with my eyes shut and my teeth clenched."},
    {W_ME, BG_ROM, 1, "Sophia, the cow print outfit... would you?"},
    {W_SOPHIA, BG_ROM, 1, "I'd rather wear a trash bag. That outfit is repulsive, and so is the idea behind it."},
    {W_SOPHIA, BG_ROM, 1, "...Fine. Fine! But I will hate every second of it. Turn around."},
    {W_ME, BG_ROM, 1, "Would you wear the cow outfit again for me?"},
    {W_SOPHIA, BG_ROM, 1, "Again?! Ew. I can still feel the fabric on my skin from last time. Why do you do this to me?"},
    {W_SOPHIA, BG_ROM, 1, "...Ugh. Fine. Face the wall. If you smile, I'm changing back instantly."},
    {W_ME, BG_ROM, 1, "Please, Sophia. Just the cow outfit, for me."},
    {W_SOPHIA, BG_ROM, 1, "Please?! You say please for something this vile? I feel sick just picturing myself in it."},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. Fine. Don't look. Don't talk. And don't you dare be pleased about it."}
);
SCENE(B_WEAR_YOGA_MM,
    {W_ME, BG_ROM, 1, "Mama, will you wear the yoga outfit for me?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! And no! It's so thin it shows everything. Ask again and I'll burn it."},
    {W_SOPHIA, BG_ROM, 1, "...Fine. Fine! Turn around, and don't you dare use that word while I change."},
    {W_ME, BG_ROM, 1, "Mama, please put on the yoga outfit today."},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! Stop it with that name! That outfit shows far too much skin. I refuse!"},
    {W_SOPHIA, BG_ROM, 1, "...Ugh. You never give up. One time. Turn around, and no names."},
    {W_ME, BG_ROM, 1, "Mama... I want to see you in the yoga outfit."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! See me in it?! It's so thin you can almost see through it. Absolutely not."},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. Fine, only to shut you up. Face the wall. And not one word."},
    {W_ME, BG_ROM, 1, "Mama, could you wear the yoga outfit again?"},
    {W_SOPHIA, BG_ROM, 1, "Again?! And you still say that name! It's so thin I felt the air through it. Indecent!"},
    {W_SOPHIA, BG_ROM, 1, "...Fine. But after this you owe me an apology. Turn around."},
    {W_ME, BG_ROM, 1, "Mama, the yoga outfit, please."},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! No. I'm not showing that much skin to anyone."},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. To you, then, and only because you're pathetic. Turn around. No names."}
);
SCENE(B_WEAR_TOP_MM,
    {W_ME, BG_ROM, 1, "Mama, would you wear the designer top for me?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! The top is too thin and shows far too much of my chest. Never."},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. Fine. Turn around, and keep that name out of your mouth while I change."},
    {W_ME, BG_ROM, 1, "Mama, could you put on the designer top today?"},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! No! The neckline is shameless and the fabric is paper-thin. I refuse!"},
    {W_SOPHIA, BG_ROM, 1, "...You are impossible. Fine. Turn around. And not a word about my chest."},
    {W_ME, BG_ROM, 1, "Mama... I'd love to see the designer top on you."},
    {W_SOPHIA, BG_ROM, 1, "Stop it with that name! See it on me?! It shows every curve. Pervert."},
    {W_SOPHIA, BG_ROM, 1, "...Ugh. Fine. One time. Face the wall until I say."},
    {W_ME, BG_ROM, 1, "Mama, please wear the designer top again."},
    {W_SOPHIA, BG_ROM, 1, "Again?! And with that name? I felt naked in it. It's so thin!"},
    {W_SOPHIA, BG_ROM, 1, "...Fine. Don't you dare comment. And no names."},
    {W_ME, BG_ROM, 1, "Mama, the designer top, please?"},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! My skin, my shoulders, my chest... it shows it all. No!"},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. Turn around. I'm only doing this so you'll stop whining."}
);
SCENE(B_WEAR_COW_MM,
    {W_ME, BG_ROM, 1, "Mama, would you wear the cow print outfit for me?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! And a cow outfit?! Ugh, that is revolting. I'm not your mama and I'm certainly not a cow."},
    {W_SOPHIA, BG_ROM, 1, "...Fine. But only because I'm too tired to argue. Turn around and keep that name to yourself."},
    {W_ME, BG_ROM, 1, "Mama, could you put on the cow outfit today?"},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! And that disgusting outfit again? I can't stand the sight of it."},
    {W_SOPHIA, BG_ROM, 1, "...Ugh. Fine. Face the wall. One more name from you and I'll tear it off."},
    {W_ME, BG_ROM, 1, "Mama... I want to see you in the cow outfit."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! Cow spots and that name together? That's degrading. I feel dirty just hearing it."},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. Fine. Eyes on the wall, mouth shut, and no names."},
    {W_ME, BG_ROM, 1, "Mama, the cow print outfit... would you?"},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! No! Not in that revolting thing. Do you hear what you're asking?"},
    {W_SOPHIA, BG_ROM, 1, "...Ugh. Fine. But I'm doing it with my eyes closed. Don't turn around."},
    {W_ME, BG_ROM, 1, "Mama, wear the cow outfit again for me?"},
    {W_SOPHIA, BG_ROM, 1, "Again?! With that name?! Ew. I hate how it feels, and I hate how you look at me in it."},
    {W_SOPHIA, BG_ROM, 1, "...Fine. Turn around. And stop calling me that. Not one word."},
    {W_ME, BG_ROM, 1, "Please, Mama. Just the cow outfit."},
    {W_SOPHIA, BG_ROM, 1, "Don't you dare say that name while you ask for that! It's vile. Both of them are vile."},
    {W_SOPHIA, BG_ROM, 1, "...Hmph. Fine. Don't look. Don't talk. And don't you dare look pleased."}
);
POOL(R_MORE_M,
    "Anything else, husband? Hurry up.",
    "Hmph. Are you done talking, dear?",
    "Well, husband? Make your move.",
    "I'm still here. Obviously. Where would I go?",
    "Ask me something else. Or sit down and be quiet.",
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
    "Go on. Surprise me.");
POOL(R_MORE_MM,
    "Anything else? And no silly names this time.",
    "Hmph. Go on, say something else. Normally.",
    "Your turn, husband. What else is on your mind?",
    "I'm still here. ...Not that I'd go anywhere.",
    "Ask me something else. Or just sit with me.",
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
    "Sit with me. And if you must talk, talk normally.");

/* ---------- v9.7: she refuses the name MAMA. A flat refusal of BREASTFEED, her memory of your last mama-mode interaction (today / yesterday), and the table lines of the home match ---------- */
POOL(R_FEED_NO_MM,
    "No. Not while you keep calling me that. Ask again later.",
    "Absolutely not. I'm not in the mood. Try again later.",
    "Don't even ask. ...Not now. Maybe later. Maybe.",
    "No. I need to think about what you've been calling me.",
    "Out of the question right now. Come back later, husband.",
    "No. Absolutely not. And don't make that face.",
    "Not today. ...Not with that name in your mouth.",
    "I said no. A woman is allowed to say no, husband.",
    "Don't ask me that in that voice. No. Later.",
    "Not now. Ask when you've earned it.",
    "Hmph. You think asking sweetly will work? No.",
    "No. I'm not in the mood, and you know why. Later.",
    "Out of the question. Come back when you've learned manners.");
POOL(R_FEED_AGAIN_MM,
    "I just said no. Come back later, husband.",
    "Asking again won't change it. Later.",
    "Still no! Wait until later, and ask better.",
    "I said no already. Don't pester me.",
    "Asking twice doesn't make me say yes. Later.",
    "No means no, husband. Come back in a while.",
    "Are you deaf? I told you no. Later.",
    "Stop asking. Ask again later, if you must.",
    "Still no. Wait until I'm in a better mood.");
POOL(R_MEM_CALL_T_MM,
    "Are you going to call me that again? I'm warning you.",
    "I haven't forgotten what you called me. Behave.",
    "Come in. ...Not a word of that name from earlier.",
    "You said it earlier. Say it again and see what happens.",
    "That name from before... I'm still thinking about it.");
POOL(R_MEM_CALL_Y_MM,
    "You called me that yesterday. I still haven't forgiven you.",
    "Come in. ...And don't start with yesterday's silly name.",
    "I remember what you called me yesterday. Don't you dare repeat it.",
    "Yesterday you called me that. I'm still stewing.",
    "Don't start that name again like you did yesterday.");
POOL(R_MEM_CHAT_T_MM,
    "Back to talk again? And no \"mama\" this time.",
    "Still thinking about our talk? Come in. Keep that name away.",
    "Our chat earlier... don't read too much into it.",
    "We talked earlier. You called me that again, didn't you?",
    "I've had a thought about our chat. Don't tempt me.");
POOL(R_MEM_CHAT_Y_MM,
    "We talked yesterday, and you called me that every time. Don't.",
    "Come in. After yesterday's chat, I'm watching your mouth.",
    "Hmph. You kept calling me that yesterday. Don't start.",
    "Yesterday's chat was fine. Until that name. Remember?",
    "You talked my ear off yesterday. And used that name.");
POOL(R_MEM_TEASE_Y_MM,
    "You teased me yesterday. Don't think I forgot. Come in. Quietly.",
    "After yesterday's nonsense I'm still sensitive. Behave.",
    "I'm still angry about yesterday. ...Come in anyway.",
    "Yesterday you teased me without mercy. Don't repeat it.",
    "I'm still annoyed about your teasing yesterday. Come in.");
POOL(R_MEM_FEED_T_MM,
    "Don't look at me like that. What happened earlier stays between us.",
    "About earlier... we don't talk about it. Not one word.",
    "My face is still hot. And don't call me that.",
    "Earlier doesn't count as anything. Do you hear me?",
    "I'm still flustered from earlier. Don't talk about it.");
POOL(R_MEM_FEED_Y_MM,
    "About yesterday... I don't want to talk about it. Come in.",
    "You said that word again yesterday. I'm still embarrassed.",
    "If you mention yesterday I'll throw you out. Sit down.",
    "Yesterday... was a one-time thing. Don't get used to it.",
    "I still can't look at you after yesterday. Come in anyway.");
POOL(R_MEM_FEEDNO_T_MM,
    "I told you no earlier. Don't try again so soon.",
    "Still no, if that's why you're here. Come in anyway.",
    "Not yet. I haven't decided what to do about you.",
    "I said no earlier. My answer hasn't changed.",
    "Don't look so hopeful. I refused you a minute ago.");
POOL(R_MEM_FEEDNO_Y_MM,
    "I refused you yesterday. Today I'm in a better mood. Maybe.",
    "Yesterday you asked and I said no. Remember that.",
    "About yesterday... you're lucky I'm calm today. Come in.",
    "I turned you down yesterday. Don't sulk about it today.",
    "Yesterday's no was a no. Come in, but behave.");
POOL(R_MEM_HUG_T_MM,
    "Don't read into that hug earlier. It meant nothing.",
    "About that hug... don't mention it. Come in.",
    "Sit. And no clinging this time. ...Maybe a little.",
    "That hug earlier was nothing. Don't get any ideas.",
    "Earlier was just... an accident. Sit.");
POOL(R_MEM_HUG_Y_MM,
    "Yesterday's hug was a one-time thing. ...Probably.",
    "I'm still thinking about the hug yesterday. Don't make it weird.",
    "Come in. Yesterday's hug doesn't mean you can call me that.",
    "Yesterday's hug... I'm still thinking about it. Don't smile.",
    "No hugs today. ...Well. Maybe a short one.");
POOL(R_MEM_KISS_T_MM,
    "That kiss earlier... forget it. I mean it.",
    "I'm still warm from earlier. Don't look at me.",
    "No more kisses until you use my name. ...Come in.",
    "I'm still blushing from earlier. Don't look at me.",
    "Don't expect a second kiss. ...Not yet.");
POOL(R_MEM_KISS_Y_MM,
    "About yesterday's kiss... don't mention it. Not a word.",
    "Yesterday was a mistake. A nice mistake. ...Don't tell anyone.",
    "Don't expect a repeat of yesterday. ...Come in.",
    "After yesterday's kiss, I can't think straight. Come in.",
    "Yesterday was a mistake. ...A lovely mistake. Don't tell.");
POOL(R_MEM_SLEEP_Y_MM,
    "You slept beside me last night. Don't make it a habit.",
    "Last night was... fine. Don't call me that this morning.",
    "Morning. No comments about last night, please.",
    "Last night was... fine. Don't gloat about it.",
    "You hogged the blanket last night. Remember that.");
/* v9.7: table lines of the free match at home in mama mode (class TC_TBL: two lines of 18 characters, split with | ) */
POOL(L_START_MM,
    "LET'S PLAY.|NO SILLY NAMES.",
    "DEAL THE CARDS,|HUSBAND.",
    "DON'T LOSE|TOO BADLY.",
    "READY? AND DON'T|CALL ME THAT.",
    "SIT. DEAL.|NO NICKNAMES.",
    "CARDS UP,|HUSBAND.",
    "I'LL GO EASY.|OR NOT.",
    "BRING YOUR|BEST HAND.",
    "LET'S BEGIN.|MIND YOUR TONGUE.",
    "GOOD LUCK.|YOU'LL NEED IT.",
    "TRY NOT TO|EMBARRASS US.",
    "SHUFFLE UP.|SOPHIA IS READY.",
    "FIRST HAND.|BEHAVE.",
    "WELCOME TO MY|TABLE, HUSBAND.",
    "NO PET NAMES.|JUST POKER.",
    "ANTE UP.|AND STAY QUIET.",
    "I'M WARMED UP.|ARE YOU?",
    "SMILE ALL YOU|WANT. I'M WINNING.",
    "PLAY NICE.|OR PAY FOR IT.",
    "DON'T STARE.|PLAY.",
    "SAY MY NAME,|THEN DEAL.");
POOL(L_HAND_MM,
    "YOUR MOVE,|HUSBAND.",
    "NO FUNNY NAMES|AT MY TABLE.",
    "EYES ON THE|CARDS.",
    "SHALL WE?",
    "NEW HAND.|STAY FOCUSED.",
    "DEAL, HUSBAND.|I'M WAITING.",
    "CARDS, NOT|COMPLIMENTS.",
    "MY TURN TO|BEAT YOU.",
    "PAY ATTENTION.|NOT TO ME.",
    "ANOTHER ONE.|KEEP UP.",
    "WELL? DON'T|JUST SIT THERE.",
    "EYES UP HERE.|ON THE CARDS.",
    "HURRY UP.|I DON'T BITE.",
    "DON'T LOOK SO|NERVOUS.",
    "STOP SMIRKING|AND PLAY.",
    "LET'S SEE YOUR|BEST TRY.",
    "NO WHISPERING|THAT WORD.",
    "COME ON.|MAKE A MOVE.",
    "BET WISELY,|HUSBAND.",
    "I'M WATCHING|YOUR FACE.",
    "THINK FASTER.|I'M BORED.");
POOL(L_LAST_MM,
    "LAST HAND.|MAKE IT COUNT.",
    "LAST HAND. NO|FAVORS, HUSBAND.",
    "FINAL HAND.|DON'T SULK.",
    "LAST HAND.|BE A MAN.",
    "ONE MORE HAND.|NO SULKING.",
    "FINAL HAND.|DO YOUR BEST.",
    "LAST ONE.|TRY NOT TO CRY.",
    "LAST HAND.|SAY SOPHIA.",
    "THE END IS|NEAR, HUSBAND.",
    "FINAL ROUND.|DON'T FLINCH.",
    "ONE LEFT. WIN|OR APOLOGIZE.",
    "LAST HAND.|NO MERCY.",
    "LAST ONE. DON'T|WASTE IT.",
    "LAST HAND. EYES|ON THE CARDS.",
    "ALMOST DONE.|BE BRAVE.",
    "LAST HAND.|STOP GRINNING.",
    "ONE FINAL TRY,|HUSBAND.",
    "FINISH IT.|IF YOU CAN.",
    "THE LAST HAND.|MAKE ME PROUD.",
    "LAST CHANCE.|IMPRESS ME.");
POOL(L_HERRAISE_MM,
    "RAISE. ARE YOU|BRAVE ENOUGH?",
    "I RAISE.|NO TEASING.",
    "BIGGER BET.|YOUR TURN.",
    "RAISING. DON'T|GET FLUSTERED.",
    "I RAISE. CALL|IF YOU DARE.",
    "UP IT GOES.|SWEAT, HUSBAND.",
    "RAISE. KEEP|YOUR EYES UP.",
    "MORE CHIPS.|MY RULES.",
    "I RAISE. YOUR|FACE SAYS FOLD.",
    "BIGGER. WILL YOU|FOLLOW, HUSBAND?",
    "RAISING. CRY|LATER.",
    "PRICE GOES UP.|PAY OR RUN.",
    "I RAISE.|SCARED YET?",
    "RAISE. AND NO|NAMES, PLEASE.",
    "MORE, PLEASE.|OR GO HOME.",
    "I'M RAISING.|STOP BLUSHING.",
    "RAISE. DON'T|MAKE A FACE.",
    "THAT'S A RAISE.|THINK HARD.",
    "UP A LITTLE.|FOLLOW IF BOLD.",
    "A RAISE. DON'T|CALL ME THAT.");
POOL(L_YOURAISE_MM,
    "OH? BRAVE|TODAY.",
    "A RAISE?|SHOW ME.",
    "BOLD. VERY|BOLD, HUSBAND.",
    "A RAISE? FROM|YOU? CUTE.",
    "OH, YOU'RE|SERIOUS NOW.",
    "FEELING LUCKY,|HUSBAND?",
    "HMPH. SHOW ME|WHAT YOU HAVE.",
    "BIG TALK. BIG|CHIPS. FINE.",
    "YOU RAISE? I'M|NOT AFRAID.",
    "BOLD. IT WON'T|SAVE YOU.",
    "TRYING TO|IMPRESS ME?",
    "RAISING ME?|HOW DARING.",
    "INTERESTING.|I'LL THINK.",
    "SHOWING OFF,|HUSBAND?",
    "SO CONFIDENT.|IT'S ADORABLE.",
    "THAT'S A BIG|BET. SWEATING?",
    "A RAISE. I'M|NOT IMPRESSED.",
    "MORE CHIPS?|FOOLISH MAN.",
    "OH? NOW YOU|WANT TO PLAY.",
    "DARING. I DO|LIKE DARING.");
POOL(L_SHEWIN_MM,
    "MINE. AND NO|SILLY NAMES.",
    "I WIN.|NO PITY.",
    "THAT'S MY POT,|HUSBAND.",
    "TOLD YOU.|I'M NOT SOFT.",
    "MINE.|DON'T SULK.",
    "POT'S MINE.|COUNT YOUR LOSS.",
    "EASY. HAVE A|TISSUE, HUSBAND.",
    "THANK YOU FOR|THE CHIPS.",
    "WINNER.|SAY MY NAME.",
    "I TOLD YOU|NOT TO STARE.",
    "AND THAT'S|HOW IT'S DONE.",
    "MY CHIPS NOW.|DON'T POUT.",
    "WON. AGAIN.|HOW SAD FOR YOU.",
    "SORRY. NOT|SORRY AT ALL.",
    "TOO SLOW,|HUSBAND.",
    "DID YOU THINK|I'D LOSE?",
    "POT TO ME.|NO NICKNAMES.",
    "CLEANED OUT.|WHAT A SHAME.",
    "HA. SOPHIA|WINS AGAIN.",
    "KEEP DREAMING.|THIS ONE'S MINE.");
POOL(L_YOUWIN_MM,
    "...WELL PLAYED.|DON'T GLOAT.",
    "YOU WON. STOP|SMILING!",
    "BEGINNER'S|LUCK.",
    "FINE. TAKE IT.|ONLY THIS ONE.",
    "HMPH. LUCKY.|DON'T GRIN.",
    "YOU WON?!|DON'T GET USED.",
    "WIPE THAT|SMILE OFF.",
    "FINE. YOURS.|STOP BOUNCING.",
    "I ALLOWED THAT.|REALLY.",
    "LUCKY CARDS,|NOT LUCKY YOU.",
    "ENJOY IT.|IT WON'T LAST.",
    "WELL PLAYED.|NOW BEHAVE.",
    "ONE POT. DON'T|BRAG, HUSBAND.",
    "GRR. TAKE IT.",
    "DON'T YOU DARE|GLOAT AT ME.",
    "SMUG LOOKS BAD|ON YOU.",
    "OKAY. YOU WON.|CALM DOWN.",
    "A FLUKE.|A LOVELY FLUKE.",
    "I WAS DISTRACTED.|THAT'S ALL.",
    "HMPH. DON'T|EXPECT A PRIZE.");
POOL(L_SHEFOLD_MM,
    "I FOLD. DON'T|LOOK SO SMUG.",
    "FINE, YOURS.|THIS TIME.",
    "NOT WORTH IT.|TAKE IT.",
    "I FOLD. ENJOY|THE SCRAPS.",
    "TAKE IT.|DON'T GET SMUG.",
    "NOT MY HAND.|NEXT TIME.",
    "FINE. YOU|WIN THAT ONE.",
    "I'M OUT. BE|PROUD, HUSBAND.",
    "FOLDING. IT'S|STRATEGY.",
    "GO ON. TAKE IT.|STOP LOOKING.",
    "MY CARDS STINK.|TAKE IT.",
    "I'LL LET THAT|ONE GO.",
    "ENOUGH. YOURS.|DON'T BRAG.",
    "I FOLD. THE|NEXT ONE'S MINE.",
    "DON'T CELEBRATE.|IT'S ONE POT.",
    "NOT WORTH MY|TIME. TAKE IT.",
    "HMPH. FINE.|GRAB YOUR CHIPS.",
    "TAKE IT.|I'M BEING KIND.",
    "YOU WIN THIS.|DON'T SMIRK.",
    "I FOLD. SAVING|MY CHIPS.");
POOL(L_YOUFOLD_MM,
    "RUNNING AWAY,|HUSBAND?",
    "YOU FOLD? COWARD|...HMPH.",
    "SMART. OR SCARED.",
    "FOLDING?|SO SOON?",
    "RUNNING AWAY|AGAIN, HUSBAND?",
    "A FOLD. HOW|VERY BRAVE OF YOU.",
    "THOUGHT SO.|COWARD.",
    "YOU FOLD? I|WASN'T TRYING.",
    "SPINELESS.|BUT PRACTICAL.",
    "SCARED OF ME,|HUSBAND?",
    "SMART. MY HAND|WAS A BEAUTY.",
    "THE POT'S MINE.|THANK YOU.",
    "GIVING UP SO|EASILY?",
    "HMPH. I DIDN'T|EVEN RAISE.",
    "NO GUTS. NO|CHIPS.",
    "YOU FOLD. I|BARELY TRIED.",
    "DON'T RUN. FACE|ME, HUSBAND.",
    "WISE. I WAS|HOLDING GOLD.",
    "FOLD? OH, COME|ON. FIGHT.",
    "HAH. RUN, THEN.");
POOL(L_SPLIT_MM,
    "A SPLIT.|HOW BORING.",
    "EVEN. DON'T|GET USED TO IT.",
    "SPLIT. FAIR|ENOUGH.",
    "SPLIT. HOW|UNSATISFYING.",
    "A TIE. DON'T|LOOK SO PLEASED.",
    "EQUAL. FOR NOW.",
    "HALF EACH.|I WANTED ALL.",
    "SPLIT POT.|HOW DULL.",
    "A DRAW. WE'LL|SETTLE THAT.",
    "SAME HAND.|STOP COPYING.",
    "A TIE. I HATE|TIES, HUSBAND.",
    "HALF IS NOT|ENOUGH FOR ME.",
    "EVEN AGAIN.|ANNOYING.",
    "SPLIT. DON'T|CALL IT FATE.",
    "EVEN? ONE MORE|HAND, THEN.",
    "A DRAW.|I'LL WIN LATER.",
    "SPLIT. YOU GOT|LUCKY AGAIN.",
    "HALF AND HALF.|MEH.",
    "IDENTICAL. HOW|SUSPICIOUS.",
    "A TIE?|YOU'RE IRRITATING.");
POOL(L_ENDHER_MM,
    "I WIN.|DON'T SULK.",
    "AS EXPECTED.|BETTER LUCK.",
    "GAME OVER.|NO NAMES NOW.",
    "I WIN. NOW|SAY MY NAME.",
    "MATCH OVER.|PAY UP.",
    "WON. AS ALWAYS.|DON'T POUT.",
    "GAME. SET.|SOPHIA WINS.",
    "THANKS FOR|THE GAME.",
    "BETTER LUCK|NEXT TIME.",
    "YOU TRIED.|THAT'S ENOUGH.",
    "I WIN. COME,|DON'T BE SAD.",
    "ALL MINE.|BEHAVE NEXT TIME.",
    "CHIN UP,|HUSBAND.",
    "ANOTHER WIN.|NO NICKNAMES.",
    "DONE. YOU'RE|OUT OF CHIPS.",
    "I WAS BETTER.|ADMIT IT.",
    "PLAY AGAIN?|I'LL WIN AGAIN.",
    "THE QUEEN WINS.|AGAIN.",
    "WELL, THAT'S|THAT. SULK AWAY.",
    "NOT EVEN CLOSE.|NEXT TIME.");
POOL(L_ENDYOU_MM,
    "YOU WIN. NOW|STOP GRINNING.",
    "WELL PLAYED.|I MEAN IT.",
    "...LUCKY. THIS|TIME ONLY.",
    "YOU WIN. DON'T|DARE GLOAT.",
    "CONGRATS.|NOW STOP SMILING.",
    "I LOST. DON'T|SAY IT TWICE.",
    "HMPH. YOU WON.|ONCE.",
    "FINE. YOU WIN.|NO NAMES, THOUGH.",
    "WELL DONE.|DON'T BE SMUG.",
    "YOU BEAT ME.|DON'T ENJOY IT.",
    "I LOST FAIRLY.|HMPH.",
    "I'LL WIN IT|BACK, HUSBAND.",
    "YOU WON. NOW|LEAVE ME BE.",
    "LUCK. PURE|LUCK.",
    "CONGRATS, I|SUPPOSE. HMPH.",
    "ENJOY YOUR WIN.|BRIEFLY.",
    "THIS ONCE. ONLY|THIS ONCE.",
    "I'M PROUD. AND|ANNOYED.",
    "WINNER. NOW BE|HUMBLE ABOUT IT.",
    "FINE. YOU WON.|BUY ME TEA.");
POOL(L_ENDTIE_MM,
    "A TIE.|HOW ANNOYING.",
    "EVEN. WE'LL|PLAY AGAIN.",
    "A TIE.|HOW UNFAIR.",
    "NO WINNER.|NO PRIDE.",
    "EVEN. I'LL|WIN THE NEXT.",
    "A DRAW. WE'LL|PLAY AGAIN.",
    "EQUAL AT LAST.|DON'T GLOAT.",
    "TIED. I HATE|IT, HUSBAND.",
    "NOBODY WON.|ANNOYING.",
    "A TIE?|WHAT A WASTE.",
    "DEAD EVEN.|REMATCH. NOW.",
    "STALEMATE.|I DISLIKE THAT.",
    "NO WINNER.|NO NICKNAMES.",
    "TIED. TRY HARDER|NEXT TIME.",
    "EVEN. HOW|IRRITATING.",
    "A TIE. DON'T|LOOK RELIEVED.",
    "EQUAL. FOR NOW.|DON'T BLINK.",
    "TIE GAME.|REMATCH SOON.",
    "A TIE. WHAT|A BORING END.",
    "NO LOSER.|NO WINNER.");
POOL(L_ENDLOST_MM,
    "YOU LOST BUT|PLAYED WELL.",
    "NOT BAD.|STILL A LOSS.",
    "BONUSES? FINE.|I'M IMPRESSED.",
    "A LOSS, BUT|NOT A SHAME.",
    "YOU LOST. BUT|THE BONUSES...",
    "LOST WITH STYLE.|HMPH.",
    "BONUSES? SWEET.|STILL LOST.",
    "NICE CARDS.|STILL A LOSS.",
    "YOU PLAYED WELL.|AND LOST.",
    "CUTE BONUSES.|WRONG RESULT.",
    "LOST BEAUTIFULLY,|HUSBAND.",
    "THE CARDS LIKED|YOU. I DIDN'T.",
    "YOU LOST. BUT|I'M IMPRESSED.",
    "WELL FOUGHT.|STILL LOST.",
    "SOME SKILL. NO|LUCK. HMPH.",
    "GOOD HANDS.|BAD ENDING.",
    "YOU LOST. EAT|YOUR BONUSES.",
    "ALMOST HAD ME.|ALMOST.",
    "A LOSS, YET|YOU SMILE. ODD.",
    "NOT BAD AT ALL.|STILL LOST.");


/* ---------- v12.1: COW MODE. While she wears the cow print outfit (married) her whole dialogue turns negative (disgust), but she still accepts every request.
   _CW = cow mode, _CW_MM = cow mode and you call her Mama. Chosen with g_cow(): no save flag, it ends when she changes out of the outfit. ---------- */
POOL(R_GREET_CW,
    "Oh, it's you. Ugh... Come in. Don't look at what I'm wearing.",
    "Back again? Fine, come in. And stop staring at the spots.",
    "You again. I'm in this ridiculous outfit because of you. Sit down and say nothing.",
    "Hmph. Come in, husband. I feel like a farm animal, so don't make it worse.",
    "Come in. Quickly. I can't stand being seen like this.",
    "It's you. Sit. And if you smile at this outfit, you can leave.",
    "Ugh, you're here. Fine. Keep your eyes above my neck. I mean it.",
    "Come in. I'm in a foul mood and this outfit isn't helping. Behave.");
POOL(R_GREET_CW_MM,
    "...Come in. I'm in this ridiculous outfit, and if you use that name, the door shuts.",
    "Oh, it's you. Disgusting outfit, disgusting name. Come in, husband, and say neither.",
    "Fine, come in. I'm not your mama and I'm not a cow. Remember both.",
    "You again. Sit. Not a word about the outfit and not a word of that name.",
    "Ugh. Come in before anyone sees me like this. And no names.",
    "Come in, husband. I feel ridiculous enough without that word from you.",
    "Hmph. Spots on me and that name from you. Come in and behave.",
    "It's you. Sit down quietly. I'm in no mood for names or comments.");
POOL(R_NIGHT_CW,
    "It's late. ...Come in, and don't you dare look at this outfit.",
    "At this hour? Ugh. Fine, come in. I'm wearing this nonsense because of you.",
    "Shh. Get in here. I feel ridiculous, so keep your comments to yourself.",
    "Couldn't sleep? Come in. And don't stare at the spots.",
    "You woke me. Hmph. Come in, husband, and say nothing about the outfit.",
    "Middle of the night, and I'm dressed like livestock. Come in, quietly.");
POOL(R_NIGHT_CW_MM,
    "It's late. ...Come in, and keep that name and any comment about this outfit to yourself.",
    "At this hour? Ugh. Come in. No names, no comments.",
    "Shh. Get in here. I'm not your mama and I'm not a cow. Remember that.",
    "Couldn't sleep? Fine. Come in. Say that name and I'll bar the door.",
    "You woke me. Come in, husband. Eyes up, mouth shut.",
    "Middle of the night, cow spots, and that name from you. Come in, quietly.");
POOL(R_MORE_CW,
    "Anything else? Hurry up. This outfit itches.",
    "Are you done? I'd like to get out of these clothes.",
    "Hmph. Well? What else do you want from me?",
    "Is that all? I feel ridiculous standing here.",
    "Done talking? Then what else, husband?",
    "Ugh. Is there more? Make it quick.");
POOL(R_MORE_CW_MM,
    "Anything else? And not that name, please.",
    "Are you done? I'm not your mama, I'm just stuck in this outfit.",
    "Hmph. What else do you want? No names this time.",
    "Is that all? I feel ridiculous. Make it quick.",
    "Done? Then what else, husband? Husband, understood?",
    "Ugh. More? Hurry up, and keep that word to yourself.");
POOL(R_NOT_FULL_CW,
    "Not full yet. Come back when it is. Ugh, don't look at my chest like that.",
    "It's not time. Stop staring at this outfit and wait.",
    "Not yet. Honestly, you think of nothing else when I wear this, do you?",
    "Still not full. Don't rush me. Don't make me say more.",
    "No. Come back when the gauge is full. Disgusting.",
    "Not yet! And wipe that grin off your face.");
POOL(R_NOT_FULL_CW_MM,
    "Not full yet. And don't call me that.",
    "It's not time. Stop using that name and wait.",
    "Not yet. You and your names. Come back when it's full.",
    "Still not full. Don't rush me, and don't say that word.",
    "No. Come back when the gauge is full. And no names, ugh.",
    "Not yet! Wipe that grin off your face and keep that name away.");
/* v12.2: the breast pump wears out: her answer when it is broken (normal / Mama / cow / cow + Mama) and her thanks for the new one */
POOL(R_PUMP_BROKEN,
    "The pump just gave out. It's broken. Get a new one online and I'll try again.",
    "Nothing is coming out, the pump is broken. It's finished. We need a new one.",
    "Wait, the pump stopped working. It's broken for good. Buy another one.",
    "It's no use, the pump is dead. Worn out. Order a new one, please.",
    "The pump broke. Don't look at me like that, it's just worn out. Get a new one.",
    "It won't run anymore. The pump is broken. Bring me a new one and we can try again.");
POOL(R_PUMP_BROKEN_MM,
    "The pump is broken. It's worn out. Get a new one, and don't call me that while you're at it.",
    "It's dead, the pump. Buy a new one. And no names, please.",
    "The pump gave out. Don't pout, just get a new one.",
    "Broken. The pump is broken. Bring a new one. That name won't fix it.",
    "It stopped working. Worn out. Order another one, and keep that name to yourself.",
    "The pump is finished. Don't give me that look, just buy a new one.");
POOL(R_PUMP_BROKEN_CW,
    "Ugh, the pump is broken. Typical. Get a new one so I can get out of this outfit.",
    "It's dead. The pump is worn out. Buy another, and don't stare at these spots.",
    "The pump broke. Finally something is on my side. Get a new one, I suppose.",
    "Nothing. The pump is broken. Disgusting outfit, useless pump. Order a new one.",
    "It gave out. Worn out like my patience. Get a new pump.",
    "Broken. The pump is broken. Don't grin about it, just go buy a new one.");
POOL(R_PUMP_BROKEN_CW_MM,
    "The pump is broken. Get a new one. And stop using that name.",
    "It's dead. Buy another pump, and keep that word to yourself.",
    "Worn out. The pump gave up. New one, now. No names, ugh.",
    "Broken! The pump is broken. Wipe that grin off and go buy a new one.",
    "It stopped. Even the pump has had enough of you. Order a new one.",
    "The pump is finished. Get another, and don't say that word again.");
/* v12.3: the MOMMY MILK INJECTION. Gift scene pools (hand, thanks, reaction, closing), her "already today" answers (one milk a day in total) and two refusals */
POOL(GH_INJECT, "I hold out the Mommy Milk Injection to her.", "I hold up the syringe with the glowing blue liquid.", "I hand her the experimental injection, still in its case.", "I place the Mommy Milk Injection in her palm.", "I offer her the syringe. The blue liquid inside shines.");
POOL(GO_INJECT, "An injection?! ...What exactly is in this thing?", "You bought something this expensive?! ...You really have no shame.", "A needle? ...I hate needles. Why would you buy this?", "Experimental? That word alone should make me throw it at you.", "You want me to put that in my body? ...Unbelievable.");
POOL(GL_INJECT, "Turn around while I do it. If anything goes wrong, it's your fault.", "Fine. Just this once... Don't you dare watch.", "If this is permanent, you'd better take responsibility.", "Face the wall. This is humiliating enough already.", "...I can't believe I'm doing this. Don't say a single word.");
POOL(GE_INJECT, "She takes the syringe and shuts the door. The gauge is gone for good. I can ask her for milk once a day.", "The door closes. A faint gasp follows. Her milk will not run dry now. I can ask once a day.", "She snatches it and slams the door. No more gauge. I can ask her for milk once a day.", "She disappears behind the door with the syringe. Her milk is permanent now. One ask a day.", "She mutters a curse and shuts the door. Her body has changed for good. One ask a day.");
POOL(R_INJ_DONE,
    "Once a day. That was today's. Come back tomorrow.",
    "Not again! Once a day is the limit. Tomorrow.",
    "I already gave you milk today. I'm not a vending machine. Tomorrow.",
    "Today's milk is done. Don't ask again until tomorrow.");
POOL(R_INJ_DONE_MM,
    "Stop calling me that. I already gave you today's. Come back tomorrow.",
    "Once a day, no more. And no names, please. Tomorrow.",
    "Not again. Today's is done. Don't pout, just wait until tomorrow.",
    "That's enough for today. And keep that name to yourself.");
POOL(R_INJ_DONE_CW,
    "Ugh, once a day is more than enough in this outfit. Tomorrow.",
    "Today's is done. Don't stare at these spots and come back tomorrow.",
    "No. Once a day. Even this ridiculous outfit has limits. Tomorrow.",
    "Already today. Stop looking at me like I'm a cow. Tomorrow.");
POOL(R_INJ_DONE_CW_MM,
    "Once a day. And stop using that name. Tomorrow.",
    "Today's is done. Wipe that grin off and wait until tomorrow.",
    "No more today. Keep that word to yourself, ugh.",
    "Enough for today. Come back tomorrow, and no names.");
POOL(R_INJ_NOPILL, "I don't need that anymore. Put it away.", "Pills? After that injection? No. Keep them.", "My body doesn't need that anymore. Don't waste your money.", "No more of that stuff. The injection already took care of it.");
POOL(R_PUMP_NEW,
    "A new pump. Thank you. Now it should work again.",
    "Good, a new pump. Let's hope this one lasts longer.",
    "You got another one. Thanks. I'll take care of it this time.",
    "A fresh pump. Thank you. Ask me again whenever you like.");
POOL(R_PUMP_NEW_MM,
    "A new pump. Thank you. And I'm still not your mama.",
    "Good, another pump. Thanks. Don't call me that.",
    "You bought a new one. Fine. Thanks, and no names.",
    "A fresh pump. Thank you. Ask when you're ready, and keep that name away.");
POOL(R_PUMP_NEW_CW,
    "A new pump. Fine. Now stop looking at this outfit and let me work.",
    "Another pump. Ugh, at least something works. Thanks, I suppose.",
    "You got a new one. Don't expect me to look happy about it.",
    "A fresh pump. Thanks. I still hate this outfit.");
POOL(R_PUMP_NEW_CW_MM,
    "A new pump. Fine. And stop using that name.",
    "Another pump. Thanks. Keep that word to yourself, ugh.",
    "You got a new one. Don't expect a smile, and no names.",
    "A fresh pump. Thanks, I suppose. Don't say that word again.");
POOL(R_HUG_OPEN_CW,
    "I open my arms. Sophia wrinkles her nose and steps back, then lets me hold her, stiff as a board.",
    "I wrap my arms around her. She shudders at the spotted fabric but doesn't pull away.",
    "I pull her close. She mutters about feeling filthy, yet she stays put.",
    "I hug her from behind. Sophia sighs like a martyr and goes rigid.",
    "I hold her without a word. She glares at the wall, jaw tight, and endures it.",
    "I step in and hug her. She turns her face away in disgust, but her hands stay on my arms.");
POOL(R_HUG_OPEN_CW_MM,
    "I open my arms and whisper that name. Sophia grimaces and goes stiff, but she doesn't pull away.",
    "I rest my head on her shoulder. She shudders at the name and the spotted fabric, and still holds on.",
    "I pull her close. She hisses a protest and stays rigid in my arms.",
    "I hug her from behind. Sophia sighs as if I'd asked for the world, and endures it.",
    "I hold her quietly. She glares at the wall, jaw tight, and mutters about the name.",
    "I step in and hug her. She turns away in disgust, but her hands stay on my arms.");
POOL(R_KISS_OPEN_CW,
    "I tilt her chin up. Sophia winces in distaste, then lets me kiss her cheek, stiff.",
    "I lean in and kiss her. She screws up her face, but she doesn't push me away.",
    "I brush her hair back and kiss her. She huffs with disgust, yet doesn't move.",
    "She turns her face away, so I kiss her cheek. She sighs and puts up with it.",
    "I kiss her quickly. Sophia grimaces as if she had tasted something sour.",
    "I cup her cheek and kiss her. She shudders, then lets it happen.");
POOL(R_KISS_OPEN_CW_MM,
    "I whisper that name and kiss her cheek. Sophia winces and shudders, but she doesn't pull back.",
    "I lean in and kiss her. She screws up her face at the name, yet lets me.",
    "I brush her hair back and kiss her. She huffs in disgust, but stays put.",
    "She turns her face away, so I kiss her cheek. She mutters about names and endures it.",
    "I kiss her quickly. Sophia grimaces as if she had tasted something sour.",
    "I cup her cheek and kiss her. She shudders at that name, then lets it happen.");
POOL(R_OUTFIT_UNCOW,
    "Finally. Out of that disgusting thing. Don't ever ask me again.",
    "Ahh... real fabric again. I feel clean. Never mention cows to me.",
    "That's better. Don't look at the discarded outfit. I'll burn it someday.",
    "Done. I can finally breathe. Don't you dare look so disappointed.");
POOL(R_OUTFIT_UNCOW_MM,
    "Finally. Out of that disgusting thing. And not one word of that name.",
    "Ahh... real fabric again. I feel clean. Forget cows. Forget names.",
    "That's better. Don't look at it. And stop calling me that.",
    "Done. I can finally breathe. Don't look so disappointed.");
SCENE(B_HUG_CW,
    {W_NAR, BG_HUG, 0, NULL},
    {W_SOPHIA, BG_HUG, 0, "...Hmph. Only because I've already said yes. This fabric itches and I hate it. Don't get used to this."},
    {W_ME, BG_HUG, 0, "I just want to hold you for a bit."},
    {W_NAR, BG_HUG, 0, "She stays stiff and grumbling in my arms, but she does not let go. For a while neither of us says anything."}
);
SCENE(B_HUG_CW_MM,
    {W_NAR, BG_HUG, 0, NULL},
    {W_SOPHIA, BG_HUG, 0, "...This doesn't count. I'm wearing this disgusting thing and you're using that name. Don't you dare say it again."},
    {W_ME, BG_HUG, 0, "Your arms are the best place in the whole hotel, Mama."},
    {W_NAR, BG_HUG, 0, "She mutters, \"Stop it, I'm Sophia,\" with a sour face, yet she doesn't let go. For a while neither of us moves."}
);
SCENE(B_KISS_CW,
    {W_NAR, BG_KISS, 0, NULL},
    {W_SOPHIA, BG_KISS, 0, "...Ugh. In this outfit?! Who said you could do that so suddenly?"},
    {W_ME, BG_KISS, 0, "I'm your husband. I don't need to ask."},
    {W_SOPHIA, BG_KISS, 0, "Hmph... Fine. Just that. I'm not looking forward to another one in this thing."},
    {W_NAR, BG_KISS, 0, "Her nose is wrinkled and her cheeks are pink. She turns away, but her hand stays in mine."}
);
SCENE(B_KISS_CW_MM,
    {W_NAR, BG_KISS, 0, NULL},
    {W_SOPHIA, BG_KISS, 0, "...Hmph. That was your one. In this outfit, with that name. Don't get ideas."},
    {W_ME, BG_KISS, 0, "Thank you, Mama."},
    {W_SOPHIA, BG_KISS, 0, "Don't say that! I'm not that and I'm not a cow. ...Say my name, and maybe one more."},
    {W_NAR, BG_KISS, 0, "She kisses me once more, quickly, with a sour face, then turns away and refuses to look at me."}
);
SCENE(B_SLEEP_CW,
    {W_NAR, BG_SLEEPT, 0, "It is late and the hotel is quiet. I ask Sophia if I can sleep beside her. She glances at her cow print outfit and winces."},
    {W_SOPHIA, BG_SLEEPT, 0, "...Fine. But only sleeping. I'm keeping this itchy thing on to spite you, husband. Stay on your half."},
    {W_ME, BG_SLEEPT, 0, "Understood. Good night, Sophia."},
    {W_NAR, BG_SLEEPT, 0, "She turns off the light with a long sigh. In the dark her hand finds mine anyway, and I hear her breathing slow down."},
    {W_SOPHIA, BG_SLEEPT, 0, "...Good night. And not one word about this outfit in the morning. Not one."},
    {W_NAR, BG_SLEEPT, 0, "I fall asleep to the sound of the rain against the window, with her warmth right beside me."}
);
SCENE(B_SLEEP_CW_MM,
    {W_NAR, BG_SLEEPT, 0, "It is late and the hotel is quiet. I ask Sophia if I can sleep beside her. She glances at her cow print outfit and winces."},
    {W_SOPHIA, BG_SLEEPT, 0, "...Only if you don't say that name. Not once. And not a word about this outfit either. Understood?"},
    {W_ME, BG_SLEEPT, 0, "Good night, Mama."},
    {W_SOPHIA, BG_SLEEPT, 0, "I SAID... ugh. Never mind. Good night, husband. And no stealing the blanket."},
    {W_NAR, BG_SLEEPT, 0, "She turns off the light, stiff and sour at first. Slowly she relaxes, and her hand finds mine."},
    {W_NAR, BG_SLEEPT, 0, "I fall asleep to the sound of the rain against the window, wrapped in her warmth."}
);
SCENE(B_NIGHTM_CW,
    {W_NAR, BG_BDOOR, 0, "It's late at night. I knock softly on our bedroom door."},
    {W_SOPHIA, BG_BDOOR, 0, "...Who is it? Don't tell me you want to see me in this ridiculous outfit again."},
    {W_SOPHIA, BG_BDOOR, 5, "...It's you. Come in, quickly. And don't look at the spots."},
    {W_NAR, BG_ROM, 0, "I slip inside and close the door behind me. The lamp is low and her room is warm."}
);
SCENE(B_NIGHTM_CW_MM,
    {W_NAR, BG_BDOOR, 0, "It's late at night. I knock softly on our bedroom door."},
    {W_SOPHIA, BG_BDOOR, 0, "...Who is it? If it's that silly name again, or a comment on this outfit, go back to bed."},
    {W_SOPHIA, BG_BDOOR, 5, "...It's you. Come in, quickly. And don't say any of it out loud."},
    {W_NAR, BG_ROM, 0, "I slip inside and close the door behind me. The lamp is low and her room is warm."}
);
SCENE(B_FEED_CW,
    {W_NAR, BG_ROM, 1, "I ask her to breastfeed me. Sophia crosses her arms over her chest and looks down at her cow print outfit with disgust."},
    {W_SOPHIA, BG_ROM, 1, "Right now?! In this? ...Ugh. Fine. You're my husband. But I feel disgusting. Don't stare while I get ready."},
    {W_NAR, BG_FEED, 0, "Sophia sits on the edge of the bed, her mouth pressed into a thin line, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "My chest is so full it aches, otherwise I would never. ...Not a word about this outfit. Not one."},
    {W_ME, BG_FEED, 0, "I won't. Tell me if it's too much."},
    {W_NAR, BG_FEED, 0, "She draws me close, stiff and sour-faced, one hand resting in my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Hurry up. I hate how this feels. Don't look so happy. You're being awfully greedy."},
    {W_NAR, BG_FEED, 0, "Afterwards she lets out a long breath and mutters that I'd better never mention it. She still holds me a bit longer."}
);
SCENE(B_FEED_CW_LR,
    {W_NAR, BG_ROM, 1, "I ask her to breastfeed me. Sophia crosses her arms over her chest and looks down at her cow print outfit with disgust."},
    {W_SOPHIA, BG_ROM, 1, "Right now?! In this? ...Ugh. Fine. You're my husband. But I feel disgusting. Sit on the sofa and don't stare."},
    {W_NAR, BG_FEED, 0, "Sophia sits on the sofa, her mouth pressed into a thin line, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "My chest is so full it aches, otherwise I would never. ...Not a word about this outfit, and keep it quiet."},
    {W_ME, BG_FEED, 0, "I won't. Tell me if it's too much."},
    {W_NAR, BG_FEED, 0, "She draws me close, stiff and sour-faced, one hand resting in my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Hurry up. I hate how this feels. Don't look so happy. And keep an eye on the door."},
    {W_NAR, BG_FEED, 0, "Afterwards she lets out a long breath and mutters that I'd better never mention it. She still holds me a bit longer."}
);
SCENE(B_FEED_CW_MM,
    {W_NAR, BG_ROM, 1, "I sit beside her and ask softly, \"Mama... may I?\" Sophia crosses her arms over her chest, glaring down at the cow print."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...And in this disgusting outfit, too. I'm only doing it because it aches. That's all."},
    {W_NAR, BG_FEED, 0, "Sophia sits on the edge of the bed, avoiding my eyes, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "...Just this once. Don't say a word. About me, the outfit, or that name. ...I feel sick about all of this."},
    {W_ME, BG_FEED, 0, "I understand. Tell me if it's too much, Sophia."},
    {W_NAR, BG_FEED, 0, "She draws me close, stiff and sour-faced, one hand hesitant in my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Good. Like that. Don't look up at me. ...I hate this outfit, and I still hate that name."},
    {W_ME, BG_FEED, 0, "Thank you."},
    {W_NAR, BG_FEED, 0, "Afterwards she lets out a long breath and tells me to forget it ever happened. She still holds me a little longer."}
);
SCENE(B_FEED_CW_MM_LR,
    {W_NAR, BG_ROM, 1, "I sit beside her and ask softly, \"Mama... may I?\" Sophia crosses her arms over her chest, glaring down at the cow print."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...And not out here, in this disgusting outfit. I'm only doing this because it aches."},
    {W_NAR, BG_FEED, 0, "Sophia sits on the sofa, avoiding my eyes, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "...Just this once. If anyone knocks, we stop. And not a word about me, the outfit, or that name."},
    {W_ME, BG_FEED, 0, "I understand. Tell me if it's too much, Sophia."},
    {W_NAR, BG_FEED, 0, "She draws me close, stiff and sour-faced, one hand hesitant in my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Good. Like that. Don't look up at me. ...I hate this outfit, and I still hate that name."},
    {W_ME, BG_FEED, 0, "Thank you."},
    {W_NAR, BG_FEED, 0, "Afterwards she lets out a long breath and tells me to forget it ever happened. She still holds me a little longer."}
);
SCENE(B_OPEN_ASK_CW,
    {W_ME, BG_ROM, 1, "Sophia, is it full yet? I'd like a bottle."},
    {W_SOPHIA, BG_ROM, 1, "Do you hear yourself? Asking your cow-print wife for a bottle... Ugh. Fine. It's heavy anyway. Turn around."},
    {W_ME, BG_ROM, 1, "Could you pump some for me today?"},
    {W_SOPHIA, BG_ROM, 1, "In this outfit you want me to... ugh, I feel like livestock. Fine. Get the pump and face the wall."},
    {W_ME, BG_ROM, 1, "My bottle is empty. Could you fill it?"},
    {W_SOPHIA, BG_ROM, 1, "Fill it?! Disgusting. Like I'm a machine in a barn. ...Hmph. Fine. Don't look at me."},
    {W_ME, BG_ROM, 1, "I'd love some of your milk today."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! It's revolting to hear. ...My chest is heavy, so fine. Turn around."},
    {W_ME, BG_ROM, 1, "Sophia, would you pump some for me?"},
    {W_SOPHIA, BG_ROM, 1, "Ugh, with spots on me, no less. ...Fine. But not one word while I do it."},
    {W_ME, BG_ROM, 1, "Is it time for a bottle, Sophia?"},
    {W_SOPHIA, BG_ROM, 1, "Stop asking so cheerfully! ...Fine. It does ache. Get out the pump and face away."}
);
SCENE(B_OPEN_ASK_CW_MM,
    {W_ME, BG_ROM, 1, "Mama, is it full yet? I'd like a bottle."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! And this outfit... ugh, I feel like livestock. ...Fine. Turn around."},
    {W_ME, BG_ROM, 1, "Mama, could you pump some for me today?"},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! Disgusting, all of it. ...Hmph. Get the pump and face the wall."},
    {W_ME, BG_ROM, 1, "Mama... my bottle is empty. Could you fill it?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! Fill it?! Like a cow in a barn?! ...Ugh. Fine. Don't look."},
    {W_ME, BG_ROM, 1, "Mama, I'd love some of your milk today."},
    {W_SOPHIA, BG_ROM, 1, "Don't say it like that! It's revolting. ...My chest is heavy, so fine. Turn around."},
    {W_ME, BG_ROM, 1, "Mama, would you pump some for me?"},
    {W_SOPHIA, BG_ROM, 1, "Mama?! In this outfit?! Ugh. ...Fine. But not one word while I do it."},
    {W_ME, BG_ROM, 1, "Mama, is it time for a bottle?"},
    {W_SOPHIA, BG_ROM, 1, "Stop asking so cheerfully, and stop that name! ...Fine. It does ache. Face away."}
);
SCENE(B_OPEN_PILL_CW,
    {W_ME, BG_ROM, 1, "Sophia, here's your pill. Take it for me?"},
    {W_SOPHIA, BG_ROM, 1, "Give me that. And don't look at me while I swallow it. Ugh, I look ridiculous."},
    {W_ME, BG_ROM, 1, "Time for your pill, Sophia. Open up."},
    {W_SOPHIA, BG_ROM, 1, "I can take a pill by myself! ...Ugh. Fine. Hand it over and turn around."},
    {W_ME, BG_ROM, 1, "Please take this one. For both of us."},
    {W_SOPHIA, BG_ROM, 1, "Both of us?! ...Disgusting. Fine. Give it here. And not a word about this outfit."},
    {W_ME, BG_ROM, 1, "Your pill. I'll hold the water."},
    {W_SOPHIA, BG_ROM, 1, "I can hold my own glass. ...Hmph. Give it here. I hate every part of this."}
);
SCENE(B_OPEN_PILL_CW_MM,
    {W_ME, BG_ROM, 1, "Mama, here's your pill. Take it for me?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...Give it here. And don't watch me swallow it in this ridiculous outfit."},
    {W_ME, BG_ROM, 1, "Mama, time for your pill. Open up."},
    {W_SOPHIA, BG_ROM, 1, "Stop calling me that! Ugh. Fine. Hand it over and turn around."},
    {W_ME, BG_ROM, 1, "Mama, please take this one. For both of us."},
    {W_SOPHIA, BG_ROM, 1, "M-mama?! Disgusting. ...Fine. Give it here. Not a word."},
    {W_ME, BG_ROM, 1, "Mama, your pill. I'll hold the water."},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! ...Hmph. Give it here. I hate every part of this."}
);
SCENE(B_MID_PUMP_CW,
    {W_ME, BG_ROM, 1, "Are you alright back there, Sophia?"},
    {W_SOPHIA, BG_ROM, 1, "Don't talk to me! I feel like livestock. ...Just keep your eyes on the wall."},
    {W_ME, BG_ROM, 1, "Take your time. I'm not looking."},
    {W_SOPHIA, BG_ROM, 1, "Good. Because I'm disgusted enough with myself already. Hmph."},
    {W_ME, BG_ROM, 1, "Is it helping? Does it hurt?"},
    {W_SOPHIA, BG_ROM, 1, "It's... fine! Ugh, stop asking. Every word makes this worse."},
    {W_ME, BG_ROM, 1, "I'm facing the wall, I promise."},
    {W_SOPHIA, BG_ROM, 1, "You'd better be. This outfit and this pump... I could die of shame. Don't you dare turn."},
    {W_ME, BG_ROM, 1, "Almost done?"},
    {W_SOPHIA, BG_ROM, 1, "Don't rush me! ...Ugh. A little more. Don't turn around."}
);
SCENE(B_MID_PUMP_CW_MM,
    {W_ME, BG_ROM, 1, "Mama, are you alright back there?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! I feel like livestock. ...Just keep your eyes on the wall."},
    {W_ME, BG_ROM, 1, "Take your time, Mama. I'm not looking."},
    {W_SOPHIA, BG_ROM, 1, "Good. And stop that name. I'm disgusted enough already."},
    {W_ME, BG_ROM, 1, "Mama, is it helping? Does it hurt?"},
    {W_SOPHIA, BG_ROM, 1, "It's fine! Ugh, stop asking, and stop that name. Every word makes this worse."},
    {W_ME, BG_ROM, 1, "I'm facing the wall, Mama. I promise."},
    {W_SOPHIA, BG_ROM, 1, "You'd better be. And don't call me that. I could die of shame. Don't turn."},
    {W_ME, BG_ROM, 1, "Mama, almost done?"},
    {W_SOPHIA, BG_ROM, 1, "Don't rush me! ...Ugh. A little more. Don't turn around, and no names."}
);
SCENE(B_MID_FEED_CW,
    {W_ME, BG_ROM, 1, "Is this alright, Sophia?"},
    {W_SOPHIA, BG_ROM, 1, "Don't ask! It's... fine. Ugh. I hate how this feels. Just hold still."},
    {W_ME, BG_ROM, 1, "Tell me if you want me to stop."},
    {W_SOPHIA, BG_ROM, 1, "If I wanted that I'd have said so. ...Hmph. Keep going. And stop talking."},
    {W_ME, BG_ROM, 1, "You're so warm, Sophia."},
    {W_SOPHIA, BG_ROM, 1, "Disgusting. Don't say things like that in this outfit. ...Fine. Stay there."},
    {W_ME, BG_ROM, 1, "Thank you, Sophia. Really."},
    {W_SOPHIA, BG_ROM, 1, "Save it. I feel humiliated enough. ...Just a little longer, then it's over."},
    {W_ME, BG_ROM, 1, "Are you comfortable?"},
    {W_SOPHIA, BG_ROM, 1, "No! ...Yes. Ugh. Don't make me answer that. Hold still."}
);
SCENE(B_MID_FEED_CW_MM,
    {W_ME, BG_ROM, 1, "Is this alright, Mama?"},
    {W_SOPHIA, BG_ROM, 1, "Don't call me that! It's... fine. Ugh. I hate how this feels. Just hold still."},
    {W_ME, BG_ROM, 1, "Mama, tell me if you want me to stop."},
    {W_SOPHIA, BG_ROM, 1, "If I wanted that I'd have said so. ...Hmph. Keep going. And no names."},
    {W_ME, BG_ROM, 1, "You're so warm, Mama."},
    {W_SOPHIA, BG_ROM, 1, "Disgusting. Don't say that in this outfit. ...Fine. Stay there."},
    {W_ME, BG_ROM, 1, "Thank you, Mama. Really."},
    {W_SOPHIA, BG_ROM, 1, "Save it, and that name. I feel humiliated enough. ...Just a little longer."},
    {W_ME, BG_ROM, 1, "Mama, are you comfortable?"},
    {W_SOPHIA, BG_ROM, 1, "No! ...Yes. Ugh. Don't make me answer. Hold still."}
);

/* ---- v13.11: mama mode, page 4: INTIMATE HUG and BOTTLE FEEDING. She is arrogant and condescending (and still pushes back on the name). No "baby" / "boy" in these texts (the mama-text test). ---- */
/* INTIMATE HUG: lying on her bed. Opener pairs (your line, her answer), then the scene, one middle pair before the last beat. */
SCENE(B_IHUG_OPEN_MM,
    {W_ME, BG_ROM, 0, "Mama, can I lie down with you for a bit? I just want to be held."},
    {W_SOPHIA, BG_ROM, 0, "Don't call me that. Hmph. A grown man begging to be cuddled. How pathetic. ...Fine. Get on the bed."},
    {W_ME, BG_ROM, 0, "Mama... I had a rough day. Will you hold me?"},
    {W_SOPHIA, BG_ROM, 0, "Stop that, I'm not your mother. Tsk. Of course you can't cope without me. Come here, and don't make it weird."},
    {W_ME, BG_ROM, 0, "Mama, please? Just a hug in bed. I'll be good."},
    {W_SOPHIA, BG_ROM, 0, "Good? You? Hah! You can't even stand up straight without me. Fine. Lie down and stay still."},
    {W_ME, BG_ROM, 0, "Mama, I'm so tired. Can we lie down together?"},
    {W_SOPHIA, BG_ROM, 0, "Don't call me that! ...Honestly, you need me for everything. Lie down, you hopeless thing."},
    {W_ME, BG_ROM, 0, "Mama, hold me tight on the bed, please. Only for a little while."},
    {W_SOPHIA, BG_ROM, 0, "'Only for a little while.' Hmph. I know how this goes. You'll cling to me for an hour. Get in, then."},
    {W_ME, BG_ROM, 0, "Mama... I miss your arms. Can I come lie with you?"},
    {W_SOPHIA, BG_ROM, 0, "Mama, mama, mama. Do you ever say anything else? ...Tsk. Fine. This once, and only because I'm generous."},
    {W_ME, BG_ROM, 0, "Mama, can we cuddle on the bed? Please?"},
    {W_SOPHIA, BG_ROM, 0, "On my bed? You should be honored. Don't wrinkle the sheets, and don't call me that. ...Come on, then."},
    {W_ME, BG_ROM, 0, "Mama, I want to lie in your arms for a while."},
    {W_SOPHIA, BG_ROM, 0, "Hmph. So needy. Fine, fine. But I'm only doing this because I pity you. Come here."}
);
SCENE(B_IHUG_MID_MM,
    {W_ME, BG_IHUG, 0, "Mama, your heartbeat is so calming."},
    {W_SOPHIA, BG_IHUG, 0, "Naturally. It's the best heartbeat in the building, and you're lucky to be listening to it."},
    {W_ME, BG_IHUG, 0, "I don't want to get up."},
    {W_SOPHIA, BG_IHUG, 0, "I know. You never do. That's why I'm the one who decides when this ends. Not yet."},
    {W_ME, BG_IHUG, 0, "Thank you for holding me, Mama."},
    {W_SOPHIA, BG_IHUG, 0, "Don't thank me. Just remember who is in charge here. Hint: it isn't you."},
    {W_ME, BG_IHUG, 0, "Can you stroke my hair a little?"},
    {W_SOPHIA, BG_IHUG, 0, "Demanding, aren't we? ...Fine. Like this? Hmph. Don't purr at me."},
    {W_ME, BG_IHUG, 0, "Everything feels better when you hold me like this."},
    {W_SOPHIA, BG_IHUG, 0, "Of course it does. I'm that good. Remember it next time you act so cocky."},
    {W_ME, BG_IHUG, 0, "Just a few more minutes, Mama."},
    {W_SOPHIA, BG_IHUG, 0, "A few more minutes. Tsk. Fine, but I'm counting, and you owe me for every one."}
);
SCENE(B_IHUG_MM,   /* beats 1-2 are filled in from B_IHUG_OPEN_MM (a pair); one pair of B_IHUG_MID_MM is added before the last beat */
    {W_ME, BG_ROM, 0, NULL},
    {W_SOPHIA, BG_ROM, 0, NULL},
    {W_NAR, BG_IHUG, 0, "She lies down first, smoothing her clothes with great dignity, and pats the spot before her. I lie down and hug her."},
    {W_SOPHIA, BG_IHUG, 0, "Not so tight, you'll wrinkle me. ...Tsk. Look at you, clinging to me like you can't sleep without me. Sad, really."},
    {W_NAR, BG_IHUG, 0, "She keeps complaining, yet her arm stays around me and her fingers drift through my hair. Neither of us gets up for a long while."}
);
POOL(R_IHUG_NO_MM,   /* INTIMATE HUG asked anywhere but in her bedroom */
    "On a sofa? In the hall? Do I look like I lie down wherever you point? My bedroom. My bed. Not here.",
    "Not out here. If you want to be held, find somewhere with a proper bed. Don't ask me again until then.",
    "Lie down? Here? Hah! Have some dignity. The bed is in my bedroom, where it belongs.",
    "You want to cuddle in the open like some stray? No. Bedroom. Think before you ask.");
/* BOTTLE FEEDING: the pump scene as for PUMP MILK / ASK FOR MILK, then this hand-over, then you choose: take it and leave, or ask her to feed you */
SCENE(B_BFEEDH_MM,
    {W_NAR, BG_BOTTLE, 0, "A while later Sophia turns around with a bottle filled to the brim. She holds it up high, just out of my reach."},
    {W_SOPHIA, BG_BOTTLE, 0, "There. Do you have any idea how much work that was? ...Of course not. You only ever show up when it's full."},
    {W_ME, BG_BOTTLE, 0, "Thank you, Mama. It looks wonderful."},
    {W_SOPHIA, BG_BOTTLE, 0, "Don't call me that. And it's not wonderful, it's milk. Honestly, you stare at it like a starving puppy."},
    {W_NAR, BG_BOTTLE, 0, "She swings the bottle in front of my nose and waits, smirking, to see what I will do about it."}
);
SCENE(B_BFEEDL_MM,   /* you take the bottle and leave */
    {W_ME, BG_BOTTLE, 0, "I'll take it with me, thank you."},
    {W_SOPHIA, BG_BOTTLE, 0, "Oh? No begging today? ...Good. Take it and go, and wash the bottle properly. I'm not cleaning up after you."},
    {W_NAR, BG_BOTTLE, 0, "I tuck the warm bottle away. Sophia pretends not to watch me go, but she does."}
);
SCENE(B_BFEED_ASK_MM,   /* you ask her to feed you: pairs (your line, her answer) */
    {W_ME, BG_BOTTLE, 0, "Mama... could you feed it to me? Please?"},
    {W_SOPHIA, BG_BOTTLE, 0, "Feed you?! Do you hear yourself? A grown man who can't even hold his own bottle. Hmph. ...Fine. Sit down."},
    {W_ME, BG_BOTTLE, 0, "Mama, will you hold the bottle for me?"},
    {W_SOPHIA, BG_BOTTLE, 0, "Hold it for you. Tsk. Next you'll want me to pat your back. Don't call me that. ...Come here, and be quick."},
    {W_ME, BG_BOTTLE, 0, "I'd like you to feed me, Mama. Just this once."},
    {W_SOPHIA, BG_BOTTLE, 0, "'Just this once.' You said that last time too. ...Hmph. Fine, but you will say thank you afterwards."},
    {W_ME, BG_BOTTLE, 0, "Mama, I don't want to drink it alone. Feed me?"},
    {W_SOPHIA, BG_BOTTLE, 0, "How helpless can one person be? ...Fine. Lie back, open up, and don't make a mess."},
    {W_ME, BG_BOTTLE, 0, "Please feed me, Mama. I'll be good."},
    {W_SOPHIA, BG_BOTTLE, 0, "You'll be good, will you? Hah! We both know that's a lie. Sit. Now."},
    {W_ME, BG_BOTTLE, 0, "Mama, I want you to give it to me yourself."},
    {W_SOPHIA, BG_BOTTLE, 0, "Aren't you the demanding one? Hmph. I'll do it, but only so you stop whining."}
);
SCENE(B_BFEED_MID_MM,
    {W_ME, BG_BFEED, 0, "More, please."},
    {W_SOPHIA, BG_BFEED, 0, "More? Greedy. There's a little left. ...Don't look at me like that, I'm giving it to you, aren't I?"},
    {W_ME, BG_BFEED, 0, "You're so gentle, Mama."},
    {W_SOPHIA, BG_BFEED, 0, "Gentle? I'm being patient, which is a very different thing. Don't mistake one for the other."},
    {W_ME, BG_BFEED, 0, "I feel so sleepy now."},
    {W_SOPHIA, BG_BFEED, 0, "Sleepy? You're being fed, not tucked in. Stay awake and finish it. ...Hmph. Fine, rest your head."},
    {W_ME, BG_BFEED, 0, "Thank you, Mama."},
    {W_SOPHIA, BG_BFEED, 0, "Don't thank me, repay me. And stop calling me that. ...Although I'll admit you drink very nicely."},
    {W_ME, BG_BFEED, 0, "Your hands are so warm."},
    {W_SOPHIA, BG_BFEED, 0, "Of course they are. They are the hands that take care of you. Remember that."}
);
SCENE(B_BFEED_MM,   /* beats 1-2 are filled in from B_BFEED_ASK_MM (a pair); one pair of B_BFEED_MID_MM is added before the last beat */
    {W_ME, BG_BOTTLE, 0, NULL},
    {W_SOPHIA, BG_BOTTLE, 0, NULL},
    {W_NAR, BG_BFEED, 0, "Sophia sits down and pulls me into her lap, tucking one arm under my head as if I weighed nothing at all."},
    {W_SOPHIA, BG_BFEED, 0, "Look up. Open. ...Tsk, don't gulp it. Honestly, you can't even drink properly without me."},
    {W_ME, BG_BFEED, 0, "Mmh... it's so sweet, Mama."},
    {W_SOPHIA, BG_BFEED, 0, "Of course it's sweet. It's mine. Drink every drop and be thankful, you greedy thing."},
    {W_NAR, BG_BFEED, 0, "She feeds me patiently, scolding me all the while, until the bottle is empty. She wipes my chin and pretends it was an accident."}
);
/* ---------- v13.21: BREASTFEED, part 2 (mama mode, her bedroom only): after the breastfeed scene you may ask her to continue; you lie on her bed ----------
   She always says yes, but she is more annoyed and disgusted than in the normal scene. Two sets: normal mama and cow mode (cow print outfit).
   Picture sc_bedfeed_<d8|yoga|top|cow>.png (BG BEDFEED, placeholder for now). The opener pairs play over the breastfeed picture (BG FEED). */
SCENE(B_BEDFEED_ASK_MM,   /* you ask her to continue: pairs (your line, her disgusted yes) */
    {W_ME, BG_FEED, 0, "Mama... could I stay a little longer? Please?"},
    {W_SOPHIA, BG_FEED, 0, "Longer?! Ugh, you are insatiable. It's disgusting how clingy you are. ...Fine. Lie down, but I'm timing you."},
    {W_ME, BG_FEED, 0, "Mama, I'm not ready to let go yet."},
    {W_SOPHIA, BG_FEED, 0, "Not ready? Hmph. You make my skin crawl, you know that? ...Move over, then. On the bed. Now."},
    {W_ME, BG_FEED, 0, "Just a little more, Mama. I'll be quiet."},
    {W_SOPHIA, BG_FEED, 0, "You said that already. Ugh, begging like a spoiled child. ...Lie back and keep your mouth shut."},
    {W_ME, BG_FEED, 0, "Can we lie down together, Mama? It's so comfortable."},
    {W_SOPHIA, BG_FEED, 0, "Comfortable for you! I'm the one who has to put up with it. How revolting. ...Fine, but don't get used to this."},
    {W_ME, BG_FEED, 0, "Mama, please don't make me stop yet."},
    {W_SOPHIA, BG_FEED, 0, "Don't make you stop? Do you hear how pathetic that sounds, husband? ...Tsk. Get on the bed."},
    {W_ME, BG_FEED, 0, "I'll do anything for a few more minutes, Mama."},
    {W_SOPHIA, BG_FEED, 0, "Anything. Hah! Don't make promises you'll forget. ...Ugh, fine. Lie down before I change my mind."}
);
SCENE(B_BEDFEED_MID_MM,   /* two different pairs (your line, her answer) are added in the middle, before the last three beats */
    {W_ME, BG_BEDFEED, 0, "You're so warm, Mama."},
    {W_SOPHIA, BG_BEDFEED, 0, "Of course I'm warm, I'm a person. Stop nuzzling. ...Ugh, that tickles. Disgusting. Stay still."},
    {W_ME, BG_BEDFEED, 0, "Does it still hurt, Mama?"},
    {W_SOPHIA, BG_BEDFEED, 0, "Don't ask me that while you're doing it! ...It aches less. That is not a compliment, so don't smile."},
    {W_ME, BG_BEDFEED, 0, "I could stay like this forever."},
    {W_SOPHIA, BG_BEDFEED, 0, "Forever?! Over my dead body. Look at you, clinging like a leech. ...How did I end up with you?"},
    {W_ME, BG_BEDFEED, 0, "I'm sorry if I'm a burden, Mama."},
    {W_SOPHIA, BG_BEDFEED, 0, "You are. A huge one. ...Ugh, don't make that face. Just stay put and be quiet."},
    {W_ME, BG_BEDFEED, 0, "Your heartbeat is so calming."},
    {W_SOPHIA, BG_BEDFEED, 0, "Stop listening to it! It's embarrassing. ...Hmph. And stop breathing against my skin, it's repulsive."},
    {W_ME, BG_BEDFEED, 0, "Can I rest my hand here, Mama?"},
    {W_SOPHIA, BG_BEDFEED, 0, "No. ...Fine. But if it moves even an inch, you're out of this bed. Honestly, what a nuisance."}
);
SCENE(B_BEDFEED_MM,   /* beats 1-2 are filled in from B_BEDFEED_ASK_MM (a pair); two pairs of B_BEDFEED_MID_MM are added before the last three beats */
    {W_ME, BG_FEED, 0, NULL},
    {W_SOPHIA, BG_FEED, 0, NULL},
    {W_NAR, BG_BEDFEED, 0, "Sophia sighs and sinks down onto the bed, then pulls me down beside her. We lie side by side, her arm under my head."},
    {W_SOPHIA, BG_BEDFEED, 0, "Ugh... like this? Look at me, lying here with a grown man at my chest. How did my life come to this?"},
    {W_ME, BG_BEDFEED, 0, "Thank you, Mama. This is the best part of my day."},
    {W_SOPHIA, BG_BEDFEED, 0, "The best part of your day. How sad. ...Hmph. Don't thank me, and don't call me that. It's disgusting."},
    {W_NAR, BG_BEDFEED, 0, "I close my eyes and drink in her warmth. She grumbles and shudders, but her fingers keep combing through my hair."},
    {W_SOPHIA, BG_BEDFEED, 0, "That's enough. Get off. ...Ugh, you're still clinging. Go, before I regret every second of this."},
    {W_NAR, BG_BEDFEED, 0, "She pushes me away at last, wiping her face with a huff. Her cheeks are red, and she won't look at me."}
);
SCENE(B_BEDFEED_ASK_CW_MM,   /* cow mode + Mama: you ask her to continue: pairs (your line, her disgusted yes) */
    {W_ME, BG_FEED, 0, "Mama... could I stay a little longer? Please?"},
    {W_SOPHIA, BG_FEED, 0, "Longer?! In THIS outfit?! Ugh, you are revolting. ...Fine, lie down. And never mention the spots."},
    {W_ME, BG_FEED, 0, "Mama, I'm not ready to let go yet."},
    {W_SOPHIA, BG_FEED, 0, "Not ready?! I look like a farm animal and you want MORE? ...Disgusting. Lie down and be quick about it."},
    {W_ME, BG_FEED, 0, "Just a little more, Mama. I'll be quiet."},
    {W_SOPHIA, BG_FEED, 0, "You'd better be. I'm in a cow costume because of you. ...Ugh. Lie back and not one word about it."},
    {W_ME, BG_FEED, 0, "Can we lie down together, Mama?"},
    {W_SOPHIA, BG_FEED, 0, "In this outfit, on my own bed? I feel sick. ...Fine. But if you say moo even once, you're out."},
    {W_ME, BG_FEED, 0, "Mama, please don't make me stop yet."},
    {W_SOPHIA, BG_FEED, 0, "Stop begging! It's pathetic, and I'm dressed like a dairy cow. ...Tsk. Get on the bed before I change my mind."},
    {W_ME, BG_FEED, 0, "I'll do anything for a few more minutes, Mama."},
    {W_SOPHIA, BG_FEED, 0, "Anything? Then you can burn this outfit afterwards. ...Ugh, fine. Lie down, and don't touch the print."}
);
SCENE(B_BEDFEED_MID_CW_MM,   /* cow mode + Mama: two different pairs are added in the middle */
    {W_ME, BG_BEDFEED, 0, "You're so warm, Mama."},
    {W_SOPHIA, BG_BEDFEED, 0, "Don't be sweet to me in this outfit! ...Ugh, stop nuzzling. It's humiliating enough as it is."},
    {W_ME, BG_BEDFEED, 0, "Does it still hurt, Mama?"},
    {W_SOPHIA, BG_BEDFEED, 0, "Less. Not that I'll thank you. I feel like livestock and you're enjoying it. Disgusting."},
    {W_ME, BG_BEDFEED, 0, "I could stay like this forever."},
    {W_SOPHIA, BG_BEDFEED, 0, "Forever?! I'd rather be sold at the market. ...Look at you, clinging like a calf. My skin is crawling."},
    {W_ME, BG_BEDFEED, 0, "I'm sorry if I'm a burden, Mama."},
    {W_SOPHIA, BG_BEDFEED, 0, "You are, and this outfit makes it worse. ...Ugh, don't pout. Stay still and let me get through this."},
    {W_ME, BG_BEDFEED, 0, "Your heartbeat is so calming."},
    {W_SOPHIA, BG_BEDFEED, 0, "Don't listen to it! I'm mortified, husband. ...And stop breathing on me, it's repulsive."},
    {W_ME, BG_BEDFEED, 0, "Can I rest my hand here, Mama?"},
    {W_SOPHIA, BG_BEDFEED, 0, "No. ...Fine, but if you pull on the print, you're out of this bed. What a nuisance you are."}
);
SCENE(B_BEDFEED_CW_MM,   /* cow mode + Mama: beats 1-2 are filled in from B_BEDFEED_ASK_CW_MM (a pair); two pairs of B_BEDFEED_MID_CW_MM are added before the last three beats */
    {W_ME, BG_FEED, 0, NULL},
    {W_SOPHIA, BG_FEED, 0, NULL},
    {W_NAR, BG_BEDFEED, 0, "Sophia sinks onto the bed with a look of deep disgust and tugs me down beside her, the cow print rumpled under my cheek."},
    {W_SOPHIA, BG_BEDFEED, 0, "Ugh... lying here in this outfit with a grown man at my chest. I'm going to burn this thing afterwards."},
    {W_ME, BG_BEDFEED, 0, "Thank you, Mama. This is the best part of my day."},
    {W_SOPHIA, BG_BEDFEED, 0, "The best part of your day. How sad. ...Hmph. Don't thank me, and don't call me that. It's disgusting."},
    {W_NAR, BG_BEDFEED, 0, "I close my eyes and drink in her warmth. She grumbles and shudders, but her fingers keep combing through my hair."},
    {W_SOPHIA, BG_BEDFEED, 0, "That's enough. Get off. ...Ugh, you're still clinging. Go, before I regret every second of this."},
    {W_NAR, BG_BEDFEED, 0, "She pushes me away at last, tugging at her cow print with a huff. Her cheeks are red, and she won't look at me."}
);
/* ---------- v13.18: PLAYING ROOM versions of PUMP MILK / ASK FOR MILK / BREASTFEED (married, at the poker table). She sits on the POOL TABLE.
   Used instead of the living-room (_LR) texts while you talk to her in the playing room. Pictures: pump_pr.png, sc_feed_pr_<outfit>.png, sc_bottle_pr_<outfit>.png, her_hide_no_pr_ / her_hide_yes_pr_<outfit>.png (placeholders). ---------- */
POOL(R_PR_HERE,
    "Here?! On the pool table?! ...Hmph. Fine, but keep your voice down, husband.",
    "Here?! In the playing room? ...Anyone could walk in. Lock the door first.",
    "Here?! Have you lost your mind, husband? I am not sitting on a pool table for this!",
    "H-here?! Among the cues and the chalk? ...Fine. But make it quick.",
    "Here?! This is where you play pool, husband! ...Hmph. Turn the light down, then.",
    "Here?! You want me on the pool table like some trophy? ...Keep it quiet.",
    "Here?! On the felt? ...Hmph. Spread a towel on the pool table first, husband.",
    "Here?! Right next to the poker table? ...Don't stare at the door. Just hurry.");
static const Beat B_LEAD_PR_D[LEAD_N - 2][LEAD_BEATS] = {
    {   {W_NAR,    BG_PLAYROOM, 0, "Sophia stops beside the pool table and presses her arms tightly against her chest."},
        {W_SOPHIA, BG_PLAYROOM, 1, "...It's happening again. It feels so full and heavy that it aches. I can barely stand up straight, husband."},
        {W_ME,     BG_PLAYROOM, 1, "That means the pills are working. Why don't you sit down and let it out?"},
        {W_SOPHIA, BG_PLAYROOM, 1, "Don't say it like it's nothing! ...Fine. I'll sit on the pool table. Just don't you dare look at me."} },
    {   {W_NAR,    BG_PLAYROOM, 0, "Sophia glances at the door, then hoists herself onto the edge of the pool table, her cheeks turning pink."},
        {W_SOPHIA, BG_PLAYROOM, 1, "My blouse is so tight again. It's hot and heavy, and I can't think about anything else."},
        {W_ME,     BG_PLAYROOM, 1, "Then it's time to use the pump again, isn't it? It helped last time."},
        {W_SOPHIA, BG_PLAYROOM, 1, "I know that already! ...Just keep your eyes on the cue rack this time."} },
    {   {W_NAR,    BG_PLAYROOM, 0, "Sophia lets out a shaky breath and sinks onto the pool table, gripping the green felt with both hands."},
        {W_SOPHIA, BG_PLAYROOM, 1, "It feels strange, husband. It feels like it's about to burst. This is all because of your stupid pills."},
        {W_ME,     BG_PLAYROOM, 1, "I'll stay right here. Tell me what I can do."},
        {W_SOPHIA, BG_PLAYROOM, 1, "...Just turn around and stare at the poker table. And no peeking."} },
    {   {W_NAR,    BG_PLAYROOM, 0, "She takes the pump out of her bag and sets it on the edge of the pool table. Her ears are bright red."},
        {W_SOPHIA, BG_PLAYROOM, 1, "What, with the pump? On a pool table? No... I will not do it."},
        {W_ME,     BG_PLAYROOM, 1, "Sorry. I want you to feel better."},
        {W_SOPHIA, BG_PLAYROOM, 1, "Fine, give the pump to me. Now, turn around and face the wall."} }
};
static const Beat (*B_LEAD_PR)[LEAD_BEATS] PQ_UNUSED = B_LEAD_PR_D;
static int B_LEAD_PR_N PQ_UNUSED = LEAD_N - 2;
SCENE(B_BOTTLE_PR,
    {W_NAR,    BG_PLAYROOM, 0, "Sophia takes out the breast pump and the bottle you gave her and perches on the edge of the pool table. Her ears are already red."},
    {W_SOPHIA, BG_PLAYROOM, 1, "...Turn around. Face the poker table. And don't even think about looking."},
    {W_ME,     BG_PLAYROOM, 1, "I'm already facing the wall."},
    {W_SOPHIA, BG_PLAYROOM, 1, "I still hate this. Why does it have to be on the pool table... Hmph. Just don't turn around!"},
    {W_NAR,    BG_PLAYROOM, 0, "I stare at the wall. Behind me I hear the soft hum of the pump and her unsteady breathing."}
);
SCENE(B_PEEKPIC_PR,
    {W_NAR,    BG_PUMP, 0, "I can't help it. I turn my head just a little and glance back over my shoulder."},
    {W_NAR,    BG_PUMP, 0, "Sophia sits on the edge of the pool table, her eyes fixed on the bottle and her cheeks bright red."},
    {W_SOPHIA, BG_PUMP, 0, "...Almost... just a little more..."},
    {W_NAR,    BG_PUMP, 0, "She hasn't noticed me yet. Before she does, I quickly turn back to the wall."}
);
SCENE(B_FEED_PR,   /* BREASTFEED in the playing room: beats 1-2 she covers her chest (the "Here?!" line comes before), then the picture sc_feed_pr_<outfit> (BG FEED) */
    {W_NAR,    BG_PLAYROOM, 1, "I ask her to breastfeed me. Sophia quickly crosses her arms over her chest and looks at the pool table, her ears bright red."},
    {W_SOPHIA, BG_PLAYROOM, 1, "...Hmph. Fine. You are my husband. Let me sit on the pool table first, and don't stare while I get ready."},
    {W_NAR,    BG_FEED, 0, "Sophia sits on the edge of the pool table and pats the green felt beside her, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "My chest is so full it aches. You are my husband now, so... just this once, and quietly, in case someone walks in."},
    {W_ME,     BG_FEED, 0, "I won't make a sound. Tell me if it's too much."},
    {W_NAR,    BG_FEED, 0, "She draws me close and holds me against her, one hand gently stroking my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Hmph. Don't look so happy. You're being awfully greedy. And keep an eye on the door."},
    {W_NAR,    BG_FEED, 0, "Afterwards she lets out a long, relieved breath, and I stay in her arms a little longer. The ache is gone."}
);
SCENE(B_FEED_MM_PR,   /* mama mode, playing room: beats 1-2 are REPLACED by a random opener from B_OPEN_FEED_MM_PR (these two stay as the fallback) */
    {W_NAR, BG_PLAYROOM, 1, "I step up to the pool table and ask softly, \"Mama... may I?\" Sophia crosses her arms over her chest, her whole face red."},
    {W_SOPHIA, BG_PLAYROOM, 1, "Don't call me that! ...And not out here, either. I'm only doing this because it aches. That's all."},
    {W_NAR, BG_FEED, 0, "Sophia sits on the edge of the pool table, avoiding my eyes, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "...Just... just this once. And if anyone knocks, we stop. Okay? ...I'm still not sure about this."},
    {W_ME, BG_FEED, 0, "I understand. Tell me if it's too much, Sophia."},
    {W_NAR, BG_FEED, 0, "She draws me close, stiff and embarrassed, one hand resting hesitantly in my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Good. Like that. Don't look up at me. ...I still don't like that name."},
    {W_ME, BG_FEED, 0, "Thank you."},
    {W_NAR, BG_FEED, 0, "Afterwards she lets out a long breath and tells me to forget it ever happened. She still holds me a little longer."}
);
SCENE(B_FEED_CW_PR,
    {W_NAR, BG_PLAYROOM, 1, "I ask her to breastfeed me. Sophia crosses her arms over her chest and looks down at her cow print outfit with disgust."},
    {W_SOPHIA, BG_PLAYROOM, 1, "Right now?! In this? ...Ugh. Fine. You're my husband. But I feel disgusting. Let me sit on the pool table, and don't stare."},
    {W_NAR, BG_FEED, 0, "Sophia sits on the edge of the pool table, her mouth pressed into a thin line, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "My chest is so full it aches, otherwise I would never. ...Not a word about this outfit, and keep it quiet."},
    {W_ME, BG_FEED, 0, "I won't. Tell me if it's too much."},
    {W_NAR, BG_FEED, 0, "She draws me close, stiff and sour-faced, one hand resting in my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Hurry up. I hate how this feels. Don't look so happy. And keep an eye on the door."},
    {W_NAR, BG_FEED, 0, "Afterwards she lets out a long breath and mutters that I'd better never mention it. She still holds me a bit longer."}
);
SCENE(B_FEED_CW_MM_PR,
    {W_NAR, BG_PLAYROOM, 1, "I step up to the pool table and ask softly, \"Mama... may I?\" Sophia crosses her arms over her chest, glaring down at the cow print."},
    {W_SOPHIA, BG_PLAYROOM, 1, "Don't call me that! ...And not out here, in this disgusting outfit. I'm only doing this because it aches."},
    {W_NAR, BG_FEED, 0, "Sophia sits on the edge of the pool table, avoiding my eyes, her ears bright red."},
    {W_SOPHIA, BG_FEED, 0, "...Just this once. If anyone knocks, we stop. And not a word about me, the outfit, or that name."},
    {W_ME, BG_FEED, 0, "I understand. Tell me if it's too much, Sophia."},
    {W_NAR, BG_FEED, 0, "She draws me close, stiff and sour-faced, one hand hesitant in my hair."},
    {W_SOPHIA, BG_FEED, 0, "...Good. Like that. Don't look up at me. ...I hate this outfit, and I still hate that name."},
    {W_ME, BG_FEED, 0, "Thank you."},
    {W_NAR, BG_FEED, 0, "Afterwards she lets out a long breath and tells me to forget it ever happened. She still holds me a little longer."}
);
SCENE(B_OPEN_FEED_MM_PR,
    {W_ME, BG_PLAYROOM, 1, "Mama... may I? My stomach feels so empty."},
    {W_SOPHIA, BG_PLAYROOM, 1, "M-mama?! Don't call me that! Here?! On the pool table?! Absolutely not. Anyone could walk in."},
    {W_ME, BG_PLAYROOM, 1, "Mama, please... can I have some?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "Don't call me that! Out here?! I'm hiding behind this pool table and you ask me that? No."},
    {W_ME, BG_PLAYROOM, 1, "Mama... I'm really hungry. May I?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "Don't say that name! Here? In the playing room?! I said no. Don't ask again."},
    {W_ME, BG_PLAYROOM, 1, "Mama... can I rest against you? Just for a while?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "W-what?! Don't call me that! Rest on the pool table?! You know where that leads. No!"},
    {W_ME, BG_PLAYROOM, 1, "Mama... it's just us. May I have some now?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "Stop calling me that! Just us? Here? The door could open any second! No way."},
    {W_ME, BG_PLAYROOM, 1, "Mama, I'll be quick. May I?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "Mama?! No! Quick? On a pool table?! That makes it worse! I refuse. Absolutely not."}
);
SCENE(B_OPEN_ASK_MM_PR,
    {W_ME, BG_PLAYROOM, 1, "Mama... could you give me some milk? Please?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "Don't call me that! Milk?! Here?! Next to the pool table? Absolutely not."},
    {W_ME, BG_PLAYROOM, 1, "Mama, is it full yet? I'd like a bottle."},
    {W_SOPHIA, BG_PLAYROOM, 1, "Stop calling me that! A bottle?! Here, with the door unlocked? No. Never."},
    {W_ME, BG_PLAYROOM, 1, "Mama... I'd love some of your milk today."},
    {W_SOPHIA, BG_PLAYROOM, 1, "Don't say it like that! Here?! Someone could see! No. Not in the playing room."},
    {W_ME, BG_PLAYROOM, 1, "Mama, I'm thirsty. Can you pump some for me?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "Mama?! No! Don't call me that! Pump?! On the pool table?! Have you lost your mind?"},
    {W_ME, BG_PLAYROOM, 1, "Mama... my bottle is empty. Could you fill it?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "Don't call me that! Fill it out here?! Absolutely not! Do you want the whole hotel to know?"},
    {W_ME, BG_PLAYROOM, 1, "Mama, could you pump some for me today?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "Mama?! Stop that! Pump here?! Among the pool cues?! No. Not a chance."}
);
SCENE(B_OPEN_PILL_MM_PR,
    {W_ME, BG_PLAYROOM, 1, "Mama, here's your pill. Take it for me?"},
    {W_SOPHIA, BG_PLAYROOM, 1, "Don't call me that! I'm not taking anything from you in the middle of the playing room!"},
    {W_ME, BG_PLAYROOM, 1, "Mama... time for your pill. Open up."},
    {W_SOPHIA, BG_PLAYROOM, 1, "Stop calling me that! Open up?! I am not a child. Keep that pill away from me."},
    {W_ME, BG_PLAYROOM, 1, "Mama, please take this one. For both of us."},
    {W_SOPHIA, BG_PLAYROOM, 1, "M-mama?! Both of us?! No! I'm not swallowing anything while you say that."},
    {W_ME, BG_PLAYROOM, 1, "Mama, your pill. I'll hold the water for you."},
    {W_SOPHIA, BG_PLAYROOM, 1, "Don't call me that! I can hold my own glass. And no, not on the pool table. Not now."}
);

#include "talk.h"   /* v7.7: CHAT and TEASE */

#endif
