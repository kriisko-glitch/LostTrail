// Copyright Kriisko-Studio. Licensed under project terms.
//
// ADayNightManager
// ----------------
// Controls the day/night cycle. A single instance lives in the level.
//
// Timeline per run:
//   Day 1: Dawn(1min) → Day(4min) → Dusk(1min) → Night(4min) = ~10min real
//   Day 2: same
//   Day 3: same, but night = game over if still in forest
//
// Broadcasts phase changes so other systems can react:
//   - Predators become active at Dusk
//   - Warmth drains at Night / during Rain
//   - Dog brain receives TimeOfDay in its state snapshot

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightManager.generated.h"

UENUM(BlueprintType)
enum class ETimePhase : uint8
{
	Dawn,
	Day,
	Dusk,
	Night
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPhaseChanged, ETimePhase, NewPhase, int32, DayNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRainStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRainStopped);

UCLASS()
class LOSTTRAIL_API ADayNightManager : public AActor
{
	GENERATED_BODY()

public:
	ADayNightManager();

	// --- Configuration ---

	/** Duration of each phase in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DayNight") float DawnDuration = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DayNight") float DayDuration = 240.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DayNight") float DuskDuration = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DayNight") float NightDuration = 240.f;

	/** How many days until the run ends (player must escape by then). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DayNight") int32 MaxDays = 3;

	/** Chance of rain per day (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DayNight|Weather") float RainChance = 0.3f;

	/** Min/max duration of rain in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DayNight|Weather") float MinRainDuration = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DayNight|Weather") float MaxRainDuration = 120.f;

	// --- State ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DayNight") ETimePhase CurrentPhase = ETimePhase::Dawn;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DayNight") int32 CurrentDay = 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DayNight") bool bIsRaining = false;

	/** Normalized progress through the current phase (0..1). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DayNight") float PhaseProgress = 0.f;

	// --- Events ---

	UPROPERTY(BlueprintAssignable) FOnPhaseChanged OnPhaseChanged;
	UPROPERTY(BlueprintAssignable) FOnRainStarted OnRainStarted;
	UPROPERTY(BlueprintAssignable) FOnRainStopped OnRainStopped;

	// --- Public API ---

	UFUNCTION(BlueprintCallable, Category="DayNight")
	FString GetTimeOfDayString() const;

	/** Total elapsed game time in seconds. */
	UFUNCTION(BlueprintCallable, Category="DayNight")
	float GetTotalElapsedSeconds() const { return TotalElapsed; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float PhaseElapsed = 0.f;
	float TotalElapsed = 0.f;

	// Rain state
	float RainTimer = 0.f;
	float RainDuration = 0.f;
	bool bRainScheduledThisDay = false;
	float RainStartTime = 0.f;

	float GetCurrentPhaseDuration() const;
	void AdvancePhase();
	void ScheduleRain();
};
