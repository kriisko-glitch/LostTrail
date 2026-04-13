// Copyright Kriisko-Studio. Licensed under project terms.
//
// UTranslatorOverlayWidget
// ------------------------
// UMG widget built programmatically using RebuildWidget() pattern.
// Shows: translator text (dog phrases), battery indicator, chat input panel.
// Adapted from NeonPatrol's ChatOverlayWidget.
//
// IMPORTANT: Widget tree is built in RebuildWidget(), NOT NativeConstruct().
// NativeConstruct() fires AFTER Slate widget is generated -- too late for WidgetTree.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Translator/TranslatorVocabulary.h"
#include "TranslatorOverlayWidget.generated.h"

class UScrollBox;
class UEditableTextBox;
class UButton;
class UTextBlock;
class UCanvasPanel;
class UBorder;
class UProgressBar;
class UTrailDogBrain;
class UTranslatorComponent;

UCLASS(BlueprintType)
class LOSTTRAIL_API UTranslatorOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Translator")
	bool IsChatVisible() const { return bChatVisible; }

	UFUNCTION(BlueprintCallable, Category = "Translator")
	void ShowChat();

	UFUNCTION(BlueprintCallable, Category = "Translator")
	void HideChat();

	UFUNCTION(BlueprintCallable, Category = "Translator")
	void ShowTranslation(const FString& Phrase, ETranslatorCategory Category);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Translator")
	float TranslationDisplayTime = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Translator")
	int32 MaxMessages = 30;

private:
	// UI elements
	UPROPERTY() UBorder* ChatPanel = nullptr;
	UPROPERTY() UScrollBox* ChatScrollBox = nullptr;
	UPROPERTY() UEditableTextBox* InputBox = nullptr;
	UPROPERTY() UButton* SendBtn = nullptr;
	UPROPERTY() UTextBlock* TranslationText = nullptr;
	UPROPERTY() UBorder* TranslationBubble = nullptr;
	UPROPERTY() UProgressBar* BatteryBar = nullptr;

	// Cached references
	UPROPERTY() UTrailDogBrain* BrainRef = nullptr;
	UPROPERTY() UTranslatorComponent* TranslatorRef = nullptr;

	bool bChatVisible = false;
	float TranslationTimer = 0.f;

	void FindComponents();
	void AddMessage(const FString& Sender, const FString& Message, const FLinearColor& Color);

	UFUNCTION() void OnSendClicked();
	UFUNCTION() void OnInputCommitted(const FText& Text, ETextCommit::Type Type);
	UFUNCTION() void OnTranslation(const FString& Phrase, ETranslatorCategory Category);
	UFUNCTION() void OnBatteryChanged(float NormalizedBattery);
	UFUNCTION() void OnDogDecision(const FTrailDogDecision& Decision);

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	FLinearColor GetCategoryColor(ETranslatorCategory Category) const;
};
