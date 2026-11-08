#include "AI/ACBTService_UpdateCombatBlackboard.h"
#include "AI/ACBlackboardKeys.h"
#include "AI/ACCombatEvaluator.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UACBTService_UpdateCombatBlackboard::UACBTService_UpdateCombatBlackboard()
{
	NodeName = TEXT("AC Update Combat Blackboard");
	Interval = 0.1f; // matches the evaluator's 100 ms cadence
	RandomDeviation = 0.f;
}

void UACBTService_UpdateCombatBlackboard::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const UACCombatEvaluator* Eval = Controller ? Controller->FindComponentByClass<UACCombatEvaluator>() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Eval || !BB) return;

	const AC::Eval::Context& C = Eval->GetContext();
	BB->SetValueAsObject(ACBB::TargetActor, Eval->GetTarget());
	BB->SetValueAsFloat(ACBB::TargetDist, Eval->GetTarget() ? C.DistanceUU : 99999.f);
	BB->SetValueAsFloat(ACBB::TargetStamina, C.TargetStamina01);
	BB->SetValueAsBool(ACBB::TargetWeapon, C.bTargetHasWeapon);
	BB->SetValueAsInt(ACBB::TargetState, static_cast<int32>(C.Target));
	BB->SetValueAsInt(ACBB::LastPlayerAttack, static_cast<int32>(Eval->GetLastPlayerAttack()));
	BB->SetValueAsFloat(ACBB::AttackPatternBias, Eval->GetAttackPatternBias());
	BB->SetValueAsFloat(ACBB::OwnHealth, C.OwnHealth01);
	BB->SetValueAsFloat(ACBB::OwnStamina, C.OwnStamina01);
	BB->SetValueAsFloat(ACBB::PreferredRange, C.PreferredRangeUU);
	BB->SetValueAsVector(ACBB::CoverPoint, Eval->GetCoverPoint());
}
