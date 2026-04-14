// Copyright Kriisko-Studio. Licensed under project terms.

#include "Translator/TranslatorOverlayWidget.h"
#include "Dog/TrailDogBrain.h"
#include "Dog/TrailDogCharacter.h"
#include "Translator/TranslatorComponent.h"
#include "LostTrail.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

TSharedRef<SWidget> UTranslatorOverlayWidget::RebuildWidget()
{
	// Build widget tree BEFORE Slate generation (critical lesson from NeonPatrol)
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;

	// === Translation bubble (floating text above center of screen) ===
	TranslationBubble = WidgetTree->ConstructWidget<UBorder>();
	TranslationBubble->SetBrushColor(FLinearColor(0.05f, 0.15f, 0.05f, 0.9f));
	TranslationBubble->SetPadding(FMargin(16.f, 8.f));

	UCanvasPanelSlot* BubbleSlot = Canvas->AddChildToCanvas(TranslationBubble);
	BubbleSlot->SetAnchors(FAnchors(0.5f, 0.15f, 0.5f, 0.15f));
	BubbleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	BubbleSlot->SetPosition(FVector2D(0.f, 0.f));
	BubbleSlot->SetAutoSize(true);

	TranslationText = WidgetTree->ConstructWidget<UTextBlock>();
	TranslationText->SetText(FText::GetEmpty());
	FSlateFontInfo BubbleFont = TranslationText->GetFont();
	BubbleFont.Size = 22;
	TranslationText->SetFont(BubbleFont);
	TranslationText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 1.0f, 0.3f, 1.0f)));
	TranslationBubble->SetContent(TranslationText);

	// Start hidden
	TranslationBubble->SetVisibility(ESlateVisibility::Collapsed);

	// === Battery bar (top-left corner) ===
	UBorder* BatteryBorder = WidgetTree->ConstructWidget<UBorder>();
	BatteryBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.7f));
	BatteryBorder->SetPadding(FMargin(6.f, 4.f));

	UCanvasPanelSlot* BatterySlot = Canvas->AddChildToCanvas(BatteryBorder);
	BatterySlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
	BatterySlot->SetAlignment(FVector2D(0.f, 0.f));
	BatterySlot->SetPosition(FVector2D(20.f, 20.f));
	BatterySlot->SetSize(FVector2D(160.f, 24.f));
	BatterySlot->SetAutoSize(false);

	UVerticalBox* BattVBox = WidgetTree->ConstructWidget<UVerticalBox>();
	BatteryBorder->SetContent(BattVBox);

	UTextBlock* BattLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BattLabel->SetText(FText::FromString(TEXT("TRANSLATOR")));
	FSlateFontInfo SmallFont = BattLabel->GetFont();
	SmallFont.Size = 10;
	BattLabel->SetFont(SmallFont);
	BattLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.8f, 0.6f, 1.0f)));
	BattVBox->AddChildToVerticalBox(BattLabel);

	BatteryBar = WidgetTree->ConstructWidget<UProgressBar>();
	BatteryBar->SetPercent(1.0f);
	BatteryBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.3f, 1.0f));
	BattVBox->AddChildToVerticalBox(BatteryBar);

	// === Chat panel (bottom-left, same as NeonPatrol) ===
	ChatPanel = WidgetTree->ConstructWidget<UBorder>();
	ChatPanel->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.02f, 0.85f));
	ChatPanel->SetPadding(FMargin(10.f));

	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(ChatPanel);
	PanelSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
	PanelSlot->SetAlignment(FVector2D(0.f, 1.f));
	PanelSlot->SetPosition(FVector2D(20.f, -20.f));
	PanelSlot->SetSize(FVector2D(420.f, 280.f));
	PanelSlot->SetAutoSize(false);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ChatPanel->SetContent(VBox);

	// Chat history
	USizeBox* ScrollSizeBox = WidgetTree->ConstructWidget<USizeBox>();
	ScrollSizeBox->SetHeightOverride(200.f);

	ChatScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	ScrollSizeBox->AddChild(ChatScrollBox);

	UVerticalBoxSlot* ScrollSlot = VBox->AddChildToVerticalBox(ScrollSizeBox);
	ScrollSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	// Input row
	UHorizontalBox* InputRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	VBox->AddChildToVerticalBox(InputRow);

	InputBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	InputBox->SetHintText(FText::FromString(TEXT("Talk to your dog...")));
	UHorizontalBoxSlot* InputSlot = InputRow->AddChildToHorizontalBox(InputBox);
	InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InputSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	SendBtn = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* SendLabel = WidgetTree->ConstructWidget<UTextBlock>();
	SendLabel->SetText(FText::FromString(TEXT("Send")));
	SendBtn->AddChild(SendLabel);
	InputRow->AddChildToHorizontalBox(SendBtn);

	return Super::RebuildWidget();
}

void UTranslatorOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind events (widgets exist from RebuildWidget)
	if (SendBtn)
	{
		SendBtn->OnClicked.AddDynamic(this, &UTranslatorOverlayWidget::OnSendClicked);
	}
	if (InputBox)
	{
		InputBox->OnTextCommitted.AddDynamic(this, &UTranslatorOverlayWidget::OnInputCommitted);
	}

	// Find dog brain and translator
	FindComponents();

	// Start with chat panel hidden
	if (ChatPanel)
	{
		ChatPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	bChatVisible = false;

	AddMessage(TEXT("Dog"), TEXT("[translator online]"), FLinearColor(0.5f, 0.8f, 0.5f, 1.0f));
}

void UTranslatorOverlayWidget::FindComponents()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Find dog brain
	for (TActorIterator<ATrailDogCharacter> It(World); It; ++It)
	{
		BrainRef = It->FindComponentByClass<UTrailDogBrain>();
		if (BrainRef)
		{
			BrainRef->OnDecision.AddDynamic(this, &UTranslatorOverlayWidget::OnDogDecision);
			UE_LOG(LogLostTrail, Log, TEXT("TranslatorOverlay: Found DogBrain"));
			break;
		}
	}

	// Find player's translator component
	APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (Player)
	{
		TranslatorRef = Player->FindComponentByClass<UTranslatorComponent>();
		if (TranslatorRef)
		{
			TranslatorRef->OnTranslation.AddDynamic(this, &UTranslatorOverlayWidget::OnTranslation);
			TranslatorRef->OnBatteryChanged.AddDynamic(this, &UTranslatorOverlayWidget::OnBatteryChanged);
			UE_LOG(LogLostTrail, Log, TEXT("TranslatorOverlay: Found TranslatorComponent"));
		}
	}
}

void UTranslatorOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Auto-hide translation bubble after timer
	if (TranslationTimer > 0.f)
	{
		TranslationTimer -= InDeltaTime;
		if (TranslationTimer <= 0.f && TranslationBubble)
		{
			TranslationBubble->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UTranslatorOverlayWidget::ShowTranslation(const FString& Phrase, ETranslatorCategory Category)
{
	if (!TranslationText || !TranslationBubble) return;

	const FLinearColor Color = GetCategoryColor(Category);
	TranslationText->SetText(FText::FromString(Phrase));
	TranslationText->SetColorAndOpacity(FSlateColor(Color));
	TranslationBubble->SetBrushColor(FLinearColor(Color.R * 0.15f, Color.G * 0.15f, Color.B * 0.15f, 0.9f));
	TranslationBubble->SetVisibility(ESlateVisibility::Visible);
	TranslationTimer = TranslationDisplayTime;

	// Also add to chat history
	AddMessage(TEXT("Dog"), Phrase, Color);
}

FLinearColor UTranslatorOverlayWidget::GetCategoryColor(ETranslatorCategory Category) const
{
	switch (Category)
	{
	case ETranslatorCategory::Danger:     return FLinearColor(1.0f, 0.3f, 0.2f, 1.0f);  // Red
	case ETranslatorCategory::Resource:   return FLinearColor(0.2f, 0.8f, 1.0f, 1.0f);  // Cyan
	case ETranslatorCategory::Emotion:    return FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);  // Gold
	case ETranslatorCategory::Navigation: return FLinearColor(0.2f, 1.0f, 0.4f, 1.0f);  // Green
	case ETranslatorCategory::Social:     return FLinearColor(0.8f, 0.6f, 1.0f, 1.0f);  // Purple
	default:                              return FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);  // Gray
	}
}

void UTranslatorOverlayWidget::ShowChat()
{
	if (!ChatPanel) return;
	ChatPanel->SetVisibility(ESlateVisibility::Visible);
	bChatVisible = true;

	FindComponents();

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameAndUI Mode;
		if (InputBox)
		{
			Mode.SetWidgetToFocus(InputBox->TakeWidget());
		}
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
}

void UTranslatorOverlayWidget::HideChat()
{
	if (!ChatPanel) return;
	ChatPanel->SetVisibility(ESlateVisibility::Collapsed);
	bChatVisible = false;

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void UTranslatorOverlayWidget::OnSendClicked()
{
	if (!InputBox) return;

	FText InputText = InputBox->GetText();
	if (InputText.IsEmpty()) return;

	FString Message = InputText.ToString();
	AddMessage(TEXT("You"), Message, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));

	if (BrainRef)
	{
		// Bind to OnDecision if not already bound (in case dog spawned after widget)
		if (!BrainRef->OnDecision.IsAlreadyBound(this, &UTranslatorOverlayWidget::OnDogDecision))
		{
			BrainRef->OnDecision.AddDynamic(this, &UTranslatorOverlayWidget::OnDogDecision);
		}
		BrainRef->SendChat(Message);
	}
	else
	{
		// Try finding the brain again (dog may have spawned after widget)
		FindComponents();
		if (BrainRef)
		{
			BrainRef->SendChat(Message);
		}
		else
		{
			AddMessage(TEXT("Dog"), TEXT("[dog brain offline]"), FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
		}
	}

	InputBox->SetText(FText::GetEmpty());
	if (InputBox)
	{
		InputBox->SetKeyboardFocus();
	}
}

void UTranslatorOverlayWidget::OnInputCommitted(const FText& Text, ETextCommit::Type Type)
{
	if (Type == ETextCommit::OnEnter)
	{
		OnSendClicked();
	}
}

void UTranslatorOverlayWidget::OnTranslation(const FString& Phrase, ETranslatorCategory Category)
{
	ShowTranslation(Phrase, Category);
}

void UTranslatorOverlayWidget::OnBatteryChanged(float NormalizedBattery)
{
	if (BatteryBar)
	{
		BatteryBar->SetPercent(NormalizedBattery);
		// Color: green > 50%, yellow > 20%, red below
		if (NormalizedBattery > 0.5f)
		{
			BatteryBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.3f, 1.0f));
		}
		else if (NormalizedBattery > 0.2f)
		{
			BatteryBar->SetFillColorAndOpacity(FLinearColor(0.9f, 0.8f, 0.1f, 1.0f));
		}
		else
		{
			BatteryBar->SetFillColorAndOpacity(FLinearColor(0.9f, 0.2f, 0.1f, 1.0f));
		}
	}
}

void UTranslatorOverlayWidget::OnDogDecision(const FTrailDogDecision& Decision)
{
	// Show the dog's speak phrase in the chat log and as floating text
	if (!Decision.SpeakPhrase.IsEmpty()
		&& !Decision.SpeakPhrase.Equals(TEXT("NONE"), ESearchCase::IgnoreCase))
	{
		// Show in chat log
		const FLinearColor DogColor(0.2f, 1.0f, 0.3f, 1.0f);
		AddMessage(TEXT("Dog"), Decision.SpeakPhrase, DogColor);

		// Also show as floating translation bubble
		ShowTranslation(Decision.SpeakPhrase, ETranslatorCategory::None);
	}
}

void UTranslatorOverlayWidget::AddMessage(const FString& Sender, const FString& Message, const FLinearColor& Color)
{
	if (!ChatScrollBox) return;

	UTextBlock* MsgText = NewObject<UTextBlock>(this);
	MsgText->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"), *Sender, *Message)));

	FSlateFontInfo Font = MsgText->GetFont();
	Font.Size = 14;
	MsgText->SetFont(Font);
	MsgText->SetColorAndOpacity(FSlateColor(Color));

	ChatScrollBox->AddChild(MsgText);

	while (ChatScrollBox->GetChildrenCount() > MaxMessages)
	{
		if (UWidget* Oldest = ChatScrollBox->GetChildAt(0))
		{
			ChatScrollBox->RemoveChild(Oldest);
		}
	}

	ChatScrollBox->ScrollToEnd();
}
