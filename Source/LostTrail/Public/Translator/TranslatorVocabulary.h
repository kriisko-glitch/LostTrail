// Copyright Kriisko-Studio. Licensed under project terms.
//
// TranslatorVocabulary
// --------------------
// The constrained set of phrases the dog translator can produce.
// The LLM is prompted to pick from this list. The translator component
// validates responses against it — anything not in the list is dropped.
//
// Design intent: the limitation IS the mechanic. Short, broken-feeling
// phrases that a crude device might produce. Players fill the gaps with
// empathy, which makes the dog feel more alive than full sentences would.

#pragma once

#include "CoreMinimal.h"
#include "TranslatorVocabulary.generated.h"

UENUM(BlueprintType)
enum class ETranslatorCategory : uint8
{
	Danger,
	Resource,
	Emotion,
	Navigation,
	Social,
	None
};

USTRUCT(BlueprintType)
struct FTranslatorPhrase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Phrase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ETranslatorCategory Category = ETranslatorCategory::None;

	/** Translator level required to unlock this phrase. 0 = always available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RequiredLevel = 0;
};

/**
 * Static helper that owns the master phrase list.
 * The LLM system prompt includes all unlocked phrases as valid options.
 * The translator component calls IsValidPhrase() to filter LLM output.
 */
UCLASS()
class LOSTTRAIL_API UTranslatorVocabulary : public UObject
{
	GENERATED_BODY()

public:
	/** Returns the full phrase list. Filtering by level is caller's job. */
	static const TArray<FTranslatorPhrase>& GetAllPhrases();

	/** Quick check: is this phrase in the master list? Case-insensitive. */
	static bool IsValidPhrase(const FString& Phrase);

	/** Returns only phrases unlocked at the given translator level. */
	static TArray<FTranslatorPhrase> GetPhrasesForLevel(int32 Level);

	/** Builds the phrase list portion of the LLM system prompt. */
	static FString BuildPromptPhraseList(int32 Level);

private:
	static TArray<FTranslatorPhrase> MasterList;
	static bool bInitialized;
	static void InitializeIfNeeded();
};
