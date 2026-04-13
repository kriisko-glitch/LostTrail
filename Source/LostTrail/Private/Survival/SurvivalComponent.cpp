// Copyright Kriisko-Studio. Licensed under project terms.

#include "Survival/SurvivalComponent.h"

USurvivalComponent::USurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // 2 Hz — survival stats don't need 60fps updates
}

void USurvivalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Hunger and thirst always drain
	DrainStat(Hunger, MaxHunger, HungerDrainRate, DeltaTime, FName("Hunger"), bHungerDepleted);
	DrainStat(Thirst, MaxThirst, ThirstDrainRate, DeltaTime, FName("Thirst"), bThirstDepleted);

	// Warmth: near fire = restore, in shelter = half drain, otherwise use multiplier
	if (bNearFire)
	{
		const float Old = Warmth;
		Warmth = FMath::Clamp(Warmth + FireWarmthRestoreRate * DeltaTime, 0.f, MaxWarmth);
		if (!FMath::IsNearlyEqual(Old, Warmth, 0.5f))
		{
			OnStatChanged.Broadcast(FName("Warmth"), GetWarmthNormalized());
		}
		bWarmthDepleted = false;
	}
	else
	{
		float EffectiveRate = WarmthDrainRate * WarmthDrainMultiplier;
		if (bInShelter) EffectiveRate *= 0.5f;
		DrainStat(Warmth, MaxWarmth, EffectiveRate, DeltaTime, FName("Warmth"), bWarmthDepleted);
	}
}

void USurvivalComponent::DrainStat(float& Current, float Max, float Rate, float DeltaTime, FName Name, bool& bWasDepleted)
{
	if (Rate <= 0.f) return;

	const float Old = Current;
	Current = FMath::Max(0.f, Current - Rate * DeltaTime);

	if (!FMath::IsNearlyEqual(Old, Current, 0.5f))
	{
		OnStatChanged.Broadcast(Name, Max > 0.f ? Current / Max : 0.f);
	}

	if (Current <= 0.f && !bWasDepleted)
	{
		bWasDepleted = true;
		OnStatDepleted.Broadcast(Name);
	}
	else if (Current > 0.f)
	{
		bWasDepleted = false;
	}
}

void USurvivalComponent::AddHunger(float Amount)
{
	Hunger = FMath::Clamp(Hunger + Amount, 0.f, MaxHunger);
	OnStatChanged.Broadcast(FName("Hunger"), GetHungerNormalized());
	if (Hunger > 0.f) bHungerDepleted = false;
}

void USurvivalComponent::AddThirst(float Amount)
{
	Thirst = FMath::Clamp(Thirst + Amount, 0.f, MaxThirst);
	OnStatChanged.Broadcast(FName("Thirst"), GetThirstNormalized());
	if (Thirst > 0.f) bThirstDepleted = false;
}

void USurvivalComponent::AddWarmth(float Amount)
{
	Warmth = FMath::Clamp(Warmth + Amount, 0.f, MaxWarmth);
	OnStatChanged.Broadcast(FName("Warmth"), GetWarmthNormalized());
	if (Warmth > 0.f) bWarmthDepleted = false;
}

float USurvivalComponent::GetHungerNormalized() const { return MaxHunger > 0.f ? Hunger / MaxHunger : 0.f; }
float USurvivalComponent::GetThirstNormalized() const { return MaxThirst > 0.f ? Thirst / MaxThirst : 0.f; }
float USurvivalComponent::GetWarmthNormalized() const { return MaxWarmth > 0.f ? Warmth / MaxWarmth : 0.f; }

bool USurvivalComponent::IsAnyStatDepleted() const
{
	return Hunger <= 0.f || Thirst <= 0.f || Warmth <= 0.f;
}
