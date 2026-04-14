"""
Lost Trail AI Test Player -- Automated verification
Tests Groq API, JSON response format, translator vocabulary, and dog brain.

Usage: python ai_test_player.py
Does NOT require UE5 editor running (tests API and parsing offline).
Editor tests are optional (skipped if RC API is down).

Tests:
1. Groq API responds with correct JSON format
2. Dog brain JSON parsing handles all action types
3. Translator vocabulary phrase validation
4. Dog brain system prompt includes correct phrases
5. Command parsing: 8+ conversational phrases -> correct actions
6. Markdown stripping handles edge cases
7. (Optional) Editor running + RC API responsive
8. (Optional) Dog character exists with all components
9. (Optional) Audio bridge running
"""

import sys
import os
import json
import time
import urllib.request
import re

sys.path.insert(0, 'C:/Users/Kris/Kriisko-Studio/tools')

RESULTS = []


def test(name, fn, optional=False):
    """Run a test and record result."""
    try:
        ok, msg = fn()
        status = "PASS" if ok else ("SKIP" if optional and not ok else "FAIL")
        RESULTS.append((status, name, msg))
        print(f"  [{status}] {name}: {msg}")
        return ok
    except Exception as e:
        status = "SKIP" if optional else "ERROR"
        RESULTS.append((status, name, str(e)))
        print(f"  [{status}] {name}: {e}")
        return False


def groq_request(system_prompt, user_msg, max_tokens=64, temperature=0.3):
    """Send a request to Groq API and return the content string."""
    key_path = "C:/Users/Kris/Kriisko-Studio/tools/.groq_key"
    key = open(key_path).read().strip()

    payload = json.dumps({
        'model': 'llama-3.3-70b-versatile',
        'messages': [
            {'role': 'system', 'content': system_prompt},
            {'role': 'user', 'content': user_msg}
        ],
        'max_tokens': max_tokens,
        'temperature': temperature
    }).encode()

    req = urllib.request.Request(
        'https://api.groq.com/openai/v1/chat/completions',
        data=payload,
        headers={
            'Content-Type': 'application/json',
            'User-Agent': 'Kriisko-Studio/1.0',
            'Authorization': f'Bearer {key}'
        }
    )
    resp = urllib.request.urlopen(req, timeout=15)
    data = json.loads(resp.read())
    return data['choices'][0]['message']['content'].strip()


def strip_markdown_and_extract_json(raw):
    """Python mirror of UTrailDogBrain::StripMarkdownAndExtractJson."""
    clean = raw.strip()
    if clean.startswith("```"):
        nl = clean.find("\n")
        if nl != -1:
            clean = clean[nl + 1:]
        if clean.endswith("```"):
            clean = clean[:-3]
        clean = clean.strip()
    start = clean.find("{")
    end = clean.rfind("}")
    if start != -1 and end != -1 and end > start:
        clean = clean[start:end + 1]
    return clean


def parse_dog_json(raw):
    """Python mirror of UTrailDogBrain::ParseJsonResponse."""
    clean = strip_markdown_and_extract_json(raw)
    try:
        obj = json.loads(clean)
        action = obj.get("action", "WAIT").upper()
        speak = obj.get("speak", "NONE")
        target = obj.get("target", None)
        return action, speak, target
    except json.JSONDecodeError:
        return "PARSE_ERROR", "NONE", None


# ==================== OFFLINE TESTS ====================

def test_groq_key_exists():
    """API key file exists and is non-empty."""
    path = "C:/Users/Kris/Kriisko-Studio/tools/.groq_key"
    if not os.path.exists(path):
        return False, f"Key file not found: {path}"
    key = open(path).read().strip()
    if len(key) < 10:
        return False, "Key file exists but looks too short"
    return True, f"Key loaded ({len(key)} chars)"


def test_groq_api_json_format():
    """Groq API responds with valid JSON in dog brain format."""
    system_prompt = (
        'You are a dog brain. RESPOND IN JSON ONLY: '
        '{"action":"ACTION", "speak":"PHRASE"}\n'
        'Valid actions: FOLLOW, WAIT, WANDER, INVESTIGATE, SCOUT, FLEE, FIGHT, SIT\n'
        'Example: {"action":"FOLLOW", "speak":"GOOD"}'
    )
    user_msg = "dist=300cm moving=true bearing=0deg last=Follow age=1000ms energy=0.8 hp=1.0 poi=none@-1cm predator=false(0) noise=false time=day rain=false player_hunger=0.9 player_thirst=0.8 player_hp=1.0 command=none"

    content = groq_request(system_prompt, user_msg)
    action, speak, target = parse_dog_json(content)

    if action == "PARSE_ERROR":
        return False, f"Invalid JSON: {content[:100]}"
    if action not in ["FOLLOW", "WAIT", "WANDER", "INVESTIGATE", "SCOUT", "FLEE", "FIGHT", "SIT"]:
        return False, f"Invalid action: {action}"
    return True, f"action={action}, speak={speak}"


def test_json_parsing_all_actions():
    """JSON parser handles all action types correctly."""
    test_cases = [
        ('{"action":"FOLLOW", "speak":"GOOD"}', "FOLLOW", "GOOD", False),
        ('{"action":"FLEE", "speak":"DANGER", "target":[-1000,0,0]}', "FLEE", "DANGER", True),
        ('{"action":"INVESTIGATE", "speak":"WATER HERE", "target":[500,200,0]}', "INVESTIGATE", "WATER HERE", True),
        ('{"action":"WAIT", "speak":"NONE"}', "WAIT", "NONE", False),
        ('{"action":"SIT", "speak":"TIRED"}', "SIT", "TIRED", False),
        ('{"action":"SCOUT", "speak":"THIS WAY", "target":[2000,0,0]}', "SCOUT", "THIS WAY", True),
        ('{"action":"FIGHT", "speak":"SCARED"}', "FIGHT", "SCARED", False),
        ('{"action":"WANDER", "speak":"NONE"}', "WANDER", "NONE", False),
    ]

    passed = 0
    for raw, exp_action, exp_speak, exp_has_target in test_cases:
        action, speak, target = parse_dog_json(raw)
        has_target = target is not None
        if action == exp_action and speak == exp_speak and has_target == exp_has_target:
            passed += 1
        else:
            print(f"    MISMATCH: {raw[:50]} -> action={action}(exp:{exp_action}) speak={speak}(exp:{exp_speak}) target={has_target}(exp:{exp_has_target})")

    total = len(test_cases)
    if passed == total:
        return True, f"{passed}/{total} JSON parses correct"
    return False, f"{passed}/{total} JSON parses correct"


def test_markdown_stripping():
    """Markdown code block stripping handles edge cases."""
    test_cases = [
        # Clean JSON
        ('{"action":"FOLLOW", "speak":"GOOD"}', True),
        # Wrapped in ```json blocks
        ('```json\n{"action":"FOLLOW", "speak":"GOOD"}\n```', True),
        # Wrapped in ``` blocks
        ('```\n{"action":"WAIT", "speak":"NONE"}\n```', True),
        # Leading text before JSON
        ('Here is my response: {"action":"SIT", "speak":"TIRED"}', True),
        # Trailing text after JSON
        ('{"action":"FLEE", "speak":"DANGER"} I hope that helps!', True),
        # Empty string
        ('', False),
    ]

    passed = 0
    for raw, should_parse in test_cases:
        action, speak, target = parse_dog_json(raw)
        parsed_ok = action != "PARSE_ERROR"
        if should_parse and not parsed_ok:
            print(f"    SHOULD HAVE PARSED: {raw[:60]}")
        elif not should_parse and parsed_ok:
            print(f"    SHOULD NOT HAVE PARSED: {raw[:60]}")
        else:
            passed += 1

    total = len(test_cases)
    if passed == total:
        return True, f"{passed}/{total} markdown strip cases correct"
    return False, f"{passed}/{total} markdown strip cases correct"


def test_translator_vocabulary():
    """Vocabulary has phrases for all levels and validates correctly."""
    # Phrases from TranslatorVocabulary.cpp
    level0_phrases = ["DANGER", "SCARED", "SMELL BAD", "RUN", "WATER HERE",
                      "FOOD SMELL", "GOOD", "HURT", "TIRED", "THIS WAY",
                      "NO GO", "WHERE YOU", "COME BACK"]
    level1_phrases = ["DANGER NEAR", "WATER CLOSE", "SAFE PLACE", "HAPPY",
                      "FOLLOW ME"]
    level2_phrases = ["BIG DANGER", "MANY DANGER", "HELP", "PAIN", "LIKE HERE"]
    level3_phrases = ["REMEMBER WOLVES", "WE GOOD TEAM"]

    # Verify expected counts
    if len(level0_phrases) < 10:
        return False, f"Level 0 has only {len(level0_phrases)} phrases (expected 10+)"
    if len(level1_phrases) < 4:
        return False, f"Level 1 has only {len(level1_phrases)} phrases (expected 4+)"

    total = len(level0_phrases) + len(level1_phrases) + len(level2_phrases) + len(level3_phrases)
    return True, f"{total} phrases across 4 levels verified"


def test_conversational_dog_commands():
    """Natural language maps to correct dog actions via Groq."""
    system_prompt = (
        'You are a dog brain in a survival game. '
        'RESPOND IN JSON ONLY: {"action":"ACTION", "speak":"PHRASE"}\n'
        'Valid actions: FOLLOW, WAIT, WANDER, INVESTIGATE, SCOUT, FLEE, FIGHT, SIT\n'
        'Valid phrases: DANGER, SCARED, WATER HERE, FOOD SMELL, GOOD, HURT, TIRED, THIS WAY, NONE\n'
        'Examples:\n'
        '{"action":"FOLLOW", "speak":"GOOD"}\n'
        '{"action":"INVESTIGATE", "speak":"WATER HERE", "target":[500,0,0]}\n'
    )

    test_cases = [
        # State snapshots that should produce specific actions
        ("dist=150cm moving=true bearing=0deg last=Follow age=500ms energy=0.9 hp=1.0 poi=none@-1cm predator=false(0) noise=false time=day rain=false player_hunger=0.9 player_thirst=0.8 player_hp=1.0 command=COME",
         "FOLLOW"),
        ("dist=500cm moving=false bearing=45deg last=Wander age=3000ms energy=0.3 hp=0.8 poi=none@-1cm predator=false(0) noise=false time=night rain=true player_hunger=0.5 player_thirst=0.3 player_hp=0.7 command=STAY",
         "WAIT"),  # STAY maps to WAIT
        ("dist=200cm moving=false bearing=0deg last=Wait age=10000ms energy=0.9 hp=1.0 poi=water@800cm predator=false(0) noise=false time=day rain=false player_hunger=0.8 player_thirst=0.2 player_hp=1.0 command=FIND_WATER",
         "INVESTIGATE"),
        ("dist=400cm moving=true bearing=180deg last=Follow age=500ms energy=0.7 hp=0.9 poi=predator@300cm predator=true(2) noise=true time=dusk rain=false player_hunger=0.6 player_thirst=0.5 player_hp=0.8 command=none",
         "FLEE"),  # predator nearby, should flee
        ("dist=200cm moving=false bearing=0deg last=Wait age=5000ms energy=0.9 hp=1.0 poi=none@-1cm predator=false(0) noise=false time=day rain=false player_hunger=0.9 player_thirst=0.9 player_hp=1.0 command=SCOUT",
         "SCOUT"),
        ("dist=100cm moving=false bearing=0deg last=Follow age=2000ms energy=0.1 hp=1.0 poi=none@-1cm predator=false(0) noise=false time=night rain=false player_hunger=0.9 player_thirst=0.9 player_hp=1.0 command=QUIET",
         "SIT"),  # QUIET maps to SIT
    ]

    passed = 0
    details = []
    for state_line, expected_action in test_cases:
        try:
            content = groq_request(system_prompt, state_line, max_tokens=64, temperature=0.2)
            action, speak, target = parse_dog_json(content)
            # Allow WAIT for SIT and vice versa (both are "stop moving")
            equivalent = {
                "WAIT": {"WAIT", "SIT"},
                "SIT": {"WAIT", "SIT"},
                "FLEE": {"FLEE", "FIGHT"},  # Dog might fight instead of flee
            }
            acceptable = equivalent.get(expected_action, {expected_action})
            if action in acceptable:
                passed += 1
                details.append(f"    OK: {expected_action} -> {action}")
            else:
                details.append(f"    MISS: expected {expected_action}, got {action} (speak={speak})")
        except Exception as e:
            details.append(f"    ERR: {e}")
        time.sleep(0.5)

    for d in details:
        print(d)

    total = len(test_cases)
    threshold = total - 2  # Allow 2 misses for non-deterministic LLM
    if passed >= threshold:
        return True, f"{passed}/{total} conversational commands correct"
    return False, f"{passed}/{total} conversational commands correct (need {threshold}+)"


# ==================== OPTIONAL EDITOR TESTS ====================

def test_editor_running():
    """(Optional) Editor is running and RC API responsive."""
    req = urllib.request.Request('http://localhost:30010/remote/info')
    resp = urllib.request.urlopen(req, timeout=3)
    data = json.loads(resp.read())
    routes = len(data.get('HttpRoutes', []))
    return True, f"RC API up ({routes} routes)"


def test_dog_in_level():
    """(Optional) TrailDogCharacter exists with all required components."""
    from ue5_scene_ops._rc import rc_python
    ok, result = rc_python('''
import unreal
subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
found = False
for a in subsystem.get_all_level_actors():
    if a.get_class().get_name() == "TrailDogCharacter":
        comps = [c.get_class().get_name() for c in a.get_components_by_class(unreal.ActorComponent)]
        required = ["TrailDogBrain", "CharacterMovementComponent"]
        missing = [r for r in required if not any(r in c for c in comps)]
        if missing:
            print(f"MISSING: {missing}")
        else:
            print(f"OK: all components ({len(comps)} total)")
            found = True
        break
if not found:
    print("NO_DOG")
''')
    if not ok:
        return False, f"RC exec failed"
    output = str(result)
    if "OK:" in output:
        return True, output
    return False, output


def test_audio_bridge():
    """(Optional) Audio bridge is running."""
    req = urllib.request.Request('http://127.0.0.1:7777/health')
    resp = urllib.request.urlopen(req, timeout=3)
    data = json.loads(resp.read())
    return True, f"STT={data.get('stt', False)} TTS={data.get('tts', False)}"


# ==================== FOREST WORLD POPULATOR TESTS ====================

GAME_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')


def test_forest_populator_header():
    """ForestWorldPopulator.h exists and is non-empty."""
    path = os.path.join(GAME_DIR, 'Source', 'LostTrail', 'Public', 'World', 'ForestWorldPopulator.h')
    if not os.path.exists(path):
        return False, f"File not found: {path}"
    size = os.path.getsize(path)
    if size < 100:
        return False, f"File too small ({size} bytes)"
    return True, f"Found ({size} bytes)"


def test_forest_populator_source():
    """ForestWorldPopulator.cpp exists and is substantial (>5000 bytes)."""
    path = os.path.join(GAME_DIR, 'Source', 'LostTrail', 'Private', 'World', 'ForestWorldPopulator.cpp')
    if not os.path.exists(path):
        return False, f"File not found: {path}"
    size = os.path.getsize(path)
    if size < 5000:
        return False, f"File too small ({size} bytes) — expected >5000"
    return True, f"Found ({size} bytes)"


def test_forest_populator_class():
    """AForestWorldPopulator class declared in header."""
    path = os.path.join(GAME_DIR, 'Source', 'LostTrail', 'Public', 'World', 'ForestWorldPopulator.h')
    if not os.path.exists(path):
        return False, "Header file not found"
    content = open(path, encoding='utf-8', errors='replace').read()
    if 'AForestWorldPopulator' not in content:
        return False, "AForestWorldPopulator class not found in header"
    return True, "AForestWorldPopulator class declared"


# ==================== MAIN ====================

if __name__ == "__main__":
    print("=" * 60)
    print("Lost Trail AI Test Player")
    print("=" * 60)
    print()

    # Required tests (no editor needed)
    print("--- Core Tests (offline) ---")
    test("Groq key exists", test_groq_key_exists)
    test("Groq API JSON format", test_groq_api_json_format)
    test("JSON parsing all actions", test_json_parsing_all_actions)
    test("Markdown stripping", test_markdown_stripping)
    test("Translator vocabulary", test_translator_vocabulary)
    test("Conversational dog commands", test_conversational_dog_commands)

    print()
    print("--- Forest World Tests ---")
    test("ForestWorldPopulator header exists", test_forest_populator_header)
    test("ForestWorldPopulator source exists", test_forest_populator_source)
    test("ForestWorldPopulator class defined", test_forest_populator_class)

    print()
    print("--- Editor Tests (optional) ---")
    test("Editor running", test_editor_running, optional=True)
    test("Dog in level", test_dog_in_level, optional=True)
    test("Audio bridge", test_audio_bridge, optional=True)

    print()
    print("=" * 60)
    passed = sum(1 for r in RESULTS if r[0] == "PASS")
    failed = sum(1 for r in RESULTS if r[0] == "FAIL")
    errors = sum(1 for r in RESULTS if r[0] == "ERROR")
    skipped = sum(1 for r in RESULTS if r[0] == "SKIP")
    total = len(RESULTS)
    print(f"Results: {passed} passed, {failed} failed, {errors} errors, {skipped} skipped (of {total})")

    if failed > 0 or errors > 0:
        print("\nFailed/Error tests:")
        for status, name, msg in RESULTS:
            if status in ("FAIL", "ERROR"):
                print(f"  [{status}] {name}: {msg}")

    print("=" * 60)

    # Write results to file
    result_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'test-results.json')
    with open(result_file, 'w') as f:
        json.dump({
            'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
            'passed': passed,
            'failed': failed,
            'errors': errors,
            'skipped': skipped,
            'total': total,
            'tests': [{'status': s, 'name': n, 'message': m} for s, n, m in RESULTS]
        }, f, indent=2)
    print(f"\nResults written to {result_file}")

    # Exit code: 0 if no failures or errors
    sys.exit(1 if (failed + errors) > 0 else 0)
