// Copyright Kriisko-Studio. Licensed under project terms.

#include "Dog/DogVoiceComponent.h"
#include "Dog/TrailDogBrain.h"
#include "LostTrail.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UDogVoiceComponent::UDogVoiceComponent()
{
	AudioBridgeURL = TEXT("http://127.0.0.1:7777");
	RecordDuration = 5.0f;
	bAutoSpeak = true;
	bRecording = false;
	BrainRef = nullptr;
}

void UDogVoiceComponent::BeginPlay()
{
	Super::BeginPlay();

	BrainRef = GetOwner()->FindComponentByClass<UTrailDogBrain>();
	if (BrainRef && bAutoSpeak)
	{
		BrainRef->OnDecision.AddDynamic(this, &UDogVoiceComponent::OnDogDecision);
	}
}

void UDogVoiceComponent::StartVoiceChat()
{
	if (bRecording) return;
	bRecording = true;

	UE_LOG(LogLostTrail, Log, TEXT("DogVoice: Recording started (%.1fs)"), RecordDuration);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(AudioBridgeURL + TEXT("/stt"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetNumberField(TEXT("duration"), RecordDuration);
	FString JsonStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(JsonStr);
	Request->OnProcessRequestComplete().BindUObject(this, &UDogVoiceComponent::OnSTTResponseReceived);
	Request->ProcessRequest();
}

void UDogVoiceComponent::OnSTTResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	bRecording = false;

	if (bWasSuccessful && Response.IsValid())
	{
		FString JsonStr = Response->GetContentAsString();
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);

		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			FString TranscribedText;
			if (JsonObject->TryGetStringField(TEXT("text"), TranscribedText) && !TranscribedText.IsEmpty())
			{
				UE_LOG(LogLostTrail, Log, TEXT("DogVoice: Transcribed: %s"), *TranscribedText);

				if (BrainRef)
				{
					BrainRef->SendChat(TranscribedText);
				}
				OnVoiceTranscribed.Broadcast(TranscribedText);
			}
		}
	}
	else
	{
		UE_LOG(LogLostTrail, Warning, TEXT("DogVoice: STT failed"));
	}
}

void UDogVoiceComponent::SpeakText(const FString& Text)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(AudioBridgeURL + TEXT("/tts"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("text"), Text);
	FString JsonStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(JsonStr);
	Request->ProcessRequest();
}

void UDogVoiceComponent::OnDogDecision(const FTrailDogDecision& Decision)
{
	// Only speak when there's a meaningful speak phrase (not NONE, not empty)
	// and ONLY for direct chat responses (not automatic decisions)
	// Translator phrases are visual-only (lesson from NeonPatrol)
	if (bAutoSpeak
		&& !Decision.SpeakPhrase.IsEmpty()
		&& !Decision.SpeakPhrase.Equals(TEXT("NONE"), ESearchCase::IgnoreCase))
	{
		// Only speak multi-word phrases that sound like conversation
		// Short vocabulary phrases (DANGER, GOOD, etc.) are visual only
		if (Decision.SpeakPhrase.Contains(TEXT(" ")))
		{
			SpeakText(Decision.SpeakPhrase);
		}
	}
}

bool UDogVoiceComponent::IsRecording() const
{
	return bRecording;
}
