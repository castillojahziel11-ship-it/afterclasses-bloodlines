// BT service: copies the evaluator's observed context into the spec §8 blackboard keys (AI/ACBlackboardKeys.h).
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "ACBTService_UpdateCombatBlackboard.generated.h"

UCLASS(meta=(DisplayName="AC Update Combat Blackboard"))
class AFTERCLASS_API UACBTService_UpdateCombatBlackboard : public UBTService
{
	GENERATED_BODY()

public:
	UACBTService_UpdateCombatBlackboard();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
