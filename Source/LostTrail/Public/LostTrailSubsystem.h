// Copyright Kriisko-Studio. Licensed under project terms.
//
// ULostTrailSubsystem
// -------------------
// World subsystem that bootstraps all LostTrail runtime systems:
//   1. Auto-spawns the dog companion if none exists
//   2. Creates and manages the TranslatorOverlayWidget
//   3. Polls input keys (Enter/Tab for chat, V for voice)
//   4. Leash system: teleports dog back if too far from player
//
// Adapted from NeonPatrolChatSubsystem (proven pattern).
// Works in both PIE and packaged builds (no editor dependency).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LostTrailSubsystem.generated.h"

class ATrailDogCharacter;
class AForestWorldPopulator;
class UTranslatorOverlayWidget;

UCLASS()
class LOSTTRAIL_API ULostTrailSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Max distance before dog gets teleported back to player. */
	UPROPERTY(EditAnywhere, Category="LostTrail")
	float LeashDistance = 2000.f;

	/** Min Z before dog is considered "fell off map". */
	UPROPERTY(EditAnywhere, Category="LostTrail")
	float MinZPosition = -1000.f;

private:
	UPROPERTY()
	UTranslatorOverlayWidget* OverlayWidget = nullptr;

	UPROPERTY()
	ATrailDogCharacter* CachedDog = nullptr;

	bool bDogSpawned = false;
	bool bWidgetCreated = false;
	bool bComponentsInjected = false;
	bool bEnterWasDown = false;
	bool bTabWasDown = false;
	bool bVKeyWasDown = false;
	float InitDelay = 0.5f;

	void EnsurePlayerComponents();
	void EnsureDogExists();
	void EnsureForestExists();
	void EnforceLeash();
	ATrailDogCharacter* FindDog() const;
};
