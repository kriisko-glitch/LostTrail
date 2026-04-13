// Copyright Kriisko-Studio. Licensed under project terms.

#include "Dog/TrailDogBrain.h"
#include "Translator/TranslatorVocabulary.h"
#include "LostTrail.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UTrailDogBrain::UTrailDogBrain()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTrailDogBrain::BeginPlay()
{
	Super::BeginPlay();
	TimeSincePoll = PollIntervalSeconds; // Fire immediately on first tick

	// Load API key from multiple locations (packaged build compatibility)
	TArray<FString> KeyPaths = {
		FPaths::Combine(FPaths::ProjectDir(), TEXT("groq.key")),          // Game directory
		FPaths::Combine(FPaths::ProjectDir(), TEXT("tools/.groq_key")),   // Dev layout
		FPaths::ConvertRelativePathToFull(ApiKeyFilePath),                 // Configured path
	};

	bool bKeyLoaded = false;
	for (const FString& Path : KeyPaths)
	{
		if (FFileHelper::LoadFileToString(ApiKey, *Path))
		{
			ApiKey.TrimStartAndEndInline();
			if (!ApiKey.IsEmpty())
			{
				UE_LOG(LogLostTrail, Log, TEXT("DogBrain: API key loaded from %s"), *Path);
				bKeyLoaded = true;
				break;
			}
		}
	}

	if (!bKeyLoaded)
	{
		UE_LOG(LogLostTrail, Error, TEXT("DogBrain: No API key found. Place groq.key in game directory."));
	}
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
	PendingState = State;

	const FString UserContent = FormatStateLine(State);
	SendHttpRequest(UserContent);
}

void UTrailDogBrain::SendChat(const FString& PlayerMessage)
{
	if (bRequestInFlight || PlayerMessage.IsEmpty()) return;

	SendHttpRequest(PlayerMessage);
}

void UTrailDogBrain::SendHttpRequest(const FString& UserContent)
{
	bRequestInFlight = true;
	RequestStartSeconds = FPlatformTime::Seconds();

	// Build JSON body using FJsonObject (robust, handles escaping)
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	Root->SetStringField(TEXT("model"), ModelName);
	Root->SetNumberField(TEXT("max_tokens"), MaxTokens);
	Root->SetNumberField(TEXT("temperature"), Temperature);

	TArray<TSharedPtr<FJsonValue>> Messages;

	// System message
	TSharedPtr<FJsonObject> SystemMsg = MakeShareable(new FJsonObject);
	SystemMsg->SetStringField(TEXT("role"), TEXT("system"));
	SystemMsg->SetStringField(TEXT("content"), BuildSystemPrompt());
	Messages.Add(MakeShareable(new FJsonValueObject(SystemMsg)));

	// User message
	TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject);
	UserMsg->SetStringField(TEXT("role"), TEXT("user"));
	UserMsg->SetStringField(TEXT("content"), UserContent);
	Messages.Add(MakeShareable(new FJsonValueObject(UserMsg)));

	Root->SetArrayField(TEXT("messages"), Messages);

	// For local endpoints (Qwen fallback), add chat_template_kwargs
	if (EndpointUrl.Contains(TEXT("127.0.0.1")))
	{
		TSharedPtr<FJsonObject> Kwargs = MakeShareable(new FJsonObject);
		Kwargs->SetBoolField(TEXT("enable_thinking"), false);
		Root->SetObjectField(TEXT("chat_template_kwargs"), Kwargs);
	}

	FString JsonStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	// Build HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(EndpointUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("Kriisko-Studio/1.0"));
	Request->SetTimeout(RequestTimeoutSeconds);

	// Add authorization for cloud endpoints
	if (!ApiKey.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	}

	Request->SetContentAsString(JsonStr);
	Request->OnProcessRequestComplete().BindUObject(this, &UTrailDogBrain::OnHttpComplete);
	Request->ProcessRequest();
}

void UTrailDogBrain::OnHttpComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	bRequestInFlight = false;
	const int32 LatencyMs = (int32)((FPlatformTime::Seconds() - RequestStartSeconds) * 1000.0);

	FTrailDogDecision Decision;
	Decision.LatencyMs = LatencyMs;

	if (!bSuccess || !Response.IsValid() || !EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		// Fallback: keep doing what we were doing, say nothing
		Decision.Action = PendingState.LastAction;
		Decision.SpeakPhrase = TEXT("NONE");
		UE_LOG(LogLostTrail, Warning, TEXT("DogBrain HTTP failed (latency=%dms, code=%d)"),
			LatencyMs, Response.IsValid() ? Response->GetResponseCode() : -1);
		OnDecision.Broadcast(Decision);
		return;
	}

	// Parse outer API response to get content string
	const FString ResponseBody = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonRoot;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

	FString Content;
	if (FJsonSerializer::Deserialize(Reader, JsonRoot) && JsonRoot.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
		if (JsonRoot->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
		{
			const TSharedPtr<FJsonObject>* ChoiceObj;
			if ((*Choices)[0]->TryGetObject(ChoiceObj))
			{
				const TSharedPtr<FJsonObject>* MsgObj;
				if (ChoiceObj->Get()->TryGetObjectField(TEXT("message"), MsgObj))
				{
					MsgObj->Get()->TryGetStringField(TEXT("content"), Content);
					Content.TrimStartAndEndInline();
				}
			}
		}
	}

	Decision.RawResponse = Content;

	// Parse the LLM's JSON response: {"action":"FOLLOW", "speak":"WATER CLOSE", "target":[100,200,0]}
	ETrailDogAction Action;
	FVector Target;
	bool bHasTarget;
	FString Phrase;
	ParseJsonResponse(Content, Action, Target, bHasTarget, Phrase);

	Decision.Action = Action;
	Decision.Target = Target;
	Decision.bHasTarget = bHasTarget;
	Decision.SpeakPhrase = Phrase;

	UE_LOG(LogLostTrail, Log, TEXT("DogBrain decision: %s | SPEAK %s (latency=%dms)"),
		*UEnum::GetValueAsString(Action), *Phrase, LatencyMs);

	OnDecision.Broadcast(Decision);
}

// --- Static helpers ---

FString UTrailDogBrain::StripMarkdownAndExtractJson(const FString& Raw)
{
	FString Clean = Raw;
	Clean.TrimStartAndEndInline();

	// Strip ```json ... ``` wrapper
	if (Clean.StartsWith(TEXT("```")))
	{
		int32 FirstNewline = Clean.Find(TEXT("\n"));
		if (FirstNewline != INDEX_NONE)
		{
			Clean = Clean.Mid(FirstNewline + 1);
		}
		if (Clean.EndsWith(TEXT("```")))
		{
			Clean = Clean.Left(Clean.Len() - 3);
		}
		Clean.TrimStartAndEndInline();
	}

	// Find the JSON object within the response (handle leading/trailing text)
	int32 JsonStart = Clean.Find(TEXT("{"));
	int32 JsonEnd = Clean.Find(TEXT("}"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (JsonStart != INDEX_NONE && JsonEnd != INDEX_NONE && JsonEnd > JsonStart)
	{
		Clean = Clean.Mid(JsonStart, JsonEnd - JsonStart + 1);
	}

	return Clean;
}

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

void UTrailDogBrain::ParseJsonResponse(const FString& Raw, ETrailDogAction& OutAction,
	FVector& OutTarget, bool& bOutHasTarget, FString& OutPhrase)
{
	OutAction = ETrailDogAction::Wait;
	OutTarget = FVector::ZeroVector;
	bOutHasTarget = false;
	OutPhrase = TEXT("NONE");

	if (Raw.IsEmpty()) return;

	// Strip markdown and extract JSON
	const FString CleanJson = StripMarkdownAndExtractJson(Raw);

	// Try to parse as JSON
	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CleanJson);

	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		// Fallback: try to find action keyword in raw text
		UE_LOG(LogLostTrail, Warning, TEXT("DogBrain: Failed to parse JSON: %s"), *CleanJson.Left(100));
		OutAction = KeywordToAction(Raw.ToUpper());
		return;
	}

	// Extract "action" field
	FString ActionStr;
	if (JsonObj->TryGetStringField(TEXT("action"), ActionStr))
	{
		OutAction = KeywordToAction(ActionStr.ToUpper());
	}

	// Extract "speak" field
	FString SpeakStr;
	if (JsonObj->TryGetStringField(TEXT("speak"), SpeakStr))
	{
		SpeakStr.TrimStartAndEndInline();
		OutPhrase = SpeakStr.IsEmpty() ? TEXT("NONE") : SpeakStr;
	}

	// Extract "target" field: [x, y, z] array
	const TArray<TSharedPtr<FJsonValue>>* TargetArray = nullptr;
	if (JsonObj->TryGetArrayField(TEXT("target"), TargetArray) && TargetArray->Num() >= 3)
	{
		OutTarget.X = (*TargetArray)[0]->AsNumber();
		OutTarget.Y = (*TargetArray)[1]->AsNumber();
		OutTarget.Z = (*TargetArray)[2]->AsNumber();
		bOutHasTarget = true;
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
			 "Given the current state, respond with EXACTLY ONE JSON object.\n\n"
			 "RESPOND IN JSON ONLY: {\"action\":\"ACTION\", \"speak\":\"PHRASE\", \"target\":[x,y,z]}\n\n"
			 "Valid actions: FOLLOW, WAIT, WANDER, INVESTIGATE, SCOUT, FLEE, FIGHT, SIT\n"
			 "The target array is optional (only for WANDER/INVESTIGATE/SCOUT/FLEE). Omit it if not needed.\n\n"
			 "%s\n"
			 "Use \"speak\":\"NONE\" if you have nothing to say.\n\n"
			 "If a player command is given (not 'none'), prioritize it. "
			 "COME=FOLLOW, STAY=WAIT, SCOUT=explore ahead, FIND_WATER=INVESTIGATE toward water, "
			 "FIND_SHELTER=INVESTIGATE toward shelter, QUIET=SIT silently.\n\n"
			 "Examples:\n"
			 "{\"action\":\"FOLLOW\", \"speak\":\"GOOD\"}\n"
			 "{\"action\":\"INVESTIGATE\", \"speak\":\"WATER HERE\", \"target\":[500,0,0]}\n"
			 "{\"action\":\"FLEE\", \"speak\":\"DANGER\", \"target\":[-1000,0,0]}\n"
			 "{\"action\":\"WAIT\", \"speak\":\"NONE\"}\n\n"
			 "You are a good dog. You care about your human. Act like it."),
		*PhraseList
	);
}
