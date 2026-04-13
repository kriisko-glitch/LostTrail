// Copyright Kriisko-Studio. Licensed under project terms.
//
// UDogVoiceComponent
// ------------------
// Voice chat with the dog via audio_bridge.py.
// Push-to-talk (V key): record → STT → send to DogBrain → TTS response.
// Adapted from NeonPatrol's SparkVoiceComponent (proven pattern).
//
// IMPORTANT: Only LLM responses use TTS (lesson from NeonPatrol).
// Translator phrases are visual only (floating text), NOT spoken.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "DogVoiceComponent.generated.h"

class UTrailDogBrain;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDogVoiceTranscribed, FString, TranscribedText);

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class LOSTTRAIL_API UDogVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDogVoiceComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DogVoice")
	FString AudioBridgeURL = TEXT("http://127.0.0.1:7777");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DogVoice")
	float RecordDuration = 5.0f;

	/** If true, auto-speak LLM responses via TTS. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DogVoice")
	bool bAutoSpeak = true;

	UPROPERTY(BlueprintAssignable, Category = "DogVoice")
	FOnDogVoiceTranscribed OnVoiceTranscribed;

	UFUNCTION(BlueprintCallable, Category = "DogVoice")
	void StartVoiceChat();

	UFUNCTION(BlueprintCallable, Category = "DogVoice")
	void SpeakText(const FString& Text);

	UFUNCTION(BlueprintCallable, Category = "DogVoice")
	bool IsRecording() const;

protected:
	virtual void BeginPlay() override;

private:
	bool bRecording = false;

	UPROPERTY()
	UTrailDogBrain* BrainRef = nullptr;

	UFUNCTION()
	void OnDogDecision(const FTrailDogDecision& Decision);

	void OnSTTResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
