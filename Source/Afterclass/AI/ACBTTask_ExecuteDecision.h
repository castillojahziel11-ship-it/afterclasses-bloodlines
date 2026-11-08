// BT task: waits (InProgress) until the controller's UACCombatEvaluator has a decision whose reaction latency has
// elapsed, executes it, then succeeds. Fails after MaxWaitSeconds so the tree can re-plan.
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ACBTTask_ExecuteDecision.generated.h"

UCLASS(meta=(DisplayName="AC Execute Combat Decision"))
class AFTERCLASS_API UACBTTask_ExecuteDecision : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UACBTTask_ExecuteDecision();

	UPROPERTY(EditAnywhere, Category="AI", meta=(Units="s", ClampMin="0.1")) float MaxWaitSeconds = 1.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

private:
	struct FTaskMemory { float Waited = 0.f; };
	static bool TryExecute(UBehaviorTreeComponent& OwnerComp);
};
