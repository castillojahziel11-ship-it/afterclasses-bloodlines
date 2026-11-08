// Boss phases (spec §8): phase 2 at 66 % health, phase 3 at 33 %; exactly one telegraphed unblockable per phase.
// Phase logic is AC::Eval::BossPhaseTracker (unit-tested). The unblockable replaces one Heavy decision per phase:
// telegraph (multicast, for VFX/VO/audio sting) -> TelegraphSeconds later the boss starts UnblockableMove.
// Not compiled in the authoring environment (no UE install; KI-002).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/ACEvaluatorCore.h"
#include "Combat/ACCombatTypes.h"
#include "ACBossPhaseComponent.generated.h"

class UACCombatComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FACOnBossPhaseChanged, int32, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FACOnBossTelegraph, int32, Phase, float, TelegraphSeconds);

UCLASS(ClassGroup=(Afterclass), meta=(BlueprintSpawnableComponent))
class AFTERCLASS_API UACBossPhaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACBossPhaseComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Server: if this phase's unblockable is unused and the boss is free, starts its telegraph. */
	bool TryTelegraphUnblockable();

	UFUNCTION(BlueprintPure, Category="Afterclass|Boss") int32 GetPhase() const { return Tracker.Phase; }
	UFUNCTION(BlueprintPure, Category="Afterclass|Boss") bool IsUnblockableReady() const { return Tracker.UnblockableReady; }

	UPROPERTY(BlueprintAssignable, Category="Afterclass|Boss") FACOnBossPhaseChanged OnPhaseChanged;
	/** Fires on every machine when the unblockable is telegraphed (flash / VO / sting hook). */
	UPROPERTY(BlueprintAssignable, Category="Afterclass|Boss") FACOnBossTelegraph OnTelegraph;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Afterclass|Boss", meta=(Units="s")) float TelegraphSeconds = 0.75f;
	/** The unblockable attack. Defaults: heavy timeline, bUnblockable, high damage. Override per boss in the BP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Afterclass|Boss") FACMoveRow UnblockableMove;

protected:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastTelegraph(int32 Phase, float Seconds);

private:
	UACCombatComponent* GetCombat() const;

	AC::Eval::BossPhaseTracker Tracker;
	double TelegraphEndTime = -1.0;
};
