// Copyright Kriisko-Studio. Licensed under project terms.

#include "World/CampfireActor.h"
#include "LostTrail.h"

#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Translator/TranslatorComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACampfireActor::ACampfireActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// --- Warmth sphere (root) ---
	WarmthSphere = CreateDefaultSubobject<USphereComponent>(TEXT("WarmthSphere"));
	WarmthSphere->InitSphereRadius(500.f);
	WarmthSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	WarmthSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(WarmthSphere);

	// --- Fire pit mesh ---
	FirePitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirePitMesh"));
	FirePitMesh->SetupAttachment(WarmthSphere);
	FirePitMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.3f)); // Flat cylinder
	FirePitMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	FirePitMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// Load a basic cylinder mesh for the fire pit
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		FirePitMesh->SetStaticMesh(CylinderMesh.Object);
	}

	// --- Point light ---
	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(WarmthSphere);
	FireLight->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	FireLight->Intensity = 3000.f;
	FireLight->LightColor = FColor(255, 140, 40); // warm orange
	FireLight->AttenuationRadius = 600.f;
}

void ACampfireActor::BeginPlay()
{
	Super::BeginPlay();

	// Update sphere radius from config
	WarmthSphere->SetSphereRadius(WarmthRadius);

	// Bind overlap events
	WarmthSphere->OnComponentBeginOverlap.AddDynamic(this, &ACampfireActor::OnWarmthBeginOverlap);
	WarmthSphere->OnComponentEndOverlap.AddDynamic(this, &ACampfireActor::OnWarmthEndOverlap);

	// Apply initial lit state
	if (bIsLit)
	{
		Ignite();
	}
	else
	{
		Extinguish();
	}

	UE_LOG(LogLostTrail, Log, TEXT("Campfire spawned (Lit=%s, Radius=%.0f)"),
		bIsLit ? TEXT("true") : TEXT("false"), WarmthRadius);
}

void ACampfireActor::OnWarmthBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (!bIsLit || !OtherActor)
	{
		return;
	}

	SetOverlappingPawnFireState(OtherActor, true);
}

void ACampfireActor::OnWarmthEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	if (!OtherActor)
	{
		return;
	}

	SetOverlappingPawnFireState(OtherActor, false);
}

void ACampfireActor::SetOverlappingPawnFireState(AActor* PawnActor, bool bNearFire)
{
	if (!PawnActor)
	{
		return;
	}

	// Set bNearFire on SurvivalComponent
	if (USurvivalComponent* Survival = PawnActor->FindComponentByClass<USurvivalComponent>())
	{
		Survival->bNearFire = bNearFire;
		UE_LOG(LogLostTrail, Verbose, TEXT("Campfire: %s bNearFire=%s on %s"),
			bNearFire ? TEXT("set") : TEXT("cleared"),
			bNearFire ? TEXT("true") : TEXT("false"),
			*PawnActor->GetName());
	}

	// Set bNearCampfire on TranslatorComponent
	if (UTranslatorComponent* Translator = PawnActor->FindComponentByClass<UTranslatorComponent>())
	{
		Translator->bNearCampfire = bNearFire;
	}
}

void ACampfireActor::Extinguish()
{
	bIsLit = false;

	if (FireLight)
	{
		FireLight->SetVisibility(false);
	}

	// Clear fire state on any currently overlapping actors
	TArray<AActor*> OverlappingActors;
	WarmthSphere->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		SetOverlappingPawnFireState(Actor, false);
	}

	UE_LOG(LogLostTrail, Log, TEXT("Campfire extinguished: %s"), *GetName());
}

void ACampfireActor::Ignite()
{
	bIsLit = true;

	if (FireLight)
	{
		FireLight->SetVisibility(true);
	}

	// Set fire state on any currently overlapping actors
	TArray<AActor*> OverlappingActors;
	WarmthSphere->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		SetOverlappingPawnFireState(Actor, true);
	}

	UE_LOG(LogLostTrail, Log, TEXT("Campfire ignited: %s"), *GetName());
}
