// Copyright Kriisko-Studio. Licensed under project terms.

#include "Dog/TrailDogCharacter.h"
#include "Survival/SurvivalComponent.h"
#include "LostTrail.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ATrailDogCharacter::ATrailDogCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	BrainComponent = CreateDefaultSubobject<UTrailDogBrain>(TEXT("DogBrain"));

	// Don't auto-possess — let the GameMode assign an AI controller
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ATrailDogCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Cache player reference
	CachedPlayer = UGameplayStatics::GetPlayerPawn(this, 0);

	// Find the player's translator component
	if (CachedPlayer)
	{
		PlayerTranslator = CachedPlayer->FindComponentByClass<UTranslatorComponent>();
		if (PlayerTranslator)
		{
			// Listen for player commands
			PlayerTranslator->OnPlayerCommand.AddDynamic(this, &ATrailDogCharacter::HandlePlayerCommand);
		}
	}

	// Listen for brain decisions
	if (BrainComponent)
	{
		BrainComponent->OnDecision.AddDynamic(this, &ATrailDogCharacter::HandleBrainDecision);
	}

	LastActionAtSeconds = GetWorld()->GetTimeSeconds();
}

void ATrailDogCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsDead()) return;

	// Energy: drains while moving, refills while sitting
	const float Speed = GetVelocity().Size();
	if (Speed > 10.f)
	{
		Energy = FMath::Max(0.f, Energy - 0.02f * DeltaSeconds);
	}
	else if (LastAction == ETrailDogAction::Sit)
	{
		Energy = FMath::Min(1.f, Energy + 0.1f * DeltaSeconds);
	}

	// Build state and request a brain decision (brain handles its own poll interval)
	if (BrainComponent)
	{
		BrainComponent->RequestDecision(BuildGameState());
	}
}

FTrailDogState ATrailDogCharacter::BuildGameState() const
{
	FTrailDogState State;

	if (CachedPlayer)
	{
		const FVector DogLoc = GetActorLocation();
		const FVector PlayerLoc = CachedPlayer->GetActorLocation();
		const FVector ToPlayer = PlayerLoc - DogLoc;

		State.DistanceToPlayerCm = ToPlayer.Size();
		State.bPlayerIsMoving = CachedPlayer->GetVelocity().Size() > 10.f;

		// Bearing: angle from dog forward to player direction
		const FVector DogForward = GetActorForwardVector();
		const FVector ToPlayerFlat = FVector(ToPlayer.X, ToPlayer.Y, 0.f).GetSafeNormal();
		State.PlayerBearingDeg = FMath::RadiansToDegrees(
			FMath::Atan2(
				FVector::CrossProduct(DogForward, ToPlayerFlat).Z,
				FVector::DotProduct(DogForward, ToPlayerFlat)
			)
		);

		// Player survival stats
		if (USurvivalComponent* Survival = CachedPlayer->FindComponentByClass<USurvivalComponent>())
		{
			State.PlayerHungerNormalized = Survival->GetHungerNormalized();
			State.PlayerThirstNormalized = Survival->GetThirstNormalized();
			State.PlayerHealthNormalized = 1.f; // TODO: wire to player health
		}
	}

	State.LastAction = LastAction;
	State.LastActionAgeMs = (int32)((GetWorld()->GetTimeSeconds() - LastActionAtSeconds) * 1000.0);
	State.DogEnergy = Energy;
	State.DogHealthNormalized = GetHealthNormalized();

	// POIs
	FindNearestPoi(State.NearestPoiKind, State.NearestPoiDistanceCm);

	// Predators
	FindNearestPredator(State.bPredatorNearby, State.PredatorCount);

	// Player command
	State.PlayerCommand = PendingPlayerCommand;

	// TODO: wire TimeOfDay and bIsRaining from DayNightManager

	return State;
}

void ATrailDogCharacter::HandleBrainDecision(const FTrailDogDecision& Decision)
{
	ExecuteDecision(Decision);

	// Forward SPEAK phrase to translator
	if (PlayerTranslator && !Decision.SpeakPhrase.IsEmpty()
		&& !Decision.SpeakPhrase.Equals(TEXT("NONE"), ESearchCase::IgnoreCase))
	{
		PlayerTranslator->ReceiveDogPhrase(Decision.SpeakPhrase);
	}

	// Clear the pending player command after it's been consumed
	PendingPlayerCommand = TEXT("none");
}

void ATrailDogCharacter::HandlePlayerCommand(EPlayerCommand Command)
{
	switch (Command)
	{
	case EPlayerCommand::Come:        PendingPlayerCommand = TEXT("COME"); break;
	case EPlayerCommand::Stay:        PendingPlayerCommand = TEXT("STAY"); break;
	case EPlayerCommand::Scout:       PendingPlayerCommand = TEXT("SCOUT"); break;
	case EPlayerCommand::FindWater:   PendingPlayerCommand = TEXT("FIND_WATER"); break;
	case EPlayerCommand::FindShelter: PendingPlayerCommand = TEXT("FIND_SHELTER"); break;
	case EPlayerCommand::Quiet:       PendingPlayerCommand = TEXT("QUIET"); break;
	}

	UE_LOG(LogLostTrail, Log, TEXT("Dog received player command: %s"), *PendingPlayerCommand);
}

void ATrailDogCharacter::ExecuteDecision(const FTrailDogDecision& Decision)
{
	LastAction = Decision.Action;
	LastActionAtSeconds = GetWorld()->GetTimeSeconds();

	AAIController* AI = GetAI();
	if (!AI) return;

	switch (Decision.Action)
	{
	case ETrailDogAction::Follow:
		if (CachedPlayer)
		{
			AI->MoveToActor(CachedPlayer, FollowStopRadius);
		}
		break;

	case ETrailDogAction::Wait:
	case ETrailDogAction::Sit:
		AI->StopMovement();
		// TODO: play sit animation for Sit
		break;

	case ETrailDogAction::Wander:
	{
		FVector WanderTarget = GetActorLocation() + FMath::VRand() * WanderRadius;
		WanderTarget.Z = GetActorLocation().Z;
		if (Decision.bHasTarget)
		{
			WanderTarget = RelativeTargetToWorld(Decision.Target);
		}
		AI->MoveToLocation(WanderTarget);
		break;
	}

	case ETrailDogAction::Investigate:
	{
		FVector InvestTarget = Decision.bHasTarget
			? RelativeTargetToWorld(Decision.Target)
			: GetActorLocation() + GetActorForwardVector() * 500.f;
		AI->MoveToLocation(InvestTarget);
		// TODO: play sniff animation on arrival
		break;
	}

	case ETrailDogAction::Scout:
	{
		// Move ahead of the player in their facing direction
		if (CachedPlayer)
		{
			FVector ScoutTarget = CachedPlayer->GetActorLocation()
				+ CachedPlayer->GetActorForwardVector() * ScoutRadius;
			if (Decision.bHasTarget)
			{
				ScoutTarget = RelativeTargetToWorld(Decision.Target);
			}
			AI->MoveToLocation(ScoutTarget);
		}
		break;
	}

	case ETrailDogAction::Flee:
	{
		// Move away from the nearest predator (or just backwards)
		FVector FleeDir = -GetActorForwardVector();
		if (Decision.bHasTarget)
		{
			FleeDir = (Decision.Target - GetActorLocation()).GetSafeNormal();
		}
		AI->MoveToLocation(GetActorLocation() + FleeDir * FleeDistance);
		break;
	}

	case ETrailDogAction::Fight:
		AI->StopMovement();
		// TODO: attack nearest predator using combat interfaces
		break;

	default:
		break;
	}
}

// --- ICombatDamageable ---

void ATrailDogCharacter::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	Health = FMath::Max(0.f, Health - Damage);
	UE_LOG(LogLostTrail, Log, TEXT("Dog took %.1f damage, health now %.1f"), Damage, Health);

	if (Health <= 0.f)
	{
		HandleDeath();
	}
}

void ATrailDogCharacter::HandleDeath()
{
	UE_LOG(LogLostTrail, Warning, TEXT("Dog has died!"));
	// TODO: ragdoll, disable brain, allow player to carry
}

void ATrailDogCharacter::ApplyHealing(float Healing, AActor* Healer)
{
	Health = FMath::Clamp(Health + Healing, 0.f, MaxHealth);
}

// --- Helpers ---

AAIController* ATrailDogCharacter::GetAI() const
{
	return Cast<AAIController>(GetController());
}

void ATrailDogCharacter::FindNearestPoi(FString& OutKind, float& OutDistanceCm) const
{
	OutKind = TEXT("none");
	OutDistanceCm = -1.f;

	float BestDist = PoiSearchRadius;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor->Tags.Num()) continue;

		for (const FName& Tag : Actor->Tags)
		{
			FString TagStr = Tag.ToString();
			if (TagStr.StartsWith(TEXT("poi.")))
			{
				const float Dist = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());
				if (Dist < BestDist)
				{
					BestDist = Dist;
					OutKind = TagStr.Mid(4); // strip "poi."
					OutDistanceCm = Dist;
				}
			}
		}
	}
}

void ATrailDogCharacter::FindNearestPredator(bool& bFound, int32& OutCount) const
{
	bFound = false;
	OutCount = 0;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->ActorHasTag(FName("predator")))
		{
			const float Dist = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());
			if (Dist < PoiSearchRadius)
			{
				bFound = true;
				OutCount++;
			}
		}
	}
}

FVector ATrailDogCharacter::RelativeTargetToWorld(const FVector& RelTarget) const
{
	// RelTarget.X = forward distance, Y = right distance from dog
	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	return GetActorLocation() + Forward * RelTarget.X + Right * RelTarget.Y + FVector(0, 0, RelTarget.Z);
}
