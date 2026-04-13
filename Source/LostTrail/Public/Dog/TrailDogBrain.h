// Copyright Kriisko-Studio. Licensed under project terms.
//
// UTrailDogBrain
// --------------
// Evolution of DogCompanion's UDogBrainComponent for the survival context.
// Same core pattern: async HTTP to local Qwen 0.8B, compact state snapshot,
// single-line response. New: the SPEAK channel for translator phrases.
//
// Response format:  ACTION [x,y,z] | SPEAK phrase
// Example:          FOLLOW | SPEAK WATER CLOSE
// Example:          FLEE 100,200,0 | SPEAK SCARED
// Example:          INVESTIGATE 50,0,0 | SPEAK NONE

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "TrailDogBrain.generated.h"

UENUM(BlueprintType)
enum class ETrailDogAction : uint8
{
	Follow       UMETA(DisplayName = "FOLLOW"),
	Wait         UMETA(DisplayName = "WAIT"),
	Wander       UMETA(DisplayName = "WANDER"),
	Investigate  UMETA(DisplayName = "INVESTIGATE"),
	Scout        UMETA(DisplayName = "SCOUT"),
	Flee         UMETA(DisplayName = "FLEE"),
	Fight        UMETA(DisplayName = "FIGHT"),
	Sit          UMETA(DisplayName = "SIT"),
	ParseError   UMETA(DisplayName = "PARSE_ERROR"),
};

USTRUCT(BlueprintType)
struct FTrailDogState
{
	GENERATED_BODY()

	// Player
	UPROPERTY() float DistanceToPlayerCm = 0.f;
	UPROPERTY() bool  bPlayerIsMoving = false;
	UPROPERTY() float PlayerBearingDeg = 0.f;

	// Dog self
	UPROPERTY() ETrailDogAction LastAction = ETrailDogAction::Wait;
	UPROPERTY() int32 LastActionAgeMs = 0;
	UPROPERTY() float DogEnergy = 1.f;
	UPROPERTY() float DogHealthNormalized = 1.f;

	// Environment
	UPROPERTY() FString NearestPoiKind = TEXT("none");   // water|food|shelter|predator|scent|none
	UPROPERTY() float NearestPoiDistanceCm = -1.f;
	UPROPERTY() bool  bPredatorNearby = false;
	UPROPERTY() int32 PredatorCount = 0;
	UPROPERTY() bool  bNoiseNearby = false;

	// Time / weather
	UPROPERTY() FString TimeOfDay = TEXT("day");          // dawn|day|dusk|night
	UPROPERTY() bool  bIsRaining = false;

	// Player survival (dog should react to player's distress)
	UPROPERTY() float PlayerHungerNormalized = 1.f;
	UPROPERTY() float PlayerThirstNormalized = 1.f;
	UPROPERTY() float PlayerHealthNormalized = 1.f;

	// Player command override (from translator)
	UPROPERTY() FString PlayerCommand = TEXT("none");     // none|COME|STAY|SCOUT|FIND_WATER|FIND_SHELTER|QUIET
};

USTRUCT(BlueprintType)
struct FTrailDogDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) ETrailDogAction Action = ETrailDogAction::Wait;
	UPROPERTY(BlueprintReadOnly) bool bHasTarget = false;
	UPROPERTY(BlueprintReadOnly) FVector Target = FVector::ZeroVector;

	/** The phrase the dog "said" this tick. Empty or "NONE" if silent. */
	UPROPERTY(BlueprintReadOnly) FString SpeakPhrase;

	UPROPERTY(BlueprintReadOnly) int32 LatencyMs = 0;
	UPROPERTY(BlueprintReadOnly) FString RawResponse;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrailDogDecision, const FTrailDogDecision&, Decision);

UCLASS(ClassGroup=(LostTrail), meta=(BlueprintSpawnableComponent))
class LOSTTRAIL_API UTrailDogBrain : public UActorComponent
{
	GENERATED_BODY()

public:
	UTrailDogBrain();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DogBrain")
	FString EndpointUrl = TEXT("http://127.0.0.1:11001/v1/chat/completions");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DogBrain")
	float PollIntervalSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DogBrain")
	float RequestTimeoutSeconds = 2.f;

	/** Current translator level — affects which phrases appear in the system prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DogBrain")
	int32 TranslatorLevel = 0;

	UPROPERTY(BlueprintAssignable, Category="DogBrain")
	FOnTrailDogDecision OnDecision;

	UFUNCTION(BlueprintCallable, Category="DogBrain")
	void RequestDecision(const FTrailDogState& State);

	/** Parse "ACTION [x,y,z] | SPEAK phrase" format. */
	static void ParseResponse(const FString& Raw, ETrailDogAction& OutAction,
		FVector& OutTarget, bool& bOutHasTarget, FString& OutPhrase);

	static FString FormatStateLine(const FTrailDogState& State);

	FString BuildSystemPrompt() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;

private:
	bool bRequestInFlight = false;
	float TimeSincePoll = 0.f;
	double RequestStartSeconds = 0.0;
	FTrailDogState PendingState;

	void OnHttpComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	static ETrailDogAction KeywordToAction(const FString& Word);
};
