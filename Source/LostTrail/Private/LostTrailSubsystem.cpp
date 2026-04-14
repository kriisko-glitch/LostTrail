// Copyright Kriisko-Studio. Licensed under project terms.

#include "LostTrailSubsystem.h"
#include "Dog/TrailDogCharacter.h"
#include "Dog/DogVoiceComponent.h"
#include "Translator/TranslatorOverlayWidget.h"
#include "Translator/TranslatorComponent.h"
#include "Survival/SurvivalComponent.h"
#include "LostTrail.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULostTrailSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bDogSpawned = false;
	bWidgetCreated = false;
	bComponentsInjected = false;
	bEnterWasDown = false;
	bTabWasDown = false;
	bVKeyWasDown = false;
	InitDelay = 0.5f;
	CachedDog = nullptr;
}

void ULostTrailSubsystem::Deinitialize()
{
	if (OverlayWidget && OverlayWidget->IsInViewport())
	{
		OverlayWidget->RemoveFromParent();
	}
	OverlayWidget = nullptr;
	CachedDog = nullptr;
	Super::Deinitialize();
}

bool ULostTrailSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId ULostTrailSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULostTrailSubsystem, STATGROUP_Tickables);
}

void ULostTrailSubsystem::Tick(float DeltaTime)
{
	// Wait for world to fully initialize
	if (InitDelay > 0.f)
	{
		InitDelay -= DeltaTime;
		return;
	}

	// Ensure player has our components (works with any player pawn, including template BPs)
	if (!bComponentsInjected)
	{
		EnsurePlayerComponents();
	}

	// Auto-spawn dog
	if (!bDogSpawned)
	{
		EnsureDogExists();
		bDogSpawned = true;
	}

	// Create translator overlay widget
	if (!bWidgetCreated)
	{
		UWorld* World = GetWorld();
		if (!World) return;

		APlayerController* PC = World->GetFirstPlayerController();
		if (!PC) return;

		OverlayWidget = CreateWidget<UTranslatorOverlayWidget>(PC, UTranslatorOverlayWidget::StaticClass());
		if (OverlayWidget)
		{
			OverlayWidget->AddToViewport(10);
			bWidgetCreated = true;
			UE_LOG(LogLostTrail, Log, TEXT("Translator overlay widget created"));
		}
		return;
	}

	// Enforce leash
	EnforceLeash();

	// Input polling
	if (!OverlayWidget) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	const bool bChatOpen = OverlayWidget->IsChatVisible();

	// Enter key: open chat when closed
	bool bEnterDown = PC->IsInputKeyDown(EKeys::Enter);
	if (bEnterDown && !bEnterWasDown && !bChatOpen)
	{
		OverlayWidget->ShowChat();
	}
	bEnterWasDown = bEnterDown;

	// Tab key: close chat
	bool bTabDown = PC->IsInputKeyDown(EKeys::Tab);
	if (bTabDown && !bTabWasDown && bChatOpen)
	{
		OverlayWidget->HideChat();
	}
	bTabWasDown = bTabDown;

	// V key: push-to-talk voice (close chat first if open)
	bool bVKeyDown = PC->IsInputKeyDown(EKeys::V);
	if (bVKeyDown && !bVKeyWasDown)
	{
		if (bChatOpen)
		{
			OverlayWidget->HideChat();
		}
		// Start voice recording on the dog
		if (CachedDog)
		{
			UDogVoiceComponent* Voice = CachedDog->FindComponentByClass<UDogVoiceComponent>();
			if (Voice && !Voice->IsRecording())
			{
				Voice->StartVoiceChat();
			}
		}
	}
	bVKeyWasDown = bVKeyDown;
}

void ULostTrailSubsystem::EnsurePlayerComponents()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!Player) return;

	// Add TranslatorComponent if missing (works with any pawn, including template BPs)
	if (!Player->FindComponentByClass<UTranslatorComponent>())
	{
		UTranslatorComponent* Translator = NewObject<UTranslatorComponent>(Player, TEXT("RuntimeTranslator"));
		Translator->RegisterComponent();
		UE_LOG(LogLostTrail, Log, TEXT("Injected TranslatorComponent onto player pawn"));
	}

	// Add SurvivalComponent if missing
	if (!Player->FindComponentByClass<USurvivalComponent>())
	{
		USurvivalComponent* Survival = NewObject<USurvivalComponent>(Player, TEXT("RuntimeSurvival"));
		Survival->RegisterComponent();
		UE_LOG(LogLostTrail, Log, TEXT("Injected SurvivalComponent onto player pawn"));
	}

	bComponentsInjected = true;
}

void ULostTrailSubsystem::EnsureDogExists()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Check if a TrailDogCharacter already exists
	CachedDog = FindDog();
	if (CachedDog)
	{
		UE_LOG(LogLostTrail, Log, TEXT("Dog already exists in level"));
		return;
	}

	// Spawn dog near the player
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn) return;

	FVector SpawnLoc = PlayerPawn->GetActorLocation() + FVector(200.f, 100.f, 0.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CachedDog = World->SpawnActor<ATrailDogCharacter>(
		ATrailDogCharacter::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);

	if (CachedDog)
	{
		// Give the dog a visible mesh (engine sphere, scaled to dog-size)
		UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(CachedDog);
		UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
		if (MeshComp && SphereMesh)
		{
			MeshComp->SetStaticMesh(SphereMesh);
			MeshComp->SetRelativeScale3D(FVector(0.4f, 0.6f, 0.35f)); // Oval dog-like shape
			MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MeshComp->AttachToComponent(CachedDog->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			MeshComp->RegisterComponent();

			// Brown-ish color for a dog
			UMaterialInstanceDynamic* DogMat = MeshComp->CreateDynamicMaterialInstance(0);
			if (DogMat)
			{
				DogMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.55f, 0.35f, 0.15f, 1.0f));
			}
		}

		// Scale the whole character to dog-size
		CachedDog->SetActorScale3D(FVector(0.6f));

		UE_LOG(LogLostTrail, Log, TEXT("Dog auto-spawned near player at %s"), *SpawnLoc.ToString());
	}
	else
	{
		UE_LOG(LogLostTrail, Error, TEXT("Failed to spawn TrailDogCharacter"));
	}
}

void ULostTrailSubsystem::EnforceLeash()
{
	if (!CachedDog || CachedDog->IsDead()) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return;

	const FVector DogLoc = CachedDog->GetActorLocation();
	const FVector PlayerLoc = Player->GetActorLocation();
	const float Distance = FVector::Dist(DogLoc, PlayerLoc);

	// Too far or fell off the map
	if (Distance > LeashDistance || DogLoc.Z < MinZPosition)
	{
		const FVector TeleportTarget = PlayerLoc + FVector(150.f, 0.f, 0.f);
		CachedDog->SetActorLocation(TeleportTarget);
		UE_LOG(LogLostTrail, Log, TEXT("Dog leashed back (dist=%.0f, Z=%.0f)"), Distance, DogLoc.Z);
	}
}

ATrailDogCharacter* ULostTrailSubsystem::FindDog() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (TActorIterator<ATrailDogCharacter> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
