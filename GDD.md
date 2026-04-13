# Lost Trail — Game Design Document

> **GAME-018** · UE5 5.6 · Kicked off 2026-04-13 · Status: pre-production
> Forked from: ThirdPerson-Combat template + DogCompanion LLM brain

---

## 1. One-line pitch

*You're lost in a vast forest with your dog and a broken prototype "dog translator." Survive together — the dog senses what you can't, and you're the only one with thumbs.*

## 2. The hook

You found an experimental device that translates between human and dog. It's crude — short
phrases, sometimes garbled, limited battery. But it turns your dog from a pet into a partner.
The dog smells predators before you see them. It knows where water is. It gets scared, gets
excited, gets hurt. And now, for the first time, it can *tell you*.

**The product is the relationship.** The forest is the pressure that makes the relationship
matter.

## 3. Pillars

1. **The translator is limited, not broken.** You don't get full sentences — you get
   fragments like "SMELL BAD THAT WAY", "WATER CLOSE", "SCARED", "FOLLOW ME". The
   limitation IS the design. Players fill in the gaps with empathy. A dog saying
   "DANGER" is more evocative than an NPC saying "There are wolves to the north."

2. **The dog is alive, not scripted.** Same philosophy as DogCompanion — every decision
   comes from an LLM call (Qwen 0.8B, local inference, ~15ms). The translator adds a
   second channel: the dog's *actions* show what it's doing, the translator shows what
   it's *thinking*.

3. **Survival creates stakes.** Hunger, thirst, temperature, health. Without stakes,
   the translator is a toy. With stakes, "WATER CLOSE" is a lifeline. The dog becomes
   essential — not decorative.

4. **Roguelike structure keeps scope contained.** Procedural forest, permadeath, runs
   last 20-40 minutes. Between runs: unlock translator upgrades (longer phrases, new
   word categories, faster battery recharge). The dog remembers across runs.

## 4. The translator mechanic

### Dog → Player (automatic, LLM-driven)

The translator passively converts the dog's internal state into short phrases displayed
as floating text above the dog (like speech bubbles, but crude/digital-looking).

| Dog state | Example translations |
|-----------|---------------------|
| Senses predator nearby | "SMELL BAD", "DANGER NEAR", "SOMETHING WRONG" |
| Found water/food | "WATER HERE", "FOOD SMELL", "GOOD THING CLOSE" |
| Scared | "SCARED", "WANT GO", "NOT SAFE" |
| Happy/relaxed | "GOOD", "LIKE HERE", "HAPPY" |
| Hurt | "HURT", "PAIN", "HELP" |
| Lost sight of player | "WHERE YOU", "WAIT", "COME BACK" |
| Found shelter | "SAFE PLACE", "HIDE HERE", "DRY SPOT" |
| Investigating something | "WHAT THIS", "SMELL NEW", "LOOK LOOK" |

The LLM picks from a constrained vocabulary (~50 phrases) based on the game state.
This keeps responses fast and prevents the LLM from going off-script.

### Player → Dog (manual, limited by battery)

The player can press a key to open a radial menu of 4-6 commands:
- **COME** — dog returns to player
- **STAY** — dog holds position
- **SCOUT** — dog explores ahead, reports back via translator
- **FIND WATER** — dog searches for water source
- **FIND SHELTER** — dog searches for cover/shelter
- **QUIET** — dog stops barking (reduces noise that attracts predators)

Each command costs translator battery. Battery recharges slowly over time.
Upgrades between runs expand the command list and reduce battery cost.

## 5. Survival systems (deliberately simple)

| Stat | Drains from | Restored by | If depleted |
|------|-------------|-------------|-------------|
| **Hunger** | Time (slow) | Berries, mushrooms, cooked meat | Stamina stops regenerating |
| **Thirst** | Time (faster than hunger) | Streams, rain collection | Health drains |
| **Warmth** | Night, rain, elevation | Fire, shelter, movement | Stamina drain + screen frost |
| **Health** | Predator attacks, falls, cold | Herbs, rest at fire, time | Death → end of run |
| **Dog health** | Predator attacks, thorns | Player feeds dog, rest | Dog can't move → you carry it (slow) |
| **Translator battery** | Using commands | Time (slow), campfire (fast) | No commands, no translations |

The dog has its own health bar. If the dog goes down, you can carry it but you're
slow and can't fight. Keeping the dog alive IS the game.

## 6. The run structure

```
START: You wake up in a forest clearing. Dog is with you. Translator is on.
       A compass points vaguely "out" — toward the forest edge.

GOAL:  Reach the forest edge before nightfall on Day 3.
       (the forest is procedurally generated, ~5 min walk in the right direction,
        but predators, terrain, weather, and resource needs create detours)

DAYS:  Each "day" is ~10 minutes real-time.
       Dawn   — safe, forage
       Midday — predators become active
       Dusk   — must find/build shelter
       Night  — can't travel, survive until dawn

DEATH: Permadeath. Return to the cabin (meta hub).
       Dog greets you. It remembers.

META:  Between runs, in the cabin:
       - Upgrade translator (new phrases, longer battery, faster recharge)
       - Dog brings you "gifts" from previous runs (unlockable cosmetics)
       - Dog's dialogue in the cabin references past runs ("REMEMBER WOLVES",
         "MISS FOREST", "AGAIN?")
```

## 7. Enemies / threats

| Threat | Behavior | Dog's role |
|--------|----------|-----------|
| **Wolves** | Hunt in packs, stalk before attacking, scared of fire | Dog smells them early ("SMELL BAD"), growls to warn, can fight one wolf but not a pack |
| **Bear** | Territorial, avoidable if you're quiet, devastating if provoked | Dog warns ("BIG DANGER"), you can command QUIET to avoid detection |
| **Cold** | Drains warmth at night and in rain | Dog can lead to shelter ("SAFE PLACE"), huddle together at campfire for warmth bonus |
| **Terrain** | Cliffs, rivers, dense thickets block paths | Dog scouts ahead ("NO GO", "WAY HERE"), finds crossings |
| **Hunger/thirst** | Time pressure | Dog finds resources ("WATER HERE", "FOOD SMELL") |

No human enemies. The forest is the antagonist.

## 8. Technical architecture

### What we inherit from the templates

**From ThirdPerson-Combat:**
- `ACombatCharacter` — player character with health, damage, melee, death/respawn
- `ACombatEnemy` — AI enemy with StateTree, combos, health bars, death
- `ACombatEnemySpawner` — enemy spawning with activation triggers
- `ICombatDamageable` / `ICombatAttacker` — clean interfaces we reuse
- Combo attack system, charged attacks, knockback
- Camera rig, Enhanced Input

**From DogCompanion (code adapted, not imported directly):**
- `UDogBrainComponent` pattern — async HTTP to local LLM, tolerant parser
- `ADogCharacter` pattern — LLM-as-behavior-tree, action→movement mapping
- `FDogGameState` / `FDogDecision` — compact state snapshot for LLM context

### New systems to build

| System | Key class | Description |
|--------|-----------|-------------|
| **Translator** | `UTranslatorComponent` | Manages battery, converts dog brain state to short phrases, handles player commands |
| **Translator UI** | `UTranslatorWidget` | Floating text above dog (digital/glitchy aesthetic), command radial menu |
| **Survival** | `USurvivalComponent` | Hunger, thirst, warmth tracking on player character |
| **Dog Survival** | Extend `ADogCharacter` | Dog's own health, energy, scared state |
| **Forest Generator** | `AForestGenerator` | Procedural forest: trees, clearings, streams, cliffs, POIs, shelter spots |
| **Day/Night** | `ADayNightManager` | Time progression, lighting changes, predator activity triggers |
| **Run Manager** | `ALostTrailGameMode` | Run state, win/lose conditions, meta progression save |
| **Predator AI** | Extend `ACombatEnemy` | Wolf/bear with perception, stalking, pack behavior |

### LLM prompt design for translator

The brain prompt is expanded from DogCompanion to include a "speak" field:

```
System: You are the brain of a dog in a survival game. Given the current state,
respond with ONE LINE: ACTION [x,y,z] | SPEAK phrase

Valid actions: FOLLOW, WAIT, WANDER, INVESTIGATE, SCOUT, FLEE, FIGHT, SIT
Valid phrases: (constrained list — see TranslatorVocabulary.h)
If nothing worth saying, use SPEAK NONE.

Example: FOLLOW | SPEAK WATER CLOSE
Example: FLEE 100,200,0 | SPEAK SCARED
Example: INVESTIGATE 50,0,0 | SPEAK WHAT THIS
```

The constrained phrase list ensures the LLM can't produce garbage. The translator
component validates the phrase against the list and drops anything not recognized.

## 9. Art direction (minimal viable aesthetic)

- **Forest:** Stylized low-poly. Think Firewatch meets The Long Dark but simpler.
  Can start with UE5 starter content trees + Fab free nature packs.
- **Dog:** Start with the mannequin. Replace with a low-poly dog mesh when one is available.
  The AI behavior sells the character more than the model.
- **Translator UI:** Retro-digital. Green monochrome text on a dark overlay. CRT scanline
  effect. The device is a prototype — it should LOOK like a prototype.
- **Lighting:** Lumen handles forest canopy light beautifully with zero custom work.

## 10. Scope & milestones

| Milestone | Definition of done | Estimated effort |
|-----------|-------------------|------------------|
| **M1: Dog talks** | Player walks in forest, dog follows, translator shows phrases from LLM | 1-2 days |
| **M2: Dog listens** | Player can send commands, dog responds, battery drains | 1 day |
| **M3: Survive a day** | Hunger/thirst/warmth, forageable items, campfire, day/night cycle | 2-3 days |
| **M4: Predators** | Wolves spawn, dog warns, player can fight or flee | 2-3 days |
| **M5: Full run** | 3-day run structure, procedural forest, win condition (reach edge) | 3-5 days |
| **M6: Meta loop** | Cabin hub, translator upgrades, dog memory across runs | 2-3 days |
| **M7: Polish** | Sound, particles, screen effects, menu, save/load | 3-5 days |

**Total estimate: 2-4 weeks to first-playable.**

## 11. Why this game sells

1. **"Talk to your dog" is an instant hook.** One sentence Steam description, one GIF of
   the translator in action, and people wishlist it.
2. **Solo-friendly social game.** The co-op "friendslop" trend (R.E.P.O., PEAK) proves
   players crave social interaction in games. This gives solo players a companion that
   feels genuinely reactive.
3. **Survival roguelikes sell.** The Long Dark, Don't Starve, Darkwood — proven market.
4. **Low art requirements.** The dog's AI sells the game, not the graphics. Stylized
   low-poly is acceptable and cheap to produce.
5. **LLM differentiator.** No other indie survival game has an AI companion that actually
   communicates dynamically. Press will cover it for the tech alone.
6. **Streamable.** Dog saying unexpected things = clip content.
7. **Price point:** $9.99-$12.99. Impulse range.

## 12. Out of scope for MVP

- Multiplayer / co-op (add post-launch if demand exists)
- Multiple dog breeds / personalities (DLC potential)
- Crafting beyond basic campfire + tools
- Story / narrative beyond "get out of the forest"
- Mod support
- Console ports
