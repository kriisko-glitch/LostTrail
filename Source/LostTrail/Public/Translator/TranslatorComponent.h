// Copyright Kriisko-Studio. Licensed under project terms.
//
// UTranslatorComponent
// --------------------
// The "dog translator" device. Lives on the player character.
// Two-way bridge between the dog's LLM brain and the player:
//
//   Dog → Player:  Receives FDogDecision with a SPEAK phrase, validates it
//                  against TranslatorVocabulary, broadcasts OnTranslation.
//                  UI listens and shows the floating text.
//
//   Player → Dog:  Player issues a command (COME, STAY, SCOUT, etc.),
//                  which costs battery. The command is forwarded to the
//                  dog brain as a forced override of its next decision.
//
// Battery: drains on player commands, recharges over time (faster at campfires).
// Level: determines which vocabulary phrases are available. Persisted in save.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TranslatorVocabulary.h"
#include "TranslatorComponent.generated.h"

UENUM(BlueprintType)
enum class EPlayerCommand : uint8
{
	Come        UMETA(DisplayName = "COME"),
	Stay        UMETA(DisplayName = "STAY"),
	Scout       UMETA(DisplayName = "SCOUT"),
	FindWater   UMETA(DisplayName = "FIND WATER"),
	FindShelter UMETA(DisplayName = "FIND SHELTER"),
	Quiet       UMETA(DisplayName = "QUIET"),
};

/** Fired when the translator produces a valid phrase from the dog. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTranslation,
	const FString&, Phrase,
	ETranslatorCategory, Category
);

/** Fired when battery changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBatteryChanged, float, NormalizedBattery);

/** Fired when a player command is issued. Dog brain component listens. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerCommand, EPlayerCommand, Command);

UCLASS(ClassGroup=(LostTrail), meta=(BlueprintSpawnableComponent))
class LOSTTRAIL_API UTranslatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTranslatorComponent();

	// --- Configuration ---

	/** Current translator upgrade level (0 = base, 3 = max). Persisted in save. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Translator")
	int32 TranslatorLevel = 0;

	/** Max battery capacity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Translator|Battery")
	float MaxBattery = 100.f;

	/** Current battery. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Translator|Battery")
	float CurrentBattery = 100.f;

	/** Battery cost per player command. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Translator|Battery")
	float CommandBatteryCost = 20.f;

	/** Battery recharge rate per second (normal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Translator|Battery")
	float RechargeRatePerSecond = 1.5f;

	/** Battery recharge rate per second (at campfire). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Translator|Battery")
	float CampfireRechargeRatePerSecond = 8.f;

	/** If true, we're near a campfire and recharging faster. Set by overlap. */
	UPROPERTY(BlueprintReadWrite, Category="Translator|Battery")
	bool bNearCampfire = false;

	/** Minimum time between translations to avoid spam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Translator")
	float TranslationCooldownSeconds = 3.f;

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category="Translator")
	FOnTranslation OnTranslation;

	UPROPERTY(BlueprintAssignable, Category="Translator")
	FOnBatteryChanged OnBatteryChanged;

	UPROPERTY(BlueprintAssignable, Category="Translator")
	FOnPlayerCommand OnPlayerCommand;

	// --- Public API ---

	/** Called by dog brain when it produces a SPEAK phrase. Validates and broadcasts. */
	UFUNCTION(BlueprintCallable, Category="Translator")
	void ReceiveDogPhrase(const FString& RawPhrase);

	/** Called by player input. Drains battery and broadcasts command to dog. */
	UFUNCTION(BlueprintCallable, Category="Translator")
	bool IssueCommand(EPlayerCommand Command);

	/** True if there's enough battery for a command. */
	UFUNCTION(BlueprintCallable, Category="Translator")
	bool CanIssueCommand() const;

	/** Normalized battery 0..1 for UI. */
	UFUNCTION(BlueprintCallable, Category="Translator")
	float GetBatteryNormalized() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float TimeSinceLastTranslation = 0.f;
};
