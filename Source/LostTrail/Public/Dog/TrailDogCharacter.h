// Copyright Kriisko-Studio. Licensed under project terms.
//
// ATrailDogCharacter
// ------------------
// The AI dog. Owns a UTrailDogBrain. Same philosophy as DogCompanion:
// the LLM IS the behavior tree. This file maps decisions to movement.
//
// New for LostTrail:
//   - Dog has its own health (can be hurt by predators)
//   - Dog can fight (one wolf at a time, takes damage)
//   - Dog forwards SPEAK phrases to the player's translator
//   - Dog listens for player commands via the translator

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Dog/TrailDogBrain.h"
#include "Dog/DogVoiceComponent.h"
#include "Translator/TranslatorComponent.h"
#include "CombatDamageable.h"
#include "TrailDogCharacter.generated.h"

class AAIController;

UCLASS()
class LOSTTRAIL_API ATrailDogCharacter : public ACharacter, public ICombatDamageable
{
	GENERATED_BODY()

public:
	ATrailDogCharacter();

	// --- Stats ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dog") float MaxHealth = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dog") float Health = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dog") float Energy = 1.f;

	// --- Movement tuning ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dog|Movement") float FollowStopRadius = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dog|Movement") float WanderRadius = 1200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dog|Movement") float ScoutRadius = 3000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dog|Movement") float PoiSearchRadius = 2500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dog|Movement") float FleeDistance = 2000.f;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dog")
	UTrailDogBrain* BrainComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dog")
	UDogVoiceComponent* VoiceComponent;

	// --- Cached references (set in BeginPlay) ---

	/** The player's translator — dog forwards SPEAK phrases here. */
	UPROPERTY(BlueprintReadOnly, Category="Dog")
	UTranslatorComponent* PlayerTranslator;

	// --- ICombatDamageable ---

	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;

	// --- Public API ---

	UFUNCTION(BlueprintCallable, Category="Dog")
	float GetHealthNormalized() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	UFUNCTION(BlueprintCallable, Category="Dog")
	bool IsDead() const { return Health <= 0.f; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	FTrailDogState BuildGameState() const;
	void ExecuteDecision(const FTrailDogDecision& Decision);

	UFUNCTION()
	void HandleBrainDecision(const FTrailDogDecision& Decision);

	UFUNCTION()
	void HandlePlayerCommand(EPlayerCommand Command);

private:
	ETrailDogAction LastAction = ETrailDogAction::Wait;
	double LastActionAtSeconds = 0.0;
	APawn* CachedPlayer = nullptr;
	FString PendingPlayerCommand = TEXT("none");

	AAIController* GetAI() const;
	void FindNearestPoi(FString& OutKind, float& OutDistanceCm) const;
	void FindNearestPredator(bool& bFound, int32& OutCount) const;
	FVector RelativeTargetToWorld(const FVector& RelTarget) const;
};
