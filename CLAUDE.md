# Lost Trail — GAME-018

> Survival roguelike with an LLM-driven dog companion and a "dog translator" device.
> Forked from UE 5.6 ThirdPerson-Combat template + DogCompanion brain architecture.

---

## LESSONS FROM NEONPATROL (GAME-017) — READ FIRST

> The NeonPatrol Director session (2026-04-12/13) built a complete AI companion game
> in one session. These lessons MUST be applied here. Full playbook:
> `C:\Users\Kris\Kriisko-Studio\docs\playbooks\llm-companion-game.md`

### Critical changes from the other Director's plan:

**1. Use Groq API, NOT local Qwen 0.8B**
- The other Director set TrailDogBrain to use `127.0.0.1:11001` (local Qwen 0.8B)
- This WILL NOT WORK when UE5 editor is open — the 4060 only has 8GB VRAM
- UE5 + Lumen needs ~4GB VRAM. Local LLM needs ~1-2GB. They can't coexist.
- **Fix**: Change default endpoint to `https://api.groq.com/openai/v1/chat/completions`
- **Fix**: Add API key loading from file (see SparkBrainComponent pattern)
- Model: `llama-3.3-70b-versatile` (smarter than 0.8B, free, cloud-based)
- Keep local fallback for packaged builds without internet

**2. Use JSON response format, NOT "ACTION | SPEAK" plaintext**
- The other Director uses: `FOLLOW | SPEAK WATER CLOSE`
- This is fragile. NeonPatrol proved JSON works better:
  `{"action":"FOLLOW", "speak":"WATER CLOSE", "target":[100,200,0]}`
- LLMs are trained on JSON. Parsing is trivial. Edge cases are handled.
- Add markdown stripping (LLM sometimes wraps in ```json blocks)

**3. Auto-spawn the dog via subsystem**
- Don't rely on Python remote exec to place the dog in the level
- Use a UTickableWorldSubsystem that spawns the dog on first tick if none exists
- This works in both editor PIE and packaged builds

**4. Widget construction: RebuildWidget(), NOT NativeConstruct()**
- If building UMG widgets programmatically in C++, override `RebuildWidget()`
- `NativeConstruct()` fires AFTER Slate widget is generated — too late for WidgetTree
- The NeonPatrol ChatOverlayWidget was invisible until this was fixed

**5. Projectile/combat damage: use ICombatDamageable + overlap detection**
- Template enemies use ICombatDamageable::ApplyDamage(), not UE5 TakeDamage
- Destructible boxes also use ICombatDamageable
- Fast projectiles need OnComponentBeginOverlap, not OnComponentHit
- Set collision: QueryOnly, overlap Pawn+WorldDynamic, block WorldStatic

**6. Visual feedback > audio commentary**
- DO NOT have the dog speak via TTS constantly — it talks over player/LLM
- Use colors/visual states for status (green=safe, orange=alert, red=danger)
- Only use TTS for direct player conversations (voice chat responses)
- The translator text IS the commentary — floating text above the dog is enough

**7. Chat UI keys: Enter=open/submit, Tab=close**
- Same key for open/close/submit causes UX conflicts
- Enter opens chat when closed, submits text when open
- Tab closes chat and returns to gameplay
- V = push-to-talk voice (no chat panel, seamless, stays in gameplay)
- Check `IsChatVisible()` not widget visibility (widget is always in viewport)

**8. Leash system: teleport companion back**
- Dog WILL wander off or fall through the floor
- Add a max distance check (2000u) in Tick — teleport back if exceeded
- Also check Z position (fell off map)

**9. Package-readiness from day 1**
- API key path: search GameDir/groq.key first, then dev paths
- Auto-spawn companion via subsystem (no editor dependency)
- Test with `tools/test_packaged.py` pattern before shipping

**10. AI test player for TDD**
- Write `tools/ai_test_player.py` that verifies without PIE:
  - Editor running + RC API
  - Dog in level with all components
  - Groq API responds with correct format
  - Conversational commands parse correctly (8+ varied phrases)
  - Audio bridge status
- Run after every change. Don't rely on Kris to test.

---

## Quick start for the Director

1. **Read the playbook**: `C:\Users\Kris\Kriisko-Studio\docs\playbooks\llm-companion-game.md`
2. **GDD:** `D:\Games\LostTrail\GDD.md` — full design, milestones, architecture
3. **First task**: Fix TrailDogBrain to use Groq API + JSON response format
4. **Second task**: Compile and fix include errors
5. **Third task**: Write a GameMode subsystem that auto-spawns the dog
6. **Fourth task**: Build translator UI using RebuildWidget() pattern
7. **VRAM**: Kill llama-server before opening UE5 editor

## Source layout

```
Source/
├── TP_ThirdPerson/              # Epic's template (combat variant) — don't modify
│   └── Variant_Combat/          # CombatCharacter, CombatEnemy, spawners, interfaces
│
└── LostTrail/                   # OUR module
    ├── Public/
    │   ├── Dog/
    │   │   ├── TrailDogBrain.h         # LLM brain — CHANGE TO GROQ + JSON FORMAT
    │   │   └── TrailDogCharacter.h     # AI dog pawn, movement, health, translator bridge
    │   ├── Survival/
    │   │   └── SurvivalComponent.h     # Hunger, thirst, warmth
    │   ├── Translator/
    │   │   ├── TranslatorComponent.h   # Battery, command issuing, phrase validation
    │   │   └── TranslatorVocabulary.h  # Constrained phrase list (50+ phrases, 4 levels)
    │   └── World/
    │       └── DayNightManager.h       # Day/night cycle, rain, phase events
    │
    └── Private/
        ├── Dog/
        │   ├── TrailDogBrain.cpp
        │   └── TrailDogCharacter.cpp
        ├── Survival/
        │   └── SurvivalComponent.cpp
        ├── Translator/
        │   ├── TranslatorComponent.cpp
        │   └── TranslatorVocabulary.cpp
        └── World/
            └── DayNightManager.cpp
```

## What's built (files on disk, not yet compiled)

- **TrailDogBrain** — Polls LLM. ⚠️ NEEDS REWRITE: switch to Groq API + JSON format
- **TrailDogCharacter** — AI dog pawn. LLM drives behavior. Has health.
- **TranslatorComponent** — Battery-managed two-way bridge. Validates phrases.
- **TranslatorVocabulary** — 40+ phrases across 4 upgrade levels.
- **SurvivalComponent** — Hunger/thirst/warmth with environmental modifiers.
- **DayNightManager** — Dawn→Day→Dusk→Night cycle. 10-minute days. Rain system.

## What's NOT built yet

1. **Fix TrailDogBrain** — Groq API + JSON format + API key loading + markdown stripping
2. **Compile and fix** — Expect include path issues (NeonPatrol had ~14 fixes)
3. **GameMode subsystem** — Auto-spawns dog, handles run state
4. **Translator UI** — Floating text (use RebuildWidget pattern from NeonPatrol)
5. **Voice chat** — Reuse audio_bridge.py + SparkVoiceComponent pattern
6. **Forest generator** — Procedural trees, POIs, predator zones
7. **Predator AI** — Extend CombatEnemy for wolves/bears
8. **Campfire** — Placeable actor, warmth + translator recharge
9. **Forageables** — Interactable survival items
10. **Package** — Use RunUAT pattern from NeonPatrol

## Delegation notes for Groq fleet

When delegating C++ to Groq (`python tools/groq_delegate.py`):
- Module is `LostTrail`, API macro is `LOSTTRAIL_API`
- Use JSON response format: `{"action":"FOLLOW", "speak":"WATER CLOSE", "target":[100,200,0]}`
- Include `Misc/FileHelper.h` for API key loading
- Use `TEXT()` macros for all JSON field names (UE5 5.6 deprecation)
- Combat interfaces from template: `ICombatDamageable`, `ICombatAttacker`
- Add `TP_ThirdPerson` to PrivateDependencyModuleNames for interface access
- Use `Cast<ICombatDamageable>()` not `Execute_ApplyDamage()`
- Survival/translator components use delegate-based events (not polling)

## Reusable from NeonPatrol (copy these)

| Component | Source | Use for |
|-----------|--------|---------|
| ChatOverlayWidget | `D:\Games\NeonPatrol\Source\NeonPatrol\ChatOverlayWidget.*` | Translator text display (adapt styling) |
| NeonPatrolChatSubsystem | Same path | Auto-spawn dog + create UI + input polling |
| SparkVoiceComponent | Same path | Voice chat with dog (rename to DogVoiceComponent) |
| audio_bridge.py | `D:\Games\NeonPatrol\tools\audio_bridge.py` | STT/TTS server |
| ai_test_player.py | Same path | Automated testing template |
| test_packaged.py | Same path | Package verification template |

## Key design decisions

- **The LLM IS the behavior tree.** No BT/blackboard for the dog.
- **JSON response format.** `{"action":"...", "speak":"...", "target":[x,y,z]}`
- **Groq API primary, local Qwen fallback.** Cloud for dev, local for offline play.
- **The translator limitation IS the game design.** Short phrases > full sentences.
- **The dog can die.** Real stakes. Player carries it (slow, can't fight).
- **Roguelike, not roguelite.** Permadeath. 20-40 minute runs.
- **Visual status > audio commentary.** Dog color states for mood, translator text for speech.
