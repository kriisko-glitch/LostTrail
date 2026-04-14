// Copyright Kriisko-Studio. Licensed under project terms.
//
// USurvivalHUDWidget
// ------------------
// Displays hunger/thirst/warmth bars on screen.
// Built programmatically using RebuildWidget() pattern (NeonPatrol lesson).
// Created by LostTrailSubsystem alongside the TranslatorOverlayWidget.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SurvivalHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UBorder;
class USurvivalComponent;

UCLASS(BlueprintType)
class LOSTTRAIL_API USurvivalHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY() UProgressBar* HungerBar = nullptr;
	UPROPERTY() UProgressBar* ThirstBar = nullptr;
	UPROPERTY() UProgressBar* WarmthBar = nullptr;
	UPROPERTY() UTextBlock* HungerLabel = nullptr;
	UPROPERTY() UTextBlock* ThirstLabel = nullptr;
	UPROPERTY() UTextBlock* WarmthLabel = nullptr;

	UPROPERTY() USurvivalComponent* CachedSurvival = nullptr;

	void FindSurvivalComponent();
	UProgressBar* CreateStatBar(UVerticalBox* Parent, UTextBlock*& OutLabel, const FString& Name,
		const FLinearColor& BarColor);
};
