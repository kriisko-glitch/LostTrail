// Copyright Kriisko-Studio. Licensed under project terms.
//
// ACampfireActor
// --------------
// A placeable campfire that provides warmth and translator recharge.
//
// Overlap with the warmth sphere:
//   - Sets bNearFire on USurvivalComponent  (warmth restores)
//   - Sets bNearCampfire on UTranslatorComponent (faster battery recharge)
//
// Components:
//   - USphereComponent (warmth radius, ~500u)
//   - UPointLightComponent (orange glow)
//   - UStaticMeshComponent (cylinder for fire pit visual)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CampfireActor.generated.h"

class USphereComponent;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class LOSTTRAIL_API ACampfireActor : public AActor
{
	GENERATED_BODY()

public:
	ACampfireActor();

	// --- Configuration ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campfire")
	float WarmthRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campfire")
	bool bIsLit = true;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Campfire")
	TObjectPtr<USphereComponent> WarmthSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Campfire")
	TObjectPtr<UPointLightComponent> FireLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Campfire")
	TObjectPtr<UStaticMeshComponent> FirePitMesh;

	// --- Public API ---

	UFUNCTION(BlueprintCallable, Category="Campfire")
	void Extinguish();

	UFUNCTION(BlueprintCallable, Category="Campfire")
	void Ignite();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnWarmthBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnWarmthEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetOverlappingPawnFireState(AActor* PawnActor, bool bNearFire);
};
