// Copyright Kriisko-Studio. Licensed under project terms.

#include "Dog/TrailDogBrain.h"
#include "Translator/TranslatorVocabulary.h"
#include "LostTrail.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

UTrailDogBrain::UTrailDogBrain()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTrailDogBrain::BeginPlay()
{
	Super::BeginPlay();
	TimeSincePoll = PollIntervalSeconds; // Fire immediately on first tick
}

void UTrailDogBrain::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TimeSincePoll += DeltaTime;
	// Actual polling is driven by the owning character calling RequestDecision
}

void UTrailDogBrain::RequestDecision(const FTrailDogState& State)
{
	if (bRequestInFlight) return;
	if (TimeSincePoll < PollIntervalSeconds) return;

	TimeSincePoll = 0.f;
	bRequestInFlight = true;
	PendingState = State;
	RequestStartSeconds = FPlatformTime::Seconds();

	// Build HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(EndpointUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetTimeout(RequestTimeoutSeconds);

	// Build JSON body
	const FString SystemPrompt = BuildSystemPrompt();
	const FString UserPrompt = FormatStateLine(State);

	// Manual JSON construction (avoids FJsonObject overhead for a simple structure)
	const FString Body = FString::Printf(
		TEXT("{\"model\":\"qwen\",\"max_tokens\":40,\"temperature\":0.7,\"messages\":["
			 "{\"role\":\"system\",\"content\":\"%s\"},"
			 "{\"role\":\"user\",\"content\":\"%s\"}"
			 "]}"),
		*SystemPrompt.ReplaceCharWithEscapedChar(),
		*UserPrompt.ReplaceCharWithEscapedChar()
	);

	Request->SetContentAsString(Body);
	Request->OnProcessRequestComplete().BindUObject(this, &UTrailDogBrain::OnHttpComplete);
	Request->ProcessRequest();
}

void UTrailDogBrain::OnHttpComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	bRequestInFlight = false;
	const int32 LatencyMs = (int32)((FPlatformTime::Seconds() - RequestStartSeconds) * 1000.0);

	FTrailDogDecision Decision;
	Decision.LatencyMs = LatencyMs;

	if (!bSuccess || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		// Fallback: keep doing what we were doing, say nothing
		Decision.Action = PendingState.LastAction;
		Decision.SpeakPhrase = TEXT("NONE");
		UE_LOG(LogLostTrail, Warning, TEXT("DogBrain HTTP failed (latency=%dms)"), LatencyMs);
		OnDecision.Broadcast(Decision);
		return;
	}

	// Parse JSON response to get the content string
	const FString ResponseBody = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonRoot;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

	FString Content;
	if (FJsonSerializer::Deserialize(Reader, JsonRoot) && JsonRoot.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
		if (JsonRoot->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
		{
			TSharedPtr<FJsonObject> Msg = (*Choices)[0]->AsObject()->GetObjectField(TEXT("message"));
			if (Msg.IsValid())
			{
				Content = Msg->GetStringField(TEXT("content"));
			}
		}
	}

	Decision.RawResponse = Content;

	// Parse the response: ACTION [x,y,z] | SPEAK phrase
	ETrailDogAction Action;
	FVector Target;
	bool bHasTarget;
	FString Phrase;
	ParseResponse(Content, Action, Target, bHasTarget, Phrase);

	Decision.Action = Action;
	Decision.Target = Target;
	Decision.bHasTarget = bHasTarget;
	Decision.SpeakPhrase = Phrase;

	UE_LOG(LogLostTrail, Log, TEXT("DogBrain decision: %s | SPEAK %s (latency=%dms)"),
		*UEnum::GetValueAsString(Action), *Phrase, LatencyMs);

	OnDecision.Broadcast(Decision);
}

// --- Static helpers ---

ETrailDogAction UTrailDogBrain::KeywordToAction(const FString& Word)
{
	if (Word.Contains(TEXT("FOLLOW")))      return ETrailDogAction::Follow;
	if (Word.Contains(TEXT("WAIT")))        return ETrailDogAction::Wait;
	if (Word.Contains(TEXT("WANDER")))      return ETrailDogAction::Wander;
	if (Word.Contains(TEXT("INVESTIGATE"))) return ETrailDogAction::Investigate;
	if (Word.Contains(TEXT("SCOUT")))       return ETrailDogAction::Scout;
	if (Word.Contains(TEXT("FLEE")))        return ETrailDogAction::Flee;
	if (Word.Contains(TEXT("FIGHT")))       return ETrailDogAction::Fight;
	if (Word.Contains(TEXT("SIT")))         return ETrailDogAction::Sit;
	return ETrailDogAction::ParseError;
}

void UTrailDogBrain::ParseResponse(const FString& Raw, ETrailDogAction& OutAction,
	FVector& OutTarget, bool& bOutHasTarget, FString& OutPhrase)
{
	OutAction = ETrailDogAction::Wait;
	OutTarget = FVector::ZeroVector;
	bOutHasTarget = false;
	OutPhrase = TEXT("NONE");

	if (Raw.IsEmpty()) return;

	// Split on "|" to get action part and speak part
	FString ActionPart;
	FString SpeakPart;

	int32 PipeIdx;
	if (Raw.FindChar('|', PipeIdx))
	{
		ActionPart = Raw.Left(PipeIdx).TrimStartAndEnd();
		SpeakPart = Raw.Mid(PipeIdx + 1).TrimStartAndEnd();
	}
	else
	{
		ActionPart = Raw.TrimStartAndEnd();
	}

	// Parse action: first word is the action keyword
	const FString ActionUpper = ActionPart.ToUpper();
	OutAction = KeywordToAction(ActionUpper);

	// Try to parse coordinates: look for x,y,z pattern
	FRegexPattern CoordPattern(TEXT("(-?[\\d.]+)\\s*,\\s*(-?[\\d.]+)\\s*,\\s*(-?[\\d.]+)"));
	FRegexMatcher Matcher(CoordPattern, ActionPart);
	if (Matcher.FindNext())
	{
		OutTarget.X = FCString::Atof(*Matcher.GetCaptureGroup(1));
		OutTarget.Y = FCString::Atof(*Matcher.GetCaptureGroup(2));
		OutTarget.Z = FCString::Atof(*Matcher.GetCaptureGroup(3));
		bOutHasTarget = true;
	}

	// Parse speak: strip "SPEAK" prefix
	if (!SpeakPart.IsEmpty())
	{
		FString Phrase = SpeakPart;
		if (Phrase.ToUpper().StartsWith(TEXT("SPEAK")))
		{
			Phrase = Phrase.Mid(5).TrimStartAndEnd();
		}
		OutPhrase = Phrase.IsEmpty() ? TEXT("NONE") : Phrase;
	}
}

FString UTrailDogBrain::FormatStateLine(const FTrailDogState& State)
{
	return FString::Printf(
		TEXT("dist=%dcm moving=%s bearing=%ddeg "
			 "last=%s age=%dms energy=%.2f hp=%.2f "
			 "poi=%s@%dcm predator=%s(%d) noise=%s "
			 "time=%s rain=%s "
			 "player_hunger=%.2f player_thirst=%.2f player_hp=%.2f "
			 "command=%s"),
		(int32)State.DistanceToPlayerCm,
		State.bPlayerIsMoving ? TEXT("true") : TEXT("false"),
		(int32)State.PlayerBearingDeg,
		*UEnum::GetValueAsString(State.LastAction),
		State.LastActionAgeMs,
		State.DogEnergy,
		State.DogHealthNormalized,
		*State.NearestPoiKind,
		(int32)State.NearestPoiDistanceCm,
		State.bPredatorNearby ? TEXT("true") : TEXT("false"),
		State.PredatorCount,
		State.bNoiseNearby ? TEXT("true") : TEXT("false"),
		*State.TimeOfDay,
		State.bIsRaining ? TEXT("true") : TEXT("false"),
		State.PlayerHungerNormalized,
		State.PlayerThirstNormalized,
		State.PlayerHealthNormalized,
		*State.PlayerCommand
	);
}

FString UTrailDogBrain::BuildSystemPrompt() const
{
	const FString PhraseList = UTranslatorVocabulary::BuildPromptPhraseList(TranslatorLevel);

	return FString::Printf(
		TEXT("You are the brain of a loyal dog in a survival game. Your human is lost in a forest. "
			 "You sense things they cannot: predators, water, food, shelter. "
			 "Given the current state, respond with EXACTLY ONE LINE in this format:\n"
			 "ACTION [x,y,z] | SPEAK phrase\n\n"
			 "Valid actions: FOLLOW, WAIT, WANDER, INVESTIGATE, SCOUT, FLEE, FIGHT, SIT\n"
			 "Coordinates are optional (only for WANDER/INVESTIGATE/SCOUT/FLEE).\n"
			 "%s\n"
			 "Use SPEAK NONE if you have nothing to say.\n\n"
			 "If a player command is given (not 'none'), prioritize it. "
			 "COME=FOLLOW, STAY=WAIT, SCOUT=explore ahead, FIND_WATER=INVESTIGATE toward water, "
			 "FIND_SHELTER=INVESTIGATE toward shelter, QUIET=SIT silently.\n\n"
			 "You are a good dog. You care about your human. Act like it."),
		*PhraseList
	);
}
