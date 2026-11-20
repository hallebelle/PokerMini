# Poker Mini (base build: pokermini.zip = former pokerqueen39, v7.5)

Handheld heads-up fixed-limit Texas Hold'em against the Poker Queen (Sophia), for the Miyoo Mini (Onion OS 4.4).
Native C + SDL (SDL 1.2 on the Miyoo, SDL2 on desktop/web). Built entirely by GitHub Actions.

## Files
- `assets/cards.png`  (v12.9) the card faces (red / black deck sheet made by `tools/make_cards.py`)
- `src/main.c`   screens, drawing, input, screenshots (`--shots`) and flow test (`--autoplay`)
- `src/logic.h`  rules, betting, her AI, match payout, ATM, hiring, save file (no SDL)
- `src/lines.h`  her dialogue (two-line table lines and coach-scene pages); `src/talk.h` chat and tease exchanges
- `src/extlines.h` + `src/extreg.h`  (v7.9) the loader for `lines.txt` / `talk.txt` and the list of everything those files can change
- `src/extitems.h`  (v9.5) the loader for `items.txt` (item names and info) and `names.txt` (the names you can type for her, R2 in her menu)
- `packaging/lines.txt`, `packaging/talk.txt`, `packaging/items.txt`, `packaging/names.txt`, `packaging/TEXT_GUIDE.txt`  (v7.9, v9.5) the editable text files (a dump of the built-in text) and the guide
- `tests/test_logic.c` logic tests (run by CI before the build)
- `assets/`, `tools/make_assets.py`, `tools/make_walk_assets.py` (maps and sprites), `packaging/` (Onion app files)
- `tests/sdl_stub/` a tiny stand-in for SDL2 + stb (python/Pillow for PNGs) so `--autoplay` / `--shots` can run where SDL is not installed: `gcc -O1 -isystem tests/sdl_stub src/main.c -o pq_test -lm && ./pq_test --autoplay`

## Walking and controls
You walk around your room, the hall and the lobby (maps `assets/map_*.png`, RPG Maker exports enlarged 3x = 72 px tiles; you are drawn at 2.2x (about 145 px), the receptionist at about 1.93x (79 px); sheet `assets/player.png`, `assets/clerk.png`).
Miyoo: D-pad walks (one tile per 350 ms, hold to keep going; no running), A uses what you face (a gold "A" bubble shows when something answers), Menu = Resume / Exit.
Doors are never walked into: face a door and press A (your room, the hall's lobby doorway, the bathroom, the Event Center opening, Sophia's door, the glass doors). Small bobbing icons float over the fridge (inventory), the ATM, the receptionist (store) and the Event Center opening (once unlocked); the thing you face shows the A bubble instead.
At the table: A call/check, B fold, X raise/bet. Start = skip a story scene.
Desktop/web: Z = A, X = B, C = X, V = Y, arrows, Esc = Menu.
ATM amount: Up/Down change the selected digit, Left/Right move, Y = max, X = clear, A = confirm, B = back.
Places: your room (fridge = inventory, bed or sofa = save, left mat = hall, right mat = bathroom, locked), hall (barred door = your room, dark doorway = lobby, mat at the left end = Sophia's door, A knocks),
lobby (right end of the desk = ATM, receptionist in the desk notch = store, left opening = Event Center once unlocked, bottom doors blocked by the storm).
Tile letters and the door table are at the top of the walking block in `src/main.c` (MG_ROOM / MG_HALL / MG_LOBBY, DOORS).
Menu in other places: at her table = Resume / Leave table (counts as a loss); story = Resume / Skip scene; title = exit. In your room, hall and lobby Menu offers Resume / Exit only.

## Save file
`pokerqueen.sav` next to the program. The game is saved ONLY when you say yes at the bed or the sofa in your room. Exit (Menu) warns you and goes back to the last save;
closing the app without saving loses everything since the last save (this includes walking out of an Exhibition: the buy-in is simply gone only for that session's progress). Delete the file to start over.
Leaving a table through Menu still counts as a loss (in memory). Save version 5 (new: hall_seen). An older save resets once.

## Quick Play, Event Center, Exhibition
Quick Play (title menu): 1 vs 1 against the CPU, starting chips 100/250/500/1000, chips only, no money, no saving, no card bonuses, no energy cost.
Event Center (unlocked after the forced first match against Sophia; until then its opening is a plain wall): Exhibition only (same game, but the buy-in 10/50/100/500/1000 is
paid from CASH when you sit down; the winner takes 3.5x the buy-in; leaving or closing the game forfeits the buy-in). It needs and uses energy by buy-in (v13.16): $10 = 5%, $50 = 10%, $100 = 15%, $500 = 25%, $1000 = 30%, the $5000 boss table 30%.
Engine: src/multi.h (fixed limit, stacks carry over, all-ins with side pots, blinds rise every 10 hands, CPU AI; it still supports up to 4 seats). Prize multiplier: g_ex_prize() in logic.h.

## Energy, store, inventory (your room)
Energy bar at the top left of your room, 5 segments of 20%. Matches against Sophia no longer cost energy: they cost CASH, paid when the match STARTS:
10 hands $10, 20 hands $20, 30 hands $30. A tie gives the fee back; a loss or walking out keeps it gone; the forced first match is free. Without the cash for
the cheapest game she refuses to play you (no free rematch). Energy is spent by an Exhibition (5 to 30% by buy-in, v13.16), by the 8 ball (same scale; the tournament 50%, once a week), by a match at her 3rd-floor table (20%, v12.4) and by a free match at home (10%).
Store (room menu, art assets/store_menu.png): X buys a soda ($1, +20%), Y buys an energy drink ($3, +50%), with a confirmation; paid from CASH.
Inventory (room menu): holds bought and won drinks; A drinks the selected one (not at full energy). Winning vs Sophia over 20 hands gives a free soda,
over 30 hands a free energy drink. Pity rule: with 0 energy, no drinks and no money anywhere, the store gives one free soda, once per save.
You start the game with 100 cash (START_CASH in logic.h). Constants: ENERGY_*, EX_ENERGY_OF / g_buyin_energy(), EX_BUYIN, PRICE_*, REST_*, g_match_cost() in logic.h. Bar art is built by tools/make_energy.py.

## Opening
New game: black monologue screen (two pages, centred, typed out; A skips the typing, then turns the page; START skips it all, text in `M_PAGE`, `src/lines.h`) -> arrival lines on the lobby map (you at the entrance) -> fade -> you walk to the front desk (hint at the top; every other door answers "check in first") ->
press A at the desk -> the bump scene with her, on the PAINTED lobby picture (assets/entrance.png) -> fade -> you stand in front of the desk, free, hint "GO TO YOUR ROOM" -> the first time you reach the hall you cannot move: you look at her
door, then yours, each line closed with A -> walking is free. Lines: `src/lines.h` (B_INTRO, B_BUMP, W_HALL1..3).
A at the glass doors (bottom of the lobby) shows assets/windows_rain.png with two lines (B_RAIN).

## Sophia's door and her room (v3.9)
Before the forced first match is over, A at her door just knocks. Afterwards A opens a small menu: Knock on her door (the normal greeting, then Play / Coach / Leave) or Enter her room.
Enter her room: fade to assets/sophia_room.png with a line, then Check her bedroom (-20% energy, one look per visit) / Check her bathroom (does nothing yet, free, can be repeated).
Bedroom: fade to assets/sophia_sleeping.png with Admire (three pages, SN_ADMIRE) / View (picture only). B goes back one level at a time: picture -> bedroom menu -> room menu -> out to the hall.
With less than 20% energy the bedroom answers "too tired" and costs nothing. Nothing about her room is saved. Cost: SNEAK_COST in main.c.

## v4.1 additions
- Sophia's table: shock / smug faces (`assets/her_shock.png`, `her_smug.png`), dealing (her card first, then yours, alternating), board cards flying in face down then turning up, chips moving after the cards (press A once to skip), pink card backs for both of you here. Your cards are face down until you press Y (Y again hides them); the same on the Quick Play / Exhibition tables with blue backs. Two-colour suits everywhere.
- After a match she sends you out of her room (`SCR_AFTER`, `assets/ignore.png` over `assets/her_room.png`); reward lines on a win, "get out" lines on a loss or tie.
- Title screen floats red suit outlines (`assets/title_suits.png`, rebuilt with `python3 tools/make_title_suits.py` from `tools/suits_sheet.png`).


Hotel (v13): the clerk and ATM icons appear after you have been in your room; the bedroom door in the middle wall of your room needs A (you cannot walk through the gap).

## v4.4 additions
- Every money amount is drawn with a small green $ badge (the `$` character in any string passed to `text()` becomes the badge): pot, NET, stacks (you / Sophia / CPU), prize, buy-ins, bet buttons, cash, ATM, store, end screens.
- Hall look-around: the camera pan takes PAN_MS (2.6 s, smoothstep) and rests PAN_HOLD (0.7 s); the room's line appears only afterwards (wk_mode 3, no input meanwhile).
- Showdown outline in steps (HL_STEP 450 ms): opponent hole cards, your hole cards, then the board cards (empty groups are skipped); the result banner waits for the last step.
- R1 (T on the keyboard) at Sophia's table opens `assets/close_up_torso.png` (no on-screen hint; your turn, table calm, once per hand). A or B closes it, then she says L_CAUGHT in the bottom dialogue box.
- Her bed menu has 3 entries (Admire / Look closer / Look longer). After the last line of a look, A shows that picture fullscreen (B skips it); closing it returns to the menu.
- Exhibition: needs and uses 5 / 10 / 15 / 25 / 30% energy for the $10 / 50 / 100 / 500 / 1000 table (EX_ENERGY_OF, g_buyin_energy, v13.16; boss 30%), prize 3.5x the buy-in (g_ex_prize).

## v4.5 additions
- Shirt debt: the bump scene now ends with Sophia demanding $300 (DEBT in logic.h). The Event Center / Exhibition opens at the end of that scene. Her door answers "I don't have the money, I will come back later." until you hold $300 CASH (bank does not count); then A starts the first-visit scene, which takes the $300 and leads to the free first match as before.
- Objectives: top hint ENTER YOUR ROOM after the look down the hall (until you are inside). Menu / Select = Resume / Objective / Exit while walking; the Objective page says what the story wants (debt with your cash, etc.).
- R1 close-up: the whole picture (not cropped, not shrunk; only its empty 30 px top margin is off-screen) with a short two-line box under it holding one of 8 thoughts; then she answers with one of 8 lines. A small R1 badge sits left of your hand box while R1 works.
- GAME OVER: in the debt phase or after the first match, if cash + bank is under the cheapest Exhibition buy-in ($10), the walking screen turns into GAME OVER; A deletes the save and returns to the title with a fresh game.
- Title screen is red. Saves from before the debt open the Exhibition on load (no reset).

## v5.0 additions
- Time of day: Day 1 starts in the morning. Only saving moves time: the sofa (morning, evening) goes morning -> evening -> night; the bed (night only, "SLEEP AND SAVE?") starts the next morning. Day/phase panel under the cash box. Night = slightly darker maps (not Sophia's room, already dark). Save version 6.
- Sophia's door: at night the menu is KNOCK / QUIETLY ENTER; knocking at night is an angry scene (assets/her_night.png, placeholder). In the daytime A knocks directly.
- After paying the $300 shirt debt she sends you away; the second knock tells the rules once, then you pick 10/20/30 hands and pay the normal fee. No free first match.

## v5.1 (replaces the older notes above where they differ)
- Lore: Sophia can no longer be hired. The coach / hire path, the 10,000 goal and the "she works for you" placeholder are gone. After the 300 shirt debt there is no fixed goal.
- Sophia matches: always 10 hands, buy-in 100 / 500 / 1000 (`SOPH_FEE`), paid when the match starts. The buy-in is also the stake: each chip is worth fee/100 money (x1 / x5 / x10) and card bonuses are multiplied the same way (`g_mult`). A tie returns the buy-in. A win at 500 gives a free soda, at 1000 a free energy drink. Exhibition and Quick Play are unchanged. Cheapest seat: 100 cash.
- Key card: right after the bump scene (`SC_KEY`, `assets/key_card.png`, lines `K_FIND/K_KEEP/K_RETURN`): A keep it (`G.keyst = 1`, the quiet way into her room at night works) or B return it to the receptionist (`G.keyst = 2`, 2 free energy drinks, no night entry until game over). Without the key the night knock only gets the angry scene. New game resets it. Save version 7 (an older save resets once).
- Watching her sleep, Look closer and Look longer cost no energy; the bedroom no longer says "too tired". Energy is now only used by the Exhibition (10%).
- Title menu: OPTIONS (between Career Mode and Exit): dialogue box opacity 100/80/60/40/20/10% with a live sample, stored in `pokerqueen.cfg` (separate from the career save, so game over does not reset it). Default 60%.
- Quick Play and Exhibition: a small box under the CPU's name shows its two cards (face down, turned up at the showdown, outlined like the table cards).

## v5.2 additions
- TESTING: a new career starts with $300 in the BANK (`TEST_BANK` in logic.h, used by `g_new()` at the title / after GAME OVER). Cash stays $100: withdraw the 300 at the ATM to pay the shirt debt. Set `TEST_BANK` to 0 for the final build. An existing save is not changed (delete `pokerqueen.sav`).
- Sound: six effects (`assets/*.wav`, converted from the supplied mp3 files to 22 kHz mono) played through SDL's own audio callback (no SDL_mixer; if audio cannot open the game stays silent). Shuffle at the start of a hand, Give_Card for each dealt / board card, Open_Card when board cards and her cards turn up and when you peek with Y, Placing_Chip for blinds / calls / raises, All_in_Chip for an all-in, Collect_Pot_Chip when the pot goes to the winner. Same on Sophia's table, Quick Play and Exhibition. v13.12: two more effects for the 8 ball (both match types): `assets/ball_sound.wav` on every ball-against-ball hit (volume follows the impact speed, a ball that only creeps makes no sound) and `assets/ball_hole.wav` when a ball drops into a pocket (the chip / card placeholders are gone from the pool). The sfx mixer now has a per-voice volume (`sfx_play_v`). v13.13: four more effects: `assets/door_knock.wav` (a narrator line about knocking plays it when the line appears; Sophia's own lines never do), `assets/door_open.wav` (every map door you walk through, and when she lets you into her room) and `assets/door_close.wav` (just after you arrive through a door, 60 percent volume, also when you leave her room), `assets/elevator.wav` (riding a floor from the elevator menu and the scripted rides after paying the shirt and after she moves in; no door sounds on a ride). Refused doors / rides (the unpack rule, locked lines, the glass lobby doors) stay silent.
- OPTIONS (title menu) now has three rows (UP/DOWN to pick the row, LEFT/RIGHT to change): DIALOGUE BOX opacity, GUI SIZE (NORMAL / LARGE: bigger letters and a taller dialogue box, 31 characters per line), SOUND (ON / OFF). Saved in `pokerqueen.cfg`.
- Key card scene: the lobby picture stays visible and the key card picture is shown at 50% in the middle (key_card.png is scaled by 1/2 when loaded).

## v5.4 controls note
L1 (E on desktop and Onion) looks at / hides your cards, R1 (T) is the close-up. Options are in every pause menu (Menu button). Sophia's win pays a prize of 5 x the buy-in on top of the net chips.

## v5.5 rules note
- Sophia plays you **once per day** (the day number; sleeping starts a new day). Knocking again gives you one of her refusal lines.
- **Rent**: $100 every 7 days (day 7, 14, 21 ...). Pay at the front desk (cash) while the receptionist is there; "RENT DUE TODAY" shows in red under the day box. When the night starts, unpaid rent is taken from your cash, then your bank. If cash + bank is under $100 it is game over.
- Tuning: `RENT_AMT` and `RENT_DAYS` in `src/logic.h`.

## Floors, stairs and elevator (v5.6)
1st floor: the lobby (reception, ATM, Event Center) and, through its dark doorway, the stair hall (assets/map_hall1.png). 2nd floor: the hall (map_hall.png) with your room. 3rd floor: Sophia's hall (map_hall3.png) and her room.
Stairs connect the 1st and 2nd floor only (the mat at the bottom of the steps in the stair hall, the dark doorway on the 2nd floor; A on it). The elevator (A at its doors) reaches all three floors and lists the OTHER two floors;
the 3rd floor is listed only with Sophia's floor key card (floor_card() = story >= ST_HOME: she hands it over at the end of the bump scene and forgets to take it back after the debt is paid; no image, narration only).
Maps, tile letters (L elevator, U stairs door) and DOORS are at the top of the walking block in src/main.c; the floor pictures are cropped by tools/make_floor_maps.py (sources tools/floor1_src.png, floor2_src.png, floor3_src.png).
First visit to the 2nd and the 3rd floor: a line, the camera swings to the door, a second line (G.hall_seen bit 1 = 2nd floor, bit 2 = 3rd floor; an old save with hall_seen 1 sees the 3rd-floor look once).

## v5.9 additions
- Stair hall (1st floor): the stairs are two tiles wide (columns 10 and 11), so you can walk the whole width.
- Event Center is a walkable room (assets/map_event.png, made by tools/make_event_map.py from tools/event_src.png; grid MG_EVENT in src/main.c). The lobby opening leads in through the wooden door (right wall). The clerk (same sprite) stands in the notch of her desk: A in front of the desk = her greeting, then the Exhibition buy-in list; B = back at the desk. The two red doors, the machine, benches and posters are only picture. No night version: it is lit 24/7 and the clerk is always there.
- After an Exhibition (finished, or walked out) you stand in front of the match-room door (EV_OUT_X/EV_OUT_Y), facing down.
- Sophia's bedroom: coming back from the sleeping pictures you stand in front of her bedroom door facing UP (away from it).
- Sophia's board cards: 43 px wide (were 40), centres 48 px apart (were 45).

## v6.1 additions
- Sophia is now the boss of the Exhibition. Paying the $300 shirt debt makes her take her floor key card back (you wake up on the 2nd floor in front of the elevator, which no longer offers the 3rd floor). Win the $1000 Exhibition once and a sixth table appears: $5000 buy-in, fixed $10000 prize, Exhibition format, she plays with Sophia's brain. Lose and you can try again. Win and she gives her name, hands the card back and invites you to her room; the table is gone for good.
- Her result lines at her own table now come after the result banner; A waits for her line. A short pause (shuffle sound) precedes every new hand.
- `assets/boss_sophia.png` is a placeholder (confident.png with a recoloured outfit). Replace it with the real picture under the same name.
- Save version 9: older saves are not loaded (a new career starts).

## v6.2 additions (romance, new room, online store)
- **New room** (11 x 7 tiles, `assets/map_room.png`; upgraded: `assets/map_room_up.png`, made by `tools/make_romance_assets.py` from the artist's 312x216 exports). Grid `MG_ROOM` / `MG_ROOM_UP` in `src/main.c` (new tile letters: `P` the computer, `g` her bedroom door). Start tile `ROOM_SX/ROOM_SY`. The upgraded picture is used once the room is upgraded or she lives with you (`g_room_lvl`, v9.3).
- **Online store** (the computer in your bedroom, `SC_SHOP`): Flower $500 (+10%), Money stack $2000 (+20%), Breast Enhancement Pill $10000, Ruby ring $12000. Paid from cash first, then the bank. The ring and the pill are one time (sold out afterwards). Prices: `SH_PRICE` in `logic.h`.
- **Story**: buy the ring -> beat her at the Exhibition (card back) -> at her door PROPOSE (`SCR_PROPOSE`, ends with her question: YES / NOT NOW). YES: next time you enter your room she visits and demands a bigger one (`SCR_DEMAND`), then the receptionist menu offers UPGRADE ROOM $5000 (cash only; rent becomes $300, `g_rent_amt`). Upgraded: ASK TO MOVE IN at her door (`SCR_MOVEIN`); asking before the upgrade sends you away. NOT NOW: no upgrade ever, she stays on the 3rd floor, matches still cost a buy-in, ASK TO MOVE IN still works. States: `RS_*` in `logic.h`.
- **Living with you**: the 3rd floor is closed (no elevator stop, her door is locked). Knock on the door beside the window in your living room (`g`) -> `SC_ROM`: visual-novel scene (her picture, `assets/rom_room.png` as the background, the romance bar top left, no energy bar). Menu: GIVE A GIFT / PLAY A MATCH / LEAVE.
- **Gifts**: one per day (`gift_day`), flower +10%, money stack +20%. The romance bar has 5 levels (v11.6; each level is harder: a gain counts 100 / 90 / 75 / 60 / 50 %); a full bar starts the next level at 0; the pill can only be given at a full level 5 bar, once, and switches her pictures (`confident_p.png`, `her_neutral_p.png`, `her_shock_p.png`, `her_smug_p.png` are PLACEHOLDERS - replace them).
- **Matches at home**: once a day (shared with the buy-in matches), 10 hands (+5% when you win) or 20 hands (+10%), costs 10% energy (`HOME_ENERGY`), no money, no card bonus, no drink. A home match is `Game.home`.
- Save version 10 (older saves are not loaded). Unused art: `baby_bottle.PNG` (not wired in).


## v6.5 milk system (after the day-8 scene)
Store: Breast Pump $500 (one time) and Milk Inducer Pill $500 are listed after the day-8 scene. Give the first pill, wait 2 days, her "even more stuffy" scene, give the pump (the bottle gauge appears top right of her scene only),
then each pill is +10%; 10 pills = a Baby Milk Bottle (inventory, A to drink: no energy drop today and tomorrow). Pump and pill ignore the one-gift-a-day rule. Save version 12.

## Screen size (752x560, the Miyoo Mini V4 panel size)
- The game is still laid out in 640x480 "logical" units (every coordinate in `src/main.c`), but it is DRAWN at the real screen size `SW x SH`: rectangles, shading, circles and letters are rasterised at full resolution, pictures are resampled once at load (bilinear, premultiplied alpha), cards are projected at screen resolution. At 640x480 everything is the identity (the 355 test screenshots are pixel-identical to v6.8, except the new line in OPTIONS).
- Miyoo build (`-DUSE_SDL1`) asks SDL for 752x560 and falls back to 640x480 if that mode is refused; the size the system really gives is used. Desktop build stays 640x480; `PQ_SIZE=752x560 ./pq_test --shots` (or `--autoplay`) tests another size.
- OPTIONS shows the size the game actually got ("SCREEN 752X560", top right of the panel).
- Memory: only one night copy of a map is kept (made when you enter a map at night), only the room picture in use is loaded, the late-game pictures (pill versions, day 6 / cover, boss) load when first needed and the ones they replace are dropped. Peak in the desktop autoplay: 83 MB at 752x560 (v6.8 at 640x480: 93 MB).
- Helpers: `XS()/YS()` logical -> screen pixel, `LW()/LH()` logical size of a loaded picture, `blit_src()` a part of a picture, `Rf()` rectangle in 1/100 logical px.
- v7.1 native art: a picture that is exactly 752x560 is loaded 1:1 (NATW/NATH in `src/main.c`); its logical size is still 640x480. Other pictures keep the resample. Make new full-screen art at 752x560.
- v7.1 your room: the sofa fills two tiles (1,4) and (1,5). The first time in your room you walk two steps up by yourself, a slow fade puts you in the bedroom at (7,5) facing left, then the "finished unpacking" line.

## v7.2 speed (screen 752x560, no asset is downscaled)
- Build flags: `-O3 -mcpu=cortex-a7 -mfpu=neon-vfpv4` (Miyoo Mini CPU) so the pixel loops can be vectorised.
- `shade_n` (panels, fades, shadows): red and blue are blended in one multiply, green separately; falls back to the old loop for other pixel layouts. Same result, pixel for pixel.
- `tilt` (cards on the table): no `SDL_GetRGB`/`SDL_MapRGB` per pixel any more; source row looked up once per row.
- `rotate180`: pointer walk instead of index maths (and the middle row is handled if the height is odd).
- Main loop waits only the rest of the 16 ms (`SDL_GetTicks`), not 16 ms on top of the drawing time.
- `night_t`: pixel format read once per picture.
- Checked: all 355 screenshots at 752x560 are identical to the v7.1 ones, autoplay 0 problems.

## v7.3 smooth walking map (screen 752x560)
- The camera is now kept in 1/256 logical px and eases by elapsed time (`cam_ease`), not by a fixed fraction per frame, so the map and your character stay in step whatever the frame time.
- The map, the clerk, the icons and your character are placed in whole screen pixels (`XSF`/`YSF`, `ADJX`/`ADJY`), not in 640x480 units that are 1.175 screen pixels wide. Before, the map moved 3, 4 or 5 pixels per frame unevenly.
- Standing still looks exactly as before (355 screenshots identical); autoplay 0 problems at 752x560 and 640x480.

## v7.4 double-buffered screen (Miyoo / SDL 1.2 build)
- The screen is requested as `SDL_HWSURFACE | SDL_DOUBLEBUF`; if the system refuses, the plain `SDL_SWSURFACE` mode is used as before (and 640x480 if even 752x560 is refused).
- The game now draws into an ordinary memory surface (blending reads pixels back, which is slow on screen memory). Once per frame `present()` copies it to the screen in one pass, turned 180 degrees on the way (this replaces the separate in-place `rotate180` pass), then `SDL_Flip` swaps the buffers.
- If the screen is not 32-bit, the game draws on the screen directly, as in v7.3.
- Troubleshooting: `PQ_NODB=1` forces the plain screen. The first line on stderr says which mode is running ("video: ...").
- Test: `tests/sdl1_shim` builds this path on the desktop (`-DUSE_SDL1 -DROTATE180 -Itests/sdl1_shim`); the rotated frame equals the unrotated one turned 180 degrees, and both fallbacks give the same picture.

## v7.5 752x560 only
- The 640x480 fallback is gone: every build (device, desktop, web) uses 752x560. If the system gives another size, the game prints "This build needs a 752x560 screen, the system gave ..." and exits.
- `PQ_SIZE=WxH` still overrides the size for tests. The 640x480 layout units inside the code and the resampling of old 640x480 pictures are unchanged.


## v7.7 changes
v7.7  CHAT and TEASE in her room (from pokermini.zip / v7.6)
  1. HER ROOM MENU: before the day-8 scene GIFT / MATCH / CHAT / LEAVE (CHAT is there as soon as she lives with you). After the day-8 scene MATCH / CHAT / TEASE / GIVE MILK PILL / LEAVE.
  2. CHAT: A picks a random topic and a random exchange (never the same one twice in a row): your line, her answer, sometimes a second line. 10 topics (poker, weather, about her, food, the building, home life,
     about you, the future, fun, her mood) with 8-12 exchanges each. Only CHAT fills the romance bar: a good chat = +2% (CHAT_ROM). CHAT_FAIL_PCT = 25%: the chat goes badly (your line, then one of 20 snappy lines,
     no romance, note "SHE WAS NOT IN THE MOOD"). At most CHAT_DAY_MAX = 4 good chats per day count for the bar; the rest still play with the note "NO MORE ROMANCE TODAY" (the daily counter is not saved).
     A good chat that fills the bar starts the next level (note "ROMANCE LEVEL n!"). Picture while chatting: confident.png (placeholder) before the pill, her_day_6.png on days 6 and 7, confident_p.png from the day-8 scene on.
  3. TEASE (only after the day-8 scene, never gives romance): topic list HER LOOKS / HER SHIRT / HER CHEST / HER BODY / HER MILK / MILK TASTE / BACK, with the warning "SHE WILL THROW YOU OUT".
     The two milk topics are LOCKED until you have received the first Baby Milk Bottle (the first pump scene gives it). Bold exchanges (10-16 per topic, arrogant tsundere answers). EVERY tease ends the same way:
     her angry line (17 variants, e.g. "Get out of my room, right now!"), a narration line (8 variants) and you are put back on the map in front of her door (note "SOPHIA THREW YOU OUT OF HER ROOM").
     If you knock again the same day she greets you with one of 8 angry lines (R_KICKED); the next day she is normal again. Tease always uses her_cover.png.
  4. SAVE: milk_st gets a new value 4 = "first bottle received" (opens the milk topics). Save version stays 12: a save with milk_st 3 that already holds or has drunk a bottle loads as 4 (no reset).
  5. FILES: all new text is in src/talk.h (included at the end of src/lines.h): Talk {me, a, b} exchanges in the pools C_POKER ... C_MOOD (C_TOPICS), T_LOOKS ... T_TASTE (T_TOPICS), C_FAIL, R_MORE, T_KICK, T_OUT, R_KICKED.
     Add or change lines there only (uppercase font: no [ ] { } ~ ^ _ ` or |; one page = 3 lines of 46 characters).
  Tests: logic 1,731,842 checks pass (every new line checked for length and characters); self-test (--shots) 0 problems with new v77 tests (menu, 400 chats, fail rate, daily cap, level-up, pictures,
  locked/unlocked milk topics, every tease ends with the kick-out and gives no romance, angry greeting, old-save migration). Screenshots v77_*. NOT yet tested on the Miyoo.


## v7.6 changes
v7.6  Slower fades, slow unpacking walk, smoother animation, close-up after day 8, milk gauge 2 pills (from pokermini.zip / v7.5)
  1. FADES: FADE_MS 260 -> 600 (every normal transition, each way); SLOW_FADE_MS 1100 -> 1800.
  2. FIRST ROOM VISIT (unpacking): you no longer walk up. You arrive at (3,5), walk 2 steps to the RIGHT towards the bedroom door at 600 ms per step (UNPACK_STEP_MS; normal walking stays 350 ms),
     input locked, then the slow fade, then you stand on the bedroom floor tile between the computer and the bed (UNPACK_X 8 / UNPACK_Y 3) FACING THE BED, with "Alright, finished unpacking my things."
  3. ANIMATION: ease() is now a smoothstep (soft start, soft landing; durations unchanged). Dealt and board cards rise 7 px on the way (lift_of). Chips leave one after another (10/256 stagger) and each has its own arc.
     Card flips (showdown, peek, board) use the same smooth curve. fly_chips() now takes LINEAR time progress (0..256) and does the smoothing itself.
  4. CLOSE-UP (R1 at Sophia's table): only after the day-8 scene (G.pill_seen); the R1 badge also only then. While she answers ("caught you staring") her table picture is assets/her_caught.png
     (PLACEHOLDER = copy of her_cover.png; replace it freely, it is separate from her_cover.png which is used at her door/room), then the normal table picture comes back when you close her line.
  5. MILK: MILK_STEP 10 -> 50: 2 pills fill the bottle gauge (gauge values are 0 or 50). Old saves with a gauge of 10..90 load as 50 (no reset, save version still 12). Texts changed (store info, unlock note,
     SH_INFO). Debug row now "MILK: PUMP + 4 PILLS" (1 for the 2-day wait, 2 for a bottle, 1 spare).
     Economy note: a bottle now costs 2 x $500 = $1,000 (was $5,000). Lead-in + pump scene now play every 2nd pill. Both left as is for playtest.
  Tests: logic 1,730,126 checks pass; autoplay 0 problems at 752x560 and 640x480. NOT yet tested on the Miyoo: how the new animation looks at the real frame rate, fade speed feel, the slow walk.



## v7.8: fridge, store tabs, stocks, black market
- Fridge (was inventory): lists only what you own; empty = just "YOU HAVE NO ITEM IN FRIDGE". Y on the Sophia breast milk row opens its info page.
- Online store (computer): tabs ITEMS / STOCKS / BLACK MARKET, R1 next tab, L1 previous tab. ITEMS = flower, money stack, pump; BLACK MARKET = ruby ring, breast enhancement pill, milk inducer pill.
- STOCKS: NVIDIA, GOOGLE, OPENAI, APPLE. A buys 1 share, X sells 1 share at today's price. Prices (all below $1,000) change once per saved night and are fixed by a seed in the save. See logic.h (STK_*).
- Save version 13; a version-12 save is upgraded on load.
- Desktop tests: `gcc -O2 -Wall tests/test_logic.c -o test_logic -lm && ./test_logic`; flow tests: `mkdir shots; gcc -O1 -isystem tests/sdl_stub src/main.c -o pq_test -lm && PQ_SIZE=752x560 ./pq_test --shots`.

## v8.0: walk speed, Sophia in the living room, prices, bigger hints
- OPTIONS has a WALK SPEED row (SLOW 600 / NORMAL 480 / FAST 350 ms per step). `assets/sophia_walk.png` (made by `tools/make_sophia_walk.py`) is Sophia's living room sprite: morning / evening only, 20 percent per entry (`LS_CHANCE`), A on her opens her menu without the knock, over `assets/my_room.png`.
- Flower $250, money stack $1,000. Button guides and store info lines are one font size bigger. Tests: `v80_tests` in `src/main.c`, price checks in `tests/test_logic.c`.

## v7.9: external text files (lines.txt, talk.txt)
- The game reads `lines.txt` and `talk.txt` from its own folder (next to `pokerqueen.cfg`) once at start-up. They replace the built-in text of `src/lines.h` / `src/talk.h`; the built-in text is the fallback for anything missing or refused. A missing file is written from the built-in text on the first start.
- `lines.txt`: all pools (random lines, table lines with `|`), single texts, fixed-size lists and every story scene (`B_...`, one beat per line: `WHO BACKGROUND SPRITE | text`). `talk.txt`: the chat and tease exchanges (`>` you, `<` her, optional second `<`). Scenes may get more or fewer beats, except the ones whose beats the game fills in by position (the `(auto)` beats stay in place; B_GIFT, B_MILK2 and B_AFTER keep their number of beats). Menu labels, names and HUD texts stay in the code.
- Every entry is checked before use (characters the font can draw, the 3 x 46 dialogue box, 2 x 18 table lines, `%d` placeholders, scene shape). A refused entry is skipped and written to `text_log.txt`; Options shows "TEXT FILES: n PROBLEMS". Nothing in the files can crash the game. The loader skips itself in `--shots` / `--autoplay`, so the tests always see the built-in text.
- **How to add lines and beats: `packaging/TEXT_GUIDE.txt`** (copied into the app folder).
- Code: pools are `POOL(NAME, ...)` and scenes `SCENE(NAME, ...)` in `src/lines.h`; use `NAME[i]` and `NAME_N` (never `NL(NAME)`, it is a compile error for these). A new item is only editable from the files after it gets one line in `src/extreg.h`.
- Refresh the template files after changing the built-in text: `gcc tests/dump_text.c -o dump_text && ./dump_text packaging`.
- Tests: `tests/test_logic.c` (new `test_ext`: dump/load round trip, bad lines, `%d`, scene shapes, lead-ins, talk exchanges, garbage input; `PQ_ONLY_EXT=1 ./test_logic` runs only the dialogue and text-file tests); flow test `v79_tests` in `src/main.c` plays a longer scene, a changed table line, the scolding insert and a lead-in loaded from text.
- vn_buf holds 200 beats (scene up to 60 + lead-in + scolding). Save version unchanged (13).


## v8.1
- Sophia is always in the living room in the morning (evening 20 %), already there when you come in, walks on all 13 free tiles and goes back to her bedroom door after the pump / bottle scenes. Knocking while she is out: "Don't you dare enter my room."
- STOCKS: SONY and SPACEX with logos (assets/stk_*.png, tools/make_stock_logos.py); the list scrolls (4 rows). Save version 14 (v13 and v12 saves are upgraded).

## v8.2
- Store ITEMS: body lotion $300 (+12%), pink card deck $600 (+15%), yoga outfit $1,500 (+25%), gold bar $6,000 (+50%) are gifts (one a day); the diamond ring $50,000 is one time only and has no use yet (for a wedding later). The ITEMS list scrolls.
- BLACK MARKET: Super milk drug $1,500 (after the day-8 scene). Given to her it keeps the gauge full 2 days and opens ASK FOR MILK (a bottle, twice a day); she refuses the normal milk pill for those 2 days and 2 more.
- Save version 15 (v14, v13, v12 saves are upgraded). Art: tools/make_gift_assets.py.

## v8.4: dialogue box on the bottom edge, flexible height, vertical romance bar
- Every dialogue-style box ends on the bottom screen edge; the dialogue boxes are full width (constants DLG_BOT / DLG_L / DLG_R in `src/main.c`).
- The dialogue box height follows the rows of the sentence: 1 row = 64, 2 rows = 86, 3 rows = 108 (`dlg_h()`); the "A >" prompt follows the box (`dlg_py`).
- The romance bar is vertical: track above, heart below with the level number inside, no percentage (`draw_romance_bar`, `assets/romance_heart.png`, `tools/make_romance_heart.py`).
- Tests: `v84_tests` in `src/main.c` (screenshots `v84_*`).

## v8.3: her menu pages, shy Sophia
Her menu has three pages (R1 next, L1 previous, B leaves; no LEAVE row): 1 PLAY A MATCH / CHAT, 2 what you can give (only what you own, no counts; YOU HAVE NO GIFT when empty; the milk pill and Super drug after the day-8 scene), 3 TEASE / ASK FOR MILK (only after the day-8 scene; ASK only while the Super drug works).
From day 6 of the pill on (`ls_shy()` in main.c) she never comes into the living room: she is embarrassed about her new body. Tests: `v83` block in `src/main.c`.

v8.5  START = MENU, NO "MENU LEAVES" HINT, LOWER POT CHIPS / STACK TEXT, SMOOTHER EXHIBITION (from pokermini.zip / v8.4)
  1. "MENU LEAVES THE TABLE" is gone from every match button guide (Sophia's table, Quick Play, Exhibition). The two boxes are shorter (Sophia 104 -> 82, Quick Play / Exhibition 104 -> 84 in the 640x480 grid).
     The pause menu itself still warns (LEAVING COUNTS AS A LOSS / YOU LOSE YOUR BUY-IN).
  2. START (RETURN) now opens the pause menu everywhere; SELECT (RCTRL) does nothing. The Menu key (ESC) still opens it too (kept on purpose, say if it should do nothing).
     Start no longer skips scenes by itself: in a story scene it opens the pause menu whose second entry is SKIP SCENE; the opening monologue now uses the same menu (RESUME / OPTIONS / SKIP SCENE).
     The corner hints "START SKIP" became "START MENU".
  3. LOWER BOTTOM ITEMS (the POT box is NOT moved): pot chip stacks 444 -> 456 (POT_Y), "HER $" / "CPU $" text 446 -> 460 (STK_TY), chip flights land / leave at 448 (POT_CY, was 436). Sophia's hole cards were not moved (they would sit on the board cards).
  4. SMOOTHER EXHIBITION / QUICK PLAY: the table backdrop (background + boss + empty table = up to three full-screen layers, two alpha-blended) is composed once into an opaque surface (feltC) and copied with one plain blit per frame.
     Picture is pixel-identical (checked by the tests). The CPU thinking time is unchanged (Exhibition 2-4 s, Quick Play 1-2 s).
  5. Tests: new v85_tests (key map, Start at the table / in the monologue, cached backdrop identical for quick / exhibition / boss). Flow tests: problems 0. Logic tests: 0 failed. No save change (version stays 15).
  NOT yet tested on the Miyoo: whether Start / Select behave as expected on the device, and how much smoother the Exhibition is. If it is still choppy the next step is caching Sophia's picture layer too or skipping unchanged frames.
  Changed files: src/main.c; README + this file.


## v8.6: wedding and page 4 of her menu
After the day-8 scene, GIVE DIAMOND RING (page 2 of her menu, the $50,000 online-store ring) plays the wedding scene (`B_WEDDING`) and sets `married`. Page 4 then holds HUG, KISS, SLEEP TOGETHER (night only) and BREASTFEED.
- Pictures: `assets/sc_<hug|kiss|sleep|feed>_<d8|yoga>.png`, 752x560, loaded one at a time by `sc_pic()` (placeholders made by `tools/make_scene_placeholders.py`; replace them with the real art, same names).
- Outfits: giving the yoga outfit sets `yoga_given`; page 1 then has CHANGE OUTFIT (`outfit` 0 = after day 8, 1 = yoga). The outfit only changes the page-4 scene pictures.
- Gauge: when married a full gauge stays at 100. PUMP MILK (page 3, a bottle), ASK FOR MILK and BREASTFEED each empty it (under the Super drug breastfeeding uses one of the daily asks). A pill on a full gauge is refused.
- Sleep together: married, a night knock opens her menu on page 4. It works like the bed (rent, next morning, save). She refuses after a tease or on a day she gave milk (`mday`).
- Save version 16 (new: married, outfit, yoga_given, mday); version 15 saves are upgraded. Logic in `logic.h` (g_marry, g_pump_use, g_feed ...), tests `test_v86` and `v86_tests`.

## v8.8 pictures: home table faces, no cover when married

- Table faces: the HOME set is used from move-in (`G.rs == RS_LIVING`) until the day-8 scene, and in the yoga outfit (`home_faces()`). After the day-8 scene (usual outfit) the table goes back to the day-8 faces `her_neutral_p / her_shock_p / her_smug_p` and `her_caught.png`. Files: `assets/her_<neutral|shock|smug>_<home|yoga>.png` and the close-up answer `assets/her_caught_<home|yoga>.png` (home = day-8 / married outfit, yoga = yoga outfit; a missing yoga file falls back to the home one). They are separate files from the covering pictures of her room. `bg()`, `face_home_pic()`, `caught_pic()` in `src/main.c`.
- Married (`her_vis()`): door, room and chat show the uncovered day-8 picture (`confident_p.png`) or `her_home_yoga.png` in the yoga outfit. She covers her chest (`her_cover.png` / new `her_cover_yoga.png`) only while `cover_now` is set: teasing about anything except HER LOOKS, ASK FOR MILK, BREASTFEED. It is cleared when her answer / the scene / the tease is over. Not married: unchanged (her_cover from day 8).
- BREASTFEED (`B_FEED`) now opens with two beats in her room (sprite 1 = she covers), then the page-4 picture. Registry count 6 -> 8.
- Placeholders: `python3 tools/make_home_placeholders.py` (replace the files in assets/ with the real art, same names). Tests: `v88` checks inside `v86_tests`, screenshots `v88_*`. No save change (version 16).

## v8.9 bet menu (Exhibition + boss), no chip on the title menu
- Title menu: the big POKER MINI chip (bottom left) is gone (`big_chip` deleted). Nothing else moved.
- EXHIBITION and the BOSS table now have a free bet size (`MP.nl = 1`, set in `mp_begin`). Quick Play and Sophia's table stay fixed limit.
- X on your turn opens the bet menu (right column, replaces the button guide): UP/DOWN small step (half a big blind), LEFT/RIGHT big step, X = pot-size bet, Y = all-in, A confirms, B cancels. The amount is always between the minimum raise and all-in; the ALL IN row lights up at the maximum.
- Rules (`src/multi.h`): minimum raise = the old fixed size, or the last raise if bigger (`mp_min_by`); a bet is capped at what the other live seat can match (`mp_cap_pay`; chips nobody can call stay in the stack); after the 4th bet of a round only all-in is possible; all-in is allowed from the first action. `mp_act_by(m, a, by)` takes the raise increment; `mp_act(m, a)` = minimum.
- CPU / boss: `mp_decide_by` picks half pot / 3/4 pot / pot, now and then all-in, and shoves when a size would commit about 60 percent of its stack; big bets need more equity to call (`mp_decide`, `mp_decide_boss`). Playtest the balance.
- Tests: `test_nl` in `tests/test_logic.c` (rules, caps, 600 random free-size games, chips conserved); `v89_tests` in `src/main.c` (menu through the real buttons, Quick Play unchanged, title menu without chip). Screenshots `v89_*`. The full logic suite now takes about 4 minutes on the sandbox.

## v9.0 icons, Event Center menu, lottery
- The feature icons (fridge, ATM, store, Event Center) stay when you face them.
- Event Center: clerk greeting -> menu EXHIBITION / LOTTERY TICKET (B leaves). Lottery: 6 tickets $20..$1500, prize 3x, 30 percent win chance, one ticket at a time, result from the next day at the clerk (logic in `logic.h`: `g_lot_buy`, `g_lot_claim`).
- Save version 17 (lot_price, lot_day, lot_win); version 16 saves are upgraded. Tests: `test_lottery` in `tests/test_logic.c`, `v90_tests` in `src/main.c`.

## v9.1 lobby: vending machine, wall ATM, desk for rent and upgrade only
- New lobby picture (assets/map_lobby.png, same 408x312 / 17x13 tiles). Tiles: `M` = the red vending machine at (5,3) = the store (opens straight away, day and night); `A` = the wall ATM, two tiles (10,3) and (11,3); the old ATM tile on the desk corner (9,5) is plain desk now.
- You must stand below and face UP to use them: (5,4) for the machine, (10,4) or (11,4) for the ATM. Facing sideways or down does nothing.
- Receptionist (desk tiles (7,5), (8,5)): only rent and the room upgrade. Menu = PAY RENT / UPGRADE ROOM / LEAVE (no STORE). On a plain day she greets you and nothing opens. Greeting lines changed (W_CLERK). At night the desk is still empty.
- Icons: the store badge floats over the vending machine, one ATM badge floats over the middle of the wall ATM; the badge above the receptionist is gone.
- The intro (ST_NEW) only reacts to the desk; the ATM and the machine show the check-in line.
- Tests: `v91` checks inside the existing flow tests (vending machine day / evening / night, facing, both ATM tiles, old ATM spot, desk menu of 2 entries, one badge each). Autoplay 0 problems at 752x560 and 640x480; new shots `58_v91_lobby_vending_prompt`.
- Still before release: DEBUG_MODE 0 and TEST_BANK 0 in src/logic.h.

## v9.2 Sophia's walkable bedroom (married only)
- New map `MAP_SBED` (8 x 7 tiles, `assets/map_sbed.png`, made by `tools/make_sbed_map.py` from the artist's `tools/sbed_src.png`; grid `MG_SBED` in `src/main.c`). Tiles: `W` wardrobe (3 tiles on the left wall), `B` her bed, `D` the door out (bottom left). The windows, nightstands and rug edges are only picture.
- Getting in: only after the wedding (`G.married`). You knock on her bedroom door in your living room; the knock scene (`SCR_KNOCKIN` in the morning / evening, the hiding scene `SCR_NIGHTM` at night) now ends with you walking into her room (`sb_enter()`), at the door tile (1,5) facing right.
  - Morning: she is in her room (unless she stands in your living room: then the old line "Don't you dare enter my room" and the door stays shut). Evening: `SB_EVE_CHANCE` (50) percent, rolled once per evening (`sb_here()`); if she is out you get the no-answer line. Night: always. Her story scenes (milk, pump) still play at the door first. Thrown out today (`kicked_day`) or not married: the old flow.
- She stands beside her bed (tiles (3,4) / (6,4)) and now and then walks to the wardrobe tile (2,4) or the window tile (2,3) (`sb_wander()`, about 60 percent beside the bed), never on you and never in the door column. Same sprite sheet as the living room (`assets/sophia_walk.png`).
- Night: the map and both sprites use the darker (her room) tint: `sbN` (made once at load) and `lsSn`.
- A on her = her menu with her room picture behind it (`TALK_BG()`), B puts you back where you stood. A on the wardrobe = a short line, then a small menu (CHANGE TO THE YOGA OUTFIT / CHANGE TO HER USUAL OUTFIT, LEAVE; wk_mode 8); needs the yoga outfit. A on the bed: a line by day, her menu on page 4 at night (sleep together; you wake up in front of her door).
- Leaving: A on the door facing down = out to the living room, in front of her door; she stays in her room for the rest of that time of day (`ls_away`). A match that started in her bedroom, leaving the table, being thrown out after a tease and sleeping together all end OUTSIDE her bedroom (`sb_out`).
- Tests: `v92_tests` in `src/main.c` (screenshots `v92_*`); the door test accepts the bedroom door arriving in the upgraded room. No save change.


## v9.3 room in three levels
v9.3  ROOM IN THREE LEVELS: BASE ROOM, UPGRADE 1 (COMPUTER), UPGRADE 2 (HER DOOR); RENT 250 / 500 / 1000 (from pokermini.zip / v9.2)
  1. THREE ROOMS (art from your base_room / room_upgrade / room_upgrade_2 exports; tools/make_room_maps.py crops the 1-tile margin and the grey editor shadow, sources in tools/room_*_src.png):
       level 0 BASE ROOM  assets/map_room.png     7 x 7 tiles  (grid MG_ROOM0): kitchen, an OLD DESK WITH A LAMP (no computer, nothing to use), sofa (1,4)+(1,5), bed (5,3)+(5,4), door (3,6), bathroom door (6,5).
       level 1 UPGRADE 1  assets/map_room_u1.png  11 x 7 tiles (grid MG_ROOM, the old grid): living room + bedroom, the COMPUTER at (7,3), the lamp stands where her door will be. NO door, no locked-door message, nothing to use there.
       level 2 UPGRADE 2  assets/map_room_up.png  11 x 7 tiles (grid MG_ROOM_UP): the lamp is gone, her bedroom door at (5,2) (works only once she lives with you; before that the old "spare room" line).
     room_apply() swaps grid, size and picture (one picture in memory); the camera already centres a map narrower than the screen. g_room_lvl(): 0..2, level 2 is also used once she lives with you (RS_LIVING).
  2. COMPUTER: the online store (items, stocks, black market, ring, pill) exists only from level 1 on (g_computer). In the base room the desk tile gives no A bubble and does nothing (the debug panel opens from it in a DEBUG_MODE build, see 7).
     The lobby vending machine (soda / energy drink) is a different thing and is unchanged.
  3. UPGRADES (reception desk, cash only; menu PAY RENT / UPGRADE ROOM <price> / LEAVE):
       upgrade 1  $5,000   on offer from the shirt scene on (story >= ST_HOME), no other condition. Brings the computer; the story is not touched.
       upgrade 2  $10,000  on offer once level 1 is bought AND she was asked to live with you (YES = RS_UPG, or NOT NOW = RS_NO). Sets RS_READY.
     She now moves in ONLY into the level-2 room (g_move_ok). ASK TO MOVE IN with a smaller room = her S_UPFIRST refusal (also after NOT NOW, so the old "move in without the upgrade" shortcut is gone).
     The desk greets with a new pool W_CLERK_UP1 while upgrade 1 is on offer; W_UPG_OK1 is the "done" line of upgrade 1; objective O_UP1 ("UPGRADE YOUR ROOM AT THE FRONT DESK ($5000)") is shown when nothing else is current.
  4. RENT: 250 (base) / 500 (upgrade 1) / 1000 (upgrade 2), RENT_AMT / RENT_UP1_AMT / RENT_UP_AMT. The fixed "$100" / "$300" are gone from the clerk and reminder lines (the desk menu shows the amount); O_UPG now says $10000.
     NOTE (economy): with START_CASH 100 and the 300 shirt debt, the first rent (day 7) is now 250: tight on purpose, needs a playtest. Sophia's room-sized money (5,000 / 10,000 / 12,000 ring) is unchanged.
  5. FIRST VISIT (unpacking): you walk 2 slow steps to the right along the bottom row, the slow fade, then you stand beside the bed at (4,4) facing it (UNPACK_X / UNPACK_Y), line "Alright, finished unpacking my things."
  6. SAVE VERSION 18. Same struct; room_up is now a LEVEL 0..2. Older saves are migrated when loaded: old room_up 1 (or she lives with you) -> level 2; a game past the shirt scene without it -> level 1 (it already had the computer); before the shirt scene -> level 0.
  7. DEBUG (DEBUG_MODE 1 only): new row ROOM UPGRADE (FREE) in the debug panel (+1 level, no money). In the base room the old desk (stand at (4,3), face up) opens the debug panel; B returns to the room (to the store when a computer exists). The status lines moved down 32 px.
  8. TEXT: 3 new items in extreg.h (registry 203); packaging/lines.txt and talk.txt regenerated.
  9. Tests: logic 3,968,661 checks, 0 failed (new test_v93: levels, rents, costs, move-in rule, v17 migration for all four cases, bad levels rejected; test_romance / test_rent rewritten for the new money). Flow tests (--shots) updated for the new rooms: all sections problems 0 (the v8.6 night-knock check was already stale since v9.2 and fails on the old zip too; it now expects the walk into her bedroom). New v93 screenshot of upgrade 1.
  NOT tested on the Miyoo. OPEN: the base room is only 5 x 3 free tiles, the economy of rent 250 in week 1, the upgrade-1 pitch text. Before release: DEBUG_MODE 0 and TEST_BANK 0 in src/logic.h.

## v9.6 money box at the multiplayer table
1. MULTIPLAYER TABLE (Quick Play / Exhibition / boss): the chip rack under your cards (four chip pictures and their X counts) is gone. Same change as v9.5 at Sophia's table.
2. The cards box is shorter (148 x 112, was 148 x 130). It holds your two cards and one text row: your hand name at showdown / "FOLDED" at the left, the gold Y badge at the right.
3. New money box right under it (y 121, 28 high), as wide as the text and flush right with the cards box. It holds only "YOU $x". Your dealer button sits inside it, right of the amount, when you have the button (the box grows by 36 px then).
   The floating dealer button left of the cards box (dealer_top) is removed. The CPU's / her button (bottom right, next to "CPU $" / "HER $") is unchanged.
4. The button guide (A / B / X lines) and the bet menu (RAISE / ALL IN) moved up 49 px to close the gap (guide at y 157, was 206).
5. Chips fly to and from the new money box (you_fy = 135). The pot-to-you flight had the old y (150) hard-coded; it now uses you_fy.
Tests: --shots self-test 0 problems at 752x560 (Quick Play, Exhibition, boss, bet menu screenshots checked by eye). NOT tested on the Miyoo.

## v9.5 items.txt, the name keyboard and mama mode
- `items.txt`: names, store info, info pages, give-page names and "bought" notes of all items; no prices (game rules stay in the code). See TEXT_GUIDE.txt 5b.
- Married: R2 (Miyoo: BACKSPACE, desktop: R) in her menu opens an on-screen keyboard. MAMA / MAMA SOPHIA (names.txt) = mama mode (Game.call = 1, saved, save version 19): her own dialogue set
  (`*_MM` sections of lines.txt / talk.txt) and the BREASTFEED row on page 4 exist only there; SOPHIA goes back. Married in SOPHIA mode she calls you husband, never boy. TEXT_GUIDE.txt 5c.

## v10.4 mama-mode openers
- In mama mode the first two beats of BREASTFEED (door, bedroom, living room), ASK FOR MILK / PUMP MILK, the milk pill and the Super drug (living room) are a random opener pair: your line with "Mama" first, then her answer, which starts with her reaction to the name (hesitant and unsure for BREASTFEED).
- Five new editable pools (B_OPEN_FEED_MM, B_OPEN_FEED_MM_LR, B_OPEN_ASK_MM_LR, B_OPEN_PILL_MM_LR, B_OPEN_SUPER_MM_LR): pairs of beats, ME then SOPHIA. Registry 301 -> 306. TEXT_GUIDE.txt 5d. Husband mode is unchanged (living room keeps the Here?! beat).
- Code: open_pair / open_replace / open_prepend in src/main.c; the bottle scene knows if a milk pill started it (bottle_pill). Script: tools/add_v104.py (one-off, already applied). Tests: v104_tests in src/main.c; registry 306 in tests/test_logic.c.

## v10.5 sofa REST / SAVE ONLY, five save slots, rewind guard
- Sofa: A opens SAVE THE GAME? with REST (save, time passes: morning and evening only, as before), SAVE ONLY (save, the time stays: no fade, no rent, Sophia is not re-rolled) and CANCEL. At night REST is greyed out and skipped; SAVE ONLY still works. The bed is unchanged. New line W_SAVED_ONLY (registry 306 -> 307, editable in lines.txt).
- Five save slots: slot 1 is the old `pokerqueen.sav` (existing saves keep working), the others `pokerqueen2.sav` ... `pokerqueen5.sav`. The sofa and the bed save to the ACTIVE slot (kept as `slot N` in pokerqueen.cfg). CAREER MODE (title) lists the slots: A loads a used slot / starts the story in an empty one, X starts over in a used slot (asks first), Y deletes (asks first). Options > SAVE SLOTS (title and while walking, not at a table or in a scene): L/R picks the active slot, A opens the list (A loads after a YES/NO that warns about unsaved progress, X = save here). Saving itself is still only at the sofa and the bed.
- GAME OVER no longer deletes a save: it goes back to the title; load a slot or start over.
- Rewind guard: every career has a random id (Game.career, SAVE_VER 22; a version-21 save is upgraded in place). `pokerqueen.prog` keeps the furthest day reached per career. Loading a slot that is BEHIND that day (a) reshuffles the share prices of every day after the loaded one (Game.mrw_*: the days up to it keep their prices and the chart) and (b) shuts the lottery counter until you reach the furthest day again ("NO TICKETS UNTIL DAY n"). Poker re-rolls (Exit, Continue) were always possible and are left alone. Deleting pokerqueen.prog switches the guard off.
- Code: src/logic.h (stk_seed, g_rewind, g_behind, prog_get/prog_set, g_open_slot), src/main.c (slot helpers, GO_LOAD, sofa menu, draw_career / career keys, Options rows, lottery lock, v105_tests).

## v10.8 designer top (a third outfit)
- New store item DESIGNER TOP ($2,500, ITEMS tab, between the yoga outfit and the gold bar). A gift for +30% romance (before the day-8 scene); once given, Sophia can wear it after the wedding.
- `Game.outfit` is 0 usual / 1 yoga / 2 designer top. Her menu and the bedroom wardrobe list one row for every outfit she can change into. Mama mode has its own lines.
- Pictures: 14 placeholder `*_top.png` files (`tools/make_top_placeholders.py`); replace them with the real art under the same names. A missing `*_top` file falls back to the yoga picture.
- Save version 23 (version-22 saves upgrade in place). New text keys: `[DESIGNER_TOP]` in items.txt and 6 pools in lines.txt.

## v11.0 friendship bar and her room by day
After you beat her at the boss table, a blue FRIENDSHIP bar fills with wins over her at her hotel tables (20 wins; it starts at zero, the boss win does not count). Tiers 1-5 change her door greetings and her talk. On her door menu, COME IN (by day, until she lives with you) lets you into her room: A on her gives TALK / PLAY A MATCH / LEAVE; the TV, windows, table and bench answer when you face them. All new lines are in packaging/lines.txt (G_FIRSTVIS, G_FR1-5, V_IN, V_HELLO, V_BYE, V_BED, V_BATH, V_TV, V_WIN, V_TABLE, V_SOFA, V_TALK1-5). Blue heart: python3 tools/make_friend_heart.py.

## v11.1 cow print outfit (a fourth outfit)
- New store item COW PRINT OUTFIT ($3,000, ITEMS tab, between the designer top and the gold bar). A gift for +35% romance (before the day-8 scene); once given, Sophia can wear it after the wedding. Info page: a thin, comfy outfit that contours the wearer's body shape, with a chest cutout.
- `Game.outfit` is now 0 usual / 1 yoga / 2 designer top / 3 cow print. With every outfit owned her menu shows PLAY, CHAT and three change rows (5 rows); the bedroom wardrobe shows three rows + LEAVE. Cycle order: usual, yoga, top, cow. Mama mode has its own cow line (`R_OUTFIT_COW_MM`).
- Pictures: 14 placeholder `*_cow.png` files (`tools/make_cow_placeholders.py`, built from the yoga pictures); replace them with the real art under the same names (`her_home_cow`, `her_cover_cow`, `her_neutral_cow`, `her_shock_cow`, `her_smug_cow`, `her_caught_cow`, `her_hide_cow`, `her_hide_no_cow`, `her_hide_yes_cow`, `sc_hug_cow`, `sc_kiss_cow`, `sc_sleep_cow`, `sc_feed_cow`, `sc_feed_lr_cow`). A missing `*_cow` file falls back to the designer-top picture, then the yoga one. Store icon: `assets/gift_cow.png` from `tools/gift_src/cow_outfit.png` (`tools/make_gift_assets.py`).
- Save version 25 (version-24 saves upgrade in place). New text keys: `[COW_PRINT_OUTFIT]` in items.txt and 6 pools in lines.txt (`GH_COW`, `GO_COW`, `GL_COW`, `GE_COW`, `R_OUTFIT_COW`, `R_OUTFIT_COW_MM`).

## v11.2 angry outfit gifts and the wear exchange
- Gift scenes (yoga outfit, designer top, cow print outfit): Sophia now reacts angrily (pools `GO_` / `GL_` / `GE_` rewritten, 5 versions each). Yoga and top: the cloth is too thin and shows far too much skin (the top is more about her chest). Cow: she asks whether you are calling her a cow. She still keeps the gift and the romance gain is unchanged.
- Milk-aware cow gift: once she has taken the beauty pill (`pill_given`) the cow gift uses the new pools `GO_COW_MILK` / `GL_COW_MILK` / `GE_COW_MILK` (her chest is swollen, she suspects the pill, cows give milk). Before the pill the plain cow pools play. Outfit gifts only exist before the day-8 scene, so she is not lactating yet at gift time.
- CHANGE OUTFIT into the yoga outfit, the designer top or the cow print outfit (her menu and the bedroom wardrobe) now plays a 3-beat exchange: your request, her refusal (stronger than at the gift), her grudging give-in. The outfit changes when the exchange ends, then her last word shows (`R_OUTFIT_YOGA` / `_TOP` / `_COW` and their `_MM` versions were rewritten as the "it's on" lines). Yoga and top: thin cloth, too much skin. Cow: she accuses you of seeing her as a cow because she makes milk and can lactate (married, so she does). Mama mode has its own exchanges (you say Mama, she rejects the name first). Going back to her usual outfit has no exchange.
- Six new scene pools `B_WEAR_YOGA`, `B_WEAR_TOP`, `B_WEAR_COW` and `_MM` versions in lines.txt: GROUPS OF 3 beats (ME, SOPHIA, SOPHIA), 5 groups each, one picked at random (never the same twice in a row). An emptied pool falls back to the old instant change.
- Registry 352 -> 361. No save change (SAVE_VER 25). A lines.txt already on the SD card overrides the new text: delete it so the game rewrites it.

## v11.3 outfits only at max romance, change only in her wardrobe
- Her menu has no CHANGE OUTFIT rows any more: the wardrobe in her bedroom (married) is the only place to change clothes (same 3-beat wear exchange).
- The yoga outfit, designer top and cow print outfit are sold, given and listed only once she lives with you and the romance bar is full at level 5. They give no romance, do not use the gift of the day and are not blocked by a full bar; the closing note says ADDED TO HER WARDROBE. They stay on the give page after the day-8 scene; a row disappears once she has that outfit.
- An outfit she already owns in an older game keeps working. No save change (SAVE_VER 25). Store info text: delete an old items.txt on the SD card so the game rewrites it.


## v11.7 Screen filter removed
The v11.4 SCREEN FILTER (CRT scanlines, sharpen, smooth), its Options entry, its cfg lines (crt / shp / smo, old ones are ignored) and assets/scanlines.png are gone. Options > TEXT BOX STYLE is a single entry again.
The debug panel (X in the online store, test build) no longer shows the small status and info lines under the list; only the list and the pop-up after a selection remain.

- v11.5: OPTIONS > TEXT BOX STYLE has TABLE OPACITY and TABLE LINE (pokerqueen.cfg: tbo, tbl); a folded CPU no longer shows a dark box on the table (its cards are mucked).
- v12.0: OPTIONS > MENU COLORS has GUI LINE and GUI TEXT (cfg: gul, gut): the border and accent text of every other box (cash, hand, day, result, icons, A bubble, save box, menus, setup screens, dialogue A prompts). TABLE LINE is removed from TEXT BOX STYLE (GUI LINE replaces it; an old tbl line is ignored). Sophia's top-left table box now follows TABLE OPACITY.
- v12.1: COW MODE. (1) Giving her the cow print outfit and asking her to change into it now makes her disgusted (ugh, ew, degrading), not only angry; she still keeps it and still gives in (gift pools GO/GL/GE_COW and the milk-aware ones, wear exchange B_WEAR_COW / _MM, 6 groups each, R_OUTFIT_COW / _MM). (2) While she wears the cow outfit (married) she is in cow mode: no save flag, g_cow() = outfit is the cow print, it ends when she changes out (one relieved line, R_OUTFIT_UNCOW / _MM). Her greeting (day and night), chat (own exchanges C_COW / C_COW_MM, own 'not in the mood' lines), the line after a chat, 'not full yet', hug, kiss, sleep, the night knock, BREASTFEED (4 own scenes), the pump scene, the milk pill and Super drug scenes all turn negative, but she ALWAYS accepts (no refusal roll). It layers with Mama mode (_CW_MM texts). New texts: 40 registry entries (401 in total) in lines.txt / talk.txt. Debug panel: COW MODE ON (OUTFIT). SAVE_VER stays 26.

## v11.6 five romance levels, pink cards only after the pink deck
- Romance has 5 levels (`ROM_LVLS` in logic.h). Everything that waited for a full level 3 (beauty pill, the milk pill / pump / Super drug chain, the three outfits, BREASTFEED) now waits for a full level 5. The heart shows 1..5; the full bar is still hidden.
- Each level is harder: gifts, chats and home wins add only 100 / 90 / 75 / 60 / 50 % of their value (`ROM_SCALE`, at least 1 point). Level-up note everywhere: ROMANCE LEVEL UP!
- Sophia's table has blue card backs (both of you) until she lives with you AND you gave her the PINK CARD DECK after she moved in (`deck_given`, `g_pink_cards`). A deck given before she moves in is a normal gift and does not count. The first deck after the move-in is always accepted (also on a day with a gift or at a full bar: then no romance, not the gift of the day) and stays on her give page until given. Other tables stay blue.
- SAVE_VER 26 (+ deck_given). Older saves: deck_given = 1 if she already lives with you; a save past the pill / wedding becomes level 5 full; a full old level 3 becomes level 4 at 0.

## v12.2 new prices, the breast pump wears out
- Prices (`SH_PRICE`): cow print outfit 4000 (the dearest outfit), designer top 3500, yoga outfit 3000, breast enhancement pill 15000, milk inducer pill 750, Super milk drug 2250. Pump stays 500.
- The breast pump wears out: each pump makes `PUMP_MIN`..`PUMP_MAX` bottles (9..15, never fewer than 9; one use = one bottle from PUMP MILK, ASK FOR MILK or a milk pill that fills the gauge before the wedding; BREASTFEED does not use it). The ask after the last good use makes no bottle: she says it is broken (`R_PUMP_BROKEN*`, normal / Mama / cow / cow + Mama), nothing is used up. Only then the store lists a new pump (`pump_dead`); you hand it over from her give page (`R_PUMP_NEW*`, no scene, new random life). Debug panel row BREAK THE PUMP.
- SAVE_VER 27 (+ `pump_life`, `pump_used`, `pump_dead`); a version-26 save loads and gets a fresh pump life on its next use. 8 new texts (409 in total).

## v12.3 the Mommy Milk Injection
- New black market item MOMMY MILK INJECTION (`SH_INJECT`, `GF_INJECT`): $30,000, one time (`inj` = bought, `injected` = given), listed after the day-8 scene. Given from her give page once the pump scene is done (`milk_st >= 3`); refused while a Super drug works (R_INJ_WAIT). It is not the gift of the day.
- After it: the bottle gauge is gone for good (`g_milk_gauge()` is false, `milk_pct` is 0); the milk pill and the Super drug are refused (R_INJ_NOPILL, code 16) and no longer sold. ONE milk a day in total (`inj_day`): ASK FOR MILK (2 bottles, one pump use), PUMP MILK (1 bottle, one pump use) or BREASTFEED (mama mode, no pump); the other two are refused that day with R_INJ_DONE / _MM / _CW / _CW_MM. The pump still wears out (v12.2). An ask that does not fit in the bag (cap 99) is refused and does not use the day.
- New background kind BOTTLE: the hand-over after ASK FOR MILK shows `assets/sc_bottle_d8.png`, `_yoga`, `_top`, `_cow` (752x560, PLACEHOLDERS: replace them by file name). In scene files the background word is BOTTLE.
- Icon: `assets/gift_inject.png`. Debug panel: new row MOMMY MILK INJECTION: GIVE.
- SAVE_VER 28 (+ `inj`, `injected`, `inj_day`); a version-27 save loads unchanged. 10 new texts (419 in total).


## v12.4 energy costs, drink prices, vending machine store
- Exhibition: EX_ENERGY 15 (was 10). Matches at Sophia's 3rd-floor table (door or inside her room): SOPH_ENERGY 20 on top of the cash buy-in (g_soph_start; charged at the start, tie / walk-out never refund it, the Baby Milk Bottle waives it). Buy-in list shows "A MATCH USES 20% ENERGY" and refuses with "NEED 20% ENERGY"; a tie rematch needs the energy too. Free match at home stays 10%.
- Drinks: soda $5 (was $1), energy drink $10 (was $3); restore amounts unchanged (20% / 50%).
- Store = the lobby VENDING MACHINE (v12.5): a STATIC picture, assets/store_menu.png (752x560) built by tools/make_vending.py from tools/vend_src/vend2.png (your own picture: drinks and the $5 / $10 price boxes are painted on it; the script only adds the X and Y buttons left of the price boxes). Top shelf = soda (X), bottom shelf = energy drink (Y). UP/DOWN/LEFT/RIGHT pick the shelf, A buys it, X = soda, Y = energy drink, B back. Info + cash box in the empty right half of the top shelf. No smiley, no falling can.

## v12.6 8 ball at the Event Center
- Event Center menu (after the clerk's greeting): EXHIBITION / **8 BALL** / LOTTERY TICKET (B leaves). 8 BALL opens the same style of buy-in list: $10 / $50 / $100 / $500 / $1000, winner is paid 1.8x the buy-in (v13.1: 2x; v13.9: now 2.5x; `POOL_PRIZE_PCT`, `g_pool_prize` in logic.h). Energy is `EX_ENERGY` (the Milk Bottle waives it), the buy-in and energy leave at once; START -> leaving loses the buy-in (same pause menu as the Exhibition). Nothing new is saved (SAVE_VER unchanged). After the game you stand at the match-room door, like after an Exhibition.
- Engine: `src/pool.h` (no SDL). Native 752x560 picture units, `assets/pool_table.png` (your picture, copied 1:1; cushion inner edges x 44..706.5, y 46.5..373.5). Fixed step 1/480 s (8 per frame, real time), equal-mass ball collisions with the exact contact moment, rolling friction (replaced in v12.7 by the real cloth model below), circular pocket capture (corner / side radii and centres in `PL_PX/PL_PY/PL_PR`). **All feel numbers are in the TUNING block at the top of pool.h** (`PL_DRAG`, `PL_FRIC`, `PL_E_BALL`, `PL_E_RAIL`, `PL_VMIN/VMAX`, ball radius `PL_R`).
- Rules: heads-up standard 8 ball, you break, open table until the first legal pot after the break, no called pockets. Fouls (scratch, no ball hit, wrong group first, no cushion after contact) = ball in hand for the other side. The 8 wins only as a separate legal shot after your group; the 8 early / scratch on it / foul on it loses; the 8 on the break is spotted again.
- Controls: LEFT/RIGHT turn the aim (hold = faster), L1/R1 fine turn, UP/DOWN tiny steps, **hold A to charge the power, let go to shoot**, B while charging cancels. Ball in hand: D-pad moves the cue ball, A places it (green ring = free spot). Guide: dotted line to the first contact, gold = where the hit ball goes, blue = where the cue ball swerves.
- CPU (`pl_ai_shot`, `pl_ai_place`): lists every ball-to-pocket shot, checks the paths, tries four powers on a copy of the real physics, picks the best, then adds aim and power error by table (`PL_AIM_ERR` 2.6 / 1.7 / 1.05 / 0.6 / 0.32 degrees, `PL_POW_ERR`). Thinks 1.4-3 s. Equal CPUs: the breaker wins about 40 percent (a break without a pot hands the other side the first shot).
- Screen: balls are sprites built in code at the real screen size (stripes, numbers, shading, shadow); HUD in the black band under the table (YOU / CPU with solids-stripes tray, status line, power bar).
- Tests: `test_v126` in tests/test_logic.c (prize, rack, physics invariants, every rule, 48 CPU games) and `v126_tests` in main.c (menu, list, start, real-button aiming, charge / cancel, a whole scripted game, both result screens, pause).

## v12.7 8 ball: spin, a real cloth, rolling balls, smoother
- **Spin.** Hold **Y** and the D-pad moves the cue tip over a small picture of the cue ball (middle box, under the power bar; the red ring marks the limit, 0.6 of the ball radius). **UP = follow** (top spin: the cue ball runs on after the hit), **DOWN = draw** (back spin: it comes back), **LEFT / RIGHT = side english**. X puts the tip back in the centre. While Y is held LEFT/RIGHT/UP/DOWN do not turn the aim. After every shot the tip returns to the centre. The cue stick comes in from the side you hit. The guide line is unchanged (it shows the aim, not the spin).
- **Engine (src/pool.h).** Every ball now has a spin (`wx wy wz`) and a picture orientation (`q`). The cloth works as real cloth: a skidding ball (a fresh cue ball, draw, follow) is dragged towards a clean roll by sliding friction (`PL_MU_S`, 2/7 of the speed difference per the physics, ends exactly at v = R w), then only a small rolling resistance (`PL_FRIC`, `PL_DRAG`) is left; side spin dies slowly (`PL_ZSPIN`). The old drag-proportional-to-speed stopping is gone, so fast shots keep their speed longer and settle in about 4-5 s. Ball against ball has friction (`PL_MU_BB`): a cut shot throws the object ball a few degrees and side english passes spin on (right english throws the object ball left). Cushions are judged at the nose (0.4 R above the centre, `PL_RAIL_H`), so a rolling ball keeps rolling after a bounce; the bounce gets softer with speed (`PL_E_RAIL0`, `PL_E_RAILK`, `PL_E_RAILMIN`) and cushion friction (`PL_MU_RAIL`) lets side spin change the bounce angle. The cue tip: `pl_shoot_sp(g, ang, power, sa, sb)`; right english also squirts the cue ball a little to the left (`PL_SQUIRT`). `pl_shoot(g, ang, power)` = the centre hit as before.
- **Break and pockets.** The break shot has its own top speed (`PL_VBREAK` 2600, normal full power `PL_VMAX` 1500). A pocket takes a ball that heads at it; one sliding past along the cushion no longer drops in (the faster the ball, the straighter it must head: `0.66 + 0.28 * speed / 900`). Ball in hand cannot be put in a pocket mouth.
- **CPU.** The shot search now runs in slices (`pl_ai_begin` / `pl_ai_work` / `out_*`): a few trial shots per frame, at most about 6 ms of work, so the picture never freezes while it thinks (it still waits 1.4-3 s in total). Skills 0-1 hit the centre, 2-3 also try follow / draw, 4 also tries side english; a shot that keeps the turn and leaves a good next pot earns a little extra, so spin has a point. `pl_ai_shot` (the old call) still gives the centre-ball answer.
- **Picture.** Balls are no longer pre-made sprites: each ball is drawn every frame as a lit sphere turned by its own orientation, at sub-pixel positions. The number, the stripe and the cue ball's little red dots roll with the ball (`PG_CUE_DOTS` in main.c turns the dots off). About 0.3 ms more per frame than before on a PC. A ball that only creeps into another makes no click.
- Tests: `test_v127` in tests/test_logic.c (the skid ends on time, follow / stun / draw, english on balls and cushions, energy never rises, orientation, determinism, pockets, the break, the CPU in slices, 16 whole games with spin) and `v127_tests` in main.c (Y + D-pad, X, the spin reaches the shot, the CPU works in slices).

## v12.8 8 ball tweaks: bigger balls, the 8 ball ring, rebound line
- **Bigger balls.** `PL_R` 8.5 -> 10.0 in the TUNING block of src/pool.h (diameter 17 -> 20 px, about +18 percent). It is a physics constant, so the balls, the rack, the cushion contact and the guide all grow together (the rack still fits; the break, the CPU and the whole logic suite still pass). If 10 is still too small, change that one number (11 is the next step). The white number patch on the ball face is wider too (`PG_PATCH` 0.832 -> 0.80 in main.c), so the number stays readable while the ball rolls.
- **The 8 ball ring.** While the 8 is on the table it wears a soft pulsing cyan-white ring (`PG_RING8_RGB` in main.c, one constant). When the player to move has cleared his group and the 8 is the target, the ring is stronger (a second ring, brighter, faster pulse). It follows the ball and is gone once the 8 is potted. The colour is not the gold hit-ball line, the white aim line or the blue cue-ball line.
- **Rebound line.** Aiming at a cushion now also shows the bounce: from the cushion contact a dotted blue line (the cue ball colour) goes on the way the ball would bounce (angle in = angle out), up to the next ball (with its ghost circle) or the next cushion, at most `PG_REB_LEN` = 220 px. The CPU's pre-shot guide shows it too. Helpers in pool.h: `pl_ray_from` (a ray from any point; `pl_ray` is now `pl_ray_from` the cue ball; `PlRay.wall` says which cushion), `pl_rebound_angle`. It is a straight-shot guide only: like the rest of the guide it ignores spin and speed, so with english or a hard shot the real bounce differs (the cushion has restitution that drops with speed, rail friction and side-spin effects).
- No save change (SAVE_VER 28). No new texts or pictures.
- Tests: `v128_tests` in main.c (ring drawn / second ring only when the 8 is the target / follows the ball / gone when potted / pulse, wall and rebound geometry, rebound stops at a ball, blue dots appear only when a cushion is hit first) and the v12.8 block in tests/test_logic.c. Screenshots `v128_*`.

## v12.9 Card faces from your pixel-art deck sheet (red and black)
- **New faces.** Every face-up card (Sophia's table, Quick Play, Exhibition, boss table, the winning-hand frame, the turning animation) is now drawn from `assets/cards.png`, your `deck_gfx.png` sheet: 13 columns (2..10, J, Q, K, A) x 4 rows (diamonds, clubs, hearts, spades), 32 px cells, the 24 x 32 card at x 4..27. Corner indices top-left and rotated bottom-right, real pip layouts, drawn J / Q / K portraits, big ace.
- **Red and black.** The sheet was four-colour (orange diamonds, green clubs, pink hearts, navy spades). `tools/make_cards.py` turns it into the two-colour deck we agreed on in v4.1: hearts and diamonds red (204,34,44), spades and clubs black (26,26,32). Plain cards and aces: every suit-colour pixel. J / Q / K: only the corner index; the painted portraits keep their own colours (orange, gold, blue). Your original is kept as `tools/deck_src/deck_gfx.png`; change the two colours at the top of the script and run it from the project folder to recolour again (`python3 tools/make_cards.py`).
- **How it is drawn.** `card_face_n()` scales the 24 x 32 card into the card rectangle by nearest neighbour (pixels stay sharp), at every game size (her hole cards 27x38, board 43x57, your cards 54x78 / 44x62, multiplayer 40x62) and while a card turns. The sheet is read once at start-up into screen-format pixels (`cards_load()`, about 53 KB). The gold winning-hand frame is drawn round the sprite. If `assets/cards.png` is missing or not 416 x 128, the old code-drawn face (letter + one big suit) is used.
- **Unchanged.** Card backs (blue, and pink at her table once the pink deck is in use), the empty slot, the tilt / projection / shadow, the animation, the hand logic. Mapping of the game's suit order (spade, heart, diamond, club) to the sheet rows is `CD_ROW[]`.
- No save change (SAVE_VER 28). No new texts.
- Tests: `v129_tests` in main.c (sheet loaded; all 52 faces pixel-exact against their sheet cell and pairwise different; index red for hearts / diamonds and black for spades / clubs on all 52; no old suit colour left on plain cards and aces; big suit on every ace; every game card size stays inside its rectangle and shows its index; gold frame; backs unchanged; fallback face without the sheet; flat and tilted turning at every step). Screenshots `v129_faces_*` are contact sheets of all 52 faces (54x78, 43x57, 27x38, gold frame).

## v13.1 8 ball: mode panel, tournament, prize 2x
- **8 BALL now opens a panel**: SINGLE MATCH / TOURNAMENT (UP/DOWN, A select, B back to the Event menu). SINGLE MATCH opens the buy-in list as before; B in the list returns to the panel.
- **Single match prize is 2x the buy-in** (`POOL_PRIZE_PCT` 200, was 180): $10 pays $20 ... $1000 pays $2000.
- **Tournament = a ladder of 6 CPUs**, played one after another, weakest to slightly stronger (`POOL_T_SKILL` {0,0,1,1,2,2}, the same skill scale as the single-match tables). Fee `POOL_T_FEE` $1000 and `POOL_T_ENERGY` 30 percent energy, both paid once at entry (the Milk Bottle waives the energy); saved at once, so leaving loses the fee. Winning all 6 pays `POOL_T_PRIZE` $6000. Losing any match ends the run (fee lost). Nothing is saved between matches (SAVE_VER 28). You break every match.
- A ladder screen (CPU 1-6: BEATEN / NEXT) shows between the matches; A starts the next one. The table's bottom line shows "MATCH n OF 6 VS CPU n". The result screen says "BEATEN BY CPU n. x OF 6 WON" or "ALL 6 CPUS BEATEN".
- Fix: the 8 ball loss screen showed the Exhibition's last buy-in ("BUY-IN LOST"); it now shows the 8 ball buy-in / fee.
- Constants: all in `src/logic.h` (v13.1 block under `g_pool_win`). Tests: `test_v131` (logic), `v131_tests` (flow, shots `v131_*`).

## v13.0 8 ball guide = the real engine, finer aim
- **The guide is now a real trial shot.** `pl_guide` (pool.h) copies the table, plays your aim, power and tip position with the same physics the shot uses (`pl_step`), and records where the balls really go. It draws: the cue ball line up to the first ball it touches (white to the first cushion, blue after it, with the real bend that speed, spin and cushion loss give), a ring where it touches, the hit ball's line (yellow, up to `PL_GD_OBJ` 200 px, ends at its first cushion / ball or in the pocket, cut throw included), the cue ball's line after the touch (blue, `PL_GD_CUE` 60 px), and a red ring where the cue ball would drop in a pocket. After the first cushion the line is at most `PL_GD_REB` 220 px and ends at the second cushion. The copy is thrown away, the real table is never changed.
- **Power.** The line follows the power: until you hold A it is drawn for `PG_GUIDE_POWER` (50 percent; the power bar says GUIDE AT 50%), while A is held it follows the power bar. The CPU's pre-shot guide shows the CPU's real shot (its power and spin).
- **Cost.** About 0.1 ms per guide on a PC. It is redone only when the aim, power, tip or any ball changed, and at most about once per 33 ms (or 3 x its own cost on a slow machine), so a fast turn on the Miyoo may trail a moment behind. `PG_GUIDE_SIM 0` in main.c brings back the v12.8 straight line + mirror bounce.
- **Finer aim.** LEFT / RIGHT start at 7 deg/s (was 22), speed up after 0.5 s held, at most 60 deg/s (was 102); L1 / R1 fine turn 2.5 deg/s (was 9); UP / DOWN nudge 0.05 deg (was 0.15). One block of constants: `PG_AIM_*` in main.c.
- What the old guide got wrong (measured on the v12.8 engine): at full power the rebound was 20-28 deg off, with draw 40-90 deg, with side english 5-25 deg; the hit ball went about 3 deg off the line (cut throw, about 20 px at a pocket 400 px away); after a thin cut the cue ball line was 20-40 deg off. The table picture and the cushion constants were checked and match.
- No save change (SAVE_VER 28). No new texts or pictures.
- Tests: `test_v130` in tests/test_logic.c (600 random shots: the guide changes nothing, repeats, touches the same ball as the real shot, ghost ball exactly 2 R from it, object ball direction, bend / cushion bookkeeping, every sample inside the table, full power rebound far from the mirror) and `v130_tests` in main.c (bend vs mirror soft / full, every sample a real position, draw / english change the line, power bar, redo only on change, scratch ring, cut shot lines, finer aim values). The v12.8 rebound checks were rewritten for the new line. Screenshots `v130_*`.

## v13.2: room upgrade 3 and the playing room
- **Upgrade 3** (reception desk, cash only): $20,000, on offer as soon as level 2 is bought (whether or not Sophia lives with you). Rent becomes $2,000. `ROOM_UP3_COST`, `RENT_UP3_AMT`, `ROOM_LVL_MAX` in `src/logic.h`; `g_room_lvl()` is 3 once it is bought (also while she lives here). The desk has its own greeting (`W_CLERK_UP3`) and done line (`W_UPG_OK3`), editable in lines.txt.
- **Level 3 room**: `assets/map_room_up3.png` (11 x 7 tiles, grid `MG_ROOM_UP3`): grey tiled living-room floor and a new door in the left wall at (0,3). Stand on (1,3) facing left and press A.
- **Playing room**: `assets/map_play.png` (11 x 9 tiles, grid `MG_PLAY`, map `MAP_PLAY`). You arrive at (9,6); its only door is at (10,6) and leads back to (1,3). Everything in it (lamp, armchair, poker table, pool table, plant) is picture only for now; nothing answers A. Sophia does not come in.
- SAVE_VER 29 (room level 0..3); a version-28 save loads unchanged. 2 new texts (421 in total).

## v13.3: invite Sophia to the playing room, the strip round the pool table, no more shyness once married
- **Invite.** Her menu (page 1: PLAY A MATCH, CHAT) has a new row INVITE TO PLAYING ROOM once room upgrade 3 is bought. Open her menu with A on her in the living room: she says yes (pool `R_INV_YES`, note USE THE LEFT DOOR). From her bedroom, or at her bedroom door, she refuses (`R_INV_NO`). The invitation is kept while you stay in the living room and is cancelled by leaving through any other door, by a new time of day, or by her going back to her room.
- **The door.** With the invitation, press A on the left door (0,3) of the living room: you arrive at (9,6) in the playing room and she appears on the free tile nearest to you, turned towards you. Without the invitation you enter alone, as before. Pressing A on the playing room door (10,6) brings her back with you: she stands beside you in the living room (she is not rolled again) and the invitation is used up.
- **In the playing room** she strolls over every walkable tile (the same slow wander as in the living room, never on your tile, and she leaves the tile in front of the door quickly so you can always leave). A on her: one short PLACEHOLDER line (`R_PLAY_TALK`, 3 lines), no menu yet.
- **The invisible wall.** The strip left of the pool table (1,4)-(1,6) and the two tiles above it (2,3), (3,3) are floor you can walk to now. You enter over the plant tile (1,7) and go up to the lamp tile (1,3). The walking picture has no layer in front of you, so on those two tiles you stand in front of the plant and the lamp. The table, the armchair and the poker table stay solid.
- **Shyness.** `ls_shy()` is false once you are married: from the wedding on she wanders the living room again (before it she stays in her room from day 6 of the pill, as before). Side effect: a married Sophia can stand in the living room in the evening, so knocking on her bedroom door then gives the "Don't you dare enter my room" line.
- No save change (SAVE_VER 29; the invitation is not saved). 3 new texts (424 in total); `packaging/lines.txt` regenerated.
- Tests: `v133_tests` in main.c (strip tiles reachable, solid table, shyness, menu row, invite yes / refusals, door with and without the invitation, 3000 wander steps, A on her, way back, invitation ends, text fit); v9.2 evening roll test and v9.9 shyness test adjusted (`ls_noroll`, `married = 0` for the shy part).

## v13.4: the poker table opens her menu, no more matches in the living room
- **Poker table.** In the playing room the table edge tiles (7,3), (7,4), (9,4) and the chair (8,5) are interactive (tile `G`, still solid). A on them with Sophia in the room opens her menu (page 1: PLAY A MATCH, CHAT); alone you get one line (`R_PLAY_ALONE`). The menu, her scenes and a match started there use the new placeholder picture `assets/bg_playroom.png` (`BG_PLAYROOM`, 752x560; `tools/make_playroom_bg.py` makes it from the map). Replace the file with real art under the same name.
- **Matches.** PLAY A MATCH is gone from her menu when you talk to her in the living room. It stays in her bedroom, at her bedroom door and at the poker table. The free home match is the same one everywhere (once a day, 10 or 20 hands, 10% energy).
- **Playing room menu.** No INVITE row (you are there already) and no SLEEP row. Her scenes use the living-room variants (`lr_on()` is true there).
- No save change (SAVE_VER 29). 1 new text (425 in total); `packaging/lines.txt` regenerated. Tests: `v134_tests` (shots `v134_*`).


- `build.txt` (v13.5): the build label at the bottom left of the title screen. Edit the first non-# line (30 characters max); empty line = no label; delete the file to get "BUILD 0.1 BETA" back.

## v13.9 8 ball: single match prize 2.5x

- `POOL_PRIZE_PCT` 250 (was 200): $10 pays $25, $50 pays $125, $100 pays $250, $500 pays $1250, $1000 pays $2500. All five buy-ins give whole dollars. The buy-in screen text now reads "WIN 2.5 TIMES IT" (it was built with integer division and would have said 2). The tournament ladder (fee 1000, prize 6000) is unchanged.

## v13.10 three chats a day, then Sophia goes to her bedroom

- `CHAT_LIMIT` 3 (logic.h): every chat with her (good or bad) counts; the 3rd chat of the day ends with her goodbye line instead of "anything else?" (new pools `R_CHAT_DONE`, `_M`, `_MM`, `_CW`, `_CW_MM` in talk.txt, same five modes as `R_MORE`).
- Living room: after the 3rd chat her menu stays as it is, but as soon as you are back on the map she walks to her bedroom door and goes in (the existing "walk home" of v8.1). She does not come out again for the rest of that day (`ls_roll` checks `chat_full()`); the next morning she is back to normal.
- Playing room (she came with you): she is simply gone when you leave her menu, with the note "SOPHIA WENT BACK TO HER ROOM".
- At her door or in her bedroom: nothing moves (she is already home). CHAT (and INVITE) answer with her goodbye line until the next day. TEASE, hugs, gifts and matches are not chats and are not limited.
- The counter is kept in memory only (like `chat_gain_n`): closing the game resets it. `CHAT_DAY_MAX` (4) can no longer be reached. No save change (SAVE_VER 29).
- Tests: new `v1310_tests` (living room 3 chats + walking home + menu close, 4th chat refused, no invitation, stays away the whole day, back next day, five text modes, playing room, door), v77 rewritten for 3 chats a day, older chat loops reset the counter, stale v131 prize check updated to 2.5x. Registry 436 -> 441.

## v13.11 mama mode page 4: INTIMATE HUG and BOTTLE FEEDING

- Page 4 in mama mode now has six rows: HUG, KISS, SLEEP, BREASTFEED, INTIMATE HUG, BOTTLE FEEDING (new action ids `RA_IHUG`, `RA_BFEED`; `RA_GIVE` moved to 18).
- INTIMATE HUG: only inside her bedroom (menu opened inside her room, or walking in her bedroom). Door, living room and playing room: she refuses (`R_IHUG_NO_MM`). Scene `SCR_IHUG`: opener pair (`B_IHUG_OPEN_MM`), lying on her bed on the new IHUG picture, one middle pair (`B_IHUG_MID_MM`), remembered as a hug.
- BOTTLE FEEDING: same gate and cost as PUMP MILK / ASK FOR MILK (full gauge, the injection's daily milk, or the Super drug's asks; broken pump, full bag handled as before; row shows LOCKED when not ready; `g_bfeed_ready`). Same pump scene and peek choice, then the hand-over `B_BFEEDH_MM` (peeking adds the scolding), then a choice (`peek_ctx` 2): TAKE THE BOTTLE AND LEAVE (`B_BFEEDL_MM`, bottle stays in the bag) or ASK HER TO FEED ME (`B_BFEED_ASK_MM` pair, `B_BFEED_MM` on the new BFEED picture, `B_BFEED_MID_MM` pair). Being fed drinks the bottle at once (`g_drink_bottle`: no energy drop for `MILK_DAYS` days); if one already works the bottle stays in the bag.
- Sophia is arrogant and condescending in all new text and still pushes back on the name. No "baby" or "boy" in these texts.
- New pictures (PLACEHOLDERS, `tools/make_ihug_bfeed_placeholders.py`): `sc_ihug_<outfit>.png` and `sc_bfeed_<outfit>.png`, outfits d8 / yoga / top / cow, 752x560. Hand-over uses `sc_bottle_*`.
- Registry 441 -> 450 (9 new texts in lines.txt). Old tests that expected 4 rows on mama page 4 now expect 6.
- Tests: test_logic 0 failed; --shots new v13.11 group problems 0; only the known old v80 x2, v81, v126 messages remain; --autoplay identical to v13.10 (3 known problems).

## v13.14 8 ball speed
- The 8 ball is drawn faster, mostly with a full table. Every ball used to be shaded again pixel by pixel on every frame, even when it had not moved, and every pixel went through SDL_MapRGB / SDL_GetRGB. Now each ball (and its shadow) is shaded into its own small picture (`PgSpr`, `pg_ball_spr` / `pg_shadow_spr` in main.c) that is kept while the ball's position and turn stay the same, so a ball at rest costs only a copy and a rolling ball is shaded once per frame. The shading maths is unchanged (the test compares it with the v13.13 code, kept as `pg_blit_ball_ref` / `pg_blit_shadow_ref` under HAVE_SHOTS).
- `assets/pool_table.png` is loaded opaque (a plain copy each frame instead of a per-pixel alpha blend); the 8 ball's ring uses a cos / sin table.
- L1 / R1 fine turn: `PG_AIM_FINE` 2.5 -> 0.8 degrees per second. The guide may be redone every frame (minimum wait 16 ms, was 33), so a slow turn does not move in steps.
- Frame meter (for tuning on the handheld): on the table screen hold L1 + R1 and press Y. Top left: frames per second and the work per frame in ms (update + draw + copy to the screen, without the wait). Off by default, not saved.
- Tests: new v1314_tests (new drawing equals the old one within 2 levels, cache hit / miss, opaque table, fine turn 0.8 and L1 = R1, frame meter toggle). test_logic untouched.

## v13.15 bigger money, high stock prices, bank interest
- MONEY: cash and bank each stop at $999,999,999 (`MONEY_CAP`, was $999,999). Every sum of money is done in 64 bits (`long long`; `long` is only 32 bits on the Miyoo). `fmt_money64` prints 64-bit values with commas.
- ITEMS: `INV_CAP` and `STK_CAP` are 999,999,999 (were 99): no real limit on item counts or shares per company. One-time items (ring, pill, pump, diamond ring, injection) stay one-time.
- STOCKS: every price is above $3,000 and at most $5,000 (`STK_MIN_PRICE` / `STK_MAX_PRICE`). Base / swing: NVIDIA 4200 +-18%, GOOGLE 3600 +-14%, OPENAI 4000 +-20%, APPLE 3400 +-9%, SONY 3800 +-15%, SPACEX 4500 +-11% (the dearest, up to $4,995). The price formula is unchanged, so a career's market still never re-rolls on load.
- BANK INTEREST: 0.5 percent a day (`INT_PM` 5 per mille) of the first $10,000 of the bank (`INT_LIMIT`), at most $50 a day, paid into the bank once per new day when you wake up in the bed (`g_interest_pay`, `int_day` guards a second payment). Sofa phases pay nothing; cash earns nothing. Waking shows "Your bank paid you $X interest." (new text `W_INTEREST`, registry 450 -> 451), before the rent reminder on the same page. The ATM menu shows the rate, `NEXT MORNING +$X` and `LAST PAID +$X`.
- ATM: the amount has 9 digits (`ATM_DIG`); the bank balance shrinks to size scale 3 when it is too wide.
- SCREENS: the cash boxes and the stock rows (OWNED / VALUE) shrink their text to fit (`text_rfit`).
- SAVE: version 30 (new fields `int_day`, `int_last`, last in the struct). Version 29 and 28 saves load and are upgraded in place (nothing lost, interest starts with the first morning). An older build refuses a new save that holds more than $999,999.
- Tests: `test_v1315` in tests/test_logic.c (caps and 64-bit money, every price of every company / day / seed, interest rules, v29 upgrade), flow tests in `src/main.c` (bed pays interest, sofa pays nothing, 9-digit ATM), shots `v1315_*` (big ATM, cash box, stocks). Old stock tests were changed for $3,000+ prices.

- v13.16: 8 BALL TOURNAMENT costs 50% energy (POOL_T_ENERGY) and is open once a week: Game.pt_day = the day of the last entry, the next entry is open 7 days later (POOL_T_WEEK, g_pool_t_wait). Refused entries do not use up the week; the Milk Bottle waives the energy but not the lock. Exhibition and 8 ball single match: energy by buy-in 5 / 10 / 15 / 25 / 30% (the $5000 boss table 30%). SAVE_VER 31 (a version-30 save upgrades in place, tournament open).
