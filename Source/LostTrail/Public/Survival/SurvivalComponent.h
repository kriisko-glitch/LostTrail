// Copyright Kriisko-Studio. Licensed under project terms.
//
// USurvivalComponent
// ------------------
// Tracks the core survival stats: hunger, thirst, warmth.
// Lives on the player character. Dog has its own simplified version.
//
// Design: stats drain over time. External systems feed them:
//   - Eat berries/meat  → hunger
//   - Drink at stream   → thirst
//   - Stand near fire   → warmth
//   - Shelter during rain → warmth
//
// When a stat depletes, consequences kick in (see GDD section 5).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivalComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChanged, FName, StatName, float, NormalizedValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatDepleted, FName, StatName);

UCLASS(ClassGroup=(LostTrail), meta=(BlueprintSpawnableComponent))
class LOSTTRAIL_API USurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalComponent();

	// --- Max values ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") float MaxHunger = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") float MaxThirst = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") float MaxWarmth = 100.f;

	// --- Drain rates (per second) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival|Drain") float HungerDrainRate = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival|Drain") float ThirstDrainRate = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival|Drain") float WarmthDrainRate = 0.0f; // Only drains at night / in rain

	// --- Current values ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Survival") float Hunger = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Survival") float Thirst = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Survival") float Warmth = 100.f;

	// --- Environmental modifiers (set by overlap volumes / day-night manager) ---

	/** Multiplier on warmth drain. 0 = no drain, 1 = normal, 2 = cold rain. */
	UPROPERTY(BlueprintReadWrite, Category="Survival") float WarmthDrainMultiplier = 0.f;

	/** If true, near a fire — warmth restores instead of draining. */
	UPROPERTY(BlueprintReadWrite, Category="Survival") bool bNearFire = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") float FireWarmthRestoreRate = 5.f;

	/** If true, in shelter — warmth drain is halved. */
	UPROPERTY(BlueprintReadWrite, Category="Survival") bool bInShelter = false;

	// --- Events ---

	UPROPERTY(BlueprintAssignable) FOnStatChanged OnStatChanged;
	UPROPERTY(BlueprintAssignable) FOnStatDepleted OnStatDepleted;

	// --- Public API ---

	UFUNCTION(BlueprintCallable, Category="Survival") void AddHunger(float Amount);
	UFUNCTION(BlueprintCallable, Category="Survival") void AddThirst(float Amount);
	UFUNCTION(BlueprintCallable, Category="Survival") void AddWarmth(float Amount);

	UFUNCTION(BlueprintCallable, Category="Survival") float GetHungerNormalized() const;
	UFUNCTION(BlueprintCallable, Category="Survival") float GetThirstNormalized() const;
	UFUNCTION(BlueprintCallable, Category="Survival") float GetWarmthNormalized() const;

	/** True if any stat is at zero. */
	UFUNCTION(BlueprintCallable, Category="Survival") bool IsAnyStatDepleted() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bHungerDepleted = false;
	bool bThirstDepleted = false;
	bool bWarmthDepleted = false;

	void DrainStat(float& Current, float Max, float Rate, float DeltaTime, FName Name, bool& bWasDepleted);
};
