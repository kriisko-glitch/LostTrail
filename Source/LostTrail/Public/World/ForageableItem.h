// Copyright Kriisko-Studio. Licensed under project terms.
//
// AForageableItem
// ---------------
// A pickup item for survival: berries, mushrooms, water sources, firewood.
// On overlap with the player pawn, applies effects to USurvivalComponent.
//
// Types:
//   Berries:     AddHunger(25)
//   Mushrooms:   AddHunger(15), 20% chance poisonous (drain 30 hunger)
//   WaterSource: AddThirst(40), persistent (reusable every 10s)
//   Firewood:    No direct effect (future: campfire fuel)
//
// Tags: "poi.food" (Berries/Mushrooms), "poi.water", "poi.shelter" (Firewood)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForageableItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EForageableType : uint8
{
	Berries     UMETA(DisplayName = "Berries"),
	Mushrooms   UMETA(DisplayName = "Mushrooms"),
	WaterSource UMETA(DisplayName = "Water Source"),
	Firewood    UMETA(DisplayName = "Firewood")
};

UCLASS()
class LOSTTRAIL_API AForageableItem : public AActor
{
	GENERATED_BODY()

public:
	AForageableItem();

	// --- Configuration ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forageable")
	EForageableType ForageableType = EForageableType::Berries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forageable")
	float InteractionRadius = 150.f;

	/** Cooldown between uses for persistent items (WaterSource). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forageable")
	float ReuseCooldown = 10.f;

	/** Chance (0..1) that mushrooms are poisonous. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forageable")
	float PoisonChance = 0.2f;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Forageable")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Forageable")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

protected:
	virtual void BeginPlay() override;

private:
	bool bOnCooldown = false;
	FTimerHandle CooldownTimerHandle;

	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	void ApplyForageEffect(AActor* PlayerActor);
	void ApplyTagForType();
	void ResetCooldown();
};
