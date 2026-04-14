// Copyright Kriisko-Studio. Licensed under project terms.

#include "World/ForestWorldPopulator.h"
#include "World/CampfireActor.h"
#include "World/ForageableItem.h"
#include "World/ForestPredator.h"
#include "LostTrail.h"

#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

AForestWorldPopulator::AForestWorldPopulator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AForestWorldPopulator::BeginPlay()
{
	Super::BeginPlay();
	LoadMeshAssets();
	PopulateForest();
}

static UStaticMesh* TryLoad(const TCHAR* Path)
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path);
	if (!Mesh)
	{
		UE_LOG(LogLostTrail, Warning, TEXT("ForestPopulator: Failed to load %s"), Path);
	}
	return Mesh;
}

void AForestWorldPopulator::LoadMeshAssets()
{
	// --- Trees (Kenney Nature Kit — imported FBX, flat in /Game/KenneyNature/) ---
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_default")))            TreeMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_detailed")))           TreeMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_oak")))                TreeMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_tall")))               TreeMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_thin")))               TreeMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_fat")))                TreeMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_pineDefaultA")))       TreeMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_pineTallA_detailed"))) TreeMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_pineRoundA")))         TreeMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/tree_cone")))               TreeMeshes.Add(M);

	// --- Bushes ---
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/plant_bush")))              BushMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/plant_bushLarge")))         BushMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/plant_bushSmall")))         BushMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/plant_bushDetailed")))      BushMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/grass_large")))             BushMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/grass_leafs")))             BushMeshes.Add(M);

	// --- Rocks ---
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/rock_largeA")))             RockMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/rock_largeB")))             RockMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/rock_largeC")))             RockMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/rock_tallA")))              RockMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/rock_smallA")))             RockMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/rock_smallB")))             RockMeshes.Add(M);
	if (UStaticMesh* M = TryLoad(TEXT("/Game/KenneyNature/stone_largeA")))            RockMeshes.Add(M);

	UE_LOG(LogLostTrail, Log, TEXT("ForestPopulator: Loaded %d trees, %d bushes, %d rocks (Kenney Nature Kit)"),
		TreeMeshes.Num(), BushMeshes.Num(), RockMeshes.Num());
}

void AForestWorldPopulator::PopulateForest()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector Center = GetActorLocation();
	APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (Player)
	{
		Center = Player->GetActorLocation();
	}
	// Ground surface is at Z=0 in this level; player floats above it
	GroundZ = 0.f;

	FRandomStream Rng(RandomSeed);

	// OBJ models from Kenney are Y-up, UE5 is Z-up — rotate +90 pitch to stand them upright
	const float ObjPitchFix = 90.f;

	// --- Spawn trees ---
	// At 200x scale a tree is player-height (~180cm), so 600-1000x gives 5-9m trees
	if (TreeMeshes.Num() > 0)
	{
		for (int32 i = 0; i < TreeCount; ++i)
		{
			FVector Loc = RandomPointInForestExcludeClearing(Rng, Center);
			UStaticMesh* Mesh = TreeMeshes[Rng.RandRange(0, TreeMeshes.Num() - 1)];

			FRotator Rot(ObjPitchFix, Rng.FRandRange(0.f, 360.f), 0.f);
			float ScaleFactor = Rng.FRandRange(600.f, 1000.f);
			SpawnMeshActor(Mesh, Loc, Rot, FVector(ScaleFactor));
		}
		UE_LOG(LogLostTrail, Log, TEXT("ForestPopulator: Spawned %d trees"), TreeCount);
	}

	// --- Spawn bushes ---
	if (BushMeshes.Num() > 0)
	{
		for (int32 i = 0; i < BushCount; ++i)
		{
			FVector Loc = RandomPointInForestExcludeClearing(Rng, Center);
			UStaticMesh* Mesh = BushMeshes[Rng.RandRange(0, BushMeshes.Num() - 1)];

			FRotator Rot(ObjPitchFix, Rng.FRandRange(0.f, 360.f), 0.f);
			float ScaleFactor = Rng.FRandRange(200.f, 400.f);
			SpawnMeshActor(Mesh, Loc, Rot, FVector(ScaleFactor));
		}
		UE_LOG(LogLostTrail, Log, TEXT("ForestPopulator: Spawned %d bushes"), BushCount);
	}

	// --- Spawn rocks ---
	if (RockMeshes.Num() > 0)
	{
		for (int32 i = 0; i < RockCount; ++i)
		{
			FVector Loc = RandomPointInForest(Rng);
			UStaticMesh* Mesh = RockMeshes[Rng.RandRange(0, RockMeshes.Num() - 1)];

			FRotator Rot(ObjPitchFix + Rng.FRandRange(-5.f, 5.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-5.f, 5.f));
			float ScaleFactor = Rng.FRandRange(200.f, 500.f);
			SpawnMeshActor(Mesh, Loc, Rot, FVector(ScaleFactor));
		}
		UE_LOG(LogLostTrail, Log, TEXT("ForestPopulator: Spawned %d rocks"), RockCount);
	}

	// --- Spawn gameplay items ---
	SpawnForageableItems(Rng, Center);
	SpawnCampfires(Rng, Center);
	SpawnPredators(Rng, Center);

	UE_LOG(LogLostTrail, Log, TEXT("ForestPopulator: World population complete (GroundZ=%.0f)"), GroundZ);
}

FVector AForestWorldPopulator::RandomPointInForest(FRandomStream& Rng) const
{
	const float Angle = Rng.FRandRange(0.f, 2.f * PI);
	const float Dist = Rng.FRandRange(0.f, ForestRadius);
	FVector Point = GetActorLocation() + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
	Point.Z = GroundZ;

	// Line trace down to snap to actual terrain
	UWorld* World = GetWorld();
	if (World)
	{
		FHitResult Hit;
		FVector TraceStart = Point + FVector(0.f, 0.f, 2000.f);
		FVector TraceEnd = Point - FVector(0.f, 0.f, 5000.f);
		FCollisionQueryParams Params;
		Params.bTraceComplex = false;
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
		{
			Point.Z = Hit.ImpactPoint.Z;
		}
	}

	return Point;
}

FVector AForestWorldPopulator::RandomPointInForestExcludeClearing(FRandomStream& Rng, const FVector& Center) const
{
	for (int32 Attempt = 0; Attempt < 20; ++Attempt)
	{
		FVector Point = RandomPointInForest(Rng);
		if (FVector::Dist2D(Point, Center) > ClearingRadius)
		{
			return Point;
		}
	}
	const float Angle = Rng.FRandRange(0.f, 2.f * PI);
	FVector Fallback = Center + FVector(FMath::Cos(Angle) * ClearingRadius * 1.1f, FMath::Sin(Angle) * ClearingRadius * 1.1f, 0.f);
	Fallback.Z = GroundZ;
	return Fallback;
}

void AForestWorldPopulator::SpawnStaticMeshActor(UStaticMesh* Mesh, const FVector& Location,
	const FRotator& Rotation, const FVector& Scale)
{
	SpawnMeshActor(Mesh, Location, Rotation, Scale);
}

void AForestWorldPopulator::SpawnMeshActor(UStaticMesh* Mesh, const FVector& Location,
	const FRotator& Rotation, const FVector& Scale)
{
	UWorld* World = GetWorld();
	if (!World || !Mesh) return;

	AActor* MeshActor = World->SpawnActor<AActor>(AActor::StaticClass(), Location,
		Rotation, FActorSpawnParameters());
	if (!MeshActor) return;

	USceneComponent* Root = NewObject<USceneComponent>(MeshActor, TEXT("Root"));
	Root->RegisterComponent();
	MeshActor->SetRootComponent(Root);

	UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(MeshActor);
	MeshComp->SetStaticMesh(Mesh);
	MeshComp->SetRelativeScale3D(Scale);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComp->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	MeshComp->RegisterComponent();
}

void AForestWorldPopulator::SpawnColoredMesh(UStaticMesh* Mesh, const FVector& Location,
	const FVector& Scale, const FRotator& Rotation, const FLinearColor& Color)
{
	// Legacy — kept for interface
	SpawnMeshActor(Mesh, Location, Rotation, Scale);
}

void AForestWorldPopulator::SpawnForageableItems(FRandomStream& Rng, const FVector& Center)
{
	UWorld* World = GetWorld();
	if (!World) return;

	auto SpawnForageable = [&](int32 Count, EForageableType Type)
	{
		for (int32 i = 0; i < Count; ++i)
		{
			FVector Loc = RandomPointInForest(Rng);
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AForageableItem* Item = World->SpawnActor<AForageableItem>(
				AForageableItem::StaticClass(), Loc, FRotator::ZeroRotator, Params);
			if (Item)
			{
				Item->ForageableType = Type;
			}
		}
	};

	SpawnForageable(BerryCount, EForageableType::Berries);
	SpawnForageable(MushroomCount, EForageableType::Mushrooms);
	SpawnForageable(WaterSourceCount, EForageableType::WaterSource);

	UE_LOG(LogLostTrail, Log, TEXT("ForestPopulator: Spawned %d berries, %d mushrooms, %d water sources"),
		BerryCount, MushroomCount, WaterSourceCount);
}

void AForestWorldPopulator::SpawnCampfires(FRandomStream& Rng, const FVector& Center)
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (int32 i = 0; i < CampfireCount; ++i)
	{
		const float Angle = Rng.FRandRange(0.f, 2.f * PI);
		const float Dist = Rng.FRandRange(ForestRadius * 0.2f, ForestRadius * 0.6f);
		FVector Loc = Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
		Loc.Z = GroundZ;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ACampfireActor* Fire = World->SpawnActor<ACampfireActor>(
			ACampfireActor::StaticClass(), Loc, FRotator::ZeroRotator, Params);
		if (Fire)
		{
			if (i > 0)
			{
				Fire->bIsLit = false;
			}
		}
	}

	UE_LOG(LogLostTrail, Log, TEXT("ForestPopulator: Spawned %d campfires"), CampfireCount);
}

void AForestWorldPopulator::SpawnPredators(FRandomStream& Rng, const FVector& Center)
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (int32 i = 0; i < WolfCount; ++i)
	{
		const float Angle = Rng.FRandRange(0.f, 2.f * PI);
		const float Dist = Rng.FRandRange(ForestRadius * 0.5f, ForestRadius * 0.9f);
		FVector Loc = Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
		Loc.Z = GroundZ;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AForestPredator* Wolf = World->SpawnActor<AForestPredator>(
			AForestPredator::StaticClass(), Loc, FRotator::ZeroRotator, Params);
		if (Wolf)
		{
			Wolf->PredatorType = EPredatorType::Wolf;
		}
	}

	UE_LOG(LogLostTrail, Log, TEXT("ForestPopulator: Spawned %d wolves"), WolfCount);
}
