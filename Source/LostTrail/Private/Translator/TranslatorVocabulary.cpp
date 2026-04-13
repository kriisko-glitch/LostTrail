// Copyright Kriisko-Studio. Licensed under project terms.

#include "Translator/TranslatorVocabulary.h"

TArray<FTranslatorPhrase> UTranslatorVocabulary::MasterList;
bool UTranslatorVocabulary::bInitialized = false;

void UTranslatorVocabulary::InitializeIfNeeded()
{
	if (bInitialized) return;
	bInitialized = true;

	// --- Level 0: basic (always available) ---

	// Danger
	MasterList.Add({TEXT("DANGER"),        ETranslatorCategory::Danger, 0});
	MasterList.Add({TEXT("SCARED"),        ETranslatorCategory::Emotion, 0});
	MasterList.Add({TEXT("SMELL BAD"),     ETranslatorCategory::Danger, 0});
	MasterList.Add({TEXT("RUN"),           ETranslatorCategory::Danger, 0});

	// Resources
	MasterList.Add({TEXT("WATER HERE"),    ETranslatorCategory::Resource, 0});
	MasterList.Add({TEXT("FOOD SMELL"),    ETranslatorCategory::Resource, 0});

	// Emotion
	MasterList.Add({TEXT("GOOD"),          ETranslatorCategory::Emotion, 0});
	MasterList.Add({TEXT("HURT"),          ETranslatorCategory::Emotion, 0});
	MasterList.Add({TEXT("TIRED"),         ETranslatorCategory::Emotion, 0});

	// Navigation
	MasterList.Add({TEXT("THIS WAY"),      ETranslatorCategory::Navigation, 0});
	MasterList.Add({TEXT("NO GO"),         ETranslatorCategory::Navigation, 0});

	// Social
	MasterList.Add({TEXT("WHERE YOU"),     ETranslatorCategory::Social, 0});
	MasterList.Add({TEXT("COME BACK"),     ETranslatorCategory::Social, 0});

	// --- Level 1: expanded awareness ---

	MasterList.Add({TEXT("DANGER NEAR"),   ETranslatorCategory::Danger, 1});
	MasterList.Add({TEXT("SOMETHING WRONG"), ETranslatorCategory::Danger, 1});
	MasterList.Add({TEXT("WATER CLOSE"),   ETranslatorCategory::Resource, 1});
	MasterList.Add({TEXT("SAFE PLACE"),    ETranslatorCategory::Navigation, 1});
	MasterList.Add({TEXT("HIDE HERE"),     ETranslatorCategory::Navigation, 1});
	MasterList.Add({TEXT("HAPPY"),         ETranslatorCategory::Emotion, 1});
	MasterList.Add({TEXT("NOT SAFE"),      ETranslatorCategory::Danger, 1});
	MasterList.Add({TEXT("WANT GO"),       ETranslatorCategory::Emotion, 1});
	MasterList.Add({TEXT("FOLLOW ME"),     ETranslatorCategory::Navigation, 1});

	// --- Level 2: detailed communication ---

	MasterList.Add({TEXT("BIG DANGER"),    ETranslatorCategory::Danger, 2});
	MasterList.Add({TEXT("MANY DANGER"),   ETranslatorCategory::Danger, 2});
	MasterList.Add({TEXT("FOOD SMELL GOOD"), ETranslatorCategory::Resource, 2});
	MasterList.Add({TEXT("FOOD SMELL BAD"),  ETranslatorCategory::Resource, 2});
	MasterList.Add({TEXT("DRY SPOT"),      ETranslatorCategory::Navigation, 2});
	MasterList.Add({TEXT("WAY HERE"),      ETranslatorCategory::Navigation, 2});
	MasterList.Add({TEXT("WHAT THIS"),     ETranslatorCategory::Emotion, 2});
	MasterList.Add({TEXT("LOOK LOOK"),     ETranslatorCategory::Emotion, 2});
	MasterList.Add({TEXT("MISS YOU"),      ETranslatorCategory::Social, 2});
	MasterList.Add({TEXT("HELP"),          ETranslatorCategory::Social, 2});
	MasterList.Add({TEXT("PAIN"),          ETranslatorCategory::Emotion, 2});
	MasterList.Add({TEXT("LIKE HERE"),     ETranslatorCategory::Emotion, 2});
	MasterList.Add({TEXT("GOOD THING CLOSE"), ETranslatorCategory::Resource, 2});

	// --- Level 3: memory / meta (cabin only, unlocked late) ---

	MasterList.Add({TEXT("REMEMBER WOLVES"), ETranslatorCategory::Social, 3});
	MasterList.Add({TEXT("MISS FOREST"),     ETranslatorCategory::Social, 3});
	MasterList.Add({TEXT("AGAIN?"),          ETranslatorCategory::Social, 3});
	MasterList.Add({TEXT("WE GOOD TEAM"),    ETranslatorCategory::Social, 3});
	MasterList.Add({TEXT("LAST TIME SCARY"), ETranslatorCategory::Social, 3});
}

const TArray<FTranslatorPhrase>& UTranslatorVocabulary::GetAllPhrases()
{
	InitializeIfNeeded();
	return MasterList;
}

bool UTranslatorVocabulary::IsValidPhrase(const FString& Phrase)
{
	InitializeIfNeeded();
	for (const FTranslatorPhrase& Entry : MasterList)
	{
		if (Entry.Phrase.Equals(Phrase, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

TArray<FTranslatorPhrase> UTranslatorVocabulary::GetPhrasesForLevel(int32 Level)
{
	InitializeIfNeeded();
	TArray<FTranslatorPhrase> Result;
	for (const FTranslatorPhrase& Entry : MasterList)
	{
		if (Entry.RequiredLevel <= Level)
		{
			Result.Add(Entry);
		}
	}
	return Result;
}

FString UTranslatorVocabulary::BuildPromptPhraseList(int32 Level)
{
	TArray<FTranslatorPhrase> Unlocked = GetPhrasesForLevel(Level);
	FString Result = TEXT("Valid phrases: ");
	for (int32 i = 0; i < Unlocked.Num(); ++i)
	{
		if (i > 0) Result += TEXT(", ");
		Result += Unlocked[i].Phrase;
	}
	return Result;
}
