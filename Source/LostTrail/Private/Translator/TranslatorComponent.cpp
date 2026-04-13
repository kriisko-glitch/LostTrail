// Copyright Kriisko-Studio. Licensed under project terms.

#include "Translator/TranslatorComponent.h"
#include "LostTrail.h"

UTranslatorComponent::UTranslatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10 Hz is fine for battery
}

void UTranslatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Recharge battery
	const float Rate = bNearCampfire ? CampfireRechargeRatePerSecond : RechargeRatePerSecond;
	const float OldBattery = CurrentBattery;
	CurrentBattery = FMath::Clamp(CurrentBattery + Rate * DeltaTime, 0.f, MaxBattery);

	if (!FMath::IsNearlyEqual(OldBattery, CurrentBattery, 0.5f))
	{
		OnBatteryChanged.Broadcast(GetBatteryNormalized());
	}

	// Translation cooldown
	TimeSinceLastTranslation += DeltaTime;
}

void UTranslatorComponent::ReceiveDogPhrase(const FString& RawPhrase)
{
	// Cooldown check
	if (TimeSinceLastTranslation < TranslationCooldownSeconds)
	{
		return;
	}

	// Dead battery = no translations
	if (CurrentBattery <= 0.f)
	{
		return;
	}

	// Validate against vocabulary
	const FString Trimmed = RawPhrase.TrimStartAndEnd().ToUpper();
	if (Trimmed.Equals(TEXT("NONE")) || Trimmed.IsEmpty())
	{
		return;
	}

	if (!UTranslatorVocabulary::IsValidPhrase(Trimmed))
	{
		UE_LOG(LogLostTrail, Verbose, TEXT("Translator rejected invalid phrase: '%s'"), *Trimmed);
		return;
	}

	// Check if phrase is unlocked at current level
	const TArray<FTranslatorPhrase>& All = UTranslatorVocabulary::GetAllPhrases();
	ETranslatorCategory Category = ETranslatorCategory::None;
	for (const FTranslatorPhrase& Entry : All)
	{
		if (Entry.Phrase.Equals(Trimmed, ESearchCase::IgnoreCase))
		{
			if (Entry.RequiredLevel > TranslatorLevel)
			{
				// Phrase exists but translator isn't upgraded enough — show garbled
				OnTranslation.Broadcast(TEXT("???"), ETranslatorCategory::None);
				TimeSinceLastTranslation = 0.f;
				return;
			}
			Category = Entry.Category;
			break;
		}
	}

	OnTranslation.Broadcast(Trimmed, Category);
	TimeSinceLastTranslation = 0.f;
}

bool UTranslatorComponent::IssueCommand(EPlayerCommand Command)
{
	if (!CanIssueCommand())
	{
		UE_LOG(LogLostTrail, Warning, TEXT("Cannot issue command — insufficient battery (%.1f)"), CurrentBattery);
		return false;
	}

	CurrentBattery = FMath::Max(0.f, CurrentBattery - CommandBatteryCost);
	OnBatteryChanged.Broadcast(GetBatteryNormalized());
	OnPlayerCommand.Broadcast(Command);

	UE_LOG(LogLostTrail, Log, TEXT("Player issued command: %d, battery now: %.1f"), (int32)Command, CurrentBattery);
	return true;
}

bool UTranslatorComponent::CanIssueCommand() const
{
	return CurrentBattery >= CommandBatteryCost;
}

float UTranslatorComponent::GetBatteryNormalized() const
{
	return MaxBattery > 0.f ? CurrentBattery / MaxBattery : 0.f;
}
