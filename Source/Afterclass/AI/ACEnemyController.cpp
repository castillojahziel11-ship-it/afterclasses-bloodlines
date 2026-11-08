#include "AI/ACEnemyController.h"
#include "Afterclass.h"
#include "AI/ACBlackboardKeys.h"
#include "Characters/ACCharacterBase.h"
#include "Characters/ACCharacterDataAsset.h"
#include "Combat/ACCombatComponent.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"

AACEnemyController::AACEnemyController()
{
	PrimaryActorTick.bCanEverTick = true;
	BehaviorTreeAsset = TSoftObjectPtr<UBehaviorTree>(FSoftObjectPath(TEXT("/Game/AI/BT_Fighter.BT_Fighter")));

	Evaluator = CreateDefaultSubobject<UACCombatEvaluator>(TEXT("CombatEvaluator"));

	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*Perception);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = SightRadius + 200.f;
	SightConfig->PeripheralVisionAngleDegrees = SightHalfAngleDeg;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->SetMaxAge(5.f);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
	HearingConfig->SetMaxAge(3.f);

	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(UAISenseConfig_Sight::StaticClass());
}

void AACEnemyController::SetDifficulty(EACDifficulty InDifficulty)
{
	if (Evaluator) Evaluator->Difficulty = InDifficulty;
}

void AACEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (UAIPerceptionComponent* Perception = GetPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AACEnemyController::HandleTargetPerceptionUpdated);
	}

	UBehaviorTree* BT = BehaviorTreeAsset.LoadSynchronous(); // STUB: async load (KI-006)
	bUsingBehaviorTree = false;
	// The generator creates BT_Fighter with its blackboard but an EMPTY graph (nodes are a manual step), so also require a root node.
	if (BT && BT->BlackboardAsset && BT->RootNode)
	{
		UBlackboardComponent* BB = nullptr;
		if (UseBlackboard(BT->BlackboardAsset, BB) && BB)
		{
			float Preferred = 150.f;
			if (const AACCharacterBase* Char = Cast<AACCharacterBase>(InPawn))
			{
				if (const UACCharacterDataAsset* Data = Char->GetCharacterData()) Preferred = Data->PreferredRange;
			}
			BB->SetValueAsFloat(ACBB::PreferredRange, Preferred);
			BB->SetValueAsFloat(ACBB::OwnHealth, 1.f);
			BB->SetValueAsFloat(ACBB::OwnStamina, 1.f);
			BB->SetValueAsFloat(ACBB::TargetDist, 99999.f);
			BB->SetValueAsVector(ACBB::CoverPoint, InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector);
			bUsingBehaviorTree = RunBehaviorTree(BT);
		}
	}
	if (!bUsingBehaviorTree)
	{
		UE_LOG(LogAfterclass, Warning, TEXT("AI: Behavior Tree '%s' missing, without BB_Fighter, or with an empty graph; %s runs evaluator decisions directly. See Docs/EDITOR_SETUP.md section 3."),
			*BehaviorTreeAsset.ToSoftObjectPath().ToString(), *GetName());
	}
}

void AACEnemyController::OnUnPossess()
{
	if (Evaluator) Evaluator->SetTarget(nullptr);
	Super::OnUnPossess();
}

void AACEnemyController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Evaluator || !Actor || Actor == GetPawn()) return;
	const APawn* OtherPawn = Cast<APawn>(Actor);
	const UACCombatComponent* OtherCombat = Actor->FindComponentByClass<UACCombatComponent>();
	if (!OtherPawn || !OtherCombat || !OtherPawn->IsPlayerControlled()) return; // fighters target players only (for now)

	if (Stimulus.WasSuccessfullySensed())
	{
		if (!Evaluator->GetTarget()) Evaluator->SetTarget(Actor);
		if (UBlackboardComponent* BB = GetBlackboardComponent()) BB->SetValueAsObject(ACBB::TargetActor, Actor);
	}
}

void AACEnemyController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bUsingBehaviorTree || !Evaluator || !HasAuthority()) return;
	FACAIDecision Decision;
	if (Evaluator->ConsumeReadyDecision(Decision)) Evaluator->ExecuteDecision(this, Decision);
}
