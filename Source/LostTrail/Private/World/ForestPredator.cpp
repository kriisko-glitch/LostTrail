// Copyright Kriisko-Studio. Licensed under project terms.

#include "World/ForestPredator.h"
#include "LostTrail.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AForestPredator::AForestPredator()
{
	PrimaryActorTick.bCanEverTick = true;

	// Use AI controller, auto-possessed when placed or spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	// Tag so the dog brain can detect predators
	Tags.Add(FName("predator"));
}

void AForestPredator::BeginPlay()
{
	Super::BeginPlay();

	ApplyPredatorTypeDefaults();
	Health = MaxHealth;

	// Apply configured move speed to the movement component
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = MoveSpeed;
	}

	UE_LOG(LogLostTrail, Log, TEXT("ForestPredator spawned: %s (Health=%.0f, Damage=%.0f)"),
		PredatorType == EPredatorType::Wolf ? TEXT("Wolf") : TEXT("Bear"),
		MaxHealth, AttackDamage);
}

void AForestPredator::ApplyPredatorTypeDefaults()
{
	// Only apply defaults if the designer hasn't customized values away from base
	// The constructor sets Wolf-like defaults; Bear overrides here.
	switch (PredatorType)
	{
	case EPredatorType::Wolf:
		// Wolf: fast, low health, moderate damage (defaults already set)
		if (MaxHealth == 100.f) MaxHealth = 80.f;
		if (AttackDamage == 15.f) AttackDamage = 12.f;
		if (MoveSpeed == 600.f) MoveSpeed = 700.f;
		if (AttackCooldown == 1.5f) AttackCooldown = 1.0f;
		break;

	case EPredatorType::Bear:
		// Bear: slow, high health, high damage
		if (MaxHealth == 100.f) MaxHealth = 250.f;
		if (AttackDamage == 15.f) AttackDamage = 35.f;
		if (MoveSpeed == 600.f) MoveSpeed = 350.f;
		if (AttackCooldown == 1.5f) AttackCooldown = 2.5f;
		if (DetectionRange == 2000.f) DetectionRange = 1500.f;
		break;
	}
}

void AForestPredator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsAlive())
	{
		return;
	}

	// Tick attack cooldown
	if (AttackTimer > 0.f)
	{
		AttackTimer -= DeltaSeconds;
	}

	// Find a target (player or dog)
	AActor* Target = FindClosestTarget();
	CurrentTarget = Target;

	if (!Target)
	{
		return;
	}

	const float DistToTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

	if (DistToTarget <= AttackRange)
	{
		TryAttack(Target);
	}
	else
	{
		MoveTowardTarget(Target);
	}
}

AActor* AForestPredator::FindClosestTarget() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Gather the player pawn and any actors tagged "dog"
	TArray<AActor*> Candidates;

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		Candidates.Add(PlayerPawn);
	}

	// Also consider the dog (tagged "dog")
	TArray<AActor*> DogActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("dog"), DogActors);
	Candidates.Append(DogActors);

	AActor* Closest = nullptr;
	float ClosestDist = DetectionRange;

	for (AActor* Candidate : Candidates)
	{
		if (!Candidate)
		{
			continue;
		}

		const float Dist = FVector::Dist(GetActorLocation(), Candidate->GetActorLocation());
		if (Dist < ClosestDist)
		{
			ClosestDist = Dist;
			Closest = Candidate;
		}
	}

	return Closest;
}

void AForestPredator::MoveTowardTarget(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC)
	{
		return;
	}

	// Refresh the move request each tick — simple but effective for MVP.
	// AcceptanceRadius slightly inside AttackRange so we get close enough to attack.
	AIC->MoveToActor(Target, AttackRange * 0.7f);
}

void AForestPredator::TryAttack(AActor* Target)
{
	if (!Target || AttackTimer > 0.f)
	{
		return;
	}

	AttackTimer = AttackCooldown;

	// Attempt to call TakePredatorDamage on the target if it's another ForestPredator,
	// otherwise look for a generic TakeDamage pathway.
	// For the player/dog: use UGameplayStatics::ApplyDamage which feeds into AActor::TakeDamage.
	UGameplayStatics::ApplyDamage(
		Target,
		AttackDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

	UE_LOG(LogLostTrail, Verbose, TEXT("%s attacked %s for %.0f damage"),
		*GetName(), *Target->GetName(), AttackDamage);
}

void AForestPredator::TakePredatorDamage(float DamageAmount)
{
	if (!IsAlive())
	{
		return;
	}

	Health = FMath::Max(0.f, Health - DamageAmount);

	UE_LOG(LogLostTrail, Log, TEXT("%s took %.0f damage, health=%.0f/%.0f"),
		*GetName(), DamageAmount, Health, MaxHealth);

	if (Health <= 0.f)
	{
		Die();
	}
}

void AForestPredator::Die()
{
	UE_LOG(LogLostTrail, Log, TEXT("%s (%s) died."),
		*GetName(),
		PredatorType == EPredatorType::Wolf ? TEXT("Wolf") : TEXT("Bear"));

	// Stop all AI and ticking
	SetActorTickEnabled(false);

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->StopMovement();
	}

	// Disable collision so it doesn't block paths
	SetActorEnableCollision(false);

	// Could add ragdoll or destroy timer here in a later pass
}
