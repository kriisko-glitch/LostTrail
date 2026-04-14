// Copyright Kriisko-Studio. Licensed under project terms.

#include "UI/SurvivalHUDWidget.h"
#include "Survival/SurvivalComponent.h"
#include "LostTrail.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Kismet/GameplayStatics.h"

TSharedRef<SWidget> USurvivalHUDWidget::RebuildWidget()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;

	// Container: top-right corner
	UBorder* Container = WidgetTree->ConstructWidget<UBorder>();
	Container->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.7f));
	Container->SetPadding(FMargin(10.f, 8.f));

	UCanvasPanelSlot* ContainerSlot = Canvas->AddChildToCanvas(Container);
	ContainerSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	ContainerSlot->SetAlignment(FVector2D(1.f, 0.f));
	ContainerSlot->SetPosition(FVector2D(-20.f, 20.f));
	ContainerSlot->SetSize(FVector2D(180.f, 120.f));
	ContainerSlot->SetAutoSize(false);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Container->SetContent(VBox);

	// Three stat bars
	HungerBar = CreateStatBar(VBox, HungerLabel, TEXT("HUNGER"), FLinearColor(0.9f, 0.6f, 0.1f, 1.0f));
	ThirstBar = CreateStatBar(VBox, ThirstLabel, TEXT("THIRST"), FLinearColor(0.2f, 0.6f, 1.0f, 1.0f));
	WarmthBar = CreateStatBar(VBox, WarmthLabel, TEXT("WARMTH"), FLinearColor(1.0f, 0.3f, 0.1f, 1.0f));

	return Super::RebuildWidget();
}

UProgressBar* USurvivalHUDWidget::CreateStatBar(UVerticalBox* Parent, UTextBlock*& OutLabel,
	const FString& Name, const FLinearColor& BarColor)
{
	// Label
	OutLabel = WidgetTree->ConstructWidget<UTextBlock>();
	OutLabel->SetText(FText::FromString(Name));
	FSlateFontInfo Font = OutLabel->GetFont();
	Font.Size = 10;
	OutLabel->SetFont(Font);
	OutLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f)));

	UVerticalBoxSlot* LabelSlot = Parent->AddChildToVerticalBox(OutLabel);
	LabelSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

	// Bar
	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>();
	Bar->SetPercent(1.0f);
	Bar->SetFillColorAndOpacity(BarColor);

	UVerticalBoxSlot* BarSlot = Parent->AddChildToVerticalBox(Bar);
	BarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	return Bar;
}

void USurvivalHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	FindSurvivalComponent();
}

void USurvivalHUDWidget::FindSurvivalComponent()
{
	if (CachedSurvival) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (Player)
	{
		CachedSurvival = Player->FindComponentByClass<USurvivalComponent>();
		if (CachedSurvival)
		{
			UE_LOG(LogLostTrail, Log, TEXT("SurvivalHUD: Found SurvivalComponent"));
		}
	}
}

void USurvivalHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CachedSurvival)
	{
		FindSurvivalComponent();
		if (!CachedSurvival) return;
	}

	// Update bars
	if (HungerBar)
	{
		float Val = CachedSurvival->GetHungerNormalized();
		HungerBar->SetPercent(Val);
		HungerBar->SetFillColorAndOpacity(Val > 0.3f
			? FLinearColor(0.9f, 0.6f, 0.1f, 1.0f)
			: FLinearColor(1.0f, 0.1f, 0.1f, 1.0f));
	}
	if (ThirstBar)
	{
		float Val = CachedSurvival->GetThirstNormalized();
		ThirstBar->SetPercent(Val);
		ThirstBar->SetFillColorAndOpacity(Val > 0.3f
			? FLinearColor(0.2f, 0.6f, 1.0f, 1.0f)
			: FLinearColor(1.0f, 0.1f, 0.1f, 1.0f));
	}
	if (WarmthBar)
	{
		float Val = CachedSurvival->GetWarmthNormalized();
		WarmthBar->SetPercent(Val);
		WarmthBar->SetFillColorAndOpacity(Val > 0.3f
			? FLinearColor(1.0f, 0.3f, 0.1f, 1.0f)
			: FLinearColor(0.5f, 0.0f, 0.5f, 1.0f));
	}
}
