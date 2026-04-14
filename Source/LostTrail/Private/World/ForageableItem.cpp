// Copyright Kriisko-Studio. Licensed under project terms.

#include "World/ForageableItem.h"
#include "LostTrail.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AForageableItem::AForageableItem()
{
	PrimaryActorTick.bCanEverTick = false;

	// --- Interaction sphere (root) ---
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->InitSphereRadius(150.f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(InteractionSphere);

	// --- Visible mesh ---
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(InteractionSphere);
	ItemMesh->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.15f)); // Small sphere
	ItemMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// Load a basic sphere mesh for the visual
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		ItemMesh->SetStaticMesh(SphereMesh.Object);
	}
}

void AForageableItem::BeginPlay()
{
	Super::BeginPlay();

	// Apply interaction radius from config
	InteractionSphere->SetSphereRadius(InteractionRadius);

	// Apply tag based on type
	ApplyTagForType();

	// Bind overlap event
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AForageableItem::OnInteractionBeginOverlap);

	UE_LOG(LogLostTrail, Log, TEXT("ForageableItem spawned: %s (%s)"),
		*GetName(),
		*UEnum::GetValueAsString(ForageableType));
}

void AForageableItem::ApplyTagForType()
{
	switch (ForageableType)
	{
	case EForageableType::Berries:
	case EForageableType::Mushrooms:
		Tags.Add(FName("poi.food"));
		break;
	case EForageableType::WaterSource:
		Tags.Add(FName("poi.water"));
		break;
	case EForageableType::Firewood:
		Tags.Add(FName("poi.shelter"));
		break;
	}
}

void AForageableItem::OnInteractionBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (!OtherActor)
	{
		return;
	}

	// Only respond to the player pawn
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (OtherActor != PlayerPawn)
	{
		return;
	}

	// Check cooldown for persistent items
	if (bOnCooldown)
	{
		UE_LOG(LogLostTrail, Verbose, TEXT("ForageableItem %s on cooldown, ignoring overlap"),
			*GetName());
		return;
	}

	ApplyForageEffect(OtherActor);
}

void AForageableItem::ApplyForageEffect(AActor* PlayerActor)
{
	USurvivalComponent* Survival = PlayerActor->FindComponentByClass<USurvivalComponent>();
	if (!Survival)
	{
		UE_LOG(LogLostTrail, Warning,
			TEXT("ForageableItem: player %s has no SurvivalComponent"), *PlayerActor->GetName());
		return;
	}

	switch (ForageableType)
	{
	case EForageableType::Berries:
		Survival->AddHunger(25.f);
		UE_LOG(LogLostTrail, Log, TEXT("Player ate berries (+25 hunger)"));
		Destroy();
		break;

	case EForageableType::Mushrooms:
	{
		const float Roll = FMath::FRand();
		if (Roll < PoisonChance)
		{
			// Poisonous mushroom — drain hunger
			Survival->AddHunger(-30.f);
			UE_LOG(LogLostTrail, Log, TEXT("Player ate POISONOUS mushroom (-30 hunger)"));
		}
		else
		{
			Survival->AddHunger(15.f);
			UE_LOG(LogLostTrail, Log, TEXT("Player ate mushroom (+15 hunger)"));
		}
		Destroy();
		break;
	}

	case EForageableType::WaterSource:
		Survival->AddThirst(40.f);
		UE_LOG(LogLostTrail, Log, TEXT("Player drank water (+40 thirst)"));

		// Water source is persistent — start cooldown instead of destroying
		bOnCooldown = true;
		GetWorldTimerManager().SetTimer(
			CooldownTimerHandle,
			this,
			&AForageableItem::ResetCooldown,
			ReuseCooldown,
			false);
		break;

	case EForageableType::Firewood:
		// No direct survival effect for MVP — future: needed to light campfires
		UE_LOG(LogLostTrail, Log, TEXT("Player picked up firewood"));
		Destroy();
		break;
	}
}

void AForageableItem::ResetCooldown()
{
	bOnCooldown = false;
	UE_LOG(LogLostTrail, Verbose, TEXT("ForageableItem %s cooldown reset"), *GetName());
}
