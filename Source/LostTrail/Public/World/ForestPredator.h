// Copyright Kriisko-Studio. Licensed under project terms.
//
// AForestPredator
// ---------------
// A wolf or bear predator that roams the forest.
// Simple Tick-based AI: detect player/dog -> move toward -> attack when in range.
// Uses AIController + MoveToActor for nav mesh navigation.
//
// NOT derived from ACombatCharacter (abstract, cross-module).
// Has its own TakePredatorDamage() instead of ICombatDamageable.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ForestPredator.generated.h"

UENUM(BlueprintType)
enum class EPredatorType : uint8
{
	Wolf  UMETA(DisplayName = "Wolf"),
	Bear  UMETA(DisplayName = "Bear")
};

UCLASS()
class LOSTTRAIL_API AForestPredator : public ACharacter
{
	GENERATED_BODY()

public:
	AForestPredator();

	// --- Configuration ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Predator")
	EPredatorType PredatorType = EPredatorType::Wolf;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Predator|Stats")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Predator|Stats")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Predator|Stats")
	float AttackDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Predator|Stats")
	float AttackRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Predator|Stats")
	float DetectionRange = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Predator|Stats")
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Predator|Stats")
	float AttackCooldown = 1.5f;

	// --- Public API ---

	/** Receive damage from external systems (player weapon, dog bite, etc.). */
	UFUNCTION(BlueprintCallable, Category="Predator")
	void TakePredatorDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Predator")
	bool IsAlive() const { return Health > 0.f; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float AttackTimer = 0.f;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	void ApplyPredatorTypeDefaults();
	AActor* FindClosestTarget() const;
	void MoveTowardTarget(AActor* Target);
	void TryAttack(AActor* Target);
	void Die();
};
