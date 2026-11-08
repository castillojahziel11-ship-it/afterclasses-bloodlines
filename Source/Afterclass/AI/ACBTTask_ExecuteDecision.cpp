#include "AI/ACBTTask_ExecuteDecision.h"
#include "AI/ACCombatEvaluator.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UACBTTask_ExecuteDecision::UACBTTask_ExecuteDecision()
{
	NodeName = TEXT("AC Execute Combat Decision");
	bNotifyTick = true;
}

uint16 UACBTTask_ExecuteDecision::GetInstanceMemorySize() const
{
	return sizeof(FTaskMemory);
}

bool UACBTTask_ExecuteDecision::TryExecute(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	UACCombatEvaluator* Eval = Controller ? Controller->FindComponentByClass<UACCombatEvaluator>() : nullptr;
	if (!Eval) return false;
	FACAIDecision Decision;
	if (!Eval->ConsumeReadyDecision(Decision)) return false;
	Eval->ExecuteDecision(Controller, Decision);
	return true;
}

EBTNodeResult::Type UACBTTask_ExecuteDecision::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller || !Controller->FindComponentByClass<UACCombatEvaluator>()) return EBTNodeResult::Failed;
	reinterpret_cast<FTaskMemory*>(NodeMemory)->Waited = 0.f;
	return TryExecute(OwnerComp) ? EBTNodeResult::Succeeded : EBTNodeResult::InProgress;
}

void UACBTTask_ExecuteDecision::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FTaskMemory* Mem = reinterpret_cast<FTaskMemory*>(NodeMemory);
	if (TryExecute(OwnerComp))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	Mem->Waited += DeltaSeconds;
	if (Mem->Waited >= MaxWaitSeconds) FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
}

FString UACBTTask_ExecuteDecision::GetStaticDescription() const
{
	return FString::Printf(TEXT("Execute UACCombatEvaluator decision (timeout %.2fs)"), MaxWaitSeconds);
}
