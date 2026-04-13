// Copyright Kriisko-Studio. Licensed under project terms.

#include "World/DayNightManager.h"
#include "LostTrail.h"

ADayNightManager::ADayNightManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADayNightManager::BeginPlay()
{
	Super::BeginPlay();

	CurrentPhase = ETimePhase::Dawn;
	CurrentDay = 1;
	PhaseElapsed = 0.f;
	TotalElapsed = 0.f;

	OnPhaseChanged.Broadcast(CurrentPhase, CurrentDay);
	ScheduleRain();

	UE_LOG(LogLostTrail, Log, TEXT("DayNight: Day %d begins — Dawn"), CurrentDay);
}

void ADayNightManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PhaseElapsed += DeltaSeconds;
	TotalElapsed += DeltaSeconds;

	const float Duration = GetCurrentPhaseDuration();
	PhaseProgress = Duration > 0.f ? FMath::Clamp(PhaseElapsed / Duration, 0.f, 1.f) : 1.f;

	// Rain timer
	if (bIsRaining)
	{
		RainTimer += DeltaSeconds;
		if (RainTimer >= RainDuration)
		{
			bIsRaining = false;
			OnRainStopped.Broadcast();
			UE_LOG(LogLostTrail, Log, TEXT("DayNight: Rain stopped"));
		}
	}
	else if (bRainScheduledThisDay && TotalElapsed >= RainStartTime && RainDuration > 0.f)
	{
		bIsRaining = true;
		RainTimer = 0.f;
		OnRainStarted.Broadcast();
		UE_LOG(LogLostTrail, Log, TEXT("DayNight: Rain started (%.0fs duration)"), RainDuration);
		bRainScheduledThisDay = false; // Don't re-trigger
	}

	// Phase transition
	if (PhaseElapsed >= Duration)
	{
		AdvancePhase();
	}
}

float ADayNightManager::GetCurrentPhaseDuration() const
{
	switch (CurrentPhase)
	{
	case ETimePhase::Dawn:  return DawnDuration;
	case ETimePhase::Day:   return DayDuration;
	case ETimePhase::Dusk:  return DuskDuration;
	case ETimePhase::Night: return NightDuration;
	default: return DayDuration;
	}
}

void ADayNightManager::AdvancePhase()
{
	PhaseElapsed = 0.f;

	switch (CurrentPhase)
	{
	case ETimePhase::Dawn:
		CurrentPhase = ETimePhase::Day;
		break;
	case ETimePhase::Day:
		CurrentPhase = ETimePhase::Dusk;
		break;
	case ETimePhase::Dusk:
		CurrentPhase = ETimePhase::Night;
		break;
	case ETimePhase::Night:
		CurrentDay++;
		if (CurrentDay > MaxDays)
		{
			UE_LOG(LogLostTrail, Warning, TEXT("DayNight: Day %d exceeds max — run should have ended"), CurrentDay);
			// GameMode handles the actual game-over
		}
		CurrentPhase = ETimePhase::Dawn;
		ScheduleRain();
		break;
	}

	OnPhaseChanged.Broadcast(CurrentPhase, CurrentDay);
	UE_LOG(LogLostTrail, Log, TEXT("DayNight: Day %d — %s"), CurrentDay, *GetTimeOfDayString());
}

void ADayNightManager::ScheduleRain()
{
	bRainScheduledThisDay = false;
	RainDuration = 0.f;

	if (FMath::FRand() < RainChance)
	{
		bRainScheduledThisDay = true;
		RainDuration = FMath::RandRange(MinRainDuration, MaxRainDuration);
		// Rain starts at a random point during the day phase
		RainStartTime = TotalElapsed + DawnDuration + FMath::RandRange(0.f, DayDuration * 0.7f);
		UE_LOG(LogLostTrail, Log, TEXT("DayNight: Rain scheduled for Day %d (%.0fs duration, starts at %.0fs)"),
			CurrentDay, RainDuration, RainStartTime);
	}
}

FString ADayNightManager::GetTimeOfDayString() const
{
	switch (CurrentPhase)
	{
	case ETimePhase::Dawn:  return TEXT("dawn");
	case ETimePhase::Day:   return TEXT("day");
	case ETimePhase::Dusk:  return TEXT("dusk");
	case ETimePhase::Night: return TEXT("night");
	default: return TEXT("day");
	}
}
