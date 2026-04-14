// Copyright Kriisko-Studio. Licensed under project terms.
//
// AForestWorldPopulator
// ---------------------
// Spawns a forest around the player using real meshes from the migrated
// EpicSurvivalGame environment pack. Place one in the level or let the
// subsystem spawn it automatically.
//
// Uses a seeded random scatter: trees, bushes, rocks, forageable items,
// campfires, and predators — all within a configurable radius.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestWorldPopulator.generated.h"

class UStaticMesh;

UCLASS()
class LOSTTRAIL_API AForestWorldPopulator : public AActor
{
	GENERATED_BODY()

public:
	AForestWorldPopulator();

	// --- Forest config ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest")
	float ForestRadius = 15000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest|Trees")
	int32 TreeCount = 120;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest|Trees")
	int32 BushCount = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest|Rocks")
	int32 RockCount = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest|Items")
	int32 BerryCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest|Items")
	int32 MushroomCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest|Items")
	int32 WaterSourceCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest|Items")
	int32 CampfireCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest|Enemies")
	int32 WolfCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest")
	int32 RandomSeed = 42;

	/** Z height to place objects on (ground level). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest")
	float GroundZ = 0.f;

	/** Clear zone around player spawn — no trees here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Forest")
	float ClearingRadius = 800.f;

protected:
	virtual void BeginPlay() override;

private:
	// Loaded mesh references
	TArray<UStaticMesh*> TreeMeshes;
	TArray<UStaticMesh*> BushMeshes;
	TArray<UStaticMesh*> RockMeshes;

	void LoadMeshAssets();
	void PopulateForest();

	FVector RandomPointInForest(FRandomStream& Rng) const;
	FVector RandomPointInForestExcludeClearing(FRandomStream& Rng, const FVector& Center) const;

	void SpawnStaticMeshActor(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale);
	void SpawnMeshActor(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale);
	void SpawnColoredMesh(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FRotator& Rotation, const FLinearColor& Color);
	void SpawnForageableItems(FRandomStream& Rng, const FVector& Center);
	void SpawnCampfires(FRandomStream& Rng, const FVector& Center);
	void SpawnPredators(FRandomStream& Rng, const FVector& Center);
};
